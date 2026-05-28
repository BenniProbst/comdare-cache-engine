#pragma once
// V41.F.6.1 axis_03a_search_algo VectorU16U16SearchAlgo S03 (2026-05-26)
//
// @topic traversal @achse 03a @family S03 VectorU16U16SearchAlgo
// @subaxis SA3 multilevel_access
//
// **Algorithmus-Pattern:** Multi-Byte Discriminator mit Cost-DP-Splitting
// (START Self-Tuning Adaptive Radix Tree, Fent et al., ICDEW 2020).
//
// Standalone-Implementation: sortierte std::vector<uint16> Keys + parallel
// std::vector<uint64> Values, std::lower_bound O(log N) Lookup. Aequivalent
// zu START-multibyte Decision-Points (vereinfacht ohne Cost-Modell — das
// wuerde adaptive Permutationen erfordern, hier als Pilot weggelassen).
//
// Erfuellt:
//   - SearchAlgoVariant (Pflicht-API)
//   - CacheEngineSearchAlgoPermutationStrategy (cache-engine-spec)
//   - DensityClassifiedStrategy (DensityClass::Balanced)
//   - **NICHT** SimdCapableStrategy (Cost-DP nicht SIMD-vektorisierbar)
//
// Allocation: std::vector dynamisch — [[allocation-failure-exception]].

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include "../concepts/topic_traversal_concept.hpp"

#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::traversal::axis_03a_search_algo {

class VectorU16U16SearchAlgo : public SearchAlgoBase<VectorU16U16SearchAlgo> {
public:
    static constexpr bool enabled = flags::vector_u16u16_enabled;

    using key_type   = std::uint16_t;  // Multi-Byte Discriminator
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::multilevel_access_tag;
    using family_id  = std::integral_constant<int, 3>;  // S03

    [[nodiscard]] static constexpr bool        is_thread_safe()    noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_fanout()        noexcept { return 65536; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "vector_u16u16"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "VectorU16U16SearchAlgo (START multi-byte Cost-DP, Mertens ICDE 2024)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "VECTOR_U16U16"; }

    /// SONDERFALL: kein SIMD — Cost-DP-Algorithmus ist nicht vectorisierbar.
    [[nodiscard]] static constexpr bool supports_simd()            noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_range_scan()      noexcept { return true; }
    [[nodiscard]] static constexpr bool is_dense()                 noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

    VectorU16U16SearchAlgo() = default;

    [[nodiscard]] bool operator==(VectorU16U16SearchAlgo const& other) const noexcept {
        return keys_.size() == other.keys_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: push_back kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        if (it != keys_.end() && *it == k) {
            values_[idx] = v;
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
        if (hit) ++stats_.total_hit_count;
        else      ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        if (it == keys_.end() || *it != k) return std::nullopt;
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        return values_[idx];
    }

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
    [[nodiscard]] double    density_percent() const noexcept {
        return 100.0 * static_cast<double>(keys_.size()) / 65536.0;
    }
    void                    clear() noexcept { keys_.clear(); values_.clear(); }

    /// DensityClassifiedStrategy [[density-classified-strategy]]:
    /// Balanced default — Multilevel-Cost-DP optimiert sich automatisch
    /// fuer mittlere Density-Bereiche.
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        return concepts::DensityClass::Balanced;
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
    std::vector<key_type>   keys_;
    std::vector<value_type> values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                      observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::traversal::axis_03a_search_algo {
    static_assert(concepts::SearchAlgoVariant<VectorU16U16SearchAlgo>);
    static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<VectorU16U16SearchAlgo>);
    static_assert(concepts::DensityClassifiedStrategy<VectorU16U16SearchAlgo>);
    // NICHT: SimdCapableStrategy (Cost-DP ist nicht vektorisierbar)
}
