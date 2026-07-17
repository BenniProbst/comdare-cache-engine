#pragma once
// V41.F.6.1 F15 / Stufe B — IMeasurableWorkload: optionales Mess-Sub-Interface.
//
// ABI-SICHER laut Designprinzip von anatomy_module_abi_v1.hpp (§ "Anatomy-API-Erweiterungen brechen
// die ABI NICHT — erweiterbar über NEUE Sub-Interfaces statt neue Funktion-Pointer/vtable-Methoden"):
// run_workload wird NICHT an IAnatomyBase gehängt (das änderte dessen vtable-Layout und bräche bereits
// kompilierte DLLs), sondern als eigenständiges Sub-Interface, das der ABI-Adapter ZUSÄTZLICH erbt.
//
// Der Host fragt es via `dynamic_cast<IMeasurableWorkload*>(ianatomy_ptr)` ab:
//   - neue DLLs (Adapter implementiert das Interface) → gültiger Pointer → Mess-Last läuft IN der DLL.
//   - alte DLLs (ohne das Interface) → nullptr → Host degradiert sauber (kein Crash, kein ABI-Bruch).
//
// Damit läuft die F15-Mess-Last (Stufe B) DURCH die geladene Komposition (deren eigene Such-Struktur),
// nicht mehr nur host-seitig (Stufe A).

#include <cstdint>
#include <type_traits> // (C-2) static_assert standard_layout/trivially_copyable für ComdareSegmentLatencyV1

namespace comdare::cache_engine::anatomy {

/// IMeasurableWorkload — optionales Mess-Sub-Interface einer geladenen Anatomie.
class IMeasurableWorkload {
public:
    virtual ~IMeasurableWorkload() = default;

    /// Fährt `batches` Batches à `ops_per_batch` Lookups (seed-deterministisch) auf der EIGENEN
    /// Such-Struktur der Komposition und schreibt die Batch-Latenzen (ns) nach
    /// out_latencies_ns[0 .. min(batches, out_capacity)). Ein Warmup-Batch wird verworfen.
    /// Rückgabe: Anzahl geschriebener Samples (0 = nicht gemessen / nullptr / Exception).
    /// noexcept — interne Exceptions (z.B. OOM bei Bulk-Load) werden zu Rückgabe 0 abgefangen.
    [[nodiscard]] virtual std::uint64_t run_workload(std::uint64_t ops_per_batch, std::uint64_t batches,
                                                     std::uint64_t seed, std::int64_t* out_latencies_ns,
                                                     std::uint64_t out_capacity) noexcept = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// (C-2) per-Segment-Latenz-POD — die 4 in run_workload/do_batch instrumentierten Achsen
// ─────────────────────────────────────────────────────────────────────────────

/// ComdareSegmentLatencyV1 — flacher, ABI-stabiler POD, der die ECHT gemessene (über die batches
/// AUFSUMMIERTE) Wall-Clock-Zeit der 4 in do_batch (abi_adapter.hpp) instrumentierten Achsen-Segmente
/// über die Modul-Binary-Grenze trägt: search_algo / allocator / memory_layout / serialization.
/// NUR int64-Felder → standard_layout + trivially_copyable (memcpy über die .dll-Grenze, identisch zum
/// Observer-Snapshot-Designprinzip). Die übrigen 15 Achsen sind passive Compile-Time-Deskriptoren ohne
/// Laufzeit-Segment-Timer → der Host kennzeichnet sie ehrlich als n/a (NICHT 0, NICHT erfunden).
struct ComdareSegmentLatencyV1 {
    std::int64_t  seg_search_algo_ns   = 0; // Segment 1 (axis_03a search_algo): Lookups auf der Such-Struktur
    std::int64_t  seg_allocator_ns     = 0; // Segment 2 (axis_06 allocator): alloc/dealloc-Churn
    std::int64_t  seg_memory_layout_ns = 0; // Segment 3 (axis_05 memory_layout): Feld-Scan
    std::int64_t  seg_serialization_ns = 0; // Segment 4 (axis_10 serialization): Encode-Scan
    std::int64_t  total_ns             = 0; // Σ der 4 Segmente über alle gemessenen Batches (Konsistenz-Diagnose)
    std::uint64_t batches_measured     = 0; // wie viele Batches einflossen (Warmup verworfen)

    [[nodiscard]] constexpr bool operator==(ComdareSegmentLatencyV1 const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<ComdareSegmentLatencyV1>,
              "ABI-Pflicht: per-Segment-Latenz-POD muss standard_layout sein");
static_assert(std::is_trivially_copyable_v<ComdareSegmentLatencyV1>,
              "ABI-Pflicht: per-Segment-Latenz-POD muss memcpy-fähig (trivially_copyable) sein");

/// IMeasurableWorkloadV2 — EIGENSTÄNDIGES Sub-Interface (V42 L-74c-ABI-Prinzip: hängt NICHT an
/// IMeasurableWorkload/IAnatomyBase → ändert deren vtable NICHT; der Host fragt via
/// `dynamic_cast<IMeasurableWorkloadV2*>(ianatomy_ptr)`, alte Module → nullptr → sauberer Degrade).
/// Fährt denselben 4-Segment-do_batch wie run_workload, misst aber JE SEGMENT einen eigenen
/// steady_clock-Timer und summiert die 4 Segment-Zeiten über die batches → echter per-Achsen-Timer
/// für genau die 4 run_workload-getriebenen Achsen.
class IMeasurableWorkloadV2 {
public:
    virtual ~IMeasurableWorkloadV2() = default;

    /// Fährt `batches` Batches à `ops_per_batch` Operationen (seed-deterministisch), je Batch die 4
    /// instrumentierten Segmente mit EIGENEM Timer, und schreibt die über alle (Nicht-Warmup-)Batches
    /// aufsummierten per-Segment-ns nach *out. out != nullptr. noexcept (interne Exception → out bleibt 0,
    /// Rückgabe 0). Rückgabe: Anzahl eingeflossener Batches (= batches_measured).
    [[nodiscard]] virtual std::uint64_t run_workload_segmented(std::uint64_t ops_per_batch, std::uint64_t batches,
                                                               std::uint64_t            seed,
                                                               ComdareSegmentLatencyV1* out) noexcept = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// (X) 18-Segment-Latenz-POD — der per-Achsen-Timer auf ALLE 18 SearchAlgorithm-Achsen (INC-2c: telemetry ist System-Achse) ausgeweitet
// ─────────────────────────────────────────────────────────────────────────────

/// ComdareSegmentLatencyV2 — flacher, ABI-stabiler POD, der die ECHT gemessene (über die batches
/// AUFSUMMIERTE) per-Achsen-Wall-Clock ALLER 18 SearchAlgorithm-Achsen über die Modul-Binary-Grenze trägt.
/// INDEXBASIERT (seg_ns[18], NICHT benannte Felder): der Slot-Index IST die kanonische Achsen-Identität
/// (T0..T17 == kCompositionAxisNames-Reihenfolge in builder/experiment_tree/axis_path_serialization.hpp:27-32);
/// der CSV-Writer iteriert dann `for i in 0..17` statt 18 Felder hartzucodieren, und eine spätere Achsen-
/// Umordnung verschiebt nur die Index-Bedeutung, bricht aber nicht still Name↔Feld. NUR int64/uint64-Felder
/// → standard_layout + trivially_copyable (memcpy über die .dll-Grenze, identisch zum Observer-Snapshot-Prinzip).
/// V1 (4 benannte Segmente) bleibt UNVERÄNDERT erhalten (ABI-Erhalt für alte DLLs / bestehende Tests).
///
/// Seg-Index-Map (T0..T17): 0 search_algo · 1 cache_traversal · 2 mapping · 3 path_compression · 4 node_type
/// · 5 memory_layout · 6 allocator · 7 prefetch · 8 concurrency · 9 serialization · 10 telemetry
/// · 11 value_handle · 12 isa · 13 index_organization · 14 io_dispatch · 15 migration_policy · 16 filter
/// · 17 queuing_q1 · 18 queuing_q2. KEINE Achse n/a — jede treibt eine reale, strategie-abhängige Op.
struct ComdareSegmentLatencyV2 {
    std::int64_t  seg_ns[18]       = {}; // Index T0..T17 == kCompositionAxisNames-Reihenfolge (INC-2c: telemetry raus)
    std::int64_t  total_ns         = 0;  // Σ seg_ns[0..18] über alle gemessenen Batches (Konsistenz-Diagnose)
    std::uint64_t batches_measured = 0;  // wie viele Batches einflossen (Warmup verworfen)
    // P-MD3 (Coverage-Versöhnung, 2026-06-18): die Attribution der Per-Achsen-Zeit braucht einen EIGENEN, kommensurablen
    // Nenner. seg_run_total_ns = die GESAMTE Wall-Clock des gemessenen Batch-Laufs DIESES Timers (äußere steady_clock
    // um die Nicht-Warmup-Batches). seg_framework_ns = seg_run_total_ns − Σseg_ns = der NICHT-segmentierte Rest (rng,
    // Schleifen-/Branch-/Aufruf-Overhead ZWISCHEN den Segment-Timern) — der bisher UNSICHTBARE Rest, der die Coverage
    // gegen total_ns (Real-Workload-Wall-Clock) auf ~33% drückte. Mit diesen 2 Feldern versöhnt sich die Per-Achsen-
    // Attribution gegen IHRE EIGENE Wall-Clock: Σseg_ns + seg_framework_ns ≡ seg_run_total_ns (Coverage ~100% per
    // Konstruktion, Rest EXPLIZIT benannt). REIN ADDITIV (hinten angehängt) → ABI-Layout der bestehenden Felder unberührt.
    std::int64_t seg_framework_ns = 0; // benannter Rest = seg_run_total_ns − Σseg_ns (Instrumentierungs-/Loop-Overhead)
    std::int64_t seg_run_total_ns = 0; // äußere Wall-Clock des Segment-Mess-Laufs (kommensurabler Nenner der Coverage)

    [[nodiscard]] constexpr bool operator==(ComdareSegmentLatencyV2 const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<ComdareSegmentLatencyV2>,
              "ABI-Pflicht: 18-Segment-Latenz-POD muss standard_layout sein");
static_assert(std::is_trivially_copyable_v<ComdareSegmentLatencyV2>,
              "ABI-Pflicht: 18-Segment-Latenz-POD muss memcpy-fähig (trivially_copyable) sein");

/// IMeasurableWorkloadV3 — EIGENSTÄNDIGES Sub-Interface (L-74c-ABI-Prinzip: hängt NICHT an
/// IMeasurableWorkload(V2)/IAnatomyBase → ändert deren vtable NICHT; der Host fragt via
/// `dynamic_cast<IMeasurableWorkloadV3*>(ianatomy_ptr)`, alte Module → nullptr → sauberer Degrade auf V2/V1).
/// Fährt einen 18-Segment-do_batch: je SearchAlgorithm-Achse ein eigener steady_clock-Timer, über die batches
/// AUFSUMMIERT → echter per-Achsen-Timer für ALLE 18 Achsen (kein n/a mehr).
class IMeasurableWorkloadV3 {
public:
    virtual ~IMeasurableWorkloadV3() = default;

    /// Fährt `batches` Batches à `ops_per_batch` Operationen (seed-deterministisch), je Batch die 18
    /// instrumentierten Achsen-Segmente mit EIGENEM Timer, und schreibt die über alle (Nicht-Warmup-)Batches
    /// aufsummierten per-Segment-ns nach *out (out != nullptr). noexcept (interne Exception → out bleibt 0,
    /// Rückgabe 0). Rückgabe: Anzahl eingeflossener Batches (= batches_measured).
    [[nodiscard]] virtual std::uint64_t run_workload_segmented_v2(std::uint64_t ops_per_batch, std::uint64_t batches,
                                                                  std::uint64_t            seed,
                                                                  ComdareSegmentLatencyV2* out) noexcept = 0;
};

} // namespace comdare::cache_engine::anatomy
