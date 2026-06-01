#pragma once
// V41.F.6.1 Batch 2 Vendor-Header-Shim: dlmalloc (W6-Pattern)
//
// @vendor A20 dlmalloc (Doug Lea 1987-2012)
//
// EINZIGE Stelle mit `#if` fuer dlmalloc im Source-Tree.

#include <axes/alloc/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_DLMALLOC
#  include <dlmalloc.h>
#else
// Forward-Stubs fuer if constexpr (false)-Zweig
extern "C" {
    inline void* dlmemalign(std::size_t /*alignment*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void  dlfree(void* /*p*/) noexcept {}
    inline void* dlcalloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void* dlrealloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
    inline std::size_t dlmalloc_usable_size(const void* /*p*/) noexcept { return 0; }
}
#endif
