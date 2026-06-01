#pragma once
// V41.F.6.1.R7.1.d axis_04 Node256NodeType (Goldstandard-Update, ART dense direct-addressed)

#include "axis_04_node_type_strategy_base.hpp"
#include "axis_04_node_type_subaxes_nt1_to_nt3.hpp"
#include "concepts/axis_04_node_type_cache_engine_permutation_concept.hpp"
#include <axes/node/axis_04_node_type_flags.hpp>
#include <topics/nodes/concepts/topic_nodes_concept.hpp>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::node {

/// Node256NodeType — ART dense node mit 256 Slots, direct-addressed.
/// Default-Variante: 1 Pointer-Lookup ohne Search (Leis ICDE 2013).
class Node256NodeType : public NodeTypeStrategyBase<Node256NodeType> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::capacity_class_tag;
    using family_id = std::integral_constant<int, 256>;

    static constexpr bool enabled = flags::node256_enabled;

    [[nodiscard]] static constexpr std::size_t      max_capacity() noexcept { return 256; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "node256"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "Node256NodeType (ART dense, direct-addressed 256-slot)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "NODE256"; }
};

}  // namespace

namespace comdare::cache_engine::node {
    static_assert(concepts::NodeTypeStrategy<Node256NodeType>);
    static_assert(concepts::CacheEnginePermutationStrategy<Node256NodeType>);
}
