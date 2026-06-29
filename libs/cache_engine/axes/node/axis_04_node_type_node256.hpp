#pragma once
// V41.F.6.1.R7.1.d axis_04 Node256NodeType (Goldstandard-Update, ART dense direct-addressed)

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

/// Node256NodeType — ART dense node mit 256 Slots, direct-addressed.
/// Default-Variante: 1 Pointer-Lookup ohne Search (Leis ICDE 2013).
class Node256NodeType : public NodeTypeStrategyBase<Node256NodeType> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::capacity_class_tag;
    using family_id = std::integral_constant<int, 256>;

    static constexpr bool enabled = flags::node256_enabled;

    [[nodiscard]] static constexpr std::size_t      max_capacity() noexcept { return 256; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "node256"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "Node256NodeType (ART dense, direct-addressed 256-slot)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "NODE256"; }

    // KF-6 (2026-06-02): Run-Body DIVERGENT je ART-Format. Node256 = DIREKT-adressiert: 256-Slot-ChildArray,
    // 1 Berührung je Lookup (kein Index, kein Scan). Prüfsumme reflektiert das O(1)-Direkt-Zugriffsmuster
    // (NUR Child-Touch, KEIN Index-Touch wie Node48 → andere Prüfsumme).
    [[nodiscard]] static std::uint64_t node_find_scan(std::uint8_t const* stored, std::size_t n,
                                                      std::uint8_t const* queries, std::size_t q) noexcept {
        cacheline_prefetch(stored);
        std::size_t const cap          = (n < 256) ? n : 256; // ART Node256: direkt 256 Slots
        bool              present[256] = {};                  // direktes ChildArray (present-Flag je key-Byte)
        for (std::size_t i = 0; i < cap; ++i) present[stored[i]] = true;
        std::uint64_t sum = 0;
        for (std::size_t i = 0; i < q; ++i) {
            std::uint8_t const key = queries[i];
            if (present[key]) sum += key; // DIREKT: ChildArray[key], 1 Touch, kein Index
        }
        return sum;
    }
};

} // namespace comdare::cache_engine::node

namespace comdare::cache_engine::node {
static_assert(concepts::NodeTypeStrategy<Node256NodeType>);
static_assert(concepts::CacheEnginePermutationStrategy<Node256NodeType>);
} // namespace comdare::cache_engine::node
