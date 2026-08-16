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
//
// A8-S5 Familie 01a (2026-08-04): der Knoten-Speicher kommt REAL aus der Allocator-Achse (axis_06), Muster
// BTreeNodePoolStore. BESONDERHEIT dieser Familie: der Knoten traegt SELBST einen Container (der Forward-Turm
// std::vector<std::size_t>) -- die Bindung muss deshalb auf BEIDEN Ebenen liegen (aeusserer Knoten-Vektor UND
// innerer Turm), sonst allozierte der Turm still am Default-Allokator vorbei. Weil der StdAllocatorAdapter
// &allocator_ haelt, rebindet die COW-Kopie BEIDE Ebenen explizit (copy_nodes_from_) und verwirft die
// Kopier-Pollution per restore_statistics-Memento; Move ist nicht deklariert -> degradiert sicher zu Copy.

#include "skip_list_node_pool_concept.hpp"
#include <organ_axes/alloc/axis_06_allocator_exgen.hpp>            // axis_06-Default-Strategie + StdAllocatorAdapter
#include <organ_axes/alloc/concepts/axis_06_allocator_concept.hpp> // AllocatorStrategy-Concept (compile-time-strikt)
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

/// A8-S5: KEIN Default-Template-Argument mehr -- der Forward-Turm MUSS mit dem Achsen-Adapter des
/// besitzenden Stores parametriert werden (ein Default waere genau die stille Default-Allokator-Bindung,
/// die der Schnitt abbaut). Der Adapter ist nicht default-konstruierbar; jeder Turm wird deshalb explizit
/// mit `allocator_.as_std_allocator<std::size_t>()` gebaut (aggregate-Init, nie ueber `next{}`).
template <class FwdAlloc>
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
          class Alloc    = ::comdare::cache_engine::alloc::ExgenAllocator>
    requires ::comdare::cache_engine::alloc::concepts::AllocatorStrategy<Alloc>
class SkipListNodePoolStore {
    static_assert(::comdare::cache_engine::nodes::axis_skip_list_shape::concepts::SkipListShape<Shape>);
    static_assert((Shape::kPDenominator & (Shape::kPDenominator - 1)) == 0,
                  "#234-F2: kPDenominator muss Power-of-2 sein (maskenbasierte Ziehung)");
    static_assert(Shape::kPNumerator >= 1 && Shape::kPNumerator < Shape::kPDenominator);

public:
    using key_type                         = std::uint64_t;
    using value_type                       = std::uint64_t;
    using allocator_type                   = Alloc;
    using forward_allocator_type           = typename Alloc::template StdAllocatorAdapter<std::size_t>;
    using forward_vector_type              = std::vector<std::size_t, forward_allocator_type>; // der Turm IM Knoten
    using node_type                        = detail::SkipListNodePoolNode<forward_allocator_type>;
    using node_allocator_type              = typename Alloc::template StdAllocatorAdapter<node_type>;
    static constexpr int         kMaxLevel = Shape::kMaxLevel; // Level-0: 16 (#234-K shape carrier)
    static constexpr std::size_t kNil      = std::numeric_limits<std::size_t>::max(); // "kein Nachfolger"
    static constexpr std::size_t kHead     = 0;                                       // Sentinel-Kopf-Index

    SkipListNodePoolStore() : nodes_(allocator_.template as_std_allocator<node_type>()), rng_(0xC0FFEEu) {
        init_head();
    }
    // COW-Pflicht (Memento): allocator_ mitkopieren, dann die Knoten EINZELN uebernehmen -- copy_nodes_from_
    // rebindet den aeusseren Knoten-Vektor UND jeden inneren Forward-Turm an DAS EIGENE allocator_. Ein
    // blosses `nodes_(o.nodes_, ...)` haette nur die aeussere Ebene rebindet und die Tuerme der Kopie weiter
    // aus dem Allokator der QUELLE bedient (stille Cross-Strategy-Allokation + dangling &allocator_).
    SkipListNodePoolStore(SkipListNodePoolStore const& o)
        : allocator_(o.allocator_), nodes_(allocator_.template as_std_allocator<node_type>()),
          live_count_(o.live_count_), level_(o.level_), rng_(o.rng_) {
        copy_nodes_from_(o.nodes_);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        allocator_.restore_statistics(o.allocator_.statistics()); // Kopier-Pollution verwerfen
#endif
    }
    SkipListNodePoolStore& operator=(SkipListNodePoolStore const& o) {
        if (this != &o) {
            nodes_.clear();
            copy_nodes_from_(o.nodes_); // Tuerme erneut an DAS EIGENE allocator_ gebunden
            live_count_ = o.live_count_;
            level_      = o.level_;
            rng_        = o.rng_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~SkipListNodePoolStore() = default;

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
    using allocator_snapshot_t = typename Alloc::snapshot_t;
    /// T6-Route (A8-S5): die ECHTE Allocator-Achsen-Statistik (rich AllocationStatistics, 5 Felder) -- sie
    /// zaehlt jetzt sowohl die Knoten-Vektor-Reallokationen als auch JEDEN Forward-Turm, weil beide Ebenen
    /// ueber dieselbe Strategie laufen. Die frueheren Store-eigenen Schaetzzaehler (alloc_calls_/
    /// bytes_allocated_ aus Capacity-Deltas) sind damit entfallen: sie waren allocator-UNABHAENGIG und
    /// haetten neben der echten Achsen-Statistik eine zweite, driftende Wahrheit gefuehrt.
    [[nodiscard]] allocator_snapshot_t store_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

    /// Allokiert einen live Knoten mit `level` Forward-Slots (alle kNil) — darf via vector werfen (kein noexcept).
    /// KAUSALITAET (Posten 64/70): der Wurf kommt seit dem A8-S5-Schnitt vom StdAllocatorAdapter der
    /// Allokator-ACHSE -- die Strategie meldet OOM per nullptr, der Adapter uebersetzt ihn an EINER Stelle in
    /// std::bad_alloc (axis_06_allocator_strategy_base.hpp). Fehlerklasse bleibt der FK-5-Boden der Achse.
    /// [[allocation-failure-exception]]: Knoten-Vektor UND Forward-Turm koennen werfen.
    std::size_t allocate_node(key_type k, value_type v, int level) {
        std::size_t const idx = nodes_.size();
        nodes_.push_back(node_type{
            k,
            v,
            true,
            forward_vector_type(static_cast<std::size_t>(level), kNil,
                                allocator_.template as_std_allocator<std::size_t>()),
        });
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
        // BEWUSST pop_back statt resize(1): resize() instanziiert den DefaultInsertable-Pfad
        // (_M_default_append) und verlangte damit einen default-konstruierbaren Knoten -- der
        // Achsen-Adapter im Forward-Turm ist es bestimmungsgemaess NICHT (er haelt &allocator_).
        while (nodes_.size() > 1) nodes_.pop_back();
        for (auto& slot : nodes_[kHead].next) slot = kNil;
        live_count_ = 0;
        level_      = 1;
    }

private:
    void init_head() {
        // Head-Sentinel (Index 0): kMaxLevel Forward-Slots, alle kNil.
        nodes_.push_back(node_type{
            key_type{},
            value_type{},
            false,
            forward_vector_type(static_cast<std::size_t>(kMaxLevel), kNil,
                                allocator_.template as_std_allocator<std::size_t>()),
        });
    }

    /// Uebernimmt die Knoten einer Quelle und bindet JEDEN Forward-Turm neu an das EIGENE allocator_.
    /// (Der Turm ist ein Container IM Element -- eine Vektor-Kopie haette seinen Adapter mitkopiert und
    /// damit auf die Fremd-Strategie gezeigt.)
    void copy_nodes_from_(std::vector<node_type, node_allocator_type> const& src) {
        nodes_.reserve(src.size());
        for (auto const& n : src) {
            nodes_.push_back(node_type{
                n.key,
                n.val,
                n.live,
                forward_vector_type(n.next.begin(), n.next.end(), allocator_.template as_std_allocator<std::size_t>()),
            });
        }
    }

    // allocator_ VOR den Containern (der Adapter haelt &allocator_ -- Init-/Destruktions-Reihenfolge).
    Alloc                                       allocator_{};
    std::vector<node_type, node_allocator_type> nodes_;
    std::size_t                                 live_count_ = 0;
    int                                         level_      = 1;
    mutable std::mt19937_64                     rng_;
};

// Selbstbeweis: das Substrat erfuellt das SkipListNodePool-Concept.
static_assert(SkipListNodePool<SkipListNodePoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
