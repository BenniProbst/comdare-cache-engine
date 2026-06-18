#pragma once
// V41.F.6.1.R7.5.e axis_filter BloomFilter (Goldstandard-Update, Bloom 1970)
//
// R7.6 Paper-Reference (Task #723):
// Bloom, B.H. "Space/Time Trade-offs in Hash Coding with Allowable Errors."
// Communications of the ACM, Vol. 13, No. 7, July 1970, pp. 422-426.
// DOI: 10.1145/362686.362692
// Klassisches Paper, kein Open-Source-Original (Re-Impl basierend auf
// Pseudocode in Paper §3).

#include "axis_filter_strategy_base.hpp"
#include "axis_filter_subaxes_ft1_to_ft3.hpp"
#include "concepts/axis_filter_cache_engine_permutation_concept.hpp"
#include <axes/filter_axis/axis_filter_flags.hpp>
#include <topics/filter/concepts/topic_filter_concept.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::filter_axis {

/// BloomFilter — Bloom 1970 classical point-query filter.
/// k Hash-Funktionen, m-bit Bitmap. False-positive only, no delete.
class BloomFilter : public FilterStrategyBase<BloomFilter> {
public:
    using topic_tag = ::comdare::cache_engine::filter::concepts::FilterTopicTag;
    using axis_tag  = subaxes::query_type_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::bloom_enabled;

    // ── P5 (#124, 2026-06-04, User §4.3) — REALE, aus den eingefuegten Keys gebaute Bloom-Bitmap ──────────────
    // Eine ECHTE persistente m-bit-Bitmap (kein Pseudo-Puffer mehr): insert_key(key) setzt die k Hash-Bits beim
    // Einfuegen (Build-Phase, in tier_insert — NICHT gemessen), probe_key(key) prueft die k Hash-Bits gegen die
    // REALE Bitmap. Klassisches Bloom 1970 (k=4 Hash-Funktionen via double-hashing Kirsch/Mitzenmacher 2006).
    // kBytes=8192 (8 KiB) → L1-resident (messneutral: gleiche Cache-Stufe wie der frühere Pseudo-Puffer; die
    // gemessene Probe-Kost = k Bit-Tests bleibt unveraendert). Voll-uint64-Key-Domain (kein 1-Byte-Truncate).
    static constexpr std::size_t kBitmapBytes = 8192;          // 8 KiB Bitmap → m = 65536 Bit
    static constexpr std::size_t kHashes      = 4;             // Bloom 1970: k unabhaengige Hash-Funktionen

    /// Bit-Positionen-Paar (double-hashing): liefert h1 + i*h2 mod m fuer i=0..k-1.
    [[nodiscard]] static constexpr std::size_t bit_pos_(std::uint64_t key, std::size_t i) noexcept {
        std::uint64_t const h1 = key * 2654435761ull + 0x9E3779B97F4A7C15ull;
        std::uint64_t const h2 = (key << 7) ^ (key >> 3) ^ 0x85EBCA6B12345678ull;
        return static_cast<std::size_t>((h1 + static_cast<std::uint64_t>(i) * h2) % (kBitmapBytes * 8ull));
    }

    /// Build-Op (Setup, NICHT gemessen): setzt die k Bits des Keys in der REALEN Bitmap.
    void insert_key(std::uint64_t key) noexcept {
        for (std::size_t i = 0; i < kHashes; ++i) {
            std::size_t const bp = bit_pos_(key, i);
            bitmap_[bp >> 3] |= static_cast<unsigned char>(1u << (bp & 7u));
        }
    }

    /// Probe der REALEN Bitmap: alle k Bits gesetzt → "moeglicherweise enthalten" (Bloom: false-positive only).
    [[nodiscard]] bool probe_key(std::uint64_t key) const noexcept {
        for (std::size_t i = 0; i < kHashes; ++i) {
            std::size_t const bp = bit_pos_(key, i);
            if ((bitmap_[bp >> 3] & static_cast<unsigned char>(1u << (bp & 7u))) == 0) return false;
        }
        return true;
    }

    void clear() noexcept { bitmap_.fill(0); }
    [[nodiscard]] bool operator==(BloomFilter const& o) const noexcept { return bitmap_ == o.bitmap_; }

    /// Probe-Multiplizitaet je Query (k Bit-Tests) — ehrlich deklariert fuer hash_probes_total.
    [[nodiscard]] static constexpr std::uint64_t probe_multiplicity() noexcept { return kHashes; }

    [[nodiscard]] static constexpr bool             supports_range_query() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "filter_bloom"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "BloomFilter (Bloom 1970, k-hash bitmap, point-query)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "BLOOM"; }

    // F15-Pfad-A Treibe-Op (Spec §5 T16, Goldstandard analog axis_05 scan_field_sum / axis_10
    // serialize_scan / axis_04 node_find_scan). SYNTHETISCHE Mindest-Op (ehrlich deklariert): es
    // existiert in diesem Wrapper KEINE persistente Bitmap-Instanz, daher wird `buf` als Pseudo-Bitmap
    // der Länge `n` Bytes interpretiert und je Query strategie-charakteristisch geprobt. Der Aufwand ist
    // REAL und strategie-abhängig (Bloom probt k unabhängige Hash-Positionen → k Bit-Tests/Query), KEINE
    // erfundene konstante Zahl. Bloom 1970: k=4 Hash-Funktionen (double-hashing Kirsch/Mitzenmacher 2006:
    // h_i = h1 + i*h2), m-bit Bitmap = n*8 Bits. Liefert query-order-sensitive Treffer-Prüfsumme
    // (Anti-Wegoptimierung). Punkt-Query only (kein Range).
    [[nodiscard]] static std::uint64_t filter_probe_scan(unsigned char const* buf, std::size_t n,
                                                         unsigned char const* queries, std::size_t q) noexcept {
        if (n == 0) return 0;
        constexpr std::size_t kHashes = 4;                 // Bloom 1970: k unabhängige Hash-Funktionen
        std::size_t const mBits = n * 8u;                  // Pseudo-Bitmap m = n Bytes * 8 Bit
        std::uint64_t hits = 0;
        for (std::size_t i = 0; i < q; ++i) {
            std::uint32_t const key = queries[i];           // 1 Byte je Query als Schlüssel
            // double-hashing (Kirsch/Mitzenmacher 2006): h1 = FNV-artig, h2 = Rotation
            std::uint32_t h1 = key * 2654435761u + 0x9E3779B9u;
            std::uint32_t h2 = (key << 5) ^ (key >> 2) ^ 0x85EBCA6Bu;
            bool all_set = true;
            for (std::size_t k = 0; k < kHashes; ++k) {     // k Hash-Positionen prüfen
                std::size_t const bitpos = (h1 + k * h2) % mBits;
                unsigned char const byte = buf[bitpos >> 3];
                if ((byte & (1u << (bitpos & 7u))) == 0) {  // ein 0-Bit → definitiv NICHT enthalten
                    all_set = false;
                    break;                                   // Bloom: früher Abbruch (echte Latenz-Divergenz)
                }
            }
            if (all_set) hits += 1u + (key & 7u);           // order-sensitive Akkumulation
        }
        return hits;
    }

private:
    // REALE persistente Bloom-Bitmap (P5 #124). std::array → trivially copyable → Strategie kopierbar (Memento).
    std::array<unsigned char, kBitmapBytes> bitmap_{};
};

}  // namespace

namespace comdare::cache_engine::filter_axis {
    static_assert(concepts::FilterStrategy<BloomFilter>);
    static_assert(concepts::CacheEnginePermutationStrategy<BloomFilter>);
}
