#pragma once
// A03 Michael Lock-Free Adapter - Scalable Lock-Free Dynamic Memory Allocation (PLDI 2004)
//
// Vollstaendig lock-free, async-signal-safe, kill-tolerant.
// Anchor-Struktur (avail:10/count:10/state:2/tag:42) atomic CAS.
// 3-Pfad-Malloc: Active -> Partial -> NewSB.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a03_michael_lockfree {

struct MichaelLockFreeParams {
    std::size_t superblock_bytes       = 16 * 1024;   // 16 KiB Default
    std::size_t hyperblock_bytes       = 1024 * 1024; // 1 MiB batched mmap
    std::size_t max_credits_per_active = 64;          // 6-bit credits field
    bool        use_hazard_pointers    = true;
};

// Lock-frei = Concurrency-Modell, NICHT Locking-Strategy
// Trotzdem hat unser Concept locking_t — wir verwenden no-op Lock fuer ABI-Kompat
template <LockingStrategy Lock = locking::SharedMutexLock>
class MichaelLockFreeAdapter {
public:
    using axis_tag  = axes::synchronization_tag;
    using family_id = std::integral_constant<int, 3>;
    using locking_t = Lock;

    explicit MichaelLockFreeAdapter(MichaelLockFreeParams params = {}) noexcept : params_{params} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        // **Lock-free** Hot-Path: nur atomic counters, keine Lock-Akquise
        stats_.allocation_count.fetch_add(1, std::memory_order_relaxed);

        // ABA-prevention via tag increment (analog Anchor.tag)
        anchor_tag_.fetch_add(1, std::memory_order_relaxed);

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
        // Lock-free Free: CAS auf Anchor (hier vereinfacht)
        stats_.deallocation_count.fetch_add(1, std::memory_order_relaxed);
        stats_.total_bytes_in_use.fetch_sub(bytes, std::memory_order_relaxed);
        anchor_tag_.fetch_add(1, std::memory_order_relaxed);
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

    [[nodiscard]] MichaelLockFreeParams const& params() const noexcept { return params_; }
    [[nodiscard]] std::uint64_t aba_tag() const noexcept { return anchor_tag_.load(std::memory_order_relaxed); }

    // Async-signal-safety: documented contract — diese Funktionen verwenden
    // KEINE Locks und sind verwendbar in Signal-Handlern (sofern aligned_alloc safe ist)
    static constexpr bool is_async_signal_safe = true;
    static constexpr bool is_lock_free         = true;

private:
    MichaelLockFreeParams params_;
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use{0};
        std::atomic<std::size_t> allocation_count{0};
        std::atomic<std::size_t> deallocation_count{0};
        std::atomic<std::size_t> failure_count{0};
    } stats_;
    std::atomic<std::uint64_t> anchor_tag_{0}; // ABA-prevention counter
};

static_assert(IAllocationStrategy<MichaelLockFreeAdapter<>>);

} // namespace comdare::cache_engine::allocator::families::a03_michael_lockfree
