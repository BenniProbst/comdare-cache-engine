#pragma once
// DOSSIER W1/234-K axis_skip_list_shape MAX16_P25

#include "axis_skip_list_shape_flags.hpp"
#include "axis_skip_list_shape_strategy_base.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_skip_list_shape {

class SkipListMax16P25 : public SkipListShapeStrategyBase<SkipListMax16P25> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = skip_list_shape_family_tag;
    using family_id = std::integral_constant<int, 25>;

    static constexpr bool enabled       = flags::max16_p25_enabled;
    static constexpr int  kMaxLevel     = 16;
    static constexpr int  kPNumerator   = 1;
    static constexpr int  kPDenominator = 4;

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "skip_list_max16_p25"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "SkipListMax16P25"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "MAX16_P25"; }
};

static_assert(concepts::SkipListShape<SkipListMax16P25>);
static_assert(concepts::CacheEnginePermutationStrategy<SkipListMax16P25>);

} // namespace comdare::cache_engine::nodes::axis_skip_list_shape