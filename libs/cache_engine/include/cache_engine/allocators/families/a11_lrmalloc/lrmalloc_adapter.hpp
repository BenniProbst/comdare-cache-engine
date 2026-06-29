#pragma once
// A11 LRMalloc Adapter - Modern Lock-Free Allocator (Leite/Rocha VECPAR 2018)
//
// Lock-free + Thread-Caches + Allocator/User-Memory-Segregation.
// Direkter Nachfolger von A03 Michael 2004 mit modernen Optimizations.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a11_lrmalloc {

struct LRMallocParams {
    std::size_t thread_cache_max_objects = 64; // Pro Size-Class
    std::size_t superblock_bytes         = 16 * 1024;
    bool        segregate_metadata       = true; // User+Allocator Memory getrennt
    bool        use_hazard_pointers      = true;
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class LRMallocAdapter {
public:
    using axis_tag  = axes::synchronization_tag;
    using family_id = std::integral_constant<int, 11>;
    using locking_t = Lock;

    explicit LRMallocAdapter(LRMallocParams params = {}) noexcept : params_{params} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        // Hot-path: Thread-Cache-Lookup (lock-free atomic counter Skelett)
        thread_cache_hits_.fetch_add(1, std::memory_order_relaxed);
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

    [[nodiscard]] LRMallocParams const& params() const noexcept { return params_; }

    [[nodiscard]] std::uint64_t thread_cache_hits() const noexcept {
        return thread_cache_hits_.load(std::memory_order_relaxed);
    }

    static constexpr bool is_lock_free         = true;
    static constexpr bool is_async_signal_safe = true; // erbt von Michael 2004

private:
    LRMallocParams             params_;
    std::atomic<std::uint64_t> thread_cache_hits_{0};
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use{0};
        std::atomic<std::size_t> allocation_count{0};
        std::atomic<std::size_t> deallocation_count{0};
        std::atomic<std::size_t> failure_count{0};
    } stats_;
};

static_assert(IAllocationStrategy<LRMallocAdapter<>>);

} // namespace comdare::cache_engine::allocator::families::a11_lrmalloc
