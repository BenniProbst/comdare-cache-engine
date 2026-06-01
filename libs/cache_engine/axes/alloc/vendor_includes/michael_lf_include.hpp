#pragma once
// V41.F.6.1 Batch 3 Vendor-Header-Shim: Michael Lock-Free CAS (W6-Pattern)
//
// @vendor A03 Michael Lock-Free (Michael PODC 2002 + JPDC 2004)
//
// Lock-Free Memory Allocator using CAS-only operations (kein mutex).

#include <axes/alloc/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_MICHAEL_LF
// ext/A03-michael-lockfree/michael_lf.h definiert die Lock-Free-API
#  include <michael_lf.h>
#else
// Forward-Stubs fuer if constexpr (false)-Zweig
extern "C" {
    inline void* michael_lf_alloc(std::size_t /*size*/, std::size_t /*alignment*/) noexcept { return nullptr; }
    inline void  michael_lf_free(void* /*p*/) noexcept {}
    inline void* michael_lf_calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void* michael_lf_realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
}
#endif
