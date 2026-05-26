#pragma once
// V41.F.6.1.F2 axis_11 LeafOnlyCounter Default-Wrapper (Kuehn 11.X1)

#include "axis_11_telemetry_base.hpp"
#include "../concepts/topic_telemetry_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::telemetry::axis_11_telemetry {

/// LeafOnlyCounter — Kuehn Hauptvariante 11.X1: Counter NUR in Blatt-Knoten
/// (vermeidet Cache-Line-Ping-Pong).
class LeafOnlyCounter : public TelemetryBase<LeafOnlyCounter> {
public:
    using topic_tag = ::comdare::cache_engine::telemetry::concepts::TelemetryTopicTag;
    using family_id = std::integral_constant<int, 1>;
    [[nodiscard]] static constexpr bool is_leaf_only() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "leaf_only_counter"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "LeafOnlyCounter (Kuehn DaMoN 2023 X1)"; }
};

}  // namespace

namespace comdare::cache_engine::telemetry::axis_11_telemetry {
    static_assert(concepts::TelemetryStrategy<LeafOnlyCounter>);
}
