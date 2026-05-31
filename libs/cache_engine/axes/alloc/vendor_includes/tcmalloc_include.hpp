#pragma once
// V41.F.6.1 Batch 2 Vendor-Header-Shim: TCMalloc (W6-Pattern)
//
// @vendor A06 TCMalloc (Google gperftools 2005)
//
// EINZIGE Stelle mit `#if` fuer TCMalloc im Source-Tree.

#include <topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_TCMALLOC
#  include <gperftools/tcmalloc.h>
#else
// Forward-Stubs fuer if constexpr (false)-Zweig
extern "C" {
    inline void* tc_memalign(std::size_t /*alignment*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void  tc_free(void* /*p*/) noexcept {}
    inline void* tc_calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void* tc_realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
    inline std::size_t tc_malloc_size(const void* /*p*/) noexcept { return 0; }
}
#endif
