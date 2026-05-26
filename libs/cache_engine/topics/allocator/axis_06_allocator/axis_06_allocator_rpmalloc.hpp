#pragma once
// V41.F.6.1 Batch 4 RPMallocAllocator A10 (2026-05-26)
//
// @topic allocator
// @achse 6
// @family A10 (RPMalloc — Mattias Jansson 2017, Per-Thread Spans)
// @subaxis AA3 thread_locality (Per-Thread-Heaps mit Span-basierter Allokation)
//
// **SONDERFALL RPMalloc:** Pflicht-Init vor erstem Aufruf!
//   - rpmalloc_initialize() — pro Prozess EINMAL (process-init guard)
//   - rpmalloc_thread_initialize() — pro Thread EINMAL (thread-local guard)
// Wir nutzen "init-on-first-use" via static-flags. Cleanup ist optional
// (rpmalloc Self-Cleanup beim Prozess-Ende). Bei expliziter Finalisierung:
//   ensure_finalize_at_exit() — Optional fuer Code mit deterministischem Cleanup.

#include "axis_06_allocator_strategy_base.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "concepts/axis_06_allocator_zeroing_strategy_concept.hpp"
#include "concepts/axis_06_allocator_reallocating_strategy_concept.hpp"
#include "concepts/axis_06_allocator_introspectable_strategy_concept.hpp"
#include "../concepts/topic_allocator_concept.hpp"

#include <topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp>
#include "vendor_includes/rpmalloc_include.hpp"

#include <cache_engine/allocators/portable_aligned_alloc.hpp>
#include <measurement/measurable_concept.hpp>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::allocator::axis_06_allocator {

namespace detail {

/**
 * @brief RPMalloc Init-on-First-Use Guard (process + thread).
 *
 * Sicherheit: idempotent via std::atomic_flag (Process-Init) + thread_local
 * bool (Thread-Init). Compile-Time skip wenn HAVE=OFF (alle Calls sind no-op).
 */
struct RPMallocInitGuard {
    static void ensure_initialized() noexcept {
        if constexpr (flags::rpmalloc_enabled) {
            static std::atomic_flag process_inited = ATOMIC_FLAG_INIT;
            if (!process_inited.test_and_set(std::memory_order_acquire)) {
                ::rpmalloc_initialize();
            }
            thread_local bool thread_inited = false;
            if (!thread_inited) {
                ::rpmalloc_thread_initialize();
                thread_inited = true;
            }
        }
    }
};

}  // namespace detail

class RPMallocAllocator : public AllocatorStrategyBase<RPMallocAllocator> {
public:
    static constexpr bool enabled = flags::rpmalloc_enabled;

    using value_type = std::byte;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = subaxes::thread_locality_tag;
    using family_id  = std::integral_constant<int, 10>;

    [[nodiscard]] static constexpr bool        is_thread_safe()  noexcept { return true; }
    [[nodiscard]] static constexpr bool        supports_pmr()    noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t max_alignment()   noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept {
        if constexpr (enabled) { return "rpmalloc"; }
        else                   { return "rpmalloc(real=std)"; }
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "RPMalloc Per-Thread Spans (Mattias Jansson 2017)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "RPMALLOC"; }

    // V41.F.6.1 Vendor-Sonderfall-Properties (Pflicht, [[vendor-sonderfaelle-als-pflicht-property]])
    [[nodiscard]] static constexpr bool has_native_aligned_alloc()    noexcept { return true; }   // rpaligned_alloc
    [[nodiscard]] static constexpr bool requires_explicit_init()      noexcept { return true; }   // SONDERFALL: rpmalloc_initialize + thread_initialize
    [[nodiscard]] static constexpr bool supports_numa_node_hint()     noexcept { return false; }
    [[nodiscard]] static constexpr bool is_lock_free()                noexcept { return false; }  // Span-allocation nutzt Atomic-Operationen aber nicht voll lock-free
    [[nodiscard]] static constexpr bool supports_thread_local_cache() noexcept { return true; }   // namesgebend: per-thread spans

    [[nodiscard]] bool operator==(RPMallocAllocator const&) const noexcept { return true; }

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        // SONDERFALL: rpmalloc verlangt Init vor erstem Aufruf
        detail::RPMallocInitGuard::ensure_initialized();
        void* p;
        if constexpr (enabled) { p = ::rpaligned_alloc(alignment, bytes); }
        else                   { p = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, bytes); }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        if (p != nullptr) {
            ++stats_.allocation_count;
            stats_.total_bytes_allocated += aligned_bytes;
            stats_.total_bytes_in_use    += aligned_bytes;
        } else {
            ++stats_.failure_count;
        }
        observer_.notify(stats_);
#endif
        return p;
    }

    void deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
        if (p == nullptr) return;
        if constexpr (enabled) { ::rpfree(p); }
        else                   { ::comdare::cache_engine::allocator::portable_aligned_free(p); }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        ++stats_.deallocation_count;
        if (aligned_bytes <= stats_.total_bytes_in_use) stats_.total_bytes_in_use -= aligned_bytes;
        else stats_.total_bytes_in_use = 0;
        observer_.notify(stats_);
#else
        (void)bytes; (void)alignment;
#endif
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::AllocationStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;

    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot()   const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; observer_.notify(stats_); }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer()       noexcept { return observer_; }
#endif

    [[nodiscard]] void* zero_allocate(std::size_t n, std::size_t size) {
        detail::RPMallocInitGuard::ensure_initialized();
        std::size_t bytes = n * size;
        void* p;
        if constexpr (enabled) { p = ::rpcalloc(n, size); }
        else                   { p = std::calloc(n, size); }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if (p != nullptr) {
            ++stats_.allocation_count;
            stats_.total_bytes_allocated += bytes;
            stats_.total_bytes_in_use    += bytes;
        } else {
            ++stats_.failure_count;
        }
        observer_.notify(stats_);
#endif
        return p;
    }

    [[nodiscard]] void* reallocate(void* p, std::size_t old_bytes, std::size_t new_bytes,
                                   std::size_t alignment) {
        detail::RPMallocInitGuard::ensure_initialized();
        void* np;
        if constexpr (enabled) {
            np = ::rprealloc(p, new_bytes);
            (void)alignment;
        } else {
            np = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, new_bytes);
            if (np != nullptr && p != nullptr) {
                std::size_t copy_bytes = (old_bytes < new_bytes) ? old_bytes : new_bytes;
                std::memcpy(np, p, copy_bytes);
                ::comdare::cache_engine::allocator::portable_aligned_free(p);
            }
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if (np == nullptr) { ++stats_.failure_count; observer_.notify(stats_); return nullptr; }
        if (p != nullptr) {
            if (old_bytes <= stats_.total_bytes_in_use) stats_.total_bytes_in_use -= old_bytes;
            ++stats_.deallocation_count;
        }
        std::size_t aligned_new = ((new_bytes + alignment - 1) / alignment) * alignment;
        stats_.total_bytes_in_use    += aligned_new;
        stats_.total_bytes_allocated += aligned_new;
        ++stats_.allocation_count;
        observer_.notify(stats_);
#endif
        return np;
    }

    [[nodiscard]] std::size_t usable_size(void* p) const noexcept {
        if constexpr (enabled) { return ::rpmalloc_usable_size(p); }
        else                   { (void)p; return 0; }
    }

private:
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::AllocationStatistics stats_{};
    observer_t observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::allocator::axis_06_allocator {
    static_assert(concepts::AllocatorStrategy<RPMallocAllocator>);
    static_assert(concepts::CacheEnginePermutationStrategy<RPMallocAllocator>);
    static_assert(concepts::ZeroingStrategy<RPMallocAllocator>);
    static_assert(concepts::ReallocatingStrategy<RPMallocAllocator>);
    static_assert(concepts::IntrospectableStrategy<RPMallocAllocator>);
}
