#pragma once
// V41.F.6.1.R2 MasstreeComposition (2026-05-26 Architektur-Refactoring)
//
// @composition Masstree (P03 Mao/Kohler/Morris EuroSys 2012)
//
// Verwendet existing axis_03a VectorU16U16 (multilevel-aequivalent fuer Masstree Layer-Slice).
// Hinweis: Echtes Masstree-Linking via Template-Adapter (rotaki-Pattern) ist Task #689 P2.D.tr.s4
// als Folge-Sprint. Aktuelle Composition referenziert existing Re-Impl Sub-Achse.

#include "../topics/traversal/axis_03a_search_algo/axis_03a_search_algo_vector_u16u16.hpp"
#include "../topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_linear_fanout.hpp"
#include "../topics/traversal/axis_03m_mapping/axis_03m_mapping_direct_placement.hpp"
#include "../topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp"

#include <string_view>

namespace comdare::cache_engine::compositions {

/// MasstreeComposition — Masstree B+/Trie-Hybrid als Permutations-Konfiguration.
///
/// Sub-Achsen-Wahl entspricht Masstree Layer-Slice Pattern:
/// - axis_03a search_algo:  VectorU16U16 (multilevel, LAYER_SLICE-aequivalent fuer u16-key)
/// - axis_03b cache_traversal: LinearFanout
/// - axis_03m mapping:      DirectPlacement (in s4 ggf. PermutationIndex pro INode)
/// - axis_06 allocator:     MimallocAllocator
///
/// **Pending Erweiterung** (s4 mit Template-Adapter):
/// - search_algo → echter `Masstree::basic_table<params>` Template-Instantiation (rotaki-Pattern)
/// - axis_03m mapping → PermutationIndex pro INode (Masstree-Spezifikum)
/// - axis_08 concurrency → OLC + Versioning (Masstree fine-grained)
struct MasstreeComposition {
    using search_algo     = traversal::axis_03a_search_algo::VectorU16U16;
    using cache_traversal = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping         = traversal::axis_03m_mapping::DirectPlacement;
    using allocator       = allocator::axis_06_allocator::MimallocAllocator;

    static constexpr std::string_view paper_id    = "P03 Mao/Kohler/Morris EuroSys 2012";
    static constexpr std::string_view paper_title = "Cache Craftiness for Fast Multicore Key-Value Storage";
    static constexpr std::string_view name        = "MasstreeComposition";
};

}  // namespace
