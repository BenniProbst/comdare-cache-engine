#pragma once
// V41.F.6.1.R7.1.d axis_04 Node48Layout (ART large node, indirect-array)

#include "axis_04_node_type_strategy_base.hpp"
#include "axis_04_node_type_subaxes_nt1_to_nt3.hpp"
#include "concepts/axis_04_node_type_cache_engine_permutation_concept.hpp"
#include <axes/node/axis_04_node_type_flags.hpp>
#include <topics/nodes/concepts/topic_nodes_concept.hpp>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::node {

/// Node48Layout — ART large node mit 48 Slots (Leis ICDE 2013).
/// Indirect-Array: 256-byte ChildIndex + 48-pointer ChildArray.
/// Trade-off: Speicher vs Cache-Line-Misses (vs Node256 direct).
class Node48Layout : public NodeTypeStrategyBase<Node48Layout> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::compactness_tag;
    using family_id = std::integral_constant<int, 48>;

    static constexpr bool enabled = flags::node48_enabled;

    [[nodiscard]] static constexpr std::size_t      max_capacity() noexcept { return 48; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "node48"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "Node48Layout (ART large, indirect-array 48-slot)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "NODE48"; }
};

}  // namespace

namespace comdare::cache_engine::node {
    static_assert(concepts::NodeTypeStrategy<Node48Layout>);
    static_assert(concepts::CacheEnginePermutationStrategy<Node48Layout>);
}
