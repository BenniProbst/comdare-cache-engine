#pragma once
// V41.F.6.1.R7.5.a axis_07_prefetch CacheEngine-Permutation-Concept

#include "axis_07_prefetch_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::prefetch_axis::concepts {

template <typename P>
concept CacheEnginePermutationStrategy = PrefetchStrategy<P> && requires {
    typename P::axis_tag;
    typename P::family_id;
    { P::name() } noexcept -> std::convertible_to<std::string_view>;
    { P::family_name() } noexcept -> std::convertible_to<std::string_view>;
    { P::flag_suffix() } noexcept -> std::convertible_to<std::string_view>;
    { P::enabled } -> std::convertible_to<bool>;
};

} // namespace comdare::cache_engine::prefetch_axis::concepts
