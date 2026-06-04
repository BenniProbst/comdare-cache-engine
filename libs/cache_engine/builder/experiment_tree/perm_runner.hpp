#pragma once
// D14 / L-CLUSTER-E2E (gate-frei, 2026-06-02) — perm_runner: der lokale Mess-Runner je Binary. Auf dem Cluster
// fährt jede SLURM-Task EINEN perm_runner mit EINER geladenen perm-DLL (AnatomyModuleLoader), treibt den
// Mess-Workload über das Tier-Sub-Interface, zieht den Observer und emittiert EINE Ergebnis-Zeile im
// result_ingest-Format (binary_id + 13 Felder). Schließt die gate-freie Kette: perm_runner (Mess→Format) ↔
// result_ingest (Format→Baum-NodeValue). LOKAL verifizierbar (Mock/in-process); echte Cluster-Submission = GATE-MAXIMAL.
//
// Format-Identität mit result_ingest.hpp garantiert den Round-Trip (Mess → Zeile → Ingest → identischer NodeValue).
//
// MESS-ARCHITEKTUR-UMBAU (2026-06-04, Anforderungen A/B/C):
//   (A) Reset je Messung: run_observable_perm ruft ZUERST tier_clear() (frischer Zustand) und bildet die
//       Observer-Werte als DELTA (post − pre) — die Statistik-Zähler im search_organ_ sind per ABI nicht
//       resetbar, also eliminiert die Delta-Bildung das kumulative Artefakt (search_lookup 2000→4000→…).
//   (B) Gesamt-Wall-Clock: steady_clock um insert+lookup → total_ns je Messung (Host-Messung, KEINE
//       Baum-Knoten-Eigenschaft → reist NUR über PermResult/LazyMeasuredRow in die CSV, NICHT über ingest).
//   (X) Echter per-Segment-Timer auf ALLE 19 Achsen: drive_segment_latencies() ruft das ABI-Sub-Interface
//       IMeasurableWorkloadV3 (run_workload_segmented_v2) → 19 aufsummierte per-Achsen-ns (T0..T18). KEINE Achse
//       n/a mehr (jede treibt eine reale, strategie-abhängige Op); nur eine DLL OHNE V3-Interface → CSV n/a.

#include "experiment_tree.hpp"                      // NodeObserverSnapshot
#include "../../anatomy/observable_tier.hpp"        // IObservableTier + ComdareTierObserverSnapshotV1
#include "../../anatomy/measurable_workload.hpp"    // (X): IMeasurableWorkloadV3 + ComdareSegmentLatencyV2 (19 Segmente)

#include <chrono>
#include <cstdint>
#include <string>

namespace comdare::cache_engine::builder::experiment {

/// Formatiert einen Observer-Snapshot als result_ingest-Zeile (binary_id + 13 ';'-Felder, NodeObserverSnapshot-
/// Reihenfolge). EXAKT das von ingest_result_line erwartete Format → Round-Trip-Garant.
[[nodiscard]] inline std::string format_perm_result(std::string const& binary_id,
                                                    anatomy::ComdareTierObserverSnapshotV1 const& s) {
    std::string out = binary_id;
    auto add = [&](std::uint64_t v) { out += ';'; out += std::to_string(v); };
    add(s.search_lookup_count);   add(s.search_hit_count);       add(s.search_miss_count);
    add(s.search_insert_count);   add(s.search_erase_count);     add(s.search_peak_occupancy);
    add(s.alloc_bytes_allocated); add(s.alloc_bytes_in_use);     add(s.alloc_allocation_count);
    add(s.alloc_deallocation_count); add(s.alloc_failure_count); add(s.observable_axis_count);
    add(s.tier_fill_level);
    return out;
}

// ── (B): Ergebnis einer Mess-Last (result_ingest-Zeile + die Host-Wall-Clock) ─────────────────────────
struct PermResult {
    std::string  line;        // result_ingest-Zeile (binary_id + 13 Observer-Delta-Felder)
    std::int64_t total_ns = 0;   // (B) steady_clock-Wall-Clock um insert+lookup DIESER Messung
    std::uint64_t n_ops   = 0;   // Eingabe-n_ops (für ns_per_op = total_ns / (2*n_ops): insert+lookup)
    // Phase A (2026-06-04): der GENERISCHE Per-Achsen-V3-Snapshot (axis_stats[19][8]) NACH der Mess-Last —
    // die 10 Phase-A-Achsen tragen echte statistics()-Werte (>0), die 9 Phase-B-Achsen 0. v3_real=false →
    // das Modul exponiert IObservableTierV3 nicht (alte DLL) → ehrlich „n/a" in der CSV.
    anatomy::ComdareTierObserverSnapshotV3 v3{};
    bool         v3_real  = false;
    // KONSOLIDIERUNG (I-B, 2026-06-04): der EINE konsolidierte Observer-Snapshot (axis_stats[19][8] + seg_ns[19] +
    // Meta) aus dem EINEN tier_observe. Trägt Observer-Stats UND das Pfad-B-Per-Achsen-Timing (reale Komposition) in
    // EINEM POD. Wird in I-B die maßgebliche CSV-Quelle (ersetzt v3 + den Pfad-A-Segment-Timer); V1/V3 entfallen in I-C.
    anatomy::ComdareTierObserverSnapshot unified{};
    bool         unified_real = false;
};

/// (A)+(B) Treibt ein geladenes IObservableTier (SearchAlgorithm-Mess-Pfad): ZUERST tier_clear() (frischer
/// Zustand → kein kumulatives Artefakt), pre-Observe (Baseline der absoluten Zähler), n_ops insert + n_ops
/// lookup UNTER steady_clock-Messung (total_ns), post-Observe, und bildet die result_ingest-Zeile aus dem
/// DELTA (post − pre) der getriebenen Zähler. Der host-/cluster-seitige Unikat-Mess-Lauf je Binary.
[[nodiscard]] inline PermResult run_observable_perm(anatomy::IObservableTier& tier,
                                                    std::string const& binary_id, std::uint64_t n_ops,
                                                    anatomy::IObservableTierV3* v3 = nullptr) {
    // (A) Reset: frischer Datenstruktur-Zustand + Baseline der (nicht resetbaren) absoluten Statistik-Zähler.
    tier.tier_clear();
    anatomy::ComdareTierObserverSnapshotV1 pre{};
    tier.tier_observe(&pre);

    // (B) Gesamt-Wall-Clock um die GANZE Mess-Last (insert + lookup).
    auto const t0 = std::chrono::steady_clock::now();
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)tier.tier_insert(i, i * 7u + 1u);
    for (std::uint64_t i = 0; i < n_ops; ++i) { std::uint64_t v = 0; (void)tier.tier_lookup(i, &v); }
    auto const t1 = std::chrono::steady_clock::now();

    anatomy::ComdareTierObserverSnapshotV1 post{};
    tier.tier_observe(&post);

    // (A) Delta-Bildung: die getriebenen Zähler als (post − pre); Meta-Felder (observable_axis_count,
    // tier_fill_level) sind Zustand zum Snapshot-Zeitpunkt → direkt aus post (keine Differenz).
    anatomy::ComdareTierObserverSnapshotV1 d{};
    auto sub = [](std::uint64_t a, std::uint64_t b) noexcept -> std::uint64_t { return (a >= b) ? (a - b) : 0u; };
    d.search_lookup_count      = sub(post.search_lookup_count,      pre.search_lookup_count);
    d.search_hit_count         = sub(post.search_hit_count,         pre.search_hit_count);
    d.search_miss_count        = sub(post.search_miss_count,        pre.search_miss_count);
    d.search_insert_count      = sub(post.search_insert_count,      pre.search_insert_count);
    d.search_erase_count       = sub(post.search_erase_count,       pre.search_erase_count);
    d.search_peak_occupancy    = post.search_peak_occupancy;   // Peak ist kein additiver Counter → post-Wert
    d.alloc_bytes_allocated    = sub(post.alloc_bytes_allocated,    pre.alloc_bytes_allocated);
    d.alloc_bytes_in_use       = post.alloc_bytes_in_use;      // Füllstand → post-Wert
    d.alloc_allocation_count   = sub(post.alloc_allocation_count,   pre.alloc_allocation_count);
    d.alloc_deallocation_count = sub(post.alloc_deallocation_count, pre.alloc_deallocation_count);
    d.alloc_failure_count      = sub(post.alloc_failure_count,      pre.alloc_failure_count);
    d.observable_axis_count    = post.observable_axis_count;   // Meta (Diagnose) → post-Wert
    d.tier_fill_level          = post.tier_fill_level;          // Füllstand → post-Wert

    PermResult r;
    r.line    = format_perm_result(binary_id, d);
    r.total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    r.n_ops   = n_ops;
    // Phase A+B (2026-06-04): den generischen Per-Achsen-V3-Snapshot NACH der Mess-Last ziehen (Pfad B).
    //
    // KONSISTENZ-STAND (nach dem tier_clear-Reset-Fix 2026-06-04, abi_adapter.hpp tier_clear()): die auto-
    // gekoppelten Instanz-Organe T1 ct / T2 map / T10 telemetry / T17 q1 / T18 q2 (+ Phase B T3 pc / T7 pf /
    // T8 cc) werden in tier_clear() jetzt NICHT NUR daten-geleert (clear()) SONDERN auch STATISTIK-genullt
    // (reset()) → der post-Workload-Snapshot trägt genau die Op-Zähler DIESER Messung. ZUVOR (Defekt) riefen
    // T1/T2/T17 nur clear() (= nur Daten) und T18/T10 gar nichts → ihre kumulativen statistics() akkumulierten
    // über die 3 Wiederholungen je (Binary×Setting) (1000→2000→3000). Mit dem Reset-Fix gilt für diese Organe
    // jetzt wieder: Reset-bei-Start == post (warmup-frei). Die Pfad-B-Scan-Achsen (T4 node / T5 layout / T9 ser /
    // T11 value_handle / T12 isa / T13 index_org / T14 io_dispatch / T15 migration / T16 filter) sind in
    // fill_observer_v3 IDEMPOTENT (reset()+scan je Observe) → Zustand zum Observe-Zeitpunkt, KEIN Warmup-Doppel-
    // zähler. T0 search_algo + T6 allocator (V1-POD) tragen ABI-seitig nicht resetbare Zähler → für SIE bildet
    // der Block oben das echte post−pre-Delta. Netto: ALLE 19 Achsen sind pro Zeile warmup-frei + konsistent mit
    // dem V1-Delta-Block (kein kumulatives Artefakt), seit der Reset-Fix die Instanz-/telemetry-Organe nullt.
    if (v3 != nullptr) { v3->tier_observe_v3(&r.v3); r.v3_real = true; }
    // KONSOLIDIERUNG (I-B): den EINEN konsolidierten Snapshot ziehen (axis_stats + Pfad-B-seg_ns in EINEM POD).
    // NACH dem V3-Observe (der V3-Pfad ist additiv noch da, bis I-C ihn entfernt). Der EINE tier_observe hält
    // intern die fixe Sequenz (axis_stats-READ → seg_ns-Timing → per-op-Reset) → keine Doppelzählung. Da tier_clear
    // jetzt search_organ_.reset() ruft, ist axis_stats[0] frisch (kein post−pre-Delta nötig).
    tier.tier_observe(&r.unified);
    r.unified_real = true;
    return r;
}

/// (X) Treibt — falls das geladene Modul IMeasurableWorkloadV3 exponiert — den 19-Segment-Workload und liefert
/// die ECHT gemessenen, über die Batches aufsummierten per-Achsen-ns ALLER 19 SearchAlgorithm-Achsen
/// (T0..T18, kein n/a mehr). `tier` ist das via dynamic_cast erhaltene Sub-Interface (nullptr → out bleibt 0,
/// → CSV ehrlich n/a). ops_per_batch/batches sind die Mess-Parameter; seed deterministisch. Gibt batches_measured (>0 = real).
[[nodiscard]] inline std::uint64_t drive_segment_latencies(anatomy::IMeasurableWorkloadV3* tier,
                                                           std::uint64_t ops_per_batch, std::uint64_t batches,
                                                           std::uint64_t seed,
                                                           anatomy::ComdareSegmentLatencyV2& out) {
    out = anatomy::ComdareSegmentLatencyV2{};
    if (tier == nullptr) return 0;   // alte/ohne-V3 DLL → ehrlich n/a (out bleibt 0)
    return tier->run_workload_segmented_v2(ops_per_batch, batches, seed, &out);
}

}  // namespace comdare::cache_engine::builder::experiment
