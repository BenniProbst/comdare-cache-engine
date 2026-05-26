#pragma once
// V41.F.6.1 axis_q2_flush_policy Registry (W6-Pattern)

#include <topics/queuing/axis_q2_flush_policy/axis_q2_flush_policy_flags.hpp>

#include "axis_q2_flush_policy_eager.hpp"
#include "axis_q2_flush_policy_watermark.hpp"
#include "axis_q2_flush_policy_lazy.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::queuing::axis_q2_flush_policy {

namespace mp = boost::mp11;

using AllPolicies = mp::mp_list<
    // Pilot Batch 1 (3 von 5 W3-Strategien)
    EagerFlush,
    WatermarkFlush,
    LazyFlush
    // Vollausbau-Batch 2 (geplant):
    // F03 TimedFlush (time_window), F05 AdaptiveLsmFlush (adaptive_lsm)
>;

template <typename P>
using is_enabled = mp::mp_bool<P::enabled>;

using EnabledPolicies = mp::mp_filter<is_enabled, AllPolicies>;

static_assert(mp::mp_size<EnabledPolicies>::value > 0,
    "axis_q2_flush_policy: at least one policy must be enabled");

}  // namespace
