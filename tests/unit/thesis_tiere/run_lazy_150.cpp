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

#include <builder/workload_driver/load_profile_parser.hpp>   // discover_load_profiles / parse_load_profile (Achse 2)

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
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
        std::string const cmd = "cl @\"" + rsp.string() + "\" > \"" + job.output.string() + ".cl.log\" 2>&1";
        return std::system(cmd.c_str());
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
