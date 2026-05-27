#pragma once
// V41.F.6.1.R7.1.c axis_02 PathCompressionNone (Goldstandard-Update)

#include "axis_02_path_compression_strategy_base.hpp"
#include "axis_02_path_compression_subaxes_pc1_to_pc3.hpp"
#include "concepts/axis_02_path_compression_cache_engine_permutation_concept.hpp"
#include "axis_02_path_compression_flags.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_02_path_compression {

/// PathCompressionNone — Default: keine Compression (raw path).
class PathCompressionNone : public PathCompressionStrategyBase<PathCompressionNone> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::granularity_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::none_enabled;

    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "path_compression_none"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "PathCompressionNone (raw path, no compression)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "NONE"; }

    [[nodiscard]] double compression_ratio() const noexcept { return 1.0; }  // raw = 1:1
};

}  // namespace

namespace comdare::cache_engine::nodes::axis_02_path_compression {
    static_assert(concepts::PathCompressionStrategy<PathCompressionNone>);
    static_assert(concepts::CacheEnginePermutationStrategy<PathCompressionNone>);
}
