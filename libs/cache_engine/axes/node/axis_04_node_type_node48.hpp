#pragma once
// V41.F.6.1.R7.1.d axis_04 Node48NodeType (ART large node, indirect-array)

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

/// Node48NodeType — ART large node mit 48 Slots (Leis ICDE 2013).
/// Indirect-Array: 256-byte ChildIndex + 48-pointer ChildArray.
/// Trade-off: Speicher vs Cache-Line-Misses (vs Node256 direct).
class Node48NodeType : public NodeTypeStrategyBase<Node48NodeType> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::compactness_tag;
    using family_id = std::integral_constant<int, 48>;

    static constexpr bool enabled = flags::node48_enabled;

    [[nodiscard]] static constexpr std::size_t      max_capacity() noexcept { return 48; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "node48"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "Node48NodeType (ART large, indirect-array 48-slot)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "NODE48"; }

    // KF-6 (2026-06-02): Run-Body DIVERGENT je ART-Format. Node48 = INDIREKTION: 256-Byte-ChildIndex
    // (key→Slot+1, 0=leer) + separates 48-Slot-ChildArray → 2 Speicher-Berührungen je Treffer (Index +
    // Child), KEIN linearer Scan. Prüfsumme reflektiert das Doppel-Indirektions-Zugriffsmuster.
    [[nodiscard]] static std::uint64_t node_find_scan(std::uint8_t const* stored, std::size_t n,
                                                      std::uint8_t const* queries, std::size_t q) noexcept {
        cacheline_prefetch(stored);
        std::size_t const cap = (n < 48) ? n : 48;        // ART Node48: ≤ 48 Schlüssel
        unsigned char child_index[256] = {};               // 256-Byte ChildIndex (0=leer, sonst Slot+1)
        for (std::size_t i = 0; i < cap; ++i) child_index[stored[i]] = static_cast<unsigned char>(i + 1);
        std::uint64_t sum = 0;
        for (std::size_t i = 0; i < q; ++i) {
            std::uint8_t const key = queries[i];
            sum += key;                                     // INDIREKTION 1: ChildIndex[key] berührt
            unsigned char const slot = child_index[key];
            if (slot != 0) sum += stored[slot - 1];         // INDIREKTION 2: ChildArray[slot] berührt
        }
        return sum;
    }
};

}  // namespace

namespace comdare::cache_engine::node {
    static_assert(concepts::NodeTypeStrategy<Node48NodeType>);
    static_assert(concepts::CacheEnginePermutationStrategy<Node48NodeType>);
}
