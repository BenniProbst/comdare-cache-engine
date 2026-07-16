#pragma once
// A22 N3916 PMR Adapter - Polymorphic Memory Resources (C++17 std::pmr Standard)
//
// std::pmr::memory_resource-konforme Variante: synchronized_pool_resource +
// monotonic_buffer_resource + new_delete_resource ueber Comdare-Concept.

#include "../../concepts/i_allocation_strategy.hpp"
#include "../../concepts/locking_concept.hpp"
#include "../../locking/shared_mutex_lock.hpp"
#include "../../portable_aligned_alloc.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <memory_resource>
#include <type_traits>

namespace comdare::cache_engine::allocator::families::a22_pmr_resource {

enum class PmrVariant : std::uint8_t {
    NewDelete          = 0, // std::pmr::new_delete_resource()
    SynchronizedPool   = 1, // std::pmr::synchronized_pool_resource (thread-safe)
    UnsynchronizedPool = 2, // std::pmr::unsynchronized_pool_resource (single-thread)
    MonotonicBuffer    = 3, // std::pmr::monotonic_buffer_resource (bump-pointer)
};

struct PmrParams {
    PmrVariant  variant                     = PmrVariant::SynchronizedPool;
    std::size_t max_blocks_per_chunk        = 100;
    std::size_t largest_required_pool_block = 1024;
    std::size_t monotonic_buffer_bytes      = 64 * 1024; // 64 KiB initial
};

template <LockingStrategy Lock = locking::SharedMutexLock>
class PmrResourceAdapter {
public:
    using axis_tag  = axes::synchronization_tag;
    using family_id = std::integral_constant<int, 22>;
    using locking_t = Lock;

    // (F57/Muster B, WP-5 2026-07-16): NICHT noexcept — make_unique der pmr-Pool-Resourcen alloziert und
    // kann werfen (IAllocationStrategy verlangt keinen nothrow-Konstruktor, nur nothrow-Destruktor).
    explicit PmrResourceAdapter(PmrParams params = {}) : params_{params} {
        switch (params_.variant) {
            case PmrVariant::NewDelete: resource_ = std::pmr::new_delete_resource(); break;
            case PmrVariant::SynchronizedPool: {
                std::pmr::pool_options opts;
                opts.max_blocks_per_chunk        = params_.max_blocks_per_chunk;
                opts.largest_required_pool_block = params_.largest_required_pool_block;
                sync_pool_                       = std::make_unique<std::pmr::synchronized_pool_resource>(opts);
                resource_                        = sync_pool_.get();
                break;
            }
            case PmrVariant::UnsynchronizedPool: {
                std::pmr::pool_options opts;
                opts.max_blocks_per_chunk        = params_.max_blocks_per_chunk;
                opts.largest_required_pool_block = params_.largest_required_pool_block;
                unsync_pool_                     = std::make_unique<std::pmr::unsynchronized_pool_resource>(opts);
                resource_                        = unsync_pool_.get();
                break;
            }
            case PmrVariant::MonotonicBuffer:
                buffer_ = portable_aligned_alloc(64, params_.monotonic_buffer_bytes);
                monotonic_ =
                    std::make_unique<std::pmr::monotonic_buffer_resource>(buffer_, params_.monotonic_buffer_bytes);
                resource_ = monotonic_.get();
                break;
        }
    }

    ~PmrResourceAdapter() {
        if (buffer_) portable_aligned_free(buffer_);
    }

    PmrResourceAdapter(PmrResourceAdapter const&)            = delete;
    PmrResourceAdapter& operator=(PmrResourceAdapter const&) = delete;

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        stats_.allocation_count.fetch_add(1, std::memory_order_relaxed);
        try {
            void* p = resource_->allocate(bytes, alignment);
            stats_.total_bytes_allocated.fetch_add(bytes, std::memory_order_relaxed);
            stats_.total_bytes_in_use.fetch_add(bytes, std::memory_order_relaxed);
            return p;
        } catch (std::bad_alloc const&) {
            stats_.failure_count.fetch_add(1, std::memory_order_relaxed);
            return nullptr;
        }
    }

    void raw_deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
        if (!p) return;
        // monotonic_buffer_resource: do_deallocate ist no-op
        if (params_.variant != PmrVariant::MonotonicBuffer) {
            try {
                resource_->deallocate(p, bytes, alignment);
            } catch (...) {}
        }
        stats_.deallocation_count.fetch_add(1, std::memory_order_relaxed);
        stats_.total_bytes_in_use.fetch_sub(bytes, std::memory_order_relaxed);
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

    [[nodiscard]] PmrParams const&           params() const noexcept { return params_; }
    [[nodiscard]] std::pmr::memory_resource* resource() const noexcept { return resource_; }

private:
    PmrParams                                               params_;
    std::pmr::memory_resource*                              resource_ = nullptr;
    std::unique_ptr<std::pmr::synchronized_pool_resource>   sync_pool_;
    std::unique_ptr<std::pmr::unsynchronized_pool_resource> unsync_pool_;
    std::unique_ptr<std::pmr::monotonic_buffer_resource>    monotonic_;
    void*                                                   buffer_ = nullptr;
    struct AtomicStats {
        std::atomic<std::size_t> total_bytes_allocated{0};
        std::atomic<std::size_t> total_bytes_in_use{0};
        std::atomic<std::size_t> allocation_count{0};
        std::atomic<std::size_t> deallocation_count{0};
        std::atomic<std::size_t> failure_count{0};
    } stats_;
};

static_assert(IAllocationStrategy<PmrResourceAdapter<>>);

} // namespace comdare::cache_engine::allocator::families::a22_pmr_resource
