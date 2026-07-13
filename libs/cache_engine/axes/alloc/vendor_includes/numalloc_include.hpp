#pragma once
// V41.F.6.1 Batch 4 Vendor-Header-Shim: NUMAlloc (W6-Pattern)
//
// @vendor A09 NUMAlloc (UTSASRG, ISMM 2023)
//
// SONDERFALL: NUMA-Node-Parameter — `numalloc_alloc(size, node)`.
// Wir uebergeben node=-1 (kernel-Default = aktueller NUMA-Node der CPU).
// Auf Single-Node-Systemen identisch zu normaler Allokation.

#include <axes/alloc/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_NUMALLOC
#include <numalloc.h>
#else
// Forward-Stubs fuer if constexpr (false)-Zweig
extern "C" {
inline void* numalloc_alloc(std::size_t /*size*/, std::size_t /*alignment*/, int /*node*/) noexcept { return nullptr; }
inline void  numalloc_free(void* /*p*/, std::size_t /*size*/) noexcept {}
inline void* numalloc_calloc(std::size_t /*n*/, std::size_t /*size*/, int /*node*/) noexcept { return nullptr; }
inline void* numalloc_realloc(void* /*p*/, std::size_t /*new_size*/, int /*node*/) noexcept { return nullptr; }
}
#endif
