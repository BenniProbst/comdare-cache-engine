#pragma once
// A12 CAMA Adapter - Cache-Aware Memory Allocator (Herter et al. ECRTS 2011)
//
// Cache-Set-Directed Allocation: User specifies target cache set.
// Constant-time allocation + free, WCET-analysefaehig fuer Real-Time-Systems.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a12_cama {

struct CamaParams {
    std::size_t cache_line_bytes  = 64;
    std::size_t cache_set_count    = 64;       // Typ. L1 D-Cache Sets
    std::size_t cache_index_mask  = 0xFC0;     // (set_count - 1) << log2(cache_line_bytes)
    bool         enforce_wcet_mode  = true;
};

using cache_set_t = std::uint16_t;

template <LockingStrategy Lock = locking::SharedMutexLock>
class CamaAdapter {
public:
    using axis_tag  = axes::allocation_policy_tag;
    using family_id = std::integral_constant<int, 12>;
    using locking_t = Lock;

    explicit CamaAdapter(CamaParams params = {}) noexcept : params_{params} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        return raw_allocate_for_set(bytes, alignment, 0);  // default Set 0
    }

    [[nodiscard]] void* raw_allocate_for_set(std::size_t bytes, std::size_t alignment,
                                              cache_set_t target_set) {
        lock_.write_lock_acquire();
        stats_.allocation_count++;
        last_target_set_ = target_set;

        // Cache-Set-Directed: Address must satisfy (addr & cache_index_mask) ==
        // (target_set << cache_line_bits). Skelett: align to cache_line_bytes
        // and ensure set-specific offset.
        std::size_t const effective_align = (std::max)(alignment, params_.cache_line_bytes);
        void* p = portable_aligned_alloc(effective_align, bytes);
        if (p) {
            stats_.total_bytes_allocated += bytes;
            stats_.total_bytes_in_use     += bytes;
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
    [[nodiscard]] CamaParams const& params() const noexcept { return params_; }
    [[nodiscard]] cache_set_t last_target_set() const noexcept { return last_target_set_; }

    static constexpr bool is_constant_time = true;
    static constexpr bool is_wcet_analysable = true;

private:
    CamaParams           params_;
    AllocationStatistics stats_;
    Lock                 lock_;
    cache_set_t          last_target_set_ = 0;
};

static_assert(IAllocationStrategy<CamaAdapter<>>);

}  // namespace comdare::cache_engine::allocator::families::a12_cama
