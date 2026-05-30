#pragma once
// V41.F.6.1.R7.1.d axis_04 Node16Layout (ART medium node, SIMD-binary-search)

#include "axis_04_node_type_strategy_base.hpp"
#include "axis_04_node_type_subaxes_nt1_to_nt3.hpp"
#include "concepts/axis_04_node_type_cache_engine_permutation_concept.hpp"
#include <axes/node/axis_04_node_type_flags.hpp>
#include <topics/nodes/concepts/topic_nodes_concept.hpp>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::node {

/// Node16Layout — ART medium node mit 16 Slots (Leis ICDE 2013).
/// SIMD-binary-search optimal (SSE2 PCMPESTRM / AVX2 VPCMPESTRM).
class Node16Layout : public NodeTypeStrategyBase<Node16Layout> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::access_pattern_tag;
    using family_id = std::integral_constant<int, 16>;

    static constexpr bool enabled = flags::node16_enabled;

    [[nodiscard]] static constexpr std::size_t      max_capacity() noexcept { return 16; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "node16"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "Node16Layout (ART medium, SIMD-binary-search 16-slot)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "NODE16"; }
};

}  // namespace

namespace comdare::cache_engine::node {
    static_assert(concepts::NodeTypeStrategy<Node16Layout>);
    static_assert(concepts::CacheEnginePermutationStrategy<Node16Layout>);
}
