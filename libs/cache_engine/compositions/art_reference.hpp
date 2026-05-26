#pragma once
// V41.F.6.1.R2 ArtComposition (2026-05-26 Architektur-Refactoring Pilot)
//
// @composition ART (P01 Leis/Kemper/Neumann ICDE 2013)
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md
// @memory [[achsen-komposition-organ-metapher]]
//
// **Konzept:** Composition-Template = reines using-Tupel das ART als konkrete
// Permutations-Konfiguration aller Achsen definiert. Composition ist KEIN
// Permutations-Element (siehe Doku 14 Organ-Metapher) — es ist ein **konkreter
// Punkt** im Permutations-Raum.

#include "../topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp"
#include "../topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_linear_fanout.hpp"
#include "../topics/traversal/axis_03m_mapping/axis_03m_mapping_direct_placement.hpp"
#include "../topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp"

#include <string_view>

namespace comdare::cache_engine::compositions {

/// ArtComposition — Algorithm-as-Achsen-Tupel fuer ART (Leis ICDE 2013).
///
/// Sub-Achsen-Wahl entspricht ART Node256-direct-addressed Pattern:
/// - axis_03a search_algo:  Array256 (direkt-adressiert, dense, BYTEBYBYTE-aequivalent)
/// - axis_03b cache_traversal: LinearFanout (sequential Cache-Line-Walk)
/// - axis_03m mapping:      DirectPlacement (Array-of-Pointers)
/// - axis_06 allocator:     MimallocAllocator (Free-List-Sharding, ART typisch)
///
/// **Pending Erweiterung** wenn Achsen verfuegbar werden:
/// - axis_04 node_type     → Node256Adaptive (Node4/16/48/256)
/// - axis_05 memory_layout → CacheLineAligned64B
/// - axis_07 prefetch      → DistanceEstimator + PathOriented
/// - axis_08 concurrency   → OlcOptimistic
struct ArtComposition {
    using search_algo     = traversal::axis_03a_search_algo::Array256;
    using cache_traversal = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping         = traversal::axis_03m_mapping::DirectPlacement;
    using allocator       = allocator::axis_06_allocator::MimallocAllocator;

    static constexpr std::string_view paper_id    = "P01 Leis/Kemper/Neumann ICDE 2013";
    static constexpr std::string_view paper_title = "The Adaptive Radix Tree: ARTful Indexing for Main-Memory Databases";
    static constexpr std::string_view name        = "ArtComposition";
};

}  // namespace
