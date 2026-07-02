#pragma once
// DOSSIER W1/234-K axis_btree_order KT8

#include "axis_btree_order_flags.hpp"
#include "axis_btree_order_strategy_base.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_btree_order {

class BtreeOrderKt8 : public BtreeOrderStrategyBase<BtreeOrderKt8> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = btree_order_family_tag;
    using family_id = std::integral_constant<int, 8>;

    static constexpr bool enabled      = flags::kt8_enabled;
    static constexpr int  kT           = 8;
    static constexpr int  kMaxKeys     = 2 * kT - 1; // 15
    static constexpr int  kMaxChildren = 2 * kT;     // 16

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "btree_order_kt8"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "BtreeOrderKt8"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "KT8"; }
};

static_assert(concepts::BtreeOrderShape<BtreeOrderKt8>);
static_assert(concepts::CacheEnginePermutationStrategy<BtreeOrderKt8>);

} // namespace comdare::cache_engine::nodes::axis_btree_order