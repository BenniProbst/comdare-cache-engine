#pragma once
// A19 Buddy Adapter - Knuth/Knowlton 1965/1968
// Klassischer Buddy-System mit XOR-Buddy-Lookup in O(1).
// Eigene Re-Implementation als Algorithmus-Studie (Public Domain).

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <bit>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::allocator::families::a19_buddy {

struct BuddyParams {
    int min_order = 6;  // 2^6 = 64 byte minimum block
    int max_order = 30; // 2^30 = 1 GiB maximum block
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class BuddyAdapter {
public:
    using axis_tag  = axes::freelist_topology_tag;
    using family_id = std::integral_constant<int, 19>;
    using locking_t = Lock;

    explicit BuddyAdapter(BuddyParams params = {}) noexcept : params_{params} {}

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        // Algorithmus-Skelett: round up zur naechsten Power-of-2-Order
        int const order = std::max(params_.min_order, static_cast<int>(std::bit_width(bytes - 1)));
        if (order > params_.max_order) return nullptr;

        lock_.write_lock_acquire();
        stats_.allocation_count++;
        void* p = portable_aligned_alloc(alignment, std::size_t{1} << order);
        if (p) {
            std::size_t const block_bytes = std::size_t{1} << order;
            stats_.total_bytes_allocated += block_bytes;
            stats_.total_bytes_in_use += block_bytes;
            // Internal fragmentation: requested vs allocated
            stats_.internal_fragmentation = 1.0 - static_cast<double>(bytes) / static_cast<double>(block_bytes);
        } else {
            stats_.failure_count++;
        }
        lock_.write_lock_release();
        return p;
    }

    void raw_deallocate(void* p, std::size_t bytes, std::size_t /*alignment*/) noexcept {
        if (!p) return;
        int const         order       = std::max(params_.min_order, static_cast<int>(std::bit_width(bytes - 1)));
        std::size_t const block_bytes = std::size_t{1} << order;

        lock_.write_lock_acquire();
        stats_.deallocation_count++;
        stats_.total_bytes_in_use -= block_bytes;
        portable_aligned_free(p);
        lock_.write_lock_release();
    }

    // Buddy-spezifisch: XOR-Buddy-Adresse berechnen
    [[nodiscard]] static std::uintptr_t buddy_address(std::uintptr_t addr, int order) noexcept {
        return addr ^ (std::uintptr_t{1} << order);
    }

    [[nodiscard]] AllocationStatistics statistics() const noexcept { return stats_; }

    [[nodiscard]] BuddyParams const& params() const noexcept { return params_; }

private:
    BuddyParams          params_;
    AllocationStatistics stats_;
    Lock                 lock_;
};

static_assert(IAllocationStrategy<BuddyAdapter<>>);

} // namespace comdare::cache_engine::allocator::families::a19_buddy
