#pragma once
// V41.F.6.1.A Sub-Concept: Reclaimable-Allocation (collect/purge, 2026-05-25)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A
//
// Optional-Refinement von AllocatorStrategy: aktive Speicher-Rueckgabe an OS.
// Wichtig fuer Long-Running-Apps + Mess-Reihen die ueber Permutationen freien
// Speicher zurueckgeben sollen.
//
// Vendor: mimalloc `mi_collect`, jemalloc `purge`, tcmalloc `ReleaseFreeMemory`.

#include "axis_06_allocator_concept.hpp"

#include <concepts>

namespace comdare::cache_engine::allocator::axis_06_allocator::concepts {

/**
 * @brief ReclaimableStrategy - Allocator mit aktivem Memory-Reclaim
 * @topic allocator
 * @achse 6
 * @refines AllocatorStrategy
 *
 * collect(force):
 *   - force=true  -> aggressive Speicher-Rueckgabe an OS (madvise(DONTNEED) etc.)
 *   - force=false -> opportunistische Bereinigung
 */
template <typename A>
concept ReclaimableStrategy = AllocatorStrategy<A>
    && requires(A a, bool force) {
        a.collect(force);
    };

}  // namespace comdare::cache_engine::allocator::axis_06_allocator::concepts
