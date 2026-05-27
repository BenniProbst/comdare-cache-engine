#pragma once
// V41.F.6.1.R7.5.c axis_10 CompressedSerialization (lz4/snappy)

#include "axis_10_serialization_strategy_base.hpp"
#include "axis_10_serialization_subaxes_sr1_to_sr3.hpp"
#include "concepts/axis_10_serialization_cache_engine_permutation_concept.hpp"
#include "axis_10_serialization_flags.hpp"
#include "../concepts/topic_serialization_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::serialization::axis_10_serialization {

/// CompressedSerialization — General-purpose Block-Compression (lz4/snappy).
/// Wird auf RawBinary aufgesetzt: serialize → compress(block).
/// Trade-off: CPU-Cost vs IO/Memory-Bandwidth (typisch LSM-Trees).
class CompressedSerialization : public SerializationStrategyBase<CompressedSerialization> {
public:
    using topic_tag = ::comdare::cache_engine::serialization::concepts::SerializationTopicTag;
    using axis_tag  = subaxes::compression_tag;
    using family_id = std::integral_constant<int, 4>;

    static constexpr bool enabled = flags::compressed_enabled;

    [[nodiscard]] static constexpr bool             supports_compression() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "serialization_compressed"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "CompressedSerialization (lz4/snappy block-compress on raw)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "COMPRESSED"; }
};

}  // namespace

namespace comdare::cache_engine::serialization::axis_10_serialization {
    static_assert(concepts::SerializationStrategy<CompressedSerialization>);
    static_assert(concepts::CacheEnginePermutationStrategy<CompressedSerialization>);
}
