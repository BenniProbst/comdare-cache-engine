// profile_run_facade.cpp — die EINE umbrella-ziehende .cpp der Fassade (E4-XML-Roadmap Phase 1 / #230).
// Isoliert source_catalog.hpp/profile_run_entry.hpp (all_axes_umbrella → alle 19 Achsen compile-time) hinter
// der Uebersetzungseinheits-Grenze und bietet main.cpp nur den schlanken POD-Header profile_run_facade.hpp
// (GoF-Pattern FACADE). Dies ist die EINZIGE produktive TU, die den Umbrella zieht.
//
// Der reale Compiler-Aufruf (CompileFn) ist g++-16 mit Response-File (@rsp) — das Linux-Muster analog zum
// cl-@rsp-Aufbau im thesis_tiere-Host run_lazy_150.cpp: je Permutation 1 perm_<id>.cpp (Umbrella-Include +
// COMDARE_DEFINE_ANATOMY_MODULE_ADHOC, emittiert vom adhoc_emitter) → 1 perm_<id>.so, mit den Mess-Defines,
// sodass die DLL das IObservableTier-Interface traegt (OHNE COMDARE_MEASUREMENT_ON kein Observer —
// thesis_tiere-Lektion, abi_adapter.hpp #if COMDARE_MEASUREMENT_ON). C++23.

#include "profile_run_facade.hpp" // die schlanke Fassaden-Signatur (POD)

// ── Umbrella-schwere Naht — GENAU HIER isoliert (nur diese TU zieht den all_axes_umbrella): ──
#include "profile_run_entry.hpp" // tlz::run_profile / RunProfileArgs / RunProfileResult (CEB-Eintritt, S7c)

#include <builder/workload_driver/load_profile_parser.hpp> // discover_load_profiles / parse_load_profile (Achse 2)

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::profile_facade {

namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace tlz = ::comdare::cache_engine::thesis_lazy;
namespace wd  = ::comdare::cache_engine::builder::workload_driver;

namespace {

// ── ';'/'|'-Split-Helfer (Include-Satz-Zerlegung). ──
[[nodiscard]] std::vector<std::string> split_on(std::string const& s, char sep) {
    std::vector<std::string> out;
    std::string              cur;
    for (char c : s) {
        if (c == sep) {
            if (!cur.empty()) out.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

// ── Include-Satz fuer den perm_<id>.cpp-Sub-Compile: env COMDARE_PILOT_INCLUDES (';'-getrennt, Harness-
//    Kompatibilitaet, run_lazy_150-Muster; der Infra-Mess-Host setzt es je Plattform) ODER — falls ungesetzt —
//    die per CMake gebackene Default-Liste COMDARE_FACADE_PERM_INCLUDES ('|'-getrennt), damit die Fassade OHNE
//    externes env selbst-genuegsam ist (inkl. der vendored Boost.MP11, die der emittierte perm.cpp via
//    all_axes_umbrella zieht). ──
[[nodiscard]] std::vector<std::string> perm_include_dirs() {
    if (char const* e = std::getenv("COMDARE_PILOT_INCLUDES"); e != nullptr && *e != '\0') return split_on(e, ';');
#ifdef COMDARE_FACADE_PERM_INCLUDES
    return split_on(COMDARE_FACADE_PERM_INCLUDES, '|');
#else
    return {};
#endif
}

// ── Die Mess-Defines fuer den perm-Sub-Compile: aus den EIGENEN Plattform-Makros DIESER TU abgeleitet
//    (comdare_set_platform_defines hat sie beim Fassaden-Bau gesetzt) → die perm.so traegt exakt dieselbe
//    Plattform-/Mess-Konfiguration wie die Fassade. COMDARE_ANATOMY_MODULE_BUILD schaltet die ABI-Export-
//    Symbole der DLL frei; die 3 Mess-Defines aktivieren Observer/Statistik/Experiment-Mode. ──
[[nodiscard]] std::vector<std::string> perm_mess_defines() {
    std::vector<std::string> d = {"-DCOMDARE_ANATOMY_MODULE_BUILD=1", "-DCOMDARE_MEASUREMENT_ON=1",
                                  "-DCOMDARE_CE_ENABLE_STATISTICS=1", "-DCOMDARE_EXPERIMENT_MODE_ON=1"};
#ifdef COMDARE_OS_LINUX
    d.emplace_back("-DCOMDARE_OS_LINUX=1");
#endif
#ifdef COMDARE_OS_WINDOWS
    d.emplace_back("-DCOMDARE_OS_WINDOWS=1");
#endif
#ifdef COMDARE_OS_MACOS
    d.emplace_back("-DCOMDARE_OS_MACOS=1");
#endif
#ifdef COMDARE_ARCH_X86_64
    d.emplace_back("-DCOMDARE_ARCH_X86_64=1");
#endif
#ifdef COMDARE_ARCH_ARM64
    d.emplace_back("-DCOMDARE_ARCH_ARM64=1");
#endif
#ifdef COMDARE_CACHE_LINE_SIZE
    d.emplace_back("-DCOMDARE_CACHE_LINE_SIZE=" + std::to_string(static_cast<long long>(COMDARE_CACHE_LINE_SIZE)));
#endif
    return d;
}

// ── Der C++-Compiler: env COMDARE_CXX ueberschreibt; Default g++-16 (Toolchain-Direktive der Diplomarbeit). ──
[[nodiscard]] std::string cxx_compiler() {
    if (char const* e = std::getenv("COMDARE_CXX"); e != nullptr && *e != '\0') return e;
    return "g++-16";
}

// ── Die REALE CompileFn (g++-16 @rsp) — Linux-Muster analog cl-@rsp in run_lazy_150.cpp. 1 perm_<id>.cpp →
//    1 perm_<id>.so, Response-File-basiert (ARG_MAX-sicher bei langen Include-Saetzen). Der BuildOrchestrator
//    ruft diese Fn je Tier-Binary (job.source → job.output); Rueckgabe 0 = Erfolg. Ein echter Compile-Fehler
//    landet ehrlich im per-Binary .cxx.log (kein Maskieren). ──
[[nodiscard]] ex::CompileFn make_gpp_compile_fn() {
    std::vector<std::string> const defs = perm_mess_defines();
    std::vector<std::string> const incs = perm_include_dirs();
    std::string const              cxx  = cxx_compiler();
    return [defs, incs, cxx](ex::BuildJob const& job) -> int {
        std::filesystem::path const rsp = job.output.string() + ".rsp";
        {
            std::ofstream rf{rsp};
            rf << "-std=c++23 -O2 -fPIC -shared -fno-gnu-unique -fdiagnostics-color=never\n";
            for (auto const& d : defs) rf << d << "\n";
            for (auto const& i : incs) rf << "-I\"" << i << "\"\n";
            rf << "\"" << job.source.string() << "\"\n";
            rf << "-o \"" << job.output.string() << "\"\n";
        }
        std::filesystem::path const log = job.output.string() + ".cxx.log";
        std::string const           cmd = cxx + " @\"" + rsp.string() + "\" > \"" + log.string() + "\" 2>&1";
        return std::system(cmd.c_str());
    };
}

} // namespace

ProfileRunResult run_profile_facade(ProfileRunArgs const& args) {
    ProfileRunResult out;

    // ── Achse 2 (#135): XML-Lastprofile aus load_profile_dir entdecken (leer = keine Workload-Achse). Gleiche
    //    Discovery wie run_lazy_150.cpp:193-206, aber POD-getrieben (kein env-Zwang). ──
    std::map<std::string, wd::WorkloadConfig> workload_registry;
    std::vector<std::string>                  workload_values;
    if (!args.load_profile_dir.empty()) {
        for (auto const& idp : wd::discover_load_profiles(args.load_profile_dir)) {
            if (auto lp = wd::parse_load_profile(idp.second)) {
                workload_registry[idp.first] = lp->config;
                workload_values.push_back(idp.first);
            }
        }
        std::cout << "[profile_facade] Lastprofile (XML, Achse 2) entdeckt: " << workload_values.size() << " aus "
                  << args.load_profile_dir.string() << "\n";
    }

    // ── RunProfileArgs befuellen (Feld-fuer-Feld aus dem POD + die intern gebaute CompileFn/Registry). ──
    tlz::RunProfileArgs a;
    a.profile_path               = args.profile_path;
    a.out_csv                    = args.out_csv;
    a.src_dir                    = args.src_dir;
    a.dll_dir                    = args.dll_dir;
    a.compile                    = make_gpp_compile_fn();
    a.n_ops                      = args.n_ops;
    a.max_binaries               = args.max_binaries; // 0 ⇒ run_profile nimmt <run_options>.cap
    a.build_version              = args.build_version;
    a.n_repeats                  = args.n_repeats;
    a.cores_per_build            = args.cores_per_build;
    a.min_free_gb                = args.min_free_gb;
    a.resume_override_set        = args.resume_override_set;
    a.resume                     = args.resume;
    a.sweep_axis                 = args.sweep_axis; // leer = Basis-Selektion; sonst deklarierter <axis_sweep>
    a.platform_override          = args.platform_override;
    a.build_version_tag_override = args.build_version_tag_override;
    a.run_sota_series            = args.run_sota_series;
    a.working_set_override       = args.working_set_override;
    a.workload_registry          = std::move(workload_registry);
    a.workload_values            = std::move(workload_values);

    tlz::RunProfileResult const r = tlz::run_profile(a);
    out.exit_code        = r.exit_code;
    out.basis_rows       = r.basis_rows;
    out.sota_rows        = r.sota_rows;
    out.basis_binary_ids = r.basis_binary_ids;
    out.sota_binary_ids  = r.sota_binary_ids;
    out.measured         = r.any_measured;
    out.resumed          = r.any_resumed;
    return out;
}

} // namespace comdare::cache_engine::builder::profile_facade
