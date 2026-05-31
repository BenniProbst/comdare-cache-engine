#pragma once
// V41.F.6.1 Batch 6 Vendor-Header-Shim: HMalloc (W6-Pattern)
//
// @vendor A15 HMalloc Hybrid Free-List (Tang 2020 + Anwendungen)

#include <topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_HMALLOC
#  include <hmalloc.h>
#else
extern "C" {
    inline void* hmalloc_alloc(std::size_t /*size*/, std::size_t /*alignment*/) noexcept { return nullptr; }
    inline void  hmalloc_free(void* /*p*/) noexcept {}
    inline void* hmalloc_calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void* hmalloc_realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
}
#endif
