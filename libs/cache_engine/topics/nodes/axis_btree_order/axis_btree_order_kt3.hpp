#pragma once
// DOSSIER W1/234-K axis_btree_order KT3

#include "axis_btree_order_flags.hpp"
#include "axis_btree_order_strategy_base.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_btree_order {

class BtreeOrderKt3 : public BtreeOrderStrategyBase<BtreeOrderKt3> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = btree_order_family_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled      = flags::kt3_enabled;
    static constexpr int  kT           = 3;
    static constexpr int  kMaxKeys     = 2 * kT - 1; // 5
    static constexpr int  kMaxChildren = 2 * kT;     // 6

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "btree_order_kt3"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "BtreeOrderKt3"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "KT3"; }
};

static_assert(concepts::BtreeOrderShape<BtreeOrderKt3>);
static_assert(concepts::CacheEnginePermutationStrategy<BtreeOrderKt3>);

} // namespace comdare::cache_engine::nodes::axis_btree_order