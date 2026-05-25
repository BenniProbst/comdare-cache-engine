#pragma once
// V41.F.6.1.A Sub-Concept: Resettable-Allocation (Pool/Arena reset, 2026-05-25)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A
//
// Optional-Refinement von AllocatorStrategy: vollstaendiges Zuruecksetzen aller
// bisher allokierten Bloecke ohne einzelnes deallocate().
//
// **WICHTIG:** Das ist NICHT das C-konforme `free()`-Aequivalent (= deallocate).
// reset() bedeutet: alle Allokationen werden GLOBAL gleichzeitig freigegeben.
// Nur bei Pool/Arena/Bump-Allokatoren sinnvoll, libc/jemalloc/tcmalloc bieten
// das nicht an.
//
// Vendor: Pool-Allokatoren (Slab/Arena/Bump/Linear-Buffer).

#include "axis_06_allocator_concept.hpp"

#include <concepts>

namespace comdare::cache_engine::allocator::axis_06_allocator::concepts {

/**
 * @brief ResettableStrategy - Allocator mit globalem Reset
 * @topic allocator
 * @achse 6
 * @refines AllocatorStrategy
 *
 * Pool/Arena/Bump-spezifisch. Erlaubt schnelles Zuruecksetzen ohne pro-Block deallocate.
 * Nuetzlich fuer Mess-Reihen die zwischen Permutationen alle Allokationen schnell loswerden wollen.
 */
template <typename A>
concept ResettableStrategy = AllocatorStrategy<A>
    && requires(A a) {
        { a.reset() } -> std::same_as<void>;
    };

}  // namespace comdare::cache_engine::allocator::axis_06_allocator::concepts
