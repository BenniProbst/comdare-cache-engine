#pragma once
// V5-I9 — WorkloadOrchestrator: host-seitiger GENERISCHER Lastprofil-Treiber über das Gattungs-ABI.
//
// User-Direktive 2026-05-31 (Mess-Architektur, GRUNDMODELL Host-Seite): „IMeasurableWorkload rein generisch
// host-seitig … mehrere Lastprofile je Binary … run_workload-in-DLL war V3-Designfehler → host-seitig
// relokalisieren." Dieser Orchestrator treibt eine reproduzierbare WorkloadConfig-Op-Sequenz AUSSCHLIESSLICH
// über IObservableTier (das Gattungs-ABI) — die Last lebt HOST-seitig, NICHT in der Tier-Binary. Jede
// gemessene Op läuft über den V5-I7-Zwei-Phasen-Treiber (save→warmup→rollback→measure), wenn die geladene
// Binary IRollbackableTier anbietet (sonst Kalt-Messung).
//
// Lastenprofil (eine der zwei Haupt-Experiment-Achsen, ⊥ Build-Profil): WorkloadConfig liefert Testdaten-
// Range + Operationsabläufe (Op-Mix) + Umfang (num_operations); mehrere Profile je Binary über MeasurementPlan.
//
// @doku docs/architecture/messarchitektur_v5_design.md §2 (Host-Seite) + §5 (3 Profile)
// @related [[feedback_hybrid_search_engine_interface]] [[feedback_pruefling_vs_pruefling_binary]]

#include <anatomy/observable_tier.hpp>
#include <anatomy/rollbackable_tier.hpp>
#include <anatomy/scannable_tier.hpp>                            // V5-#49-E: Range-Scan-Sub-Interface (YCSB-E)
#include <builder/anatomy_commands/tier_observe_trace_abi.hpp>   // detail::two_phase_measure + abi_dur_ns

#include "workload_config.hpp"
#include "workload_generator.hpp"

#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::workload_driver {

namespace an = ::comdare::cache_engine::anatomy;
namespace ac = ::comdare::cache_engine::builder::anatomy_commands;

// ─────────────────────────────────────────────────────────────────────────────
// WorkloadRunResult — Mess-Ergebnis EINES Lastprofils über EIN Tier
// ─────────────────────────────────────────────────────────────────────────────

/// Roh-Wall-Clock-ns je Op-Kind (GETRENNT, Doku 24 §2.1) + Wall-Clock-korrelierter Observer-POD am Ende.
struct WorkloadRunResult {
    std::string               profile_name{};
    std::uint64_t             op_count   = 0;
    bool                      two_phase  = false;   ///< true = Rollback aktiv (sonst Kalt-Messung)
    std::vector<std::int64_t> insert_ns{};
    std::vector<std::int64_t> lookup_ns{};
    std::vector<std::int64_t> erase_ns{};
    std::vector<std::int64_t> clear_ns{};
    std::vector<std::int64_t> scan_ns{};            ///< V5-#49-E: Range-Scan-Latenzen (leer wenn Tier nicht scanbar)
    std::vector<std::int64_t> rmw_ns{};             ///< V5-#49-F: Read-Modify-Write-Latenzen
    std::uint64_t             read_sink = 0;        ///< Anti-Wegoptimierungs-Senke (gemessene Lookups + Scans)
    an::ComdareTierObserverSnapshot observer{};   ///< EIN konsolidierter Observer-POD am Lauf-Ende (korreliert, I1)
};

// ─────────────────────────────────────────────────────────────────────────────
// run_workload_profile — generischer Host-Treiber EINES Lastprofils
// ─────────────────────────────────────────────────────────────────────────────

/// Treibt die (vorgenerierte, reproduzierbare) Op-Sequenz `ops` über das Tier; jede gemessene Op via
/// Zwei-Phasen-Treiber (V5-I7) wenn `rollback != nullptr`. `ops` wird typischerweise via
/// `WorkloadGenerator(config).generate_all()` erzeugt — dieselbe Config+Seed ⇒ identische Sequenz über alle
/// Binaries (Reproduzierbarkeit-Pflicht). Latenzen werden je Op-Kind getrennt gesammelt.
[[nodiscard]] inline WorkloadRunResult
run_workload_profile(an::IObservableTier& tier,
                     an::IRollbackableTier* rollback,
                     std::vector<WorkloadOp> const& ops,
                     std::string_view profile_name,
                     an::IScannableTier* scan = nullptr) {   // V5-#49-E: optional (alte DLLs → nullptr → Scan-Ops übersprungen)
    using clock = std::chrono::steady_clock;
    WorkloadRunResult r;
    r.profile_name = std::string(profile_name);
    r.op_count     = ops.size();
    r.two_phase    = (rollback != nullptr);

    for (auto const& op : ops) {
        switch (op.kind) {
            case WorkloadOpKind::Insert: {
                auto const ns = ac::detail::two_phase_measure(rollback, [&]() -> std::int64_t {
                    auto const t0 = clock::now();
                    (void)tier.tier_insert(op.key, op.value);
                    auto const t1 = clock::now();
                    return ac::detail::abi_dur_ns(t0, t1);
                });
                r.insert_ns.push_back(ns);
                break;
            }
            case WorkloadOpKind::Lookup: {
                std::uint64_t measured_out = 0;
                bool          measured_hit = false;
                auto const ns = ac::detail::two_phase_measure(rollback, [&]() -> std::int64_t {
                    std::uint64_t out = 0;
                    auto const t0 = clock::now();
                    bool const hit = tier.tier_lookup(op.key, &out);
                    auto const t1 = clock::now();
                    measured_hit = hit; measured_out = out;   // spiegelt nach two_phase_measure die MESS-Phase
                    return ac::detail::abi_dur_ns(t0, t1);
                });
                r.read_sink += measured_hit ? measured_out : 0u;
                r.lookup_ns.push_back(ns);
                break;
            }
            case WorkloadOpKind::Erase: {
                auto const ns = ac::detail::two_phase_measure(rollback, [&]() -> std::int64_t {
                    auto const t0 = clock::now();
                    (void)tier.tier_erase(op.key);
                    auto const t1 = clock::now();
                    return ac::detail::abi_dur_ns(t0, t1);
                });
                r.erase_ns.push_back(ns);
                break;
            }
            case WorkloadOpKind::Clear: {
                auto const ns = ac::detail::two_phase_measure(rollback, [&]() -> std::int64_t {
                    auto const t0 = clock::now();
                    tier.tier_clear();
                    auto const t1 = clock::now();
                    return ac::detail::abi_dur_ns(t0, t1);
                });
                r.clear_ns.push_back(ns);
                break;
            }
            case WorkloadOpKind::Scan: {   // V5-#49-E: Range-Scan ab op.key über op.value (=scan_length) Records
                if (scan != nullptr) {
                    std::uint64_t scan_sum = 0;
                    auto const ns = ac::detail::two_phase_measure(rollback, [&]() -> std::int64_t {
                        std::uint64_t cs = 0;
                        auto const t0 = clock::now();
                        (void)scan->tier_scan(op.key, op.value, &cs);
                        auto const t1 = clock::now();
                        scan_sum = cs;   // spiegelt nach two_phase_measure die MESS-Phase
                        return ac::detail::abi_dur_ns(t0, t1);
                    });
                    r.read_sink += scan_sum;
                    r.scan_ns.push_back(ns);
                }
                // scan == nullptr → Tier nicht scanbar (alte DLL / Release): Op ehrlich übersprungen (kein Fake-Sample).
                break;
            }
            case WorkloadOpKind::ReadModifyWrite: {   // V5-#49-F: lookup → modifizieren → upsert als EINE Op
                auto const ns = ac::detail::two_phase_measure(rollback, [&]() -> std::int64_t {
                    std::uint64_t cur = 0;
                    auto const t0 = clock::now();
                    bool const hit = tier.tier_lookup(op.key, &cur);              // READ
                    std::uint64_t const modified = (hit ? cur : 0u) ^ op.value;   // MODIFY (deterministisch)
                    (void)tier.tier_insert(op.key, modified);                     // WRITE (ComposedSearch-Upsert)
                    auto const t1 = clock::now();
                    return ac::detail::abi_dur_ns(t0, t1);
                });
                r.rmw_ns.push_back(ns);
                break;
            }
        }
    }

    tier.tier_observe(&r.observer);   // EIN korrelierter Observer-POD am Lauf-Ende
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
// MeasurementPlan + run_measurement_plan — MEHRERE Lastprofile je Binary
// ─────────────────────────────────────────────────────────────────────────────

/// Mehrere Lastprofile, die nacheinander (je frischer Tier-Zustand) gegen DASSELBE geladene Binary laufen.
/// Das ist die Lastenprofil-Achse des Experiments (⊥ Build-Profil): eine Binary, N Lastprofile.
struct MeasurementPlan {
    std::vector<WorkloadConfig> profiles{};
};

/// Fährt alle Profile des Plans gegen ein Tier. Vor JEDEM Profil wird der Tier geleert (tier_clear), damit
/// die Profile unabhängig + reproduzierbar sind. Ein Ergebnis je Profil (in Plan-Reihenfolge).
///
/// **GATE-VERTRAG (V5, Aufrufer-Pflicht):** Der Aufrufer MUSS das Tier VOR dem Aufruf gegen die std::map-
/// Konformität geprüft haben (pruef_dock::run_conformance_gate, import → GATE → messen) — diese Funktion misst
/// nur Performance und prüft KEINE Funktionalität (vgl. IPruefDock::measure-Vertrag, pruef_dock.hpp).
[[nodiscard]] inline std::vector<WorkloadRunResult>
run_measurement_plan(an::IObservableTier& tier,
                     an::IRollbackableTier* rollback,
                     MeasurementPlan const& plan,
                     an::IScannableTier* scan = nullptr) {   // V5-#49-E: Scan-fähiges Tier (für YCSB-E-Profile)
    std::vector<WorkloadRunResult> results;
    results.reserve(plan.profiles.size());
    // V5-Audit-Härtung: nur zwei-phasig, wenn der Rollback EMPIRISCH exakt ist (einmalige Probe je Binary).
    if (rollback != nullptr && !ac::detail::rollback_is_empirically_exact(tier, rollback)) rollback = nullptr;
    for (auto const& cfg : plan.profiles) {
        if (!cfg.is_valid()) continue;                 // ungültiges Profil überspringen (Robustheit)
        tier.tier_clear();                              // frischer Start je Profil
        WorkloadGenerator gen{cfg};                     // dieselbe Config+Seed ⇒ identische Sequenz je Binary
        auto const ops = gen.generate_all();
        results.push_back(run_workload_profile(tier, rollback, ops, cfg.name, scan));
    }
    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// Lastprofil-Serialisierung (Persistenz / Mess-Protokoll-Header)
// ─────────────────────────────────────────────────────────────────────────────

/// serialize_workload_config — Ein-Zeilen-Klartext-Repräsentation eines Lastprofils (für Mess-Protokoll +
/// Reproduzierbarkeits-Dokumentation; bewusst ohne externe JSON-Abhängigkeit, [[feedback_no_quick_fixes]]).
/// Format: `name|seed|num_ops|key_min|key_max|pct_insert|pct_lookup|pct_erase|pct_clear`.
[[nodiscard]] inline std::string serialize_workload_config(WorkloadConfig const& c) {
    std::string s;
    s.reserve(96);
    s += std::string(c.name);                  s += '|';
    s += std::to_string(c.seed);               s += '|';
    s += std::to_string(c.num_operations);     s += '|';
    s += std::to_string(c.key_min);            s += '|';
    s += std::to_string(c.key_max);            s += '|';
    s += std::to_string(c.pct_insert);         s += '|';
    s += std::to_string(c.pct_lookup);         s += '|';
    s += std::to_string(c.pct_erase);          s += '|';
    s += std::to_string(c.pct_clear);
    return s;
}

/// serialize_workload_run_results_csv — Mess-Ergebnis-CSV des Lastprofil-Pfads (eine Zeile je Profil-Lauf).
/// Trägt je Op-Kind die Sample-Zahl + p50/p99-Perzentile (Tier-Wall-Clock, Doku 24 §2.1, GETRENNT) UND die
/// korrelierten Observer-Zähler am Lauf-Ende. `two_phase`-Spalte dokumentiert, ob zwei-phasig (Rollback aktiv)
/// gemessen wurde. Perzentile via `anatomy_commands::detail::nearest_rank_p` (Nearest-Rank, robust).
[[nodiscard]] inline std::string serialize_workload_run_results_csv(std::vector<WorkloadRunResult> const& rs) {
    std::ostringstream os;
    os << "profile,op_count,two_phase,"
          "insert_n,insert_p50_ns,insert_p99_ns,lookup_n,lookup_p50_ns,lookup_p99_ns,"
          "erase_n,erase_p50_ns,erase_p99_ns,clear_n,clear_p50_ns,clear_p99_ns,"
          "scan_n,scan_p50_ns,scan_p99_ns,rmw_n,rmw_p50_ns,rmw_p99_ns,"   // V5-#49-E/F
          "search_insert,search_lookup,search_hit,search_miss,search_erase,search_peak_occupancy,"
          "alloc_bytes_in_use,alloc_alloc_count,observable_axes\n";
    for (auto const& r : rs) {
        auto const& o = r.observer;
        os << r.profile_name << ',' << r.op_count << ',' << (r.two_phase ? 1 : 0) << ','
           << r.insert_ns.size() << ',' << ac::detail::nearest_rank_p(r.insert_ns, 0.5) << ',' << ac::detail::nearest_rank_p(r.insert_ns, 0.99) << ','
           << r.lookup_ns.size() << ',' << ac::detail::nearest_rank_p(r.lookup_ns, 0.5) << ',' << ac::detail::nearest_rank_p(r.lookup_ns, 0.99) << ','
           << r.erase_ns.size()  << ',' << ac::detail::nearest_rank_p(r.erase_ns,  0.5) << ',' << ac::detail::nearest_rank_p(r.erase_ns,  0.99) << ','
           << r.clear_ns.size()  << ',' << ac::detail::nearest_rank_p(r.clear_ns,  0.5) << ',' << ac::detail::nearest_rank_p(r.clear_ns,  0.99) << ','
           << r.scan_ns.size()   << ',' << ac::detail::nearest_rank_p(r.scan_ns,   0.5) << ',' << ac::detail::nearest_rank_p(r.scan_ns,   0.99) << ','
           << r.rmw_ns.size()    << ',' << ac::detail::nearest_rank_p(r.rmw_ns,    0.5) << ',' << ac::detail::nearest_rank_p(r.rmw_ns,    0.99) << ','
           << o.axis_stats[0][3] << ',' << o.axis_stats[0][0] << ',' << o.axis_stats[0][1] << ','
           << o.axis_stats[0][2] << ',' << o.axis_stats[0][4] << ',' << o.axis_stats[0][5] << ','
           << o.axis_stats[6][1] << ',' << o.axis_stats[6][2] << ',' << o.observable_axis_count << '\n';
    }
    return os.str();
}

}  // namespace comdare::cache_engine::builder::workload_driver
