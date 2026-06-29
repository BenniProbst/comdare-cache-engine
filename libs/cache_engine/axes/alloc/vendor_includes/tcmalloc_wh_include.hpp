#pragma once
// V41.F.6.1 Batch 6 Vendor-Header-Shim: TCMalloc-Warehouse (W6-Pattern)
//
// @vendor A14 TCMalloc-Warehouse (Google Cloud, Hyperscale-Erweiterung von TCMalloc)
// Erweitert TCMalloc um Warehouse-style storage fuer Cloud-Workloads.

#include <axes/alloc/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_TCMALLOC_WH
#include <tcmalloc_warehouse/tcmalloc_warehouse.h>
#else
extern "C" {
inline void* tc_wh_alloc(std::size_t /*size*/, std::size_t /*alignment*/) noexcept { return nullptr; }
inline void  tc_wh_free(void* /*p*/) noexcept {}
inline void* tc_wh_calloc(std::size_t /*n*/, std::size_t /*size*/) noexcept { return nullptr; }
inline void* tc_wh_realloc(void* /*p*/, std::size_t /*new_size*/) noexcept { return nullptr; }
}
#endif
