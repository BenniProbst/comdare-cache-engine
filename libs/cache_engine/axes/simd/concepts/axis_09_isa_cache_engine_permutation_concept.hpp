#pragma once
// V41.F.6.1.R7.5.i axis_09_isa CacheEngine-Permutation-Concept

#include "axis_09_isa_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::simd::concepts {

template <typename I>
concept CacheEnginePermutationStrategy =
    IsaStrategy<I>
    && requires {
        typename I::axis_tag;
        typename I::family_id;
        { I::name() }         noexcept -> std::convertible_to<std::string_view>;
        { I::family_name() }  noexcept -> std::convertible_to<std::string_view>;
        { I::flag_suffix() }  noexcept -> std::convertible_to<std::string_view>;
        { I::enabled }                 -> std::convertible_to<bool>;
    };

}  // namespace
