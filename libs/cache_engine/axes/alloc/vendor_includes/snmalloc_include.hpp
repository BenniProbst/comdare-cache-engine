#pragma once
// V41.F.6.1.C Stufe 1 Vendor-Header-Shim: Snmalloc (W6-Pattern)
//
// @vendor A07 Snmalloc
//
// EINZIGE Stelle mit `#if` fuer Snmalloc im Source-Tree.

#include <topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_SNMALLOC
#  include <snmalloc/snmalloc.h>
#else
// Forward-Stubs fuer if constexpr (false)-Zweig
namespace snmalloc::libc {
    inline void* aligned_alloc(std::size_t /*alignment*/, std::size_t /*size*/) noexcept {
        return nullptr;
    }
    inline void  free_aligned_sized(void* /*p*/, std::size_t /*alignment*/,
                                    std::size_t /*size*/) noexcept {}
    inline void* calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void* realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
    inline std::size_t malloc_usable_size(const void* /*p*/) noexcept { return 0; }
}  // namespace
#endif
