#pragma once
// V41.F.6.1 Batch 7 Vendor-Header-Shim: Crystalline (W6-Pattern)
//
// @vendor A17 Crystalline Wait-Free Reclamation (Solodkyy/Bunkov PLDI 2021)
// SONDERFALL: Wait-Free (strikter als Lock-Free) — alle Threads progress-garantiert.

#include <axes/alloc/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_CRYSTALLINE
#include <crystalline.h>
#else
extern "C" {
inline void* crystalline_alloc(std::size_t /*size*/, std::size_t /*alignment*/) noexcept { return nullptr; }
inline void  crystalline_free(void* /*p*/) noexcept {}
inline void* crystalline_calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
inline void* crystalline_realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
}
#endif
