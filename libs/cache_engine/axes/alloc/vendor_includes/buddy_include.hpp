#pragma once
// V41.F.6.1 Batch 7 Vendor-Header-Shim: Buddy (W6-Pattern)
//
// @vendor A19 Buddy Power-of-2 Splitting (Knuth TAoCP Vol 1, Classic 1968)

#include <axes/alloc/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_BUDDY
#  include <buddy_alloc.h>
#else
extern "C" {
    inline void* buddy_alloc(std::size_t /*size*/, std::size_t /*alignment*/) noexcept { return nullptr; }
    inline void  buddy_free(void* /*p*/) noexcept {}
    inline void* buddy_calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void* buddy_realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
}
#endif
