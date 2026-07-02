#pragma once
// DOSSIER W1/234-K axis_hash_probe_shape Standard-Concept

#include "../../concepts/topic_nodes_concept.hpp"
#include <concepts>

namespace comdare::cache_engine::nodes::axis_hash_probe_shape::concepts {

template <typename S>
concept HashProbeShape = ::comdare::cache_engine::nodes::concepts::NodesComponent<S> && requires {
    { S::kOpenAddressing } -> std::convertible_to<bool>;
    { S::kLoadNumerator } -> std::convertible_to<int>;
    { S::kLoadDenominator } -> std::convertible_to<int>;
} && (S::kLoadNumerator > 0) && (S::kLoadDenominator > 0) && (S::kLoadNumerator <= S::kLoadDenominator);

} // namespace comdare::cache_engine::nodes::axis_hash_probe_shape::concepts