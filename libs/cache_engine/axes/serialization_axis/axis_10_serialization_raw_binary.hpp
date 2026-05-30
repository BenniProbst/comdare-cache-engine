#pragma once
// V41.F.6.1.R7.5.c axis_10 RawBinarySerialization (Goldstandard-Update)

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

/// RawBinarySerialization — Default: memcpy raw bytes (baseline).
class RawBinarySerialization : public SerializationStrategyBase<RawBinarySerialization> {
public:
    using topic_tag = ::comdare::cache_engine::serialization::concepts::SerializationTopicTag;
    using axis_tag  = subaxes::byte_order_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::raw_binary_enabled;

    [[nodiscard]] static constexpr bool             supports_compression() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "serialization_raw_binary"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "RawBinarySerialization (memcpy raw bytes baseline)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "RAW_BINARY"; }

    // R5.B (Doku 22 §3.2/§4): behaviorale Laufzeit-API — macht die serialization-Achse RUNTIME-OPERATIV
    // (nicht mehr trait-only), analog axis_05 `scan_field_sum`. Encodiert je Datensatz das 32-Bit-Feld im
    // STRATEGIE-CHARAKTERISTISCHEN Aufwand + liefert eine Order-sensitive Prüfsumme (Anti-Wegoptimierung).
    // RawBinary = Baseline: bulk-memcpy der Roh-Bytes, KEINE Transformation (minimaler Aufwand).
    [[nodiscard]] static std::uint64_t serialize_scan(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        std::uint64_t s = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * record_size, sizeof(v));
            s += v;
        }
        return s;
    }
};

}  // namespace

namespace comdare::cache_engine::serialization_axis {
    static_assert(concepts::SerializationStrategy<RawBinarySerialization>);
    static_assert(concepts::CacheEnginePermutationStrategy<RawBinarySerialization>);
}
