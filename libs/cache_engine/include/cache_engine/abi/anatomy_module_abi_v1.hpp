#pragma once
// V41.F.6.1.R5.D — Anatomy Module ABI v1 (Minimal Factory-Pattern)
//
// User-Direktive 2026-05-27 (Doku 14 §41.5):
// "Per Doku 14 §41.5 reicht: comdare_create_anatomy() → IAnatomyBase*
//  + comdare_destroy_anatomy(IAnatomyBase*)."
//
// Diese ABI ist BEWUSST minimaler als die existing module_abi_v1.hpp
// (REV 7.6 PermutationModule). Sie nutzt das `IAnatomyBase`-Virtual-Interface
// fuer alle Pflicht-API (composition_name/paper_id/genus/organ_count +
// engine_name/lifecycle_state/warm_up/run/reset/shutdown) — der ABI-Bridge
// reduziert sich auf 2 Factory-Functions pro generierter .so/.dll.
//
// Vorteile gegenueber module_abi_v1.hpp:
// 1. Eine .so/.dll exportiert nur 2 Symbole (Factory + Destroy), nicht 9+
// 2. Anatomy-API-Erweiterungen brechen die ABI NICHT (Virtual-Interface
//    erweiterbar ueber neue Sub-Interfaces statt neue Funktion-Pointer)
// 3. C++ RAII statt manueller Pointer-Tracking
// 4. Type-Safety: kein void*-Cast erforderlich
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §41.5 + §43
// @task #706 V41.F.6.1.R5.D
// @related [[execution-engine-als-wurzel]] [[anatomie-gattungen]]

#include "../../../anatomy/anatomy_base.hpp"

#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// ABI-Version + Magic-Number (Compile-Time-Konstanten fuer Module-Loader-Check)
// ─────────────────────────────────────────────────────────────────────────────

/// Anatomy-Module ABI Version. Major: 1. Minor: 0.
/// Wird in jeder generierten .so/.dll als constexpr exportiert.
/// Module-Loader verifiziert Version-Match bevor Factory aufgerufen wird.
#define COMDARE_ANATOMY_ABI_MAJOR 1
#define COMDARE_ANATOMY_ABI_MINOR 0

/// Magic-Number als Sanity-Check fuer dlopen/LoadLibrary-Compatibility.
/// "COMDA·A1·" als big-endian uint64_t.
#define COMDARE_ANATOMY_ABI_MAGIC 0x434F4D444141312EULL

// ─────────────────────────────────────────────────────────────────────────────
// Export/Import Macros (Cross-Plattform)
// ─────────────────────────────────────────────────────────────────────────────

#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(COMDARE_ANATOMY_MODULE_BUILD)
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
/// Module-Loader prueft: (version >> 32) == COMDARE_ANATOMY_ABI_MAJOR.
/// Low-32 = Minor. Mismatch fuehrt zu Loader-Fehler vor Factory-Aufruf.
COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_version() noexcept;

/// comdare_anatomy_abi_magic() — liefert Magic-Number. Compatibility-Sanity-Check.
COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_magic() noexcept;

/// comdare_create_anatomy() — Factory: erzeugt eine Instanz der in dieser .so/.dll
/// hinterlegten Composition-Anatomie. Ownership: Caller (Module-Loader).
///
/// Implementierung in generiertem Permutations-Binary:
/// ```cpp
/// COMDARE_ANATOMY_ABI_EXPORT
/// comdare::cache_engine::anatomy::IAnatomyBase*
/// comdare_create_anatomy() noexcept {
///     using A = comdare::cache_engine::anatomy::SearchAlgorithmAnatomy<MyComposition>;
///     return new comdare::cache_engine::anatomy::SearchAlgorithmAbiAdapter<A>{};
/// }
/// ```
COMDARE_ANATOMY_ABI_EXPORT
::comdare::cache_engine::anatomy::IAnatomyBase* comdare_create_anatomy() noexcept;

/// comdare_destroy_anatomy(ptr) — Gegenstueck zu comdare_create_anatomy().
/// Muss innerhalb der gleichen .so/.dll aufgerufen werden (gleicher Allocator).
COMDARE_ANATOMY_ABI_EXPORT
void comdare_destroy_anatomy(
    ::comdare::cache_engine::anatomy::IAnatomyBase* ptr) noexcept;

}  // extern "C"

// ─────────────────────────────────────────────────────────────────────────────
// COMDARE_DEFINE_ANATOMY_MODULE — Convenience-Macro fuer Permutations-Binary
// ─────────────────────────────────────────────────────────────────────────────

/// COMDARE_DEFINE_ANATOMY_MODULE(CompositionType) generiert die 4 Pflicht-extern-C-
/// Symbole eines Anatomy-Permutations-Binary. CompositionType muss IsComposition
/// erfuellen UND fuer SearchAlgorithm-Gattung zugelassen sein.
///
/// Verwendung:
/// ```cpp
/// // generated_perm_<hash>.cpp
/// #include <cache_engine/abi/anatomy_module_abi_v1.hpp>
/// #include "art_reference.hpp"  // konkrete Composition
///
/// COMDARE_DEFINE_ANATOMY_MODULE(comdare::cache_engine::compositions::ArtComposition)
/// ```
///
/// Macro expandiert zu den 4 extern "C" Function-Bodies. Compiler emittiert die
/// .so/.dll Export-Tabelle.
#define COMDARE_DEFINE_ANATOMY_MODULE(CompositionType)                              \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t                             \
    comdare_anatomy_abi_version() noexcept {                                        \
        return (static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR) << 32) |      \
               static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MINOR);               \
    }                                                                                \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t                             \
    comdare_anatomy_abi_magic() noexcept {                                          \
        return COMDARE_ANATOMY_ABI_MAGIC;                                            \
    }                                                                                \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT                                           \
    ::comdare::cache_engine::anatomy::IAnatomyBase*                                 \
    comdare_create_anatomy() noexcept {                                              \
        using AnatomyType =                                                          \
            ::comdare::cache_engine::anatomy::SearchAlgorithmAnatomy<CompositionType>; \
        return new (::std::nothrow)                                                  \
            ::comdare::cache_engine::anatomy::SearchAlgorithmAbiAdapter<AnatomyType>{}; \
    }                                                                                \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT void                                      \
    comdare_destroy_anatomy(                                                         \
        ::comdare::cache_engine::anatomy::IAnatomyBase* ptr) noexcept {             \
        delete ptr;                                                                  \
    }

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
        return (static_cast<std::uint64_t>(major) << 32) |
               static_cast<std::uint64_t>(minor);
    }

    [[nodiscard]] constexpr bool host_compatible_with(AnatomyAbiVersion module) const noexcept {
        // Major muss identisch sein. Minor des Moduls darf <= Host sein
        // (Module darf alt sein, aber nicht aus der Zukunft).
        return major == module.major && module.minor <= minor;
    }
};

/// Compile-Time Host-Version (zur Build-Zeit der cache-engine eingebrannt).
inline constexpr AnatomyAbiVersion kHostAnatomyAbiVersion{
    COMDARE_ANATOMY_ABI_MAJOR,
    COMDARE_ANATOMY_ABI_MINOR
};

}  // namespace comdare::cache_engine::abi
