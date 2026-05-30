#pragma once
// V41.F.6.1.R7.5.c axis_10 VarLenSerialization (ART signaling-bits VarInt)

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

/// VarLenSerialization — Variable-Length-Encoding mit Signaling-Bits.
/// Standard fuer ART (Leis ICDE 2013): kleine Werte = wenige Bytes,
/// signaling-bits markieren Laenge. Typisch fuer integer-Schluessel.
class VarLenSerialization : public SerializationStrategyBase<VarLenSerialization> {
public:
    using topic_tag = ::comdare::cache_engine::serialization::concepts::SerializationTopicTag;
    using axis_tag  = subaxes::byte_order_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::var_len_enabled;

    [[nodiscard]] static constexpr bool             supports_compression() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "serialization_var_len"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "VarLenSerialization (ART signaling-bits VarInt encoding)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "VAR_LEN"; }

    // R5.B: behaviorale Laufzeit-API (s. RawBinarySerialization). VarLen = LEB128-VarInt: 7 Nutz-Bits je Byte,
    // MSB = Continuation-Flag (kleine Werte = wenige Bytes). Order-sensitiver FNV-Mix der emittierten Bytes —
    // datenabhängige Schleifenlänge (1–5 Bytes), echter Branch-Aufwand pro Datensatz.
    [[nodiscard]] static std::uint64_t serialize_scan(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        std::uint64_t s = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * record_size, sizeof(v));
            std::uint64_t mix = 1469598103934665603ULL;        // FNV-1a offset basis
            do {
                unsigned char byte = static_cast<unsigned char>(v & 0x7Fu);
                v >>= 7;
                if (v != 0u) byte = static_cast<unsigned char>(byte | 0x80u);  // Continuation-Bit
                mix = (mix ^ byte) * 1099511628211ULL;          // FNV-1a Mix der emittierten Bytes
            } while (v != 0u);
            s += mix;
        }
        return s;
    }
};

}  // namespace

namespace comdare::cache_engine::serialization_axis {
    static_assert(concepts::SerializationStrategy<VarLenSerialization>);
    static_assert(concepts::CacheEnginePermutationStrategy<VarLenSerialization>);
}
