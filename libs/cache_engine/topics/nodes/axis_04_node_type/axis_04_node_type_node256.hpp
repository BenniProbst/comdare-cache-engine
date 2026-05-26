#pragma once
// V41.F.6.1.F1 axis_04 Node256Type Default-Wrapper (Skelett-Stufe-A, ART-style)

#include "axis_04_node_type_base.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_04_node_type {

/// Node256Type — Default-Variante: ART Node256 dense direct-addressed.
class Node256Type : public NodeTypeBase<Node256Type> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using family_id = std::integral_constant<int, 256>;

    [[nodiscard]] static constexpr std::size_t max_capacity()  noexcept { return 256; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "node256"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "Node256Type (ART direct-addressed, Leis ICDE 2013)"; }
};

}  // namespace

namespace comdare::cache_engine::nodes::axis_04_node_type {
    static_assert(concepts::NodeTypeStrategy<Node256Type>);
}
