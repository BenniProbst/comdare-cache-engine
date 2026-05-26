#pragma once
#include "../../concepts/topic_value_handle_concept.hpp"
#include <concepts>
namespace comdare::cache_engine::value_handle::axis_14_value_handle::concepts {
template <typename V>
concept ValueHandleStrategy =
    ::comdare::cache_engine::value_handle::concepts::ValueHandleComponent<V>
    && requires { { V::is_inline() } noexcept -> std::convertible_to<bool>; };
}  // namespace
