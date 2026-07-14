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
#include <axes/prefetch_axis/axis_07_prefetch_flags.hpp>
#include "axis_07_prefetch_distance_estimator_impl.hpp" // V41.F.6.1.F.6 native Logik (prt-art-Migration)
#include <topics/prefetch/concepts/topic_prefetch_concept.hpp>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::prefetch_axis {

/// DistanceEstimatorPrefetch — Distance-Estimator-basierter Software-Prefetch.
/// Standard fuer ART (Leis ICDE 2013): schaetzt Cache-Distance zur naechsten
/// Node + emittiert __builtin_prefetch wenn > Threshold.
class DistanceEstimatorPrefetch : public PrefetchStrategyBase<DistanceEstimatorPrefetch> {
public:
    using topic_tag = ::comdare::cache_engine::prefetch::concepts::PrefetchTopicTag;
    using axis_tag  = subaxes::distance_heuristic_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::distance_estimator_enabled;

    [[nodiscard]] static constexpr bool             is_active() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "prefetch_distance_estimator"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::prefetch_axis::DistanceEstimatorPrefetch",
                                  "axes/prefetch_axis/axis_07_prefetch_distance_estimator.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "DistanceEstimatorPrefetch (ART software-prefetch, distance-based)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "DISTANCE_ESTIMATOR"; }

    // V41.F.6.1.F.6 — native Density-/Latenz-Heuristik (prt-art REV 6 §5.17), stateless+constexpr.
    using impl_type                            = impl::DistanceEstimatorImpl;
    static constexpr std::uint8_t kMinDistance = impl_type::kMinDistance;
    static constexpr std::uint8_t kMaxDistance = impl_type::kMaxDistance;

    /// Schaetzt die Prefetch-Distanz (Cache-Lines) aus Knoten-Density [%] + Tier-Latenz [cycles].
    [[nodiscard]] static constexpr std::uint8_t estimate(double density_percent, double tier_latency_cycles) noexcept {
        return impl_type::estimate(density_percent, tier_latency_cycles);
    }
    [[nodiscard]] static constexpr std::uint8_t clamp(int raw) noexcept { return impl_type::clamp(raw); }
};

} // namespace comdare::cache_engine::prefetch_axis

namespace comdare::cache_engine::prefetch_axis {
static_assert(concepts::PrefetchStrategy<DistanceEstimatorPrefetch>);
static_assert(concepts::CacheEnginePermutationStrategy<DistanceEstimatorPrefetch>);
} // namespace comdare::cache_engine::prefetch_axis
