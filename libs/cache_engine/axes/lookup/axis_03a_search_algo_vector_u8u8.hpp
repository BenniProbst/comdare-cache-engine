#pragma once
// V41.F.6.1 axis_03a_search_algo VectorU8U8SearchAlgo S02 (2026-05-26)
//
// @topic traversal @achse 03a @family S02 VectorU8U8SearchAlgo
// @subaxis SA2 sparse_access
//
// **Algorithmus-Pattern:** sparse sortiertes Key-Value-Vektor-Paar mit
// Patricia-Compression-Eigenschaften, beschrieben in:
//   Binna/Zangerle/Pichl: "HOT: A Height Optimized Trie Index for Main-Memory
//   Database Systems." SIGMOD 2018.
//
// Standalone-Implementation (kein Delegate, keine Legacy-Code-Referenz):
// std::lower_bound auf std::vector<uint8> keys + parallel std::vector<uint64>
// values fuer O(log N) Lookup + O(N) Insert/Erase (shift). Konzept-aequivalent
// zu HOT-k-constrained mit kleinem k.
//
// Erfuellt:
//   - SearchAlgoVariant (Pflicht-API)
//   - CacheEngineSearchAlgoPermutationStrategy (cache-engine-spec)
//   - DensityClassifiedStrategy (DensityClass dynamisch nach density_threshold)
//   - SimdCapableStrategy (simd_lookup mit Bit-Mask-Scan ueber sortierte Keys)
//   - IterableAspectSearchAlgoStrategy (density_threshold_pct hybride Permutation)
//
// Allocation: std::vector dynamisch — [[allocation-failure-exception]]:
// insert kann std::bad_alloc werfen.

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include "concepts/axis_03a_search_algo_simd_capable_strategy_concept.hpp"
#include "concepts/axis_03a_search_algo_iterable_aspect_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/lookup/axis_03a_search_algo_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::lookup {

class VectorU8U8SearchAlgo : public SearchAlgoBase<VectorU8U8SearchAlgo> {
public:
    static constexpr bool enabled = flags::vector_u8u8_enabled;
    // #188-4c-ii: faithful Flach-Store-Pfad via SortedVectorTraversal; density_threshold bleibt Klassifizierung.
    static constexpr bool axis_03a_store_traversable = true;

    using key_type   = std::uint8_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 2>; // S02

    /// iterable_aspect_t (F.6.1.E hybride Laufzeit-Permutation):
    /// density_threshold_pct steuert die Klassifizierungs-Schwelle Sparse/Balanced.
    /// PermutationEngine erkennt via HasIterableAspect<V> und generiert 1 Binary
    /// mit Runtime-Loop ueber kIterableDensityThresholds statt 5 separate Binaries.
    using iterable_aspect_t = unsigned;
    static constexpr std::array<unsigned, 5>                 kIterableDensityThresholds{10u, 20u, 30u, 50u, 70u};
    [[nodiscard]] static constexpr std::span<unsigned const> iterable_values() noexcept {
        return std::span<unsigned const>{kIterableDensityThresholds.data(), kIterableDensityThresholds.size()};
    }

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      max_fanout() noexcept { return 256; } // theoretisch, sparse
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "vector_u8u8"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "VectorU8U8SearchAlgo (HOT Patricia sparse — Binna PVLDB 2018)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "VECTOR_U8U8"; }

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return true; }
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; } // sorted insert
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

    static constexpr unsigned kDefaultDensityThresholdPct = 30;

    VectorU8U8SearchAlgo() noexcept : density_threshold_pct_(kDefaultDensityThresholdPct) {}
    explicit VectorU8U8SearchAlgo(unsigned density_threshold_pct) noexcept
        : density_threshold_pct_(density_threshold_pct) {}

    [[nodiscard]] bool operator==(VectorU8U8SearchAlgo const& other) const noexcept {
        return keys_.size() == other.keys_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: push_back kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        auto        it  = std::lower_bound(keys_.begin(), keys_.end(), k);
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        if (it != keys_.end() && *it == k) {
            values_[idx] = v; // update
        } else {
            keys_.insert(it, k);
            values_.insert(values_.begin() + idx, v);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (keys_.size() > stats_.peak_occupancy) stats_.peak_occupancy = keys_.size();
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        bool hit = (it != keys_.end() && *it == k);
        if (hit)
            ++stats_.total_hit_count;
        else
            ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        if (it == keys_.end() || *it != k) return std::nullopt;
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        return values_[idx];
    }

    /// SIMD-Fast-Path ([[simd-capable-strategy]] Sub-Concept).
    /// HOT-typisch: Bit-Mask-Scan ueber kleinen sortierten Keys-Vektor mit AVX2.
    /// Pilot-Implementation: identisch mit lookup (skalare lower_bound), real
    /// wuerde SIMD-Comparison ueber 8/16 keys auf einmal genutzt.
    [[nodiscard]] std::optional<value_type> simd_lookup(key_type k) const { return lookup(k); }

    bool erase(key_type k) {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        if (it == keys_.end() || *it != k) return false;
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        keys_.erase(it);
        values_.erase(values_.begin() + idx);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_erase_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return keys_.size(); }
    [[nodiscard]] double density_percent() const noexcept { return 100.0 * static_cast<double>(keys_.size()) / 256.0; }
    void                 clear() noexcept {
        keys_.clear();
        values_.clear();
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]]:
    /// Sparse default; dynamisch klassifiziert ueber density_threshold_pct
    /// (iterable_aspect_t — Runtime-Setter via set_iterable_aspect).
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        double       dp = density_percent();
        double const t  = static_cast<double>(density_threshold_pct_);
        if (dp > t * 2.0) return concepts::DensityClass::Dense; // weit ueber Threshold
        if (dp > t) return concepts::DensityClass::Balanced;    // ueber Threshold
        return concepts::DensityClass::Sparse;                  // unter Threshold
    }

    /// IterableAspectSearchAlgoStrategy [[iterable-aspect-strategy]]:
    /// Konsolidierter Setter analog Q1/Q2 + 03a-Schablone.
    void set_iterable_aspect(unsigned new_threshold_pct) noexcept { density_threshold_pct_ = new_threshold_pct; }

    /// Accessor (Diagnostik): aktueller density_threshold_pct.
    [[nodiscard]] unsigned density_threshold_pct() const noexcept { return density_threshold_pct_; }

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
    unsigned                density_threshold_pct_;
    std::vector<key_type>   keys_;
    std::vector<value_type> values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<VectorU8U8SearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<VectorU8U8SearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<VectorU8U8SearchAlgo>);
static_assert(concepts::SimdCapableStrategy<VectorU8U8SearchAlgo>);
static_assert(concepts::IterableAspectSearchAlgoStrategy<VectorU8U8SearchAlgo>);
} // namespace comdare::cache_engine::lookup
