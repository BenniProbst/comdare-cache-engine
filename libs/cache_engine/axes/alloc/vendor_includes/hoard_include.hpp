#pragma once
// V41.F.6.1 Batch 3 Vendor-Header-Shim: Hoard (W6-Pattern)
//
// @vendor A01 Hoard (Berger/McKinley/Blumofe/Wilson, ASPLOS-IX 2000)

#include <axes/alloc/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_HOARD
// Hoard intercept via LD_PRELOAD (libhoard.so) — Standard libc-API verfuegbar.
// Wenn man explizit Hoard linkt: <hoardheap.h> + custom API.
#else
// Forward-Stubs fuer if constexpr (false)-Zweig
extern "C" {
inline void* hoard_memalign(std::size_t /*alignment*/, std::size_t /*size*/) noexcept { return nullptr; }
inline void  hoard_free(void* /*p*/) noexcept {}
inline void* hoard_calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
inline void* hoard_realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
}
#endif
