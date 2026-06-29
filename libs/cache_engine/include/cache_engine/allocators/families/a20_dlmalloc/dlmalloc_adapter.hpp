#pragma once
// A20 dlmalloc Adapter - Doug Lea's malloc (1987-2024+)
// Historic baseline allocator. Reference implementation of boundary tag
// coalescing + smallbin/treebin/wilderness pattern.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <cstddef>
#include <cstdlib>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a20_dlmalloc {

struct DlmallocParams {
    std::size_t mmap_threshold_bytes = 128 * 1024; // 128 KiB → mmap fallback
    std::size_t smallbin_count       = 32;
    std::size_t treebin_count        = 32;
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class DlmallocAdapter {
public:
    using axis_tag  = axes::reclamation_tag; // Boundary-Tag-Coalesce
    using family_id = std::integral_constant<int, 20>;
    using locking_t = Lock;

    explicit DlmallocAdapter(DlmallocParams params = {}) noexcept : params_{params} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        lock_.write_lock_acquire();
        stats_.allocation_count++;
        void* p = portable_aligned_alloc(alignment, bytes);
        if (p) {
            stats_.total_bytes_allocated += bytes;
            stats_.total_bytes_in_use += bytes;
        } else {
            stats_.failure_count++;
        }
        lock_.write_lock_release();
        return p;
    }

    void raw_deallocate(void* p, std::size_t bytes, std::size_t /*alignment*/) noexcept {
        if (!p) return;
        lock_.write_lock_acquire();
        stats_.deallocation_count++;
        stats_.total_bytes_in_use -= bytes;
        portable_aligned_free(p);
        lock_.write_lock_release();
    }

    [[nodiscard]] AllocationStatistics statistics() const noexcept { return stats_; }

    [[nodiscard]] DlmallocParams const& params() const noexcept { return params_; }

private:
    DlmallocParams       params_;
    AllocationStatistics stats_;
    Lock                 lock_;
};

static_assert(IAllocationStrategy<DlmallocAdapter<>>);

} // namespace comdare::cache_engine::allocator::families::a20_dlmalloc
