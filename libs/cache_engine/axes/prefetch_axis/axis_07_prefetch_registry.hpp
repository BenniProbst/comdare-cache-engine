#pragma once
// V41.F.6.1.R7.5.a axis_07_prefetch Zentrale Registry

#include <axes/prefetch_axis/axis_07_prefetch_flags.hpp>

#include "axis_07_prefetch_none.hpp"
#include "axis_07_prefetch_distance_estimator.hpp"
#include "axis_07_prefetch_hardware_prefetch.hpp"
#include "axis_07_prefetch_path_oriented.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::prefetch_axis {

namespace mp = boost::mp11;

using AllPrefetchers = mp::mp_list<
    NonePrefetch,
    DistanceEstimatorPrefetch,
    HardwarePrefetch,
    PathOrientedPrefetch
>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledPrefetchers = mp::mp_filter<is_enabled, AllPrefetchers>;

static_assert(mp::mp_size<EnabledPrefetchers>::value > 0,
    "Axis 07 Prefetch: at least one prefetcher must be enabled");

}  // namespace
