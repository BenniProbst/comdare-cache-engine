#pragma once
// V41 Umstufung-A (Task #41) — SkipListNodePoolStore: verzeigertes Skip-Listen-Substrat (erfuellt SkipListNodePool).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Reines Substrat OHNE Such-Logik — 1:1-Port der Knoten-Struktur aus axis_03a_search_algo_skip_list.hpp
// (Node{key,val,live,next[]}; nodes_; live_count_; level_=1; mt19937_64 rng_{0xC0FFEEu}; init_head;
// random_level Muenzwurf P=0.5; clear allokationsfrei), aber generisch ueber uint64-Key. Die Multi-Level-
// Walk-/Verkettungs-Navigation lebt im SkipListTraversalOrgan, NICHT hier (genetisches Experiment, Doku 14 §1.2).
//
// RNG-im-Store (NICHT im stateless Organ): random_level() mutiert rng_ → ist hier `draw_level()`. allocate_node
// erzeugt einen live Knoten (++live_count_); das Organ verkettet ihn anschliessend ueber set_forward_at.
// Deterministischer Seed 0xC0FFEEu → reproduzierbar/testbar (Aequivalenz zum Monolith). Forward-Indizes als
// std::size_t (das Monolith-uint32-Detail ist messungsirrelevant — Skip-Liste-Merkmal ist supports_range_scan).
// Seit #234-F2 zieht draw_level() P=Shape::kPNumerator/Shape::kPDenominator maskenbasiert und haelt P=1/2 bit-treu.

#include "skip_list_node_pool_concept.hpp"
#include <topics/nodes/axis_skip_list_shape/axis_skip_list_shape_max16_p50.hpp>
#include <topics/nodes/axis_skip_list_shape/concepts/axis_skip_list_shape_concept.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <random>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

namespace detail {

template <class FwdAlloc = std::allocator<std::size_t>>
struct SkipListNodePoolNode {
    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;

    key_type                           key{};
    value_type                         val{};
    bool                               live{};
    std::vector<std::size_t, FwdAlloc> next{}; // Forward-Indizes je Level (kNil = Ende)
};

} // namespace detail

/// Index-stabiler Skip-Listen-Pool: Knoten behalten ihren Index; Erase setzt nur das live-Flag (Tombstone,
/// unverlinkt → unerreichbar). RNG zieht die Knoten-Hoehe (Substrat-Verantwortung Pool-Wachstum).
template <typename Shape = ::comdare::cache_engine::nodes::axis_skip_list_shape::SkipListMax16P50,
          class A        = std::allocator<detail::SkipListNodePoolNode<>>>
class SkipListNodePoolStore {
    static_assert(::comdare::cache_engine::nodes::axis_skip_list_shape::concepts::SkipListShape<Shape>);
    static_assert((Shape::kPDenominator & (Shape::kPDenominator - 1)) == 0,
                  "#234-F2: kPDenominator muss Power-of-2 sein (maskenbasierte Ziehung)");
    static_assert(Shape::kPNumerator >= 1 && Shape::kPNumerator < Shape::kPDenominator);

public:
    using key_type                         = std::uint64_t;
    using value_type                       = std::uint64_t;
    using forward_allocator_type           = typename std::allocator_traits<A>::template rebind_alloc<std::size_t>;
    using node_type                        = detail::SkipListNodePoolNode<forward_allocator_type>;
    using allocator_type                   = A;
    using node_allocator_type              = typename std::allocator_traits<A>::template rebind_alloc<node_type>;
    static constexpr int         kMaxLevel = Shape::kMaxLevel; // Level-0: 16 (#234-K shape carrier)
    static constexpr std::size_t kNil      = std::numeric_limits<std::size_t>::max(); // "kein Nachfolger"
    static constexpr std::size_t kHead     = 0;                                       // Sentinel-Kopf-Index

    SkipListNodePoolStore() : rng_(0xC0FFEEu) { init_head(); }

    [[nodiscard]] std::size_t head() const noexcept { return kHead; }
    [[nodiscard]] int         list_level() const noexcept { return level_; }
    [[nodiscard]] std::size_t live_count() const noexcept { return live_count_; }
    [[nodiscard]] key_type    node_key(std::size_t i) const noexcept { return nodes_[i].key; }
    [[nodiscard]] value_type  node_value(std::size_t i) const noexcept { return nodes_[i].val; }
    [[nodiscard]] bool        node_live(std::size_t i) const noexcept { return nodes_[i].live; }
    [[nodiscard]] std::size_t forward_at(std::size_t node, std::size_t level) const noexcept {
        return nodes_[node].next[level];
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    struct allocator_statistics_snapshot {
        std::uint64_t alloc_calls     = 0;
        std::uint64_t bytes_allocated = 0;
        std::uint64_t live_nodes      = 0;
    };

    [[nodiscard]] allocator_statistics_snapshot store_allocator_statistics() const noexcept {
        return allocator_statistics_snapshot{
            alloc_calls_,
            bytes_allocated_,
            live_count_,
        };
    }
#endif

    /// Allokiert einen live Knoten mit `level` Forward-Slots (alle kNil) — darf via vector werfen (kein noexcept).
    std::size_t allocate_node(key_type k, value_type v, int level) {
        std::size_t const idx          = nodes_.size();
        std::size_t const old_capacity = nodes_.capacity();
        nodes_.push_back(node_type{
            k,
            v,
            true,
            std::vector<std::size_t, forward_allocator_type>(static_cast<std::size_t>(level), kNil),
        });
        record_capacity_growth_(old_capacity, nodes_.capacity(), sizeof(node_type));
        record_tower_allocation_(static_cast<std::size_t>(level));
        ++live_count_;
        return idx;
    }
    /// Muenzwurf-Level-Ziehung (P = Shape::kPNumerator/kPDenominator; Level-0: 0.5) — mutiert rng_ (deshalb im Store).
    [[nodiscard]] int draw_level() noexcept {
        int lvl = 1;
        // #234-F2: P = kPNumerator/kPDenominator, kPDenominator MUSS Power-of-2 (maskenbasiert, kein Modulo-Bias).
        // Weiter-Wuerfeln solange der Draw in den obersten kPNumerator Restklassen liegt — fuer 1/2 ist
        // ((rng_() & 1u) >= 1u) EXAKT das alte ((rng_() & 1u) != 0u): gleicher RNG-Konsum, gleiches Praedikat.
        while ((rng_() & (static_cast<std::uint64_t>(Shape::kPDenominator) - 1u)) >=
                   static_cast<std::uint64_t>(Shape::kPDenominator - Shape::kPNumerator) &&
               lvl < kMaxLevel)
            ++lvl;
        return lvl;
    }
    void set_forward_at(std::size_t node, std::size_t level, std::size_t target) noexcept {
        nodes_[node].next[level] = target;
    }
    void set_node_value(std::size_t i, value_type v) noexcept { nodes_[i].val = v; }
    void set_node_live(std::size_t i, bool b) noexcept { nodes_[i].live = b; }
    void set_list_level(int lvl) noexcept { level_ = lvl; }
    void dec_live() noexcept { --live_count_; }

    void clear() noexcept {
        // Allokationsfrei: nur den Head behalten + seine Forward-Slots auf kNil zuruecksetzen.
        nodes_.resize(1);
        for (auto& slot : nodes_[kHead].next) slot = kNil;
        live_count_ = 0;
        level_      = 1;
    }

private:
    void init_head() {
        // Head-Sentinel (Index 0): kMaxLevel Forward-Slots, alle kNil.
        std::size_t const old_capacity = nodes_.capacity();
        nodes_.push_back(node_type{
            key_type{},
            value_type{},
            false,
            std::vector<std::size_t, forward_allocator_type>(static_cast<std::size_t>(kMaxLevel), kNil),
        });
        record_capacity_growth_(old_capacity, nodes_.capacity(), sizeof(node_type));
        record_tower_allocation_(static_cast<std::size_t>(kMaxLevel));
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    // Ehrliche Allokator-Metrik: gezaehlt werden nur erfolgreiche vector-capacity-Zuwaechse, als Capacity-Delta
    // mal Elementgroesse. Reuse/clear ohne Capacity-Wachstum erzeugt bewusst keine kuenstlichen Werte.
    void record_capacity_growth_(std::size_t old_capacity, std::size_t new_capacity, std::size_t elem_bytes) noexcept {
        if (new_capacity <= old_capacity) return;
        ++alloc_calls_;
        bytes_allocated_ +=
            static_cast<std::uint64_t>(new_capacity - old_capacity) * static_cast<std::uint64_t>(elem_bytes);
    }

    void record_tower_allocation_(std::size_t level) noexcept {
        ++alloc_calls_;
        bytes_allocated_ += static_cast<std::uint64_t>(level) * static_cast<std::uint64_t>(sizeof(std::size_t));
    }
#else
    static void record_capacity_growth_(std::size_t, std::size_t, std::size_t) noexcept {}
    static void record_tower_allocation_(std::size_t) noexcept {}
#endif

    std::vector<node_type, node_allocator_type> nodes_{};
    std::size_t                                 live_count_ = 0;
    int                                         level_      = 1;
    mutable std::mt19937_64                     rng_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    std::uint64_t alloc_calls_     = 0;
    std::uint64_t bytes_allocated_ = 0;
#endif
};

// Selbstbeweis: das Substrat erfuellt das SkipListNodePool-Concept.
static_assert(SkipListNodePool<SkipListNodePoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
