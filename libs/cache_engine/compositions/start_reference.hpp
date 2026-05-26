#pragma once
// V41.F.6.1.R2 StartComposition (2026-05-26 Architektur-Refactoring)
//
// @composition START (P05 Mertens/Mueller et al. ICDE 2024)

#include "../topics/traversal/axis_03a_search_algo/axis_03a_search_algo_vector_u16u16.hpp"
#include "../topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_linear_fanout.hpp"
#include "../topics/traversal/axis_03m_mapping/axis_03m_mapping_direct_placement.hpp"
#include "../topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp"

#include <string_view>

namespace comdare::cache_engine::compositions {

/// StartComposition — START Cost-DP als Permutations-Konfiguration.
///
/// Sub-Achsen-Wahl entspricht START Multi-Byte-Span Cost-DP:
/// - axis_03a search_algo:  VectorU16U16 (multilevel, MULTIBYTE_SPAN-aequivalent)
/// - axis_03b cache_traversal: LinearFanout
/// - axis_03m mapping:      DirectPlacement
/// - axis_06 allocator:     MimallocAllocator
struct StartComposition {
    using search_algo     = traversal::axis_03a_search_algo::VectorU16U16;
    using cache_traversal = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping         = traversal::axis_03m_mapping::DirectPlacement;
    using allocator       = allocator::axis_06_allocator::MimallocAllocator;

    static constexpr std::string_view paper_id    = "P05 Mertens et al. ICDE 2024";
    static constexpr std::string_view paper_title = "START: Self-Tuning Adaptive Radix Tree";
    static constexpr std::string_view name        = "StartComposition";
};

}  // namespace
