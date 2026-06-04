#pragma once
// V41.F.6.1.R6 — IObservableTier: ABI-stabiles Observer-Zugriffs-Sub-Interface (Pfad B über die
// Modul-Binary-Grenze).
//
// Doku 24 §8.6/§8.7 (HYBRID-Mess-Modell, User-Direktive 2026-05-30): Der composite Tier wird als
// separates, dynamisch ladbares C++23-Modul (.so/.dll) gebaut; die CacheEngineBuilder kommuniziert mit
// ihm AUSSCHLIESSLICH über das ABI-stabile Interface der GATTUNG (SearchAlgorithm). Der Builder
//   (1) testet die Gattungs-API durch (tier_insert/lookup/erase),
//   (2) misst die IM Tier eingebauten Observer (tier_observe → Snapshot),
//   (3) zieht den Snapshot als flachen POD durch die Schnittstelle, und
//   (4) persistiert die korrelierten (wall_clock ↔ Observer)-Ergebnisse.
//
// ABI-SICHER nach demselben Designprinzip wie IMeasurableWorkload (measurable_workload.hpp):
//   - IObservableTier hängt NICHT an IAnatomyBase (das änderte dessen vtable-Layout, bräche alte DLLs),
//     sondern ist ein eigenständiges Sub-Interface, das der genus-typisierte ABI-Adapter ZUSÄTZLICH erbt.
//   - Der Host fragt es via `dynamic_cast<IObservableTier*>(ianatomy_ptr)` ab; alte DLLs → nullptr →
//     Host degradiert sauber (kein ABI-Bruch).
//   - Der Observer-Snapshot quert die Grenze als FLACHER, komposition-UNABHÄNGIGER POD (nur uint64-Felder,
//     fixe Layout, keine STL/vtable) → memcpy-fähig zwischen Host und Modul-Binary.
//
// @doku docs/architecture/24_messmodell_korrektur_zwei_dimensionen.md §8.6/§8.7
// @related [[feedback_zwei_dimensionen_messmodell]] [[execution-engine-als-wurzel]]

#include "idriveable_tier.hpp"   // V5-I2: IObservableTier erbt den funktionalen Antrieb (immer einkompiliert)

#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

// ─────────────────────────────────────────────────────────────────────────────
// ComdareTierObserverSnapshotV1 — flacher, ABI-stabiler Observer-Snapshot
// ─────────────────────────────────────────────────────────────────────────────

/// Komposition-UNABHÄNGIGER POD-Snapshot, der die Per-Achsen-Observer eines composite Tiers über die
/// Modul-Binary-ABI-Grenze trägt. Anders als ObserverAggregate<Composition> (dessen Layout je Composition
/// variiert) hat dieser eine FIXE, versionierte Layout (V1) — Voraussetzung für memcpy über die .dll-Grenze.
/// NUR uint64-Felder → garantiert standard_layout + trivially_copyable, identisches Layout in Host + Modul.
///
/// V1 trägt die beiden bereits real getriebenen Achsen (search_algo + allocator, Doku 24 §5.4/§8.5) plus
/// Meta. Erweiterung um weitere Achsen = NEUE Snapshot-Version (V2…), das Interface bleibt ABI-stabil.
struct ComdareTierObserverSnapshotV1 {
    // ── Achse search_algo (axis_03a) — SearchAlgoStatistics, gespiegelt ──
    std::uint64_t search_lookup_count   = 0;
    std::uint64_t search_hit_count      = 0;
    std::uint64_t search_miss_count     = 0;
    std::uint64_t search_insert_count   = 0;
    std::uint64_t search_erase_count    = 0;
    std::uint64_t search_peak_occupancy = 0;
    // ── Achse allocator (axis_06) — AllocationStatistics, gespiegelt ──
    std::uint64_t alloc_bytes_allocated = 0;
    std::uint64_t alloc_bytes_in_use    = 0;
    std::uint64_t alloc_allocation_count   = 0;
    std::uint64_t alloc_deallocation_count = 0;
    std::uint64_t alloc_failure_count   = 0;
    // ── Meta ──
    std::uint64_t observable_axis_count = 0;   // ObserverAggregate::observable_count() — Diagnose
    std::uint64_t tier_fill_level       = 0;   // tier_size() zum Snapshot-Zeitpunkt

    [[nodiscard]] constexpr bool operator==(ComdareTierObserverSnapshotV1 const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<ComdareTierObserverSnapshotV1>,
              "ABI-Pflicht: Cross-Boundary-Snapshot muss standard_layout sein");
static_assert(std::is_trivially_copyable_v<ComdareTierObserverSnapshotV1>,
              "ABI-Pflicht: Cross-Boundary-Snapshot muss memcpy-fähig (trivially_copyable) sein");

/// ABI-Version des Snapshot-Formats. Major-Bump bei Layout-Änderung (neue Achse = neuer Snapshot-Typ V2).
inline constexpr std::uint32_t kTierObserverSnapshotVersion = 1;

// ─────────────────────────────────────────────────────────────────────────────
// ComdareTierObserverSnapshotV2 — erweitert V1 um die 4 OperativeCapable-Achsen (V42 L-74c)
// ─────────────────────────────────────────────────────────────────────────────

/// V2 trägt die V1-Achsen (search_algo + allocator) UND die 4 in V42 voll observable+getriebenen
/// OperativeCapable-Achsen (telemetry/memory_layout/serialization/node_type, Doc 29 §3c–§3f). Nach der
/// POD-Design-Regel (oben) = NEUER versionierter Typ statt append-only an V1 — V1 bleibt ABI-stabil für
/// bestehende Konsumenten. NUR uint64 → standard_layout + trivially_copyable (memcpy über die DLL-Grenze).
/// Die ersten 13 Felder sind layout-identisch zu V1 (bewusst, erleichtert die Migration host-seitig).
struct ComdareTierObserverSnapshotV2 {
    // ── V1-Block (layout-identisch zu ComdareTierObserverSnapshotV1) ──
    std::uint64_t search_lookup_count   = 0;
    std::uint64_t search_hit_count      = 0;
    std::uint64_t search_miss_count     = 0;
    std::uint64_t search_insert_count   = 0;
    std::uint64_t search_erase_count    = 0;
    std::uint64_t search_peak_occupancy = 0;
    std::uint64_t alloc_bytes_allocated = 0;
    std::uint64_t alloc_bytes_in_use    = 0;
    std::uint64_t alloc_allocation_count   = 0;
    std::uint64_t alloc_deallocation_count = 0;
    std::uint64_t alloc_failure_count   = 0;
    std::uint64_t observable_axis_count = 0;
    std::uint64_t tier_fill_level       = 0;
    // ── Achse telemetry (axis_11) — TelemetrySnapshot, gespiegelt ──
    std::uint64_t telemetry_total_events = 0;
    std::uint64_t telemetry_leaf_updates = 0;
    std::uint64_t telemetry_node_updates = 0;
    std::uint64_t telemetry_peak_tracked = 0;
    // ── Achse memory_layout (axis_05) — MemoryLayoutSnapshot, gespiegelt ──
    std::uint64_t layout_scan_count          = 0;
    std::uint64_t layout_records_scanned     = 0;
    std::uint64_t layout_field_bytes_read    = 0;
    std::uint64_t layout_cache_lines_touched = 0;
    std::uint64_t layout_last_checksum       = 0;
    // ── Achse serialization (axis_10) — SerializationSnapshot, gespiegelt ──
    std::uint64_t serialization_serialize_count    = 0;
    std::uint64_t serialization_records_serialized = 0;
    std::uint64_t serialization_bytes_serialized   = 0;
    std::uint64_t serialization_last_checksum      = 0;
    // ── Achse node_type (axis_04) — NodeTypeSnapshot, gespiegelt ──
    std::uint64_t node_find_count    = 0;
    std::uint64_t node_keys_stored   = 0;
    std::uint64_t node_queries_run   = 0;
    std::uint64_t node_last_checksum = 0;

    [[nodiscard]] constexpr bool operator==(ComdareTierObserverSnapshotV2 const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<ComdareTierObserverSnapshotV2>,
              "ABI-Pflicht: V2-Snapshot muss standard_layout sein");
static_assert(std::is_trivially_copyable_v<ComdareTierObserverSnapshotV2>,
              "ABI-Pflicht: V2-Snapshot muss memcpy-fähig (trivially_copyable) sein");

inline constexpr std::uint32_t kTierObserverSnapshotVersionV2 = 2;

// ─────────────────────────────────────────────────────────────────────────────
// ComdareTierObserverSnapshotV3 — GENERISCHER schema-stabiler Per-Achsen-Observer-POD
// (Hauptagent-Entscheidung 2026-06-04, analog dem seg_ns[19]-Muster aus measurable_workload.hpp)
// ─────────────────────────────────────────────────────────────────────────────

/// Anzahl der Achsen-Slots im V3-POD = die 19 SearchAlgorithm-Achsen (T0..T18, kCompositionAxisNames-
/// Reihenfolge, identisch zu seg_ns[19]).
inline constexpr std::size_t kV3AxisCount  = 19;
/// Feld-Spalten je Achse (K). 8 deckt die breiteste jetzt-befüllte statistics()-Struktur (search_algo: 6,
/// alloc: 5, cache_traversal/mapping: 6, q1/q2: 5) mit Reserve; schema-stabil gegen weitere Felder (Phase B).
inline constexpr std::size_t kV3FieldCount = 8;

/// V3 ist EIN generischer, schema-stabiler POD: statt je Achse eigene named Felder (V1/V2-Weg, der bei
/// jeder neuen Achse einen NEUEN POD-Typ erzwingt) trägt V3 eine feste `axis_stats[19][8]`-Matrix — EXAKT
/// das bereits etablierte `seg_ns[19]`-Muster (measurable_workload.hpp), nur 2-dimensional. axis_stats[T][f]
/// = das f-te statistics()-Feld der Achse T (Feld-Reihenfolge: kV3AxisSchema, single-source). Achsen ohne
/// jetzt-befüllten Observer (Phase B: T3,T7,T8,T11..T16) bleiben 0 (ehrlich, NICHT erfunden). NUR uint64 →
/// standard_layout + trivially_copyable (memcpy über die DLL-Grenze). ADDITIV: V1/V2 + IObservableTier/V2
/// bleiben unverändert; der Host fragt V3 via `dynamic_cast<IObservableTierV3*>` ab, alte Module → nullptr.
struct ComdareTierObserverSnapshotV3 {
    std::uint64_t axis_stats[kV3AxisCount][kV3FieldCount] = {};
    // ── Meta (analog V1/V2; KEINE Achsen-Daten) ──
    std::uint64_t observable_axis_count = 0;   // ObserverAggregate::observable_count() — Diagnose
    std::uint64_t tier_fill_level       = 0;   // tier_size() zum Snapshot-Zeitpunkt
    std::uint64_t filled_axis_count     = 0;   // wie viele der 19 Achsen jetzt befüllt sind (Phase A: 10)

    [[nodiscard]] constexpr bool operator==(ComdareTierObserverSnapshotV3 const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<ComdareTierObserverSnapshotV3>,
              "ABI-Pflicht: V3-Snapshot muss standard_layout sein");
static_assert(std::is_trivially_copyable_v<ComdareTierObserverSnapshotV3>,
              "ABI-Pflicht: V3-Snapshot muss memcpy-fähig (trivially_copyable) sein");

inline constexpr std::uint32_t kTierObserverSnapshotVersionV3 = 3;

// ── V3-Schema-Tabelle: (axis_idx, field_idx) → Feldname. SINGLE-SOURCE in kCompositionAxisNames-Reihenfolge.
//    Leere Strings = (noch) nicht befülltes Feld; eine Achse, deren Felder alle "" sind, ist Phase-B (= 0).
//    Diese Tabelle treibt die CSV-Spaltennamen (stat_<achse>_<feld>) → keine Namens-Drift. Die 10 jetzt
//    befüllten Achsen (Phase A): T0 search_algo, T1 cache_traversal, T2 mapping, T4 node_type, T5 memory_layout,
//    T6 allocator, T9 serialization, T10 telemetry, T17 queuing_q1, T18 queuing_q2. ───────────────────────────
struct V3AxisFieldNames {
    char const* names[kV3FieldCount] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
};

/// kV3AxisSchema[T].names[f] = Name des Feldes axis_stats[T][f] (oder nullptr = ungenutzt / Phase B).
/// Reihenfolge JE Achse = exakt die Schreib-Reihenfolge in fill_observer_v3 (abi_adapter.hpp) — diese Tabelle
/// IST der Vertrag zwischen Schreiber (DLL) und CSV-Spaltennamen (Host). Nur uint64-Felder der jeweiligen
/// statistics()-Struct (die double-Felder wie avg_occupancy/avg_collision_chain_length sind bewusst weggelassen).
inline constexpr V3AxisFieldNames kV3AxisSchema[kV3AxisCount] = {
    /*T0  search_algo*/      {{"lookup", "hit", "miss", "insert", "erase", "peak", nullptr, nullptr}},
    /*T1  cache_traversal*/  {{"resolve", "resolve_hit", "resolve_miss", "register", "unregister", "peak_tracked", nullptr, nullptr}},
    /*T2  mapping*/          {{"register", "resolve", "resolve_hit", "resolve_miss", "reverse_lookup", "peak_mapped", nullptr, nullptr}},
    /*T3  path_compression*/ {{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},  // Phase B
    /*T4  node_type*/        {{"find", "keys_stored", "queries", "checksum", nullptr, nullptr, nullptr, nullptr}},
    /*T5  memory_layout*/    {{"scan", "records", "field_bytes", "cache_lines", "checksum", nullptr, nullptr, nullptr}},
    /*T6  allocator*/        {{"bytes_alloc", "bytes_in_use", "alloc_cnt", "dealloc_cnt", "fail", nullptr, nullptr, nullptr}},
    /*T7  prefetch*/         {{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},  // Phase B
    /*T8  concurrency*/      {{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},  // Phase B
    /*T9  serialization*/    {{"serialize", "records", "bytes", "checksum", nullptr, nullptr, nullptr, nullptr}},
    /*T10 telemetry*/        {{"events", "leaf_updates", "node_updates", "peak_tracked", nullptr, nullptr, nullptr, nullptr}},
    /*T11 value_handle*/     {{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},  // Phase B
    /*T12 isa*/              {{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},  // Phase B
    /*T13 index_org*/        {{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},  // Phase B
    /*T14 io_dispatch*/      {{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},  // Phase B
    /*T15 migration_policy*/ {{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},  // Phase B
    /*T16 filter*/           {{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},  // Phase B
    /*T17 queuing_q1*/       {{"put", "get", "overflow", "underflow", "peak_size", nullptr, nullptr, nullptr}},
    /*T18 queuing_q2*/       {{"decisions", "full_flush", "partial_flush", "no_flush", "flush_complete", nullptr, nullptr, nullptr}},
};

/// Phase A: genau diese 10 Achsen-Indizes sind im V3-POD befüllt (alle anderen = 0 = Phase B). Diagnose-/
/// Test-Konstante (test_gate4_r5b_operative-Nachfolger, observable_axis_count-Erwartung). Single-source.
inline constexpr std::size_t kV3FilledAxisCount = 10;

// ─────────────────────────────────────────────────────────────────────────────
// IObservableTier — ABI-stabiles Observer-Zugriffs-Sub-Interface
// ─────────────────────────────────────────────────────────────────────────────

/// IObservableTier — optionales Sub-Interface einer geladenen Tier-Anatomie (Pfad B, Doku 24 §8.6).
///
/// Der host-seitige CacheEngineBuilder treibt damit die Gattungs-API des Tier-Moduls (tier_insert/lookup/
/// erase über uint64 — der gemeinsame Key-Raum nach Umstufung-B) und zieht dessen eingebaute Observer als
/// flachen POD (tier_observe). Beide Trigger-Modi aus §8.7 (Zeitschritt-Sync / Zustands-Manipulation)
/// realisiert der Host, indem er nach Operationen bzw. Intervallen tier_observe() aufruft und den Snapshot
/// mit einem Wall-Clock-Zeitstempel korreliert + persistiert.
/// V5-I2: IObservableTier erbt den funktionalen Antrieb aus IDriveableTier (idriveable_tier.hpp, IMMER einkompiliert)
/// und ergänzt NUR die Beobachtung (tier_observe). Der ABI-Adapter vererbt IObservableTier NUR bei Messung-AN
/// (COMDARE_MEASUREMENT_ON) — die Antriebs-Ops bleiben über IDriveableTier auch in der Release-/funktional-only-DLL.
class IObservableTier : public IDriveableTier {
public:
    ~IObservableTier() override = default;

    // ── Observer-Zugriff: die IM Tier eingebauten Observer als flachen POD durch die ABI-Grenze ziehen ──

    /// Schreibt den aktuellen Observer-Snapshot (observe_all → flacher POD) nach *out. out != nullptr.
    /// noexcept (reines Auslesen von Zählern). Der Host stempelt das Resultat mit Wall-Clock + persistiert.
    virtual void tier_observe(ComdareTierObserverSnapshotV1* out) const noexcept = 0;
};

/// V42 L-74c — IObservableTierV2: EIGENSTÄNDIGES Sub-Interface für den erweiterten V2-Snapshot (V1-Achsen +
/// die 4 OperativeCapable telemetry/memory_layout/serialization/node_type). ABI-ROBUST nach exakt demselben
/// Prinzip wie IObservableTier selbst (hängt NICHT an IObservableTier/IAnatomyBase → ändert deren vtable
/// NICHT): der Host fragt es via `dynamic_cast<IObservableTierV2*>(ianatomy_ptr)`; alte Module → nullptr →
/// sauberer Degrade auf tier_observe-V1. Das vermeidet den vtable-Append-Crash über die DLL-Grenze (SEH
/// 0xc0000005, Befund 2026-06-03: eine neue virtuelle Methode AM vtable-Ende von IObservableTier ist nur
/// bei synchronem DLL+Host-Rebuild sicher; ein eigenständiges Interface ist es IMMER, weil der dynamic_cast
/// fehlschlägt statt über eine fremde vtable zu springen).
class IObservableTierV2 {
public:
    virtual ~IObservableTierV2() = default;

    /// Schreibt den erweiterten V2-Observer-Snapshot nach *out. out != nullptr. noexcept (reines Auslesen).
    virtual void tier_observe_v2(ComdareTierObserverSnapshotV2* out) const noexcept = 0;
};

/// Per-Achsen-Vervollständigung Phase A (2026-06-04) — IObservableTierV3: EIGENSTÄNDIGES Sub-Interface für den
/// GENERISCHEN, schema-stabilen V3-Snapshot (`axis_stats[19][8]`, alle 19 SearchAlgorithm-Achsen in EINEM POD).
/// ABI-ROBUST nach exakt demselben Prinzip wie IObservableTier/V2 (hängt NICHT an V1/V2/IAnatomyBase → ändert
/// deren vtable NICHT): der Host fragt es via `dynamic_cast<IObservableTierV3*>(ianatomy_ptr)`; alte Module →
/// nullptr → sauberer Degrade auf V2/V1. Damit ist die Per-Achsen-Erweiterung um die 9 Phase-B-Achsen ein
/// reines Befüllen weiterer axis_stats-Zeilen — OHNE neuen POD-Typ und OHNE ABI-Bruch (Hauptagent-Entscheid).
class IObservableTierV3 {
public:
    virtual ~IObservableTierV3() = default;

    /// Schreibt den generischen V3-Per-Achsen-Snapshot nach *out. out != nullptr. noexcept (reines Auslesen).
    virtual void tier_observe_v3(ComdareTierObserverSnapshotV3* out) const noexcept = 0;
};

}  // namespace comdare::cache_engine::anatomy
