#pragma once
// V41.F.6.1 Batch 8 Vendor-Header-Shim: ptmalloc2 (W6-Pattern)
//
// @vendor A21 ptmalloc2 explizit (Wolfram Gloger, glibc malloc Default-Implementation)
//
// Im Gegensatz zu StdMalloc (A22 = plattform-default std::malloc) ruft dieser
// Wrapper explizit __libc_malloc/__libc_free Symbole auf — bypassed LD_PRELOAD
// und garantiert ptmalloc2-Implementation auch wenn andere Allokator-Library
// im Prozess vorhanden ist.

#include <topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_PTMALLOC2
#  include <malloc.h>  // __libc_malloc etc. (glibc-spezifisch)
#else
extern "C" {
    inline void* __libc_memalign(std::size_t /*alignment*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void  __libc_free(void* /*p*/) noexcept {}
    inline void* __libc_calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void* __libc_realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
}
#endif
