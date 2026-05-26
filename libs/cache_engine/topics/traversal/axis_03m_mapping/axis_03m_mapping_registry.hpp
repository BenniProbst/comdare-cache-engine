#pragma once
// V41.F.6.1 axis_03m_mapping Registry (W6-Pattern)

#include <topics/traversal/axis_03m_mapping/axis_03m_mapping_flags.hpp>

#include "axis_03m_mapping_direct_placement.hpp"
#include "axis_03m_mapping_pool_relative.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::traversal::axis_03m_mapping {

namespace mp = boost::mp11;

using AllStrategies = mp::mp_list<
    // Pilot Batch 1 (2026-05-26)
    DirectPlacement,
    PoolRelative
    // Vollausbau-Roadmap: MP03 PermutationIndexed, MP04 HashedOffset, ...
>;

template <typename M>
using is_enabled = mp::mp_bool<M::enabled>;

using EnabledStrategies = mp::mp_filter<is_enabled, AllStrategies>;

static_assert(mp::mp_size<EnabledStrategies>::value > 0,
    "axis_03m_mapping: at least one strategy must be enabled");

}  // namespace
