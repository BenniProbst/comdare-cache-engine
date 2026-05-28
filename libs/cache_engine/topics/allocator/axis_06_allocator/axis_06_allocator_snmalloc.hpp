#pragma once
// V41.F.6.1.C SnmallocAllocator A07 (2026-05-25)
//
// @topic allocator
// @achse 6
// @family A07 (Snmalloc — Paul Liétar et al., ISMM 2019)
// @subaxis AA3 thread_locality (Message-Passing zwischen Threads)
// @stand V41.F.6.1.C Batch 1 Vendor-Vollausbau
//
// **Vendor-Source:** ext/A07-snmalloc/src/snmalloc/snmalloc.h (header-only)
// **Build-Detection:** #ifdef COMDARE_HAVE_SNMALLOC
// **Fallback:** portable_aligned_alloc
//
// Snmalloc Message-Passing: Free-Operationen werden via Lock-Free-Queue an
// den Allocating-Thread zurueckgesandt (vermeidet Cross-Thread-Lock-Contention).

#include "axis_06_allocator_strategy_base.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "concepts/axis_06_allocator_zeroing_strategy_concept.hpp"
#include "concepts/axis_06_allocator_reallocating_strategy_concept.hpp"
#include "concepts/axis_06_allocator_introspectable_strategy_concept.hpp"
#include "../concepts/topic_allocator_concept.hpp"

#include <topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp>
#include "vendor_includes/snmalloc_include.hpp"   // V41.F.6.1.C Stufe 2: Shim mit Forward-Stubs

// V41.F.6.1.P2.D snmalloc Paper-Legacy-Code Mixin (auto-generated via Pre-Build-Tool)
#include "concepts/axis_06_allocator_original_code_mixin.hpp"
#include <topics/allocator/axis_06_allocator/legacy_code/paper_a07_snmalloc_is_original.hpp>

#include <cache_engine/allocators/portable_aligned_alloc.hpp>
#include <measurement/measurable_concept.hpp>   // V41.F.6.1 Stufe 3: MeasurableObserver<snapshot_t>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::allocator::axis_06_allocator {

/**
 * @brief SnmallocAllocator - Wrapper auf microsoft/snmalloc (A07)
 *
 * Message-Passing-Allocator (Paul Liétar et al. ISMM 2019).
 *
 * V41.F.6.1.C Stufe 2 (W6-Pattern): KEIN #ifdef mehr. enabled via flags::snmalloc_enabled.
 */
class SnmallocAllocator
    : public AllocatorStrategyBase<SnmallocAllocator>,
      public generated::a07_snmalloc::OriginalCodeMixin {  // V41.F.6.1.P2.D Paper-Mixin (Habich-Compliance)
public:
    // V41.F.6.1.P2.D Diamond-Disambiguation (Pattern wie MimallocAllocator):
    using generated::a07_snmalloc::OriginalCodeMixin::get_compiler;
    using generated::a07_snmalloc::OriginalCodeMixin::is_original_allocate;
    using generated::a07_snmalloc::OriginalCodeMixin::is_original_deallocate;
    using generated::a07_snmalloc::OriginalCodeMixin::is_original_module;

    static constexpr bool enabled = flags::snmalloc_enabled;

    using value_type = std::byte;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = subaxes::thread_locality_tag;
    using family_id  = std::integral_constant<int, 7>;

    [[nodiscard]] static constexpr bool        is_thread_safe()  noexcept { return true; }
    [[nodiscard]] static constexpr bool        supports_pmr()    noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t max_alignment()   noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept {
        if constexpr (enabled) { return "snmalloc"; }
        else                   { return "snmalloc(real=std)"; }
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "Snmalloc Message-Passing (Paul Liétar et al., ISMM 2019)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "SNMALLOC"; }

    // V41.F.6.1 Vendor-Sonderfall-Properties (Pflicht, [[vendor-sonderfaelle-als-pflicht-property]])
    [[nodiscard]] static constexpr bool has_native_aligned_alloc()    noexcept { return true; }   // snmalloc::libc::aligned_alloc
    [[nodiscard]] static constexpr bool requires_explicit_init()      noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_numa_node_hint()     noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_thread_local_cache() noexcept { return true; }   // per-thread allocator slabs
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept { return concepts::ProgressGuarantee::LockFree; }  // Message-Passing-Queue lock-free
    [[nodiscard]] static constexpr bool requires_specialized_hardware() noexcept { return false; }

    [[nodiscard]] bool operator==(SnmallocAllocator const&) const noexcept { return true; }

    // ───────────────────────────────────────────────────────────────────────
    // AllocatorStrategy Pflicht (snmalloc::libc::aligned_alloc / free_aligned_sized)
    // ───────────────────────────────────────────────────────────────────────

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        void* p;
        if constexpr (enabled) {
            // snmalloc verlangt size als Multiple of alignment fuer aligned_alloc
            std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
            p = ::snmalloc::libc::aligned_alloc(alignment, aligned_bytes);
        } else {
            p = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, bytes);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_bytes_track = ((bytes + alignment - 1) / alignment) * alignment;
        if (p != nullptr) {
            ++stats_.allocation_count;
            stats_.total_bytes_allocated += aligned_bytes_track;
            stats_.total_bytes_in_use    += aligned_bytes_track;
        } else {
            ++stats_.failure_count;
        }
        observer_.notify(stats_);
#endif
        return p;
    }

    void deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
        if (p == nullptr) return;
        if constexpr (enabled) {
            ::snmalloc::libc::free_aligned_sized(p, alignment, bytes);
        } else {
            ::comdare::cache_engine::allocator::portable_aligned_free(p);
        }
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

    // ───────────────────────────────────────────────────────────────────────
    // Sub-Concept: ZeroingStrategy (snmalloc::libc::calloc)
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] void* zero_allocate(std::size_t n, std::size_t size) {
        std::size_t bytes = n * size;
        void* p;
        if constexpr (enabled) { p = ::snmalloc::libc::calloc(n, size); }
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

    // ───────────────────────────────────────────────────────────────────────
    // Sub-Concept: ReallocatingStrategy
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] void* reallocate(void* p, std::size_t old_bytes, std::size_t new_bytes,
                                   std::size_t alignment) {
        void* np;
        if constexpr (enabled) {
            // snmalloc realloc respektiert Alignment automatisch
            np = ::snmalloc::libc::realloc(p, new_bytes);
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

    // ───────────────────────────────────────────────────────────────────────
    // Sub-Concept: IntrospectableStrategy (snmalloc::libc::malloc_usable_size)
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] std::size_t usable_size(void* p) const noexcept {
        if constexpr (enabled) { return ::snmalloc::libc::malloc_usable_size(p); }
        else                   { (void)p; return 0; }
    }

private:
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::AllocationStatistics stats_{};
    observer_t observer_{};
#endif
};

}  // namespace comdare::cache_engine::allocator::axis_06_allocator

namespace comdare::cache_engine::allocator::axis_06_allocator {
    static_assert(concepts::AllocatorStrategy<SnmallocAllocator>);
    static_assert(concepts::CacheEnginePermutationStrategy<SnmallocAllocator>);
    static_assert(concepts::ZeroingStrategy<SnmallocAllocator>);
    static_assert(concepts::ReallocatingStrategy<SnmallocAllocator>);
    static_assert(concepts::IntrospectableStrategy<SnmallocAllocator>);
}  // namespace
