#pragma once
// V41.F.6.1.R7.1.a.2 axis_12 General-Hardware Zentrale Topic-Registry
//
// @topic hardware
// @achse 12
//
// Goldstandard-Pattern (analog axis_06_allocator_registry.hpp):
//
// **Zweck:** Zentrale Stelle fuer Plattform-Klassen-Liste + Compile-Time-Filter
// via Boost.MP11. Konsolidiert alle Plattform-Wrapper in einer mp_list und
// filtert sie ueber `static constexpr bool enabled` zur Compile-Time
// (`mp_filter`). CacheEngineBuilder konsumiert `EnabledPlatforms` und generiert
// Permutationen NUR fuer aktive Plattformen.
//
// Compiler-Verhalten: nicht-referenzierte Wrapper-Klassen werden via
// Dead-Code-Elimination aus dem Binary entfernt (`-ffunction-sections
// -fdata-sections -Wl,--gc-sections` auf GCC/Clang, `/OPT:REF /OPT:ICF` auf MSVC).

// Flags-Header ist CMake-generiert via configure_file (siehe CMakeLists.txt)
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_flags.hpp>

// Plattform-Wrapper-Includes
#include "axis_12_general_hardware_generic.hpp"
#include "axis_12_general_hardware_x86_64.hpp"
#include "axis_12_general_hardware_aarch64.hpp"

#include <boost/mp11.hpp>

#include <type_traits>

namespace comdare::cache_engine::hardware::axis_12_general_hardware {

namespace mp = boost::mp11;

// ─────────────────────────────────────────────────────────────────────────────
// (1) AllPlatforms — Komplette statische Liste aller bekannten Plattform-Wrapper
// ─────────────────────────────────────────────────────────────────────────────
//
// Eine zentrale Stelle. Jede neue Plattform: 1 Include + 1 Eintrag in
// AllPlatforms. AllPlatforms ist die Single-Source-of-Truth fuer die Achse.

using AllPlatforms = mp::mp_list<
    GenericHardwareProfile,
    X86_64HardwareProfile,
    Aarch64HardwareProfile
>;

// ─────────────────────────────────────────────────────────────────────────────
// (2) is_enabled — Compile-Time-Predicate ueber Plattform-Klasse
// ─────────────────────────────────────────────────────────────────────────────
//
// Jeder Wrapper hat `static constexpr bool enabled` als Compile-Time-Konstante,
// die ueber den CMake-generierten Flags-Header gesetzt wird. Plattform wird
// nur dann in EnabledPlatforms aufgenommen wenn T::enabled true ist.

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

// ─────────────────────────────────────────────────────────────────────────────
// (3) EnabledPlatforms — Compile-Time-Filter ueber AllPlatforms mit is_enabled
// ─────────────────────────────────────────────────────────────────────────────

using EnabledPlatforms = mp::mp_filter<is_enabled, AllPlatforms>;

// Compile-Time-Sanity: mindestens 1 Plattform muss aktiviert sein.
static_assert(mp::mp_size<EnabledPlatforms>::value > 0,
    "Axis 12 GeneralHardware: at least one platform must be enabled "
    "(alle COMDARE_AXIS_12_ENABLE_* OFF?)");

}  // namespace comdare::cache_engine::hardware::axis_12_general_hardware
