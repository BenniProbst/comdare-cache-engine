#pragma once
// A15 HMalloc Adapter - Hybrid Local/Shared Memory (Li/Yao 2019)
//
// Strikte Trennung Local vs Shared Memory.
// Local: coalescence-free, kein Lock; Shared: flag-basiert lock-free.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a15_hmalloc {

enum class MemoryClass : std::uint8_t {
    Local  = 0, // thread-local, kein lock, coalescence-free
    Shared = 1, // global, flag-basiert lock-free
};

struct HmallocParams {
    bool        enable_coalescence_free_local = true;
    bool        use_flag_based_shared         = true;
    std::size_t local_pool_bytes              = 16 * 1024 * 1024; // 16 MiB per thread
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class HmallocAdapter {
public:
    using axis_tag  = axes::thread_locality_tag;
    using family_id = std::integral_constant<int, 15>;
    using locking_t = Lock;

    explicit HmallocAdapter(HmallocParams params = {}) noexcept : params_{params} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        return raw_allocate_with_class(bytes, alignment, MemoryClass::Local);
    }

    [[nodiscard]] void* raw_allocate_with_class(std::size_t bytes, std::size_t alignment, MemoryClass mclass) {
        if (mclass == MemoryClass::Local) {
            local_alloc_count_.fetch_add(1, std::memory_order_relaxed);
        } else {
            shared_alloc_count_.fetch_add(1, std::memory_order_relaxed);
        }

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

    [[nodiscard]] HmallocParams const& params() const noexcept { return params_; }
    [[nodiscard]] std::uint64_t        local_count() const noexcept {
        return local_alloc_count_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] std::uint64_t shared_count() const noexcept {
        return shared_alloc_count_.load(std::memory_order_relaxed);
    }

private:
    HmallocParams              params_;
    std::atomic<std::uint64_t> local_alloc_count_{0};
    std::atomic<std::uint64_t> shared_alloc_count_{0};
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use{0};
        std::atomic<std::size_t> allocation_count{0};
        std::atomic<std::size_t> deallocation_count{0};
        std::atomic<std::size_t> failure_count{0};
    } stats_;
};

static_assert(IAllocationStrategy<HmallocAdapter<>>);

} // namespace comdare::cache_engine::allocator::families::a15_hmalloc
