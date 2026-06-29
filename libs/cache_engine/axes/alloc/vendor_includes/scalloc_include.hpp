#pragma once
// V41.F.6.1 Batch 4 Vendor-Header-Shim: Scalloc (W6-Pattern)
//
// @vendor A08 Scalloc (Aigner/Iurca/Wimmer PPoPP 2015)
//
// SONDERFALL: Scalloc Standard-API hat KEINE native aligned_alloc.
// Aligned-Allocation muss via Overallocation + manueller Pointer-Justierung
// erfolgen — siehe Wrapper-Klasse axis_06_allocator_scalloc.hpp.

#include <axes/alloc/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_SCALLOC
#include <scalloc.h>
#else
// Forward-Stubs fuer if constexpr (false)-Zweig
extern "C" {
inline void* scalloc_malloc(std::size_t /*size*/) noexcept { return nullptr; }
inline void  scalloc_free(void* /*p*/) noexcept {}
inline void* scalloc_calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
inline void* scalloc_realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
}
#endif
