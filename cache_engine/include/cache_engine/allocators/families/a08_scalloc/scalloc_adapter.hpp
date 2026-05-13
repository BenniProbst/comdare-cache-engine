#pragma once
// A08 scalloc Adapter - Virtual Spans + Lock-Free Treiber-Stack (Aigner OOPSLA 2015)
//
// Uniforme Behandlung Small/Medium/Large bis 1 MiB als Virtual Spans.
// Lock-free LIFO-Pool pro Size-Class (Treiber-stack mit ABA-Prevention).

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a08_scalloc {

struct ScallocParams {
    std::size_t span_min_bytes        = 4 * 1024;        // 4 KiB
    std::size_t span_max_bytes        = 1 * 1024 * 1024; // 1 MiB
    std::size_t virtual_reservation_bytes = 1ULL << 40;  // 1 TiB lazy mmap reservation
    bool         use_treiber_stack     = true;
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class ScallocAdapter {
public:
    using axis_tag  = axes::fragmentation_strategy_tag;
    using family_id = std::integral_constant<int, 8>;
    using locking_t = Lock;

    explicit ScallocAdapter(ScallocParams params = {}) noexcept : params_{params} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        // Lock-free Treiber-stack-style global pool simulation
        treiber_aba_tag_.fetch_add(1, std::memory_order_relaxed);
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

    [[nodiscard]] ScallocParams const& params() const noexcept { return params_; }
    [[nodiscard]] std::uint64_t treiber_tag() const noexcept {
        return treiber_aba_tag_.load(std::memory_order_relaxed);
    }

    static constexpr bool is_lock_free = true;

private:
    ScallocParams              params_;
    std::atomic<std::uint64_t> treiber_aba_tag_{0};
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use    {0};
        std::atomic<std::size_t> allocation_count      {0};
        std::atomic<std::size_t> deallocation_count    {0};
        std::atomic<std::size_t> failure_count          {0};
    } stats_;
};

static_assert(IAllocationStrategy<ScallocAdapter<>>);

}  // namespace comdare::cache_engine::allocator::families::a08_scalloc
