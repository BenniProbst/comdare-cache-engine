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
#include "../../../anatomy/measurable_workload.hpp" // IMeasurableWorkload (Loader-dynamic_cast)
#include "../../../anatomy/observable_tier.hpp"     // IObservableTier (Loader-dynamic_cast, R6 Pfad B)
#include "../../../anatomy/rollbackable_tier.hpp"   // IRollbackableTier (Loader-dynamic_cast, V5-I6 memento_all)

#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// ABI-Version + Magic-Number (Compile-Time-Konstanten fuer Module-Loader-Check)
// ─────────────────────────────────────────────────────────────────────────────

/// Anatomy-Module ABI Version. Major: 6 (#216-H2 tier_reset_statistics). Minor: 0.
/// V5-I2.2 ABI-Bruch (Major 1→2): IObservableTier→IDriveableTier-Split + konditionale Adapter-Vererbung
/// (observer_all nur bei MESSUNG-AN compile-time einkompiliert).
/// I1 Observer-Konsolidierung (Major 2→3, Minor→0, User-Direktive 2026-06-04 „EINE konsistente Observer-
/// Schnittstelle", Historie docs/architecture/31_observer_interface_konsolidierung_i1.md): die früheren
/// getrennten Observer-Sub-Interfaces + die früheren mehrfach versionierten Observer-PODs ENTFALLEN; es gibt
/// GENAU EINE `IObservableTier::tier_observe(ComdareTierObserverSnapshot*)` + EINEN versionierten POD
/// (axis_stats[17][8] + seg_ns[17]/Pfad B + Meta; INC-2d, war [19] bei Konsolidierung).
/// Echter ABI-Bruch (vtable + POD-Layout) → Loader (`AnatomyAbiVersion::host_compatible_with`) lehnt alle
/// alt-gebauten Major-2-DLLs per Major-Mismatch ab → ALLE Permutations-DLLs neu zu bauen. Minor auf 0 zurück-
/// gesetzt (die V5-I6/#49-E-Minor-Stufen sind im Major-Bump aufgegangen; IRollbackableTier/IScannableTier
/// bleiben additive Sub-Interfaces der MESSUNG-AN-Variante). Magic kodiert den Major → von .A2. auf .A3. bewegt.
/// #216-H2 ABI-Bruch (Major 3→4, Minor→0): IObservableTier erhält den daten-erhaltenden vtable-Slot
/// `tier_reset_statistics()`; der Observer-POD bleibt unverändert. Loader lehnt Major-3-DLLs ab, damit Host und
/// Modul dieselbe IObservableTier-vtable sehen. Magic kodiert den Major → von .A3. auf .A4. bewegt.
/// Bau-INC-2b (2026-07-17, TABU-GO) ABI-Bruch Major 4→5, Minor→0: der EINE koordinierte Bündel-Bump am
/// Experiment-Planer-Dock (F12iii Telemetrie-Herauslösung aus der binary_id + F1b Set-Ebene-1-Gattung +
/// F2 native Set-ABI + #37 Scheduling-CT-Ersatz + H-7/Q5-Metadaten-Version; Bauplan
/// docs/sessions/backups/20260717-inc2-planung/). Loader lehnt Major-4-DLLs ab (alle Permutations-DLLs
/// werden neu gebaut); Alt-golden als golden_fullpilot_320_binary_ids_abi4.txt additiv eingefroren
/// (W3=A: autoritative Neu-Materialisierung = Bau-INC-3). Magic kodiert den Major → von .A4. auf .A5. bewegt.
/// Bau-INC-2d (2026-07-18, TABU-GO) ABI-Bruch Major 5→6, Minor→0: isa-Herauslösung aus der binary_id-
/// permutierenden Komposition (Target-ISA-System-Achse, build-config-gewählter Codegen-Codepfad; exakt
/// telemetry-/INC-2c-treu). Observer-POD schrumpft axis_stats[18][8]+seg_ns[18] → axis_stats[17][8]+seg_ns[17]
/// (sizeof 1344→1272). Loader lehnt Major-5-DLLs ab (alle Permutations-DLLs werden neu gebaut); Alt-golden als
/// golden_fullpilot_320_binary_ids_abi5.txt additiv eingefroren. Magic kodiert den Major → von .A5. auf .A6. bewegt.
#define COMDARE_ANATOMY_ABI_MAJOR 6
#define COMDARE_ANATOMY_ABI_MINOR 0

/// Magic-Number als Sanity-Check fuer dlopen/LoadLibrary-Compatibility. "COMDA·A6·" als big-endian uint64_t (Bau-INC-2d Major 6).
#define COMDARE_ANATOMY_ABI_MAGIC 0x434F4D444141362EULL

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

} // extern "C"

// -----------------------------------------------------------------------------
// Optionale Versionierungs-Stempel-Probe (W12-A2 / Section 43) -- KEIN Major-Bump
// -----------------------------------------------------------------------------
// Ein OPTIONALES 5. extern-"C"-Symbol, das die einkompilierten Organ-/System-Stempel-Zeilen
// (kOrganAxisVersionLine / kSystemAxisVersionLine) als POD exponiert. Der Loader verlangt weiter
// NUR die 4 Pflicht-Symbole (comdare_create_anatomy et al.) -> KEIN ABI-Major-Bump; ein Modul ohne
// COMDARE_ANATOMY_VERSION_STAMP exportiert das Symbol schlicht nicht (dlsym/GetProcAddress findet es
// nicht). Der POD traegt nur String-Literal-Zeiger (im Modul static constexpr), kein std::string.

namespace comdare::cache_engine::abi {

/// AnatomyVersionLines -- POD der einkompilierten Versionierungs-Stempel eines Tier-Binary (W12-A2,
/// Section 43). organ_line == kOrganAxisVersionLine, system_line == kSystemAxisVersionLine
/// (anatomy_version_stamp.hpp). Die *_len-Felder geben die Laenge OHNE Nullterminator; der Zeiger ist
/// dennoch nullterminiert (String-Literal). Loader-Seite liest read-only.
struct AnatomyVersionLines {
    std::uint32_t stamp_layout_version; ///< == kAnatomyVersionLinesLayout (POD-Layout-Wache)
    std::uint32_t reserved;             ///< 0 (Ausrichtung / kuenftige Flags)
    char const*   organ_line;           ///< kOrganAxisVersionLine (nullterminiert)
    std::uint64_t organ_len;            ///< organ_line-Laenge ohne '\0'
    char const*   system_line;          ///< kSystemAxisVersionLine (nullterminiert)
    std::uint64_t system_len;           ///< system_line-Laenge ohne '\0'
};

/// Layout-Version des AnatomyVersionLines-POD -- unabhaengig vom ABI-Major. Ein POD-Layout-Wechsel bumpt
/// DIESE Konstante, NICHT COMDARE_ANATOMY_ABI_MAJOR (das optionale Symbol ist nicht Loader-Pflicht).
inline constexpr std::uint32_t kAnatomyVersionLinesLayout = 1;

} // namespace comdare::cache_engine::abi

extern "C" {

/// comdare_anatomy_version_lines() -- OPTIONALES Probe-Symbol: liefert die einkompilierten Stempel-Zeilen
/// eines Tier-Binary. NICHT Teil der 4 Loader-Pflicht-Symbole; ein ohne COMDARE_ANATOMY_VERSION_STAMP
/// gebautes Modul exportiert es gar nicht (dlsym liefert dann nullptr).
COMDARE_ANATOMY_ABI_EXPORT
::comdare::cache_engine::abi::AnatomyVersionLines const* comdare_anatomy_version_lines() noexcept;

} // extern "C"

// ─────────────────────────────────────────────────────────────────────────────
// AnatomyAbiVersion Helper-Klasse (host-seitig im Module-Loader)
// ─────────────────────────────────────────────────────────────────────────────

namespace comdare::cache_engine::abi {

/// AnatomyAbiVersion — entpackt ABI-Version aus geladener .so/.dll.
struct AnatomyAbiVersion {
    std::uint32_t major;
    std::uint32_t minor;

    [[nodiscard]] static constexpr AnatomyAbiVersion unpack(std::uint64_t raw) noexcept {
        return AnatomyAbiVersion{static_cast<std::uint32_t>(raw >> 32),
                                 static_cast<std::uint32_t>(raw & 0xFFFFFFFFULL)};
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
inline constexpr AnatomyAbiVersion kHostAnatomyAbiVersion{COMDARE_ANATOMY_ABI_MAJOR, COMDARE_ANATOMY_ABI_MINOR};

// ─────────────────────────────────────────────────────────────────────────────
// CEB-Contract-Version (inkrementeller Tier-Binary-Cache, Bauplan §4)
// ─────────────────────────────────────────────────────────────────────────────
/// Codegen-Minor der CEB-Contract-Version. Der Major IST der ABI-Major (jeder echte ABI-Bruch — POD-Schema/
/// vtable/Emitter-Aritaet — bumpt ihn AUTOMATISCH ueber COMDARE_ANATOMY_ABI_MAJOR). Der Minor wird HIER von Hand
/// gebumpt, wenn sich eine CEB-UNIVERSELLE Codegen-Quelle aendert, die ALLE Tier-Binaries betrifft, OHNE das
/// POD-/vtable-ABI zu brechen (z.B. all_axes_umbrella / adhoc_emitter / Observer-Basis-Emission). Beide zusammen
/// bilden ceb_contract_version = <ABI-Major>.<codegen-Minor>. System-/Framework-Provenienz: sie wird via
/// +ceb=<major>.<minor> in die build_version (system_axes_version_suffix) eingefaltet — NIE in perm.algos (Organ)
/// und NIE in die binary_id. Jeder Bump laesst jede perm.dll.version mismatchen -> ALLE Binaries neu ("CEB-
/// Aenderung betrifft alle"). CI-Tripwire-gated (Bauplan §5): ein universeller Codegen-Diff ohne Minor-Bump = rot.
inline constexpr std::uint32_t kCebContractCodegenMinor = 0;

/// ceb_contract_version als Tupel (Major = ABI-Major, Minor = codegen-Minor). host_compatible_with-Backstop des
/// Loaders (host_compatible_with, decl:124-127) lehnt Major-Mismatch-DLLs ohnehin ab -> +ceb= macht Bau-Skip +
/// Lade-Akzeptanz konsistent.
inline constexpr AnatomyAbiVersion kCebContractVersion{COMDARE_ANATOMY_ABI_MAJOR, kCebContractCodegenMinor};

} // namespace comdare::cache_engine::abi
