#pragma once
// V41.F.6.1.R7.5.c axis_10_serialization CacheEngine-Permutation-Concept

#include "axis_10_serialization_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::serialization_axis::concepts {

template <typename S>
concept CacheEnginePermutationStrategy = SerializationStrategy<S> && requires {
    typename S::axis_tag;
    typename S::family_id;
    { S::name() } noexcept -> std::convertible_to<std::string_view>;
    { S::family_name() } noexcept -> std::convertible_to<std::string_view>;
    { S::flag_suffix() } noexcept -> std::convertible_to<std::string_view>;
    { S::enabled } -> std::convertible_to<bool>;
};

} // namespace comdare::cache_engine::serialization_axis::concepts
