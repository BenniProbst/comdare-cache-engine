#pragma once
// V41.F.6.1.R7.1.c axis_02 ByteWisePathCompression (ART path-compressed)

#include "axis_02_path_compression_strategy_base.hpp"
#include "axis_02_path_compression_subaxes_pc1_to_pc3.hpp"
#include "concepts/axis_02_path_compression_cache_engine_permutation_concept.hpp"
#include "concepts/axis_02_path_compression_concept.hpp"   // ByteSkipPathCompression (#43 s4)
#include <axes/path_compression/axis_02_path_compression_flags.hpp>
#include <topics/nodes/concepts/topic_nodes_concept.hpp>
#include <bit>          // std::countr_zero
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::path_compression {

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

/// ByteWiseKeyPrefix — das ECHTE Byte-Prefix-Organ einer ART-Trie-Inner-Node (V41 #43 s4, Leis ICDE 2013).
///
/// Originalgetreue Re-Impl des gepackten `unodb::key_prefix` (art_internal_impl.hpp:877-1070, Apache-2.0,
/// is_original=false [[pseudocode-papers-fallback]]): EIN std::uint64_t haelt bis zu 7 Prefix-Bytes (Byte i
/// in Bits [i*8 .. i*8+7], also Byte 0 = niederwertigstes) + die Laenge im High-Byte (Bits 56-63). So liegt
/// das komprimierte Inner-Node-Prefix cache-line-freundlich in einem Wort. Wird vom Trie-Traversal-Organ
/// (Folge-Increment) beim Byte-Descent konsumiert (Prefix-Skip via common_prefix_len + cut). Technischer
/// Identifier ([[technical-identifiers-over-metaphor]]); Tier-Metapher nur im Kommentar.
struct ByteWiseKeyPrefix {
    static constexpr unsigned      kCapacity     = 7;                       // max. Prefix-Bytes (Low 7 Bytes)
    static constexpr std::uint64_t kKeyBytesMask = 0x00FF'FFFF'FFFF'FFFFULL;

    std::uint64_t packed_ = 0;   // Low 7 Bytes = Prefix-Bytes (Byte 0 = LSB), High-Byte = Laenge

    constexpr ByteWiseKeyPrefix() noexcept = default;
    explicit constexpr ByteWiseKeyPrefix(std::uint64_t packed) noexcept : packed_(packed) {}

    /// Laenge -> 64-Bit-Wort mit Laenge im High-Byte (verbatim length_to_word, Z.1038).
    [[nodiscard]] static constexpr std::uint64_t length_to_word(unsigned len) noexcept {
        return static_cast<std::uint64_t>(len) << 56U;
    }
    /// Konstruktion aus byteweise gepackten Prefix-Bytes (Byte 0 = LSB) + Laenge.
    [[nodiscard]] static constexpr ByteWiseKeyPrefix from_bytes(std::uint64_t prefix_bytes, unsigned len) noexcept {
        return ByteWiseKeyPrefix{(prefix_bytes & kKeyBytesMask) | length_to_word(len)};
    }

    [[nodiscard]] constexpr unsigned length() const noexcept {
        return static_cast<unsigned>(packed_ >> 56U);
    }
    [[nodiscard]] constexpr std::uint8_t operator[](unsigned i) const noexcept {
        return static_cast<std::uint8_t>((packed_ >> (i * 8U)) & 0xFFU);
    }

    /// Gemeinsame Low-Byte-Laenge zweier u64, geklemmt auf clamp_byte_pos (verbatim shared_len, Z.1045-1052).
    [[nodiscard]] static constexpr unsigned shared_len(std::uint64_t k1, std::uint64_t k2,
                                                       unsigned clamp_byte_pos) noexcept {
        const std::uint64_t diff    = k1 ^ k2;
        const std::uint64_t clamped = diff | (1ULL << (clamp_byte_pos * 8U));   // begrenzt Ergebnis auf clamp
        return static_cast<unsigned>(std::countr_zero(clamped) >> 3U);          // gemeinsame BYTES
    }
    /// Gemeinsame Prefix-Laenge dieses Prefix mit einem (bereits geshifteten) Schluessel — geklemmt auf length().
    /// (Die Laenge im High-Byte von packed_ liegt ueber dem Clamp-Bit und beeinflusst das Ergebnis NICHT.)
    [[nodiscard]] constexpr unsigned common_prefix_len(std::uint64_t shifted_key) const noexcept {
        return shared_len(shifted_key, packed_, length());
    }

    /// Fuehrende cut_len Bytes entfernen (Abstieg, verbatim cut, Z.967-975). cut_len in (0, length()].
    constexpr void cut(unsigned cut_len) noexcept {
        packed_ = ((packed_ >> (cut_len * 8U)) & kKeyBytesMask)
                  | length_to_word(length() - cut_len);
    }
    /// Ergebnis = prefix1 + prefix2 + dieses Prefix (Knoten-Kollaps, verbatim prepend, Z.983-1001).
    constexpr void prepend(ByteWiseKeyPrefix const& prefix1, std::uint8_t prefix2) noexcept {
        const unsigned      p1_bits         = prefix1.length() * 8U;
        const std::uint64_t p1_mask         = (1ULL << p1_bits) - 1ULL;
        const unsigned      p3_bits         = length() * 8U;
        const std::uint64_t p3_mask         = (1ULL << p3_bits) - 1ULL;
        const std::uint64_t prefix3         = packed_ & p3_mask;
        const std::uint64_t shifted_prefix3 = prefix3 << (p1_bits + 8U);
        const std::uint64_t shifted_prefix2 = static_cast<std::uint64_t>(prefix2) << p1_bits;
        const std::uint64_t masked_prefix1  = prefix1.packed_ & p1_mask;
        packed_ = shifted_prefix3 | shifted_prefix2 | masked_prefix1
                  | length_to_word(length() + prefix1.length() + 1U);
    }
};

}  // namespace

namespace comdare::cache_engine::path_compression {
    static_assert(concepts::PathCompressionStrategy<ByteWisePathCompression>);
    static_assert(concepts::CacheEnginePermutationStrategy<ByteWisePathCompression>);
    // V41 #43 s4: das echte Prefix-Organ erfuellt das (separate, additive) Byte-Skip-Concept.
    static_assert(concepts::ByteSkipPathCompression<ByteWiseKeyPrefix>);
}
