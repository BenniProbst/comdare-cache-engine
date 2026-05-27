#pragma once
// V41.F.6.1.R7.5.h axis_01_index_organization Zentrale Registry

#include <topics/search_engine/axis_01_index_organization/axis_01_index_organization_flags.hpp>

#include "axis_01_index_organization_heap.hpp"
#include "axis_01_index_organization_clustered.hpp"
#include "axis_01_index_organization_non_clustered.hpp"
#include "axis_01_index_organization_index_organized_table.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::search_engine::axis_01_index_organization {

namespace mp = boost::mp11;

using AllOrganizations = mp::mp_list<
    HeapOrganization,
    ClusteredOrganization,
    NonClusteredOrganization,
    IndexOrganizedTable
>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledOrganizations = mp::mp_filter<is_enabled, AllOrganizations>;

static_assert(mp::mp_size<EnabledOrganizations>::value > 0,
    "Axis 01 IndexOrganization: at least one clustering must be enabled");

}  // namespace
