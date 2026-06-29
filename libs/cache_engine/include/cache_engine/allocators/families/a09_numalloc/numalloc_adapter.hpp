#pragma once
// A09 NUMAlloc Adapter - NUMA-Origin-Aware Memory Allocator (Liu/Berger ISMM 2023)
//
// Origin-Aware Memory Management + Incremental Hugepage Sharing.
// Block taggt mit NUMA-Origin-Node, Free returns to origin-node freelist.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <thread>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a09_numalloc {

struct NumaAllocParams {
    std::size_t numa_node_count                 = 1;               // typ. 1-8 sockets
    std::size_t hugepage_size_bytes             = 2 * 1024 * 1024; // 2 MiB
    bool        enable_incremental_share        = true;
    std::size_t incremental_share_threshold_pct = 80; // % memory pressure
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class NumaAllocAdapter {
public:
    using axis_tag  = axes::allocation_policy_tag;
    using family_id = std::integral_constant<int, 9>;
    using locking_t = Lock;

    explicit NumaAllocAdapter(NumaAllocParams params = {}) noexcept : params_{params} {
        if (params_.numa_node_count == 0) {
            unsigned int cpus       = std::thread::hardware_concurrency();
            params_.numa_node_count = (cpus >= 16) ? 2 : 1; // Heuristik
        }
    }

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        // Origin-Aware: Tag with calling thread's NUMA-Node (Skelett: hardcoded 0)
        std::size_t const origin_node = origin_node_for_thread();
        per_node_alloc_count_[origin_node].fetch_add(1, std::memory_order_relaxed);

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
        // NUMA-Origin-Return: Block geht zurueck zu Free-List des Origin-Nodes
        // (Skelett: einfach atomic counter increment)
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

    [[nodiscard]] NumaAllocParams const& params() const noexcept { return params_; }

    [[nodiscard]] std::uint64_t allocations_for_node(std::size_t node_id) const noexcept {
        if (node_id >= kMaxNumaNodes) return 0;
        return per_node_alloc_count_[node_id].load(std::memory_order_relaxed);
    }

private:
    [[nodiscard]] std::size_t origin_node_for_thread() const noexcept {
        // Skelett: thread-id-stable assignment — Phase 7 nutzt syscall (getcpu auf Linux)
        thread_local std::size_t const my_node =
            (std::hash<std::thread::id>{}(std::this_thread::get_id())) % params_.numa_node_count;
        return my_node;
    }

    static constexpr std::size_t kMaxNumaNodes = 8;
    NumaAllocParams              params_;
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use{0};
        std::atomic<std::size_t> allocation_count{0};
        std::atomic<std::size_t> deallocation_count{0};
        std::atomic<std::size_t> failure_count{0};
    } stats_;
    std::array<std::atomic<std::uint64_t>, kMaxNumaNodes> per_node_alloc_count_{};
};

static_assert(IAllocationStrategy<NumaAllocAdapter<>>);

} // namespace comdare::cache_engine::allocator::families::a09_numalloc
