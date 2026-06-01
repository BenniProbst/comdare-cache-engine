#pragma once
// V41.F.6.1.R6 Inkrement 2b — Anatomy Module ABI v1 (LEICHTES Decl-Header, Loader-Seite).
//
// **Entkopplung (Doku 24 §8.6):** Der host-seitige `AnatomyModuleLoader` (ein reiner dlopen/LoadLibrary-
// Wrapper) braucht NUR die ABI-Schnittstelle: die extern-"C"-Factory-Deklarationen, die ABI-Version/Magic
// und die Interface-Typen (`IAnatomyBase` + die Sub-Interfaces `IMeasurableWorkload`/`IObservableTier` für
// `dynamic_cast`). Er braucht NICHT die schwere Adapter-Template (`abi_adapter.hpp`) noch die
// `COMDARE_DEFINE_ANATOMY_MODULE`-Makros — die gehören zur MODUL-AUTOR-Seite (die generierten Permutations-
// .cpp/.dll). Vorher zog `anatomy_module_abi_v1.hpp` `abi_adapter.hpp` mit, was den Loader an die GANZE
// Achsen-Library + generierte-Flags-Maschinerie koppelte (C1083 beim ComposedStore-Ausbau). Dieses
// Decl-Header trennt die Loader-Seite sauber ab; das volle `anatomy_module_abi_v1.hpp` inkludiert es +
// ergänzt die Makro-/Adapter-Seite (unverändert für DLLs/Tests).
//
// @doku docs/architecture/24_messmodell_korrektur_zwei_dimensionen.md §8.6
// @related [[execution-engine-als-wurzel]] [[anatomie-gattungen]]

#include "../../../anatomy/anatomy_base.hpp"        // IAnatomyBase (Rückgabetyp der Factory)
#include "../../../anatomy/measurable_workload.hpp"  // IMeasurableWorkload (Loader-dynamic_cast)
#include "../../../anatomy/observable_tier.hpp"      // IObservableTier (Loader-dynamic_cast, R6 Pfad B)
#include "../../../anatomy/rollbackable_tier.hpp"    // IRollbackableTier (Loader-dynamic_cast, V5-I6 memento_all)

#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// ABI-Version + Magic-Number (Compile-Time-Konstanten fuer Module-Loader-Check)
// ─────────────────────────────────────────────────────────────────────────────

/// Anatomy-Module ABI Version. Major: 2 (V5-I2.2). Minor: 1 (V5-I6).
/// V5-I2.2 ABI-Bruch (Major 1→2): IObservableTier→IDriveableTier-Split + konditionale Adapter-Vererbung
/// (observer_all nur bei MESSUNG-AN compile-time einkompiliert). Alt-gebaute DLLs (major 1) werden vom Loader
/// (`AnatomyAbiVersion::host_compatible_with`) sauber abgelehnt → alle Permutations-DLLs neu zu bauen.
/// V5-I6 (Minor 0→1): der IMMER-präsente IDriveableTier-Kontrakt (= Major) bleibt UNVERÄNDERT; die MESSUNG-AN-
/// Variante gewinnt ZUSÄTZLICH IRollbackableTier (memento_all, tier_save_all/tier_rollback_all). Minor-Bump:
/// ein neuerer Host akzeptiert ältere Module (minor 0, ohne Rollback) weiterhin — der Loader-`dynamic_cast` auf
/// IRollbackableTier liefert dort null → der Zwei-Phasen-Treiber (I7) fällt graziös auf Kalt-Messung zurück.
/// V5-#49-E (Minor 1→2): die MESSUNG-AN-Variante gewinnt ZUSÄTZLICH IScannableTier (tier_scan, YCSB-E-Range-Scan).
/// Wieder rein additiv: Major (IDriveableTier) unverändert; ältere Module (minor ≤ 1, ohne Scan) bleiben kompatibel
/// (host_compatible_with: minor 1 ≤ 2), der `dynamic_cast<IScannableTier*>` liefert dort null → YCSB-E-Profile
/// fallen für solche Module ehrlich aus. (Magic kodiert nur Major → durch den Minor-Bump UNVERÄNDERT.)
#define COMDARE_ANATOMY_ABI_MAJOR 2
#define COMDARE_ANATOMY_ABI_MINOR 2

/// Magic-Number als Sanity-Check fuer dlopen/LoadLibrary-Compatibility. "COMDA·A2·" als big-endian uint64_t (V5-I2.2).
#define COMDARE_ANATOMY_ABI_MAGIC 0x434F4D444141322EULL

// ─────────────────────────────────────────────────────────────────────────────
// Export/Import Macros (Cross-Plattform)
// ─────────────────────────────────────────────────────────────────────────────

// Drei Build-Modi:
//   - COMDARE_ANATOMY_ABI_STATIC   : STATIC-Library oder In-Process Build (kein dll*)
//   - COMDARE_ANATOMY_MODULE_BUILD : SHARED-Lib Author-Side (dllexport)
//   - (default Consumer-Side)      : SHARED-Lib Consumer-Side (dllimport)
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(COMDARE_ANATOMY_ABI_STATIC)
        #define COMDARE_ANATOMY_ABI_EXPORT
    #elif defined(COMDARE_ANATOMY_MODULE_BUILD)
        #define COMDARE_ANATOMY_ABI_EXPORT __declspec(dllexport)
    #else
        #define COMDARE_ANATOMY_ABI_EXPORT __declspec(dllimport)
    #endif
#else
    #define COMDARE_ANATOMY_ABI_EXPORT __attribute__((visibility("default")))
#endif

// ─────────────────────────────────────────────────────────────────────────────
// extern "C" Factory + Destroy + Version-Probe (Pflicht-API jeder .so/.dll)
// ─────────────────────────────────────────────────────────────────────────────

extern "C" {

/// comdare_anatomy_abi_version() — liefert ABI-Version der geladenen .so/.dll.
COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_version() noexcept;

/// comdare_anatomy_abi_magic() — liefert Magic-Number. Compatibility-Sanity-Check.
COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_magic() noexcept;

/// comdare_create_anatomy() — Factory: erzeugt eine Instanz der in dieser .so/.dll hinterlegten
/// Composition-Anatomie. Ownership: Caller (Module-Loader).
COMDARE_ANATOMY_ABI_EXPORT
::comdare::cache_engine::anatomy::IAnatomyBase* comdare_create_anatomy() noexcept;

/// comdare_destroy_anatomy(ptr) — Gegenstueck zu comdare_create_anatomy(). Muss innerhalb der gleichen
/// .so/.dll aufgerufen werden (gleicher Allocator).
COMDARE_ANATOMY_ABI_EXPORT
void comdare_destroy_anatomy(::comdare::cache_engine::anatomy::IAnatomyBase* ptr) noexcept;

}  // extern "C"

// ─────────────────────────────────────────────────────────────────────────────
// AnatomyAbiVersion Helper-Klasse (host-seitig im Module-Loader)
// ─────────────────────────────────────────────────────────────────────────────

namespace comdare::cache_engine::abi {

/// AnatomyAbiVersion — entpackt ABI-Version aus geladener .so/.dll.
struct AnatomyAbiVersion {
    std::uint32_t major;
    std::uint32_t minor;

    [[nodiscard]] static constexpr AnatomyAbiVersion unpack(std::uint64_t raw) noexcept {
        return AnatomyAbiVersion{
            static_cast<std::uint32_t>(raw >> 32),
            static_cast<std::uint32_t>(raw & 0xFFFFFFFFULL)
        };
    }

    [[nodiscard]] constexpr std::uint64_t pack() const noexcept {
        return (static_cast<std::uint64_t>(major) << 32) | static_cast<std::uint64_t>(minor);
    }

    [[nodiscard]] constexpr bool host_compatible_with(AnatomyAbiVersion module) const noexcept {
        // Major muss identisch sein. Minor des Moduls darf <= Host sein (Module darf alt sein, nicht aus der Zukunft).
        return major == module.major && module.minor <= minor;
    }
};

/// Compile-Time Host-Version (zur Build-Zeit der cache-engine eingebrannt).
inline constexpr AnatomyAbiVersion kHostAnatomyAbiVersion{
    COMDARE_ANATOMY_ABI_MAJOR,
    COMDARE_ANATOMY_ABI_MINOR
};

}  // namespace comdare::cache_engine::abi
