#pragma once
// V41.F.6.1 Batch 3 Vendor-Header-Shim: Slab Magazine-Cache (W6-Pattern)
//
// @vendor A02 Slab (Bonwick USENIX 1994 + Magazines USENIX 2001)

#include <topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_SLAB
// Userland-Slab-Implementation (z.B. tbb::scalable_allocator oder umem.h vom OpenSolaris).
// Pragmatisch: ext/A02-slab/slab.h definiert die API.
#  include <slab.h>
#else
// Forward-Stubs fuer if constexpr (false)-Zweig
extern "C" {
    inline void* slab_alloc(std::size_t /*size*/, std::size_t /*alignment*/) noexcept { return nullptr; }
    inline void  slab_free(void* /*p*/, std::size_t /*size*/) noexcept {}
    inline void* slab_calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void* slab_realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
}
#endif
