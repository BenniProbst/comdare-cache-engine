#pragma once
// A04 mimalloc Adapter - Free List Sharding (Leijen MSR-TR-2019-18)
//
// Wrapper fuer microsoft/mimalloc (ext/A04-mimalloc/). 3 Free-Lists pro Page
// (free / local_free / thread_free), temporal cadence + deferred_free hook.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a04_mimalloc {

struct MimallocParams {
    std::size_t page_bytes          = 64 * 1024;       // mimalloc default
    std::size_t segment_bytes       = 4  * 1024 * 1024; // 4 MiB segment alignment
    bool        enable_secure_mode  = false;            // smimalloc-variant
    bool        enable_deferred_free = true;
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class MimallocAdapter {
public:
    using axis_tag  = axes::freelist_topology_tag;
    using family_id = std::integral_constant<int, 4>;
    using locking_t = Lock;

    explicit MimallocAdapter(MimallocParams params = {}) noexcept : params_{params} {}

    // Phase 6.2.E Skelett: delegates to std::aligned_alloc; Phase 7 replaces
    // with mi_malloc_aligned() from ext/A04-mimalloc/.
    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        // mimalloc verzichtet auf locks im Common-Case (NoLocks-Synchronization)
        // → Atomic-Counter-Updates statt Lock-Akquise im Hot-Path.
        stats_.allocation_count.fetch_add(1, std::memory_order_relaxed);
        void* p = portable_aligned_alloc(alignment, bytes);
        if (p) {
            stats_.total_bytes_allocated.fetch_add(bytes, std::memory_order_relaxed);
            stats_.total_bytes_in_use    .fetch_add(bytes, std::memory_order_relaxed);
        } else {
            stats_.failure_count.fetch_add(1, std::memory_order_relaxed);
        }
        return p;
    }

    void raw_deallocate(void* p, std::size_t bytes, std::size_t /*alignment*/) noexcept {
        if (!p) return;
        stats_.deallocation_count.fetch_add(1, std::memory_order_relaxed);
        stats_.total_bytes_in_use.fetch_sub(bytes, std::memory_order_relaxed);
        portable_aligned_free(p);
    }

    [[nodiscard]] AllocationStatistics statistics() const noexcept {
        AllocationStatistics s{};
        s.total_bytes_allocated = stats_.total_bytes_allocated.load(std::memory_order_relaxed);
        s.total_bytes_in_use     = stats_.total_bytes_in_use   .load(std::memory_order_relaxed);
        s.allocation_count       = stats_.allocation_count     .load(std::memory_order_relaxed);
        s.deallocation_count     = stats_.deallocation_count   .load(std::memory_order_relaxed);
        s.failure_count           = stats_.failure_count       .load(std::memory_order_relaxed);
        return s;
    }

    // mimalloc-spezifisch: Deferred-Free-Callback (User-Hook, REV 7 §1.4 Reclamation)
    using DeferredFreeFn = void (*)(void* user_data);
    void set_deferred_free(DeferredFreeFn fn, void* user_data) noexcept {
        deferred_free_fn_   = fn;
        deferred_free_data_ = user_data;
    }

    void trigger_deferred_free() {
        if (deferred_free_fn_ && params_.enable_deferred_free) {
            deferred_free_fn_(deferred_free_data_);
        }
    }

    [[nodiscard]] MimallocParams const& params() const noexcept { return params_; }

private:
    MimallocParams params_;

    // mimalloc-Synchronisation: atomic-counters statt lock (NoLocks-Modell)
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use    {0};
        std::atomic<std::size_t> allocation_count      {0};
        std::atomic<std::size_t> deallocation_count    {0};
        std::atomic<std::size_t> failure_count          {0};
    } stats_;

    DeferredFreeFn deferred_free_fn_   = nullptr;
    void*          deferred_free_data_ = nullptr;
};

static_assert(IAllocationStrategy<MimallocAdapter<>>);

}  // namespace comdare::cache_engine::allocator::families::a04_mimalloc
