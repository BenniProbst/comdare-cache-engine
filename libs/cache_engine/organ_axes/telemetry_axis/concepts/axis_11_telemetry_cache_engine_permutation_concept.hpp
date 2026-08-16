#pragma once
// V41.F.6.1.R7.5.b axis_11_telemetry CacheEngine-Permutation-Concept

#include "axis_11_telemetry_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::telemetry_axis::concepts {

template <typename T>
concept CacheEnginePermutationStrategy = TelemetryStrategy<T> && requires {
    typename T::axis_tag;
    typename T::family_id;
    { T::name() } noexcept -> std::convertible_to<std::string_view>;
    { T::family_name() } noexcept -> std::convertible_to<std::string_view>;
    { T::flag_suffix() } noexcept -> std::convertible_to<std::string_view>;
    { T::enabled } -> std::convertible_to<bool>;
};

} // namespace comdare::cache_engine::telemetry_axis::concepts
