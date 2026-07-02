#pragma once
// DOSSIER W1/234-K axis_hash_probe_shape OA_LF90

#include "axis_hash_probe_shape_flags.hpp"
#include "axis_hash_probe_shape_strategy_base.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_hash_probe_shape {

class HashOaLf90 : public HashProbeShapeStrategyBase<HashOaLf90> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = hash_probe_shape_family_tag;
    using family_id = std::integral_constant<int, 90>;

    static constexpr bool enabled          = flags::oa_lf90_enabled;
    static constexpr bool kOpenAddressing  = true;
    static constexpr int  kLoadNumerator   = 9;
    static constexpr int  kLoadDenominator = 10;

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "hash_oa_lf90"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "HashOaLf90"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "OA_LF90"; }
};

static_assert(concepts::HashProbeShape<HashOaLf90>);
static_assert(concepts::CacheEnginePermutationStrategy<HashOaLf90>);

} // namespace comdare::cache_engine::nodes::axis_hash_probe_shape