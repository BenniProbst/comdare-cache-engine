#pragma once
// V41.F.6.1.R7.5.e axis_filter CuckooFilter (Fan CoNEXT 2014)
//
// R7.6 Paper-Reference (Task #723):
// Fan, B., Andersen, D.G., Kaminsky, M., Mitzenmacher, M.D. "Cuckoo Filter:
// Practically Better Than Bloom." Proceedings of the 10th ACM International
// Conference on Emerging Networking Experiments and Technologies (CoNEXT 2014),
// pp. 75-88.
// DOI: 10.1145/2674005.2674994
// URL: https://www.cs.cmu.edu/~dga/papers/cuckoo-conext2014.pdf
// Code: https://github.com/efficient/cuckoofilter (Apache-2.0 License)
// Original-Code-Files: src/cuckoofilter.h, src/cuckoofilter.cc

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

/// CuckooFilter — Fan/Andersen/Kaminsky/Mitzenmacher CoNEXT 2014.
/// Bucketed-Cuckoo-Hashing mit Fingerprints. Vorteil vs Bloom:
/// **unterstuetzt delete + lookup-Counting**. Trade-off: hoeherer Insert-Aufwand.
class CuckooFilter : public FilterStrategyBase<CuckooFilter> {
public:
    using topic_tag = ::comdare::cache_engine::filter::concepts::FilterTopicTag;
    using axis_tag  = subaxes::mutability_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::cuckoo_enabled;

    [[nodiscard]] static constexpr bool             supports_range_query() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "filter_cuckoo"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "CuckooFilter (Fan CoNEXT 2014, supports delete + counting)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "CUCKOO"; }

    // F15-Pfad-A Treibe-Op (Spec §5 T16, Goldstandard analog axis_05 scan_field_sum / axis_10
    // serialize_scan / axis_04 node_find_scan). SYNTHETISCHE Mindest-Op (ehrlich deklariert): es
    // existiert in diesem Wrapper KEINE persistente Bucket-Tabelle, daher wird `buf` als Pseudo-Bucket-
    // Array (1 Byte = 1 Fingerprint-Slot) der Länge `n` interpretiert. Der Aufwand ist REAL und
    // strategie-abhängig (Cuckoo: 1-Byte-Fingerprint + ZWEI Kandidaten-Buckets i1 und i2 = i1 XOR
    // hash(fp) — partielle-Schlüssel-Cuckoo-Hashing nach Fan CoNEXT 2014 §3), KEINE erfundene Zahl.
    // Charakteristisch vs Bloom: 2 Bucket-Lookups + Fingerprint-Byte-Vergleich (statt k Bit-Tests),
    // dadurch andere Cache-Zugriffs- und Latenz-Signatur. Punkt-Query only.
    [[nodiscard]] static std::uint64_t filter_probe_scan(unsigned char const* buf, std::size_t n,
                                                         unsigned char const* queries, std::size_t q) noexcept {
        if (n == 0) return 0;
        std::uint64_t hits = 0;
        for (std::size_t i = 0; i < q; ++i) {
            std::uint32_t const key = queries[i];
            std::uint8_t const fp = static_cast<std::uint8_t>((key * 0x1Bu) | 1u);   // 1-Byte-Fingerprint (≠0)
            std::uint32_t const h  = key * 2654435761u;                              // Basis-Hash
            std::size_t const i1 = h % n;                                            // erster Bucket
            std::size_t const i2 = (i1 ^ (fp * 0x5BD1E995u)) % n;                    // partial-key: i2 = i1 XOR hash(fp)
            // ZWEI Bucket-Lookups + Fingerprint-Vergleich (Cuckoo-charakteristisch)
            if (buf[i1] == fp) {
                hits += 1u + (key & 3u);
            } else if (buf[i2] == fp) {                                              // Fallback-Bucket
                hits += 2u + (key & 1u);
            }
        }
        return hits;
    }
};

}  // namespace

namespace comdare::cache_engine::filter_axis {
    static_assert(concepts::FilterStrategy<CuckooFilter>);
    static_assert(concepts::CacheEnginePermutationStrategy<CuckooFilter>);
}
