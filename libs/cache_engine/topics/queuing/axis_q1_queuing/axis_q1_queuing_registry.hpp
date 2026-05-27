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
// Batch 2 (Q-APP AppendOnlyBuffer, Q-PRIO PriorityHeapBuffer)
#include "axis_q1_queuing_append_only.hpp"
#include "axis_q1_queuing_priority_heap.hpp"
// Batch 3 (Q-DELTA DeltaChainBuffer, Q-SKIP SkiplistBuffer, Q-TOMB TombstoneBuffer)
#include "axis_q1_queuing_delta_chain.hpp"
#include "axis_q1_queuing_skiplist_buffer.hpp"
#include "axis_q1_queuing_tombstone_buffer.hpp"
// Batch 4 (Q-COW CopyOnWriteBuffer, Q-EPOCH EpochBuffer, Q-BATCH BatchedInsertBuffer)
#include "axis_q1_queuing_copy_on_write.hpp"
#include "axis_q1_queuing_epoch_buffer.hpp"
#include "axis_q1_queuing_batched_insert_buffer.hpp"
// Batch 5 VOLLAUSBAU (Q-SPSC LockFreeSPSCBuffer Lamport, Q-MPMC LockFreeMPMCBuffer Vyukov)
#include "axis_q1_queuing_lockfree_spsc.hpp"
#include "axis_q1_queuing_lockfree_mpmc.hpp"
// V41.F.6.1.P2.D.q.s2 Original-Paper-Wrapper Q01 (moodycamel ConcurrentQueue, BSD-2)
#include "axis_q1_queuing_original_concurrentqueue.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::queuing::axis_q1_queuing {

namespace mp = boost::mp11;

/// AllStrategies — KOMPLETTE Liste aller 14 W2-Strategien (Vollausbau Batch 5)
using AllStrategies = mp::mp_list<
    // Pilot Batch 1 (2026-05-26)
    NoBuffer,
    FIFOQueueBuffer,
    LIFOStackBuffer,
    BoundedRingBuffer,
    // Batch 2 (2026-05-26)
    AppendOnlyBuffer,
    PriorityHeapBuffer,
    // Batch 3 (2026-05-26)
    DeltaChainBuffer,
    SkiplistBuffer,
    TombstoneBuffer,
    // Batch 4 (2026-05-26)
    CopyOnWriteBuffer,
    EpochBuffer,
    BatchedInsertBuffer,
    // Batch 5 VOLLAUSBAU (2026-05-26) — axis_q1_queuing 14/14 KOMPLETT
    LockFreeSPSCBuffer,
    LockFreeMPMCBuffer,
    // V41.F.6.1.P2.D.q.s2 Original-Paper-Wrapper Q15 (moodycamel ConcurrentQueue, 2/6 originall)
    OriginalLockFreeMpmcConcurrentQueue
>;

template <typename S>
using is_enabled = mp::mp_bool<S::enabled>;

using EnabledStrategies = mp::mp_filter<is_enabled, AllStrategies>;

static_assert(mp::mp_size<EnabledStrategies>::value > 0,
    "axis_q1_queuing: at least one strategy must be enabled");

}  // namespace
