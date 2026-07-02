#pragma once
// DOSSIER W1/234-K axis_skip_list_shape MAX16_P50 (Level-0 Default)

#include "axis_skip_list_shape_flags.hpp"
#include "axis_skip_list_shape_strategy_base.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_skip_list_shape {

class SkipListMax16P50 : public SkipListShapeStrategyBase<SkipListMax16P50> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = skip_list_shape_family_tag;
    using family_id = std::integral_constant<int, 16>;

    static constexpr bool enabled       = flags::max16_p50_enabled;
    static constexpr int  kMaxLevel     = 16;
    static constexpr int  kPNumerator   = 1;
    static constexpr int  kPDenominator = 2;

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "skip_list_max16_p50"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "SkipListMax16P50"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "MAX16_P50"; }
};

static_assert(concepts::SkipListShape<SkipListMax16P50>);
static_assert(concepts::CacheEnginePermutationStrategy<SkipListMax16P50>);

} // namespace comdare::cache_engine::nodes::axis_skip_list_shape