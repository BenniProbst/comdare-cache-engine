#pragma once
// V41.F.6.1.R7.5.f axis_io CacheEngine-Permutation-Concept

#include "axis_io_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::io_dispatch::concepts {

template <typename I>
concept CacheEnginePermutationStrategy = IoStrategy<I> && requires {
    typename I::axis_tag;
    typename I::family_id;
    { I::name() } noexcept -> std::convertible_to<std::string_view>;
    { I::family_name() } noexcept -> std::convertible_to<std::string_view>;
    { I::flag_suffix() } noexcept -> std::convertible_to<std::string_view>;
    { I::enabled } -> std::convertible_to<bool>;
};

} // namespace comdare::cache_engine::io_dispatch::concepts
