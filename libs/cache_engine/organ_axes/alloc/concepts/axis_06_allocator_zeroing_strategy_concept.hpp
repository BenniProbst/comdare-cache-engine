#pragma once
// V41.F.6.1.A Sub-Concept: Zeroing-Allocation (calloc-style, 2026-05-25)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A
//
// Optional-Refinement von AllocatorStrategy: zero-initialized Allocation
// (libc `calloc`, mimalloc `mi_calloc`, jemalloc `MALLOCX_ZERO`).
//
// Compile-Time-Detection im Permutations-Visitor:
//   if constexpr (ZeroingStrategy<S>) {
//       auto* p = strategy.zero_allocate(n, sizeof(T));
//   } else {
//       auto* p = strategy.allocate(n * sizeof(T), alignof(T));
//       std::memset(p, 0, n * sizeof(T));
//   }

#include "axis_06_allocator_concept.hpp"

#include <concepts>
#include <cstddef>

namespace comdare::cache_engine::alloc::concepts {

/**
 * @brief ZeroingStrategy - Allocator mit nativer zero-init API
 * @topic allocator
 * @achse 6
 * @refines AllocatorStrategy
 *
 * Liefert Speicher der bereits mit Null-Bytes initialisiert ist.
 * Vendor: libc `calloc(n, size)`, mimalloc `mi_calloc(n, size)`,
 *         jemalloc `mallocx(bytes, MALLOCX_ZERO)`, tcmalloc `tc_calloc`.
 */
template <typename A>
concept ZeroingStrategy = AllocatorStrategy<A> && requires(A a, std::size_t n, std::size_t size) {
    { a.zero_allocate(n, size) } -> std::same_as<void*>;
};

} // namespace comdare::cache_engine::alloc::concepts
