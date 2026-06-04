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
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

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

    [[nodiscard]] static constexpr bool             supports_range_query() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "filter_xor"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "XorFilter (Graf+Lemire 2020, ~9 bits/key, smaller than Bloom)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "XOR"; }

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
        if (n < 3) return 0;                               // Xor-Filter braucht 3 disjunkte Tabellen-Drittel
        std::uint64_t hits = 0;
        std::size_t const third = n / 3u;                  // garantiert >= 1 (n >= 3), Rest fällt in Drittel 3
        std::size_t const rest  = n - 2u * third;          // Drittel 3 = Rest (>= third >= 1)
        for (std::size_t i = 0; i < q; ++i) {
            std::uint32_t const key = queries[i];
            std::uint8_t const fp = static_cast<std::uint8_t>(key * 0x9Du ^ 0xA5u);   // 1-Byte-Fingerprint
            // 3 unabhängige Hash-Positionen (Graf+Lemire: drei disjunkte Tabellen-Drittel)
            std::size_t const h0 = (key * 2654435761u) % third;
            std::size_t const h1 = third + ((key * 40503u) % third);
            std::size_t const h2 = 2u * third + ((key * 0x85EBCA6Bu) % rest);
            // GENAU 3 Slot-Reads + 3-fach-XOR (branch-frei, kein early-exit — Xor-charakteristisch)
            std::uint8_t const x = static_cast<std::uint8_t>(buf[h0] ^ buf[h1] ^ buf[h2]);
            hits += (x == fp) ? (1u + (key & 7u)) : 0u;                                // order-sensitiv
        }
        return hits;
    }
};

}  // namespace

namespace comdare::cache_engine::filter_axis {
    static_assert(concepts::FilterStrategy<XorFilter>);
    static_assert(concepts::CacheEnginePermutationStrategy<XorFilter>);
}
