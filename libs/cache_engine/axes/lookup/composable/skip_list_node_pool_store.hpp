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

#include "skip_list_node_pool_concept.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

/// Index-stabiler Skip-Listen-Pool: Knoten behalten ihren Index; Erase setzt nur das live-Flag (Tombstone,
/// unverlinkt → unerreichbar). RNG zieht die Knoten-Hoehe (Substrat-Verantwortung Pool-Wachstum).
class SkipListNodePoolStore {
public:
    using key_type                         = std::uint64_t;
    using value_type                       = std::uint64_t;
    static constexpr int         kMaxLevel = 16;
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

    /// Allokiert einen live Knoten mit `level` Forward-Slots (alle kNil) — darf via vector werfen (kein noexcept).
    std::size_t allocate_node(key_type k, value_type v, int level) {
        std::size_t const idx = nodes_.size();
        nodes_.push_back(Node{k, v, true, std::vector<std::size_t>(static_cast<std::size_t>(level), kNil)});
        ++live_count_;
        return idx;
    }
    /// Muenzwurf-Level-Ziehung (P=0.5) — verbatim random_level(), mutiert rng_ (deshalb im Store).
    [[nodiscard]] int draw_level() noexcept {
        int lvl = 1;
        while ((rng_() & 1u) != 0u && lvl < kMaxLevel) ++lvl;
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
    struct Node {
        key_type                 key{};
        value_type               val{};
        bool                     live{};
        std::vector<std::size_t> next{}; // Forward-Indizes je Level (kNil = Ende)
    };

    void init_head() {
        // Head-Sentinel (Index 0): kMaxLevel Forward-Slots, alle kNil.
        nodes_.push_back(
            Node{key_type{}, value_type{}, false, std::vector<std::size_t>(static_cast<std::size_t>(kMaxLevel), kNil)});
    }

    std::vector<Node>       nodes_{};
    std::size_t             live_count_ = 0;
    int                     level_      = 1;
    mutable std::mt19937_64 rng_;
};

// Selbstbeweis: das Substrat erfuellt das SkipListNodePool-Concept.
static_assert(SkipListNodePool<SkipListNodePoolStore>);

} // namespace comdare::cache_engine::lookup::composable
