#pragma once
// V41.F.6.1.R7.5.b axis_11_telemetry Zentrale Registry

#include <topics/telemetry/axis_11_telemetry/axis_11_telemetry_flags.hpp>

#include "axis_11_telemetry_leaf_only.hpp"
#include "axis_11_telemetry_density_tracker.hpp"
#include "axis_11_telemetry_insert_counter.hpp"
#include "axis_11_telemetry_latency_histogram.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::telemetry::axis_11_telemetry {

namespace mp = boost::mp11;

using AllTelemetries = mp::mp_list<
    LeafOnlyCounter,
    DensityTracker,
    InsertCounter,
    LatencyHistogram
>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledTelemetries = mp::mp_filter<is_enabled, AllTelemetries>;

static_assert(mp::mp_size<EnabledTelemetries>::value > 0,
    "Axis 11 Telemetry: at least one telemetry must be enabled");

}  // namespace
