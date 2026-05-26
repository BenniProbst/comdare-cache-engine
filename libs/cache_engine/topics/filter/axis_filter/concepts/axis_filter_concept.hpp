#pragma once
#include "../../concepts/topic_filter_concept.hpp"
#include <concepts>
namespace comdare::cache_engine::filter::axis_filter::concepts {
template <typename F>
concept FilterStrategy =
    ::comdare::cache_engine::filter::concepts::FilterComponent<F>
    && requires { { F::supports_range_query() } noexcept -> std::convertible_to<bool>; };
}  // namespace
