#pragma once
// DOSSIER W1/234-K axis_bst_shape Standard-Concept

#include "../../concepts/topic_nodes_concept.hpp"
#include <concepts>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_bst_shape::concepts {

template <typename S>
concept BstShape = ::comdare::cache_engine::nodes::concepts::NodesComponent<S> && requires {
    typename S::index_type;
    { S::kIndexBytes } -> std::convertible_to<std::size_t>;
} && std::is_unsigned_v<typename S::index_type> && (S::kIndexBytes == sizeof(typename S::index_type));

} // namespace comdare::cache_engine::nodes::axis_bst_shape::concepts