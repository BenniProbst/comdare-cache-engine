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
#include <builder/anatomy_commands/tier_observe_trace_abi.hpp>   // detail::two_phase_measure + abi_dur_ns

#include "workload_config.hpp"
#include "workload_generator.hpp"

#include <chrono>
#include <cstdint>
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
    std::uint64_t             read_sink = 0;        ///< Anti-Wegoptimierungs-Senke (gemessene Lookups)
    an::ComdareTierObserverSnapshotV1 observer{};   ///< EIN Observer-POD am Lauf-Ende (korreliert)
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
                     std::string_view profile_name) {
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
[[nodiscard]] inline std::vector<WorkloadRunResult>
run_measurement_plan(an::IObservableTier& tier,
                     an::IRollbackableTier* rollback,
                     MeasurementPlan const& plan) {
    std::vector<WorkloadRunResult> results;
    results.reserve(plan.profiles.size());
    for (auto const& cfg : plan.profiles) {
        if (!cfg.is_valid()) continue;                 // ungültiges Profil überspringen (Robustheit)
        tier.tier_clear();                              // frischer Start je Profil
        WorkloadGenerator gen{cfg};                     // dieselbe Config+Seed ⇒ identische Sequenz je Binary
        auto const ops = gen.generate_all();
        results.push_back(run_workload_profile(tier, rollback, ops, cfg.name));
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

}  // namespace comdare::cache_engine::builder::workload_driver
