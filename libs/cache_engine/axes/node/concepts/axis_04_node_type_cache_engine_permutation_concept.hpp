#pragma once
// V41.F.6.1.R7.1.d axis_04_node_type CacheEngine-Permutation-Concept

#include "axis_04_node_type_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::node::concepts {

template <typename N>
concept CacheEnginePermutationStrategy = NodeTypeStrategy<N> && requires {
    typename N::axis_tag;
    typename N::family_id;
    { N::name() } noexcept -> std::convertible_to<std::string_view>;
    { N::family_name() } noexcept -> std::convertible_to<std::string_view>;
    { N::flag_suffix() } noexcept -> std::convertible_to<std::string_view>;
    { N::enabled } -> std::convertible_to<bool>;
};

} // namespace comdare::cache_engine::node::concepts
