#pragma once
// V41.F.6.1 Batch 5 Vendor-Header-Shim: CAMA (W6-Pattern)
//
// @vendor A12 CAMA Cache-Aware (Bhattacharyya/Beard/Cohen 2020)

#include <axes/alloc/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_CAMA
#  include <cama.h>
#else
extern "C" {
    inline void* cama_alloc(std::size_t /*size*/, std::size_t /*alignment*/) noexcept { return nullptr; }
    inline void  cama_free(void* /*p*/) noexcept {}
    inline void* cama_calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void* cama_realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
}
#endif
