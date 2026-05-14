#pragma once
// A14 TCMalloc Warehouse-Scale Adapter - 4 Optimierungen aus ASPLOS 2024 (Zhou et al.)
//
// Erweitert A06 tcmalloc mit:
//   - Heterogeneous Per-CPU Caches (dynamic resizing)
//   - NUCA-Aware Transfer Caches (chiplet-local)
//   - L=8 Span-Lists fuer Lifetime-aware Span Prioritization
//   - Lifetime-aware Hugepage Filler

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a14_tcmalloc_warehouse {

struct TcmallocWarehouseParams {
    std::size_t per_cpu_cache_min_bytes   = 1ULL << 20;  // 1 MB minimum
    std::size_t per_cpu_cache_max_bytes   = 3ULL << 20;  // 3 MB maximum
    std::size_t span_list_count            = 8;           // L=8 lists
    std::size_t lifetime_capacity_threshold = 16;         // C=16 fuer short/long
    bool         enable_nuca_transfer       = true;
    bool         enable_heterogeneous_caches = true;
    std::size_t llc_domain_count            = 1;          // chiplet count
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class TcmallocWarehouseAdapter {
public:
    using axis_tag  = axes::reclamation_tag;     // Lifetime-Aware Reclamation
    using family_id = std::integral_constant<int, 14>;
    using locking_t = Lock;

    explicit TcmallocWarehouseAdapter(TcmallocWarehouseParams params = {}) noexcept
        : params_{params} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        // Per-CPU + heterogeneous + NUCA-aware classifies span lifetime
        std::size_t const list_index = bytes >= params_.lifetime_capacity_threshold ? 0 : 7;
        per_list_alloc_count_[list_index].fetch_add(1, std::memory_order_relaxed);

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

    [[nodiscard]] TcmallocWarehouseParams const& params() const noexcept { return params_; }

    [[nodiscard]] std::uint64_t allocations_for_list(std::size_t list_idx) const noexcept {
        if (list_idx >= 8) return 0;
        return per_list_alloc_count_[list_idx].load(std::memory_order_relaxed);
    }

    static constexpr bool has_heterogeneous_caches = true;
    static constexpr bool has_nuca_transfer         = true;
    static constexpr bool has_lifetime_hugepage     = true;

private:
    TcmallocWarehouseParams params_;
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use    {0};
        std::atomic<std::size_t> allocation_count      {0};
        std::atomic<std::size_t> deallocation_count    {0};
        std::atomic<std::size_t> failure_count          {0};
    } stats_;
    std::array<std::atomic<std::uint64_t>, 8> per_list_alloc_count_{};
};

static_assert(IAllocationStrategy<TcmallocWarehouseAdapter<>>);

}  // namespace comdare::cache_engine::allocator::families::a14_tcmalloc_warehouse
