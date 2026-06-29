#pragma once
// V41.F.6.1 F.6 axis_01_page_type CacheEngine-Permutation-Concept

#include "axis_01_page_type_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::nodes::axis_01_page_type::concepts {

template <typename P>
concept CacheEnginePermutationStrategy = PageTypeStrategy<P> && requires {
    typename P::axis_tag;
    typename P::family_id;
    { P::name() } noexcept -> std::convertible_to<std::string_view>;
    { P::family_name() } noexcept -> std::convertible_to<std::string_view>;
    { P::flag_suffix() } noexcept -> std::convertible_to<std::string_view>;
    { P::enabled } -> std::convertible_to<bool>;
};

} // namespace comdare::cache_engine::nodes::axis_01_page_type::concepts
