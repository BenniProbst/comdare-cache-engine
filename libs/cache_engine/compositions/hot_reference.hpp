#pragma once
// V41.F.6.1.R2 HotComposition (2026-05-26 Architektur-Refactoring)
//
// @composition HOT (P02 Binna/Zangerle/Pichl/Specht PVLDB 2018)

#include "../topics/traversal/axis_03a_search_algo/axis_03a_search_algo_vector_u8u8.hpp"
#include "../topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_linear_fanout.hpp"
#include "../topics/traversal/axis_03m_mapping/axis_03m_mapping_direct_placement.hpp"
#include "../topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp"

#include <string_view>

namespace comdare::cache_engine::compositions {

/// HotComposition — HOT als Permutations-Konfiguration.
///
/// Sub-Achsen-Wahl entspricht HOT Patricia-comprimiert Pattern:
/// - axis_03a search_algo:  VectorU8U8 (sparse, DISCRIMINATIVE_BITS-aequivalent)
/// - axis_03b cache_traversal: LinearFanout
/// - axis_03m mapping:      DirectPlacement
/// - axis_06 allocator:     MimallocAllocator
///
/// Pending: axis_04 node_type → PatriciaCompressed, axis_08 concurrency → RcuLight.
struct HotComposition {
    using search_algo     = traversal::axis_03a_search_algo::VectorU8U8;
    using cache_traversal = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping         = traversal::axis_03m_mapping::DirectPlacement;
    using allocator       = allocator::axis_06_allocator::MimallocAllocator;

    static constexpr std::string_view paper_id    = "P02 Binna/Zangerle/Pichl/Specht PVLDB 11(3) 2018";
    static constexpr std::string_view paper_title = "HOT: A Height Optimized Trie Index for Main-Memory Database Systems";
    static constexpr std::string_view name        = "HotComposition";
};

}  // namespace
