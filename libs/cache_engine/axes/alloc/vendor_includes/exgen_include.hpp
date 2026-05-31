#pragma once
// V41.F.6.1 Batch 7 Vendor-Header-Shim: Exgen-Malloc (W6-Pattern)
//
// @vendor A18 Exgen-Malloc Exception-Generated Single-Thread Specialized
// Optimiert fuer single-threaded Code mit Exception-Safety.

#include <topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_EXGEN
#  include <exgen_malloc.h>
#else
extern "C" {
    inline void* exgen_alloc(std::size_t /*size*/, std::size_t /*alignment*/) noexcept { return nullptr; }
    inline void  exgen_free(void* /*p*/) noexcept {}
    inline void* exgen_calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void* exgen_realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
}
#endif
