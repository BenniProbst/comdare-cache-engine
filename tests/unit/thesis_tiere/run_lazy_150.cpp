// run_lazy_150 — L-LAZY-E2E: das HOST-EXECUTABLE des Lazy-E2E-Treibers fuer den >=150-Lauf. PROFIL-GETRIEBEN
// (STRANG A KORRIGIERT, Inc4/S5, 2026-06-18): baut den ExperimentTree DEKLARATIV aus einem comdare_thesis_profile
// (parse_thesis_profile -> build_axis_levels -> tree.build, die offizielle Kette Doc 10 §2.2), ruft
// run_lazy_static_then_dynamic (3 Lazy-Iteratoren: statische Kompilierung -> laden -> dyn-Variation -> messen ->
// ingest) fuer die profil-selektierten Blaetter und schreibt eine CSV. RESUMIERBAR ueber build_version (.version-
// Sidecar je DLL): ein erneuter Lauf ueberspringt versions-aktuelle DLLs (ueberlebt Absturz/Teil-Lauf).
//
// Die CompileFn baut mit den MESS-Defines (COMDARE_MEASUREMENT_ON …) + dem Include-Satz aus COMDARE_PILOT_INCLUDES
// (';'-getrennt, vom Harness gesetzt). vcvars64 muss aktiv sein (cl im PATH).
//
// argv: run_lazy_150 <out_csv> <max_binaries> <n_ops> <build_version> <src_dir> <dll_dir> [min_free_gb]
//       [cores_per_build] [n_repeats] [profile:<pfad>[@<achse>]] [resume=1|0]
//   • argv[10] = der PROFIL-PFAD (optional "profile:"-Praefix; optional "@<achse>" = ein im Profil als
//     <axis_sweep> DEKLARIERTER Per-Achsen-Sweep, sonst die Basis-Selektion). Alternativ env COMDARE_THESIS_PROFILE.
//
// Mess-RESUME (#139, 2026-06-11): Default AN (argv[11]=1). Binaries mit vollstaendiger+konfigurations-aktueller
// per-Binary result.csv (result.csv.stamp == aktueller Config-Stempel) werden uebersprungen; ihre Zeilen fliessen
// unveraendert in die globale CSV. Stale Ergebnisse (anderer BuildVersion/n_ops/Workload-Set) matchen nicht →
// Neu-Messung (Zwei-Phasen-Cache-Warmup intrinsisch je Op). Ueberlebt damit Reboots/Abbrueche auf Binary-Granularitaet.
//
// SELEKTION (PROFIL-GETRIEBEN, S5): die frueheren hartkodierten Code-Modi + die Selektions-Mode-Env + die
// hartkodierte Achsen->Level-Map sind ENTFERNT. Die Auswahl kommt jetzt aus tlz::profile_select: entweder die
// Basis-Selektion (ersten N View-Indizes) oder ein im Profil DEKLARIERTER Per-Achsen-Sweep. Eine nicht-deklarierte
// Achse wird verweigert (profil-getriebene Whitelist). Mess-/Reps-/Ordner-Infrastruktur unveraendert.
//
// MESS-ARCHITEKTUR-UMBAU (2026-06-04): die CSV traegt total_ns + ns_per_op (B/C-1), 19 echte per-Segment-ns
// (X, ALLE SearchAlgorithm-Achsen T0..T18 — kein n/a mehr; die fruehere na_axes-Notiz-Spalte ist entfallen),
// eine repetition-Spalte (D, je Rep eine eigene Roh-Zeile, Default 3 via [n_repeats]) und die Observer-DELTA-
// Counter (A, kein kumulatives Artefakt). seg_*-Spalten = n/a nur, falls eine DLL kein IMeasurableWorkloadV3 traegt.
// Die perm-DLLs + Source + .obj + .cl.log + .version + per-Binary-result.csv liegen je Binary in einem eigenen
// Unterordner unter dll_dir/<stem>/ (E, per_binary_subdirs). Die SourceGenFn kommt aus dem profil-agnostischen
// Anatomie-Quell-KATALOG (source_catalog.hpp, make_catalog_source_gen) — binary_id -> reale Modul-Quelle, lazy.
#include "source_catalog.hpp"                             // make_catalog_source_gen (profil-getriebene SourceGenFn)
#include "profile_runner.hpp"                             // build_profile_basis_levels / profile_select / profile_* (Inc3)
#include <builder/build_orchestrator/system_ram.hpp>     // make_system_free_ram_fn (real)

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
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
                     " [min_free_gb] [cores_per_build] [n_repeats] [profile:<pfad>[@<achse>]] [resume=1|0]\n";
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
    // STRANG A Inc4/S5 (2026-06-18): die hartkodierten Code-Selektions-Modi + die Selektions-Mode-Env sind ENTFERNT.
    // Der Treiber ist NUR noch profil-getrieben. argv[10] ist jetzt der PROFIL-PFAD (optional mit "@<achse>"-Sweep-
    // Selektor) bzw. das "profile:<pfad>"-Präfix (rückwärts-kompatibel).
    std::string profile_arg = (argc >= 11) ? std::string{argv[10]} : std::string{};
    // Mess-RESUME (#139): argv[11] (1=an Default / 0=aus → alles neu messen, Stamps werden überschrieben).
    // STRANG-A Inc3: ist argv[11] NICHT gesetzt, darf <run_options resume=..> aus dem Profil den Default liefern
    // (s.u., nach dem Profil-Parse). argv hat Vorrang (Rueckwaerts-Kompatibilitaet).
    bool resume = (argc >= 12) ? (std::strtoul(argv[11], nullptr, 10) != 0) : true;
    bool const resume_from_argv = (argc >= 12);

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
        // GOAL-M1.3 (Audit-Blocker „stiller Voll-Lauf OHNE Lastprofil-Achse"): Verzeichnis gesetzt, aber
        // keine gültigen Profile → ABBRUCH statt stillem Fallback (der fertige Ergebnisse überschreiben könnte).
        if (workload_values.empty()) {
            std::cerr << "run_lazy_150: COMDARE_LOAD_PROFILE_DIR gesetzt, aber 0 gueltige Profile in '" << lpd
                      << "' — Abbruch (Achse 2 darf nicht still entfallen).\n";
            return 4;
        }
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

    // STRANG A Inc4/S5 (2026-06-18): der Baum kommt AUSSCHLIESSLICH aus dem comdare_thesis_profile
    // (parse_thesis_profile → build_axis_levels → tree.build, die offizielle Kette Doc 10 §2.2). Der frühere
    // frueher hartkodierte Code-Pfad ist ENTFERNT (Inc4/S5). Quelle des Profil-Pfads: argv[10]
    // ("profile:<pfad>" oder direkt "<pfad>", optional "@<achse>"-Sweep-Selektor) bzw. env COMDARE_THESIS_PROFILE.
    // Die binary_ids sind golden-belegt identisch zur eingefrorenen golden_fullpilot_320_binary_ids.txt (Resume #139).
    std::string profile_path = profile_arg, profile_select_axis;
    if (profile_path.rfind("profile:", 0) == 0) profile_path = profile_path.substr(std::string("profile:").size());
    if (profile_path.empty()) { char const* e = std::getenv("COMDARE_THESIS_PROFILE"); if (e != nullptr) profile_path = e; }
    if (auto at = profile_path.rfind('@'); at != std::string::npos) {   // "<pfad>@<achse>" → Sweep-Achse
        profile_select_axis = profile_path.substr(at + 1);
        profile_path.erase(at);
    }
    // Alternativ via env (ueberschreibt das @-Suffix), z.B. fuer die PS-Sweep-Schleife.
    if (char const* e = std::getenv("COMDARE_PROFILE_SELECT"); e != nullptr && *e != '\0') profile_select_axis = e;
    if (profile_path.empty()) {
        std::cerr << "run_lazy_150: kein Profil angegeben (argv[10]=profile:<pfad> ODER env COMDARE_THESIS_PROFILE)."
                     " Der Treiber ist profil-getrieben (STRANG A Inc4/S5) — Abbruch.\n";
        return 5;
    }

    // Profil EINMAL parsen (konsumiert daraus working_set_sweep / axis_sweeps / run_options / Selektion).
    std::optional<comdare::builder::xml::ThesisProfile> const tp_opt = tlz::load_thesis_profile(profile_path);
    if (!tp_opt) { std::cerr << "run_lazy_150: Profil '" << profile_path
                         << "' nicht lesbar (parse_thesis_profile=nullopt) — Abbruch.\n"; return 5; }
    auto const& tp = *tp_opt;
    std::string const mode_name = tp.modes.empty() ? std::string{"m3v2_base"} : tp.modes.front().name;

    auto factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    // build_axis_levels emittiert ein "tier"-Level oben (base_tiers). Fuer den reinen Achsen-Permutations-Lauf
    // (Basis-320) wird es abgezogen → DERSELBE binary_id-Raum wie die GOLDEN-Liste (Resume #139 stabil).
    std::vector<ex::AxisLevel> const profile_static_basis =
        tlz::build_profile_basis_levels(tp, mode_name, /*with_dynamic=*/true);
    tree.build(profile_static_basis);

    std::cout << "PROFIL-MODUS: " << profile_path << "  (id=" << tp.id << " mode=" << mode_name
              << " select_axis=" << (profile_select_axis.empty() ? "basis" : profile_select_axis) << ")"
              << "  tree.binary_count() = " << tree.binary_count()
              << "  dyn_dims = " << tree.dynamic_filter().size()
              << "  n_repeats = " << n_repeats
              << "  max_binaries = " << max_binaries << "\n";
    {
        std::cout << "  workloads (Achse 2) = " << workload_values.size() << " [";
        for (std::size_t i = 0; i < workload_values.size(); ++i) std::cout << (i ? "," : "") << workload_values[i];
        std::cout << "]\n";
    }

    // Selektion (PROFIL-GETRIEBEN, S5): Basis-Selektion (ersten N View-Indizes) ODER ein im Profil als <axis_sweep>
    // deklarierter Per-Achsen-Sweep (variiert GENAU eine Achse gegen die Index-0-Baseline). Mess-/Reps-/Ordner-Infra
    // IDENTISCH; nur die View-Index-Auswahl unterscheidet sich. Quelle = tlz::profile_select.
    ex::StaticBinaryView const view = tree.static_binary_view();

    // STRANG-A Inc4/S5 (run_options, PROFIL-GETRIEBEN): cap/resume/platform/build_version aus <run_options> als
    // Defaults; argv/env darf weiterhin uebersteuern (Rueckwaerts-Kompatibilitaet). cap greift, wenn argv[2]=0.
    tlz::ProfileRunOptions const ro = tlz::profile_run_options(tp);
    std::size_t eff_cap = max_binaries;
    if (ro.cap > 0 && max_binaries == 0) eff_cap = ro.cap;
    if (!resume_from_argv) resume = ro.resume;
    std::size_t const N = (std::min)(eff_cap, tree.binary_count());

    // Die 4 Lauf-/Selektions-Tag-Spalten (series/sweep_axis/platform/build_version) je Mess-Zeile. series/sweep_axis
    // kommen PROFIL-GETRIEBEN aus tlz::profile_select (s.u.); platform/build_version aus <run_options> (env Vorrang).
    // working_set_n = cfg.workload_records (separate N-Sweep-Achse, s.u.). KEINE hartkodierte Selektions-Quelle mehr.
    std::string tag_series = "-", tag_sweep_axis = "-";
    std::string tag_platform      = ro.platform.empty()      ? std::string{"win-x86_64"} : ro.platform;
    std::string tag_build_version = ro.build_version.empty() ? std::string{"m3v2"}       : ro.build_version;
    if (char const* p = std::getenv("COMDARE_PLATFORM");      p != nullptr && *p != '\0') tag_platform = p;
    if (char const* bv = std::getenv("COMDARE_BUILD_VERSION"); bv != nullptr && *bv != '\0') tag_build_version = bv;

    // SELEKTION (PROFIL-GETRIEBEN, S5): entweder BASIS-320 (profile_select_axis leer) oder ein im Profil als
    // <axis_sweep> DEKLARIERTER Per-Achsen-Sweep. Der level_d wird ueber profile_axis_level aus den tier-freien
    // Basis-Levels aufgeloest (KEINE hartkodierte axis_to_level-Map; nicht-deklarierte Achse → REFUSED-Provenance).
    tlz::ProfileTaggedSelection const pts =
        tlz::profile_select(tp, profile_static_basis, view, profile_select_axis, N);
    ex::BuildSelection const sel = pts.selection;
    tag_series = pts.series; tag_sweep_axis = pts.sweep_axis;
    std::cout << "PROFIL-SELEKTION: label=" << pts.label << "  provenance=" << sel.provenance
              << "  indices=" << sel.size()
              << "  [tags: series=" << tag_series << " sweep_axis=" << tag_sweep_axis
              << " platform=" << tag_platform << " build_version=" << tag_build_version
              << " working_set_n=via-records]\n";

    ex::CompileFn compile = [defs, incs](ex::BuildJob const& job) -> int {
        // (A2.8-Fix 2026-06-13) Response-File statt Inline-cl: die 50+ Include-Dirs (generated/-Unterordner wuchsen
        // seit #138) sprengen sonst cmd.exe's 8191-Zeichen-Limit -> "Die Befehlszeile ist zu lang" -> JEDER DLL-Build
        // scheitert -> 0 Mess-Zeilen. cl @rsp hat KEIN Laengen-Limit (identisch zu den scratch_compile_*-Skripten).
        // Eine .rsp je Binary im per-Binary-Unterordner; das eigentliche system()-Kommando ist nun kurz.
        std::filesystem::path const rsp = job.output.string() + ".rsp";
        {
            std::ofstream rf{rsp};
            rf << "/nologo /std:c++latest /EHsc /O2 /LD /MP" << job.cores << "\n";
            for (auto const& d : defs) rf << d << "\n";
            for (auto const& i : incs) rf << "/I\"" << i << "\"\n";
            rf << "\"" << job.source.string() << "\"\n";
            rf << "/Fe:\"" << job.output.string() << "\"\n";
            rf << "/Fo:\"" << job.output.string() << ".obj\"\n";
        }
        std::string const cmd = "cl @\"" + rsp.string() + "\" > \"" + job.output.string() + ".cl.log\" 2>&1";
        return std::system(cmd.c_str());
    };
    // SourceGenFn = der profil-agnostische Anatomie-Quell-KATALOG (S4b): binary_id → reale Modul-Quelle. Der Treiber
    // hat den Baum + die Selektion AUS DEM PROFIL gebaut; der Katalog liefert NUR die Quelle je selektiertem binary_id
    // (kein Code-Selektor, kein String→Typ-Dispatch). Lazy-Compile (1 DLL = 1 TU) bleibt erhalten.
    ex::SourceGenFn gen = tlz::make_catalog_source_gen();
    ex::FreeRamFn   ram = ex::make_system_free_ram_fn();

    // STRANG-A Inc3 (working_set_sweep): die N-Liste der AEUSSEREN Lauf-Iteration. ERSETZT die PS-foreach
    // (build_and_measure_150_tiere.ps1:166) + COMDARE_WORKLOAD_RECORDS-env je N. Quelle:
    //   • PROFIL: <working_set_sweep>{N}</working_set_sweep> (Inc3), falls vorhanden und COMDARE_WORKLOAD_RECORDS
    //     NICHT extern gesetzt ist (env hat Vorrang fuer Rueckwaerts-Kompatibilitaet / die alte PS-Schleife).
    //   • SONST: ein einziger Pass mit COMDARE_WORKLOAD_RECORDS (0/ungesetzt → records = n_ops im Iterator).
    // Je N: ein run_lazy_static_then_dynamic-Pass mit cfg.workload_records=N → working_set_n-Tag-Spalte = N.
    // Die per-N-Zeilen werden in EINE Gesamt-CSV zusammengefuehrt (Header genau EINMAL).
    std::vector<std::uint64_t> n_sweep;
    bool const env_records_set = (std::getenv("COMDARE_WORKLOAD_RECORDS") != nullptr);
    if (!env_records_set) n_sweep = tlz::profile_working_set_sweep(tp);
    if (n_sweep.empty()) {   // kein Profil-Sweep (oder env hat Vorrang) → EIN Pass mit dem env-Wert (0=Default)
        std::uint64_t rec = 0;
        if (char const* lr = std::getenv("COMDARE_WORKLOAD_RECORDS"); lr != nullptr) rec = std::strtoull(lr, nullptr, 10);
        n_sweep.push_back(rec);   // 0 ⇒ Iterator setzt records = n_ops
    }
    std::cout << "WORKING-SET-SWEEP (aeussere Iteration): [";
    for (std::size_t i = 0; i < n_sweep.size(); ++i) std::cout << (i ? "," : "") << n_sweep[i];
    std::cout << "]  Quelle=" << ((!env_records_set && !tp.working_set_sweep.empty()) ? "profil" : "env/default") << "\n";

    std::ofstream csv{out_csv, std::ios::trunc};
    csv << ex::lazy_csv_header();          // Header GENAU EINMAL (alle N-Teile darunter)
    std::size_t total_resumed = 0, total_fresh = 0;
    std::uint64_t any_measured = 0, any_resumed = 0;

    for (std::uint64_t const ws_n : n_sweep) {
        ex::LazyRunConfig cfg;
        cfg.max_binaries   = N;
        cfg.n_ops          = n_ops;
        // Achse 2 (INC-3c): YCSB-Load-Records (Sätze, die VOR der gemessenen Run-Phase befüllt werden) =
        // der aktuelle Working-Set-N-Wert (0 ⇒ records = n_ops). Key-Verteilung wird auf [1, records] ausgerichtet.
        cfg.workload_records = ws_n;
        cfg.workload_configs = workload_registry;   // Achse 2 (#135): XML-Lastprofil-Registry → Iterator (Kopie je N)
        cfg.build_version  = build_version;
        // Die 5 Lauf-/Selektions-Tags je Mess-Zeile (CSV-Spalten series/sweep_axis/working_set_n/platform/
        // build_version). working_set_n = cfg.workload_records (= ws_n); die 4 uebrigen profil-getrieben (S5).
        cfg.row_series        = tag_series;
        cfg.row_sweep_axis    = tag_sweep_axis;
        cfg.row_platform      = tag_platform;
        cfg.row_build_version = tag_build_version;
        cfg.source_dir     = src_dir;
        cfg.output_dir     = dll_dir;
        cfg.cores_per_build = cpb;
        cfg.per_binary_subdirs = true;      // (E): je Tier-Binary ein eigener Ordner unter dll_dir/<stem>/
        cfg.resume_completed_binaries = resume;   // #139: vollständige+aktuelle Binaries überspringen (Stamp-Match)
        cfg.n_repeats          = n_repeats; // (D): dokumentiert/durchgereicht (Wirkung über die Baum-repetition-Dim)
        cfg.env_limits.thread_count = 16;   // System-Obergrenze für die dyn. thread_count-Variation (clamp)
        if (min_free_gb > 0.0) {
            // RAM-Admission: je Build min_free_gb als Budget annehmen (grobe Reserve; mind. 1 Build läuft immer).
            cfg.ram_per_build_bytes     = static_cast<std::uint64_t>(min_free_gb * 1024.0 * 1024.0 * 1024.0);
            cfg.ram_safety_margin_bytes = cfg.ram_per_build_bytes;  // halte zusätzlich 1 Budget frei
        }

        std::cout << "\n=== [working_set_n=" << ws_n << "] run_lazy_static_then_dynamic (baut die ersten " << N
                  << " DLLs real [resumierbar], dann dyn-Loop + Messung) ===\n";
        ex::LazyRunResult const r = ex::run_lazy_static_then_dynamic(tree, sel, compile, gen, ram, cfg);

        std::cout << "  selected=" << r.selected << " built=" << r.built << " (new=" << r.built_new
                  << " skip=" << r.built_skip << ") loaded=" << r.loaded << " load_failed=" << r.load_failed
                  << " measured=" << r.measured << " resumed_binaries=" << r.resumed_binaries
                  << " dyn_settings_total=" << r.dynamic_settings_total
                  << " min_free_ram_MB=" << (r.min_free_ram_bytes / (1024 * 1024)) << "\n";

        // CSV: eine Zeile je (Binary × dyn-Setting × Rep). EINHEITLICHES Schema (lazy_csv_header/format_csv_row).
        // Mess-RESUME (#139): zuerst die unverändert übernommenen Zeilen der resumierten Binaries, dann die frisch
        // gemessenen. Der Header steht bereits oben (einmal) → hier NUR die Daten-Zeilen je N-Pass.
        csv << r.resumed_csv_rows;
        for (auto const& row : r.csv_rows) csv << ex::format_csv_row(row);
        std::size_t resumed_rows = 0;
        for (char c : r.resumed_csv_rows) if (c == '\n') ++resumed_rows;
        total_resumed += resumed_rows; total_fresh += r.csv_rows.size();
        any_measured += r.measured; any_resumed += r.resumed_binaries;
    }

    std::cout << "CSV geschrieben: " << out_csv << "  (" << (total_resumed + total_fresh)
              << " Zeilen = " << total_resumed << " resumiert + " << total_fresh << " frisch, ueber "
              << n_sweep.size() << " Working-Set-N)\n";

    // Exit 0 = mind. 1 (Binary × Setting) real gemessen ODER resumiert (Voll-Resume = gültiger Lauf).
    return (any_measured > 0 || any_resumed > 0) ? 0 : 1;
}
