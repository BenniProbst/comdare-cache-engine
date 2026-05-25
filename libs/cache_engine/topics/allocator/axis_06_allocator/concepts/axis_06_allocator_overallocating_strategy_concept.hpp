#pragma once
// V41.F.6.1.A Sub-Concept: Over-Allocation (C++23 allocate_at_least, 2026-05-25)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A
//
// Optional-Refinement von AllocatorStrategy: liefert mindestens N Bytes,
// aber moeglicherweise mehr — z.B. wenn Vendor-Bin-Groessen aufrunden.
// Spart Speicher-Verschwendung wenn der Aufrufer mehr nutzen kann.
//
// Referenz: WG21 P0401R6 (C++23), jemalloc nallocx+mallocx, mimalloc mi_good_size,
//           tcmalloc nallocx.

#include "axis_06_allocator_concept.hpp"

#include <concepts>
#include <cstddef>
#include <utility>

namespace comdare::cache_engine::allocator::axis_06_allocator::concepts {

/**
 * @brief AllocationResult - Pair von {Pointer, tatsaechliche Bytes}
 */
struct AllocationResult {
    void*        pointer;
    std::size_t  actual_bytes;
};

/**
 * @brief OverAllocatingStrategy - Allocator mit allocate_at_least Semantik
 * @topic allocator
 * @achse 6
 * @refines AllocatorStrategy
 *
 * Vendor: std::allocator (C++23), jemalloc nallocx/mallocx, mimalloc mi_good_size,
 *         tcmalloc nallocx, snmalloc external sizes.
 */
template <typename A>
concept OverAllocatingStrategy = AllocatorStrategy<A>
    && requires(A a, std::size_t bytes, std::size_t align) {
        { a.allocate_at_least(bytes, align) } -> std::same_as<AllocationResult>;
    };

}  // namespace comdare::cache_engine::allocator::axis_06_allocator::concepts
