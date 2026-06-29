#pragma once
// V41.F.6.1.R7.5.j axis_09b_simd_extension CacheEngine-Permutation-Concept

#include "axis_09b_simd_extension_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::hardware::axis_09b_simd_extension::concepts {

template <typename E>
concept CacheEnginePermutationStrategy = SimdExtensionStrategy<E> && requires {
    typename E::axis_tag;
    typename E::family_id;
    { E::name() } noexcept -> std::convertible_to<std::string_view>;
    { E::family_name() } noexcept -> std::convertible_to<std::string_view>;
    { E::flag_suffix() } noexcept -> std::convertible_to<std::string_view>;
    { E::enabled } -> std::convertible_to<bool>;
};

} // namespace comdare::cache_engine::hardware::axis_09b_simd_extension::concepts
