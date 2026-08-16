#pragma once
// V41.F.6.1.R7.5.d axis_14_value_handle CacheEngine-Permutation-Concept

#include "axis_14_value_handle_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::value_handle_axis::concepts {

template <typename V>
concept CacheEnginePermutationStrategy = ValueHandleStrategy<V> && requires {
    typename V::axis_tag;
    typename V::family_id;
    { V::name() } noexcept -> std::convertible_to<std::string_view>;
    { V::family_name() } noexcept -> std::convertible_to<std::string_view>;
    { V::flag_suffix() } noexcept -> std::convertible_to<std::string_view>;
    { V::enabled } -> std::convertible_to<bool>;
};

} // namespace comdare::cache_engine::value_handle_axis::concepts
