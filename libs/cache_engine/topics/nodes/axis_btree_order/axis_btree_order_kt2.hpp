#pragma once
// DOSSIER W1/234-K axis_btree_order KT2

#include "axis_btree_order_flags.hpp"
#include "axis_btree_order_strategy_base.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_btree_order {

class BtreeOrderKt2 : public BtreeOrderStrategyBase<BtreeOrderKt2> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = btree_order_family_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled      = flags::kt2_enabled;
    static constexpr int  kT           = 2;
    static constexpr int  kMaxKeys     = 2 * kT - 1; // 3
    static constexpr int  kMaxChildren = 2 * kT;     // 4

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "btree_order_kt2"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "BtreeOrderKt2"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "KT2"; }
};

static_assert(concepts::BtreeOrderShape<BtreeOrderKt2>);
static_assert(concepts::CacheEnginePermutationStrategy<BtreeOrderKt2>);

} // namespace comdare::cache_engine::nodes::axis_btree_order