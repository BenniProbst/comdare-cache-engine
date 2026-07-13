#pragma once
// V41.F.6.1 Batch 5 Vendor-Header-Shim: StarMalloc (W6-Pattern)
//
// @vendor A13 StarMalloc Formal Verified (Inria-Prosecco: Reitz/Fromherz/Protzenko et al., OOPSLA 2024)
//
// SONDERFALL/HINWEIS: Formal-verified Allocator (F* + KaRaMeL extraction).
// Build-Time-Eigenschaft, KEINE Runtime-Property (laeuft identisch wie andere).

#include <axes/alloc/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_STARMALLOC
#include <starmalloc.h>
#else
extern "C" {
inline void* starmalloc_alloc(std::size_t /*size*/, std::size_t /*alignment*/) noexcept { return nullptr; }
inline void  starmalloc_free(void* /*p*/) noexcept {}
inline void* starmalloc_calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
inline void* starmalloc_realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
}
#endif
