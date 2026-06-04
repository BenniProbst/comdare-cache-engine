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
#include <type_traits>   // (C-2) static_assert standard_layout/trivially_copyable für ComdareSegmentLatencyV1

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
    [[nodiscard]] virtual std::uint64_t run_workload(std::uint64_t ops_per_batch,
                                                     std::uint64_t batches,
                                                     std::uint64_t seed,
                                                     std::int64_t* out_latencies_ns,
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
    std::int64_t seg_search_algo_ns   = 0;   // Segment 1 (axis_03a search_algo): Lookups auf der Such-Struktur
    std::int64_t seg_allocator_ns     = 0;   // Segment 2 (axis_06 allocator): alloc/dealloc-Churn
    std::int64_t seg_memory_layout_ns = 0;   // Segment 3 (axis_05 memory_layout): Feld-Scan
    std::int64_t seg_serialization_ns = 0;   // Segment 4 (axis_10 serialization): Encode-Scan
    std::int64_t total_ns             = 0;   // Σ der 4 Segmente über alle gemessenen Batches (Konsistenz-Diagnose)
    std::uint64_t batches_measured    = 0;   // wie viele Batches einflossen (Warmup verworfen)

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
    [[nodiscard]] virtual std::uint64_t run_workload_segmented(std::uint64_t ops_per_batch,
                                                               std::uint64_t batches,
                                                               std::uint64_t seed,
                                                               ComdareSegmentLatencyV1* out) noexcept = 0;
};

}  // namespace comdare::cache_engine::anatomy
