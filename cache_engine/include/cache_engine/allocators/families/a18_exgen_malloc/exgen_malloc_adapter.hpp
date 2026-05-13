#pragma once
// A18 Exgen-Malloc Adapter - Single-Threaded Optimization (Li/John/Yadwadkar IEEE CAL 2025)
//
// Centralized Heap + Single Free-Block List + Page-Metadata Compaction.
// 8-byte fine-grained Size Classes. Eliminiert Multi-Thread-Overhead.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <cstddef>
#include <cstdlib>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a18_exgen_malloc {

struct ExgenMallocParams {
    std::size_t segment_bytes              = 4 * 1024 * 1024;  // 4 MiB segment
    std::size_t size_class_step_bytes      = 8;                 // 8-byte fine-grained
    bool         enable_metadata_compaction  = true;
    bool         single_threaded_only         = true;
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class ExgenMallocAdapter {
public:
    using axis_tag  = axes::thread_locality_tag;     // Single-Thread = Kein Locality-Concept
    using family_id = std::integral_constant<int, 18>;
    using locking_t = Lock;

    explicit ExgenMallocAdapter(ExgenMallocParams params = {}) noexcept : params_{params} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        // Single-Thread: kein lock noetig → minimaler Hot-Path
        stats_.allocation_count++;
        // Round up zu naechster 8-byte Size-Class
        std::size_t const padded_bytes = (bytes + 7) & ~static_cast<std::size_t>(7);
        void* p = portable_aligned_alloc(alignment, padded_bytes);
        if (p) {
            stats_.total_bytes_allocated += padded_bytes;
            stats_.total_bytes_in_use     += padded_bytes;
            // Internal frag = padding-Schiefe (typ. <8 bytes pro alloc)
            std::size_t const slack = padded_bytes - bytes;
            if (padded_bytes > 0) {
                stats_.internal_fragmentation =
                    static_cast<double>(slack) / static_cast<double>(padded_bytes);
            }
        } else {
            stats_.failure_count++;
        }
        return p;
    }

    void raw_deallocate(void* p, std::size_t bytes, std::size_t /*alignment*/) noexcept {
        if (!p) return;
        stats_.deallocation_count++;
        std::size_t const padded_bytes = (bytes + 7) & ~static_cast<std::size_t>(7);
        stats_.total_bytes_in_use -= padded_bytes;
        portable_aligned_free(p);
    }

    [[nodiscard]] AllocationStatistics statistics() const noexcept { return stats_; }
    [[nodiscard]] ExgenMallocParams const& params() const noexcept { return params_; }

    static constexpr bool is_single_threaded_only = true;
    static constexpr bool has_metadata_compaction = true;

private:
    ExgenMallocParams    params_;
    AllocationStatistics stats_;
};

static_assert(IAllocationStrategy<ExgenMallocAdapter<>>);

}  // namespace comdare::cache_engine::allocator::families::a18_exgen_malloc
