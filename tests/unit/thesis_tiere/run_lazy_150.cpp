// run_lazy_150 — L-LAZY-E2E: das HOST-EXECUTABLE des Lazy-E2E-Treibers fuer den >=150-Lauf. PROFIL-GETRIEBEN
// (STRANG A KORRIGIERT, Inc6/S7c, 2026-06-19): der Host reduziert sich auf Argument-/Toolchain-Aufbereitung +
// EINEN Aufruf der deklarativen CEB-Eintritts-API tlz::run_profile(RunProfileArgs) (profile_run_entry.hpp).
// run_profile selbst baut den ExperimentTree DEKLARATIV aus dem comdare_thesis_profile, faehrt aus EINEM Profil
// BEIDE Subsets (Basis-320 → source_catalog UND die <sota_series>-Reihen → sota_catalog, S7b Multi-Pass) ueber
// EINE vereinigte SourceGenFn (source_catalog ∪ sota_catalog, S7a) und schreibt EINE CSV. RESUMIERBAR ueber
// build_version (.version-Sidecar je DLL) + per-Binary-result.csv-Stamp (#139).
//
// Die CompileFn baut mit den MESS-Defines (COMDARE_MEASUREMENT_ON …) + dem Include-Satz aus COMDARE_PILOT_INCLUDES
// (';'-getrennt, vom Harness gesetzt). vcvars64 muss aktiv sein (cl im PATH).
//
// argv: run_lazy_150 <out_csv> <max_binaries> <n_ops> <build_version> <src_dir> <dll_dir> [min_free_gb]
//       [cores_per_build] [n_repeats] [profile:<pfad>[@<achse>]] [resume=1|0]
//   • argv[10] = der PROFIL-PFAD (optional "profile:"-Praefix; optional "@<achse>" = ein im Profil als
//     <axis_sweep> DEKLARIERTER Per-Achsen-Sweep, sonst die Basis-Selektion). Alternativ env COMDARE_THESIS_PROFILE.
//
// SELEKTION (PROFIL-GETRIEBEN): die WHAT-Konfiguration (Lebewesen/Achsen/Sweeps/SOTA/Working-Set/run_options)
// kommt VOLLSTAENDIG aus dem Profil. argv ist NUR noch Pfad/Toolchain/Output + die 4 Resume-/cap-/platform-/
// build_version-Overrides (Rueckwaerts-Kompatibilitaet). Die Code-Selektions-Schicht ist seit Inc4/S5 entfernt.
//
// MESS-ARCHITEKTUR (2026-06-04): die CSV traegt total_ns + ns_per_op, 19 echte per-Segment-ns, eine repetition-
// Spalte (D, Default 3) und die Observer-DELTA-Counter (A). Die perm-DLLs + Source + .obj + .cl.log + .version +
// per-Binary-result.csv liegen je Binary in einem eigenen Unterordner unter dll_dir/<stem>/ (E).
#include "profile_run_entry.hpp"   // tlz::run_profile / RunProfileArgs (S7c CEB-Eintritts-API)
#include "validate_profile.hpp"    // #169(A): validate_profile / axis_registry_from_levels (rein-lesend, kein Bau)

#include <builder/workload_driver/load_profile_parser.hpp>   // discover_load_profiles / parse_load_profile (Achse 2)
#include <builder/experiment_tree/registry_to_axis_levels.hpp>  // #169(A): build_all_axis_levels (EnabledStrategies-Quelle)

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <thread>
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

// #169(A) — Profil-Pfad aus einem rohen Argument extrahieren ("profile:"-Praefix + "@<achse>"-Suffix entfernen).
static std::string strip_profile_token(std::string s) {
    if (s.rfind("profile:", 0) == 0) s = s.substr(std::string("profile:").size());
    if (auto at = s.rfind('@'); at != std::string::npos) s.erase(at);
    return s;
}

// ── C1083-RETRY (Build-Haertung, gate-frei) ──────────────────────────────────────────────────────────────
// PROBLEM (Teil 1, traf 1/6 DLLs): der OneDrive-Cloud-Sync sperrt die soeben vom BuildOrchestrator geschriebene
// perm.cpp (write/read-Race), WAEHREND cl sie zu lesen versucht → `cl : Command line error D8003` bzw.
// haeufiger `fatal error C1083: Cannot open source file: 'perm.cpp': Permission denied`. Das ist KEIN echter
// Quell-/Code-Defekt, sondern ein transienter Datei-Lock des Sync-Daemons → ein kurzes Backoff + Wiederholung
// des CL-Aufrufs (die perm.cpp liegt unveraendert auf Disk) baut beim 2./3. Versuch durch.
//
// WARUM RETRY (und nicht „ausserhalb OneDrive schreiben"): die perm.cpp/perm.dll MUESSEN im per-Binary-Ordner
// unter dem Repo-Build-Baum (build/thesis_tiere/tiere/<stem>/, OneDrive-synchronisiert) liegen, weil (a) der
// Resume-.version-Sidecar + die per-Binary-result.csv (#139) dort erwartet werden und (b) der Pfad in der
// committe-baren CSV/Doku referenziert ist. Ein Auslagern nach %TEMP% wuerde Resume #139 + die Ordner-
// Konvention (E) brechen (NICHT gate-frei). Der Retry ist die minimale, lokale, benannte Haertung.
[[nodiscard]] inline bool cl_log_has_transient_lock(std::filesystem::path const& log) {
    std::ifstream f{log, std::ios::binary};
    if (!f) return false;
    std::string const content((std::istreambuf_iterator<char>(f)), {});
    // C1083 = "Cannot open source file ... Permission denied"; D8003 = fehlende Quelle (Sync-Rename-Fenster).
    return content.find("C1083") != std::string::npos
        || content.find("Permission denied") != std::string::npos
        || content.find("D8003") != std::string::npos;
}

/// run_cl_with_c1083_retry — fuehrt den CL-Befehl aus; bei Exit != 0 UND C1083/Permission-denied im .cl.log
/// wird nach kurzem Backoff (50ms, 150ms, 400ms) erneut versucht (max kRetries). Ein echter Compile-Fehler
/// (kein Lock-Marker im Log) wird SOFORT durchgereicht (kein Maskieren echter Defekte — Ehrlichkeit).
[[nodiscard]] inline int run_cl_with_c1083_retry(std::string const& cmd, std::filesystem::path const& log) {
    constexpr int kRetries = 3;
    constexpr int kBackoffMs[kRetries] = {50, 150, 400};
    int rc = std::system(cmd.c_str());
    for (int attempt = 0; attempt < kRetries && rc != 0; ++attempt) {
        if (!cl_log_has_transient_lock(log)) break;   // echter Fehler → nicht wiederholen
        std::cerr << "  [C1083-retry] transienter Datei-Lock (OneDrive-Sync) erkannt — Versuch "
                  << (attempt + 2) << "/" << (kRetries + 1) << " nach " << kBackoffMs[attempt] << "ms Backoff\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(kBackoffMs[attempt]));
        rc = std::system(cmd.c_str());
    }
    return rc;
}

int main(int argc, char** argv) {
    // ── #169(A) NUTZERFREUNDLICHKEIT: das REIN-LESENDE --validate-Kommando. Prueft ein comdare_thesis_profile gegen
    //    die AxisRegistry/EnabledStrategies, BEVOR teuer gebaut/gemessen wird. KEIN DLL-Bau, KEINE Messung, kein
    //    Tree-/Source-Materialisieren — nur parse_thesis_profile + validate_profile gegen build_all_axis_levels().
    //    Aufruf:  run_lazy_150 --validate <profil-pfad>   (ODER --validate + env COMDARE_THESIS_PROFILE).
    //    Exit 0 + Zusammenfassung bei OK; Exit != 0 + klare Meldung (Achse + ungueltiger Wert) bei Fehler. ──
    {
        bool want_validate = false;
        std::string validate_path;
        for (int i = 1; i < argc; ++i) {
            std::string const arg = argv[i];
            if (arg == "--validate" || arg == "-validate" || arg == "--check") { want_validate = true; continue; }
            // Ein nachfolgendes Nicht-Flag-Argument = der Profil-Pfad (auch "profile:<pfad>[@<achse>]"-Form).
            if (want_validate && validate_path.empty() && !arg.empty() && arg[0] != '-')
                validate_path = strip_profile_token(arg);
        }
        if (want_validate) {
            if (validate_path.empty()) {
                if (char const* e = std::getenv("COMDARE_THESIS_PROFILE"); e != nullptr && *e != '\0')
                    validate_path = strip_profile_token(e);
            }
            if (validate_path.empty()) {
                std::cerr << "run_lazy_150 --validate: kein Profil-Pfad (Aufruf: --validate <pfad> ODER env "
                             "COMDARE_THESIS_PROFILE). KEIN Bau ausgefuehrt.\n";
                return 6;
            }
            std::optional<comdare::builder::xml::ThesisProfile> const tp =
                tlz::load_thesis_profile(validate_path);
            if (!tp) {
                std::cerr << "run_lazy_150 --validate: Profil '" << validate_path
                          << "' nicht lesbar (parse_thesis_profile=nullopt). KEIN Bau ausgefuehrt.\n";
                return 5;
            }
            // Die gueltigen Achsen-Werte kommen aus den REALEN EnabledStrategies (build_all_axis_levels reflektiert
            // die TopicConfigSet::StaticAxisVariants*-Listen) — NICHT aus einer hartkodierten Liste.
            ex::AxisRegistry const registry =
                tlz::axis_registry_from_levels(ex::build_all_axis_levels());
            tlz::ProfileValidationResult const vr = tlz::validate_profile(*tp, registry);
            tlz::print_validation_report(vr, *tp, std::cout);
            std::cout << "(--validate: rein-lesend — es wurde KEINE DLL gebaut und KEINE Messung durchgefuehrt.)\n";
            return vr.ok ? 0 : 1;
        }
    }

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
    std::uint32_t const n_repeats = (argc >= 10)
        ? static_cast<std::uint32_t>(std::strtoul(argv[9], nullptr, 10)) : 3u;
    std::string profile_arg = (argc >= 11) ? std::string{argv[10]} : std::string{};
    bool resume = (argc >= 12) ? (std::strtoul(argv[11], nullptr, 10) != 0) : true;
    bool const resume_from_argv = (argc >= 12);

    // ── Profil-Pfad + optionaler @<achse>-Sweep-Selektor (env COMDARE_THESIS_PROFILE / COMDARE_PROFILE_SELECT). ──
    std::string profile_path = profile_arg, profile_select_axis;
    if (profile_path.rfind("profile:", 0) == 0) profile_path = profile_path.substr(std::string("profile:").size());
    if (profile_path.empty()) { char const* e = std::getenv("COMDARE_THESIS_PROFILE"); if (e != nullptr) profile_path = e; }
    if (auto at = profile_path.rfind('@'); at != std::string::npos) {
        profile_select_axis = profile_path.substr(at + 1);
        profile_path.erase(at);
    }
    if (char const* e = std::getenv("COMDARE_PROFILE_SELECT"); e != nullptr && *e != '\0') profile_select_axis = e;
    if (profile_path.empty()) {
        std::cerr << "run_lazy_150: kein Profil angegeben (argv[10]=profile:<pfad> ODER env COMDARE_THESIS_PROFILE)."
                     " Der Treiber ist profil-getrieben (STRANG A) — Abbruch.\n";
        return 5;
    }

    // ── Achse 2 (#135): XML-Lastprofile = Werte der dynamischen Workload-Achse (Discovery aus COMDARE_LOAD_PROFILE_DIR). ──
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

    // ── Die CompileFn (Response-File-cl, MAX_PATH-/8191-sicher): wie bisher, aus den Mess-Defines + Includes. ──
    std::vector<std::string> const defs = {
        "/DCOMDARE_ANATOMY_MODULE_BUILD=1", "/DCOMDARE_MEASUREMENT_ON=1", "/DCOMDARE_CE_ENABLE_STATISTICS=1",
        "/DCOMDARE_EXPERIMENT_MODE_ON=1",   "/DCOMDARE_OS_WINDOWS=1",     "/DCOMDARE_ARCH_X86_64=1",
        "/DCOMDARE_CACHE_LINE_SIZE=64",     "/DWIN32",                    "/D_WINDOWS"
    };
    std::vector<std::string> const incs = env_includes();
    ex::CompileFn compile = [defs, incs](ex::BuildJob const& job) -> int {
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
        std::filesystem::path const log = job.output.string() + ".cl.log";
        std::string const cmd = "cl @\"" + rsp.string() + "\" > \"" + log.string() + "\" 2>&1";
        // C1083-Haertung (Build-Haertung, gate-frei): transienter OneDrive-Sync-Lock der perm.cpp → Backoff+Retry.
        return run_cl_with_c1083_retry(cmd, log);
    };

    // ── DIE EINE deklarative CEB-Eintritts-API (S7c). Die WHAT-Konfiguration kommt komplett aus dem Profil. ──
    tlz::RunProfileArgs a;
    a.profile_path = profile_path;
    a.out_csv      = out_csv;
    a.src_dir      = src_dir;
    a.dll_dir      = dll_dir;
    a.compile      = compile;
    a.n_ops        = n_ops;
    a.max_binaries = max_binaries;     // 0 ⇒ run_profile nimmt <run_options>.cap
    a.build_version = build_version;
    a.n_repeats    = n_repeats;
    a.cores_per_build = cpb;
    a.min_free_gb  = min_free_gb;
    a.resume_override_set = resume_from_argv;  // argv hat Vorrang vor <run_options resume=..>
    a.resume       = resume;
    a.sweep_axis   = profile_select_axis;      // leer = Basis-Selektion; sonst deklarierter <axis_sweep>
    // S7b: die <sota_series>-Paesse mitfahren. env COMDARE_RUN_SOTA=0 schaltet sie ab (z.B. reiner Basis-Lauf
    // oder die alte PS-foreach-je-N, die den SOTA-Block nicht je N wiederholen will).
    if (char const* rs = std::getenv("COMDARE_RUN_SOTA"); rs != nullptr && std::string{rs} == "0")
        a.run_sota_series = false;
    a.workload_registry = workload_registry;
    a.workload_values   = workload_values;
    // CSV-Tag-Overrides (Infra-Agent setzt sie je Plattform/Build-Reihe via env; sonst aus <run_options>).
    if (char const* p  = std::getenv("COMDARE_PLATFORM");      p  != nullptr && *p  != '\0') a.platform_override          = p;
    if (char const* bv = std::getenv("COMDARE_BUILD_VERSION"); bv != nullptr && *bv != '\0') a.build_version_tag_override = bv;
    // Working-Set: ist COMDARE_WORKLOAD_RECORDS gesetzt (alte PS-Schleife je N), faehrt run_profile EINEN N-Wert
    // (Override) — sonst den Profil-<working_set_sweep>. Erhaelt die Rueckwaerts-Kompatibilitaet der PS-foreach.
    if (char const* lr = std::getenv("COMDARE_WORKLOAD_RECORDS"); lr != nullptr && *lr != '\0')
        a.working_set_override = std::strtoull(lr, nullptr, 10);

    tlz::RunProfileResult const r = tlz::run_profile(a);
    return r.exit_code;
}
