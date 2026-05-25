#pragma once
// V41.F.6.1.A Sub-Concept: Pool-Resettable (release_all, 2026-05-25 revidiert)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A
//
// **WICHTIG (User-Klarstellung 2026-05-25):**
//   Dieses Sub-Concept ist KEIN Statistik-Reset (das ist in CacheEnginePermutationStrategy
//   als reset() Pflicht-API). Hier geht es um **globales Freigeben aller bisherigen
//   Allokationen** (Pool/Arena/Bump-Allokatoren), OHNE pro-Block deallocate().
//
//   Daher Methodenname `release_all()` (nicht `reset()`), um Verwechslung auszuschliessen.
//
// Vendor: Pool-Allokatoren (Slab/Arena/Bump/Linear-Buffer), z.B. monotonic_buffer_resource,
//         eigene SlabAllocator-Implementierungen.

#include "axis_06_allocator_concept.hpp"

#include <concepts>

namespace comdare::cache_engine::allocator::axis_06_allocator::concepts {

/**
 * @brief PoolResettableStrategy - Allocator mit globalem Pool-Release
 * @topic allocator
 * @achse 6
 * @refines AllocatorStrategy
 *
 * release_all() gibt ALLE bisherigen Allokationen gleichzeitig frei.
 * Nur bei Pool/Arena/Bump-Allokatoren sinnvoll. libc malloc/jemalloc/tcmalloc
 * bieten das nicht an.
 *
 * Hinweis: Statistik-Reset zwischen Mess-Permutationen ist NICHT diese Funktion —
 * das ist `reset()` in CacheEnginePermutationStrategy.
 */
template <typename A>
concept PoolResettableStrategy = AllocatorStrategy<A>
    && requires(A a) {
        { a.release_all() } -> std::same_as<void>;
    };

}  // namespace comdare::cache_engine::allocator::axis_06_allocator::concepts
