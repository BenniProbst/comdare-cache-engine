#pragma once
// V41.F.6.1.F2 Topic telemetry Marker-Concept (Skelett-Stufe-A)

#include <concepts>

namespace comdare::cache_engine::telemetry::concepts {

struct TelemetryTopicTag {};

template <typename T>
concept TelemetryComponent =
    requires { typename T::topic_tag; } && std::same_as<typename T::topic_tag, TelemetryTopicTag>;

} // namespace comdare::cache_engine::telemetry::concepts
