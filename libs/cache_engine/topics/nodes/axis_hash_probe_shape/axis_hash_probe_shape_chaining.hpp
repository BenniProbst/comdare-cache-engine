#pragma once
// DOSSIER W1/234-K axis_hash_probe_shape CHAINING

#include "axis_hash_probe_shape_flags.hpp"
#include "axis_hash_probe_shape_strategy_base.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_hash_probe_shape {

class HashChaining : public HashProbeShapeStrategyBase<HashChaining> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = hash_probe_shape_family_tag;
    using family_id = std::integral_constant<int, 100>;

    static constexpr bool enabled          = flags::chaining_enabled;
    static constexpr bool kOpenAddressing  = false;
    static constexpr int  kLoadNumerator   = 1;
    static constexpr int  kLoadDenominator = 1;

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "hash_chaining"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "HashChaining"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "CHAINING"; }
};

static_assert(concepts::HashProbeShape<HashChaining>);
static_assert(concepts::CacheEnginePermutationStrategy<HashChaining>);

} // namespace comdare::cache_engine::nodes::axis_hash_probe_shape