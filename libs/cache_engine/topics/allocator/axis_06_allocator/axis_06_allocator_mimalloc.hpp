#pragma once
// V41.F.6.1.C MimallocAllocator A04 (2026-05-25)
//
// @topic allocator
// @achse 6
// @family A04 (Mimalloc — Leijen/Zorn/de Moura, MSR-TR-2019-18 / APLAS 2019)
// @subaxis AA1 freelist_topology (Free-List-Sharding)
// @stand V41.F.6.1.C Batch 1 Vendor-Vollausbau (Direktive [[achsen-vendor-vollausbau]])
//
// **Vendor-Source:** ext/A04-mimalloc/include/mimalloc.h
// **Build-Detection:** #ifdef COMDARE_HAVE_MIMALLOC (CMake try_compile setze define)
// **Fallback:** portable_aligned_alloc (markiert real=std im Output)
//
// Erfuellt:
//   - AllocatorStrategy (Pflicht-Standard, PMR-Naming)
//   - CacheEnginePermutationStrategy (Pflicht cache-engine-spec)
//   - ZeroingStrategy            (mi_zalloc_aligned)
//   - ReallocatingStrategy       (mi_realloc_aligned)
//   - IntrospectableStrategy     (mi_usable_size)
//   - ReclaimableStrategy        (mi_collect)

#include "axis_06_allocator_strategy_base.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "concepts/axis_06_allocator_zeroing_strategy_concept.hpp"
#include "concepts/axis_06_allocator_reallocating_strategy_concept.hpp"
#include "concepts/axis_06_allocator_introspectable_strategy_concept.hpp"
#include "concepts/axis_06_allocator_reclaimable_strategy_concept.hpp"
#include "../concepts/topic_allocator_concept.hpp"

#include <cache_engine/allocators/portable_aligned_alloc.hpp>
#include <cstddef>
#include <cstring>
#include <string_view>
#include <type_traits>

#ifdef COMDARE_HAVE_MIMALLOC
#  include <mimalloc.h>
#endif

namespace comdare::cache_engine::allocator::axis_06_allocator {

/**
 * @brief MimallocAllocator - Wrapper auf microsoft/mimalloc (A04)
 *
 * Free-List-Sharding (Leijen MSR-TR-2019-18): 3 Free-Lists pro Page
 * (free / local_free / thread_free), temporal cadence, deferred_free hook.
 */
class MimallocAllocator : public AllocatorStrategyBase<MimallocAllocator> {
public:
    using value_type = std::byte;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = subaxes::freelist_topology_tag;
    using family_id  = std::integral_constant<int, 4>;

    [[nodiscard]] static constexpr bool        is_thread_safe()  noexcept { return true; }
    [[nodiscard]] static constexpr bool        supports_pmr()    noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t max_alignment()   noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept {
#ifdef COMDARE_HAVE_MIMALLOC
        return "mimalloc";
#else
        return "mimalloc(real=std)";
#endif
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "Mimalloc Free-List-Sharding (Leijen/Zorn/de Moura MSR-TR-2019-18 APLAS 2019)";
    }

    [[nodiscard]] bool operator==(MimallocAllocator const&) const noexcept { return true; }

    // ───────────────────────────────────────────────────────────────────────
    // AllocatorStrategy Pflicht (mi_malloc_aligned / mi_free direkter Vendor-Call)
    // ───────────────────────────────────────────────────────────────────────

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
#ifdef COMDARE_HAVE_MIMALLOC
        void* p = ::mi_malloc_aligned(bytes, alignment);
#else
        void* p = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, bytes);
#endif
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        if (p != nullptr) {
            ++stats_.allocation_count;
            stats_.total_bytes_allocated += aligned_bytes;
            stats_.total_bytes_in_use    += aligned_bytes;
        } else {
            ++stats_.failure_count;
        }
#endif
        return p;
    }

    void deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
        if (p == nullptr) return;
#ifdef COMDARE_HAVE_MIMALLOC
        // mi_free verarbeitet aligned-Allocations korrekt (kein separates mi_free_aligned)
        ::mi_free(p);
#else
        ::comdare::cache_engine::allocator::portable_aligned_free(p);
#endif
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        ++stats_.deallocation_count;
        if (aligned_bytes <= stats_.total_bytes_in_use) stats_.total_bytes_in_use -= aligned_bytes;
        else stats_.total_bytes_in_use = 0;
#else
        (void)bytes; (void)alignment;
#endif
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    [[nodiscard]] concepts::AllocationStatistics statistics() const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; }
#endif

    // ───────────────────────────────────────────────────────────────────────
    // Sub-Concept: ZeroingStrategy (mi_zalloc / mi_calloc Vendor-direkt)
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] void* zero_allocate(std::size_t n, std::size_t size) {
        std::size_t bytes = n * size;
#ifdef COMDARE_HAVE_MIMALLOC
        void* p = ::mi_calloc(n, size);
#else
        void* p = std::calloc(n, size);
#endif
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if (p != nullptr) {
            ++stats_.allocation_count;
            stats_.total_bytes_allocated += bytes;
            stats_.total_bytes_in_use    += bytes;
        } else {
            ++stats_.failure_count;
        }
#endif
        return p;
    }

    // ───────────────────────────────────────────────────────────────────────
    // Sub-Concept: ReallocatingStrategy (mi_realloc_aligned Vendor-direkt)
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] void* reallocate(void* p, std::size_t old_bytes, std::size_t new_bytes,
                                   std::size_t alignment) {
#ifdef COMDARE_HAVE_MIMALLOC
        void* np = ::mi_realloc_aligned(p, new_bytes, alignment);
#else
        // Portable Fallback: alloc-new + memcpy + free-old
        void* np = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, new_bytes);
        if (np != nullptr && p != nullptr) {
            std::size_t copy_bytes = (old_bytes < new_bytes) ? old_bytes : new_bytes;
            std::memcpy(np, p, copy_bytes);
            ::comdare::cache_engine::allocator::portable_aligned_free(p);
        }
#endif
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if (np == nullptr) { ++stats_.failure_count; return nullptr; }
        if (p != nullptr) {
            if (old_bytes <= stats_.total_bytes_in_use) stats_.total_bytes_in_use -= old_bytes;
            ++stats_.deallocation_count;
        }
        std::size_t aligned_new = ((new_bytes + alignment - 1) / alignment) * alignment;
        stats_.total_bytes_in_use    += aligned_new;
        stats_.total_bytes_allocated += aligned_new;
        ++stats_.allocation_count;
#endif
        return np;
    }

    // ───────────────────────────────────────────────────────────────────────
    // Sub-Concept: IntrospectableStrategy (mi_usable_size)
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] std::size_t usable_size(void* p) const noexcept {
#ifdef COMDARE_HAVE_MIMALLOC
        return ::mi_usable_size(p);
#else
        (void)p;
        return 0;  // libc malloc hat kein portables usable_size
#endif
    }

    // ───────────────────────────────────────────────────────────────────────
    // Sub-Concept: ReclaimableStrategy (mi_collect)
    // ───────────────────────────────────────────────────────────────────────
    void collect(bool force) noexcept {
#ifdef COMDARE_HAVE_MIMALLOC
        ::mi_collect(force);
#else
        (void)force;
        // no-op fuer std-fallback
#endif
    }

private:
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::AllocationStatistics stats_{};
#endif
};

}  // namespace comdare::cache_engine::allocator::axis_06_allocator

namespace comdare::cache_engine::allocator::axis_06_allocator {
    static_assert(concepts::AllocatorStrategy<MimallocAllocator>,
        "Pflicht: MimallocAllocator muss AllocatorStrategy erfuellen");
    static_assert(concepts::CacheEnginePermutationStrategy<MimallocAllocator>,
        "Pflicht: MimallocAllocator muss CacheEnginePermutationStrategy erfuellen");
    static_assert(concepts::ZeroingStrategy<MimallocAllocator>);
    static_assert(concepts::ReallocatingStrategy<MimallocAllocator>);
    static_assert(concepts::IntrospectableStrategy<MimallocAllocator>);
    static_assert(concepts::ReclaimableStrategy<MimallocAllocator>);
}  // namespace
