#pragma once
// DOSSIER W1/234-K axis_btree_order KT4 (Level-0 Default)

#include "axis_btree_order_flags.hpp"
#include "axis_btree_order_strategy_base.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_btree_order {

class BtreeOrderKt4 : public BtreeOrderStrategyBase<BtreeOrderKt4> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = btree_order_family_tag;
    using family_id = std::integral_constant<int, 4>;

    static constexpr bool enabled      = flags::kt4_enabled;
    static constexpr int  kT           = 4;
    static constexpr int  kMaxKeys     = 2 * kT - 1; // 7
    static constexpr int  kMaxChildren = 2 * kT;     // 8

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "btree_order_kt4"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "BtreeOrderKt4"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "KT4"; }
};

static_assert(concepts::BtreeOrderShape<BtreeOrderKt4>);
static_assert(concepts::CacheEnginePermutationStrategy<BtreeOrderKt4>);

} // namespace comdare::cache_engine::nodes::axis_btree_order