#pragma once
// V41.F.6.1.R7.5.e axis_filter CacheEngine-Permutation-Concept

#include "axis_filter_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::filter_axis::concepts {

template <typename F>
concept CacheEnginePermutationStrategy =
    FilterStrategy<F>
    && requires {
        typename F::axis_tag;
        typename F::family_id;
        { F::name() }         noexcept -> std::convertible_to<std::string_view>;
        { F::family_name() }  noexcept -> std::convertible_to<std::string_view>;
        { F::flag_suffix() }  noexcept -> std::convertible_to<std::string_view>;
        { F::enabled }                 -> std::convertible_to<bool>;
    };

}  // namespace
