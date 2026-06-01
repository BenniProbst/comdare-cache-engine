#pragma once
// V41.F.6.1 Batch 5 Vendor-Header-Shim: LRMalloc (W6-Pattern)
//
// @vendor A11 LRMalloc (Leite/Rocha JPDC 2019)
// Lock-Free Memory Allocator using Hazard-Pointers fuer safe Reclamation.

#include <axes/alloc/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_LRMALLOC
#  include <lrmalloc.h>
#else
extern "C" {
    inline void* lrmalloc_alloc(std::size_t /*size*/, std::size_t /*alignment*/) noexcept { return nullptr; }
    inline void  lrmalloc_free(void* /*p*/) noexcept {}
    inline void* lrmalloc_calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void* lrmalloc_realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
}
#endif
