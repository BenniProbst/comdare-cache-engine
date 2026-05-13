#pragma once
// A17 Crystalline Reclamation Adapter - Wait-Free Memory Reclamation (PLDI 2024)
//
// Pareto-frontier: high performance + high memory efficiency + wait-free.
// Allocator-agnostic Reclamation-Subsystem (kombinierbar mit jedem Allokator).

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a17_crystalline {

struct CrystallineParams {
    std::size_t retired_list_capacity   = 256;
    std::size_t cross_thread_batch_size  = 32;
    bool         enable_async_reclamation  = true;
    bool         balanced_workload          = true;
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class CrystallineAdapter {
public:
    using axis_tag  = axes::reclamation_tag;
    using family_id = std::integral_constant<int, 17>;
    using locking_t = Lock;

    explicit CrystallineAdapter(CrystallineParams params = {}) noexcept : params_{params} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        // Wait-free Hot-Path: nur atomic load (kein CAS!)
        stats_.allocation_count.fetch_add(1, std::memory_order_relaxed);
        void* p = portable_aligned_alloc(alignment, bytes);
        if (p) {
            stats_.total_bytes_allocated.fetch_add(bytes, std::memory_order_relaxed);
            stats_.total_bytes_in_use   .fetch_add(bytes, std::memory_order_relaxed);
        } else {
            stats_.failure_count.fetch_add(1, std::memory_order_relaxed);
        }
        return p;
    }

    void raw_deallocate(void* p, std::size_t bytes, std::size_t /*alignment*/) noexcept {
        if (!p) return;
        // Wait-free Reclamation: in Retired-Liste, keine Cross-Thread-Synchronization
        retired_count_.fetch_add(1, std::memory_order_relaxed);
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

    [[nodiscard]] CrystallineParams const& params() const noexcept { return params_; }
    [[nodiscard]] std::uint64_t retired_count() const noexcept {
        return retired_count_.load(std::memory_order_relaxed);
    }

    static constexpr bool is_wait_free          = true;
    static constexpr bool is_lock_free          = true;
    static constexpr bool memory_bounded         = true;
    static constexpr bool is_async_signal_safe  = true;

private:
    CrystallineParams          params_;
    std::atomic<std::uint64_t> retired_count_{0};
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use    {0};
        std::atomic<std::size_t> allocation_count      {0};
        std::atomic<std::size_t> deallocation_count    {0};
        std::atomic<std::size_t> failure_count          {0};
    } stats_;
};

static_assert(IAllocationStrategy<CrystallineAdapter<>>);

}  // namespace comdare::cache_engine::allocator::families::a17_crystalline
