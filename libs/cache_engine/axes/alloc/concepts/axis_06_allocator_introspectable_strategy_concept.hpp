#pragma once
// V41.F.6.1.A Sub-Concept: Introspectable-Allocation (usable_size, 2026-05-25)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A
//
// Optional-Refinement von AllocatorStrategy: Abfrage der tatsaechlich nutzbaren
// Groesse einer Allokation (interne Bin-Groesse).
//
// Vendor: mimalloc `mi_usable_size`, jemalloc `sallocx`, tcmalloc `tc_malloc_size`,
//         glibc `malloc_usable_size`, MSVC `_msize`.

#include "axis_06_allocator_concept.hpp"

#include <concepts>
#include <cstddef>

namespace comdare::cache_engine::alloc::concepts {

/**
 * @brief IntrospectableStrategy - Allocator mit usable_size-Abfrage
 * @topic allocator
 * @achse 6
 * @refines AllocatorStrategy
 */
template <typename A>
concept IntrospectableStrategy = AllocatorStrategy<A> && requires(A const& a, void* p) {
    { a.usable_size(p) } -> std::same_as<std::size_t>;
};

} // namespace comdare::cache_engine::alloc::concepts
