#pragma once
// V41.F.6.1.R7.5.b axis_11 DensityTracker (ART per-node density)

#include "axis_11_telemetry_strategy_base.hpp"
#include "axis_11_telemetry_subaxes_tm1_to_tm3.hpp"
#include "concepts/axis_11_telemetry_cache_engine_permutation_concept.hpp"
#include "axis_11_telemetry_flags.hpp"
#include "../concepts/topic_telemetry_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::telemetry::axis_11_telemetry {

/// DensityTracker — per-Node Density-Tracking (ART Leis ICDE 2013).
/// Misst Slot-Occupancy pro Node fuer adaptive Node-Type-Wahl (4→16→48→256).
/// Hoeherer Overhead als LeafOnly (alle Nodes), aber liefert Adaption-Signal.
class DensityTracker : public TelemetryStrategyBase<DensityTracker> {
public:
    using topic_tag = ::comdare::cache_engine::telemetry::concepts::TelemetryTopicTag;
    using axis_tag  = subaxes::metric_type_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::density_tracker_enabled;

    [[nodiscard]] static constexpr bool             is_leaf_only() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "telemetry_density_tracker"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "DensityTracker (ART per-node density for adaptive Node-Type)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "DENSITY_TRACKER"; }
};

}  // namespace

namespace comdare::cache_engine::telemetry::axis_11_telemetry {
    static_assert(concepts::TelemetryStrategy<DensityTracker>);
    static_assert(concepts::CacheEnginePermutationStrategy<DensityTracker>);
}
