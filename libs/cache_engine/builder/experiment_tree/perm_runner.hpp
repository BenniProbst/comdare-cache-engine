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
//       Observer-Werte als DELTA (post − pre) — die Statistik-Zähler waren 2026-06-04 per ABI nicht resetbar,
//       die Delta-Bildung eliminierte das kumulative Artefakt (search_lookup 2000→4000→…). Seit #216-H2
//       (ABI Major 4) ergänzt der virtuelle Reset-Punkt tier_reset_statistics() diesen Delta-Ansatz.
//   (B) Gesamt-Wall-Clock: steady_clock um insert+lookup → total_ns je Messung (Host-Messung, KEINE
//       Baum-Knoten-Eigenschaft → reist NUR über PermResult/LazyMeasuredRow in die CSV, NICHT über ingest).
//   (X) Echter per-Segment-Timer auf ALLE 19 Achsen: drive_segment_latencies() ruft das ABI-Sub-Interface
//       IMeasurableWorkloadV3 (run_workload_segmented_v2) → 19 aufsummierte per-Achsen-ns (T0..T18). KEINE Achse
//       n/a mehr (jede treibt eine reale, strategie-abhängige Op); nur eine DLL OHNE V3-Interface → CSV n/a.

#include "experiment_tree.hpp"               // NodeObserverSnapshot
#include "../../anatomy/observable_tier.hpp" // IObservableTier + ComdareTierObserverSnapshot (I1: EINE Schnittstelle/EIN POD)
#include "../../anatomy/measurable_workload.hpp" // Pfad A: IMeasurableWorkloadV3 + ComdareSegmentLatencyV2 (19 Segmente)
#include "../../anatomy/rollbackable_tier.hpp" // Achse 2 (INC-1): IRollbackableTier (Zwei-Phasen-Cache-Warmup, PFLICHT für Gültigkeit)
#include "../../anatomy/scannable_tier.hpp" // Achse 2 (INC-1): IScannableTier (YCSB-E Range-Scan)
// Achse 2 (INC-1): run_workload_profile-Op-Skript-Runner (generischer CS-Interpreter über den flachen Op-Vektor — KEIN GoF-Interpreter mit Grammatik/AST) + WorkloadGenerator.
#include "../workload_driver/workload_orchestrator.hpp" // Achse 2 (INC-1): run_workload_profile-Interpreter + WorkloadGenerator
#include "../workload_driver/workload_profiles.hpp"   // Achse 2 (INC-1): Single-Source profile_by_name (Fallback)
#include "../workload_driver/load_profile_parser.hpp" // Achse 2 (#135): XML-Lastprofil-Registry (id → WorkloadConfig)
#include "../pruef_dock/conformance_gate.hpp" // (Audit K9 / V5-I4): Konformitäts-Gate VOR der Messung (import→GATE→messen)
#include "../pmc_source_factory.hpp" // #156-De-Risk: make_pmc_source() (IPmcSource/PmcCounters) — PMC in den WIDE-Mess-Pfad

#include <array> // GOAL-L1: kOpKindNames + PermResult::op_lat (per-Interface-Funktions-Latenzen)
#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

/// KONSOLIDIERUNG (I-B.3, 2026-06-04, User-Fork „voll auf axis_stats[19][8]"): formatiert den EINEN konsolidierten
/// Observer-POD als result_ingest-Zeile = binary_id + die VOLLE Matrix: axis_stats[19][8] (152) + seg_ns[19] (19)
/// + Meta (observable_axis_count, tier_fill_level, filled_axis_count, batches_measured) = 175 Felder. EXAKT das von
/// ingest_result_line erwartete Format → Round-Trip-Garant (Cluster: perm_runner→Zeile→ingest→Baum-NodeValue).
/// MAJOR-MESS-09 (Mess-Validität, Audit A1): binary_id ist das ERSTE ';'-Feld der result_ingest-Zeile (= Baum-Key).
/// Enthielte es selbst ein ';' oder einen Zeilenumbruch, verschöbe es beim Re-Parse ALLE 175 nachfolgenden Felder
/// (axis_stats/seg_ns landeten im falschen Slot) — eine stille Mess-Verfälschung. Da der Wert ein Datei-Stem/Label ist
/// (keine ';'/Newline by-construction), lehnen wir einen verletzenden Wert HART ab (leere Zeile → ingest verwirft sie),
/// statt zu escapen: das hält das ';'-Wire-Format 1:1 feldzählbar (exaktes ==176, s. ingest_result_line).
[[nodiscard]] inline bool binary_id_is_wire_safe(std::string_view binary_id) noexcept {
    return !binary_id.empty() && binary_id.find(';') == std::string_view::npos &&
           binary_id.find('\n') == std::string_view::npos && binary_id.find('\r') == std::string_view::npos;
}

[[nodiscard]] inline std::string format_perm_result(std::string const&                          binary_id,
                                                    anatomy::ComdareTierObserverSnapshot const& s) {
    if (!binary_id_is_wire_safe(binary_id)) return std::string{}; // ungültige ID → leere Zeile (ingest verwirft)
    std::string out  = binary_id;
    auto        addu = [&](std::uint64_t v) {
        out += ';';
        out += std::to_string(v);
    };
    auto addi = [&](std::int64_t v) {
        out += ';';
        out += std::to_string(v);
    };
    for (std::size_t t = 0; t < 19; ++t)
        for (std::size_t f = 0; f < 8; ++f) addu(s.axis_stats[t][f]);
    for (std::size_t t = 0; t < 19; ++t) addi(s.seg_ns[t]);
    addu(s.observable_axis_count);
    addu(s.tier_fill_level);
    addu(s.filled_axis_count);
    addu(s.batches_measured);
    // P-MD3 (2026-06-18): die 2 additiven Coverage-Versöhnungs-Meta-Felder hinten anhängen → Wire-Format 175→177
    // Felder (binary_id + 177 = 178 total). Round-Trip-Garant mit ingest_result_line (exaktes ==178).
    addi(s.seg_framework_ns);
    addi(s.seg_run_total_ns);
    return out;
}

// ── GOAL-M/L1 (2026-06-12): per-Interface-Funktions-Latenzen für die Konfig×Tier-Auswertung ───────────
/// Aggregat der getimten Latenzen EINER Interface-Funktion (Op-Art) dieser Messung: Sample-Zahl +
/// Nearest-Rank-Perzentile (konsistent zu serialize_workload_run_results_csv). n==0 = Op-Art im Profil
/// nicht vorhanden (bzw. Scan auf nicht-scanbarem Tier ehrlich übersprungen).
struct OpKindLatency {
    std::uint64_t n      = 0;
    std::int64_t  p50_ns = 0;
    std::int64_t  p99_ns = 0;
};
/// Single-Source der Op-Art-Reihenfolge (CSV-Spalten ↔ PermResult::op_lat ↔ WorkloadRunResult-Vektoren).
inline constexpr std::array<char const*, 6> kOpKindNames{"insert", "lookup", "erase", "clear", "scan", "rmw"};

// ── (B): Ergebnis einer Mess-Last (result_ingest-Zeile + die Host-Wall-Clock) ─────────────────────────
struct PermResult {
    std::string   line;         // result_ingest-Zeile (binary_id + volle Observer-Matrix)
    std::int64_t  total_ns = 0; // (B) steady_clock-Wall-Clock-Summe ALLER getimten Ops DIESER Messung
    std::uint64_t n_ops    = 0; // Eingabe-n_ops (Skala des Profils)
    // GOAL-M1.1 (Audit K2): Anzahl der TATSÄCHLICH getimten Einzel-Ops → ns_per_op = total_ns/timed_ops.
    // Workload-Pfad: Σ der per-Op-Art-Samples (deckt Scan-Skips ehrlich ab); Legacy-Pfad: 2*n_ops
    // (n_ops Inserts + n_ops Lookups). Ersetzt den fehlerhaften fixen 2*n_ops-Divisor für Profil-Zeilen.
    std::uint64_t timed_ops = 0;
    // GOAL-L1: per-Interface-Funktions-Latenzen (Reihenfolge kOpKindNames) — die z-Achsen-Quelle der
    // 3D-Diagramme (Verarbeitungsdauer je Testdatensatz-Operation, getrennt je Interface-Funktion).
    std::array<OpKindLatency, 6> op_lat{};
    // KONSOLIDIERUNG (I1): der EINE konsolidierte Observer-Snapshot (axis_stats[19][8] + seg_ns[19]/Pfad B + Meta)
    // aus dem EINEN tier_observe. Trägt Observer-Stats UND das Pfad-B-Per-Achsen-Timing (reale Komposition) in EINEM
    // POD — die maßgebliche CSV-Quelle.
    anatomy::ComdareTierObserverSnapshot unified{};
    bool                                 unified_real = false;
    // #156-De-Risk (2026-06-20): die HW-Performance-Counter (PMC) DIESER Messung = Delta über den getimten Batch.
    // EINMAL pro Treiber-Lauf via make_pmc_source() erzeugt (KEIN Hot-Loop-Overhead), begin() unmittelbar VOR
    // t0=steady_clock, end() unmittelbar NACH t1. Default = PmcCounters{} (alle 0, available=false) → lokal mit
    // NullPmcSource (COMDARE_ENABLE_PMC=OFF) ehrlich 0/available=0; mit Intel-PCM (Montag, Linux/Win+Treiber) real.
    builder::PmcCounters pmc{};
    // Achse 2 (INC-1): Lastprofil-Metadaten + Mess-GÜLTIGKEIT.
    std::string profile_name{};          // Lastprofil-Name (z.B. "YCSB_C_read_only"); leer = alter fixer Workload
    bool        two_phase_valid = false; // Zwei-Phasen-Cache-Warmup aktiv+empirisch-exakt → Messung GÜLTIG
                                         // (false = ungültig: KEINE stille Kalt-Messung als gültiges Ergebnis)
    // (Audit K9 / V5-I4) Konformitäts-Gate-Ergebnis (import→GATE→messen): die Hülle wurde VOR der Messung gegen
    // std::map<uint64,uint64> als Oracle getrieben. conformance_passed=false ⇒ NICHT std::map-konform → es gibt
    // KEINE gültige Performance-Zeile (gated≠gültig; der Mess-Block wird übersprungen, Zeile = genullte Matrix).
    // Reist host-seitig (wie two_phase_valid), NICHT im 175-Feld-Wire-Format → Round-Trip unverändert.
    bool          conformance_passed       = false; ///< cases_total>0 && alle Zusicherungen bestanden
    std::uint64_t conformance_cases_total  = 0;     ///< Anzahl geprüfter Einzel-Zusicherungen (0 = Gate nicht gelaufen)
    std::uint64_t conformance_cases_passed = 0;     ///< davon bestanden
    std::uint64_t conformance_first_fail   = 0;     ///< 1-basierter Index der ersten Verletzung (0 = keine)
};

// ── (Audit K9 / V5-I4) Konformitäts-Gate VOR der Messung — Reihenfolge bindend: import → GATE → (nur bei pass) messen ──
/// Treibt das funktionale IDriveableTier-Sub-Interface (IMMER vorhanden, auch in Release-/funktional-only-DLLs) gegen
/// std::map<uint64,uint64> als Oracle (Randfälle RF1–7 + 2000 deterministische Zufalls-Ops). Schreibt die Konformitäts-
/// Quoten in r. passed()=false ⇒ die Hülle ist NICHT std::map-konform → der Aufrufer überspringt die Messung.
inline void apply_conformance_gate_(anatomy::IDriveableTier& tier, PermResult& r) noexcept {
    auto const cg              = ::comdare::cache_engine::builder::pruef_dock::run_conformance_gate(tier);
    r.conformance_cases_total  = cg.cases_total;
    r.conformance_cases_passed = cg.cases_passed;
    r.conformance_first_fail   = cg.first_fail;
    r.conformance_passed       = cg.passed();
}
/// Gate-Fehlschlag: ehrliche genullte Matrix-Zeile, KEINE Performance-Messung (gated≠gültig). two_phase_valid/unified_real
/// bleiben false → der Iterator emittiert KEINE gültige Mess-Zeile für eine nicht-konforme Hülle.
[[nodiscard]] inline PermResult gate_failed_result_(PermResult r, std::string const& binary_id) {
    r.two_phase_valid = false;
    r.unified_real    = false;
    r.line            = format_perm_result(binary_id, r.unified); // genullter POD → ehrlich „nicht gemessen"
    return r;
}

/// (A)+(B) Treibt ein geladenes IObservableTier (SearchAlgorithm-Mess-Pfad): ZUERST tier_clear() (frischer
/// Zustand → kein kumulatives Artefakt), pre-Observe (Baseline der absoluten Zähler), n_ops insert + n_ops
/// lookup UNTER steady_clock-Messung (total_ns), post-Observe, und bildet die result_ingest-Zeile aus dem
/// DELTA (post − pre) der getriebenen Zähler. Der host-/cluster-seitige Unikat-Mess-Lauf je Binary.
/// #156-De-Risk (2026-06-20): `pmc` ist die EINE, vom Aufrufer pro Treiber-Lauf via make_pmc_source() erzeugte
/// HW-Counter-Quelle (Strategy: NullPmcSource/WindowsPcmPmcSource hinter IPmcSource). nullptr = kein PMC (Default
/// → r.pmc bleibt 0/available=false, exakt das alte Verhalten; KEIN Mess-Overhead). begin()/end() klammern NUR
/// den getimten Batch (NICHT je Op), parallel zur steady_clock-Wall-Clock.
[[nodiscard]] inline PermResult run_observable_perm(anatomy::IObservableTier& tier, std::string const& binary_id,
                                                    std::uint64_t n_ops, builder::IPmcSource* pmc = nullptr) {
    PermResult r;
    // (Audit K9 / V5-I4) import → GATE → (nur bei pass) messen: nicht-std::map-konforme Hüllen erzeugen KEINE gültige
    // Performance-Zeile. Das Gate leert das Tier selbst (RF1+RF7) → der folgende tier_clear() ist idempotent.
    apply_conformance_gate_(tier, r);
    if (!r.conformance_passed) return gate_failed_result_(std::move(r), binary_id);

    // (A) Reset: frischer Datenstruktur-Zustand. tier_clear() ruft jetzt container_algorithm_.reset() (I-B.1) → auch die
    // T0-search-Statistik wird je Messung genullt → KEIN post−pre-Delta mehr nötig (axis_stats warmup-frei aus
    // EINEM Post-Observe). Die result_ingest-Zeile entsteht unten aus dem EINEN konsolidierten POD (volle Matrix).
    tier.tier_clear();

    // (B) Gesamt-Wall-Clock um die GANZE Mess-Last (insert + lookup).
    // #156-De-Risk: PMC begin() UNMITTELBAR VOR t0, end() UNMITTELBAR NACH t1 → das Counter-Delta deckt exakt den
    // getimten Batch (kein Hot-Loop-Overhead). pmc==nullptr → r.pmc bleibt Default (0/available=false).
    if (pmc != nullptr) pmc->begin();
    auto const t0 = std::chrono::steady_clock::now();
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)tier.tier_insert(i, i * 7u + 1u);
    for (std::uint64_t i = 0; i < n_ops; ++i) {
        std::uint64_t v = 0;
        (void)tier.tier_lookup(i, &v);
    }
    auto const t1 = std::chrono::steady_clock::now();
    if (pmc != nullptr) r.pmc = pmc->end();

    r.total_ns  = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    r.n_ops     = n_ops;
    r.timed_ops = 2u * n_ops; // GOAL-M1.1: Legacy-Fix-Workload = n_ops Inserts + n_ops Lookups getimt
    // KONSOLIDIERUNG (I1): den EINEN konsolidierten Snapshot ziehen (axis_stats + Pfad-B-seg_ns in EINEM POD).
    // Der EINE tier_observe hält intern die fixe Q1-Sequenz (axis_stats-READ → seg_ns-Timing → per-op-Reset) → keine
    // Doppelzählung. Da tier_clear() jetzt container_algorithm_.reset() ruft, sind ALLE 19 Achsen pro Zeile warmup-frei
    // (kein post−pre-Delta nötig): die auto-gekoppelten Instanz-Organe (T1/T2/T3/T7/T8/T10/T17/T18) werden in
    // tier_clear() statistik-genullt, die Scan-Achsen (T4/T5/T9/T11..T16) sind in fill_observer_v3 idempotent
    // (reset()+scan je Observe), T0 search_algo + T6 allocator werden über container_algorithm_.reset() frisch.
    tier.tier_observe(&r.unified);
    r.unified_real = true;
    // KONSOLIDIERUNG (I-B.3): die result_ingest-Zeile aus dem EINEN POD = volle Matrix (axis_stats + seg_ns + Meta).
    r.line = format_perm_result(binary_id, r.unified);
    return r;
}

// ── Achse 2 (INC-1): Lastprofil-Mess-Lauf über den BEREITS implementierten Op-Skript-Runner (generischer CS-Interpreter über den flachen Op-Vektor — KEIN GoF-Interpreter mit Grammatik/AST) ──
namespace wd  = ::comdare::cache_engine::builder::workload_driver;
namespace acd = ::comdare::cache_engine::builder::anatomy_commands::detail;

/// run_workload_perm — treibt EIN Lastprofil (workload_id) über `workload_driver::run_workload_profile` statt
/// des hartgecodeten insert/lookup-Loops (run_observable_perm). Die Op-Sequenz wird host-seitig reproduzierbar
/// materialisiert (gleiche Config+Seed ⇒ bit-identische Sequenz über alle Binaries). Jede gemessene Op läuft
/// über den Zwei-Phasen-Cache-Warmup (save→warmup→rollback→measure), der für die Mess-GÜLTIGKEIT PFLICHT ist
/// ([[feedback_two_phase_warmup_mandatory_validity]]): Ohne exakten Rollback ist die Messung UNGÜLTIG — wir
/// markieren das (two_phase_valid=false) und erzeugen KEINE stille Kalt-Messung als gültiges Ergebnis.
/// Unbekanntes Profil → Rückwärts-Kompat-Fallback auf den alten fixen Workload (run_observable_perm).
[[nodiscard]] inline PermResult run_workload_perm(anatomy::IObservableTier& tier, anatomy::IRollbackableTier* rollback,
                                                  anatomy::IScannableTier* scan, std::string const& binary_id,
                                                  std::string_view workload_id, std::uint64_t n_ops, std::uint64_t seed,
                                                  std::uint64_t                                    load_records = 0,
                                                  std::map<std::string, wd::WorkloadConfig> const* registry = nullptr,
                                                  builder::IPmcSource*                             pmc      = nullptr) {
    // Achse 2 (#135): workload_id zuerst aus der XML-Lastprofil-Registry (Charakteristik aus dem XML); sonst aus
    // dem hartcodierten profile_by_name (env-String-Rückwärts-Kompat). Skala (records/n_ops) setzt der Aufrufer.
    wd::WorkloadConfig cfg{};
    bool               resolved = false;
    if (registry != nullptr) {
        auto const it = registry->find(std::string(workload_id));
        if (it != registry->end()) {
            cfg      = it->second;
            resolved = true;
        }
    }
    if (!resolved) {
        cfg = wd::profile_by_name(workload_id, seed, static_cast<std::size_t>(n_ops));
        if (cfg.name.empty())
            return run_observable_perm(tier, binary_id, n_ops, pmc); // unbekannt → alter fixer Workload
    }
    cfg.num_operations = static_cast<std::size_t>(n_ops);
    // YCSB Load-/Run-Phasen-Trennung (INC-3c): `records` Sätze werden VOR der gemessenen Run-Phase befüllt (load),
    // und die Key-Verteilung wird auf [1, records] ausgerichtet → Lookups/Scans treffen befüllte Keys. Ohne diese
    // Load-Phase wären read-heavy/scan-Profile (C/E) auf leerem Tier 100% Miss = ungültig. Default records = n_ops.
    std::uint64_t const records = (load_records > 0) ? load_records : n_ops;
    cfg.key_min                 = 1;
    cfg.key_max                 = (records > 1) ? records : 2; // key_max>key_min Pflicht (WorkloadConfig::is_valid)

    PermResult r;
    r.profile_name = std::string(workload_id); // schon hier (Gate-Früh-Return + CSV nutzen den Achsen-Wert)
    // (Audit K9 / V5-I4) import → GATE → (nur bei pass) messen: nicht-std::map-konforme Hülle erzeugt KEINE gültige
    // Performance-Zeile (gated≠gültig). Das Gate läuft VOR Load-/Run-Phase (es leert das Tier selbst).
    apply_conformance_gate_(tier, r);
    if (!r.conformance_passed) return gate_failed_result_(std::move(r), binary_id);

    // GÜLTIGKEIT: Zwei-Phasen-Warmup Pflicht. Rollback-Exaktheit (Adapter-strukturell) auf leerem Tier prüfen (billig).
    tier.tier_clear();
    bool const rb_exact = (rollback != nullptr) && acd::rollback_is_empirically_exact(tier, rollback);
    r.two_phase_valid   = rb_exact;

    // MINOR-MESS-02 (Mess-Validität, Audit A1): LOAD + Run liegen NACH der bestandenen rb_exact-Probe. Wirft eine
    // Kapsel-Op hier (z.B. std::bad_alloc/OOM beim Befüllen großer `records` oder beim CoW-Memento), so wäre der bis
    // dahin gesetzte r.two_phase_valid=rb_exact eine GEFÄLSCHTE Gültigkeit über einer abgebrochenen Mess-Last
    // (Teil-/Nullzähler). Darum: die ganze Mess-Region kapseln und bei JEDER Ausnahme die Messung ehrlich als
    // UNGÜLTIG markieren (two_phase_valid=false, genullte Matrix-Zeile) statt sie als valide einzuspielen.
    // (Hinweis: ein vom Tier INTERN via catch(...) verschluckter OOM bleibt eine ehrliche Restlimitierung — s.
    //  le_limitierung-Zeile „Kapsel-internes OOM"; diese Schranke deckt den nach außen propagierten Fall ab.)
    try {
        // LOAD-Phase (UNGEMESSEN): records Sätze einfügen → befülltes Tier für die gemessene Run-Phase (YCSB-Load).
        tier.tier_clear();
        for (std::uint64_t i = 1; i <= records; ++i) (void)tier.tier_insert(i, i * 7u + 1u);
        // #216-H2: Observer-Statistik nach Load nullen (daten-erhaltend) -> axis_stats misst nur die Run-Phase; Wall-Clock/PMC klammern weiterhin nur die Run-Phase.
        tier.tier_reset_statistics();

        wd::WorkloadGenerator             gen{cfg}; // gleiche Config+Seed ⇒ bit-identische Op-Sequenz je Binary
        std::vector<wd::WorkloadOp> const ops = gen.generate_all();
        // #156-De-Risk: PMC klammert NUR die gemessene Run-Phase (run_workload_profile) — die LOAD-Phase oben ist
        // ungemessen, also bewusst AUSSERHALB des begin()/end()-Intervalls. EINMAL pro Treiber-Lauf erzeugte Quelle.
        if (pmc != nullptr) pmc->begin();
        wd::WorkloadRunResult const res =
            wd::run_workload_profile(tier, rb_exact ? rollback : nullptr, ops, cfg.name, scan);
        if (pmc != nullptr) r.pmc = pmc->end();

        // total_ns + GOAL-M1.1/L1: per-Op-Art-Aggregate (n/p50/p99, Reihenfolge kOpKindNames) + timed_ops als
        // Σ der Samples — die korrekte ns_per_op-Basis (Audit K2: der fixe 2*n_ops-Divisor galt nur dem Legacy-Pfad).
        std::int64_t                                          total = 0;
        std::array<std::vector<std::int64_t> const*, 6> const per_kind{&res.insert_ns, &res.lookup_ns, &res.erase_ns,
                                                                       &res.clear_ns,  &res.scan_ns,   &res.rmw_ns};
        for (std::size_t k = 0; k < per_kind.size(); ++k) {
            auto const& v = *per_kind[k];
            for (std::int64_t ns : v) total += ns;
            r.op_lat[k] = OpKindLatency{static_cast<std::uint64_t>(v.size()), acd::nearest_rank_p(v, 0.5),
                                        acd::nearest_rank_p(v, 0.99)};
            r.timed_ops += static_cast<std::uint64_t>(v.size());
        }

        r.total_ns     = total;
        r.n_ops        = res.op_count;
        r.unified      = res.observer; // EIN konsolidierter Observer-POD (axis_stats[19][8]+seg_ns[19])
        r.unified_real = true;
        r.line = format_perm_result(binary_id, res.observer); // profile_name bereits oben gesetzt (Gate-Früh-Return)
    } catch (...) {
        // Kapsel-OOM/Exception NACH bestandener rb_exact-Probe: KEINE gültige Mess-Zeile. Ehrlich entwerten.
        r.two_phase_valid = false;
        r.unified         = anatomy::ComdareTierObserverSnapshot{};
        r.unified_real    = false;
        r.op_lat          = std::array<OpKindLatency, 6>{};
        r.total_ns        = 0;
        r.timed_ops       = 0;
        r.line            = format_perm_result(binary_id, r.unified); // genullter POD → ehrlich „nicht gemessen"
    }
    return r;
}

/// (X) Treibt — falls das geladene Modul IMeasurableWorkloadV3 exponiert — den 19-Segment-Workload und liefert
/// die ECHT gemessenen, über die Batches aufsummierten per-Achsen-ns ALLER 19 SearchAlgorithm-Achsen
/// (T0..T18, kein n/a mehr). `tier` ist das via dynamic_cast erhaltene Sub-Interface (nullptr → out bleibt 0,
/// → CSV ehrlich n/a). ops_per_batch/batches sind die Mess-Parameter; seed deterministisch. Gibt batches_measured (>0 = real).
[[nodiscard]] inline std::uint64_t drive_segment_latencies(anatomy::IMeasurableWorkloadV3* tier,
                                                           std::uint64_t ops_per_batch, std::uint64_t batches,
                                                           std::uint64_t seed, anatomy::ComdareSegmentLatencyV2& out) {
    out = anatomy::ComdareSegmentLatencyV2{};
    if (tier == nullptr) return 0; // alte/ohne-V3 DLL → ehrlich n/a (out bleibt 0)
    return tier->run_workload_segmented_v2(ops_per_batch, batches, seed, &out);
}

} // namespace comdare::cache_engine::builder::experiment
