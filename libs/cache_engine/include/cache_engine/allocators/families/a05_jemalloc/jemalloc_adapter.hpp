#pragma once
// A05 jemalloc Adapter - Multi-Arena malloc(3) Implementation (Evans BSDCan 2006)
//
// Multiple Arenas (4*ncpu default), 4-Tier Size-Class (Tiny/Quantum/Sub-page/Large/Huge),
// Per-Arena Lock + RB-Tree fuer Chunks/Runs.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <thread>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::allocator::families::a05_jemalloc {

struct JemallocParams {
    std::size_t arena_count          = 0;               // 0 = 4*ncpu auto
    std::size_t chunk_bytes          = 2 * 1024 * 1024; // 2 MiB
    std::size_t tiny_max_bytes       = 16;
    std::size_t quantum_bytes        = 16; // 16-Byte quantum spacing
    std::size_t subpage_max_bytes    = 2048;
    bool        enable_decay_purging = true;
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class JemallocAdapter {
public:
    using axis_tag  = axes::thread_locality_tag; // Multiple Arenas
    using family_id = std::integral_constant<int, 5>;
    using locking_t = Lock;

    explicit JemallocAdapter(JemallocParams params = {}) noexcept : params_{params} {
        if (params_.arena_count == 0) {
            unsigned int cpus = std::thread::hardware_concurrency();
            if (cpus == 0) cpus = 4;
            params_.arena_count = 4 * cpus;
        }
    }

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        // jemalloc: arena selection based on thread (round-robin)
        std::size_t const arena_idx = next_arena_for_thread();
        (void)arena_idx; // Skelett: in Phase 7 echter Per-Arena-Lock

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

    [[nodiscard]] JemallocParams const& params() const noexcept { return params_; }

    // jemalloc-spezifisch: 4-Tier Size-Class-Selektor
    enum class SizeTier { Tiny, Quantum, Subpage, Large, Huge };

    [[nodiscard]] SizeTier classify_size(std::size_t bytes) const noexcept {
        if (bytes <= params_.tiny_max_bytes) return SizeTier::Tiny;
        if (bytes <= 480) return SizeTier::Quantum;
        if (bytes <= params_.subpage_max_bytes) return SizeTier::Subpage;
        if (bytes < params_.chunk_bytes) return SizeTier::Large;
        return SizeTier::Huge;
    }

private:
    [[nodiscard]] std::size_t next_arena_for_thread() noexcept {
        // Round-robin assignment, thread-id-stable (Pflicht: same thread → same arena)
        thread_local std::size_t arena_for_this_thread = next_arena_global_.fetch_add(1, std::memory_order_relaxed);
        return arena_for_this_thread % params_.arena_count;
    }

    JemallocParams           params_;
    std::atomic<std::size_t> next_arena_global_{0};
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use{0};
        std::atomic<std::size_t> allocation_count{0};
        std::atomic<std::size_t> deallocation_count{0};
        std::atomic<std::size_t> failure_count{0};
    } stats_;
};

static_assert(IAllocationStrategy<JemallocAdapter<>>);

} // namespace comdare::cache_engine::allocator::families::a05_jemalloc
