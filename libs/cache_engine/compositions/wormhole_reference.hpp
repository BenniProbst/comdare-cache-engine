#pragma once
// V41.F.6.1.R2 WormholeComposition (2026-05-26 Architektur-Refactoring Pilot)
//
// @composition Wormhole (P07 Wu/Ni/Jiang ATC 2019)

#include "../topics/traversal/axis_03a_search_algo/axis_03a_search_algo_vector_u8u8.hpp"
#include "../topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_hash_lookup.hpp"
#include "../topics/traversal/axis_03m_mapping/axis_03m_mapping_direct_placement.hpp"
#include "../topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp"

#include <string_view>

namespace comdare::cache_engine::compositions {

/// WormholeComposition — Wormhole als konkrete Permutations-Konfiguration.
///
/// Sub-Achsen-Wahl entspricht Wormhole Hash-Anchor Pattern:
/// - axis_03a search_algo:  VectorU8U8 (sparse, HASH_ANCHOR-aequivalent ueber Wormhole)
/// - axis_03b cache_traversal: HashLookup (Hash-basiert, Wormhole-typisch)
/// - axis_03m mapping:      DirectPlacement
/// - axis_06 allocator:     MimallocAllocator
///
/// Pending: axis_04 node_type → HashAnchorNode, axis_08 concurrency → RwLatches.
struct WormholeComposition {
    using search_algo     = traversal::axis_03a_search_algo::VectorU8U8;
    using cache_traversal = traversal::axis_03b_cache_traversal::HashLookup;
    using mapping         = traversal::axis_03m_mapping::DirectPlacement;
    using allocator       = allocator::axis_06_allocator::MimallocAllocator;

    static constexpr std::string_view paper_id    = "P07 Wu/Ni/Jiang ATC 2019";
    static constexpr std::string_view paper_title = "Wormhole: A Fast Ordered Index for In-memory Data Management";
    static constexpr std::string_view name        = "WormholeComposition";
};

}  // namespace
