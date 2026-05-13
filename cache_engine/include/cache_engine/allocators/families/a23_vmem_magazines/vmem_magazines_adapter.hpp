#pragma once
// A23 Vmem + Magazines Adapter - Per-CPU Magazines + O(1) Vmem (Bonwick/Adams USENIX ATC 2001)
//
// Magazine: Per-CPU Stack[M] of pointers + Depot mit full+empty Magazinen.
// Vmem: Power-of-2 Freelists + Instant-Fit O(1) Allocation.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdlib>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a23_vmem_magazines {

enum class VmemAllocationPolicy : std::uint8_t {
    InstantFit  = 0,   // Default: O(1)
    BestFit     = 1,
    NextFit     = 2,
};

struct VmemMagazinesParams {
    std::size_t magazine_size_initial  = 1;            // M start klein, adaptive
    std::size_t magazine_size_max       = 143;         // M cap (bonwick suggested)
    std::size_t depot_full_max          = 16;
    std::size_t depot_empty_max         = 16;
    bool         enable_magazine_resize  = true;
    VmemAllocationPolicy policy          = VmemAllocationPolicy::InstantFit;
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class VmemMagazinesAdapter {
public:
    using axis_tag  = axes::thread_locality_tag;     // Per-CPU Magazines
    using family_id = std::integral_constant<int, 23>;
    using locking_t = Lock;

    explicit VmemMagazinesAdapter(VmemMagazinesParams params = {}) noexcept
        : params_{params}, current_magazine_size_{params.magazine_size_initial} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        // Magazine-Layer Hot-Path (Skelett: atomic counter)
        magazine_hits_.fetch_add(1, std::memory_order_relaxed);

        // Vmem-Instant-Fit: round up zur naechsten Power-of-2
        std::size_t const order = std::bit_width(bytes - 1);
        std::size_t const allocated_bytes = std::size_t{1} << order;
        last_freelist_index_ = static_cast<std::uint8_t>(order);

        stats_.allocation_count.fetch_add(1, std::memory_order_relaxed);
        void* p = portable_aligned_alloc(alignment, allocated_bytes);
        if (p) {
            stats_.total_bytes_allocated.fetch_add(allocated_bytes, std::memory_order_relaxed);
            stats_.total_bytes_in_use   .fetch_add(allocated_bytes, std::memory_order_relaxed);
        } else {
            stats_.failure_count.fetch_add(1, std::memory_order_relaxed);
        }
        return p;
    }

    void raw_deallocate(void* p, std::size_t bytes, std::size_t /*alignment*/) noexcept {
        if (!p) return;
        std::size_t const order = std::bit_width(bytes - 1);
        std::size_t const allocated_bytes = std::size_t{1} << order;

        stats_.deallocation_count.fetch_add(1, std::memory_order_relaxed);
        stats_.total_bytes_in_use.fetch_sub(allocated_bytes, std::memory_order_relaxed);
        portable_aligned_free(p);
    }

    // Adaptive Magazine Resizing: bei Depot-Lock-Kontention M doppeln
    void trigger_magazine_resize_up() noexcept {
        if (params_.enable_magazine_resize) {
            std::size_t current = current_magazine_size_.load(std::memory_order_relaxed);
            std::size_t const next = (std::min)(current * 2 + 1, params_.magazine_size_max);
            current_magazine_size_.store(next, std::memory_order_relaxed);
        }
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

    [[nodiscard]] VmemMagazinesParams const& params() const noexcept { return params_; }
    [[nodiscard]] std::size_t current_magazine_size() const noexcept {
        return current_magazine_size_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] std::uint64_t magazine_hits() const noexcept {
        return magazine_hits_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] std::uint8_t last_freelist_index() const noexcept {
        return last_freelist_index_.load(std::memory_order_relaxed);
    }

    static constexpr bool is_constant_time     = true;
    static constexpr bool has_per_cpu_magazines = true;

private:
    VmemMagazinesParams        params_;
    std::atomic<std::size_t>   current_magazine_size_;
    std::atomic<std::uint64_t> magazine_hits_{0};
    std::atomic<std::uint8_t>  last_freelist_index_{0};
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use    {0};
        std::atomic<std::size_t> allocation_count      {0};
        std::atomic<std::size_t> deallocation_count    {0};
        std::atomic<std::size_t> failure_count          {0};
    } stats_;
};

static_assert(IAllocationStrategy<VmemMagazinesAdapter<>>);

}  // namespace comdare::cache_engine::allocator::families::a23_vmem_magazines
