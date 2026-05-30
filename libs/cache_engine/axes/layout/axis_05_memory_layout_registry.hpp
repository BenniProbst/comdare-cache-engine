#pragma once
// V41.F.6.1.R7.1.b axis_05 Memory-Layout Zentrale Topic-Registry
//
// @topic memory_layout
// @achse 05
//
// Goldstandard-Pattern (analog axis_06_allocator_registry.hpp):
// Zentrale Stelle fuer Layout-Klassen-Liste + Compile-Time-Filter via
// Boost.MP11. Konsolidiert alle Layout-Wrapper in einer mp_list und filtert
// sie ueber `static constexpr bool enabled` zur Compile-Time (`mp_filter`).
// CacheEngineBuilder konsumiert `EnabledLayouts` und generiert Permutationen
// NUR fuer aktivierte Layouts.

// Flags-Header ist CMake-generiert via configure_file (siehe CMakeLists.txt)
#include <axes/layout/axis_05_memory_layout_flags.hpp>

// Layout-Wrapper-Includes
#include "axis_05_memory_layout_cache_line_aligned.hpp"
#include "axis_05_memory_layout_aos_strict.hpp"
#include "axis_05_memory_layout_soa.hpp"
#include "axis_05_memory_layout_packed_bitmap.hpp"
// V41.F.6.1 A4 (2026-05-29) — AoSoA Hybrid-Layout (Block-SoA + Block-AoS, SIMD-tiled)
#include "axis_05_memory_layout_aosoa.hpp"

#include <boost/mp11.hpp>

#include <type_traits>

namespace comdare::cache_engine::layout {

namespace mp = boost::mp11;

// ─────────────────────────────────────────────────────────────────────────────
// (1) AllLayouts — komplette statische Liste aller bekannten Layout-Wrapper
// ─────────────────────────────────────────────────────────────────────────────
using AllLayouts = mp::mp_list<
    CacheLineAlignedMemoryLayout,
    AoSStrictMemoryLayout,
    SoAMemoryLayout,
    PackedBitmapMemoryLayout,
    AoSoAMemoryLayout
>;

// ─────────────────────────────────────────────────────────────────────────────
// (2) is_enabled — Compile-Time-Predicate ueber Layout-Klasse
// ─────────────────────────────────────────────────────────────────────────────
template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

// ─────────────────────────────────────────────────────────────────────────────
// (3) EnabledLayouts — Compile-Time-Filter ueber AllLayouts mit is_enabled
// ─────────────────────────────────────────────────────────────────────────────
using EnabledLayouts = mp::mp_filter<is_enabled, AllLayouts>;

// Compile-Time-Sanity: mindestens 1 Layout muss aktiviert sein.
static_assert(mp::mp_size<EnabledLayouts>::value > 0,
    "Axis 05 MemoryLayout: at least one layout must be enabled "
    "(alle COMDARE_AXIS_05_ENABLE_* OFF?)");

}  // namespace
