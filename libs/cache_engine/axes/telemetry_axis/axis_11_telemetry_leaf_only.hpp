#pragma once
// V41.F.6.1.R7.5.b axis_11 LeafOnlyCounter (Goldstandard-Update, leaf-scope counter)

#include "axis_11_telemetry_strategy_base.hpp"
#include "axis_11_telemetry_subaxes_tm1_to_tm3.hpp"
#include "concepts/axis_11_telemetry_cache_engine_permutation_concept.hpp"
#include <axes/telemetry_axis/axis_11_telemetry_flags.hpp>
#include <topics/telemetry/concepts/topic_telemetry_concept.hpp>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::telemetry_axis {

/// LeafOnlyCounter — Counter NUR in Blatt-Knoten
/// (vermeidet Cache-Line-Ping-Pong durch shared Inner-Node-Updates).
/// HINWEIS: KEINE Paper-Attribution — das Kuehn-DaMoN-2023-Paper behandelt
/// cache-freundliches B+-Layout, NICHT einen Leaf-only-Counter.
class LeafOnlyCounter : public TelemetryStrategyBase<LeafOnlyCounter> {
public:
    using topic_tag = ::comdare::cache_engine::telemetry::concepts::TelemetryTopicTag;
    using axis_tag  = subaxes::scope_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::leaf_only_counter_enabled;

    [[nodiscard]] static constexpr bool             is_leaf_only() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "telemetry_leaf_only_counter"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "LeafOnlyCounter (leaf-scope only)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "LEAF_ONLY_COUNTER"; }
};

} // namespace comdare::cache_engine::telemetry_axis

namespace comdare::cache_engine::telemetry_axis {
static_assert(concepts::TelemetryStrategy<LeafOnlyCounter>);
static_assert(concepts::CacheEnginePermutationStrategy<LeafOnlyCounter>);
} // namespace comdare::cache_engine::telemetry_axis
