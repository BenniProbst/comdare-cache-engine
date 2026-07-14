#pragma once
// V41.F.6.1.R7.5.e axis_filter XorFilter (Graf/Lemire 2020)
//
// R7.6 Paper-Reference (Task #723):
// Graf, T.M., Lemire, D. "Xor Filters: Faster and Smaller Than Bloom and
// Cuckoo Filters." ACM Journal of Experimental Algorithmics (JEA), Vol. 25,
// Article No. 1.5, March 2020.
// DOI: 10.1145/3376122
// URL (Preprint): https://arxiv.org/abs/1912.08258
// Code: https://github.com/FastFilter/fastfilter_cpp (Apache-2.0 License)
// Original-Code-File: src/xorfilter/xorfilter.h

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

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::filter_axis {

/// XorFilter — Graf/Lemire 2020 (JEA, Apache-2.0 License).
/// XOR-Hash-Based Filter mit kleinerer Space-Bound als Bloom (~9 bits/key
/// statt ~10 bits/key bei gleicher false-positive-Rate). Immutable nach Build.
class XorFilter : public FilterStrategyBase<XorFilter> {
public:
    using topic_tag = ::comdare::cache_engine::filter::concepts::FilterTopicTag;
    using axis_tag  = subaxes::error_profile_tag;
    using family_id = std::integral_constant<int, 4>;

    static constexpr bool enabled = flags::xor_enabled;

    // ── P5 (#124, 2026-06-04, User §4.3) — REALE, aus den eingefuegten Keys gebaute XOR-Fingerprint-Tabelle ───
    // Eine ECHTE persistente 3-geteilte Fingerprint-Tabelle: probe_key prueft die Xor-Invariante
    // fp(key) == B[h0] XOR B[h1] XOR B[h2] (GENAU 3 Slot-Reads + 3-fach-XOR, branch-frei — Graf+Lemire 2020 §3).
    // insert_key STELLT die Invariante her, indem es den dritten Slot loest: B[h2] = fp XOR B[h0] XOR B[h1]
    // (h0/h1 aus disjunkten Dritteln; h2 aus dem dritten Drittel). Das ist eine LEICHTGEWICHTIGE XOR-Konstruktion
    // OHNE das vollstaendige Peeling/3-Hypergraph-Matching des Original-Xor-Filters — ehrlich deklariert: bei
    // h2-Kollisionen ueberschreibt ein spaeterer Key den geloesten Slot eines frueheren (mogliche FN; bewusste
    // Apparat-Vereinfachung. Der rigorose Membership-Beleg laeuft ueber Bloom/Cuckoo). kBytes=12288 → 3×4096
    // Slots, L1-resident (messneutral). Voll-uint64-Key-Domain.
    static constexpr std::size_t kThird = 4096;        // Slots je Drittel (Zweierpotenz)
    static constexpr std::size_t kSlots = 3u * kThird; // 12 KiB Gesamt-Tabelle

    [[nodiscard]] static constexpr std::uint8_t fingerprint_(std::uint64_t key) noexcept {
        return static_cast<std::uint8_t>((key * 0x9E3779B97F4A7C15ull >> 40) ^ 0xA5u);
    }
    [[nodiscard]] static constexpr std::size_t h0_(std::uint64_t key) noexcept {
        return static_cast<std::size_t>((key * 2654435761ull) & (kThird - 1u));
    }
    [[nodiscard]] static constexpr std::size_t h1_(std::uint64_t key) noexcept {
        return kThird + static_cast<std::size_t>((key * 40503ull) & (kThird - 1u));
    }
    [[nodiscard]] static constexpr std::size_t h2_(std::uint64_t key) noexcept {
        return 2u * kThird + static_cast<std::size_t>((key * 0x85EBCA6B12345678ull >> 16) & (kThird - 1u));
    }

    /// Build-Op (Setup, NICHT gemessen): loest den dritten Slot, damit die Xor-Invariante fuer key gilt.
    void insert_key(std::uint64_t key) noexcept {
        std::uint8_t const fp = fingerprint_(key);
        table_[h2_(key)]      = static_cast<std::uint8_t>(fp ^ table_[h0_(key)] ^ table_[h1_(key)]);
    }

    /// Probe der REALEN Tabelle: GENAU 3 Slot-Reads + 3-fach-XOR == Fingerprint (branch-frei).
    [[nodiscard]] bool probe_key(std::uint64_t key) const noexcept {
        std::uint8_t const x = static_cast<std::uint8_t>(table_[h0_(key)] ^ table_[h1_(key)] ^ table_[h2_(key)]);
        return x == fingerprint_(key);
    }

    void               clear() noexcept { table_.fill(0); }
    [[nodiscard]] bool operator==(XorFilter const& o) const noexcept { return table_ == o.table_; }

    /// Probe-Multiplizitaet je Query (3 Slot-Reads) — ehrlich deklariert fuer hash_probes_total.
    [[nodiscard]] static constexpr std::uint64_t probe_multiplicity() noexcept { return 3u; }

    /// SPACE-Seite des T16-Kern-Trade-offs: strukturelles Bit-Budget der REALEN Slot-Tabelle (kSlots 1-Byte-Slots =
    /// sizeof(table_)*8), compile-time. Static-constexpr-Descriptor analog probe_multiplicity() — kein Datenmember,
    /// kein neuer Achsenwert, kein POD-Feld (golden-320/Gate-1/1416 neutral).
    [[nodiscard]] static constexpr std::size_t filter_bit_capacity() noexcept { return kSlots * 8u; }
    /// bits/key bei key_count Keys — spiegelt composable/ bits_per_key() (key_count==0 → 0.0).
    [[nodiscard]] static constexpr double bits_per_key(std::size_t key_count) noexcept {
        return key_count ? static_cast<double>(filter_bit_capacity()) / static_cast<double>(key_count) : 0.0;
    }

    [[nodiscard]] static constexpr bool             supports_range_query() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "filter_xor"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::filter_axis::XorFilter",
                                  "axes/filter_axis/axis_filter_xor.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "XorFilter (Graf+Lemire 2020, ~9 bits/key, smaller than Bloom)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "XOR"; }

    // F15-Pfad-A Treibe-Op (Spec §5 T16, Goldstandard analog axis_05 scan_field_sum / axis_10
    // serialize_scan / axis_04 node_find_scan). SYNTHETISCHE Mindest-Op (ehrlich deklariert): es
    // existiert in diesem Wrapper KEINE persistente XOR-Tabelle, daher wird `buf` als Pseudo-Fingerprint-
    // Tabelle (1 Byte/Slot) der Länge `n` interpretiert. Der Aufwand ist REAL und strategie-abhängig
    // (Xor-Filter nach Graf+Lemire 2020 §3: Lookup = fp == B[h0(x)] XOR B[h1(x)] XOR B[h2(x)] — GENAU 3
    // unabhängige Tabellenzugriffe + 3-fach-XOR, KEIN früher Abbruch), KEINE erfundene Zahl.
    // Charakteristisch vs Bloom/Cuckoo: immer exakt 3 (branch-freie) Slot-Reads → andere, gleichmäßige
    // Latenz-Signatur ohne datenabhängigen early-exit. Punkt-Query only, immutable nach Build.
    [[nodiscard]] static std::uint64_t filter_probe_scan(unsigned char const* buf, std::size_t n,
                                                         unsigned char const* queries, std::size_t q) noexcept {
        if (n < 3) return 0; // Xor-Filter braucht 3 disjunkte Tabellen-Drittel
        std::uint64_t     hits  = 0;
        std::size_t const third = n / 3u;         // garantiert >= 1 (n >= 3), Rest fällt in Drittel 3
        std::size_t const rest  = n - 2u * third; // Drittel 3 = Rest (>= third >= 1)
        for (std::size_t i = 0; i < q; ++i) {
            std::uint32_t const key = queries[i];
            std::uint8_t const  fp  = static_cast<std::uint8_t>(key * 0x9Du ^ 0xA5u); // 1-Byte-Fingerprint
            // 3 unabhängige Hash-Positionen (Graf+Lemire: drei disjunkte Tabellen-Drittel)
            std::size_t const h0 = (key * 2654435761u) % third;
            std::size_t const h1 = third + ((key * 40503u) % third);
            std::size_t const h2 = 2u * third + ((key * 0x85EBCA6Bu) % rest);
            // GENAU 3 Slot-Reads + 3-fach-XOR (branch-frei, kein early-exit — Xor-charakteristisch)
            std::uint8_t const x = static_cast<std::uint8_t>(buf[h0] ^ buf[h1] ^ buf[h2]);
            hits += (x == fp) ? (1u + (key & 7u)) : 0u; // order-sensitiv
        }
        return hits;
    }

private:
    // REALE persistente XOR-Fingerprint-Tabelle (P5 #124). 3 disjunkte Drittel, 1 Byte/Slot.
    std::array<std::uint8_t, kSlots> table_{};
};

} // namespace comdare::cache_engine::filter_axis

namespace comdare::cache_engine::filter_axis {
static_assert(concepts::FilterStrategy<XorFilter>);
static_assert(concepts::CacheEnginePermutationStrategy<XorFilter>);
} // namespace comdare::cache_engine::filter_axis
