#pragma once
// A16 PIM-malloc Adapter - Processing-In-Memory Allocator (Lee et al. HPCA-32 2026)
//
// PIM-Metadata + PIM-Executed: Allokator-Metadata + Compute in PIM-Memory.
// Per-PIM-Core eigene Free-Lists. Future-Plattform fuer Comdare.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a16_pim_malloc {

struct PimMallocParams {
    std::size_t pim_core_count           = 8;                // typ. 1-2048 fuer UPMEM
    std::size_t pim_local_bytes_per_core = 64 * 1024 * 1024; // 64 MiB DRAM-PIM
    bool        enable_per_core_hw_cache = true;
    bool        pim_metadata_in_pim      = true; // Metadata in PIM, NICHT Host
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class PimMallocAdapter {
public:
    using axis_tag  = axes::thread_locality_tag;
    using family_id = std::integral_constant<int, 16>;
    using locking_t = Lock;

    explicit PimMallocAdapter(PimMallocParams params = {}) noexcept : params_{params} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        // PIM-Core-Local-Allokation (Skelett: Host-Side Simulation)
        // In Phase 7: ueber UPMEM-API + PIM-Code-Execution
        stats_.allocation_count.fetch_add(1, std::memory_order_relaxed);
        void* p = portable_aligned_alloc(alignment, bytes);
        if (p) {
            stats_.total_bytes_allocated.fetch_add(bytes, std::memory_order_relaxed);
            stats_.total_bytes_in_use.fetch_add(bytes, std::memory_order_relaxed);
            pim_local_alloc_count_.fetch_add(1, std::memory_order_relaxed);
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

    [[nodiscard]] PimMallocParams const& params() const noexcept { return params_; }
    [[nodiscard]] std::uint64_t          pim_local_count() const noexcept {
        return pim_local_alloc_count_.load(std::memory_order_relaxed);
    }

    static constexpr bool requires_pim_hardware = true;
    static constexpr bool is_future_platform    = true;

private:
    PimMallocParams            params_;
    std::atomic<std::uint64_t> pim_local_alloc_count_{0};
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use{0};
        std::atomic<std::size_t> allocation_count{0};
        std::atomic<std::size_t> deallocation_count{0};
        std::atomic<std::size_t> failure_count{0};
    } stats_;
};

static_assert(IAllocationStrategy<PimMallocAdapter<>>);

} // namespace comdare::cache_engine::allocator::families::a16_pim_malloc
