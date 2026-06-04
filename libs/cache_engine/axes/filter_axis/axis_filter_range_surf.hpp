#pragma once
// V41.F.6.1.R7.5.e axis_filter RangeSurfFilter (Zhang SIGMOD 2018)
//
// R7.6 Paper-Reference (Task #723):
// Zhang, H., Lim, H., Leis, V., Andersen, D.G., Kaminsky, M., Keeton, K.,
// Pavlo, A. "SuRF: Practical Range Query Filtering with Fast Succinct Tries."
// Proceedings of the 2018 International Conference on Management of Data
// (SIGMOD 2018), pp. 323-336.
// DOI: 10.1145/3183713.3196931
// URL: https://www.cs.cmu.edu/~huanche1/publications/surf_paper.pdf
// Code: https://github.com/efficient/SuRF (Apache-2.0 License)
// Original-Code-File: include/surf.hpp + src/louds_dense.cpp

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

/// RangeSurfFilter — Zhang/Lim/Leis/Andersen/Pavlo SIGMOD 2018.
/// Succinct Range Filter (SuRF) basierend auf LOUDS-Bitmap-Trie.
/// **Erstes Filter mit Range-Query** (lower_bound/upper_bound). Wird in
/// RocksDB-Optimierungen verwendet (Range-Filter fuer SST-Files).
class RangeSurfFilter : public FilterStrategyBase<RangeSurfFilter> {
public:
    using topic_tag = ::comdare::cache_engine::filter::concepts::FilterTopicTag;
    using axis_tag  = subaxes::query_type_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::range_surf_enabled;

    [[nodiscard]] static constexpr bool             supports_range_query() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "filter_range_surf"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "RangeSurfFilter (Zhang SIGMOD 2018, succinct LOUDS-Trie, range-query)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "RANGE_SURF"; }

    // F15-Pfad-A Treibe-Op (Spec §5 T16, Goldstandard analog axis_05 scan_field_sum / axis_10
    // serialize_scan / axis_04 node_find_scan). SYNTHETISCHE Mindest-Op (ehrlich deklariert): es
    // existiert in diesem Wrapper KEIN persistenter LOUDS-Trie, daher wird `buf` als Pseudo-Trie-
    // Knotenarray (1 Byte = 1 Branch-Label) der Länge `n` interpretiert und je Query ein Prefix-Pfad
    // BYTE-WEISE deszendiert. Der Aufwand ist REAL und strategie-abhängig (SuRF: mehrstufiger Trie-
    // Prefix-Abstieg mit succinct-LOUDS-Navigation nach Zhang SIGMOD 2018, daher pro Query mehrere
    // abhängige Level-Schritte statt eines Hash-Tests), KEINE erfundene Zahl. EINZIGE Filter-Strategie
    // mit Range-Semantik: liefert lower_bound-artige Prefix-Treffer (kein exakter Punkt-Test) → die
    // Prefix-Länge bestimmt die Latenz. Mehrere benachbarte Query-Bytes bilden den abgefragten Prefix.
    [[nodiscard]] static std::uint64_t filter_probe_scan(unsigned char const* buf, std::size_t n,
                                                         unsigned char const* queries, std::size_t q) noexcept {
        if (n == 0 || q == 0) return 0;
        constexpr std::size_t kMaxDepth = 6;               // SuRF: succinct-Trie-Höhe (bounded suffix)
        std::uint64_t matched = 0;
        for (std::size_t i = 0; i < q; ++i) {
            std::size_t node = (static_cast<std::uint8_t>(queries[i]) * 131u) % n;   // LOUDS-Wurzel-Position
            std::size_t depth = 0;
            // BYTE-WEISER Prefix-Abstieg: vergleiche aufeinanderfolgende Query-Bytes gegen Branch-Labels
            for (; depth < kMaxDepth; ++depth) {
                std::uint8_t const label = static_cast<std::uint8_t>(queries[(i + depth) % q]);
                if (buf[node] != label) break;             // Prefix endet → range-bound erreicht
                // LOUDS-child: nächste Knotenposition aus aktuellem Label deterministisch ableiten
                node = (node * 31u + label + 1u) % n;
            }
            matched += depth;                              // Prefix-Länge = strategie-bestimmte Treffer-Tiefe
        }
        return matched;
    }
};

}  // namespace

namespace comdare::cache_engine::filter_axis {
    static_assert(concepts::FilterStrategy<RangeSurfFilter>);
    static_assert(concepts::CacheEnginePermutationStrategy<RangeSurfFilter>);
}
