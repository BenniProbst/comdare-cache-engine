#pragma once
// V41.F.6.1.R7.5.c axis_10 CompressedSerialization (lz4/snappy)

#include "axis_10_serialization_strategy_base.hpp"
#include "axis_10_serialization_subaxes_sr1_to_sr3.hpp"
#include "concepts/axis_10_serialization_cache_engine_permutation_concept.hpp"
#include <axes/serialization_axis/axis_10_serialization_flags.hpp>
#include <topics/serialization/concepts/topic_serialization_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::serialization_axis {

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
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "serialization_compressed"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "CompressedSerialization (lz4/snappy block-compress on raw)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "COMPRESSED"; }

    // R5.B: behaviorale Laufzeit-API (s. RawBinarySerialization). Compressed = Delta-Encoding gegen den
    // Vorgängerwert + Zigzag-Transform (negatives Delta → kleine unsigned) — der typische Kompressions-
    // Vorverarbeitungs-Aufwand (mehr CPU als Roh-memcpy, weniger als Bit-Packing/Varint).
    [[nodiscard]] static std::uint64_t serialize_scan(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        std::uint64_t s    = 0;
        std::uint32_t prev = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * record_size, sizeof(v));
            std::int64_t const  delta = static_cast<std::int64_t>(v) - static_cast<std::int64_t>(prev);
            std::uint64_t const zig   = static_cast<std::uint64_t>((delta << 1) ^ (delta >> 63)); // zigzag
            prev                      = v;
            s += zig;
        }
        return s;
    }
};

} // namespace comdare::cache_engine::serialization_axis

namespace comdare::cache_engine::serialization_axis {
static_assert(concepts::SerializationStrategy<CompressedSerialization>);
static_assert(concepts::CacheEnginePermutationStrategy<CompressedSerialization>);
} // namespace comdare::cache_engine::serialization_axis
