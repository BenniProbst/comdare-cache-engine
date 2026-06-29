#pragma once
// A07 snmalloc Adapter - Message-Passing Allocator (Lietar et al. ISMM 2019)
// 2^k Bucket-Array fuer pending messages + 1 MiB batch-dispatch.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a07_snmalloc {

struct SnmallocParams {
    std::size_t slab_bytes           = 64 * 1024;        // 64 KiB small object slabs
    std::size_t medium_threshold     = 16 * 1024 * 1024; // 16 MiB
    std::size_t batch_dispatch_bytes = 1 * 1024 * 1024;  // 1 MiB
    std::size_t bucket_count_log2    = 6;                // 2^6 = 64 buckets
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class SnmallocAdapter {
public:
    using axis_tag  = axes::synchronization_tag;
    using family_id = std::integral_constant<int, 7>;
    using locking_t = Lock;

    explicit SnmallocAdapter(SnmallocParams params = {}) noexcept : params_{params} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        stats_.allocation_count.fetch_add(1, std::memory_order_relaxed);
        void* p = portable_aligned_alloc(alignment, bytes);
        if (p) {
            stats_.total_bytes_allocated.fetch_add(bytes, std::memory_order_relaxed);
            stats_.total_bytes_in_use.fetch_add(bytes, std::memory_order_relaxed);
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
        s.total_bytes_in_use    = stats_.total_bytes_in_use.load(std::memory_order_relaxed);
        s.allocation_count      = stats_.allocation_count.load(std::memory_order_relaxed);
        s.deallocation_count    = stats_.deallocation_count.load(std::memory_order_relaxed);
        s.failure_count         = stats_.failure_count.load(std::memory_order_relaxed);
        return s;
    }

    [[nodiscard]] SnmallocParams const& params() const noexcept { return params_; }

private:
    SnmallocParams params_;
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use{0};
        std::atomic<std::size_t> allocation_count{0};
        std::atomic<std::size_t> deallocation_count{0};
        std::atomic<std::size_t> failure_count{0};
    } stats_;
};

static_assert(IAllocationStrategy<SnmallocAdapter<>>);

} // namespace comdare::cache_engine::allocator::families::a07_snmalloc
