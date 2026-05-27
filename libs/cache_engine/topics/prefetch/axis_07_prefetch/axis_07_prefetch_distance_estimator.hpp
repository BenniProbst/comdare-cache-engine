#pragma once
// V41.F.6.1.R7.5.a axis_07 DistanceEstimatorPrefetch (ART)
//
// R7.6 Paper-Reference (Task #723):
// Leis, V., Kemper, A., Neumann, T. "The Adaptive Radix Tree: ARTful
// Indexing for Main-Memory Databases." Proceedings of IEEE ICDE 2013,
// pp. 38-49.
// DOI: 10.1109/ICDE.2013.6544812
// URL: https://db.in.tum.de/~leis/papers/ART.pdf
// Original-Implementation: https://github.com/armon/libart (verwendete
// Distance-Estimator-Heuristik aus Paper)
//
// Original-Pattern: Schaetzt Cache-Distance zur naechsten Node (typisch via
// Trie-Depth + Node-Capacity), emittiert __builtin_prefetch wenn Distance >
// Threshold (typisch 1 Cache-Line). Threshold heuristisch in Leis 2013 §4.2.

#include "axis_07_prefetch_strategy_base.hpp"
#include "axis_07_prefetch_subaxes_pf1_to_pf3.hpp"
#include "concepts/axis_07_prefetch_cache_engine_permutation_concept.hpp"
#include "axis_07_prefetch_flags.hpp"
#include "../concepts/topic_prefetch_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::prefetch::axis_07_prefetch {

/// DistanceEstimatorPrefetch — Distance-Estimator-basierter Software-Prefetch.
/// Standard fuer ART (Leis ICDE 2013): schaetzt Cache-Distance zur naechsten
/// Node + emittiert __builtin_prefetch wenn > Threshold.
class DistanceEstimatorPrefetch : public PrefetchStrategyBase<DistanceEstimatorPrefetch> {
public:
    using topic_tag = ::comdare::cache_engine::prefetch::concepts::PrefetchTopicTag;
    using axis_tag  = subaxes::distance_heuristic_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::distance_estimator_enabled;

    [[nodiscard]] static constexpr bool             is_active()    noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "prefetch_distance_estimator"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "DistanceEstimatorPrefetch (ART software-prefetch, distance-based)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "DISTANCE_ESTIMATOR"; }
};

}  // namespace

namespace comdare::cache_engine::prefetch::axis_07_prefetch {
    static_assert(concepts::PrefetchStrategy<DistanceEstimatorPrefetch>);
    static_assert(concepts::CacheEnginePermutationStrategy<DistanceEstimatorPrefetch>);
}
