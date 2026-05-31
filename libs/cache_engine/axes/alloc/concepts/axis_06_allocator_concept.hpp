#pragma once
// V41.F.6.1.A Standard-Allokator Pflicht-Concept (2026-05-25, W1 Web-Recherche revidiert)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A
//
// **Pflicht-Concept fuer ALLE Allokator-Familien** (libc malloc, jemalloc, tcmalloc,
// mimalloc, snmalloc, Hoard, dlmalloc, ptmalloc2, std::allocator, std::pmr::memory_resource).
//
// Diese Pflicht-API ist die SCHNITTMENGE aller modernen Allokator-Bibliotheken
// (ISO C11/C17 §7.22.3, C++17 [mem.res.public], C++23 [allocator.requirements]).
//
// Nicht-standardisierbare API (statistics, mallopt, vendor-spezifisch) ist in
// separaten Sub-Concepts (siehe Geschwister-Files):
//   - axis_06_allocator_zeroing_strategy_concept.hpp       (calloc)
//   - axis_06_allocator_overallocating_strategy_concept.hpp (C++23 allocate_at_least)
//   - axis_06_allocator_introspectable_strategy_concept.hpp (usable_size)
//   - axis_06_allocator_reclaimable_strategy_concept.hpp    (collect/purge)
//   - axis_06_allocator_resettable_strategy_concept.hpp     (Pool/Arena reset)
//   - axis_06_allocator_reallocating_strategy_concept.hpp   (realloc)
//
// cache-engine-spezifische Pflicht-API (axis_tag/family_id/name/...):
//   - axis_06_allocator_cache_engine_permutation_concept.hpp (parallel zu AllocatorStrategy)

#include <topics/allocator/concepts/topic_allocator_concept.hpp>

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::alloc::concepts {

/**
 * @brief AllocatorStrategy - PFLICHT-Concept fuer alle Allokator-Familien
 * @topic allocator
 * @achse 6
 *
 * Schnittmenge aller modernen Allokator-Familien (ISO C11/C17, C++17 PMR, C++23):
 *
 * **(1) Typedefs (Pflicht):**
 *   - typename A::value_type      (z.B. std::byte fuer raw allokator)
 *   - typename A::size_type       (typischerweise std::size_t)
 *
 * **(2) Runtime-API (Pflicht):**
 *   - allocate(bytes, alignment)   -> void*   (PMR-konform)
 *   - deallocate(p, bytes, align)  noexcept   (sized+aligned, C++17 [mem.res.public])
 *
 * **(3) Identitaet (Pflicht):**
 *   - operator==(other)            (STL/PMR Allocator-Gleichheit)
 *   - std::copy_constructible<A>
 *   - std::is_nothrow_destructible_v<A>
 *
 * **Beispiel-Wrapper (mimalloc):**
 * ```cpp
 * struct MimallocStrategy {
 *     using value_type = std::byte;
 *     using size_type  = std::size_t;
 *     void* allocate(std::size_t bytes, std::size_t align) {
 *         return ::mi_malloc_aligned(bytes, align);
 *     }
 *     void deallocate(void* p, std::size_t bytes, std::size_t align) noexcept {
 *         ::mi_free_size_aligned(p, bytes, align);
 *     }
 *     bool operator==(const MimallocStrategy&) const noexcept { return true; }
 * };
 * static_assert(AllocatorStrategy<MimallocStrategy>);
 * ```
 *
 * **WICHTIG zu C-konformem free:**
 *   `deallocate(p, bytes, alignment)` ist die C++17-PMR-Form von `free` — sized + aligned.
 *   Naked `free(p)` (libc) ist trivial drauf zurueckfuehrbar im Wrapper.
 *   `reset()` ist KEIN free-Aequivalent — gehoert in Sub-Concept ResettableStrategy.
 */
template <typename A>
concept AllocatorStrategy =
    ::comdare::cache_engine::allocator::concepts::AllocatorComponent<A>
    && requires {
        typename A::value_type;
        typename A::size_type;
    }
    && requires(A a, void* p, std::size_t bytes, std::size_t align) {
        // (2) Pflicht Runtime-API
        { a.allocate(bytes, align) }                -> std::same_as<void*>;
        { a.deallocate(p, bytes, align) } noexcept;
    }
    && requires(A const& a, A const& b) {
        // (3) Identitaet
        { a == b } -> std::convertible_to<bool>;
    }
    && std::copy_constructible<A>
    && std::is_nothrow_destructible_v<A>;

}  // namespace comdare::cache_engine::alloc::concepts
