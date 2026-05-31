#pragma once
// V41.F.6.1 Batch 4 Vendor-Header-Shim: RPMalloc (W6-Pattern)
//
// @vendor A10 RPMalloc (Mattias Jansson 2017, Per-Thread Spans)
//
// SONDERFALL: PFLICHT-INIT vor erstem Aufruf:
//   rpmalloc_initialize() — pro Prozess EINMAL
//   rpmalloc_thread_initialize() — pro Thread EINMAL
//   rpmalloc_thread_finalize(1) — beim Thread-Ende
// Wrapper-Klasse axis_06_allocator_rpmalloc.hpp nutzt static-Init-Flag fuer
// init-on-first-use Pattern.

#include <topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_RPMALLOC
#  include <rpmalloc.h>
#else
// Forward-Stubs fuer if constexpr (false)-Zweig
extern "C" {
    inline int   rpmalloc_initialize(void) noexcept { return 0; }
    inline void  rpmalloc_finalize(void) noexcept {}
    inline int   rpmalloc_thread_initialize(void) noexcept { return 0; }
    inline void  rpmalloc_thread_finalize(int /*release_caches*/) noexcept {}
    inline void* rpaligned_alloc(std::size_t /*alignment*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void  rpfree(void* /*p*/) noexcept {}
    inline void* rpcalloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void* rprealloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
    inline std::size_t rpmalloc_usable_size(void* /*p*/) noexcept { return 0; }
}
#endif
