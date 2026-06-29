#pragma once
#include <topics/value_handle/concepts/topic_value_handle_concept.hpp>
#include <concepts>
namespace comdare::cache_engine::value_handle_axis::concepts {
template <typename V>
concept ValueHandleStrategy = ::comdare::cache_engine::value_handle::concepts::ValueHandleComponent<V> && requires {
    { V::is_inline() } noexcept -> std::convertible_to<bool>;
};
} // namespace comdare::cache_engine::value_handle_axis::concepts
