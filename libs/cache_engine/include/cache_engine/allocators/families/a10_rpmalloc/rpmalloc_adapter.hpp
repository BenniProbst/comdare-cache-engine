#pragma once
// A10 rpmalloc Adapter - Public Domain Lock-Free Thread-Caching (Mattias Jansson)
//
// Single-file C lib (~2000 LOC), embedded-friendly. 16-Byte aligned default,
// 32-Byte alignment optional fuer SIMD AVX2/AVX-512.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a10_rpmalloc {

struct RpmallocParams {
    std::size_t default_alignment_bytes  = 16;            // 16-Byte default
    bool         enable_simd_alignment     = false;        // 32-Byte fuer AVX2
    std::size_t thread_cache_max_pct      = 25;           // % of max class allocations
    std::size_t global_cache_multiplier   = 4;            // global = 4 * thread cache
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class RpmallocAdapter {
public:
    using axis_tag  = axes::thread_locality_tag;
    using family_id = std::integral_constant<int, 10>;
    using locking_t = Lock;

    explicit RpmallocAdapter(RpmallocParams params = {}) noexcept : params_{params} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        // rpmalloc: lock-free thread cache lookup (atomic counter Skelett)
        std::size_t const effective_alignment = (std::max)(
            alignment,
            params_.enable_simd_alignment ? std::size_t{32} : params_.default_alignment_bytes);

        stats_.allocation_count.fetch_add(1, std::memory_order_relaxed);
        void* p = portable_aligned_alloc(effective_alignment, bytes);
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

    [[nodiscard]] RpmallocParams const& params() const noexcept { return params_; }

    static constexpr bool is_lock_free          = true;
    static constexpr bool is_embedded_friendly  = true;
    static constexpr bool is_public_domain      = true;

private:
    RpmallocParams params_;
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use    {0};
        std::atomic<std::size_t> allocation_count      {0};
        std::atomic<std::size_t> deallocation_count    {0};
        std::atomic<std::size_t> failure_count          {0};
    } stats_;
};

static_assert(IAllocationStrategy<RpmallocAdapter<>>);

}  // namespace comdare::cache_engine::allocator::families::a10_rpmalloc
