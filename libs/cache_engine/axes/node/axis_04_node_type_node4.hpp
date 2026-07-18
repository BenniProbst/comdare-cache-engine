#pragma once
// V41.F.6.1.R7.1.d axis_04 Node4NodeType (ART small node, 4 slots)

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

/// Node4NodeType — ART small node mit 4 Slots (Leis ICDE 2013).
/// Linear-Search optimal fuer kleine Node-Fanouts (Cache-Line passend).
class Node4NodeType : public NodeTypeStrategyBase<Node4NodeType> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::capacity_class_tag;
    using family_id = std::integral_constant<int, 4>;

    static constexpr bool enabled = flags::node4_enabled;

    [[nodiscard]] static constexpr std::size_t      max_capacity() noexcept { return 4; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "node4"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "Node4NodeType (ART small node, 4-slot linear-search)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "NODE4"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1";

    // KF-6 (2026-06-02): verhaltens-tragender Run-Body, DIVERGENT je ART-Node-Format (Leis ICDE 2013) —
    // macht die node-Achse F15-operativ (analog axis_05 scan_field_sum / axis_10 serialize_scan).
    // Node4 = LINEAR-Scan über bis zu 4 sortierte Schlüssel; Prüfsumme der berührten Zellen (Scan-Kosten,
    // Anti-Wegoptimierung). cacheline_prefetch (geerbt von CacheLineAware, KF-5) bäckt den konfigurierten
    // SW-Prefetch-Hint ein (no-op bei Default None).
    [[nodiscard]] static std::uint64_t node_find_scan(std::uint8_t const* stored, std::size_t n,
                                                      std::uint8_t const* queries, std::size_t q) noexcept {
        cacheline_prefetch(stored);
        std::size_t const cap = (n < 4) ? n : 4; // ART Node4 hält ≤ 4 Schlüssel
        std::uint64_t     sum = 0;
        for (std::size_t i = 0; i < q; ++i) {
            std::uint8_t const key = queries[i];
            for (std::size_t j = 0; j < cap; ++j) { // LINEAR-Scan sortierter Schlüssel
                sum += stored[j];                   // jede verglichene Zelle berührt
                if (stored[j] >= key) break;        // ART: sortiert → Abbruch bei >=
            }
        }
        return sum;
    }
};

} // namespace comdare::cache_engine::node

namespace comdare::cache_engine::node {
static_assert(concepts::NodeTypeStrategy<Node4NodeType>);
static_assert(concepts::CacheEnginePermutationStrategy<Node4NodeType>);
} // namespace comdare::cache_engine::node
