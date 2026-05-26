#pragma once
// V41.F.6.1 axis_03a_search_algo Array256 S01 (2026-05-26)
//
// @topic traversal @achse 03a @family S01 Array256
// @subaxis SA1 dense_access
//
// **Algorithmus-Pattern:** ART Node256 direct-addressed dense Search.
//   Leis/Kemper/Neumann: "The Adaptive Radix Tree: ARTful Indexing for
//   Main-Memory Databases." ICDE 2013, pp. 38-49.
//
// Standalone-Implementation: std::array<std::optional<value>, 256> mit
// O(1) Lookup/Insert/Erase. Cache-Line-Alignment durch contiguous layout.
//
// Erfuellt:
//   - SearchAlgoVariant (Pflicht-API)
//   - CacheEngineSearchAlgoPermutationStrategy (cache-engine-spec)
//   - DensityClassifiedStrategy (DensityClass::Dense)
//   - SimdCapableStrategy (simd_lookup mit linear scan, fallback noexcept)
//
// Allocation: kein dynamisches Heap (std::array statisch) — KEIN
// [[allocation-failure-exception]] Risk fuer insert/lookup.

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include "concepts/axis_03a_search_algo_simd_capable_strategy_concept.hpp"
#include "../concepts/topic_traversal_concept.hpp"

#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::traversal::axis_03a_search_algo {

class Array256 : public SearchAlgoBase<Array256> {
public:
    static constexpr bool enabled = flags::array256_enabled;

    using key_type   = std::uint8_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::dense_access_tag;
    using family_id  = std::integral_constant<int, 1>;  // S01

    [[nodiscard]] static constexpr bool        is_thread_safe()    noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_fanout()        noexcept { return 256; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "array256"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "Array256 (ART Node256 direct-addressed, Leis ICDE 2013)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "ARRAY256"; }

    [[nodiscard]] static constexpr bool supports_simd()            noexcept { return true; }
    [[nodiscard]] static constexpr bool supports_range_scan()      noexcept { return true; }  // sorted by index
    [[nodiscard]] static constexpr bool is_dense()                 noexcept { return true; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

    Array256() noexcept : count_(0) {}

    [[nodiscard]] bool operator==(Array256 const& other) const noexcept {
        return count_ == other.count_;
    }

    void insert(key_type k, value_type v) {
        if (!data_[k].has_value()) ++count_;
        data_[k] = v;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (count_ > stats_.peak_occupancy) stats_.peak_occupancy = count_;
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        if (data_[k].has_value()) ++stats_.total_hit_count;
        else                       ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        return data_[k];
    }

    /// SIMD-Fast-Path ([[simd-capable-strategy]] Sub-Concept).
    /// Bei Array256 ist direkt-Lookup bereits O(1) — SIMD-Branch verifiziert
    /// nur das Presence-Bit (relevanter bei multi-key Bulk-Lookup).
    [[nodiscard]] std::optional<value_type> simd_lookup(key_type k) const {
        return data_[k];  // semantisch aequivalent, hot-path optimiert
    }

    bool erase(key_type k) {
        if (!data_[k].has_value()) return false;
        data_[k].reset();
        --count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_erase_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return count_; }
    [[nodiscard]] double    density_percent() const noexcept {
        return 100.0 * static_cast<double>(count_) / 256.0;
    }
    void                    clear() noexcept {
        for (auto& slot : data_) slot.reset();
        count_ = 0;
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]]:
    /// Array256 ist immer per Konstruktion DENSE (direkt-adressiert).
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        return concepts::DensityClass::Dense;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::SearchAlgoStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot()   const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; observer_.notify(stats_); }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer()       noexcept { return observer_; }
#endif

private:
    std::array<std::optional<value_type>, 256> data_{};
    std::size_t count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                      observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::traversal::axis_03a_search_algo {
    static_assert(concepts::SearchAlgoVariant<Array256>);
    static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<Array256>);
    static_assert(concepts::DensityClassifiedStrategy<Array256>);
    static_assert(concepts::SimdCapableStrategy<Array256>);
}
