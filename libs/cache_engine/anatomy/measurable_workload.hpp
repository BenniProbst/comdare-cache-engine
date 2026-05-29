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

}  // namespace comdare::cache_engine::anatomy
