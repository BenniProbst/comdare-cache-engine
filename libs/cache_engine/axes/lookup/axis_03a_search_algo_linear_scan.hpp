#pragma once
// V41.F.6.1 R7.2 axis_03a_search_algo LinearScanSearchAlgo S15 (2026-05-29)
//
// @topic traversal @achse 03a @family S15 LinearScanSearchAlgo
// @subaxis SA2 sparse_access (UNSORTIERT — Einfuege-Reihenfolge, linearer Scan)
//
// **Algorithmus:** linearer Scan über ein UNSORTIERTES Key-Value-Array. Klassische Strategie für
// KLEINE Knoten: der ART Node4 (Leis ICDE 2013) nutzt linearen Scan, da bei <16 Eintraegen die
// fehlende Branch-Misprediction + die sequentielle Cache-Locality die O(n)-Kosten schlagen (keine
// Sortier-/Hash-/Baum-Overhead). Komplettiert die axis_03a-Palette um das unsortierte Baseline-
// Paradigma: WEDER dense (Array256/65535) noch sortiert (Vector/k-ary/interpolation/eytzinger) noch
// geordnete Struktur (skip-list) noch Hash (hash_search) noch Trie (ART/HOT/…), sondern der einfachste
// Vergleichs-Nullpunkt. supports_range_scan=false (keine Ordnung); insert ist O(1)-amortisiert (kein
// Sortieren), erase O(1) via swap-and-pop (Reihenfolge irrelevant), lookup O(n).
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** Konzept-Ableitung des ART-Node4-Linear-
// Scan (Leis ICDE 2013) bzw. Lehrbuch-Baseline → C++23-Re-Impl, is_original=false.
//
// Allocation: std::vector — [[allocation-failure-exception]]: insert kann std::bad_alloc werfen.

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/lookup/axis_03a_search_algo_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::lookup {

class LinearScanSearchAlgo : public SearchAlgoBase<LinearScanSearchAlgo> {
public:
    static constexpr bool enabled = flags::linear_scan_enabled;
    // (E-Welle-A2 / Befund-2 / A2.4-S1) Array-Familie (unsortierter Scan über flachen KV-Slot-Store) → store-traversierbar:
    // die Suche kann über DENSELBEN LayoutAwareChunkedStore laufen (node/layout/allocator wirken real). G3-klassifiziert.
    static constexpr bool axis_03a_store_traversable = true;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 15>; // S15

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      max_fanout() noexcept { return 65536; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "linear_scan"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::LinearScanSearchAlgo",
                                  "axes/lookup/axis_03a_search_algo_linear_scan.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "LinearScanSearchAlgo (unsorted linear scan, ART Node4-Strategie — Leis ICDE 2013)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "LINEAR_SCAN"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return false; }       // skalare Baseline
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return false; } // UNSORTIERT
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return false; }

    LinearScanSearchAlgo() noexcept = default;

    [[nodiscard]] bool operator==(LinearScanSearchAlgo const& other) const noexcept {
        return entries_.size() == other.entries_.size();
    }

    /// O(n)-Scan auf Duplikat (Update), sonst O(1)-amortisiertes Anhaengen (kein Sortieren).
    /// SONDERFALL [[allocation-failure-exception]]: push_back kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        for (auto& e : entries_) {
            if (e.first == k) {
                e.second = v;
                notify_insert();
                return;
            } // Update
        }
        entries_.emplace_back(k, v);
        notify_insert();
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        for (auto const& e : entries_) {
            if (e.first == k) {
                result = e.second;
                break;
            }
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
        for (std::size_t i = 0; i < entries_.size(); ++i) {
            if (entries_[i].first == k) {
                entries_[i] = entries_.back(); // swap-and-pop: O(1), Reihenfolge irrelevant (unsortiert)
                entries_.pop_back();
#ifdef COMDARE_CE_ENABLE_STATISTICS
                ++stats_.total_erase_count;
                observer_.notify(stats_);
#endif
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return entries_.size(); }
    [[nodiscard]] double    density_percent() const noexcept {
        return 100.0 * static_cast<double>(entries_.size()) / 65536.0;
    }
    void clear() noexcept { entries_.clear(); }

    /// DensityClassifiedStrategy [[density-classified-strategy]] — linear scan lohnt nur bei kleiner Belegung.
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        std::size_t const n = entries_.size();
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
    void notify_insert() noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (entries_.size() > stats_.peak_occupancy) stats_.peak_occupancy = entries_.size();
        observer_.notify(stats_);
#endif
    }

    std::vector<std::pair<key_type, value_type>> entries_; // UNSORTIERT (Einfuege-Reihenfolge)
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<LinearScanSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<LinearScanSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<LinearScanSearchAlgo>);
} // namespace comdare::cache_engine::lookup
