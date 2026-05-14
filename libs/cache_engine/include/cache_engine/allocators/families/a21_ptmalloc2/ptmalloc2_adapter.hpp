#pragma once
// A21 ptmalloc2 / glibc Adapter - Multi-Arena dlmalloc-Erweiterung (Wolfram Gloger)
//
// Bin-Hierarchie: tcache (glibc 2.26+) -> fastbin -> smallbin -> unsorted -> largebin.
// Default Linux malloc.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <thread>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a21_ptmalloc2 {

struct Ptmalloc2Params {
    std::size_t arena_count_max          = 0;             // 0 = 8*ncpu auto
    std::size_t tcache_bin_count          = 64;
    std::size_t tcache_max_chunks_per_bin = 7;
    std::size_t fastbin_count             = 10;           // chunks ≤160 bytes
    std::size_t smallbin_count            = 62;
    std::size_t largebin_count            = 32;
    bool         enable_tcache             = true;        // glibc 2.26+
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class Ptmalloc2Adapter {
public:
    using axis_tag  = axes::freelist_topology_tag;
    using family_id = std::integral_constant<int, 21>;
    using locking_t = Lock;

    explicit Ptmalloc2Adapter(Ptmalloc2Params params = {}) noexcept : params_{params} {
        if (params_.arena_count_max == 0) {
            unsigned int cpus = std::thread::hardware_concurrency();
            if (cpus == 0) cpus = 1;
            params_.arena_count_max = 8 * cpus;
        }
    }

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        // Hierarchie-Klassifizierung fuer Telemetry
        if (bytes <= 1032 && params_.enable_tcache) {
            tcache_alloc_count_.fetch_add(1, std::memory_order_relaxed);
        } else if (bytes <= 160) {
            fastbin_alloc_count_.fetch_add(1, std::memory_order_relaxed);
        } else if (bytes <= 1024) {
            smallbin_alloc_count_.fetch_add(1, std::memory_order_relaxed);
        } else {
            largebin_alloc_count_.fetch_add(1, std::memory_order_relaxed);
        }

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

    [[nodiscard]] Ptmalloc2Params const& params() const noexcept { return params_; }
    [[nodiscard]] std::uint64_t tcache_count()  const noexcept { return tcache_alloc_count_.load(std::memory_order_relaxed); }
    [[nodiscard]] std::uint64_t fastbin_count() const noexcept { return fastbin_alloc_count_.load(std::memory_order_relaxed); }
    [[nodiscard]] std::uint64_t smallbin_count() const noexcept { return smallbin_alloc_count_.load(std::memory_order_relaxed); }
    [[nodiscard]] std::uint64_t largebin_count() const noexcept { return largebin_alloc_count_.load(std::memory_order_relaxed); }

private:
    Ptmalloc2Params            params_;
    std::atomic<std::uint64_t> tcache_alloc_count_{0};
    std::atomic<std::uint64_t> fastbin_alloc_count_{0};
    std::atomic<std::uint64_t> smallbin_alloc_count_{0};
    std::atomic<std::uint64_t> largebin_alloc_count_{0};
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use    {0};
        std::atomic<std::size_t> allocation_count      {0};
        std::atomic<std::size_t> deallocation_count    {0};
        std::atomic<std::size_t> failure_count          {0};
    } stats_;
};

static_assert(IAllocationStrategy<Ptmalloc2Adapter<>>);

}  // namespace comdare::cache_engine::allocator::families::a21_ptmalloc2
