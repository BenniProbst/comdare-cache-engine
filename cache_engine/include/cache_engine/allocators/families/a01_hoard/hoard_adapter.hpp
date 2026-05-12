#pragma once
// A01 Hoard Adapter - Per-Processor + Global Heap (Berger ASPLOS 2000)
// Wrappers the upstream Hoard allocator (ext/A01-hoard/) into a Comdare-conformant
// IAllocationStrategy. Default Concurrency: SharedMutexLock (REV 7 §1.4 A3).

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <new>

namespace comdare::cache_engine::allocator::families::a01_hoard {

// Hoard-Permutationsparameter (REV 7 §1.3 + Hoard-Paper §3.1):
struct HoardParams {
    std::size_t superblock_bytes      = 8 * 1024;  // S = 8 KiB default
    double      empty_fraction        = 0.25;       // f = 1/4 default
    std::size_t empty_threshold_k     = 4;          // K default
    double      size_class_base       = 1.2;        // b = 1.2 → internal frag ≤ 20%
};

// Stub-Adapter: leitet aktuell an malloc/free durch.
// Phase 6.2.E Skelett — Phase 7 ersetzt durch echte Hoard-Library-Aufrufe
// (dlopen ext/A01-hoard/libhoard.so) oder integrierte Reimplementation.
template <LockingStrategy Lock = locking::SharedMutexLock>
class HoardAdapter {
public:
    using axis_tag  = axes::freelist_topology_tag;
    using family_id = std::integral_constant<int, 1>;  // A01
    using locking_t = Lock;

    explicit HoardAdapter(HoardParams params = {}) noexcept : params_{params} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        lock_.write_lock_acquire();
        stats_.allocation_count++;
        void* p = portable_aligned_alloc(alignment, bytes);
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

    [[nodiscard]] AllocationStatistics statistics() const noexcept {
        // Snapshot ohne Lock-Protection (relaxed-ok fuer Diagnose)
        return stats_;
    }

    [[nodiscard]] HoardParams const& params() const noexcept { return params_; }

private:
    HoardParams           params_;
    AllocationStatistics  stats_;
    Lock                  lock_;
};

static_assert(IAllocationStrategy<HoardAdapter<>>);

}  // namespace comdare::cache_engine::allocator::families::a01_hoard
