// run_lazy_150 — L-LAZY-E2E (gate-frei, 2026-06-03): das HOST-EXECUTABLE des Lazy-E2E-Treibers für den ≥150-Lauf.
// Baut den ExperimentTree aus dem FullPilot (320 reale SA-Kompositionen ≥150) + dyn. Dimensionen, ruft
// run_lazy_static_then_dynamic (3 Lazy-Iteratoren: statische Kompilierung → laden → dyn-Variation → messen →
// ingest) für die ersten -MaxBinaries Blätter und schreibt eine CSV. RESUMIERBAR über build_version (.version-
// Sidecar je DLL): ein erneuter Lauf überspringt versions-aktuelle DLLs (überlebt Absturz/Teil-Lauf).
//
// Die CompileFn baut mit den MESS-Defines (COMDARE_MEASUREMENT_ON …) + dem Include-Satz aus COMDARE_PILOT_INCLUDES
// (';'-getrennt, vom Harness gesetzt). vcvars64 muss aktiv sein (cl im PATH).
//
// argv: run_lazy_150 <out_csv> <max_binaries> <n_ops> <build_version> <src_dir> <dll_dir> [min_free_gb] [cores_per_build]

#include "lazy_pilot_engine.hpp"                         // FullPilot/build_pilot_levels/make_pilot_source_gen
#include <builder/build_orchestrator/system_ram.hpp>     // make_system_free_ram_fn (real)

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace fs  = std::filesystem;

static std::vector<std::string> env_includes() {
    std::vector<std::string> inc;
    char const* e = std::getenv("COMDARE_PILOT_INCLUDES");
    if (e == nullptr) return inc;
    std::string s = e, cur;
    for (char c : s) { if (c == ';') { if (!cur.empty()) inc.push_back(cur); cur.clear(); } else cur += c; }
    if (!cur.empty()) inc.push_back(cur);
    return inc;
}

int main(int argc, char** argv) {
    if (argc < 7) {
        std::cerr << "usage: run_lazy_150 <out_csv> <max_binaries> <n_ops> <build_version> <src_dir> <dll_dir>"
                     " [min_free_gb] [cores_per_build]\n";
        return 2;
    }
    std::string const out_csv      = argv[1];
    std::size_t const max_binaries = std::strtoull(argv[2], nullptr, 10);
    std::uint64_t const n_ops      = std::strtoull(argv[3], nullptr, 10);
    std::string const build_version = argv[4];
    fs::path const src_dir = argv[5];
    fs::path const dll_dir = argv[6];
    double const min_free_gb = (argc >= 8) ? std::strtod(argv[7], nullptr) : 0.0;
    std::size_t const cpb    = (argc >= 9) ? std::strtoull(argv[8], nullptr, 10) : 4;

    std::vector<std::string> const defs = {
        "/DCOMDARE_ANATOMY_MODULE_BUILD=1", "/DCOMDARE_MEASUREMENT_ON=1", "/DCOMDARE_CE_ENABLE_STATISTICS=1",
        "/DCOMDARE_EXPERIMENT_MODE_ON=1",   "/DCOMDARE_OS_WINDOWS=1",     "/DCOMDARE_ARCH_X86_64=1",
        "/DCOMDARE_CACHE_LINE_SIZE=64",     "/DWIN32",                    "/D_WINDOWS"
    };
    std::vector<std::string> const incs = env_includes();

    // Baum: FullPilot statische Achsen (320 ≥150) + dyn. Dimensionen (thread_count × prefetch_distance = 6).
    auto factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(tlz::build_pilot_levels<tlz::FullPilot>(/*with_dynamic=*/true));

    std::cout << "FullPilot::Engine::count() = " << tlz::FullPilot::Engine::count()
              << "  tree.binary_count() = " << tree.binary_count()
              << "  dyn_dims = " << tree.dynamic_filter().size()
              << "  max_binaries = " << max_binaries << "\n";

    // Selektion = die ersten N View-Indizes (lazy: select_explicit, NIE die ganze ∏-View materialisieren).
    std::size_t const N = (std::min)(max_binaries, tree.binary_count());
    std::vector<std::size_t> first_n; first_n.reserve(N);
    for (std::size_t i = 0; i < N; ++i) first_n.push_back(i);
    ex::BuildSelection const sel = ex::select_explicit(first_n);

    ex::CompileFn compile = [defs, incs](ex::BuildJob const& job) -> int {
        std::string cmd = "cl /nologo /std:c++latest /EHsc /O2 /LD /MP" + std::to_string(job.cores);
        for (auto const& d : defs) cmd += " " + d;
        for (auto const& i : incs) cmd += " /I\"" + i + "\"";
        cmd += " \"" + job.source.string() + "\"";
        cmd += " /Fe:\"" + job.output.string() + "\"";
        cmd += " /Fo:\"" + job.output.string() + ".obj\"";
        cmd += " > \"" + job.output.string() + ".cl.log\" 2>&1";
        return std::system(cmd.c_str());
    };
    ex::SourceGenFn gen = tlz::make_pilot_source_gen<tlz::FullPilot>();
    ex::FreeRamFn   ram = ex::make_system_free_ram_fn();

    ex::LazyRunConfig cfg;
    cfg.max_binaries   = N;
    cfg.n_ops          = n_ops;
    cfg.build_version  = build_version;
    cfg.source_dir     = src_dir;
    cfg.output_dir     = dll_dir;
    cfg.cores_per_build = cpb;
    cfg.env_limits.thread_count = 16;   // System-Obergrenze für die dyn. thread_count-Variation (clamp)
    if (min_free_gb > 0.0) {
        // RAM-Admission: je Build min_free_gb als Budget annehmen (grobe Reserve; mind. 1 Build läuft immer).
        cfg.ram_per_build_bytes     = static_cast<std::uint64_t>(min_free_gb * 1024.0 * 1024.0 * 1024.0);
        cfg.ram_safety_margin_bytes = cfg.ram_per_build_bytes;  // halte zusätzlich 1 Budget frei
    }

    std::cout << "\n=== run_lazy_static_then_dynamic (baut die ersten " << N
              << " DLLs real [resumierbar], dann dyn-Loop + Messung) ===\n";
    ex::LazyRunResult const r = ex::run_lazy_static_then_dynamic(tree, sel, compile, gen, ram, cfg);

    std::cout << "  selected=" << r.selected << " built=" << r.built << " (new=" << r.built_new
              << " skip=" << r.built_skip << ") loaded=" << r.loaded << " load_failed=" << r.load_failed
              << " measured=" << r.measured << " dyn_settings_total=" << r.dynamic_settings_total
              << " min_free_ram_MB=" << (r.min_free_ram_bytes / (1024 * 1024)) << "\n";

    // CSV: eine Zeile je (Binary × dyn-Setting). Header analog thesis_measurements.csv + setting-Spalte.
    std::ofstream csv{out_csv, std::ios::trunc};
    csv << "binary_id;setting;n_ops;search_lookup;hit;miss;insert;erase;peak;"
           "bytes_alloc;bytes_in_use;alloc_cnt;dealloc_cnt;fail;obs_axes;fill;applied_axes\n";
    for (auto const& row : r.csv_rows) {
        auto const& o = row.observer;
        csv << row.binary_id << ';' << (row.setting_label.empty() ? "-" : row.setting_label) << ';' << n_ops << ';'
            << o.search_lookup_count << ';' << o.search_hit_count << ';' << o.search_miss_count << ';'
            << o.search_insert_count << ';' << o.search_erase_count << ';' << o.search_peak_occupancy << ';'
            << o.alloc_bytes_allocated << ';' << o.alloc_bytes_in_use << ';' << o.alloc_allocation_count << ';'
            << o.alloc_deallocation_count << ';' << o.alloc_failure_count << ';' << o.observable_axis_count << ';'
            << o.tier_fill_level << ';' << row.applied_axis_count << '\n';
    }
    std::cout << "CSV geschrieben: " << out_csv << "  (" << r.csv_rows.size() << " Zeilen)\n";

    // Exit 0 = mind. 1 (Binary × Setting) real gebaut+gemessen+ingestet (echte Werte > 0 erwartet).
    return (r.measured > 0) ? 0 : 1;
}
