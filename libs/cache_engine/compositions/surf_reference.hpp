#pragma once
// V41.F.6.1.R2 SurfComposition (2026-05-26 Architektur-Refactoring Pilot)
//
// @composition SuRF (P10 Zhang/Lim/Andersen SIGMOD 2018)

#include "../topics/traversal/axis_03a_search_algo/axis_03a_search_algo_vector_u16u16.hpp"
#include "../topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_linear_fanout.hpp"
#include "../topics/traversal/axis_03m_mapping/axis_03m_mapping_pool_relative.hpp"
#include "../topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp"

#include <string_view>

namespace comdare::cache_engine::compositions {

/// SurfComposition — SuRF als konkrete Permutations-Konfiguration.
///
/// Sub-Achsen-Wahl entspricht SuRF LOUDS-Bitmap Pattern:
/// - axis_03a search_algo:  VectorU16U16 (multilevel, LOUDS-aequivalent)
/// - axis_03b cache_traversal: LinearFanout (Bitmap-Walk)
/// - axis_03m mapping:      PoolRelative (succinct kompakt, Pool-Offsets)
/// - axis_06 allocator:     MimallocAllocator (Bulk-Loaded — Allocator wenig kritisch)
///
/// **Hinweis:** SuRF ist Read-Only-Index (Bulk-Loaded LOUDS-Trie). Composition
/// kann CRUD-API anbieten, aber Insert/Erase erfordern Rebuild-Trigger (Re-Impl).
struct SurfComposition {
    using search_algo     = traversal::axis_03a_search_algo::VectorU16U16;
    using cache_traversal = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping         = traversal::axis_03m_mapping::PoolRelative;
    using allocator       = allocator::axis_06_allocator::MimallocAllocator;

    static constexpr std::string_view paper_id    = "P10 Zhang/Lim/Andersen SIGMOD 2018";
    static constexpr std::string_view paper_title = "SuRF: Practical Range Query Filtering with Fast Succinct Tries";
    static constexpr std::string_view name        = "SurfComposition";
};

}  // namespace
