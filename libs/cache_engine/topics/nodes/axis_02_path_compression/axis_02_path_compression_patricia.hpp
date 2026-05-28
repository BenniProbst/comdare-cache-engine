#pragma once
// V41.F.6.1.R7.1.c axis_02 PatriciaPathCompression (Single-Bit-Split, HOT/Wormhole)

#include "axis_02_path_compression_strategy_base.hpp"
#include "axis_02_path_compression_subaxes_pc1_to_pc3.hpp"
#include "concepts/axis_02_path_compression_cache_engine_permutation_concept.hpp"
#include "axis_02_path_compression_flags.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_02_path_compression {

/// PatriciaPathCompression — Single-Bit-Split (Patricia-Trie).
/// Verwendet von HOT (Binna PVLDB 2018) + Wormhole (Wu EuroSys 2019, 10.1145/3302424.3303955).
/// Vorteil: Trie-Hoehe drastisch reduziert (nur signifikante Bits).
class PatriciaPathCompression : public PathCompressionStrategyBase<PatriciaPathCompression> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::skip_strategy_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::patricia_enabled;

    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "path_compression_patricia"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "PatriciaPathCompression (single-bit-split, HOT/Wormhole)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "PATRICIA"; }

    [[nodiscard]] double compression_ratio() const noexcept { return 0.3; }  // typisch ~3x kompakter
};

}  // namespace

namespace comdare::cache_engine::nodes::axis_02_path_compression {
    static_assert(concepts::PathCompressionStrategy<PatriciaPathCompression>);
    static_assert(concepts::CacheEnginePermutationStrategy<PatriciaPathCompression>);
}
