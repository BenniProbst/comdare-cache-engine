// test_lazy_static_dynamic_driver — L-LAZY-E2E (gate-frei, 2026-06-03): verifiziert die EINE Host-Treiber-Funktion
// run_lazy_static_then_dynamic END-TO-END auf einem KLEINEN Pilot (SmallPilot = 4 reale SA-Binaries × 6 dyn-Settings):
//   (1) Haupt/statisch-Iterator  → DLLs real gebaut (cl /LD, reale Anatomie via Pilot-Source-Map, resumierbar),
//   (2) je DLL geladen           → IObservableTier + IResourceControllableTier (dynamic_cast),
//   (3) gefiltert-dynamisch-Iter → RuntimeVariableLoop variiert thread_count×prefetch_distance auf der DLL,
//   je Setting gemessen (Observer REAL, >0) + via result_ingest in den Baum (sparse NodeValue) ge-ingestet.
//
// LITERAL-ASSERTIONS: gebaut == N, geladen == N, je Binary dyn_settings >= 1, gemessene Observer-Summe > 0,
// measured_node_count == N×(#dyn-Settings), und JEDER gemessene Baum-Knoten hat observer_real==true + Werte>0.
//
// Der CompileFn baut mit den MESS-Defines (COMDARE_MEASUREMENT_ON etc.) + dem Include-Satz aus der Umgebung
// (COMDARE_PILOT_INCLUDES, ';'-getrennt — vom Harness/CTest gesetzt). OHNE COMDARE_MEASUREMENT_ON gäbe es kein
// IObservableTier (Mess-Build-Pflicht, thesis_tiere-Lektion). vcvars64 muss aktiv sein (cl im PATH).
//
// Build: pwsh tests/unit/thesis_tiere/build_and_measure_150_tiere.ps1 -RunTest   (oder analoger cl-Aufruf).

#include "thesis_tiere/lazy_pilot_engine.hpp"   // FullPilot/SmallPilot/build_pilot_levels/make_pilot_source_gen

#include <builder/build_orchestrator/system_ram.hpp>   // free physical RAM (real)

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace fs  = std::filesystem;

static int g_fail = 0;
template <class A, class B>
static void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) { std::cout << "  (erwartet: " << want << ")"; ++g_fail; }
    std::cout << "\n";
}
static void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n"; if (!c) ++g_fail;
}

// ── Include-Satz aus der Umgebung (vom Harness gesetzt) → /I-Flags ──
static std::vector<std::string> env_includes() {
    std::vector<std::string> inc;
    char const* e = std::getenv("COMDARE_PILOT_INCLUDES");
    if (e == nullptr) return inc;
    std::string s = e, cur;
    for (char c : s) { if (c == ';') { if (!cur.empty()) inc.push_back(cur); cur.clear(); } else cur += c; }
    if (!cur.empty()) inc.push_back(cur);
    return inc;
}

int main() {
    std::cout << "=== L-LAZY-E2E Treiber-Test (SmallPilot, 4 Binaries × 6 dyn-Settings) ===\n";

    // Mess-Defines: OHNE diese baut die DLL ohne IObservableTier (thesis_tiere abi_adapter.hpp:89 #if COMDARE_MEASUREMENT_ON).
    std::vector<std::string> const defs = {
        "/DCOMDARE_ANATOMY_MODULE_BUILD=1", "/DCOMDARE_MEASUREMENT_ON=1", "/DCOMDARE_CE_ENABLE_STATISTICS=1",
        "/DCOMDARE_EXPERIMENT_MODE_ON=1",   "/DCOMDARE_OS_WINDOWS=1",     "/DCOMDARE_ARCH_X86_64=1",
        "/DCOMDARE_CACHE_LINE_SIZE=64",     "/DWIN32",                    "/D_WINDOWS"
    };
    std::vector<std::string> const incs = env_includes();
    std::cout << "Include-Dirs (env): " << incs.size() << "\n";

    // ── (1) Treiber-Eingaben: Baum (static + dynamic), Selektion (erste 4), reale Source-Map, CompileFn, RAM ──
    fs::path const work = fs::temp_directory_path() / "comdare_lazy_e2e_test";
    fs::path const srcDir = work / "src", outDir = work / "dll";

    auto factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(tlz::build_pilot_levels<tlz::SmallPilot>(/*with_dynamic=*/true));

    std::cout << "SmallPilot::Engine::count() = " << tlz::SmallPilot::Engine::count()
              << "  tree.binary_count() = " << tree.binary_count()
              << "  dyn_dims = " << tree.dynamic_filter().size() << "\n";
    check_eq("tree.binary_count() == SmallPilot::count()", tree.binary_count(), tlz::SmallPilot::Engine::count());

    std::size_t const N = tree.binary_count();   // 4
    std::vector<std::size_t> first_n; for (std::size_t i = 0; i < N; ++i) first_n.push_back(i);
    ex::BuildSelection const sel = ex::select_explicit(first_n);

    // Reale CompileFn: cl /LD /MP<cores> + Mess-Defines + env-Includes; perm_<id>.cpp → perm_<id>.dll.
    ex::CompileFn compile = [defs, incs](ex::BuildJob const& job) -> int {
        std::string cmd = "cl /nologo /std:c++latest /EHsc /LD /MP" + std::to_string(job.cores);
        for (auto const& d : defs) cmd += " " + d;
        for (auto const& i : incs) cmd += " /I\"" + i + "\"";
        cmd += " \"" + job.source.string() + "\"";
        cmd += " /Fe:\"" + job.output.string() + "\"";
        cmd += " /Fo:\"" + job.output.string() + ".obj\"";
        cmd += " > \"" + job.output.string() + ".cl.log\" 2>&1";
        return std::system(cmd.c_str());
    };
    ex::SourceGenFn gen = tlz::make_pilot_source_gen<tlz::SmallPilot>();
    ex::FreeRamFn   ram = ex::make_system_free_ram_fn();   // reale OS-RAM-Abfrage (system_ram.hpp)

    ex::LazyRunConfig cfg;
    cfg.max_binaries   = N;
    cfg.n_ops          = 2000;
    cfg.build_version  = "lazy-e2e-test-v3-measupgrade";   // v3: Mess-Umbau (seg-Timer/Delta-Reset → DLL-Source neu)
    cfg.per_binary_subdirs = true;   // (E): je Tier-Binary ein eigener Ordner unter outDir/<stem>/
    cfg.source_dir     = srcDir;
    cfg.output_dir     = outDir;
    cfg.cores_per_build = 4;
    cfg.env_limits.thread_count = 8;   // System-Obergrenze für die dyn. thread_count-Variation (clamp)

    // ── (Audit K8) Resume-Stamp-Härtung: env_limits + XML-Lastprofil-INHALT invalidieren den Stamp ──────────
    // Beweis, dass ein Lauf mit anderen Resource-Caps ODER geändertem XML-Inhalt (GLEICHE Profil-id) NICHT mehr
    // fälschlich resumed wird (sonst würde eine stale Messung als gültig übernommen). Stamp-Format = resume-v2.
    {
        namespace wd = comdare::cache_engine::builder::workload_driver;
        std::vector<ex::DynamicDim> const no_dims;
        std::string const base = ex::lazy_resume_stamp_prefix(cfg, no_dims);
        check_true("K8: Stamp-Format = resume-v2", base.rfind("resume-v2|", 0) == 0);
        ex::LazyRunConfig cfg_env = cfg; cfg_env.env_limits.prefetch_distance = 7;
        check_true("K8: env_limits-Aenderung invalidiert den Stamp",
                   ex::lazy_resume_stamp_prefix(cfg_env, no_dims) != base);
        ex::LazyRunConfig cfg_x1 = cfg; { wd::WorkloadConfig c{}; c.pct_insert = 0.5; c.pct_lookup = 0.5; cfg_x1.workload_configs["LP01"] = c; }
        ex::LazyRunConfig cfg_x2 = cfg; { wd::WorkloadConfig c{}; c.pct_insert = 0.9; c.pct_lookup = 0.1; cfg_x2.workload_configs["LP01"] = c; }
        std::string const sx1 = ex::lazy_resume_stamp_prefix(cfg_x1, no_dims);
        check_true("K8: XML-Profil-id im Stamp (wlcfg)", sx1.find("LP01:") != std::string::npos);
        check_true("K8: XML-Inhalts-Aenderung (gleiche id) invalidiert den Stamp",
                   sx1 != ex::lazy_resume_stamp_prefix(cfg_x2, no_dims));
        std::cout << "  [K8] Resume-Stamp-Haertung verifiziert (env_limits + XML-Inhalt + resume-v2).\n";
    }

    // Vorab-Diagnose: Source-Gen liefert für das erste Blatt reale Anatomie? (flush vor dem Build-Loop)
    {
        auto v0 = tree.static_binary_view();
        std::string const s0 = (v0.size() > 0) ? gen(v0[0].binary_id) : std::string{};
        std::cout << "Source-Gen[0]: " << s0.size() << " B  enthält ADHOC-Makro: "
                  << (s0.find("COMDARE_DEFINE_ANATOMY_MODULE_ADHOC") != std::string::npos ? "ja" : "NEIN")
                  << std::endl;
    }

    std::cout << "\n=== run_lazy_static_then_dynamic (baut " << N << " DLLs real, dann dyn-Loop + Messung) ==="
              << std::endl;
    ex::LazyRunResult const r = ex::run_lazy_static_then_dynamic(tree, sel, compile, gen, ram, cfg);

    // Build-Diagnose: erste Build-Result-Stati + Existenz von src/dll des ersten Blatts.
    std::cout << "DIAG build_stats: total=" << r.build_stats.total_jobs << " ok=" << r.build_stats.succeeded
              << " fail=" << r.build_stats.failed << " peak_conc=" << r.build_stats.peak_concurrency << "\n";
    {
        std::error_code ec;
        std::cout << "DIAG source_dir exists=" << fs::exists(cfg.source_dir, ec)
                  << " files=" << (fs::exists(cfg.source_dir, ec)
                        ? std::distance(fs::directory_iterator(cfg.source_dir), fs::directory_iterator{}) : 0) << "\n";
        std::cout << "DIAG output_dir exists=" << fs::exists(cfg.output_dir, ec)
                  << " files=" << (fs::exists(cfg.output_dir, ec)
                        ? std::distance(fs::directory_iterator(cfg.output_dir), fs::directory_iterator{}) : 0) << "\n";
    }

    // ── (2) LITERAL-Verifikation der Kette ──
    std::cout << "\n--- Ergebnis ---\n";
    std::cout << "  selected=" << r.selected << " built=" << r.built << " (new=" << r.built_new
              << " skip=" << r.built_skip << ") loaded=" << r.loaded << " load_failed=" << r.load_failed
              << " measured=" << r.measured << " dyn_settings_total=" << r.dynamic_settings_total << "\n";

    check_eq("selektiert == N", r.selected, N);
    check_eq("gebaut == N (alle DLLs bereitgestellt)", r.built, N);
    check_eq("geladen == N (alle als IObservableTier nutzbar)", r.loaded, N);
    check_eq("load_failed == 0", r.load_failed, std::size_t{0});

    // (D, KF-10): 3 (thread_count) × 2 (prefetch_distance) × 3 (repetition, Default) = 18 Settings je Binary.
    std::size_t const expected_settings = 18;   // 3 × 2 × n_repeats(=3)
    check_eq("dyn_settings_total == N × 18", r.dynamic_settings_total, N * expected_settings);
    check_eq("measured == N × 18 (jede Binary × dyn-Setting × Rep ge-ingestet)", r.measured, N * expected_settings);
    check_eq("measured_node_count (sparse Baum) == measured", tree.measured_node_count(), r.measured);

    // Observer REAL + Werte > 0 (sonst „Nullen", d.h. kein Mess-Build): Summe über alle Zeilen.
    std::uint64_t sum_lookup = 0, sum_insert = 0; std::size_t rows_real = 0, rows_pos = 0;
    for (auto const& row : r.csv_rows) {
        sum_lookup += row.observer.search_lookup_count;
        sum_insert += row.observer.search_insert_count;
        ex::NodeValue const nv = tree.node_value(row.setting_id);
        if (nv.observer_real) ++rows_real;
        if (nv.observer.search_lookup_count > 0 || nv.observer.search_insert_count > 0) ++rows_pos;
    }
    std::cout << "  Observer-Summen: lookup=" << sum_lookup << " insert=" << sum_insert
              << "  rows_real=" << rows_real << " rows_pos=" << rows_pos << "\n";
    check_true("Observer search_insert_count Summe > 0 (echte Messung, keine Nullen)", sum_insert > 0);
    check_eq("alle gemessenen Knoten observer_real==true", rows_real, r.csv_rows.size());
    check_eq("alle gemessenen Knoten haben Observer-Werte > 0", rows_pos, r.csv_rows.size());

    // Ein paar Beispiel-Zeilen literal ausgeben.
    std::cout << "\n--- Beispiel-Mess-Zeilen (binary_id#setting → lookup/insert/fill) ---\n";
    for (std::size_t i = 0; i < r.csv_rows.size() && i < 6; ++i) {
        auto const& row = r.csv_rows[i];
        std::cout << "  " << row.setting_id << "  -> lookup=" << row.observer.search_lookup_count
                  << " insert=" << row.observer.search_insert_count
                  << " fill=" << row.observer.tier_fill_level
                  << " applied_axes=" << row.applied_axis_count << "\n";
    }

    std::cout << "\n==== L-LAZY-E2E Treiber-Test: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
