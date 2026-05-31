#pragma once
// V41.F.6.1.A Sub-Concept: Reallocating-Allocation (realloc, 2026-05-25)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A
//
// Optional-Refinement von AllocatorStrategy: in-place oder kopier-realloc.
// In-Place wenn moeglich (jemalloc xallocx, mimalloc mi_expand), sonst alloc+memcpy+free.
//
// Vendor: libc `realloc`, jemalloc `rallocx`, mimalloc `mi_realloc_aligned`,
//         tcmalloc `tc_realloc`. Nur snmalloc/Hoard haben es nicht direkt.

#include "axis_06_allocator_concept.hpp"

#include <concepts>
#include <cstddef>

namespace comdare::cache_engine::alloc::concepts {

/**
 * @brief ReallocatingStrategy - Allocator mit realloc-API
 * @topic allocator
 * @achse 6
 * @refines AllocatorStrategy
 *
 * reallocate(p, old_bytes, new_bytes, align):
 *   - Liefert ggf. denselben Pointer p (in-place expansion)
 *   - oder neuen Pointer mit kopiertem Inhalt
 *   - nullptr wenn fehlgeschlagen (alter p bleibt gueltig)
 */
template <typename A>
concept ReallocatingStrategy = AllocatorStrategy<A>
    && requires(A a, void* p, std::size_t old_bytes, std::size_t new_bytes, std::size_t align) {
        { a.reallocate(p, old_bytes, new_bytes, align) } -> std::same_as<void*>;
    };

}  // namespace comdare::cache_engine::alloc::concepts
