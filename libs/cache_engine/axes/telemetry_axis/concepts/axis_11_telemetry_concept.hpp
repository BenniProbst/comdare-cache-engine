#pragma once
// V41.F.6.1.F2 axis_11_telemetry Standard-Concept (Skelett-Stufe-A, Kuehn Sub-Strategien 11.X1-X4)

#include <topics/telemetry/concepts/topic_telemetry_concept.hpp>
#include <concepts>

namespace comdare::cache_engine::telemetry_axis::concepts {

template <typename T>
concept TelemetryStrategy =
    ::comdare::cache_engine::telemetry::concepts::TelemetryComponent<T>
    && requires { { T::is_leaf_only() } noexcept -> std::convertible_to<bool>; };

}  // namespace
