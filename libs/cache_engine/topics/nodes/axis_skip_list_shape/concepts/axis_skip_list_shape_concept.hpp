#pragma once
// DOSSIER W1/234-K axis_skip_list_shape Standard-Concept

#include "../../concepts/topic_nodes_concept.hpp"
#include <concepts>

namespace comdare::cache_engine::nodes::axis_skip_list_shape::concepts {

template <typename S>
concept SkipListShape = ::comdare::cache_engine::nodes::concepts::NodesComponent<S> && requires {
    { S::kMaxLevel } -> std::convertible_to<int>;
    { S::kPNumerator } -> std::convertible_to<int>;
    { S::kPDenominator } -> std::convertible_to<int>;
}
&& (S::kMaxLevel > 0) && (S::kPNumerator > 0) && (S::kPDenominator > 0) && (S::kPNumerator < S::kPDenominator);

} // namespace comdare::cache_engine::nodes::axis_skip_list_shape::concepts