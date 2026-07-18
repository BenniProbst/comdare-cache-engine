#pragma once
// V41.F.6.1.R7.1.d axis_04 Node16NodeType (ART medium node, SIMD-binary-search)

#include "axis_04_node_type_strategy_base.hpp"
#include "axis_04_node_type_subaxes_nt1_to_nt3.hpp"
#include "concepts/axis_04_node_type_cache_engine_permutation_concept.hpp"
#include <axes/node/axis_04_node_type_flags.hpp>
#include <topics/nodes/concepts/topic_nodes_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::node {

/// Node16NodeType — ART medium node mit 16 Slots (Leis ICDE 2013).
/// SIMD-binary-search optimal (SSE2 PCMPESTRM / AVX2 VPCMPESTRM).
class Node16NodeType : public NodeTypeStrategyBase<Node16NodeType> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::access_pattern_tag;
    using family_id = std::integral_constant<int, 16>;

    static constexpr bool enabled = flags::node16_enabled;

    [[nodiscard]] static constexpr std::size_t      max_capacity() noexcept { return 16; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "node16"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "Node16NodeType (ART medium, SIMD-binary-search 16-slot)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "NODE16"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1";

    // KF-6 (2026-06-02): Run-Body DIVERGENT je ART-Format. Node16 = SIMD-binary-search-Format (hier
    // Baseline-Linear) über bis zu 16 sortierte Schlüssel — höhere Kapazitätsgrenze als Node4 (→ andere
    // Prüfsumme bei n>4). cacheline_prefetch (KF-5) bäckt den Prefetch-Hint ein.
    [[nodiscard]] static std::uint64_t node_find_scan(std::uint8_t const* stored, std::size_t n,
                                                      std::uint8_t const* queries, std::size_t q) noexcept {
        cacheline_prefetch(stored);
        std::size_t const cap = (n < 16) ? n : 16; // ART Node16 hält ≤ 16 Schlüssel
        std::uint64_t     sum = 0;
        for (std::size_t i = 0; i < q; ++i) {
            std::uint8_t const key = queries[i];
            for (std::size_t j = 0; j < cap; ++j) {
                sum += stored[j];
                if (stored[j] >= key) break;
            }
        }
        return sum;
    }
};

} // namespace comdare::cache_engine::node

namespace comdare::cache_engine::node {
static_assert(concepts::NodeTypeStrategy<Node16NodeType>);
static_assert(concepts::CacheEnginePermutationStrategy<Node16NodeType>);
} // namespace comdare::cache_engine::node
