#pragma once
// A06 tcmalloc Adapter - Per-CPU Cache via RSEQ (Google 2009/2024)
// Wrapper fuer google/tcmalloc (ext/A06-tcmalloc/). Hierarchical Cache:
// Per-CPU (RSEQ lockless) -> Central FreeList -> Page Heap (Hugepage-aware).

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a06_tcmalloc {

struct TcmallocParams {
    std::size_t per_cpu_cache_bytes      = 256 * 1024;
    std::size_t max_per_class_objects     = 1024;
    bool        enable_hugepage_aware     = true;
    bool        enable_rseq               = true;
    std::size_t hugepage_size_bytes       = 2 * 1024 * 1024;   // 2 MiB Linux default
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class TcmallocAdapter {
public:
    using axis_tag  = axes::thread_locality_tag;
    using family_id = std::integral_constant<int, 6>;
    using locking_t = Lock;

    explicit TcmallocAdapter(TcmallocParams params = {}) noexcept : params_{params} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        // Modern tcmalloc: per-CPU cache lockless via RSEQ. Skelett delegiert
        // an std::aligned_alloc; Phase 7 ersetzt durch tc_malloc/tc_new aus ext.
        stats_.allocation_count.fetch_add(1, std::memory_order_relaxed);
        void* p = std::aligned_alloc(alignment, bytes);
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
        std::free(p);
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

    [[nodiscard]] TcmallocParams const& params() const noexcept { return params_; }

private:
    TcmallocParams params_;
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use    {0};
        std::atomic<std::size_t> allocation_count      {0};
        std::atomic<std::size_t> deallocation_count    {0};
        std::atomic<std::size_t> failure_count          {0};
    } stats_;
};

static_assert(IAllocationStrategy<TcmallocAdapter<>>);

}  // namespace comdare::cache_engine::allocator::families::a06_tcmalloc
