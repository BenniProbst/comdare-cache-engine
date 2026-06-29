#pragma once
// V41.F.6.1.F1 axis_04_node_type Standard-Concept (2026-05-26 Skelett-Stufe-A)

#include <topics/nodes/concepts/topic_nodes_concept.hpp>
#include <concepts>
#include <cstddef>

namespace comdare::cache_engine::node::concepts {

/**
 * @brief NodeTypeStrategy — Pflicht-API fuer Knoten-Typ-Strategien
 *
 * Konkrete Varianten: Node4/Node16/Node48/Node256 (ART), PatriciaNode (HOT),
 * BPlusPage (Masstree), HashAnchorNode (Wormhole).
 */
template <typename N>
concept NodeTypeStrategy = ::comdare::cache_engine::nodes::concepts::NodesComponent<N> && requires(N const& n) {
    { n.max_capacity() } noexcept -> std::convertible_to<std::size_t>;
};

} // namespace comdare::cache_engine::node::concepts
