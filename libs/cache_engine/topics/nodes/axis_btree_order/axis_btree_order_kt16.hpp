#pragma once
// DOSSIER W1/234-K axis_btree_order KT16

#include "axis_btree_order_flags.hpp"
#include "axis_btree_order_strategy_base.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_btree_order {

class BtreeOrderKt16 : public BtreeOrderStrategyBase<BtreeOrderKt16> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = btree_order_family_tag;
    using family_id = std::integral_constant<int, 16>;

    static constexpr bool enabled      = flags::kt16_enabled;
    static constexpr int  kT           = 16;
    static constexpr int  kMaxKeys     = 2 * kT - 1; // 31
    static constexpr int  kMaxChildren = 2 * kT;     // 32

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "btree_order_kt16"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "BtreeOrderKt16"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "KT16"; }
};

static_assert(concepts::BtreeOrderShape<BtreeOrderKt16>);
static_assert(concepts::CacheEnginePermutationStrategy<BtreeOrderKt16>);

} // namespace comdare::cache_engine::nodes::axis_btree_order