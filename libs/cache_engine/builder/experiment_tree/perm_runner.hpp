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
#include "../../anatomy/observable_tier.hpp"        // IObservableTier + ComdareTierObserverSnapshot (I1: EINE Schnittstelle/EIN POD)
#include "../../anatomy/measurable_workload.hpp"    // Pfad A: IMeasurableWorkloadV3 + ComdareSegmentLatencyV2 (19 Segmente)

#include <chrono>
#include <cstdint>
#include <string>

namespace comdare::cache_engine::builder::experiment {

/// KONSOLIDIERUNG (I-B.3, 2026-06-04, User-Fork „voll auf axis_stats[19][8]"): formatiert den EINEN konsolidierten
/// Observer-POD als result_ingest-Zeile = binary_id + die VOLLE Matrix: axis_stats[19][8] (152) + seg_ns[19] (19)
/// + Meta (observable_axis_count, tier_fill_level, filled_axis_count, batches_measured) = 175 Felder. EXAKT das von
/// ingest_result_line erwartete Format → Round-Trip-Garant (Cluster: perm_runner→Zeile→ingest→Baum-NodeValue).
[[nodiscard]] inline std::string format_perm_result(std::string const& binary_id,
                                                    anatomy::ComdareTierObserverSnapshot const& s) {
    std::string out = binary_id;
    auto addu = [&](std::uint64_t v) { out += ';'; out += std::to_string(v); };
    auto addi = [&](std::int64_t v)  { out += ';'; out += std::to_string(v); };
    for (std::size_t t = 0; t < 19; ++t) for (std::size_t f = 0; f < 8; ++f) addu(s.axis_stats[t][f]);
    for (std::size_t t = 0; t < 19; ++t) addi(s.seg_ns[t]);
    addu(s.observable_axis_count); addu(s.tier_fill_level); addu(s.filled_axis_count); addu(s.batches_measured);
    return out;
}

// ── (B): Ergebnis einer Mess-Last (result_ingest-Zeile + die Host-Wall-Clock) ─────────────────────────
struct PermResult {
    std::string  line;        // result_ingest-Zeile (binary_id + volle Observer-Matrix)
    std::int64_t total_ns = 0;   // (B) steady_clock-Wall-Clock um insert+lookup DIESER Messung
    std::uint64_t n_ops   = 0;   // Eingabe-n_ops (für ns_per_op = total_ns / (2*n_ops): insert+lookup)
    // KONSOLIDIERUNG (I1): der EINE konsolidierte Observer-Snapshot (axis_stats[19][8] + seg_ns[19]/Pfad B + Meta)
    // aus dem EINEN tier_observe. Trägt Observer-Stats UND das Pfad-B-Per-Achsen-Timing (reale Komposition) in EINEM
    // POD — die maßgebliche CSV-Quelle.
    anatomy::ComdareTierObserverSnapshot unified{};
    bool         unified_real = false;
};

/// (A)+(B) Treibt ein geladenes IObservableTier (SearchAlgorithm-Mess-Pfad): ZUERST tier_clear() (frischer
/// Zustand → kein kumulatives Artefakt), pre-Observe (Baseline der absoluten Zähler), n_ops insert + n_ops
/// lookup UNTER steady_clock-Messung (total_ns), post-Observe, und bildet die result_ingest-Zeile aus dem
/// DELTA (post − pre) der getriebenen Zähler. Der host-/cluster-seitige Unikat-Mess-Lauf je Binary.
[[nodiscard]] inline PermResult run_observable_perm(anatomy::IObservableTier& tier,
                                                    std::string const& binary_id, std::uint64_t n_ops) {
    // (A) Reset: frischer Datenstruktur-Zustand. tier_clear() ruft jetzt search_organ_.reset() (I-B.1) → auch die
    // T0-search-Statistik wird je Messung genullt → KEIN post−pre-Delta mehr nötig (axis_stats warmup-frei aus
    // EINEM Post-Observe). Die result_ingest-Zeile entsteht unten aus dem EINEN konsolidierten POD (volle Matrix).
    tier.tier_clear();

    // (B) Gesamt-Wall-Clock um die GANZE Mess-Last (insert + lookup).
    auto const t0 = std::chrono::steady_clock::now();
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)tier.tier_insert(i, i * 7u + 1u);
    for (std::uint64_t i = 0; i < n_ops; ++i) { std::uint64_t v = 0; (void)tier.tier_lookup(i, &v); }
    auto const t1 = std::chrono::steady_clock::now();

    PermResult r;
    r.total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    r.n_ops   = n_ops;
    // KONSOLIDIERUNG (I1): den EINEN konsolidierten Snapshot ziehen (axis_stats + Pfad-B-seg_ns in EINEM POD).
    // Der EINE tier_observe hält intern die fixe Q1-Sequenz (axis_stats-READ → seg_ns-Timing → per-op-Reset) → keine
    // Doppelzählung. Da tier_clear() jetzt search_organ_.reset() ruft, sind ALLE 19 Achsen pro Zeile warmup-frei
    // (kein post−pre-Delta nötig): die auto-gekoppelten Instanz-Organe (T1/T2/T3/T7/T8/T10/T17/T18) werden in
    // tier_clear() statistik-genullt, die Scan-Achsen (T4/T5/T9/T11..T16) sind in fill_observer_v3 idempotent
    // (reset()+scan je Observe), T0 search_algo + T6 allocator werden über search_organ_.reset() frisch.
    tier.tier_observe(&r.unified);
    r.unified_real = true;
    // KONSOLIDIERUNG (I-B.3): die result_ingest-Zeile aus dem EINEN POD = volle Matrix (axis_stats + seg_ns + Meta).
    r.line = format_perm_result(binary_id, r.unified);
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
