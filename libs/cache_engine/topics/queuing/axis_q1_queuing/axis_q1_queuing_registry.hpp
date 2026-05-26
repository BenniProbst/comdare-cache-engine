#pragma once
// V41.F.6.1 axis_q1_queuing Registry (W6-Pattern, 2026-05-26)
//
// @topic queuing @achse Q1 buffer_strategy

#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_flags.hpp>

// Pilot Batch 1 (Q-NONE, Q-FIFO, Q-LIFO, Q-RING)
#include "axis_q1_queuing_no_buffer.hpp"
#include "axis_q1_queuing_fifo.hpp"
#include "axis_q1_queuing_lifo.hpp"
#include "axis_q1_queuing_bounded_ring.hpp"
// Batch 2 (Q-APP AppendOnly, Q-PRIO PriorityHeap)
#include "axis_q1_queuing_append_only.hpp"
#include "axis_q1_queuing_priority_heap.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::queuing::axis_q1_queuing {

namespace mp = boost::mp11;

/// AllStrategies — Komplette Liste aller bekannten Q1-Strategien
using AllStrategies = mp::mp_list<
    // Pilot Batch 1 (2026-05-26)
    NoBuffer,
    FIFOQueue,
    LIFOStack,
    BoundedRing,
    // Batch 2 (2026-05-26)
    AppendOnly,
    PriorityHeap
    // Vollausbau-Batches 3-5 (geplant):
    // Q07 DeltaChain, Q08 SkiplistBuffer, Q09 TombstoneBuffer, Q10 CopyOnWrite,
    // Q11 EpochBuffer, Q12 BatchedInsertBuffer, Q13 LockFreeSPSC, Q14 LockFreeMPMC
>;

template <typename S>
using is_enabled = mp::mp_bool<S::enabled>;

using EnabledStrategies = mp::mp_filter<is_enabled, AllStrategies>;

static_assert(mp::mp_size<EnabledStrategies>::value > 0,
    "axis_q1_queuing: at least one strategy must be enabled");

}  // namespace
