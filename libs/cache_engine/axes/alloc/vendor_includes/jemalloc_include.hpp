#pragma once
// V41.F.6.1 Batch 2 Vendor-Header-Shim: Jemalloc (W6-Pattern)
//
// @vendor A05 Jemalloc (Evans 2006, FreeBSD/Facebook)
//
// EINZIGE Stelle mit `#if` fuer Jemalloc im Source-Tree.

#include <axes/alloc/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_JEMALLOC
#include <jemalloc/jemalloc.h>
#else
// Forward-Stubs fuer if constexpr (false)-Zweig
extern "C" {
inline void*       je_aligned_alloc(std::size_t /*alignment*/, std::size_t /*size*/) noexcept { return nullptr; }
inline void        je_free(void* /*p*/) noexcept {}
inline void*       je_calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
inline void*       je_realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
inline std::size_t je_malloc_usable_size(const void* /*p*/) noexcept { return 0; }
}
#endif
