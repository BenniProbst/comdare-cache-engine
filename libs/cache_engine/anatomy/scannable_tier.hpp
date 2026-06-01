#pragma once
// V5-#49-E — IScannableTier: das ABI-stabile Range-Scan-Sub-Interface (Tier-Binary-Seite, YCSB-E).
//
// YCSB-Treue (Task #49 Pfad A, User-Direktive 2026-05-31): YCSB-E ist das „short-range scan"-Profil
// (95% Range-Scan / 5% Insert). Eine Range-Scan-Operation liest, ausgehend von einem Start-Key, die nächsten
// `scan_length` Records IN KEY-REIHENFOLGE. Das funktionale Antriebs-Interface (IDriveableTier) bietet nur
// Punkt-Ops (insert/lookup/erase) — ein Range-Scan braucht GEORDNETE Iteration, eine NEUE Fähigkeit.
//
// ABI-SICHER nach EXAKT demselben Designprinzip wie IRollbackableTier/IObservableTier/IMeasurableWorkload:
//   - eigenständiges Sub-Interface, das der genus-typisierte ABI-Adapter NUR bei COMDARE_MEASUREMENT_ON
//     ZUSÄTZLICH erbt (in der Release-/funktional-only-DLL restlos compile-time entfernt — kein vtable-Slot).
//   - IScannableTier hängt NICHT an IObservableTier/IDriveableTier (das änderte deren vtable-Layout, bräche
//     bereits gebaute DLLs — genau der Fehler, der den ersten YCSB-E/F-Versuch zum SEH-Crash führte).
//   - Der Host fragt es via `dynamic_cast<IScannableTier*>(ianatomy_ptr)` ab; alt-gebaute DLLs ohne diese
//     Fähigkeit → nullptr → der Host überspringt Scan-Ops graziös (kein ABI-Bruch, wie IRollbackableTier).
//   - Quert die Grenze als reine vtable (uint64-Parameter, kein POD-by-value, keine STL) → ABI-stabil.
//
// ABI-Minor 1→2: ein neuerer Host (kennt IScannableTier) akzeptiert ältere Module (minor 0/1, ohne Scan)
// weiterhin; der dynamic_cast liefert dort null → YCSB-E-Profile fallen für solche Module aus (ehrlich).
//
// @doku docs/architecture/messarchitektur_v5_design.md §4 (Op-Schleife) + Cooper et al., ACM SoCC 2010 (YCSB-E)
// @related [[feedback_no_quick_fixes]] [[feedback_algorithm_correctness_when_named]]

#include <cstdint>

namespace comdare::cache_engine::anatomy {

/// IScannableTier — optionales Range-Scan-Sub-Interface einer geladenen Tier-Anatomie (nur Messung-AN, YCSB-E).
///
/// Der Host treibt damit das YCSB-E-Profil: ab `start_key` werden bis zu `max_count` Records IN KEY-REIHENFOLGE
/// gelesen. Liefert die Anzahl tatsächlich besuchter Records (≤ max_count, ≤ aktuelle Füllung ab start_key).
/// `out_checksum` (falls != nullptr) akkumuliert die gelesenen Werte — Anti-Wegoptimierungs-Senke UND
/// Korrektheits-Probe (deterministisch bei gleichem Zustand). const (Scan verändert das Tier nicht) + noexcept
/// (eine interne Störung muss abgefangen werden → Rückgabe 0, Mess-Robustheit).
class IScannableTier {
public:
    virtual ~IScannableTier() = default;

    virtual std::uint64_t tier_scan(std::uint64_t start_key,
                                    std::uint64_t max_count,
                                    std::uint64_t* out_checksum) const noexcept = 0;
};

}  // namespace comdare::cache_engine::anatomy
