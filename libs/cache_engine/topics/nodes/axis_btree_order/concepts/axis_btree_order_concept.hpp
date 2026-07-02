#pragma once
// DOSSIER W1/234-K axis_btree_order Standard-Concept

#include "../../concepts/topic_nodes_concept.hpp"
#include <concepts>

namespace comdare::cache_engine::nodes::axis_btree_order::concepts {

template <typename S>
concept BtreeOrderShape = ::comdare::cache_engine::nodes::concepts::NodesComponent<S> && requires {
    { S::kT } -> std::convertible_to<int>;
    { S::kMaxKeys } -> std::convertible_to<int>;
    { S::kMaxChildren } -> std::convertible_to<int>;
} && (S::kT >= 2) && (S::kMaxKeys == 2 * S::kT - 1) && (S::kMaxChildren == 2 * S::kT);

} // namespace comdare::cache_engine::nodes::axis_btree_order::concepts