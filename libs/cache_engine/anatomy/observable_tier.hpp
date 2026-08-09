#pragma once
// V41.F.6.1.R6 / I1 — IObservableTier: ABI-stabiles Observer-Zugriffs-Sub-Interface (Pfad B über die
// Modul-Binary-Grenze), KONSOLIDIERT auf GENAU EINE Schnittstelle + EINEN POD.
//
// Doku 24 §8.6/§8.7 (HYBRID-Mess-Modell, User-Direktive 2026-05-30): Der composite Tier wird als separates,
// dynamisch ladbares C++23-Modul (.so/.dll) gebaut; die CacheEngineBuilder kommuniziert mit ihm AUSSCHLIESSLICH
// über das ABI-stabile Interface der GATTUNG (SearchAlgorithm). Der Builder
//   (1) testet die Gattungs-API durch (tier_insert/lookup/erase),
//   (2) misst die IM Tier eingebauten Observer (tier_observe → Snapshot),
//   (3) zieht den Snapshot als flachen POD durch die Schnittstelle, und
//   (4) persistiert die korrelierten (wall_clock ↔ Observer)-Ergebnisse.
//
// I1 KONSOLIDIERUNG (User-Direktive 2026-06-04 „EINE konsistente Observer-Schnittstelle"): Es gibt GENAU EINE
// versionierte `IObservableTier` mit GENAU EINER `tier_observe(ComdareTierObserverSnapshot*)` + GENAU EINEM
// versionierten POD (axis_stats[17][8] + seg_ns[17]/Pfad B + Meta -- ABI-HISTORIE gegen SHA 42b34354).
// LEBEND sind axis_stats[18][8] + seg_ns[18]; die Zahl lebt in kV3AxisCount (:61), die 17 ist der
// Stand VOR dem 6->7-Bump. HY-0 08.08.2026: dieselbe Klasse wie die Major-Kopie unten -- eine Zahl,
// die anderswo lebt, als Kopie im Kommentar. Die frueheren parallelen Observer-Sub-
// Interfaces + die früheren mehrfach versionierten Observer-PODs sind ENTFERNT; die Versionierung läuft
// jetzt ueber ABI-Major (anatomy_module_abi_v1_decl.hpp) -- der Loader lehnt inkompatible Alt-DLLs per
// Major-Mismatch ab (KEINE per-Version-Sub-Interface-Vermehrung mit dynamic_cast-Degrade mehr). Historie:
// docs/architecture/31_observer_interface_konsolidierung_i1.md.
//
// ABI-ETIKETT DIESER DATEI (HY-0, 2026-08-08) -- warum in der Zeile oben keine Zahl mehr steht:
// Sie trug bis heute eine eigene Major-Zahl, eingeleitet mit dem Wort "aktuell". Das ist eine
// Behauptung ueber den LEBENDEN Stand, und sie lag seit dem 26.07.2026 falsch -- ueber ZWEI Bumps
// hinweg (6->7 STRUKT-R ORG-18, ce 774a5d5f; 7->8 E-24 C8, ce 4f569051), 13 Tage lang, ausserhalb
// jeder Wache. Eine Kopie einer Zahl, die anderswo lebt, veraltet genau so lautlos.
//   Stand 2026-07-26, ABI-HISTORIE gegen SHA 42b34354: aktuell Major 6 (anatomy_module_abi_v1_decl.hpp:54)
//   LEBEND seit 2026-08-04: Major 8 -- Beleg anatomy_module_abi_v1_decl.hpp:89
// Die Zahl gehoert in den Decl-Header und nur dorthin; hier steht ab jetzt der VERWEIS statt einer
// Kopie. scripts/ci_hy_label_gate.sh (ctest: hy_label_gate) haelt beide Zeilen gegen den Header:
// Teil B der Wache sucht repo-weit nach genau dieser Klasse (Wort "aktuell" plus eigene Major-Zahl).
//
// ABI-SICHER nach demselben Designprinzip wie IMeasurableWorkload (measurable_workload.hpp):
//   - IObservableTier hängt NICHT an IAnatomyBase (das änderte dessen vtable-Layout), sondern ist ein
//     eigenständiges Sub-Interface, das der genus-typisierte ABI-Adapter ZUSÄTZLICH erbt.
//   - Der Host fragt es via `dynamic_cast<IObservableTier*>(ianatomy_ptr)` ab (1× kalt je Modul, nie im Hot-Loop;
//     messarchitektur_design_observer_handle_no_dynamic_cast.md); alte DLLs → nullptr → Host degradiert sauber.
//   - Der Observer-Snapshot quert die Grenze als FLACHER, komposition-UNABHÄNGIGER POD (nur uint64/int64-Felder,
//     fixe Layout, keine STL/vtable) → memcpy-fähig zwischen Host und Modul-Binary.
//
// @doku docs/architecture/24_messmodell_korrektur_zwei_dimensionen.md §8.6/§8.7
// @related [[feedback_zwei_dimensionen_messmodell]] [[feedback_one_consistent_observer_interface_pruefdock]]

#include "idriveable_tier.hpp"     // V5-I2: IObservableTier erbt den funktionalen Antrieb (immer einkompiliert)
#include "measurable_workload.hpp" // ComdareSegmentLatencyV2: Pfad-B-Timing im konsolidierten POD
// Die seg_ns-Groesse ist kV3AxisCount, lebend also seg_ns[18]. Die frueher an dieser Stelle
// genannte seg_ns[17] ist der Stand vor dem 6->7-Bump -- ABI-HISTORIE gegen SHA 42b34354.

#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

// ─────────────────────────────────────────────────────────────────────────────
// Achsen-Schema-Konstanten des konsolidierten Observer-POD (kV3*-Namen bewusst beibehalten = Single-Source
// Schreiber↔CSV-Spaltenname; I1-Konsolidierung lässt diese unverändert, nur die Alt-PODs V1/V2/V3 entfallen).
// ─────────────────────────────────────────────────────────────────────────────

/// Anzahl der Achsen-Slots im Observer-POD = die 18 SearchAlgorithm-Achsen (T0..T17, kCompositionAxisNames-
/// Reihenfolge, identisch zu seg_ns[18]). Bau-INC-2c (F12iii, ABI-5): telemetry hat die Komposition verlassen
/// (CEB-System-Achse, H-10-Sidecar) — vorher T10 von 19. Bau-INC-2d (ABI-6): isa hat die Komposition ebenfalls
/// verlassen (Target-ISA-System-Achse, build-config-Codepfad) — vorher T11 von 18.
/// STRUKT-R ORG-18 (ABI-7): persistence_target kommt als T17 hinzu (Session-Doc §5) -- 17 -> 18.
inline constexpr std::size_t kV3AxisCount = 18;
/// Feld-Spalten je Achse (K). 8 deckt die breiteste befüllte statistics()-Struktur (search_algo: 6,
/// alloc: 5, cache_traversal/mapping: 6, q1/q2: 5) mit Reserve; schema-stabil gegen weitere Felder (Phase B).
inline constexpr std::size_t kV3FieldCount = 8;

// ── Schema-Tabelle: (axis_idx, field_idx) → Feldname. SINGLE-SOURCE in kCompositionAxisNames-Reihenfolge.
//    Leere Strings = (noch) nicht befülltes Feld; eine Achse, deren Felder alle "" sind, ist Phase-B (= 0).
//    Diese Tabelle treibt die CSV-Spaltennamen (stat_<achse>_<feld>) -> keine Namens-Drift.
//    LEBEND: befuellt sind ALLE 18 Achsen (kV3AxisCount; nachgerechnet von kV3FilledAxisCount unten,
//    T17 persistence_target seit STRUKT-R ORG-18 eingeschlossen).
//    Nachsatz HY-0 08.08.2026: die Zeile darunter behauptete im Praesens, alle -- damals 17 -- Achsen
//    seien befuellt: dieselbe Klasse wie die Major-Kopie im Dateikopf, zehn Zeilen UNTER der Heilung,
//    und die erste Wachen-Fassung sah sie nicht (Teil D kannte nur axis_stats[N]/seg_ns[N]). Jetzt Historie:
//    Seit Phase-B-Abschluss (2026-06-04) waren ALLE 17 Achsen befuellt -- ABI-HISTORIE gegen SHA 42b34354
//    (Phase A: T0,T1,T2,T4,T5,T6,T9,T15,T16; Phase B ergaenzt T3 path_compression, T7 prefetch,
//    T8 concurrency, T10 value_handle, T11 index_org, T12 io_dispatch, T13 migration_policy, T14 filter;
//    INC-2d: isa raus). ----------------------------------------------------------------------------------
struct V3AxisFieldNames {
    char const* names[kV3FieldCount] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
};

/// kV3AxisSchema[T].names[f] = Name des Feldes axis_stats[T][f] (oder nullptr = ungenutzt / Phase B).
/// Reihenfolge JE Achse = exakt die Schreib-Reihenfolge in fill_observer_v3 (abi_adapter.hpp) — diese Tabelle
/// IST der Vertrag zwischen Schreiber (DLL) und CSV-Spaltennamen (Host). Nur uint64-Felder der jeweiligen
/// statistics()-Struct (die double-Felder wie avg_occupancy/avg_collision_chain_length sind bewusst weggelassen).
inline constexpr V3AxisFieldNames kV3AxisSchema[kV3AxisCount] = {
    /*T0  search_algo*/ {{"lookup", "hit", "miss", "insert", "erase", "peak", nullptr, nullptr}},
    /*T1  cache_traversal*/
    {{"resolve", "resolve_hit", "resolve_miss", "register", "unregister", "peak_tracked", "batch_size",
      "batch_visited"}},
    /*T2  mapping*/
    {{"register", "resolve", "resolve_hit", "resolve_miss", "reverse_lookup", "peak_mapped", "indirect_steps",
      nullptr}}, // slot[6] war reserviert (nullptr) -> layout-neutral benannt; sizeof-neutral (Ist 1344, ABI-7/STRUKT-R ORG-18, Version 8)
    /*T3  path_compression*/
    {{"compress", "prefix_len", "bytes_saved", "cuts", "checksum", nullptr, nullptr, nullptr}}, // Phase B (T3)
    /*T4  node_type*/ {{"find", "keys_stored", "queries", "checksum", nullptr, nullptr, nullptr, nullptr}},
    /*T5  memory_layout. B14-NB4: line_bytes in den RESERVIERTEN Slot [5][5] (vorher nullptr) -- die EINHEIT
      von cache_lines. Seit B14-NB3 zaehlt cache_lines in Linien DER cacheline-Unterachse; der Verbraucher
      (measurement/system_axis.hpp, CLU = field_bytes/(cache_lines*line_bytes)) sitzt HINTER der Modul-ABI
      und kann die Line-Groesse dort nicht rekonstruieren -- sie ist Compile-Zeit-Eigenschaft der DLL.
      LAYOUT-NEUTRAL wie der T2-Praezedenzfall indirect_steps in [2][6]: axis_stats ist fix [18][8],
      sizeof bleibt 1344 (ABI-7), kein ABI-Major-Bump. Additiv ist nur die CSV-Spalte
      stat_memory_layout_line_bytes -- genau der Fall, den lazy_csv_header als "Phase B fuellt leere
      Schema-Slots" vorsieht.*/
    {{"scan", "records", "field_bytes", "cache_lines", "checksum", "line_bytes", nullptr, nullptr}},
    /*T6  allocator*/
    {{"bytes_alloc", "bytes_in_use", "alloc_cnt", "dealloc_cnt", "fail", "budget_reject", nullptr, nullptr}},
    /*T7  prefetch*/
    {{"trigger", "suggestions", "hot_path_hints", "max_queue_depth", "addrs_enqueued", nullptr, nullptr,
      nullptr}}, // Phase B (T7)
                 /*T8  concurrency*/
    {{"acquire", "release", "contention", "validation_fail", "pattern_id", nullptr, nullptr, nullptr}}, // Phase B (T8)
    /*T9  serialization*/ {{"serialize", "records", "bytes", "checksum", nullptr, nullptr, nullptr, nullptr}},
    /*T10 value_handle (INC-2c: telemetry-Zeile entfernt; INC-2d: isa-Zeile entfernt, Indizes ab hier -1)*/
    {{"access", "indirect_deref", "version_strips", "peak_chain_depth", nullptr, nullptr, nullptr, nullptr}}, // Phase B
    /*T11 index_org*/
    {{"scan", "records", "predicate_evals", "indirect_lookups", "checksum", nullptr, nullptr, nullptr}}, // Phase B
    /*T12 io_dispatch*/
    {{"rounds", "bytes", "align_adjusts", "dispatch_cnt", "checksum", nullptr, nullptr, nullptr}}, // Phase B
    /*T13 migration_policy*/
    {{"decisions", "migrations", "hot_votes", "cold_votes", "tier_moves", nullptr, nullptr, nullptr}}, // Phase B
    /*T14 filter*/ {{"probe", "pos", "neg", "hash_probes", "checksum", nullptr, nullptr, nullptr}},    // Phase B
    /*T15 queuing_q1*/ {{"put", "get", "overflow", "underflow", "peak_size", nullptr, nullptr, nullptr}},
    /*T16 queuing_q2*/
    {{"decisions", "full_flush", "partial_flush", "no_flush", "flush_complete", nullptr, nullptr, nullptr}},
    /*T17 persistence_target (STRUKT-R ORG-18). Reihenfolge = Schreib-Reihenfolge in fill_observer_v3.
      EHRLICHKEIT: bytes_staged ist Rueckschreib-STAGING, nicht Platten-Durchsatz; device_flushes bleibt 0,
      solange DiskWritebackTarget::has_device_writeback_path() false meldet.*/
    {{"rounds", "bytes_staged", "records_staged", "device_flushes", "checksum", nullptr, nullptr, nullptr}},
};

/// Anzahl der im POD befüllten Achsen = Achsen mit mindestens EINEM benannten Schema-Feld (single-source aus
/// kV3AxisSchema abgeleitet, NICHT hartkodiert). So zieht diese Diagnose-/Test-Konstante automatisch mit, wenn
/// eine Phase-B-Achse (T3/T7/T8/T11..T16) ihre Schema-Zeile befüllt — keine Drift, kein manuelles Nachziehen.
[[nodiscard]] constexpr std::size_t v3_count_filled_axes() noexcept {
    std::size_t n = 0;
    for (std::size_t t = 0; t < kV3AxisCount; ++t) {
        if (kV3AxisSchema[t].names[0] != nullptr) ++n; // erste Spalte benannt ⇒ Achse trägt Observer-Felder
    }
    return n;
}
inline constexpr std::size_t kV3FilledAxisCount = v3_count_filled_axes();

// ─────────────────────────────────────────────────────────────────────────────
// ComdareTierObserverSnapshot — der EINE konsolidierte, versionierte Observer-POD (I1)
// ─────────────────────────────────────────────────────────────────────────────

/// Komposition-UNABHÄNGIGER, flacher, versionierter Observer-POD über die Modul-Binary-ABI-Grenze. EIN POD für
/// ALLES: `axis_stats[18][8]` = die Per-Achsen-Observer-Felder (Schema = kV3AxisSchema, single-source);
/// `seg_ns[18]` = das Pfad-B-Per-Achsen-Timing ueber die REALE Komposition (User-Entscheid 2026-06-04); + Meta.
/// Ersetzt die getrennten V1/V2/V3-Observer-PODs + den seg_ns-Timing-POD (Preflight wkqt7a0il: axis_stats+Meta
/// subsumiert JEDES frühere V1- (13) + V2-Feld (26) — V1 search→[0][0..5], alloc→[6][0..4]; V2 telemetry→[10],
/// layout→[5], serialization→[9], node_type→[4]; obs_axes/fill→Meta). Layout bitweise stabil (alle Member 8-B-
/// Ints, kein Padding; sizeof==1344 nach STRUKT-R ORG-18 (war 1272 nach INC-2d, 1344 mit T11-isa nach INC-2c,
/// 1416 mit T10-telemetry; ABI-7 = 18*8*8 + 18*8 + 48 Meta), alignof==8) -> memcpy ueber die ABI-Grenze. Versionierung
/// jetzt über ABI-Major.
struct ComdareTierObserverSnapshot {
    std::uint64_t axis_stats[kV3AxisCount][kV3FieldCount] = {}; // T0..T17 x 8 Felder (Schema = kV3AxisSchema)
    std::int64_t  seg_ns[kV3AxisCount]                    = {}; // Pfad-B Per-Achsen-Timing (ns, je Achse)
    std::uint64_t observable_axis_count                   = 0;  // Meta: # observable Achsen (in-process)
    std::uint64_t tier_fill_level                         = 0;  // Meta: aktueller Füllstand (tier_size)
    std::uint64_t filled_axis_count                       = 0;  // Meta: # Achsen mit Observer-Werten
    std::uint64_t batches_measured                        = 0;  // Meta: # Timing-Batches (Warmup verworfen)
    // P-MD3 (Coverage-Versöhnung, 2026-06-18): der kommensurable Nenner + benannte Rest des Pfad-B-Per-Achsen-Timings.
    // seg_run_total_ns = äußere Wall-Clock des Segment-Mess-Laufs (fill_segment_timing_v3); seg_framework_ns =
    // seg_run_total_ns − Σseg_ns[0..16] (NICHT-segmentierter Loop-/Instrumentierungs-Overhead). Damit gilt
    // Σseg_ns + seg_framework_ns ≡ seg_run_total_ns → Coverage gegen die EIGENE Wall-Clock ~100% (Rest EXPLIZIT benannt),
    // statt die irreführende sum(seg)/total_ns-Quote gegen die unkommensurable Real-Workload-Wall-Clock (war ~33,6%).
    // REIN ADDITIV (hinten angehängt) → ABI-Layout aller bestehenden Felder/Slots unberührt.
    std::int64_t seg_framework_ns = 0; // Meta: benannter Rest des Segment-Laufs
    std::int64_t seg_run_total_ns = 0; // Meta: äußere Wall-Clock des Segment-Laufs (Coverage-Nenner)

    [[nodiscard]] constexpr bool operator==(ComdareTierObserverSnapshot const&) const noexcept = default;
};
static_assert(std::is_standard_layout_v<ComdareTierObserverSnapshot>,
              "ABI-Pflicht: konsolidierter Observer-POD muss standard_layout sein (memcpy über DLL-Grenze)");
static_assert(std::is_trivially_copyable_v<ComdareTierObserverSnapshot>,
              "ABI-Pflicht: konsolidierter Observer-POD muss trivially_copyable sein");
// L3/K-2 (Register QW1, 2026-07-19): das dokumentierte Layout (Kommentar oben, sizeof==1344 seit
// STRUKT-R ORG-18/ABI-7 = 18*8*8 axis_stats + 18*8 seg_ns + 48 Meta) wird hier compile-time zementiert --
// vorher nur Laufzeit-EXPECT. Jede Layout-Aenderung ist ein ABI-Major-Bump und muss diesen Wert bewusst anfassen.
// STRUKT-R ORG-18 (ABI-7): 18*8*8 axis_stats (1152) + 18*8 seg_ns (144) + 48 Meta = 1344.
// ACHTUNG Verwechslungs-Falle: 1344 war schon einmal der Wert (INC-2c, 18 Achsen INKLUSIVE isa). Gleiche Zahl,
// ANDERER Achsen-Satz -- die Unterscheidung leistet der ABI-Major (6 -> 7), nicht die Groesse.
static_assert(sizeof(ComdareTierObserverSnapshot) == 1344,
              "ABI-Bruch: sizeof(ComdareTierObserverSnapshot) != 1344 (ABI-7/STRUKT-R ORG-18) -- "
              "Layout-Aenderung erfordert koordinierten ABI-Major-Bump (anatomy_module_abi_v1_decl.hpp)");

/// Format-Version des konsolidierten Observer-POD (die Loader-Kompatibilität läuft über ABI-Major, s.
/// anatomy_module_abi_v1_decl.hpp; diese Konstante dient der Diagnose/Tests).
inline constexpr std::uint32_t kTierObserverSnapshotVersionUnified =
    8; // STRUKT-R ORG-18 (ABI-7): persistence_target-Slot -- axis_stats[18][8] + seg_ns[18], sizeof 1344
       // (war 7: INC-2d 1272, 6: INC-2c 1344 mit isa, 5: P-MD3 1416)

// ─────────────────────────────────────────────────────────────────────────────
// IObservableTier — die EINE ABI-stabile Observer-Schnittstelle (I1)
// ─────────────────────────────────────────────────────────────────────────────

/// IObservableTier — optionales Sub-Interface einer geladenen Tier-Anatomie (Pfad B, Doku 24 §8.6).
///
/// Der host-seitige CacheEngineBuilder treibt damit die Gattungs-API des Tier-Moduls (tier_insert/lookup/
/// erase über uint64 — der gemeinsame Key-Raum nach Umstufung-B) und zieht dessen eingebaute Observer als
/// flachen POD (tier_observe). Beide Trigger-Modi aus §8.7 (Zeitschritt-Sync / Zustands-Manipulation)
/// realisiert der Host, indem er nach Operationen bzw. Intervallen tier_observe() aufruft und den Snapshot
/// mit einem Wall-Clock-Zeitstempel korreliert + persistiert.
/// V5-I2: IObservableTier erbt den funktionalen Antrieb aus IDriveableTier (idriveable_tier.hpp, IMMER einkompiliert)
/// und ergänzt NUR die Mess-/Beobachtungs-Methoden (tier_observe/tier_reset_statistics). Der ABI-Adapter vererbt
/// IObservableTier NUR bei Messung-AN (COMDARE_MEASUREMENT_ON) — die Antriebs-Ops bleiben über IDriveableTier auch in
/// der Release-/funktional-only-DLL.
class IObservableTier : public IDriveableTier {
public:
    ~IObservableTier() override = default;

    /// Die EINE Observer-Methode (I1): schreibt den konsolidierten Snapshot (axis_stats[kV3AxisCount][8] +
    /// seg_ns[kV3AxisCount] + Meta) nach *out. out != nullptr. noexcept (reines Auslesen + lesendes
    /// Pfad-B-Timing). Der Host stempelt das Resultat mit Wall-Clock + persistiert. Der ABI-Adapter
    /// implementiert die feste Sequenz Observer-READ der auto-gekoppelten Achsen -> Pfad-B-Timing -> SPAETER
    /// Observer-READ der NUR dort getriebenen Achsen (A8-S3) -> Organ-Reset (gegen Doppelzaehlung UND gegen
    /// strukturell nicht gelesene Zaehler).
    virtual void tier_observe(ComdareTierObserverSnapshot* out) const noexcept = 0;

    /// Daten-erhaltender Statistik-Reset für Messphasen-Grenzen: nullt ausschließlich die kumulativen
    /// Observer-/Achsen-Zähler, ohne Tier-Daten oder reale Hilfsstrukturen (Filter/Slots/Trie/Store) zu leeren.
    virtual void tier_reset_statistics() noexcept = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// IMigratableTier — additives Sub-Interface fuer den ECHTEN 2-Ebenen-Migrations-Schritt (P4, #123)
// ─────────────────────────────────────────────────────────────────────────────

/// IMigratableTier — optionales Sub-Interface einer geladenen Tier-Anatomie: triggert EINEN ECHTEN (nicht
/// simulierten) Migrations-Schritt der aktiven migration_policy-Strategie. Der Host (oder ein Mess-Treiber) ruft
/// tier_migrate_step → die markierten (kalten) Records wandern REAL aus der heissen 1. Ebene in die kalte 2.
/// Ebene (container_tier1_ im ABI-Adapter); der migration-Observer bucht die bewegten Records (tier_moves > 0 bei
/// aktiver Strategie). Fuer NoMigration (das gepinnte Verhalten aller 320 None-Lebewesen) bewegt der Schritt NIE
/// einen Record → tier_moves bleibt 0 (Rueckgabe 0), die Mess-Pfad-Semantik aendert sich fuer None NICHT.
///
/// ABI-SICHER nach demselben Designprinzip wie IObservableTier/IRollbackableTier: eigenstaendiges Sub-Interface,
/// das der ABI-Adapter NUR bei COMDARE_MEASUREMENT_ON zusaetzlich erbt; Host-Abfrage via dynamic_cast (1× kalt).
/// Quert die Grenze als reine vtable (uint64-Parameter/-Rueckgabe, kein STL/POD-by-value) → ABI-stabil.
class IMigratableTier {
public:
    virtual ~IMigratableTier() = default;

    /// Fuehrt EINEN echten Migrations-Schritt aus: bewegt bis zu `max_moves` markierte Records (0 = unbegrenzt) aus
    /// der heissen in die kalte Ebene. Rueckgabe = Zahl der REAL bewegten Records (NoMigration → stets 0). noexcept
    /// (Mess-Robustheit: eine interne Stoerung darf den Lauf nicht abreissen → 0).
    [[nodiscard]] virtual std::uint64_t tier_migrate_step(std::uint64_t max_moves) noexcept = 0;
};

} // namespace comdare::cache_engine::anatomy
