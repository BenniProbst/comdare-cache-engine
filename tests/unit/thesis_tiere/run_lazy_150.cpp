// run_lazy_150 — L-LAZY-E2E (gate-frei, 2026-06-03): das HOST-EXECUTABLE des Lazy-E2E-Treibers für den ≥150-Lauf.
// Baut den ExperimentTree aus dem FullPilot (320 reale SA-Kompositionen ≥150) + dyn. Dimensionen, ruft
// run_lazy_static_then_dynamic (3 Lazy-Iteratoren: statische Kompilierung → laden → dyn-Variation → messen →
// ingest) für die ersten -MaxBinaries Blätter und schreibt eine CSV. RESUMIERBAR über build_version (.version-
// Sidecar je DLL): ein erneuter Lauf überspringt versions-aktuelle DLLs (überlebt Absturz/Teil-Lauf).
//
// Die CompileFn baut mit den MESS-Defines (COMDARE_MEASUREMENT_ON …) + dem Include-Satz aus COMDARE_PILOT_INCLUDES
// (';'-getrennt, vom Harness gesetzt). vcvars64 muss aktiv sein (cl im PATH).
//
// argv: run_lazy_150 <out_csv> <max_binaries> <n_ops> <build_version> <src_dir> <dll_dir> [min_free_gb] [cores_per_build] [n_repeats] [select_mode] [resume=1|0]
//
// Mess-RESUME (#139, 2026-06-11): Default AN (argv[11]=1). Binaries mit vollständiger+konfigurations-aktueller
// per-Binary result.csv (result.csv.stamp == aktueller Config-Stempel) werden übersprungen; ihre Zeilen fließen
// unverändert in die globale CSV. Stale Ergebnisse (anderer BuildVersion/n_ops/Workload-Set) matchen nicht →
// Neu-Messung (Zwei-Phasen-Cache-Warmup intrinsisch je Op). Überlebt damit Reboots/Abbrüche auf Binary-Granularität.
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
#include "m3v2_select_profile.hpp"                       // M3v2-SELEKTION (Task #156): Basis/Sweep/SOTA-Reihen + Tags
#include "profile_runner.hpp"                             // STRANG-A Inc1 (ADDITIV): build_profile_levels (build_axis_levels live)
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

    // Baum: FullPilot statische Achsen (320 ≥150) + dyn. Dimensionen (thread_count × prefetch_distance ×
    // repetition = 3·2·n_repeats Settings je Binary). Die repetition-Achse (D) erzeugt je Wiederholung eine
    // EIGENE setting_id → je Rep eine eigene Roh-CSV-Zeile (nie interpoliert).
    // STRANG-A Inc1 (ADDITIV, gate-frei): select_mode "profile:<pfad>" baut den Baum DEKLARATIV aus einem
    // comdare_thesis_profile (parse_thesis_profile → build_axis_levels → tree.build) — die offizielle, bisher
    // verwaiste Kette (Doc 10 §2.2). Der Code-Pfad (FullPilot, build_pilot_levels) bleibt UNANGETASTET der
    // Default; das Profil ersetzt NUR die AxisLevels-Quelle. ENV COMDARE_THESIS_PROFILE als Alternative zum
    // select_mode-Suffix. Der Round-Trip-Beleg (test_profile_roundtrip) garantiert binary_id-Identität.
    // STRANG-A Inc3 (S3-complete, 2026-06-18): das select_mode-Suffix nach "profile:" ist der PROFIL-PFAD, optional
    // mit einem "@<achse>"-Sub-Selektor fuer einen PER-ACHSEN-SWEEP (sonst BASIS-320). Ersetzt die SelectMode-
    // Branches ("axis_sweep:<a>" + "sota:<s>") fuer den profil-getriebenen Weg: die Sweep-Achse muss als
    // <axis_sweep> im Profil DEKLARIERT sein (profil-getriebene Whitelist statt hartkodierter axis_to_level-Map).
    std::string profile_path, profile_select_axis;
    if (select_mode.rfind("profile:", 0) == 0) profile_path = select_mode.substr(std::string("profile:").size());
    if (profile_path.empty()) { char const* e = std::getenv("COMDARE_THESIS_PROFILE"); if (e != nullptr) profile_path = e; }
    if (auto at = profile_path.rfind('@'); at != std::string::npos) {   // "profile:<pfad>@<achse>" → Sweep-Achse
        profile_select_axis = profile_path.substr(at + 1);
        profile_path.erase(at);
    }
    // Alternativ via env (ueberschreibt das @-Suffix), z.B. fuer die PS-Sweep-Schleife.
    if (char const* e = std::getenv("COMDARE_PROFILE_SELECT"); e != nullptr && *e != '\0') profile_select_axis = e;
    bool const profile_mode = !profile_path.empty();

    // Profil EINMAL parsen (Inc3 konsumiert daraus working_set_sweep / axis_sweeps / run_options).
    std::optional<comdare::builder::xml::ThesisProfile> tp_opt;
    if (profile_mode) {
        tp_opt = tlz::load_thesis_profile(profile_path);
        if (!tp_opt) { std::cerr << "run_lazy_150: COMDARE_THESIS_PROFILE/'" << profile_path
                             << "' nicht lesbar (parse_thesis_profile=nullopt) — Abbruch.\n"; return 5; }
    }

    auto factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    std::vector<ex::AxisLevel> profile_static_basis;   // tier-frei (fuer profile_axis_level), Inc3
    if (profile_mode) {
        auto const& tp = *tp_opt;
        std::string const mode_name = tp.modes.empty() ? std::string{"pilot_base"} : tp.modes.front().name;
        // tier-Level-Handling (Inc3): build_axis_levels emittiert ein "tier"-Level oben (base_tiers). Fuer den
        // BASIS-320-Lauf (reine 4-Achsen-Permutation) wird es abgezogen → DERSELBE binary_id-Raum wie FullPilot
        // (Resume #139 stabil; Round-Trip belegt in test_profile_roundtrip.cpp (6)).
        std::vector<ex::AxisLevel> const lv = tlz::build_profile_basis_levels(tp, mode_name, /*with_dynamic=*/true);
        profile_static_basis = lv;
        tree.build(lv);
        std::cout << "PROFIL-MODUS: " << profile_path << "  (id=" << tp.id << " mode=" << mode_name
                  << " select_axis=" << (profile_select_axis.empty() ? "basis" : profile_select_axis)
                  << ")  tree.binary_count() = " << tree.binary_count()
                  << "  dyn_dims = " << tree.dynamic_filter().size() << "\n";
    } else {
        tree.build(tlz::build_pilot_levels<tlz::FullPilot>(/*with_dynamic=*/true, n_repeats, workload_values));
    }

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

    // STRANG-A Inc3 (run_options): cap aus <run_options cap=..> als max_binaries-Default, wenn argv[2] nicht
    // gesetzt war (argv hat Vorrang). Hier wird der EFFEKTIVE Cap bestimmt; max_binaries (argv) bleibt unangetastet.
    std::size_t eff_cap = max_binaries;
    if (profile_mode) {
        tlz::ProfileRunOptions const ro = tlz::profile_run_options(*tp_opt);
        if (ro.cap > 0 && max_binaries == 0) eff_cap = ro.cap;   // argv[2]=0 ⇒ Profil-cap greift
        // resume aus <run_options resume=..>, falls argv[11] NICHT gesetzt war (argv hat Vorrang).
        if (!resume_from_argv) resume = ro.resume;
    }
    std::size_t const N = (std::min)(eff_cap, tree.binary_count());

    // M3v2-SELEKTION (Task #156): die 5 Lauf-/Selektions-Tags. series/sweep_axis kommen aus dem SelectMode (s.u.);
    // working_set_n = workload_records (vom Harness via COMDARE_WORKLOAD_RECORDS gesetzt → je N-Sweep-Pass ein Lauf);
    // platform/build_version aus env (Infra-Agent setzt sie je Plattform-Reihe), sonst Defaults.
    // STRANG-A Inc3 (run_options): platform/build_version-Defaults aus <run_options> (env hat Vorrang).
    namespace m3 = ::comdare::cache_engine::thesis_lazy::m3v2;
    m3::RowTags row_tags;   // Default: series="-" sweep_axis="-" platform="win-x86_64" build_version="m3v2"
    if (profile_mode) {     // Profil-Defaults zuerst (env darf gleich darauf uebersteuern)
        tlz::ProfileRunOptions const ro = tlz::profile_run_options(*tp_opt);
        if (!ro.platform.empty())      row_tags.platform      = ro.platform;
        if (!ro.build_version.empty()) row_tags.build_version = ro.build_version;
    }
    if (char const* p = std::getenv("COMDARE_PLATFORM");      p != nullptr && *p != '\0') row_tags.platform = p;
    if (char const* bv = std::getenv("COMDARE_BUILD_VERSION"); bv != nullptr && *bv != '\0') row_tags.build_version = bv;
    else if (!profile_mode) row_tags.build_version = build_version;   // ohne Tag = die BuildVersion-Marke (m3v2)

    // SELEKTIONS-MODI (m3v2): die reproduzierbare m3v2_select_profile-Quelle erzeugt je Modus den getaggten Pass.
    //   • "index"            : BASIS (voll-faktoriell, ersten N) — make_basis. series/sweep="-".
    //   • "search_algo_grid" : F15-Grid (NUR search_algo) — rückwärtskompatibel.
    //   • "axis_sweep:<axis>": PER-ACHSEN-SWEEP gegen die feste Baseline (eine Achse über ihre Ausprägungen).
    //   • "sota:<A|B|C>"     : SOTA-REIHE — Stufe-1/2/3-Tag. Im Pilot über die Basis-Binaries getaggt (SOTA-Engine HELD).
    // Achsen-Namen→Level-Index für axis_sweep (FullPilot-static_levels-Reihenfolge): search_algo=0, node_type=4,
    // memory_layout=5, prefetch=7, path_compression=3 (im FullPilot gepinnt → 1 Binary = Baseline). Single-Source.
    auto axis_to_level = [](std::string const& a) -> std::size_t {
        if (a == "search_algo")      return 0;
        if (a == "path_compression") return 3;
        if (a == "node_type")        return 4;
        if (a == "memory_layout")    return 5;
        if (a == "prefetch")         return 7;
        return static_cast<std::size_t>(-1);   // unbekannt → make_axis_sweep verweigert (provenance-Marker)
    };

    ex::BuildSelection sel;
    if (profile_mode) {
        // STRANG-A Inc3 (axis_sweeps): die Selektion kommt PROFIL-GETRIEBEN aus tlz::profile_select — entweder
        // BASIS-320 (profile_select_axis leer) oder ein im Profil als <axis_sweep> DEKLARIERTER Per-Achsen-Sweep.
        // Der level_d wird ueber profile_axis_level aus den tier-freien Basis-Levels aufgeloest (KEINE hartkodierte
        // axis_to_level-Map). Funktional == m3::make_basis/make_axis_sweep, nur die Auswahl ist deklarativ.
        tlz::ProfileTaggedSelection const pts =
            tlz::profile_select(*tp_opt, profile_static_basis, view, profile_select_axis, N);
        sel = pts.selection;
        row_tags.series = pts.series; row_tags.sweep_axis = pts.sweep_axis;
        std::cout << "PROFIL-SELEKTION: label=" << pts.label << "  provenance=" << sel.provenance << "\n";
    } else if (select_mode == "search_algo_grid") {
        sel = select_search_algo_grid(view, N);          // je search_algo ein Binary (≤ K_search Binaries)
    } else if (select_mode.rfind("axis_sweep:", 0) == 0) {
        std::string const axis = select_mode.substr(std::string("axis_sweep:").size());
        m3::TaggedSelection const tsel = m3::make_axis_sweep(view, axis_to_level(axis), axis, N);
        sel = tsel.selection;
        row_tags.series = tsel.tags.series; row_tags.sweep_axis = tsel.tags.sweep_axis;
    } else if (select_mode.rfind("sota:", 0) == 0) {
        // SOTA-Reihe (A/B/C): die SOTA-/PRT-ART-Engine-Erweiterung ist HELD → der Pilot belegt den A/B/C-Tag-Apparat
        // über die Basis-Binaries (Reihe-Repräsentanten). series=<A|B|C>; sweep_axis trägt den Reihen-Marker "sota".
        std::string const series = select_mode.substr(std::string("sota:").size());
        m3::TaggedSelection const tsel = m3::make_basis(view, N);
        sel = tsel.selection;
        row_tags.series = series; row_tags.sweep_axis = "sota";
    } else {   // "index" / "basis" (Default): BASIS-320 (voll-faktoriell, ersten N)
        m3::TaggedSelection const tsel = m3::make_basis(view, N);
        sel = tsel.selection;
        row_tags.series = tsel.tags.series; row_tags.sweep_axis = tsel.tags.sweep_axis;
    }
    std::cout << "Selektion: provenance=" << sel.provenance << "  indices=" << sel.size()
              << "  [m3v2 tags: series=" << row_tags.series << " sweep_axis=" << row_tags.sweep_axis
              << " platform=" << row_tags.platform << " build_version=" << row_tags.build_version
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
    ex::SourceGenFn gen = tlz::make_pilot_source_gen<tlz::FullPilot>();
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
    if (profile_mode && !env_records_set) n_sweep = tlz::profile_working_set_sweep(*tp_opt);
    if (n_sweep.empty()) {   // kein Profil-Sweep (oder env hat Vorrang) → EIN Pass mit dem env-Wert (0=Default)
        std::uint64_t rec = 0;
        if (char const* lr = std::getenv("COMDARE_WORKLOAD_RECORDS"); lr != nullptr) rec = std::strtoull(lr, nullptr, 10);
        n_sweep.push_back(rec);   // 0 ⇒ Iterator setzt records = n_ops
    }
    std::cout << "WORKING-SET-SWEEP (aeussere Iteration): [";
    for (std::size_t i = 0; i < n_sweep.size(); ++i) std::cout << (i ? "," : "") << n_sweep[i];
    std::cout << "]  Quelle=" << ((profile_mode && !env_records_set && !tp_opt->working_set_sweep.empty())
                                  ? "profil" : "env/default") << "\n";

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
        // M3v2-SELEKTION (Task #156): die 5 Lauf-/Selektions-Tags je Mess-Zeile (CSV-Spalten series/sweep_axis/
        // working_set_n/platform/build_version). working_set_n = cfg.workload_records (= ws_n); 4 übrige aus row_tags.
        cfg.row_series        = row_tags.series;
        cfg.row_sweep_axis    = row_tags.sweep_axis;
        cfg.row_platform      = row_tags.platform;
        cfg.row_build_version = row_tags.build_version;
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
