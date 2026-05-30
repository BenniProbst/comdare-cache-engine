#pragma once
// V41.F.6.1.R7.1.d axis_04 Node4Layout (ART small node, 4 slots)

#include "axis_04_node_type_strategy_base.hpp"
#include "axis_04_node_type_subaxes_nt1_to_nt3.hpp"
#include "concepts/axis_04_node_type_cache_engine_permutation_concept.hpp"
#include <axes/node/axis_04_node_type_flags.hpp>
#include <topics/nodes/concepts/topic_nodes_concept.hpp>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::node {

/// Node4Layout — ART small node mit 4 Slots (Leis ICDE 2013).
/// Linear-Search optimal fuer kleine Node-Fanouts (Cache-Line passend).
class Node4Layout : public NodeTypeStrategyBase<Node4Layout> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::capacity_class_tag;
    using family_id = std::integral_constant<int, 4>;

    static constexpr bool enabled = flags::node4_enabled;

    [[nodiscard]] static constexpr std::size_t      max_capacity() noexcept { return 4; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "node4"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "Node4Layout (ART small node, 4-slot linear-search)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "NODE4"; }
};

}  // namespace

namespace comdare::cache_engine::node {
    static_assert(concepts::NodeTypeStrategy<Node4Layout>);
    static_assert(concepts::CacheEnginePermutationStrategy<Node4Layout>);
}
