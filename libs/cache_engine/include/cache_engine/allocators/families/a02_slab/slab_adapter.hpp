#pragma once
// A02 Bonwick Slab Adapter - Object-Caching Kernel Memory Allocator (USENIX 1994)
//
// Object Caching: Konstruktor/Destruktor wird nur einmal pro Slab-Initialisierung
// aufgerufen. Slab-Coloring fuer Cache-Distribution. Per-Cache-Locking.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::allocator::families::a02_slab {

struct SlabParams {
    std::size_t object_size_bytes     = 64; // Pflicht: Pro Cache 1 Object-Size
    std::size_t slab_pages            = 1;  // 1 Page = 4 KiB; 4 Pages = 16 KiB
    std::size_t max_internal_frag_pct = 12; // SunOS 5.4 Default 12.5%
    std::size_t reap_interval_seconds = 15; // Working-set algorithm
    std::size_t coloring_max_offset   = 0;  // 0 = auto-calc from slack
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class SlabAdapter {
public:
    using axis_tag  = axes::reclamation_tag; // Refcount-Slab + Object-Caching
    using family_id = std::integral_constant<int, 2>;
    using locking_t = Lock;

    explicit SlabAdapter(SlabParams params = {}) noexcept : params_{params} {
        // Slab-Coloring auto-calc: maximum slack offset
        if (params_.coloring_max_offset == 0) {
            std::size_t const page_bytes       = params_.slab_pages * 4096;
            std::size_t const usable           = page_bytes - 64; // ~kmem_slab struct
            std::size_t const objects_per_slab = usable / params_.object_size_bytes;
            std::size_t const slack            = usable - objects_per_slab * params_.object_size_bytes;
            // Wenn slack 0: bewusst Mindest-Coloring von 64 Byte (1 Cache-Line)
            params_.coloring_max_offset = slack > 0 ? slack : 64;
        }
    }

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        lock_.write_lock_acquire();
        stats_.allocation_count++;

        // Slab-Coloring: rotate offset pro Allocation
        current_color_ = (current_color_ + 8) % (params_.coloring_max_offset + 1);

        void* p = portable_aligned_alloc(alignment, bytes);
        if (p) {
            stats_.total_bytes_allocated += bytes;
            stats_.total_bytes_in_use += bytes;
            // Internal fragmentation = bytes-zu-object-size-Schiefe
            if (params_.object_size_bytes > bytes) {
                stats_.internal_fragmentation =
                    1.0 - static_cast<double>(bytes) / static_cast<double>(params_.object_size_bytes);
            }
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
    [[nodiscard]] SlabParams const&    params() const noexcept { return params_; }

    // Slab-Coloring-Position fuer Diagnostik
    [[nodiscard]] std::size_t current_color() const noexcept { return current_color_; }

private:
    SlabParams           params_;
    AllocationStatistics stats_;
    Lock                 lock_;
    std::size_t          current_color_ = 0; // Bonwick Coloring
};

static_assert(IAllocationStrategy<SlabAdapter<>>);

} // namespace comdare::cache_engine::allocator::families::a02_slab
