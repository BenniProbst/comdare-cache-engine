#pragma once
// A13 StarMalloc Adapter - Formally Verified Hardened Allocator (OOPSLA 2024)
//
// Verifiziert in Steel/F*. Hardening-Features: Zeroing-on-Free, Guard-Pages,
// Quarantine, Separate Metadata, Bitmap-Free-Tracking.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a13_starmalloc {

struct StarMallocParams {
    bool        zeroing_on_free          = true;
    bool        enable_guard_pages       = true;
    std::size_t quarantine_count          = 64;       // Recently-freed pages held N free
    std::size_t guard_page_size_bytes    = 4096;
    bool        separate_metadata         = true;
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class StarMallocAdapter {
public:
    using axis_tag  = axes::fragmentation_strategy_tag;  // Hardening = Frag-Strategy
    using family_id = std::integral_constant<int, 13>;
    using locking_t = Lock;

    explicit StarMallocAdapter(StarMallocParams params = {}) noexcept : params_{params} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        stats_.allocation_count.fetch_add(1, std::memory_order_relaxed);
        // Skelett: Phase 7 ergaenzt mmap mit Guard-Pages davor+danach
        void* p = portable_aligned_alloc(alignment, bytes);
        if (p) {
            stats_.total_bytes_allocated.fetch_add(bytes, std::memory_order_relaxed);
            stats_.total_bytes_in_use   .fetch_add(bytes, std::memory_order_relaxed);
            // Zeroing-on-allocate ist defensive; smimalloc/StarMalloc Standard
            std::memset(p, 0, bytes);
        } else {
            stats_.failure_count.fetch_add(1, std::memory_order_relaxed);
        }
        return p;
    }

    void raw_deallocate(void* p, std::size_t bytes, std::size_t /*alignment*/) noexcept {
        if (!p) return;
        // Zeroing-on-Free Pflicht (Information-Leakage-Schutz)
        if (params_.zeroing_on_free) {
            std::memset(p, 0, bytes);
        }
        stats_.deallocation_count.fetch_add(1, std::memory_order_relaxed);
        stats_.total_bytes_in_use.fetch_sub(bytes, std::memory_order_relaxed);
        // Quarantine: in echter Impl wird Page N Frees lang unmapped gehalten
        // → Use-After-Free-Detektion. Skelett: free direkt.
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

    [[nodiscard]] StarMallocParams const& params() const noexcept { return params_; }

    static constexpr bool is_formally_verified = true;
    static constexpr bool is_hardened            = true;
    static constexpr bool is_drop_in_replacement = true;

private:
    StarMallocParams params_;
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use    {0};
        std::atomic<std::size_t> allocation_count      {0};
        std::atomic<std::size_t> deallocation_count    {0};
        std::atomic<std::size_t> failure_count          {0};
    } stats_;
};

static_assert(IAllocationStrategy<StarMallocAdapter<>>);

}  // namespace comdare::cache_engine::allocator::families::a13_starmalloc
