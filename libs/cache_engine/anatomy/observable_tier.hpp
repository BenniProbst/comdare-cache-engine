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
// versionierten POD (axis_stats[19][8] + seg_ns[19]/Pfad B + Meta). Die früheren parallelen Observer-Sub-
// Interfaces + die früheren mehrfach versionierten Observer-PODs sind ENTFERNT; die Versionierung läuft
// jetzt über ABI-Major (anatomy_module_abi_v1_decl.hpp, Major 2→3) — der Loader lehnt inkompatible Alt-DLLs per
// Major-Mismatch ab (KEINE per-Version-Sub-Interface-Vermehrung mit dynamic_cast-Degrade mehr). Historie:
// docs/architecture/31_observer_interface_konsolidierung_i1.md.
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

#include "idriveable_tier.hpp"   // V5-I2: IObservableTier erbt den funktionalen Antrieb (immer einkompiliert)
#include "measurable_workload.hpp"  // ComdareSegmentLatencyV2 (seg_ns[19]) für das Pfad-B-Timing im konsolidierten POD

#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

// ─────────────────────────────────────────────────────────────────────────────
// Achsen-Schema-Konstanten des konsolidierten Observer-POD (kV3*-Namen bewusst beibehalten = Single-Source
// Schreiber↔CSV-Spaltenname; I1-Konsolidierung lässt diese unverändert, nur die Alt-PODs V1/V2/V3 entfallen).
// ─────────────────────────────────────────────────────────────────────────────

/// Anzahl der Achsen-Slots im Observer-POD = die 19 SearchAlgorithm-Achsen (T0..T18, kCompositionAxisNames-
/// Reihenfolge, identisch zu seg_ns[19]).
inline constexpr std::size_t kV3AxisCount  = 19;
/// Feld-Spalten je Achse (K). 8 deckt die breiteste befüllte statistics()-Struktur (search_algo: 6,
/// alloc: 5, cache_traversal/mapping: 6, q1/q2: 5) mit Reserve; schema-stabil gegen weitere Felder (Phase B).
inline constexpr std::size_t kV3FieldCount = 8;

// ── Schema-Tabelle: (axis_idx, field_idx) → Feldname. SINGLE-SOURCE in kCompositionAxisNames-Reihenfolge.
//    Leere Strings = (noch) nicht befülltes Feld; eine Achse, deren Felder alle "" sind, ist Phase-B (= 0).
//    Diese Tabelle treibt die CSV-Spaltennamen (stat_<achse>_<feld>) → keine Namens-Drift. Seit Phase-B-Abschluss
//    (2026-06-04) sind ALLE 19 Achsen befüllt (Phase A: T0,T1,T2,T4,T5,T6,T9,T10,T17,T18; Phase B ergänzt
//    T3 path_compression, T7 prefetch, T8 concurrency, T11 value_handle, T12 isa, T13 index_org, T14 io_dispatch,
//    T15 migration_policy, T16 filter). ─────────────────────────────────────────────────────────────────────────
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
    /*T3  path_compression*/ {{"compress", "prefix_len", "bytes_saved", "cuts", "checksum", nullptr, nullptr, nullptr}},  // Phase B (T3)
    /*T4  node_type*/        {{"find", "keys_stored", "queries", "checksum", nullptr, nullptr, nullptr, nullptr}},
    /*T5  memory_layout*/    {{"scan", "records", "field_bytes", "cache_lines", "checksum", nullptr, nullptr, nullptr}},
    /*T6  allocator*/        {{"bytes_alloc", "bytes_in_use", "alloc_cnt", "dealloc_cnt", "fail", nullptr, nullptr, nullptr}},
    /*T7  prefetch*/         {{"trigger", "suggestions", "hot_path_hints", "max_queue_depth", "addrs_enqueued", nullptr, nullptr, nullptr}},  // Phase B (T7)
    /*T8  concurrency*/      {{"acquire", "release", "contention", "validation_fail", "pattern_id", nullptr, nullptr, nullptr}},  // Phase B (T8)
    /*T9  serialization*/    {{"serialize", "records", "bytes", "checksum", nullptr, nullptr, nullptr, nullptr}},
    /*T10 telemetry*/        {{"events", "leaf_updates", "node_updates", "peak_tracked", nullptr, nullptr, nullptr, nullptr}},
    /*T11 value_handle*/     {{"access", "indirect_deref", "version_strips", "peak_chain_depth", nullptr, nullptr, nullptr, nullptr}},  // Phase B (T11)
    /*T12 isa*/              {{"simd_calls", "elements", "simd_iters", "scalar_fallback", "checksum", nullptr, nullptr, nullptr}},  // Phase B (T12)
    /*T13 index_org*/        {{"scan", "records", "predicate_evals", "indirect_lookups", "checksum", nullptr, nullptr, nullptr}},  // Phase B (T13)
    /*T14 io_dispatch*/      {{"rounds", "bytes", "align_adjusts", "dispatch_cnt", "checksum", nullptr, nullptr, nullptr}},  // Phase B (T14)
    /*T15 migration_policy*/ {{"decisions", "migrations", "hot_votes", "cold_votes", "tier_moves", nullptr, nullptr, nullptr}},  // Phase B (T15)
    /*T16 filter*/           {{"probe", "pos", "neg", "hash_probes", "checksum", nullptr, nullptr, nullptr}},  // Phase B (T16)
    /*T17 queuing_q1*/       {{"put", "get", "overflow", "underflow", "peak_size", nullptr, nullptr, nullptr}},
    /*T18 queuing_q2*/       {{"decisions", "full_flush", "partial_flush", "no_flush", "flush_complete", nullptr, nullptr, nullptr}},
};

/// Anzahl der im POD befüllten Achsen = Achsen mit mindestens EINEM benannten Schema-Feld (single-source aus
/// kV3AxisSchema abgeleitet, NICHT hartkodiert). So zieht diese Diagnose-/Test-Konstante automatisch mit, wenn
/// eine Phase-B-Achse (T3/T7/T8/T11..T16) ihre Schema-Zeile befüllt — keine Drift, kein manuelles Nachziehen.
[[nodiscard]] constexpr std::size_t v3_count_filled_axes() noexcept {
    std::size_t n = 0;
    for (std::size_t t = 0; t < kV3AxisCount; ++t) {
        if (kV3AxisSchema[t].names[0] != nullptr) ++n;   // erste Spalte benannt ⇒ Achse trägt Observer-Felder
    }
    return n;
}
inline constexpr std::size_t kV3FilledAxisCount = v3_count_filled_axes();

// ─────────────────────────────────────────────────────────────────────────────
// ComdareTierObserverSnapshot — der EINE konsolidierte, versionierte Observer-POD (I1)
// ─────────────────────────────────────────────────────────────────────────────

/// Komposition-UNABHÄNGIGER, flacher, versionierter Observer-POD über die Modul-Binary-ABI-Grenze. EIN POD für
/// ALLES: `axis_stats[19][8]` = die Per-Achsen-Observer-Felder (Schema = kV3AxisSchema, single-source);
/// `seg_ns[19]` = das Pfad-B-Per-Achsen-Timing über die REALE Komposition (User-Entscheid 2026-06-04); + Meta.
/// Ersetzt die getrennten V1/V2/V3-Observer-PODs + den seg_ns-Timing-POD (Preflight wkqt7a0il: axis_stats+Meta
/// subsumiert JEDES frühere V1- (13) + V2-Feld (26) — V1 search→[0][0..5], alloc→[6][0..4]; V2 telemetry→[10],
/// layout→[5], serialization→[9], node_type→[4]; obs_axes/fill→Meta). Layout bitweise stabil (alle Member 8-B-
/// Ints, kein Padding; sizeof==1400, alignof==8) → memcpy über die ABI-Grenze. Versionierung jetzt über ABI-Major.
struct ComdareTierObserverSnapshot {
    std::uint64_t axis_stats[kV3AxisCount][kV3FieldCount] = {};  // T0..T18 × 8 Felder (Schema = kV3AxisSchema)
    std::int64_t  seg_ns[kV3AxisCount]                    = {};  // Pfad-B Per-Achsen-Timing (ns, je Achse)
    std::uint64_t observable_axis_count                  = 0;    // Meta: # observable Achsen (in-process)
    std::uint64_t tier_fill_level                        = 0;    // Meta: aktueller Füllstand (tier_size)
    std::uint64_t filled_axis_count                      = 0;    // Meta: # Achsen mit Observer-Werten
    std::uint64_t batches_measured                       = 0;    // Meta: # Timing-Batches (Warmup verworfen)

    [[nodiscard]] constexpr bool operator==(ComdareTierObserverSnapshot const&) const noexcept = default;
};
static_assert(std::is_standard_layout_v<ComdareTierObserverSnapshot>,
              "ABI-Pflicht: konsolidierter Observer-POD muss standard_layout sein (memcpy über DLL-Grenze)");
static_assert(std::is_trivially_copyable_v<ComdareTierObserverSnapshot>,
              "ABI-Pflicht: konsolidierter Observer-POD muss trivially_copyable sein");

/// Format-Version des konsolidierten Observer-POD (die Loader-Kompatibilität läuft über ABI-Major, s.
/// anatomy_module_abi_v1_decl.hpp; diese Konstante dient der Diagnose/Tests).
inline constexpr std::uint32_t kTierObserverSnapshotVersionUnified = 4;

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
/// und ergänzt NUR die Beobachtung (tier_observe). Der ABI-Adapter vererbt IObservableTier NUR bei Messung-AN
/// (COMDARE_MEASUREMENT_ON) — die Antriebs-Ops bleiben über IDriveableTier auch in der Release-/funktional-only-DLL.
class IObservableTier : public IDriveableTier {
public:
    ~IObservableTier() override = default;

    /// Die EINE Observer-Methode (I1): schreibt den konsolidierten Snapshot (axis_stats[19][8] + seg_ns[19] +
    /// Meta) nach *out. out != nullptr. noexcept (reines Auslesen + lesendes Pfad-B-Timing). Der Host stempelt
    /// das Resultat mit Wall-Clock + persistiert. Der ABI-Adapter implementiert die feste Sequenz Observer-READ
    /// → Pfad-B-Timing → per-op-Reset (gegen Doppelzählung).
    virtual void tier_observe(ComdareTierObserverSnapshot* out) const noexcept = 0;
};

}  // namespace comdare::cache_engine::anatomy
