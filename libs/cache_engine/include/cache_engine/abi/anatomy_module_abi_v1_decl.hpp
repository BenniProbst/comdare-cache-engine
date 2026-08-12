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

// S-1 (P2): die Baustein-Anbindung der beiden ABI-PODs (unten). RICHTUNG BEWUSST: decl -> stempel_basis.
// stempel_basis.hpp inkludiert weder diese decl noch anatomy_stamp_entries.hpp (nur measurement) -- die
// Gegenrichtung waere der Include-Zyklus decl -> basis -> entries -> decl.
#include <cache_engine/abi/stempel_basis.hpp>

#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// ABI-Version + Magic-Number (Compile-Time-Konstanten fuer Module-Loader-Check)
// ─────────────────────────────────────────────────────────────────────────────

/// Anatomy-Module ABI Version. Lebender Stand: Major 9 (NAHT-1, Mess-Naht am Genus-Interface), Minor 0.
/// Vorher: Major 8 (E-24 C8, Ebene-1-Gattung als ABI-Flaeche) -- ABI-HISTORIE gegen SHA 0f08fab5.
/// (Diese Kopfzeile nannte bis E-24 C8 stale "Major: 6 (#216-H2 tier_reset_statistics)" -- eine Zahl aus der
/// 6er-Aera mit der Begruendung des 4er-Bumps. Sie steht bewusst als EIN Satz da; die Begruendungen je Major
/// stehen darunter in der Historien-Kette, und der verbindliche Wert ist ausschliesslich das #define unten.)
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
/// STRUKT-R ORG-18 (2026-07-26, Owner-GO Session-Doc §5) ABI-Bruch Major 6->7, Minor->0: die 18. Organ-Haupt-
/// Achse persistence_target tritt in die binary_id-permutierende Komposition ein (memory_only golden_wired +
/// disk_writeback per option() AUS, Owner-Entscheid Q-1 = FALL B). Observer-POD waechst
/// axis_stats[17][8]+seg_ns[17] -> axis_stats[18][8]+seg_ns[18] (sizeof 1272->1344); organ_count() 17->18;
/// jede binary_id erhaelt das Segment "/persistence_target=<name>". Loader lehnt Major-6-DLLs ab (alle
/// Permutations-DLLs werden neu gebaut). Magic kodiert den Major -> von .A6. auf .A7. bewegt.
/// ACHTUNG: sizeof 1344 gab es schon bei INC-2c (18 Achsen INKLUSIVE isa) -- gleiche Groesse, anderer
/// Achsen-Satz; die Unterscheidung leistet ausschliesslich der Major.
/// E-24 C8 (2026-08-04, GATE 4; Manager-Entscheid LEDGER:3731, Owner-F1b LEDGER:1576-1581 "MAJOR,
/// UNVERHANDELBAR") ABI-Bruch Major 7->8, Minor->0: die Ebene-1-GATTUNG wird ABI-Flaeche. Der Bruch
/// transportiert das GESAMTE E-24-Container-Gattungs-Fenster als EIN Ereignis, je Stueck ein Satz:
///   (C6a-f) Die vier Genus-Wire-Formen stehen als GenusObserverAggregate<G, N> PER ACHSE (Set 13 /
///           Sequence 9 / Adapter 11 / View 5) statt als flache Hand-PODs, womit die gattungs-eigenen
///           Achsen-Themen erstmals einen benannten Slot auf dem Draht haben statt einer stillen Luecke.
///   (C6)    Die neuen Sub-Interfaces ISetTierV2 und ISetAlgebraTier treten als EIGENE Interfaces hinzu
///           (nie als vtable-Anhang eines bestehenden), weshalb die Vererbungsreihenfolge eingefroren bleibt
///           und ein kalter dynamic_cast die einzige Erweiterungs-Naht ist.
///   (C6b/d) Der Gattungs-Draht fuehrt den E1-Milli-Fixpunkt (double -> uint64 x1000, Suffix _milli)
///           einschliesslich growth_factor_milli, statt den SA-double-Drop zu wiederholen.
///   (C7-1)  Die Ebene-1-Kategorie heisst Map statt SearchAlgorithm -- die Umbenennung MUSSTE vor die
///           Sichtbarmachung, weil sie danach selbst ein ABI-Bruch waere.
///   (C7-2/3/6) Der Container-Gattungs-Kern ist im Kopf-Framework benannt (Identitaet + observe_axes + size,
///           clear als gestufter optionaler Block), element_type ist Gattungs-Typ-Vertrag, und die Gattung
///           ist ABI-sichtbar OHNE einen neuen Wire-String einzufrieren (Ableitung aus genus()).
/// Loader lehnt Major-7-DLLs ab (host_compatible_with, Schritt 5 der 7-Schritt-Validierung) -> alle
/// Permutations-DLLs werden neu gebaut. Magic kodiert den Major -> von .A7. auf .A8. bewegt.
/// GOLDEN-BILANZ (AUSGEWIESEN, nicht behauptet): die binary_id-permutierende Komposition ist UNBERUEHRT --
/// keine neue Organ-Haupt-Achse, organ_count() bleibt 18, der Observer-POD bleibt axis_stats[18][8] +
/// seg_ns[18] (sizeof 1344). golden_fullpilot_320 und der CRC-Anker kNewGolden131072Crc64 sind deshalb
/// byte-neutral, und es entsteht KEINE golden_fullpilot_320_binary_ids_abi7.txt: der Alt-golden-Freeze der
/// Majors 4/5/6 (decl:47/:53) hing jeweils an einer BEWEGTEN binary_id -- hier bewegt sich keine. Was dieser
/// Major bewegt, ist die Lade-Akzeptanz und (ueber den Minor-Reset unten) der Objekt-Store-Bucket.
/// NAHT-1 (2026-08-09, Owner-KERN "der Ansatz mit den SIDECARS IST FALSCH") ABI-Bruch Major 8->9,
/// Minor->0: DIE MESS-NAHT AM GENUS-INTERFACE. IObservableTier erhaelt den vtable-Slot
/// `tier_measure_accept(IMessVisitor&)`; der Host reicht den Mess-Visitor HINEIN, statt mit
/// tier_observe(Snapshot*) von aussen einen POD abzuschaben. Der Pull-Slot bleibt vorerst stehen --
/// als ABGELEITETER Klient der Push-Naht (abi_adapter.hpp: SnapshotSink + MessEdge), nicht als
/// zweiter Mess-Pfad -- weil ihn 78 Aufrufstellen in 55 Dateien rufen; deren Migration ist das
/// benannte Folgepaket NAHT-2. Praezedenz dieses Bump-Typs: Major 3->4 (#216-H2) ergaenzte auf
/// genau dieselbe Weise EINEN IObservableTier-vtable-Slot.
/// Loader lehnt Major-8-DLLs ab (host_compatible_with, Schritt 5 der 7-Schritt-Validierung) -> ALLE
/// Permutations-DLLs werden neu gebaut. Das ist GEWOLLT und der lauteste Kanal, den das Haus hat:
/// jede bestehende .so traegt die verworfene Sidecar-Naht.
/// GOLDEN-BILANZ (AUSGEWIESEN, nicht behauptet): die binary_id-permutierende Komposition ist
/// UNBERUEHRT -- keine neue Organ-Haupt-Achse, organ_count() bleibt 18, der Observer-POD bleibt
/// axis_stats[18][8] + seg_ns[18] (sizeof 1344, static_assert in observable_tier.hpp haelt es fest).
/// golden_fullpilot_320 und kNewGolden131072Crc64 sind deshalb byte-neutral, und es entsteht KEINE
/// golden_fullpilot_320_binary_ids_abi8.txt: der Alt-golden-Freeze der Majors 4/5/6 hing jeweils an
/// einer BEWEGTEN binary_id -- hier bewegt sich keine. Bewegt wird die Lade-Akzeptanz.
/// Magic kodiert den Major -> von .A8. auf .A9. bewegt.
#define COMDARE_ANATOMY_ABI_MAJOR 9
#define COMDARE_ANATOMY_ABI_MINOR 0

/// Magic-Number als Sanity-Check fuer dlopen/LoadLibrary-Compatibility.
/// "COMDA*A9*" als big-endian uint64_t (NAHT-1 Major 9).
#define COMDARE_ANATOMY_ABI_MAGIC 0x434F4D444141392EULL

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

/// AnatomyStampEntryV1 -- G2-1a (Lager-Gate A3, Section 58-V/Section 66): EIN geparster Stempel-Eintrag
/// "achse=algorithmus@X.Y.Z" als ABI-stabiler POD. Die Zeiger sind {ptr,len}-Sichten INS Zeilen-Literal (D2-Doktrin,
/// NICHT nullterminiert -- eine Laenge, kein '\0'); der gerenderte X.Y.Z-Teil ist als Tripel geparst (A10: X.Y =
/// Feature, Z = Debug-Revision). NUR uint32/Zeiger -> standard_layout, cross-boundary-fest.
///
/// A3 = reine PARSER-/POD-VORSTUFE: dieser Entry-POD + der consteval-Parser (anatomy_stamp_entries.hpp). Die drei
/// Array-Felder (organ/system/measurement entries + counts) haengen ERST in A4 (POD 88->136, Layout 4->5) ans
/// AnatomyVersionLines-POD-Ende -- HIER waechst das POD noch NICHT (emitter-/golden-neutral).
struct AnatomyStampEntryV1 {
    char const*   axis;      ///< Achsen-Name, {ptr,len}-Sicht ins Zeilen-Literal (NICHT nullterminiert)
    std::uint64_t axis_len;  ///< Laenge von axis
    char const*   algorithm; ///< gewaehlter Algorithmus, {ptr,len}-Sicht ins Zeilen-Literal
    std::uint64_t algo_len;  ///< Laenge von algorithm
    std::uint32_t x;         ///< gerenderte X.Y.Z: X (Feature-Major)
    std::uint32_t y;         ///< Y (Feature-Minor)
    std::uint32_t z;         ///< Z (Debug-Revision)
    std::uint32_t reserved;  ///< 0 (Ausrichtung / kuenftige Flags)
};

/// sizeof-Pin (ABI-Gate): 2x {char const*, uint64} + 4x uint32 -> 48 Byte auf x86_64, 8-aligned. Bricht der Wert,
/// ist das Entry-POD-Layout gewandert (Parser-Materialisierung + Loader-Sicht in A4 haengen daran).
static_assert(sizeof(AnatomyStampEntryV1) == 48,
              "AnatomyStampEntryV1-POD-Layout gewandert -- erwarteten sizeof (48 auf x86_64) aktualisieren.");
static_assert(alignof(AnatomyStampEntryV1) == 8, "AnatomyStampEntryV1: 8-Byte-Ausrichtung erwartet (Zeiger).");

/// AnatomyVersionLines -- POD der einkompilierten Versionierungs-Stempel eines Tier-Binary (W12-A2/A3,
/// Section 43). organ_line == kOrganAxisVersionLine, system_line == kSystemAxisVersionLine,
/// measurement_line == kMeasurementAxisVersionLine (anatomy_version_stamp.hpp). Die *_len-Felder geben die
/// Laenge OHNE Nullterminator; der Zeiger ist dennoch nullterminiert (String-Literal). Loader-Seite liest
/// read-only.
struct AnatomyVersionLines {
    std::uint32_t stamp_layout_version; ///< == kAnatomyVersionLinesLayout (POD-Layout-Wache)
    std::uint32_t reserved;             ///< 0 (Ausrichtung / kuenftige Flags)
    char const*   organ_line;           ///< kOrganAxisVersionLine (nullterminiert)
    std::uint64_t organ_len;            ///< organ_line-Laenge ohne '\0'
    char const*   system_line;          ///< kSystemAxisVersionLine (nullterminiert)
    std::uint64_t system_len;           ///< system_line-Laenge ohne '\0'
    // W12-A3 (Section 43, Section 47: Mess-Tooling == HAUPT): die einkompilierte kMeasurementAxisVersionLine traegt
    // GENAU die gewaehlte Mess-Tooling-HAUPT-Wahl {wallclock/macro/micro} (collector-Achse, Plan-D1). NUR die
    // Haupt-Achse (Section 43): Ablaufmethodik (run_methodology) und Workloads/Framework (UNTER-Achsen) sind NIE
    // Stempel-Bestandteil. APPEND-ONLY ans POD-Ende -> die Offsets von organ_/system_ bleiben stabil; nur
    // kAnatomyVersionLinesLayout bumpt (1 -> 2). Leerer Stempel (kein Tooling einkompiliert) -> Zeiger auf ""
    // (nie nullptr), measurement_len == 0.
    char const*   measurement_line; ///< kMeasurementAxisVersionLine (nullterminiert; "" wenn kein Tooling gewaehlt)
    std::uint64_t measurement_len;  ///< measurement_line-Laenge ohne '\0'
    // A13-M3 (Owner-E2 02.08.2026, "Merge Zeile kann daher nicht existieren"): die frueheren Felder
    // merge_line/merge_len (K7a, der "dritte Tier-Binary-Stempel" = Merge-Kombination) sind HIER ERSATZLOS
    // ENTFALLEN. Die Merge-DURCHFUEHRUNG bleibt (profile_facade/merge_plan.hpp, Owner-Q2) -- sie lebt im
    // Stempel nur noch ueber das 'e'-Experimentalflag und die erweiterten hierarchischen Namen, nicht mehr
    // als eigene Zeile. Es ist der ERSTE Feld-ENTFALL dieses POD (bis Layout 5 war alles append-only).
    // K7b-3 (Section 62-B / Section 64, 2026-07-22; Manager-Entscheid D2={ptr,len}, D3=feste Preimage-Ordnung):
    // der SHA-512-Fingerprint der Stempel-Zeilen als 128-hex-Zeile = kompakter Provenienz-Anker (Saat fuer den
    // #46b-std::map-Lookup). INNEN im Makro consteval aus der K7b-1-Primitive berechnet (anatomy_fingerprint.hpp) ->
    // der emittierte Quelltext bleibt byte-identisch (2/3-arg-Call unveraendert), golden-CRC UNBERUEHRT.
    char const*   sha512_line; ///< SHA-512-Fingerprint ueber die Glied-Folge (anatomy_fingerprint_glieder), 128-hex
    std::uint64_t sha512_len;  ///< sha512_line-Laenge ohne '\0' (immer 128)
    // G2-1b (Lager-Gate A4, Section 58-V/Section 66): die ARRAY-Form der drei ersten Stempel-Zeilen als je ein
    // {Zeiger,Count}-Paar von AnatomyStampEntryV1 (aus DENSELBEN Literalen INNEN im Makro consteval parst, keine zweite
    // Wahrheit). Organ- und System-Array bleiben ZWEI getrennte Felder, NIE fusioniert (Layer-Doktrin). count==0 ->
    // Zeiger auf kAnatomyStampNoEntries (nie nullptr, ""-Doktrin). KEIN binary_id-/CRC-Bruch (POD-Layout !=
    // binary_id). Ein Konsument liest diese Felder NUR bei stamp_layout_version == 6 (stamp_pod_has_entries) --
    // A13-M3 hat die Offsets um -16 verschoben, ein >=-Praedikat waere ab hier falsch (K-4, s.u.).
    AnatomyStampEntryV1 const* organ_entries;     ///< Organ-Array [g,h,i] (17 Haupt-Achsen)
    std::uint64_t              organ_entry_count; ///< Anzahl organ_entries
    // K-6 (W10-C5): hier stand "(5 Achsen)" -- falsch seit der Achsen-Neuordnung und doppelt falsch seit
    // A13-M2. IST: DREI System-Haupt-Achsen (kSystemAxisOrder: target_isa, operating_system, external_utils)
    // + EIN geklammerter Meta-Meta-Anhang am Zeilen-ENDE ([simd=...]) = 4 Eintraege. Seit W10-C4 tragen die
    // drei Haupt-Namen zusaetzlich den ZELLWERT als Namens-Anteil ("code" -> "code.<token>"); die Zahl der
    // Eintraege aendert das nicht -- der Zellwert steht IN einem Eintrag, nie als eigenes Segment.
    AnatomyStampEntryV1 const* system_entries;          ///< System-Array [d,e,f] -- NIE mit Organ fusioniert
    std::uint64_t              system_entry_count;      ///< Anzahl system_entries
    AnatomyStampEntryV1 const* measurement_entries;     ///< Mess-Array {wallclock,macro,micro}; count==0 -> Sentinel
    std::uint64_t              measurement_entry_count; ///< Anzahl measurement_entries
};

/// Layout-Version des AnatomyVersionLines-POD -- unabhaengig vom ABI-Major. Ein POD-Layout-Wechsel bumpt
/// DIESE Konstante, NICHT COMDARE_ANATOMY_ABI_MAJOR (das optionale Symbol ist nicht Loader-Pflicht).
/// W12-A3 (2026-07-20): 1 -> 2 (measurement_line/measurement_len ans POD-Ende angehaengt).
/// K7a (Section 59, 2026-07-20): 2 -> 3 (merge_line/merge_len ans POD-Ende angehaengt = dritter Tier-Stempel).
/// K7b-3 (Section 62-B, 2026-07-22): 3 -> 4 (sha512_line/sha512_len ans POD-Ende = SHA-512-Fingerprint der 4 Zeilen).
/// G2-1b (Lager-Gate A4, Section 58-V/Section 66): 4 -> 5 (drei {ptr,count}-Array-Paare ans POD-Ende = die Array-Form
/// der Organ-/System-/Mess-Stempel-Zeilen; INNEN consteval aus denselben Literalen parst -> emitter-/golden-neutral).
/// A13-M3 (Owner-E2 02.08.2026): 5 -> 6 (merge_line/merge_len ERSATZLOS ENTFERNT). Das ist der ERSTE Feld-ENTFALL
/// dieses POD -- KEIN Append. Er verschiebt die Offsets von sha512_line/organ_entries/... um -16 Byte.
inline constexpr std::uint32_t kAnatomyVersionLinesLayout = 6;

/// POD-Layout-Wache: AnatomyVersionLines ist ein POD aus 16 Feldern (2x uint32 + 4x {char const*, uint64} +
/// 3x {AnatomyStampEntryV1 const*, uint64}), 8-aligned -> 120 Byte auf x86_64 (8-Byte-Zeiger). Bricht dieser Wert, ist
/// das POD-Layout gewandert: dann kAnatomyVersionLinesLayout bumpen UND diesen erwarteten sizeof aktualisieren (die
/// COMDARE_ANATOMY_VERSION_STAMP-Materialisierung + jeder Modul-Rebau haengen daran). KLEINES ABI-Gate, KEIN
/// binary_id-/CRC-Bruch (das optionale Probe-Symbol ist nicht Loader-Pflicht, binary_id bleibt Organ-only).
static_assert(sizeof(AnatomyVersionLines) == 120,
              "AnatomyVersionLines-POD-Layout gewandert -- kAnatomyVersionLinesLayout bumpen + erwarteten "
              "sizeof (120 auf x86_64) aktualisieren.");
static_assert(alignof(AnatomyVersionLines) == 8, "AnatomyVersionLines: 8-Byte-Ausrichtung erwartet (Zeiger).");

/// Loader-Gate (A4) -- A13-M3/K-4: GLEICHHEITS-Wache statt `>= 5`.
///
/// TRAGENDE BEGRUENDUNG (nicht Stilfrage): bis Layout 5 wuchs der POD ausschliesslich am ENDE, deshalb war ein
/// `>=`-Praedikat korrekt -- ein neueres POD trug die alten Felder an denselben Offsets. Der A13-M3-Feld-ENTFALL
/// (merge_line/merge_len) bricht genau diese Voraussetzung: er verschiebt sha512_line/organ_entries/... um -16.
/// Ein `>= 5`-Leser wuerde ein v6-POD mit v5-Offsets lesen (und ein v5-POD mit v6-Offsets) -- er laege um zwei
/// Felder daneben und wuerde stille Zeiger-Muell liefern. Mit `== 6` faellt jedes fremde Layout hart durch.
[[nodiscard]] constexpr bool stamp_pod_has_entries(AnatomyVersionLines const& v) noexcept {
    return v.stamp_layout_version == 6;
}

namespace detail {
/// CT-Probe-POD mit frei waehlbarer Layout-Version. Die Zeiger sind nullptr und werden NIE dereferenziert --
/// die Wache liest ausschliesslich stamp_layout_version.
[[nodiscard]] constexpr AnatomyVersionLines stamp_pod_layout_probe(std::uint32_t layout) noexcept {
    return AnatomyVersionLines{layout, 0u, "", 0u, "", 0u, "", 0u, "", 0u, nullptr, 0u, nullptr, 0u, nullptr, 0u};
}
} // namespace detail

/// A13-M3/K-4 CT-NEGATIV-PROBE: der Alt-Wert 5 (und jede Zukunft) MUSS false liefern. Ohne diese Probe waere
/// ein versehentliches Zurueckdrehen auf `>=` gruen -- die Offset-Verschiebung ist unsichtbar, bis ein
/// Konsument Muell liest.
static_assert(stamp_pod_has_entries(detail::stamp_pod_layout_probe(6u)),
              "K-4: das EIGENE Layout 6 muss die Array-Form fuehren.");
static_assert(!stamp_pod_has_entries(detail::stamp_pod_layout_probe(5u)),
              "K-4: Layout 5 traegt merge_line/merge_len und hat damit ANDERE Offsets -- es darf NIE als "
              "Array-Form-faehig gelten (das war der Fehler, den Append-only bisher ausschloss).");
static_assert(!stamp_pod_has_entries(detail::stamp_pod_layout_probe(7u)),
              "K-4: ein kuenftiges Layout 7 ist ebenfalls unbekannt -- Gleichheit, nicht Ordnung.");

// -- S-1 (P2): Baustein-Anbindung der beiden ABI-PODs -- NUR Concept/Trait, KEIN Basisklassen-Einbau:
//    beide sind positional-init-PODs (stamp_pod_layout_probe oben initialisiert positionell), und die
//    sizeof-/align-/Layout-Pins (:191-193, :254, :261-264) bleiben byte-unberuehrt.
template <>
struct ist_stempel_baustein<AnatomyStampEntryV1> : StempelBausteinTag<StempelBausteinRolle::AbiPod> {};
static_assert(StempelBaustein<AnatomyStampEntryV1>);
template <>
struct ist_stempel_baustein<AnatomyVersionLines> : StempelBausteinTag<StempelBausteinRolle::AbiPod> {};
static_assert(StempelBaustein<AnatomyVersionLines>);

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

/// HISTORIEN-FREEZE Major 7 (E-24 C8, ADDITIV nach dem _abi4/_abi5/_abi6-Muster der Alt-golden-Dateien,
/// decl:47/:53). Der Freeze der frueheren Majors lief ueber Alt-golden-DATEIEN, weil dort jeweils die
/// binary_id wanderte; hier wandert sie NICHT (Golden-Bilanz oben), also friert dieser Major seine Werte
/// als BENANNTE KONSTANTEN neben den lebenden ein statt als Datei. WOFUER: die G5-Ablehnungs-Probe (ein
/// Major-7-Referenzmodul MUSS vom Loader per Major-Mismatch verworfen werden) und die Bucket-Diff-Beweise
/// zitieren damit den Vorgaenger-Wert benannt, statt ein nacktes Zahlen-/Magic-Literal zu wiederholen --
/// genau der Drift-Weg, den der best_binary_selector-Spiegel schon einmal gegangen ist (K-5).
/// Diese drei Konstanten sind reine BEWEISMITTEL: kein Produktions-Pfad liest sie, sie bewegen nichts.
inline constexpr AnatomyAbiVersion kHostAnatomyAbiVersionAbi7{7, 0};
inline constexpr std::uint64_t     kAnatomyAbiMagicAbi7 = 0x434F4D444141372EULL; // "COMDA.A7." (STRUKT-R ORG-18)

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
///
/// A13-M4 VOLLZUG (03.08.2026) -- Minor 0 -> 1. Der Schritt transportiert das GESAMTE A13-Stempel-Fenster
/// (M1/M1b/M2/M3) als EINE rueckwaerts-kompatible Vertrags-Erweiterung. Minor und NICHT Major, weil keines der
/// VIER Loader-Pflicht-Symbole und keine vtable sich bewegt hat: der einzige POD, der wanderte
/// (AnatomyVersionLines), haengt am OPTIONALEN Probe-Symbol comdare_anatomy_version_lines und traegt seine
/// eigene Layout-Zahl (kAnatomyVersionLinesLayout, s. dort) -- ein Alt-Modul laedt weiterhin. Je Punkt EIN Satz:
///   (M1)  Die Versions-Grammatik fuehrt das Pruefling-Merkmal als eigenes Glied: ein Trailing-'e' hinter dem
///         Hardware-Flag markiert einen experimentellen Algorithmus-Stand und reist als Flag-Bit im
///         AnatomyStampEntryV1 mit, statt als Namens-Konvention im Text zu stehen.
///   (M1b) Die Kurzform ist zurueckgebaut: jede Version steht dreistellig. SEIT DER FLAG-GRAMMATIK v2
///         (07.08.2026) OHNE 'v'-Praefix und mit einem PUNKT vor jedem Flag ("1.0.0.c"). Frueher: mit
///         Hardware-Flag und optionalem 'e' da, und "vN"/"vNe"/"vNc" parsen ab jetzt auf den Sentinel.
///   (M2)  Die Meta-Meta-Achsen haengen GEKLAMMERT ans ENDE ihrer Realm-Zeile (System- und Mess-Zeile), statt
///         als weiteres Haupt-Achsen-Glied mitzulaufen -- damit wandern die gerenderten Stempel-Zeilen.
///   (M3a) Der AnatomyVersionLines-POD steht auf Layout 6: merge_line/merge_len sind ERSATZLOS entfallen
///         (sizeof 136 -> 120) und das Leser-Gate ist auf die Gleichheits-Wache stamp_pod_has_entries == 6 gezogen.
///   (M3b) anatomy_fingerprint_hex nimmt den Overlay-Hash als benannten Typ OverlayHash entgegen, weshalb jeder
///         Alt-Aufruf der vierstelligen string_view-Form compile-hart bricht statt still ein Feld zu verrutschen.
///   (M3c) Das Fingerprint-Preimage ist INJEKTIV: '\n'-Domain-Separator zwischen allen Gliedern, die Kennung
///         "fingerprint_format=2" als Erstglied und das Sub-Achsen-Werteset als eigenes Glied.
///   (M3d) Alle ce-EIGENEN Versions-Literale sind auf die CPU-Flag-Form "1.0.0.c" migriert und
///         COMDARE_VERSION_HW_FLAG_ENFORCE steht auf 1, womit eine flaglose ce-eigene Version compile-hart bricht.
///   (M3e) Die SOTA-Modul-Emission traegt die VOLLEN Organ-/System-/Mess-Stempel-Zeilen statt der Kurzform, und
///         der rohe .algos-Sentinel steht dreistellig als "0.0.0" statt als Kurzform "v0".
/// WIRKUNG DES BUMPS: "+ceb=7.0" -> "+ceb=7.1" in jeder build_version -> jede perm.dll.version mismatcht in
/// dll_is_current -> ALLE Tier-Binaries werden neu gebaut, und cache_key_prefix zeigt auf einen NEUEN
/// Objekt-Store-Bucket. binary_id und perm.algos (Organ-Provenienz) bleiben UNBERUEHRT -- golden_fullpilot_320
/// und der CRC-Anker kNewGolden131072Crc64 sind deshalb byte-neutral.
///
/// W10-M2 VOLLZUG (04.08.2026) -- Minor 1 -> 2. Manager-Entscheid W10-M2 des W10-Bauplan-Dossiers
/// (Review-Befund B2, CONFIRMED am Mechanismus), gefaellt nach der M4-Praezedenz. WAS DIE BUMP-KLASSE HIER
/// TRIFFT: W10-C4 schaltet die Zellwert-Naht an perm_compile scharf und injiziert damit
/// -DCOMDARE_SYSTEM_CELL_VALUES in JEDEN Tier-Bau -- eine CEB-UNIVERSELLE Codegen-Quelle, die ALLE
/// Tier-Binaries betrifft, OHNE POD-/vtable-ABI zu brechen (Bump-Klasse s.o., woertlich). Die System-Zeile
/// im Tier-POD traegt ab jetzt die Bau-ZELLE (ISA + OS-Familie + simd) als Namens-Erweiterung des
/// Algorithmus-Markers ("code" -> "code.<token>"), womit sich kS/kFP/kSE und jeder Lager-Key eines
/// definierten Baus verschieben.
/// DAS PROBLEM, DAS DER BUMP LOEST: ohne einen Sidecar-sichtbaren Marker bliebe die lokale zweite
/// Verteidigungslinie dll_is_current (Dreifach-String-Gleichheit .version/.algos/.variant) W10-BLIND --
/// keines der drei Sidecars bewegt sich durch C4. Stale prae-W10-Binaries (runner-persistente gn_out)
/// wuerden still geskippt und truegen kS/kFP OHNE Zellwerte; "beendet Skip-nur-gleiche-OS-Familie" gaelte
/// fuer den Lokal-Bestand dann NICHT. Ein einmaliger gn_out-Purge ist VERWORFEN, weil nicht mechanisch
/// (runner-persistente Verzeichnisse + vier lokale Klone: ein Ereignis kann verpasst werden, ein Marker nicht).
/// PFAD-DIFFERENZIERTE WIRKUNG: am EINZEL-Pfad traegt die build_version das '+ceb='-Glied bereits --
/// dort wirkt der Bump direkt ('+ceb=7.1' -> '+ceb=7.2'). Am PERM-Pfad existierte das Glied bis W10 GAR
/// NICHT (beide Perm-Schleifen fuellten SystemVersionSuffixParts ohne .ceb) -- der Bump waere genau am
/// Scharfschalt-Pfad unsichtbar geblieben. C4 verdrahtet es deshalb im SELBEN Commit in beide
/// Perm-Schleifen aus DIESER Konstante und stellt cache_key_prefix von 'anhaengen' auf
/// 'konsumieren/dedupe' um (sonst Doppel-+ceb im Objekt-Store-Key). KEIN neues Suffix-SEGMENT: die
/// Glied-Klasse ist Bestand, am Perm-Pfad wird sie NEU VERDRAHTET.
/// WIRKUNG DES BUMPS: "+ceb=7.1" -> "+ceb=7.2" in jeder build_version -> jede perm.dll.version mismatcht in
/// dll_is_current -> ALLE Tier-Binaries werden neu gebaut statt still geskippt, und cache_key_prefix zeigt
/// auf einen NEUEN Objekt-Store-Bucket (deklarierte EINMALIGE Bucket-Invalidierung, M4-Praezedenz).
/// binary_id und perm.algos (Organ-Provenienz) bleiben UNBERUEHRT -- golden_fullpilot_320 und der CRC-Anker
/// kNewGolden131072Crc64 sind deshalb byte-neutral.
///
/// E-24 C8 VOLLZUG (04.08.2026) -- Minor 2 -> 0, RESET. Bauplan-Entscheid Paragraf 5.1 des
/// E-24-Fenster-Bauplans (docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md), gefaellt nach der
/// M4-/W10-M2-Praezedenz und hier vollzogen.
/// WARUM RESET UND NICHT FORTSCHREIBUNG: der codegen-Minor zaehlt Vertrags-Erweiterungen INNERHALB eines
/// Majors. Die 1 (A13-M4) und die 2 (W10-M2) sind beide ausschliesslich unter Major 7 begruendet -- ihre
/// Vollzugs-Absaetze oben nennen keinen anderen Grund. Ein "8.1" ohne je existierende 8.0-Basis waere
/// deshalb eine Versions-Luege. Einen automatischen Reset gibt es NICHT (diese Konstante ist handgepflegt,
/// nur der Major wandert von selbst mit COMDARE_ANATOMY_ABI_MAJOR), also ist der Reset ein bewusster Edit
/// im SELBEN Commit wie der Major -- und der literale Pin unten bricht bei jeder Drehung, genau dafuer
/// wurde er gebaut.
/// WIRKUNG: "+ceb=7.2" -> "+ceb=8.0" in jeder build_version -> jede perm.dll.version mismatcht in
/// dll_is_current -> ALLE Tier-Binaries werden neu gebaut, und cache_key_prefix zeigt auf einen NEUEN
/// Objekt-Store-Bucket = DEKLARIERTE EINMALIGE BUCKET-INVALIDIERUNG vor Voll-Bau-4 (M4-/W10-M2-Praezedenz).
/// Der Shift ist an BEIDEN Pfaden sichtbar: der Einzel-Pfad trug das Glied schon immer, der Perm-Pfad seit
/// der W10-C4-Verdrahtung -- der Bump ist also nicht am Scharfschalt-Pfad blind. AKZEPTIERT: vor Voll-Bau-4
/// existiert kein schuetzenswerter Voll-Bestand; betroffen sind nur die TP1-Proben, deren Neu-Inventur
/// ohnehin am Fenster-Ende steht (GATE 5).
/// binary_id und perm.algos (Organ-Provenienz) bleiben UNBERUEHRT -- golden_fullpilot_320 und der CRC-Anker
/// kNewGolden131072Crc64 sind deshalb byte-neutral.
/// LITERALER PIN: tests/unit/test_v41_anatomy_module_abi.cpp (R5D_CebContract). Die drei Konsumenten-Tests
/// (test_g1_binary_version_stamp, test_s1_cache_key_prefix, test_s5_artifact_cache_bounded) leiten den Wert
/// bewusst aus DIESER Konstante ab (Anti-Drift gegen den automatischen Major) und wuerden einen vergessenen
/// oder falschen Minor-Bump deshalb NICHT sehen; der eine literale Pin dort ist die Gegenprobe. BEFUND E-24
/// C8 (grep-Doktrin, gegen den Wortlaut "der EINE literale Pin"): tests/unit/test_w10_c4_zellwert_naht.cpp
/// fuehrte einen ZWEITEN literalen Wert-Pin (drei ".2"-Stellen). Er ist mit diesem Commit auf die Konstante
/// umgestellt -- die TU bezeugt die C4-VERDRAHTUNG (Glied vorhanden, Ordnung, Dedupe), nicht den Zahlenwert.
///
/// B14-NB4 VOLLZUG (06.08.2026) -- Minor 0 -> 1 unter Major 8. GRUND, in der Sprache dieses Absatzes:
/// kV3AxisSchema[5][5] traegt ab jetzt `line_bytes` (vorher reservierter nullptr-Slot, T2-Praezedenz
/// [2][6] `indirect_steps`). Das ist eine VERTRAGS-ERWEITERUNG INNERHALB des Majors -- genau die Klasse,
/// die diese Konstante zaehlt: der POD waechst NICHT (sizeof bleibt 1344, axis_stats[18][8] unveraendert,
/// static_assert in observable_tier.hpp), die vtable bewegt sich nicht, der Loader haette also KEINEN
/// Grund, eine aeltere Major-8-DLL abzulehnen.
/// DAS PROBLEM, DAS DER BUMP LOEST (identisch zur W10-M2-Begruendung oben, nur ein Feld weiter): eine
/// Major-8-DLL, die VOR diesem Commit gebaut wurde, schreibt in [5][5] nichts -- der Slot bleibt 0. Der
/// Host liest ihn als "Einheit unbekannt" und meldet die CLU fail-closed als n/a. Das ist die RICHTIGE
/// Reaktion des Verbrauchers, aber das FALSCHE Ergebnis fuer die Messung: eine stale Binary wuerde von
/// dll_is_current still geskippt (der `.fingerprint` bewegt sich durch diesen Commit NICHT -- binary_id,
/// perm.algos und algo_version sind unberuehrt) und truege eine dauerhaft leere CLU-Spalte durch den
/// ganzen Lauf. Die Heilung der CLU-Kette waere damit gebaut, aber am Messobjekt wirkungslos.
/// WIRKUNG: "+ceb=8.0" -> "+ceb=8.1" in jeder build_version (Einzel- UND Perm-Pfad, letzterer seit der
/// W10-C4-Verdrahtung) -> der Fingerprint mismatcht -> ALLE Tier-Binaries werden neu gebaut, und
/// cache_key_prefix zeigt auf einen NEUEN Objekt-Store-Bucket = deklarierte EINMALIGE
/// Bucket-Invalidierung (M4-/W10-M2-/E-24-C8-Praezedenz).
/// KEINE VERSIONS-LUEGE: anders als beim verworfenen "8.1 ohne 8.0" des E-24-C8-Resets existiert die
/// 8.0-Basis hier wirklich (sie ist der Stand, gegen den gebumpt wird).
/// EHRLICH AUSGEWIESENE KOSTEN: die Invalidierung trifft JEDEN vorhandenen Tier-Bestand unter 8.0. Sie ist
/// vertretbar, solange der Voll-Bau noch aussteht (Lage-Stand 06.08.: Messbeginn nicht erfolgt). Existiert
/// zum Landezeitpunkt ein schuetzenswerter Voll-Bestand, ist DIESE Zeile der Ort, an dem neu zu entscheiden
/// ist -- die Alternative waere, den Bestand bewusst mit leerer CLU-Spalte weiterzufuehren.
/// binary_id und perm.algos (Organ-Provenienz) bleiben UNBERUEHRT -- golden_fullpilot_320 und der
/// CRC-Anker kNewGolden131072Crc64 sind byte-neutral.
///
/// DIE FUNDSTELLEN-LISTE (B14-NB4) -- und warum sie diesen Absatz ersetzt hat. Der Text oben sprach
/// zweimal von "dem EINEN literalen Pin"; beide Male war das falsch. E-24 C8 fand einen ZWEITEN
/// (test_w10_c4_zellwert_naht) und stellte ihn auf die Konstante um -- der Wortlaut blieb aber stehen.
/// Beim Bump 8.0 -> 8.1 sind daraufhin VIER weitere literale Stellen uebersehen worden; gefangen hat
/// sie nicht dieser Header, sondern der ctest-Doppellauf. Statt die Zusage ein drittes Mal zu
/// wiederholen, steht hier ab jetzt, WO die Zahl wirklich literal steht:
///   tests/unit/test_v41_anatomy_module_abi.cpp      -- static_assert + 2x EXPECT_EQ (der Kontroll-Pin)
///   tests/unit/test_e24_c10_g5_lade_wache.cpp       -- ceb_contract_version_text() + --version-Block
///   tests/unit/test_e24_c10_g6_identitaets_bilanz.cpp -- +ceb=-Wert, Minor-Anteil, Objekt-Store-Key
/// WER DIESE KONSTANTE DREHT, DREHT DIESE DREI DATEIEN MIT. Die drei Konsumenten-Tests
/// (test_g1_binary_version_stamp, test_s1_cache_key_prefix, test_s5_artifact_cache_bounded) leiten den
/// Wert weiterhin bewusst AB und wandern von selbst mit -- sie sind hier bewusst NICHT gelistet.
inline constexpr std::uint32_t kCebContractCodegenMinor = 1;

/// HISTORIEN-FREEZE des Vorgaenger-Minors (E-24 C8, additiv -- s. kHostAnatomyAbiVersionAbi7). Der Wert 2
/// ist der W10-M2-Stand unter Major 7; er wird von den Bucket-Diff-Beweisen als "Vorgaenger-Bucket" zitiert.
inline constexpr std::uint32_t kCebContractCodegenMinorAbi7 = 2;

/// ceb_contract_version als Tupel (Major = ABI-Major, Minor = codegen-Minor). host_compatible_with-Backstop des
/// Loaders (host_compatible_with, decl:305-308) lehnt Major-Mismatch-DLLs ohnehin ab -> +ceb= macht Bau-Skip +
/// Lade-Akzeptanz konsistent.
inline constexpr AnatomyAbiVersion kCebContractVersion{COMDARE_ANATOMY_ABI_MAJOR, kCebContractCodegenMinor};

} // namespace comdare::cache_engine::abi
