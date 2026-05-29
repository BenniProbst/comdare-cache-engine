#pragma once
// V41.F.6.1 axis_03b_cache_traversal Registry (W6-Pattern)

#include <topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_flags.hpp>

#include "axis_03b_cache_traversal_linear_fanout.hpp"
#include "axis_03b_cache_traversal_hash_lookup.hpp"
// V41.F.6.1.R7.2 (2026-05-29) CT03 sorted fanout + binary search (B+ inner-node, Bayer/McCreight 1972)
#include "axis_03b_cache_traversal_binary_search_fanout.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::traversal::axis_03b_cache_traversal {

namespace mp = boost::mp11;

using AllStrategies = mp::mp_list<
    // Pilot Batch 1 (2026-05-26)
    LinearFanout,
    HashLookup,
    // V41.F.6.1.R7.2 (2026-05-29)
    BinarySearchFanout    // CT03 — sortierter Fanout + lower_bound (B+ inner-node binary search)
    // Vollausbau-Roadmap: CT04 SkiplistTraversal, CT05 RadixTraversal, ...
>;

template <typename S>
using is_enabled = mp::mp_bool<S::enabled>;

using EnabledStrategies = mp::mp_filter<is_enabled, AllStrategies>;

static_assert(mp::mp_size<EnabledStrategies>::value > 0,
    "axis_03b_cache_traversal: at least one strategy must be enabled");

}  // namespace
