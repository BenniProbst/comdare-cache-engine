#pragma once
// V41.F.6.1.R7.1.c axis_02 ByteWisePathCompression (ART path-compressed)

#include "axis_02_path_compression_strategy_base.hpp"
#include "axis_02_path_compression_subaxes_pc1_to_pc3.hpp"
#include "concepts/axis_02_path_compression_cache_engine_permutation_concept.hpp"
#include "axis_02_path_compression_flags.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_02_path_compression {

/// ByteWisePathCompression — Byte-by-Byte Path-Compression (ART).
/// Standard fuer Adaptive Radix Tree (Leis ICDE 2013). Speichert
/// gemeinsame Byte-Prefixe in Inner-Nodes, kein Single-Bit-Split.
class ByteWisePathCompression : public PathCompressionStrategyBase<ByteWisePathCompression> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::granularity_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::byte_wise_enabled;

    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "path_compression_byte_wise"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "ByteWisePathCompression (byte-by-byte, ART Leis ICDE 2013)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "BYTE_WISE"; }

    [[nodiscard]] double compression_ratio() const noexcept { return 0.5; }  // typisch ~2x kompakter
};

}  // namespace

namespace comdare::cache_engine::nodes::axis_02_path_compression {
    static_assert(concepts::PathCompressionStrategy<ByteWisePathCompression>);
    static_assert(concepts::CacheEnginePermutationStrategy<ByteWisePathCompression>);
}
