#pragma once
// DOSSIER W1/234-K axis_skip_list_shape MAX8_P50

#include "axis_skip_list_shape_flags.hpp"
#include "axis_skip_list_shape_strategy_base.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_skip_list_shape {

class SkipListMax8P50 : public SkipListShapeStrategyBase<SkipListMax8P50> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = skip_list_shape_family_tag;
    using family_id = std::integral_constant<int, 8>;

    static constexpr bool enabled       = flags::max8_p50_enabled;
    static constexpr int  kMaxLevel     = 8;
    static constexpr int  kPNumerator   = 1;
    static constexpr int  kPDenominator = 2;

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "skip_list_max8_p50"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "SkipListMax8P50"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "MAX8_P50"; }
};

static_assert(concepts::SkipListShape<SkipListMax8P50>);
static_assert(concepts::CacheEnginePermutationStrategy<SkipListMax8P50>);

} // namespace comdare::cache_engine::nodes::axis_skip_list_shape