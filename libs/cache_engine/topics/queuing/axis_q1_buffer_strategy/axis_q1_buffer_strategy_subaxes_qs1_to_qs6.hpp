#pragma once
// V41.F.6.1 axis_q1_buffer_strategy Subaxis-Tags QS1-QS6 (2026-05-26)
//
// @topic queuing
// @achse Q1
//
// Klassifikation der Buffer-Strategien (analog Allocator AA1-AA7):

namespace comdare::cache_engine::queuing::axis_q1_buffer_strategy::subaxes {

/// QS1 sequential — sequenzieller Zugriff (FIFO/LIFO/AppendOnly)
struct sequential_access_tag {};

/// QS2 ordered — sortierter Zugriff (Priority, Skiplist)
struct ordered_access_tag {};

/// QS3 cyclic — zyklischer Zugriff (BoundedRing/Disruptor)
struct cyclic_access_tag {};

/// QS4 versioned — versionierter Zugriff (DeltaChain/Tombstone/CoW/Epoch)
struct versioned_access_tag {};

/// QS5 batched — Batch-Operationen (BulkInsert/BulkRead)
struct batched_access_tag {};

/// QS6 lock_free — lock-freier Concurrent-Zugriff (SPSC/MPMC)
struct lock_free_access_tag {};

}  // namespace
