#pragma once
// V41.F.6.1 axis_q2_queuing Registry (W6-Pattern)

#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_flags.hpp>

#include "axis_q2_queuing_eager.hpp"
#include "axis_q2_queuing_watermark.hpp"
#include "axis_q2_queuing_lazy.hpp"
// Vollausbau Batch 2 (2026-05-26): F03 TimedFlush + F05 AdaptiveLsmFlush
#include "axis_q2_queuing_timed.hpp"
#include "axis_q2_queuing_adaptive_lsm.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::queuing::axis_q2_queuing {

namespace mp = boost::mp11;

/// AllPolicies — KOMPLETTE Liste aller 5 W3-Flush-Policies (Vollausbau Batch 2)
using AllPolicies = mp::mp_list<
    // Pilot Batch 1 (2026-05-26)
    EagerFlush,
    WatermarkFlush,
    LazyFlush,
    // Vollausbau Batch 2 (2026-05-26) — axis_q2_queuing 5/5 KOMPLETT
    TimedFlush,
    AdaptiveLsmFlush
>;

template <typename P>
using is_enabled = mp::mp_bool<P::enabled>;

using EnabledPolicies = mp::mp_filter<is_enabled, AllPolicies>;

static_assert(mp::mp_size<EnabledPolicies>::value > 0,
    "axis_q2_queuing: at least one policy must be enabled");

}  // namespace
