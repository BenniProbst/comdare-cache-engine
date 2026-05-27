#pragma once
// V41.F.6.1 Topic traversal Config-Set fuer PermutationEngine (2026-05-26)
//
// @topic traversal
//
// traversal-Topic hat 3 Achsen (analog allocator=1, queuing=2). TopicConfigSet
// bietet alle 3 separat fuer PermutationEngine + ein kombiniertes
// Cartesian-Product fuer (search_algo x cache_traversal x mapping).

#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_registry.hpp>
#include <topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_registry.hpp>
#include <topics/traversal/axis_03m_mapping/axis_03m_mapping_registry.hpp>

#include <boost/mp11.hpp>

namespace comdare::cache_engine::traversal {

namespace mp = boost::mp11;

/// TopicConfigSet — alle 3 Achsen zentral, PermutationEngine-tauglich
struct TopicConfigSet {
    // axis_03a: Search-Algorithm (Array256SearchAlgo/VectorU8U8SearchAlgo/VectorU16U16SearchAlgo pilot)
    using StaticAxisVariants_03a = axis_03a_search_algo::EnabledStrategies;

    // axis_03b: Cache-Traversal (LinearFanout/HashLookup pilot)
    using StaticAxisVariants_03b = axis_03b_cache_traversal::EnabledStrategies;

    // axis_03m: Mapping (DirectPlacement/PoolRelative pilot)
    using StaticAxisVariants_03m = axis_03m_mapping::EnabledStrategies;

    /// Default-StaticAxisVariants — PermutationEngine 1-Topic-Variante nimmt 03a
    /// (Search-Algorithm ist die primaere Achse, 03b/03m als Sub-Permutationen)
    using StaticAxisVariants = StaticAxisVariants_03a;

    /// Cartesian-Product 03a x 03b x 03m
    /// (Pilot: 3 x 2 x 2 = 12 Search-Traversal-Mapping-Kombinationen)
    using Cartesian03ax03bx03m = mp::mp_product<
        mp::mp_list,
        StaticAxisVariants_03a,
        StaticAxisVariants_03b,
        StaticAxisVariants_03m
    >;
};

}  // namespace
