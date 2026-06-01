#pragma once
// V41.F.6.1 Batch 6 Vendor-Header-Shim: PIM-Malloc (W6-Pattern)
//
// @vendor A16 PIM-Malloc Processing-In-Memory (UPMEM/HBM-PIM 2023+)
//
// SONDERFALL: Verlangt PIM-Hardware (UPMEM DPUs, Samsung HBM-PIM, ...).
// Auf nicht-PIM-Hardware: Fallback auf portable_aligned_alloc (Host-Memory).
// `requires_specialized_hardware()` Property = true im Wrapper.

#include <axes/alloc/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_PIM_MALLOC
#  include <pim_malloc.h>
#else
extern "C" {
    inline void* pim_alloc(std::size_t /*size*/, std::size_t /*alignment*/, int /*dpu_id*/) noexcept { return nullptr; }
    inline void  pim_free(void* /*p*/, int /*dpu_id*/) noexcept {}
    inline void* pim_calloc(std::size_t /*n*/, std::size_t /*size*/, int /*dpu_id*/) noexcept { return nullptr; }
    inline void* pim_realloc(void* /*p*/, std::size_t /*new_size*/, int /*dpu_id*/) noexcept { return nullptr; }
    inline int   pim_detect_hardware() noexcept { return 0; }  // 0 = no PIM-Hardware
}
#endif
