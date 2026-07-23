#pragma once
// V41.F.6.1 R7.2 axis_03a_search_algo EytzingerSearchAlgo S12 (2026-05-29)
//
// @topic traversal @achse 03a @family S12 EytzingerSearchAlgo
// @subaxis SA2 sparse_access (sortierte Keys, Such-METHODE = cache-conscious Layout + branch-free)
//
// **Algorithmus:** Eytzinger- (BFS-/Heap-) Layout-Suche, beschrieben in:
//   Paul-Virak Khuong, Pat Morin: "Array Layouts for Comparison-Based Searching."
//   Journal of Experimental Algorithmics (JEA) 22, 2017 (arXiv:1509.05053).
//
// Statt sortierter Reihenfolge werden die Keys in Breadth-First-Order des impliziten
// Suchbaums abgelegt (Wurzel an Index 1, Kinder von i an 2i/2i+1). Die Suche ist branch-free:
//   k = 1;  while (k <= n)  k = 2*k + (eyt[k] < x);
// danach liefert  idx = k >> (countr_one(k) + 1)  den Eytzinger-Index des lower_bound (0 = keiner).
// Vorteil (Paper): die ersten Ebenen liegen dicht beieinander → cache-/prefetch-freundlich, der
// branch-free Kern vermeidet Branch-Mispredictions. Fuer grosse Arrays laut Paper das schnellste
// Allzweck-Layout. Distinkt von KArySearchAlgo (SIMD-Partition), InterpolationSearchAlgo
// (Verteilung) und den lower_bound-Wrappern (sortiert/halbierend): CACHE-LAYOUT-Such-METHODE —
// trifft direkt das cache-engine-/F15-Thema (Speicher-Layout der std::map-Innenstruktur → Performance).
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** Referenz-Code (github.com/patmorin/
// arraylayout) ist ein Experiment-Harness ohne deklarierte Standard-OSS-Lizenz; diese Implementierung
// ist eine originalgetreue C++23-Re-Implementierung des Eytzinger-Layouts + der branch-free Suche
// → is_original = false (via AxisBase-Default).
//
// Erfuellt: SearchAlgoVariant, CacheEngineSearchAlgoPermutationStrategy, DensityClassifiedStrategy.
//
// Allocation: std::vector dynamisch — [[allocation-failure-exception]]: insert kann
// std::bad_alloc werfen.

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/lookup/axis_03a_search_algo_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::lookup {

class EytzingerSearchAlgo : public SearchAlgoBase<EytzingerSearchAlgo> {
public:
    static constexpr bool enabled = flags::eytzinger_enabled;
    // #188-4a (2026-07-02, Option b): store-traversierbar via organ_for_search_algo -> EytzingerOrgan
    // (EytzingerLayoutStore: sortierter Primaerzustand + lazy BFS-Puffer). BEWUSST weiterhin KEIN
    // axis_03a_store_traversable-Marker (der gilt dem FLAT-Store-Pfad); Registry-static_assert bleibt wahr.
    // Dieser u16-Wrapper bleibt Registry-Tier + Aequivalenz-Referenz; die 320er-Messung laeuft ueber das Organ.

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 12>; // S12

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      max_fanout() noexcept { return 65536; } // u16 Keyraum
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "eytzinger"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::EytzingerSearchAlgo",
                                  "axes/lookup/axis_03a_search_algo_eytzinger.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "EytzingerSearchAlgo (cache-conscious BFS layout, branch-free — Khuong/Morin JEA 2017)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "EYTZINGER"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0";

    [[nodiscard]] static constexpr bool supports_simd() noexcept {
        return false;
    } // branch-free + prefetch, nicht vektorisiert
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; }      // sortierte Quelle
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }                // sparse sortiert
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; } // Kern-Vorteil des Layouts

    EytzingerSearchAlgo() noexcept = default;

    [[nodiscard]] bool operator==(EytzingerSearchAlgo const& other) const noexcept {
        return keys_.size() == other.keys_.size();
    }

    /// Quelle der Wahrheit ist die sortierte keys_/values_-Liste; das Eytzinger-Layout wird lazy
    /// (bei naechstem lookup) neu gebaut. SONDERFALL [[allocation-failure-exception]]: std::bad_alloc.
    void insert(key_type k, value_type v) {
        auto        it  = std::lower_bound(keys_.begin(), keys_.end(), k);
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        if (it != keys_.end() && *it == k) {
            values_[idx] = v; // update (Layout-Werte aendern sich -> dirty)
        } else {
            keys_.insert(it, k);
            values_.insert(values_.begin() + static_cast<std::ptrdiff_t>(idx), v);
        }
        dirty_ = true;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (keys_.size() > stats_.peak_occupancy) stats_.peak_occupancy = keys_.size();
        observer_.notify(stats_);
#endif
    }

    /// Eytzinger branch-free Suche (Khuong/Morin 2017). Baut das Layout bei Bedarf neu auf.
    [[nodiscard]] std::optional<value_type> lookup(key_type x) const {
        std::optional<value_type> result = std::nullopt;
        std::size_t const         n      = keys_.size();
        if (n != 0) {
            if (dirty_) rebuild_eytzinger();
            std::size_t k = 1;
            while (k <= n) {
                k = 2 * k + (eyt_keys_[k] < x ? 1u : 0u); // 0 = links (>=x), 1 = rechts (<x)
            }
            // lower_bound-Eytzinger-Index: trailing-1s + 1 wegshiften (0 ⇒ kein Element >= x).
            std::size_t const idx = k >> (static_cast<unsigned>(std::countr_one(k)) + 1u);
            if (idx >= 1 && idx <= n && eyt_keys_[idx] == x) result = eyt_vals_[idx];
        }
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
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        if (it == keys_.end() || *it != k) return false;
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        keys_.erase(it);
        values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(idx));
        dirty_ = true;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_erase_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return keys_.size(); }
    [[nodiscard]] double    density_percent() const noexcept {
        return 100.0 * static_cast<double>(keys_.size()) / 65536.0;
    }
    void clear() noexcept {
        keys_.clear();
        values_.clear();
        eyt_keys_.clear();
        eyt_vals_.clear();
        dirty_ = true;
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]]: Belegungs-basierte Klassifikation.
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        std::size_t const n = keys_.size();
        if (n > 1024) return concepts::DensityClass::Dense;
        if (n > 64) return concepts::DensityClass::Balanced;
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
    /// Baut das 1-indizierte Eytzinger-Layout (eyt_keys_[1..n]) aus der sortierten Quelle via
    /// In-Order-Befuellung des impliziten Suchbaums.
    void rebuild_eytzinger() const {
        std::size_t const n = keys_.size();
        eyt_keys_.assign(n + 1, key_type{});
        eyt_vals_.assign(n + 1, value_type{});
        std::size_t pos = 0;
        fill_eytzinger(1, n, pos);
        dirty_ = false;
    }
    void fill_eytzinger(std::size_t k, std::size_t n, std::size_t& pos) const {
        if (k <= n) {
            fill_eytzinger(2 * k, n, pos);
            eyt_keys_[k] = keys_[pos];
            eyt_vals_[k] = values_[pos];
            ++pos;
            fill_eytzinger(2 * k + 1, n, pos);
        }
    }

    std::vector<key_type>           keys_;
    std::vector<value_type>         values_;
    mutable std::vector<key_type>   eyt_keys_;
    mutable std::vector<value_type> eyt_vals_;
    mutable bool                    dirty_ = false;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<EytzingerSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<EytzingerSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<EytzingerSearchAlgo>);
} // namespace comdare::cache_engine::lookup
