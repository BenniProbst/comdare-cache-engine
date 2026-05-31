#pragma once
// V41.F.6.1.C Stufe 1 Zentrale Topic-Registry fuer Allocator-Achse 6 (W6-Pattern)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.C Stufe 1
//
// **Zweck:** Zentrale Stelle fuer Vendor-Klassen-Liste + Compile-Time-Filter via
// Boost.MP11. Konsolidiert alle Vendor-Wrapper-Klassen in einer mp_list und
// filtert sie ueber `static constexpr bool enabled` zur Compile-Time
// (`mp_filter`). CacheEngineBuilder konsumiert `EnabledVendors` und generiert
// Permutationen NUR fuer aktive Vendor.
//
// Compiler-Verhalten: nicht-referenzierte Wrapper-Klassen werden via
// Dead-Code-Elimination aus dem Binary entfernt (`-ffunction-sections -fdata-sections
// -Wl,--gc-sections` auf GCC/Clang, `/OPT:REF /OPT:ICF` auf MSVC).
//
// **HINWEIS:** Stufe 1 hat noch die Wrapper im alten Stand (#ifdef-basiert).
// Die `enabled` Konstante in jedem Wrapper wird in Stufe 2 ergaenzt — bis dahin
// ist `is_enabled<T>` ein Dummy-Predicate, das alle includierten Vendor true zurueck gibt.

// V41.F.6.1.C Stufe 1: Flags-Header ist CMake-generiert in ${BUILD}/generated/...
#include <axes/alloc/axis_06_allocator_flags.hpp>

// Vendor-Wrapper-Includes (alle bekannten Allocator-Wrapper)
// Batch 1 (Pilot, 2026-05-25)
#include "axis_06_allocator_std_malloc.hpp"
#include "axis_06_allocator_mimalloc.hpp"
#include "axis_06_allocator_snmalloc.hpp"
#include "axis_06_allocator_pmr_resource.hpp"
// Batch 2 (2026-05-26)
#include "axis_06_allocator_jemalloc.hpp"
#include "axis_06_allocator_tcmalloc.hpp"
#include "axis_06_allocator_dlmalloc.hpp"
// Batch 3 (2026-05-26)
#include "axis_06_allocator_hoard.hpp"
#include "axis_06_allocator_slab.hpp"
#include "axis_06_allocator_michael_lf.hpp"
// Batch 4 (2026-05-26)
#include "axis_06_allocator_scalloc.hpp"
#include "axis_06_allocator_numalloc.hpp"
#include "axis_06_allocator_rpmalloc.hpp"
// Batch 5 (2026-05-26)
#include "axis_06_allocator_lrmalloc.hpp"
#include "axis_06_allocator_cama.hpp"
#include "axis_06_allocator_starmalloc.hpp"
// Batch 6 (2026-05-26)
#include "axis_06_allocator_tcmalloc_wh.hpp"
#include "axis_06_allocator_hmalloc.hpp"
#include "axis_06_allocator_pim_malloc.hpp"
// Batch 7 (2026-05-26)
#include "axis_06_allocator_crystalline.hpp"
#include "axis_06_allocator_exgen.hpp"
#include "axis_06_allocator_buddy.hpp"
// Batch 8 (2026-05-26) — VOLLAUSBAU-Abschluss
#include "axis_06_allocator_ptmalloc2.hpp"
#include "axis_06_allocator_vmem_mag.hpp"
// V41.F.6.1 R5.B (2026-05-29) — eigener std::pmr-Pool, behavioral-distinkt ohne externes Linking
#include "axis_06_allocator_pool_resource.hpp"

#include <boost/mp11.hpp>

#include <type_traits>

namespace comdare::cache_engine::alloc {

namespace mp = boost::mp11;

// ───────────────────────────────────────────────────────────────────────────
// (1) AllVendors — Komplette statische Liste aller bekannten Vendor-Wrapper
// ───────────────────────────────────────────────────────────────────────────
//
// Eine zentrale Stelle. Jeder neue Vendor: 1 Include + 1 Eintrag in AllVendors.
// AllVendors ist die Single-Source-of-Truth fuer die Achse.

using AllVendors = mp::mp_list<
    // Batch 1 (Pilot, 2026-05-25)
    StdMalloc,
    MimallocAllocator,
    SnmallocAllocator,
    PmrResourceAllocator,
    // Batch 2 (2026-05-26)
    JemallocAllocator,
    TCMallocAllocator,
    DlmallocAllocator,
    // Batch 3 (2026-05-26)
    HoardAllocator,
    SlabAllocator,
    MichaelLockFreeAllocator,
    // Batch 4 (2026-05-26)
    ScallocAllocator,
    NUMAllocAllocator,
    RPMallocAllocator,
    // Batch 5 (2026-05-26)
    LRMallocAllocator,
    CAMAAllocator,
    StarMallocAllocator,
    // Batch 6 (2026-05-26)
    TCMallocWarehouseAllocator,
    HMallocAllocator,
    PIMMallocAllocator,
    // Batch 7 (2026-05-26)
    CrystallineAllocator,
    ExgenAllocator,
    BuddyAllocator,
    // Batch 8 (2026-05-26) — VOLLAUSBAU-Abschluss A21+A23
    PtMalloc2Allocator,
    VmemMagazinesAllocator,
    // V41.F.6.1 R5.B (2026-05-29) — eigener std::pmr::unsynchronized_pool_resource (F15-operativ)
    PoolResourceAllocator
    // Allocator-Achse 6 KOMPLETT (25 Vendor: Batch 1-8 + R5.B Pool)
>;

// ───────────────────────────────────────────────────────────────────────────
// (2) is_enabled - Compile-Time-Predicate ueber Vendor-Klasse (Stufe 2 LIVE)
// ───────────────────────────────────────────────────────────────────────────
//
// V41.F.6.1.C Stufe 2 (HEUTE): jeder Wrapper hat `static constexpr bool enabled`
// als Compile-Time-Konstante, die ueber den zentralen Flags-Header gesetzt wird.
// Vendor wird nur dann in EnabledVendors aufgenommen wenn T::enabled true ist.

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

// ───────────────────────────────────────────────────────────────────────────
// (3) EnabledVendors - Compile-Time-Filter ueber AllVendors mit is_enabled
// ───────────────────────────────────────────────────────────────────────────

using EnabledVendors = mp::mp_filter<is_enabled, AllVendors>;

// Compile-Time-Sanity: mindestens 1 Vendor muss aktiviert sein.
static_assert(mp::mp_size<EnabledVendors>::value > 0,
    "Axis 06 Allocator: at least one vendor must be enabled (alle COMDARE_AXIS_06_ENABLE_* OFF?)");

}  // namespace comdare::cache_engine::alloc
