// profile_run_facade.cpp -- die einzige umbrella-ziehende Uebersetzungseinheit
// der produktiven run_profile-Fassade.

#include "profile_run_facade.hpp"

#include "profile_run_entry.hpp"
#include "validate_profile.hpp" // P5: axis_registry_from_levels / validate_profile / print_validation_report

#include <builder/build_orchestrator/build_orchestrator.hpp>
#include <builder/experiment_tree/registry_to_axis_levels.hpp> // P5: build_all_axis_levels (EnabledStrategies)
#include <builder/workload_driver/load_profile_parser.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
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

[[nodiscard]] std::vector<std::string> split_on(std::string const& s, char sep) {
    std::vector<std::string> out;
    std::string              cur;
    for (char const c : s) {
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

[[nodiscard]] bool any_existing_directory(std::vector<std::string> const& dirs) {
    for (auto const& d : dirs) {
        if (std::filesystem::is_directory(d)) return true;
    }
    return false;
}

[[nodiscard]] std::vector<std::string> baked_perm_include_dirs() {
#ifdef COMDARE_FACADE_PERM_INCLUDES
    return split_on(COMDARE_FACADE_PERM_INCLUDES, '|');
#else
    return {};
#endif
}

[[nodiscard]] std::vector<std::string> perm_include_dirs() {
    if (char const* e = std::getenv("COMDARE_PILOT_INCLUDES"); e != nullptr && *e != '\0') {
        std::vector<std::string> const env_dirs = split_on(e, ';');
        if (any_existing_directory(env_dirs)) return env_dirs;
        std::cerr << "[profile_facade] COMDARE_PILOT_INCLUDES gesetzt, aber kein Verzeichnis existiert; "
                     "nutze gebackene Include-Liste.\n";
    }
    return baked_perm_include_dirs();
}

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

[[nodiscard]] std::string cxx_compiler() {
    if (char const* e = std::getenv("COMDARE_CXX"); e != nullptr && *e != '\0') return e;
    return "g++-16";
}

} // namespace

ProfileRunResult run_profile_facade(ProfileRunArgs const& args) {
    ProfileRunResult out;

    // Achse-2-Lastprofile (#135/G1/#229): Gibt der Host kein Verzeichnis vor, defaultet die WIE-Schicht
    // auf die zum Thesis-Profil co-lokalisierten Lastprofile (algorithm_profiles/load_profiles/,
    // Schwesterordner von thesis_profiles/) — so ist die Profil-XML selbst-suffizient und braucht kein
    // COMDARE_LOAD_PROFILE_DIR; env bleibt reiner Override. Findet die Fassade 0 gueltige Profile, bricht
    // SIE mit exit 4 ab (Achse 2 darf nicht still entfallen = two_phase_valid=0-Schutz).
    std::filesystem::path load_profile_dir = args.load_profile_dir;
    if (load_profile_dir.empty() && !args.profile_path.empty())
        load_profile_dir = args.profile_path.parent_path().parent_path() / "load_profiles";

    // <workloads> im Thesis-Profil ist die AUTORITATIVE Achse-2-Auswahl: die ids der Lastprofile
    // (z.B. ycsb_a..ycsb_f). Nur die dort genannten werden aus dem (bewusst reicheren) load_profiles/-
    // Verzeichnis uebernommen — so steuert die XML den Workload-Satz vollstaendig (#229). Leer bzw. kein
    // parsbares Profil => alle entdeckten Profile (Rueckwaerts-Kompatibilitaet mit dem env-Override-Pfad).
    std::vector<std::string> workload_select;
    if (auto tp = tlz::load_thesis_profile(args.profile_path)) workload_select = tp->workloads;
    auto const is_selected = [&workload_select](std::string const& id) {
        return workload_select.empty() ||
               std::find(workload_select.begin(), workload_select.end(), id) != workload_select.end();
    };

    std::map<std::string, wd::WorkloadConfig> workload_registry;
    std::vector<std::string>                  workload_values;
    if (!load_profile_dir.empty()) {
        for (auto const& idp : wd::discover_load_profiles(load_profile_dir)) {
            if (!is_selected(idp.first)) continue;
            if (auto lp = wd::parse_load_profile(idp.second)) {
                workload_registry[idp.first] = lp->config;
                workload_values.push_back(idp.first);
            }
        }
        std::cout << "[profile_facade] Lastprofile (XML, Achse 2, <workloads>-Auswahl): " << workload_values.size()
                  << " aus " << load_profile_dir.string() << "\n";
        if (workload_values.empty()) {
            std::cerr << "[profile_facade] 0 gueltige Lastprofile fuer die <workloads>-Auswahl in '"
                      << load_profile_dir.string() << "' -- Abbruch (Achse 2 darf nicht still entfallen).\n";
            out.exit_code = 4;
            return out;
        }
    }

    tlz::RunProfileArgs a;
    a.profile_path               = args.profile_path;
    a.out_csv                    = args.out_csv;
    a.src_dir                    = args.src_dir;
    a.dll_dir                    = args.dll_dir;
    a.compile                    = ex::make_gpp_compile_fn(perm_include_dirs(), perm_mess_defines(), cxx_compiler());
    a.n_ops                      = args.n_ops;
    a.max_binaries               = args.max_binaries;
    a.build_version              = args.build_version;
    a.n_repeats                  = args.n_repeats;
    a.cores_per_build            = args.cores_per_build;
    a.min_free_gb                = args.min_free_gb;
    a.resume_override_set        = args.resume_override_set;
    a.resume                     = args.resume;
    a.sweep_axis                 = args.sweep_axis;
    a.platform_override          = args.platform_override;
    a.build_version_tag_override = args.build_version_tag_override;
    a.run_sota_series            = args.run_sota_series;
    a.working_set_override       = args.working_set_override;
    a.workload_registry          = std::move(workload_registry);
    a.workload_values            = std::move(workload_values);

    tlz::RunProfileResult const r = tlz::run_profile(a);
    out.exit_code                 = r.exit_code;
    out.basis_rows                = r.basis_rows;
    out.sota_rows                 = r.sota_rows;
    out.basis_binary_ids          = r.basis_binary_ids;
    out.sota_binary_ids           = r.sota_binary_ids;
    out.measured                  = r.any_measured;
    out.resumed                   = r.any_resumed;
    return out;
}

int validate_profile_facade(std::filesystem::path const& profile_path, std::ostream& os) {
    auto const tp = tlz::load_thesis_profile(profile_path);
    if (!tp) {
        os << "[validate] Profil '" << profile_path.string()
           << "' nicht lesbar (parse_thesis_profile=nullopt). KEIN Bau ausgefuehrt.\n";
        return 5;
    }
    // Die gueltigen Achsen-Werte kommen aus den REALEN EnabledStrategies (build_all_axis_levels
    // reflektiert sie) → Registry → validate_profile prueft jeden <axis>-Wert dagegen.
    ex::AxisRegistry const             registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());
    tlz::ProfileValidationResult const vr       = tlz::validate_profile(*tp, registry);
    tlz::print_validation_report(vr, *tp, os);
    os << "(--validate: rein-lesend — es wurde KEINE DLL gebaut und KEINE Messung durchgefuehrt.)\n";
    return vr.ok ? 0 : 1;
}

} // namespace comdare::cache_engine::builder::profile_facade
