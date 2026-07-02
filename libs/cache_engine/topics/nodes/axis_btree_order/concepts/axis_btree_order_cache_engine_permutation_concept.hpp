#pragma once
// DOSSIER W1/234-K axis_btree_order CacheEngine-Permutation-Concept

#include "axis_btree_order_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::nodes::axis_btree_order::concepts {

template <typename S>
concept CacheEnginePermutationStrategy = BtreeOrderShape<S> && requires {
    typename S::axis_tag;
    typename S::family_id;
    { S::name() } noexcept -> std::convertible_to<std::string_view>;
    { S::family_name() } noexcept -> std::convertible_to<std::string_view>;
    { S::flag_suffix() } noexcept -> std::convertible_to<std::string_view>;
    { S::enabled } -> std::convertible_to<bool>;
};

} // namespace comdare::cache_engine::nodes::axis_btree_order::concepts