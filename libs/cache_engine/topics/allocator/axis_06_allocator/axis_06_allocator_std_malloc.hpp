#pragma once
// V41.F.6.1.A StdMalloc Concept-Beweis-Klasse (libc malloc Wrapper, 2026-05-25, W1-revidiert)
//
// @topic allocator
// @achse 6
// @family A22 (ptmalloc2 / glibc malloc)
// @subaxis AA2 size_class_schema
// @stand V41.F.6.1.A
//
// **Concept-Beweis:** erste konkrete Klasse die ALLE relevanten Concepts erfuellt:
//   - AllocatorStrategy                  (Pflicht-Standard)
//   - CacheEnginePermutationStrategy     (Pflicht cache-engine-spec)
//   - ZeroingStrategy                    (calloc)
//   - ReallocatingStrategy               (realloc)
//
// NICHT erfuellt: ResettableStrategy (libc malloc kennt kein globales reset),
//                 IntrospectableStrategy (kein portables usable_size — glibc spezifisch),
//                 OverAllocatingStrategy (kein std::allocator-internal),
//                 ReclaimableStrategy (kein malloc_trim portable).

#include "axis_06_allocator_strategy_base.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "concepts/axis_06_allocator_zeroing_strategy_concept.hpp"
#include "concepts/axis_06_allocator_reallocating_strategy_concept.hpp"
#include "../concepts/topic_allocator_concept.hpp"

#include <cache_engine/allocators/portable_aligned_alloc.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::allocator::axis_06_allocator {

/**
 * @brief StdMalloc - libc malloc Concept-Beweis-Klasse
 * @topic allocator
 * @achse 6
 * @subaxis AA2 size_class_schema_tag
 * @family A22 (ptmalloc2 / glibc)
 *
 * Erfuellt:
 *   - AllocatorStrategy                  (allocate/deallocate/value_type/size_type/operator==)
 *   - CacheEnginePermutationStrategy     (axis_tag/family_id/name/.../statistics)
 *   - ZeroingStrategy                    (zero_allocate via calloc)
 *   - ReallocatingStrategy               (reallocate via realloc)
 */
class StdMalloc : public AllocatorStrategyBase<StdMalloc> {
public:
    // ───────────────────────────────────────────────────────────────────────
    // Standard-AllocatorStrategy Pflicht-Typedefs
    // ───────────────────────────────────────────────────────────────────────
    using value_type = std::byte;
    using size_type  = std::size_t;

    // ───────────────────────────────────────────────────────────────────────
    // CacheEnginePermutationStrategy Pflicht: Compile-Time-Eigenschaften
    // ───────────────────────────────────────────────────────────────────────
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = subaxes::size_class_schema_tag;
    using family_id  = std::integral_constant<int, 22>;   // A22 ptmalloc2/glibc

    [[nodiscard]] static constexpr bool        is_thread_safe()   noexcept { return true; }
    [[nodiscard]] static constexpr bool        supports_pmr()     noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t max_alignment()    noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "std_malloc"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "Standard libc malloc (ptmalloc2 / glibc)"; }

    // ───────────────────────────────────────────────────────────────────────
    // AllocatorStrategy: Identitaet (operator==)
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] bool operator==(StdMalloc const&) const noexcept {
        // libc malloc ist global — alle Instanzen sind aequivalent
        return true;
    }

    // ───────────────────────────────────────────────────────────────────────
    // AllocatorStrategy Pflicht: Runtime-API (PMR-Naming, sized+aligned)
    // ───────────────────────────────────────────────────────────────────────

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        void* p = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, bytes);
        if (p != nullptr) {
            ++stats_.allocation_count;
            stats_.total_bytes_allocated += aligned_bytes;
            stats_.total_bytes_in_use    += aligned_bytes;
        } else {
            ++stats_.failure_count;
        }
        return p;
    }

    void deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
        if (p == nullptr) return;
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        ::comdare::cache_engine::allocator::portable_aligned_free(p);
        ++stats_.deallocation_count;
        if (aligned_bytes <= stats_.total_bytes_in_use) {
            stats_.total_bytes_in_use -= aligned_bytes;
        } else {
            stats_.total_bytes_in_use = 0;
        }
    }

    // ───────────────────────────────────────────────────────────────────────
    // CacheEnginePermutationStrategy Pflicht: Mess-API
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] concepts::AllocationStatistics statistics() const noexcept {
        return stats_;
    }

    // ───────────────────────────────────────────────────────────────────────
    // Sub-Concept: ZeroingStrategy (calloc-style)
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] void* zero_allocate(std::size_t n, std::size_t size) {
        std::size_t bytes = n * size;
        void* p = std::calloc(n, size);
        if (p != nullptr) {
            ++stats_.allocation_count;
            stats_.total_bytes_allocated += bytes;
            stats_.total_bytes_in_use    += bytes;
        } else {
            ++stats_.failure_count;
        }
        return p;
    }

    // ───────────────────────────────────────────────────────────────────────
    // Sub-Concept: ReallocatingStrategy
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] void* reallocate(void* p, std::size_t old_bytes, std::size_t new_bytes,
                                   std::size_t alignment) {
        // Portable Pfad: alloc-new + memcpy + free-old (POSIX-/MSVC-sicher,
        // weil _aligned_malloc-Speicher nicht mit std::realloc kompatibel ist).
        void* np = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, new_bytes);
        if (np == nullptr) {
            ++stats_.failure_count;
            return nullptr;
        }
        if (p != nullptr) {
            std::size_t copy_bytes = (old_bytes < new_bytes) ? old_bytes : new_bytes;
            std::memcpy(np, p, copy_bytes);
            ::comdare::cache_engine::allocator::portable_aligned_free(p);
            if (old_bytes <= stats_.total_bytes_in_use)
                stats_.total_bytes_in_use -= old_bytes;
            ++stats_.deallocation_count;
        }
        std::size_t aligned_new = ((new_bytes + alignment - 1) / alignment) * alignment;
        stats_.total_bytes_in_use    += aligned_new;
        stats_.total_bytes_allocated += aligned_new;
        ++stats_.allocation_count;
        return np;
    }

private:
    concepts::AllocationStatistics stats_{};
};

}  // namespace comdare::cache_engine::allocator::axis_06_allocator

// ───────────────────────────────────────────────────────────────────────────
// Compile-Time-Beweise: alle erfuellten Concepts
// ───────────────────────────────────────────────────────────────────────────
namespace comdare::cache_engine::allocator::axis_06_allocator {
    static_assert(concepts::AllocatorStrategy<StdMalloc>,
        "Pflicht: StdMalloc muss AllocatorStrategy erfuellen (Standard-PMR-API)");
    static_assert(concepts::CacheEnginePermutationStrategy<StdMalloc>,
        "Pflicht: StdMalloc muss CacheEnginePermutationStrategy erfuellen (cache-engine-spec)");
    static_assert(concepts::ZeroingStrategy<StdMalloc>,
        "Optional: StdMalloc bietet zero_allocate (calloc)");
    static_assert(concepts::ReallocatingStrategy<StdMalloc>,
        "Optional: StdMalloc bietet reallocate (realloc)");
}  // namespace
