// run_lazy_150 — L-LAZY-E2E (gate-frei, 2026-06-03): das HOST-EXECUTABLE des Lazy-E2E-Treibers für den ≥150-Lauf.
// Baut den ExperimentTree aus dem FullPilot (320 reale SA-Kompositionen ≥150) + dyn. Dimensionen, ruft
// run_lazy_static_then_dynamic (3 Lazy-Iteratoren: statische Kompilierung → laden → dyn-Variation → messen →
// ingest) für die ersten -MaxBinaries Blätter und schreibt eine CSV. RESUMIERBAR über build_version (.version-
// Sidecar je DLL): ein erneuter Lauf überspringt versions-aktuelle DLLs (überlebt Absturz/Teil-Lauf).
//
// Die CompileFn baut mit den MESS-Defines (COMDARE_MEASUREMENT_ON …) + dem Include-Satz aus COMDARE_PILOT_INCLUDES
// (';'-getrennt, vom Harness gesetzt). vcvars64 muss aktiv sein (cl im PATH).
//
// argv: run_lazy_150 <out_csv> <max_binaries> <n_ops> <build_version> <src_dir> <dll_dir> [min_free_gb] [cores_per_build] [n_repeats] [select_mode]
//
// SELEKTIONS-MODI (2026-06-04, adversarial-Fix): die DEMONSTRATION der per-Achsen-Differenzierung braucht ≥3
// VERSCHIEDENE search_algo-Werte in der CSV. Im Default-Index-Modus ("index": select_explicit({0..N-1})) variiert
// search_algo NICHT (Ebene 0 = höchstwertig → die ersten N Blätter teilen denselben search_algo). Daher ein
// zweiter, KONFIGURIERBARER Modus "search_algo_grid" (argv[10] ODER env COMDARE_SELECT_MODE): F15-Grid, das NUR die
// search_algo-Ebene (Ebene 0) variiert und ALLE übrigen statischen Ebenen auf Index 0 pinnt → reine search_algo-
// Wirkung, je search_algo genau EIN Binary. Die Mess-/Reps-/Ordner-Infrastruktur bleibt UNVERÄNDERT (nur die
// View-Index-Auswahl ändert sich). Der bisherige Index-Modus bleibt Default (rückwärtskompatibel).
//
// MESS-ARCHITEKTUR-UMBAU (2026-06-04): die CSV trägt jetzt total_ns + ns_per_op (B/C-1), 19 echte per-Segment-ns
// (X, ALLE SearchAlgorithm-Achsen T0..T18 — kein n/a mehr; die frühere na_axes-Notiz-Spalte ist entfallen),
// eine repetition-Spalte (D, je Rep eine eigene Roh-Zeile, Default 3 via [n_repeats]) und die Observer-DELTA-
// Counter (A, kein kumulatives Artefakt). seg_*-Spalten = n/a nur, falls eine DLL kein IMeasurableWorkloadV3 trägt.
// Die perm-DLLs + Source + .obj + .cl.log + .version + per-Binary-result.csv liegen je Binary in einem eigenen
// Unterordner unter dll_dir/<stem>/ (E, per_binary_subdirs).

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

// F15-Grid (2026-06-04): die EINE search_algo-variierende, alle-anderen-statisch-pinnende Selektion. Setzt die
// search_algo-Ebene (Ebene 0, höchstwertig) auf 0..K-1 und ALLE übrigen statischen Ebenen auf Index 0 → je
// search_algo GENAU ein Binary, identische sonstige Achsen (reine search_algo-Wirkung; analog dem ABI-Test). Reine
// Wiederverwendung der bestehenden StaticBinaryView::flat_index/level_size — KEINE neue Baum-/Mess-Logik. Liefert
// bis zu `cap` View-Indizes. Pinnt search_algo (Ebene 0) NICHT (Fanout K) → die binary_ids tragen search_algo als
// freie Achse; jeder Index ist ein gültiges View-Tupel (∈ FullPilot-Source-Map → baubar).
static ex::BuildSelection select_search_algo_grid(ex::StaticBinaryView const& view, std::size_t cap) {
    ex::BuildSelection sel;
    sel.provenance = "search_algo_grid";
    if (view.level_count() == 0) return sel;
    std::size_t const k_search = view.level_size(0);              // Anzahl search_algo-Varianten (Ebene 0)
    std::vector<std::size_t> tuple(view.level_count(), 0);        // alle übrigen Ebenen auf Index 0 gepinnt
    std::size_t const n = (std::min)(cap, k_search);
    sel.indices.reserve(n);
    for (std::size_t s = 0; s < n; ++s) {
        tuple[0] = s;                                            // NUR die search_algo-Ebene variieren
        sel.indices.push_back(view.flat_index(tuple));
    }
    return sel;
}

int main(int argc, char** argv) {
    if (argc < 7) {
        std::cerr << "usage: run_lazy_150 <out_csv> <max_binaries> <n_ops> <build_version> <src_dir> <dll_dir>"
                     " [min_free_gb] [cores_per_build] [n_repeats] [select_mode=index|search_algo_grid]\n";
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
    // (D, KF-10): Wiederholungen je (Binary×Setting), Default 3, konfigurierbar via argv[9].
    std::uint32_t const n_repeats = (argc >= 10)
        ? static_cast<std::uint32_t>(std::strtoul(argv[9], nullptr, 10)) : 3u;
    // Selektions-Modus (2026-06-04): "index" (Default, rückwärtskompatibel) ODER "search_algo_grid" (F15-Grid,
    // variiert NUR search_algo). Quelle: argv[10], sonst env COMDARE_SELECT_MODE, sonst "index".
    std::string select_mode = (argc >= 11) ? std::string{argv[10]} : std::string{};
    if (select_mode.empty()) { char const* e = std::getenv("COMDARE_SELECT_MODE"); if (e != nullptr) select_mode = e; }
    if (select_mode.empty()) select_mode = "index";

    // Achse 2 (#135, 2026-06-08): Lastprofile = Werte der dynamischen Workload-Achse. PRIMÄR via XML-Discovery aus
    // COMDARE_LOAD_PROFILE_DIR (runtime-interpretierte comdare_load_profile-XMLs → WorkloadConfig-Registry; op-mix/
    // dist/negative_query_pct aus dem XML). FALLBACK: COMDARE_WORKLOADS env-String (hartcodierte make_* via profile_by_name).
    namespace wd = ::comdare::cache_engine::builder::workload_driver;
    std::vector<std::string> workload_values;
    std::map<std::string, wd::WorkloadConfig> workload_registry;
    if (char const* lpd = std::getenv("COMDARE_LOAD_PROFILE_DIR"); lpd != nullptr && *lpd != '\0') {
        for (auto const& idp : wd::discover_load_profiles(lpd)) {
            if (auto lp = wd::parse_load_profile(idp.second)) {
                workload_registry[idp.first] = lp->config;
                workload_values.push_back(idp.first);
            }
        }
        std::cout << "  load_profiles (XML, Achse 2) entdeckt: " << workload_values.size() << " aus " << lpd << "\n";
    }
    if (workload_values.empty()) {   // Fallback: env-String (hartcodierte Profile via profile_by_name)
        if (char const* w = std::getenv("COMDARE_WORKLOADS"); w != nullptr) {
            std::string const s{w};
            std::size_t b = 0;
            while (b <= s.size()) {
                std::size_t const e = s.find(',', b);
                std::string tok = s.substr(b, (e == std::string::npos ? s.size() : e) - b);
                while (!tok.empty() && tok.front() == ' ') tok.erase(tok.begin());
                while (!tok.empty() && tok.back()  == ' ') tok.pop_back();
                if (!tok.empty()) workload_values.push_back(tok);
                if (e == std::string::npos) break;
                b = e + 1;
            }
        }
    }

    std::vector<std::string> const defs = {
        "/DCOMDARE_ANATOMY_MODULE_BUILD=1", "/DCOMDARE_MEASUREMENT_ON=1", "/DCOMDARE_CE_ENABLE_STATISTICS=1",
        "/DCOMDARE_EXPERIMENT_MODE_ON=1",   "/DCOMDARE_OS_WINDOWS=1",     "/DCOMDARE_ARCH_X86_64=1",
        "/DCOMDARE_CACHE_LINE_SIZE=64",     "/DWIN32",                    "/D_WINDOWS"
    };
    std::vector<std::string> const incs = env_includes();

    // Baum: FullPilot statische Achsen (320 ≥150) + dyn. Dimensionen (thread_count × prefetch_distance ×
    // repetition = 3·2·n_repeats Settings je Binary). Die repetition-Achse (D) erzeugt je Wiederholung eine
    // EIGENE setting_id → je Rep eine eigene Roh-CSV-Zeile (nie interpoliert).
    auto factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(tlz::build_pilot_levels<tlz::FullPilot>(/*with_dynamic=*/true, n_repeats, workload_values));

    std::cout << "FullPilot::Engine::count() = " << tlz::FullPilot::Engine::count()
              << "  tree.binary_count() = " << tree.binary_count()
              << "  dyn_dims = " << tree.dynamic_filter().size()
              << "  n_repeats = " << n_repeats
              << "  max_binaries = " << max_binaries
              << "  select_mode = " << select_mode << "\n";
    {
        std::cout << "  workloads (Achse 2) = " << workload_values.size() << " [";
        for (std::size_t i = 0; i < workload_values.size(); ++i) std::cout << (i ? "," : "") << workload_values[i];
        std::cout << "]\n";
    }

    // Selektion: ZWEI Modi (Mess-/Reps-/Ordner-Infra IDENTISCH; nur die View-Index-Auswahl unterscheidet sich).
    //   • "search_algo_grid" (F15): variiert NUR die search_algo-Ebene (Ebene 0), alle übrigen statisch gepinnt →
    //     je search_algo GENAU ein Binary; die CSV zeigt die per-search_algo-Differenzierung (Demonstration).
    //   • "index" (Default): die ersten N View-Indizes (select_explicit) — search_algo konstant über die ersten N
    //     (Ebene 0 höchstwertig). Rückwärtskompatibel.
    ex::StaticBinaryView const view = tree.static_binary_view();
    std::size_t const N = (std::min)(max_binaries, tree.binary_count());
    ex::BuildSelection sel;
    if (select_mode == "search_algo_grid") {
        sel = select_search_algo_grid(view, N);          // je search_algo ein Binary (≤ K_search Binaries)
    } else {
        std::vector<std::size_t> first_n; first_n.reserve(N);
        for (std::size_t i = 0; i < N; ++i) first_n.push_back(i);
        sel = ex::select_explicit(std::move(first_n));   // ersten N (Default-Index-Modus)
    }
    std::cout << "Selektion: provenance=" << sel.provenance << "  indices=" << sel.size() << "\n";

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
    // Achse 2 (INC-3c): YCSB-Load-Records (Sätze, die VOR der gemessenen Run-Phase befüllt werden) aus env
    // COMDARE_WORKLOAD_RECORDS; 0/ungesetzt → records = n_ops. Key-Verteilung wird auf [1, records] ausgerichtet.
    if (char const* lr = std::getenv("COMDARE_WORKLOAD_RECORDS"); lr != nullptr)
        cfg.workload_records = std::strtoull(lr, nullptr, 10);
    cfg.workload_configs = std::move(workload_registry);   // Achse 2 (#135): XML-Lastprofil-Registry → Iterator
    cfg.build_version  = build_version;
    cfg.source_dir     = src_dir;
    cfg.output_dir     = dll_dir;
    cfg.cores_per_build = cpb;
    cfg.per_binary_subdirs = true;      // (E): je Tier-Binary ein eigener Ordner unter dll_dir/<stem>/
    cfg.n_repeats          = n_repeats; // (D): dokumentiert/durchgereicht (Wirkung über die Baum-repetition-Dim)
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

    // CSV: eine Zeile je (Binary × dyn-Setting × Rep). EINHEITLICHES Schema (lazy_csv_header/format_csv_row,
    // identisch zur per-Binary-result.csv): total_ns + ns_per_op (B/C-1), 4 echte per-Segment-ns (C-2),
    // repetition-Spalte (D), Observer-DELTA-Counter (A), na_axes-Notiz für die 15 passiven Achsen.
    std::ofstream csv{out_csv, std::ios::trunc};
    csv << ex::lazy_csv_header();
    for (auto const& row : r.csv_rows) csv << ex::format_csv_row(row);
    std::cout << "CSV geschrieben: " << out_csv << "  (" << r.csv_rows.size() << " Zeilen)\n";

    // Exit 0 = mind. 1 (Binary × Setting) real gebaut+gemessen+ingestet (echte Werte > 0 erwartet).
    return (r.measured > 0) ? 0 : 1;
}
