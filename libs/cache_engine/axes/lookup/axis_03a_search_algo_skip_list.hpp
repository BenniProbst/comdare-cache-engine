#pragma once
// V41.F.6.1 R7.2 axis_03a_search_algo SkipListSearchAlgo S13 (2026-05-29)
//
// @topic traversal @achse 03a @family S13 SkipListSearchAlgo
// @subaxis SA2 sparse_access (geordnete probabilistische STRUKTUR, nicht Array/Trie)
//
// **Algorithmus:** Skip-Liste — probabilistisch balancierte geordnete Struktur, beschrieben in:
//   William Pugh: "Skip Lists: A Probabilistic Alternative to Balanced Trees."
//   Communications of the ACM 33(6), Juni 1990, S. 668-676. DOI 10.1145/78973.78977.
//
// Jeder Knoten hat 1..kMaxLevel Forward-Zeiger; die Level-Zahl wird per Muenzwurf gezogen
// (P=0.5). Suche laeuft von oben nach unten + vorwaerts (ueberspringt grosse Distanzen auf hohen
// Levels) → erwartet O(log n) fuer search/insert/erase, OHNE explizite Rebalancierung. Distinkt
// von allen bisherigen axis_03a-Wrappern: weder Array (Array256/65535) noch sortierter Vektor
// (VectorU8U8/U16U16) noch Such-METHODE (k-ary/interpolation/eytzinger) noch Radix-Trie (ART/HOT/…),
// sondern eine eigenstaendige verzeigerte ORDERED-MAP-Struktur — erweitert die R7.2-Achse von
// Such-Methoden in die Struktur-Dimension. Trifft das F15-Thema (Datenstruktur des std::map-
// Innenlebens → Performance).
//
// **Implementierung:** index-basiert (std::vector<Node> mit uint32-Forward-INDICES + Tombstone-
// Erase) → vollstaendig WERT-semantisch (kopierbar, kein manuelles new/delete, kein Dangling),
// konsistent mit den Geschwister-Wrappern. Level-RNG: deterministischer std::mt19937_64 (fester
// Seed → reproduzierbar/testbar).
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** Lehrbuch-Algorithmus (Pugh-Pseudocode),
// kein kanonischer permissiver Single-Repo-Code → originalgetreue C++23-Re-Impl, is_original=false.
//
// Erfuellt: SearchAlgoVariant, CacheEngineSearchAlgoPermutationStrategy, DensityClassifiedStrategy.
//
// Allocation: std::vector dynamisch — [[allocation-failure-exception]]: insert kann std::bad_alloc.

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/lookup/axis_03a_search_algo_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::lookup {

class SkipListSearchAlgo : public SearchAlgoBase<SkipListSearchAlgo> {
public:
    static constexpr bool enabled = flags::skip_list_enabled;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 13>; // S13

    static constexpr int           kMaxLevel = 16;
    static constexpr std::uint32_t kNil      = 0xFFFFFFFFu; // "kein Nachfolger"
    static constexpr std::uint32_t kHead     = 0u;          // Sentinel-Kopf-Index

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      max_fanout() noexcept { return 65536; } // u16 Keyraum
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "skip_list"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "SkipListSearchAlgo (probabilistic ordered structure — Pugh CACM 1990)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "SKIP_LIST"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return false; }      // Pointer-Chasing
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; } // geordnet (Level-0-Kette)
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept {
        return false;
    } // verzeigert, nicht aligned

    SkipListSearchAlgo() noexcept : rng_(0xC0FFEEu) { init_head(); }

    [[nodiscard]] bool operator==(SkipListSearchAlgo const& other) const noexcept {
        return live_count_ == other.live_count_;
    }

    /// SONDERFALL [[allocation-failure-exception]]: nodes_-Wachstum kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        std::array<std::uint32_t, kMaxLevel> update{};
        std::uint32_t const                  cand = find_update(k, update);
        if (cand != kNil && nodes_[cand].key == k && nodes_[cand].live) {
            nodes_[cand].val = v; // Update vorhandener Key
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.total_insert_count;
            observer_.notify(stats_);
#endif
            return;
        }
        int const lvl = random_level();
        if (lvl > level_) {
            for (int i = level_; i < lvl; ++i) update[static_cast<std::size_t>(i)] = kHead;
            level_ = lvl;
        }
        std::uint32_t const idx = static_cast<std::uint32_t>(nodes_.size());
        nodes_.push_back(Node{k, v, true, std::vector<std::uint32_t>(static_cast<std::size_t>(lvl), kNil)});
        for (int i = 0; i < lvl; ++i) {
            std::uint32_t const pred                       = update[static_cast<std::size_t>(i)];
            nodes_[idx].next[static_cast<std::size_t>(i)]  = nodes_[pred].next[static_cast<std::size_t>(i)];
            nodes_[pred].next[static_cast<std::size_t>(i)] = idx;
        }
        ++live_count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (live_count_ > stats_.peak_occupancy) stats_.peak_occupancy = live_count_;
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        std::uint32_t             x      = kHead;
        for (int i = level_ - 1; i >= 0; --i) {
            std::uint32_t nxt = nodes_[x].next[static_cast<std::size_t>(i)];
            while (nxt != kNil && nodes_[nxt].key < k) {
                x   = nxt;
                nxt = nodes_[x].next[static_cast<std::size_t>(i)];
            }
        }
        std::uint32_t const cand = nodes_[x].next[0];
        if (cand != kNil && nodes_[cand].key == k && nodes_[cand].live) result = nodes_[cand].val;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        if (result)
            ++stats_.total_hit_count;
        else
            ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        return result;
    }

    bool erase(key_type k) {
        std::array<std::uint32_t, kMaxLevel> update{};
        std::uint32_t const                  cand = find_update(k, update);
        if (cand == kNil || nodes_[cand].key != k || !nodes_[cand].live) return false;
        for (int i = 0; i < level_; ++i) {
            std::uint32_t const pred = update[static_cast<std::size_t>(i)];
            if (nodes_[pred].next[static_cast<std::size_t>(i)] == cand) {
                nodes_[pred].next[static_cast<std::size_t>(i)] = nodes_[cand].next[static_cast<std::size_t>(i)];
            }
        }
        nodes_[cand].live = false; // Tombstone (unverlinkt → unerreichbar)
        while (level_ > 1 && nodes_[kHead].next[static_cast<std::size_t>(level_ - 1)] == kNil) --level_;
        --live_count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_erase_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return live_count_; }
    [[nodiscard]] double density_percent() const noexcept { return 100.0 * static_cast<double>(live_count_) / 65536.0; }
    void                 clear() noexcept {
        // Allokationsfrei: nur den Head behalten + seine Forward-Slots auf kNil zuruecksetzen
        // (kein push_back → kein bad_alloc im noexcept-Pfad).
        nodes_.resize(1);
        for (auto& slot : nodes_[kHead].next) slot = kNil;
        live_count_ = 0;
        level_      = 1;
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]]: Belegungs-basierte Klassifikation.
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        if (live_count_ > 1024) return concepts::DensityClass::Dense;
        if (live_count_ > 64) return concepts::DensityClass::Balanced;
        return concepts::DensityClass::Sparse;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::SearchAlgoStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_ = {};
        observer_.notify(stats_);
    }
    // CoW-Memento (#142/Audit-K3): Stat-POD-Restore -> organ_cow_capable_v aktiv (spiegelt Observable-Huelle).
    void restore_statistics(snapshot_t const& s) noexcept {
        stats_ = s;
        observer_.notify(stats_);
    }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }
#endif

private:
    struct Node {
        key_type                   key{};
        value_type                 val{};
        bool                       live{};
        std::vector<std::uint32_t> next{}; // Forward-INDICES je Level (kNil = Ende)
    };

    void init_head() {
        // Head-Sentinel (Index 0): kMaxLevel Forward-Slots, alle kNil.
        nodes_.push_back(Node{key_type{}, value_type{}, false,
                              std::vector<std::uint32_t>(static_cast<std::size_t>(kMaxLevel), kNil)});
    }

    /// Fuellt update[i] = Praedezessor-Index auf Level i und liefert den Level-0-Kandidaten (>= k).
    [[nodiscard]] std::uint32_t find_update(key_type k, std::array<std::uint32_t, kMaxLevel>& update) const {
        std::uint32_t x = kHead;
        for (int i = level_ - 1; i >= 0; --i) {
            std::uint32_t nxt = nodes_[x].next[static_cast<std::size_t>(i)];
            while (nxt != kNil && nodes_[nxt].key < k) {
                x   = nxt;
                nxt = nodes_[x].next[static_cast<std::size_t>(i)];
            }
            update[static_cast<std::size_t>(i)] = x;
        }
        return nodes_[x].next[0];
    }

    [[nodiscard]] int random_level() noexcept {
        int lvl = 1;
        while ((rng_() & 1u) != 0u && lvl < kMaxLevel) ++lvl; // Muenzwurf P=0.5
        return lvl;
    }

    std::vector<Node>       nodes_{};
    std::size_t             live_count_ = 0;
    int                     level_      = 1;
    mutable std::mt19937_64 rng_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<SkipListSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<SkipListSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<SkipListSearchAlgo>);
} // namespace comdare::cache_engine::lookup
