#pragma once
// V41.F.6.1 Batch 8 Vendor-Header-Shim: Vmem-Magazines (W6-Pattern)
//
// @vendor A23 Vmem-Magazines (Bonwick USENIX 2001 "Magazines and Vmem")
//
// Vmem als Kernel-Resource-Allocator + Magazine-Cache pro CPU fuer Per-CPU
// Object-Caching. Bonwick's Erweiterung des Slab-Allokators (A02) — beide
// stammen vom selben Autor, aber A23 hat den vollen Vmem-Layer + Magazines.

#include <axes/alloc/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_VMEM_MAG
#  include <vmem_magazine.h>
#else
extern "C" {
    inline void* vmem_alloc(std::size_t /*size*/, std::size_t /*alignment*/) noexcept { return nullptr; }
    inline void  vmem_free(void* /*p*/, std::size_t /*size*/) noexcept {}
    inline void* vmem_calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
    inline void* vmem_realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
}
#endif
