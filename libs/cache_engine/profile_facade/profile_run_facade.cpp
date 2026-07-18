// profile_run_facade.cpp -- die einzige umbrella-ziehende Uebersetzungseinheit
// der produktiven run_profile-Fassade.

#include "profile_run_facade.hpp"

#include "profile_run_entry.hpp"
#include "experiment_run_entry.hpp" // Brücke-I4: run_experiment_profile (comdare_experiment-Lauf-Unterbau)
#include "validate_profile.hpp"     // P5: axis_registry_from_levels / validate_profile / print_validation_report

#include "xml_config_parser/xml_config_parser.hpp" // Bruecke-I2: XmlConfigParser / ExperimentProfile

#include <cache_engine/measurement/compiler_system_axis.hpp>           // INC-1h: Compiler-System-Achse (gcc|clang)
#include <cache_engine/measurement/extension_hardware_system_axis.hpp> // INC-1d: Erweiterungshardware-Achse (Flag-Quelle)
#include <cache_engine/measurement/optimization_level_sub_axis.hpp> // INC-2c.opt-c: opt_level-Unter-Achse (Flag-Quelle)

#include <builder/build_orchestrator/build_orchestrator.hpp>
#include <builder/experiment_tree/registry_to_axis_levels.hpp> // P5: build_all_axis_levels (EnabledStrategies)
#include <builder/workload_driver/load_profile_parser.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::profile_facade {

namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace tlz = ::comdare::cache_engine::thesis_lazy;
namespace wd  = ::comdare::cache_engine::builder::workload_driver;
namespace cx  = ::comdare::builder::xml; // Bruecke-I2: XmlConfigParser / ExperimentProfile

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

[[nodiscard]] std::vector<std::string> baked_perm_link_libs() {
#ifdef COMDARE_FACADE_PERM_LINK_LIBS
    return split_on(COMDARE_FACADE_PERM_LINK_LIBS, '|');
#else
    return {};
#endif
}

[[nodiscard]] std::vector<std::string> perm_link_libs() {
    if (char const* e = std::getenv("COMDARE_PILOT_LINK_LIBS"); e != nullptr && *e != '\0') {
        std::vector<std::string> env_libs = split_on(e, ';');
        std::erase_if(env_libs, [](std::string const& p) { return !std::filesystem::exists(p); });
        if (!env_libs.empty()) return env_libs;
        std::cerr << "[profile_facade] COMDARE_PILOT_LINK_LIBS gesetzt, aber keine Datei existiert; "
                     "nutze gebackene Link-Lib-Liste.\n";
    }
    return baked_perm_link_libs();
}

[[nodiscard]] std::vector<std::string> baked_perm_extra_cflags() {
#ifdef COMDARE_FACADE_PERM_EXTRA_CFLAGS
    return split_on(COMDARE_FACADE_PERM_EXTRA_CFLAGS, '|');
#else
    return {};
#endif
}

// Erweiterungshardware-System-Achse (Bau-INC-1d, Q2-Option-C): die -march-Flag-QUELLE ist die
// Achse (compile-time-Reflexion), der ORT ist diese CompileFn-Flag-Kette. Default = Generic
// (keine Flags, Ist-Verhalten byte-identisch); die CEB-Laufzeit-Permutation aller Auspraegungen
// kommt mit dem Planer-Strang, bis dahin ist COMDARE_PILOT_SIMD_POLICY der Smoke-Schalter.
[[nodiscard]] std::string_view active_simd_policy() {
    namespace cm            = ::comdare::cache_engine::measurement;
    std::string_view policy = cm::GenericExtensionHardwareAxis::simd_extension_id();
    if (char const* e = std::getenv("COMDARE_PILOT_SIMD_POLICY"); e != nullptr && *e != '\0') policy = e;
    return policy;
}

[[nodiscard]] std::vector<std::string> perm_extension_hardware_cflags() {
    namespace cm                  = ::comdare::cache_engine::measurement;
    std::string_view const policy = active_simd_policy();
    std::string_view       flag;
    if (policy == cm::Avx2ExtensionHardwareAxis::simd_extension_id()) {
        flag = cm::Avx2ExtensionHardwareAxis::gcc_march_flag();
    } else if (policy == cm::Avx512ExtensionHardwareAxis::simd_extension_id()) {
        flag = cm::Avx512ExtensionHardwareAxis::gcc_march_flag();
    } else if (policy != cm::GenericExtensionHardwareAxis::simd_extension_id()) {
        std::cerr << "[profile_facade] COMDARE_PILOT_SIMD_POLICY='" << policy
                  << "' unbekannt; nutze no_extension (generisch).\n";
    }
    if (flag.empty()) return {};
    return {std::string{flag}};
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
    // snmalloc ist header-only: seine INTERFACE-Defs + -mcx16 muessen dem g++-Subprozess
    // explizit mitgegeben werden (gebacken, gleiche Luecke wie bei den Vendor-Archiven).
    for (auto& f : baked_perm_extra_cflags()) d.push_back(std::move(f));
    for (auto& f : perm_extension_hardware_cflags()) d.push_back(std::move(f));
    return d;
}

[[nodiscard]] std::string cxx_compiler() {
    if (char const* e = std::getenv("COMDARE_CXX"); e != nullptr && *e != '\0') return e;
    // INC-1h: der Default-Treiber kommt Single-Source aus der Compiler-System-Achse (gcc-Leg);
    // das clang-Leg faehrt der Experiment-Planer ueber dieselbe Achse (Q3: beide Compiler).
    return std::string{::comdare::cache_engine::measurement::GccCompilerAxis::driver_default()};
}

// opt-d (A2-Hybrid Teil 2): die EINE String->Compiler-Achsen-Typ-Aufloesung sitzt GENAU HIER (Facade), nicht
// im achsen-blinden Builder. Der Builder empfaengt supports_fno_gnu_unique als vom Facade gesteuerten bool-WERT
// (Muster (2)); der fragile cxx.find("clang")-Sniff im build_orchestrator faellt ersatzlos weg.
[[nodiscard]] bool facade_supports_fno_gnu_unique() {
    namespace cm = ::comdare::cache_engine::measurement;
    return cxx_compiler().find("clang") != std::string::npos ? cm::ClangCompilerAxis::supports_fno_gnu_unique()
                                                             : cm::GccCompilerAxis::supports_fno_gnu_unique();
}

// opt_level-Unter-Achse der Compiler-Haupt-Achse (Bau-INC-2c.opt-c). Die Flag-QUELLE ist die Achse
// (OptO*SubAxis::gcc/clang/msvc_opt_flag, compile-time-Reflexion), der ORT ist der opt_flag-Param von
// make_gpp_compile_fn (opt-b). CEB-DEFAULT = O3 (Ruling 2026-07-18, Option B): IEEE-754-deterministisch,
// wahrt den 1-Thread-Mess-Determinismus der golden-Reihe. NICHTS GLOBAL GEPINNT — der Startwert kommt aus
// der benannten Single-Source DefaultOptLevelSubAxis (=O3), env COMDARE_PILOT_OPT_LEVEL + XML/Planer (A3)
// bewegen JEDES Teil. Ofast/O0/O1/O2 leben additiv als +opt=-Sidecar-Vergleichs-Extreme.
[[nodiscard]] std::string_view active_opt_level() {
    // Startwert aus der benannten Achsen-Single-Source (kein rohes Literal, kein Pin) = "O3".
    std::string_view level = ::comdare::cache_engine::measurement::DefaultOptLevelSubAxis::opt_level_id();
    if (char const* e = std::getenv("COMDARE_PILOT_OPT_LEVEL"); e != nullptr && *e != '\0') level = e;
    return level;
}

[[nodiscard]] std::string perm_opt_level_cflags() {
    namespace cm                 = ::comdare::cache_engine::measurement;
    std::string_view const level = active_opt_level();
    bool const             clang = cxx_compiler().find("clang") != std::string::npos;
    auto pick = [&](std::string_view gcc, std::string_view cl) { return std::string{clang ? cl : gcc}; };
    if (level == cm::OptO0SubAxis::opt_level_id())
        return pick(cm::OptO0SubAxis::gcc_opt_flag(), cm::OptO0SubAxis::clang_opt_flag());
    if (level == cm::OptO1SubAxis::opt_level_id())
        return pick(cm::OptO1SubAxis::gcc_opt_flag(), cm::OptO1SubAxis::clang_opt_flag());
    if (level == cm::OptO2SubAxis::opt_level_id())
        return pick(cm::OptO2SubAxis::gcc_opt_flag(), cm::OptO2SubAxis::clang_opt_flag());
    if (level == cm::OptO3SubAxis::opt_level_id())
        return pick(cm::OptO3SubAxis::gcc_opt_flag(), cm::OptO3SubAxis::clang_opt_flag());
    if (level == cm::OptOfastSubAxis::opt_level_id())
        return pick(cm::OptOfastSubAxis::gcc_opt_flag(), cm::OptOfastSubAxis::clang_opt_flag());
    // Fehlerklasse (INC-29.0, KonfigXmlParse-Nachbar): unbekannter Smoke-Wert -> sichtbar degradiert, NIE leer
    // (kein impliziter Compiler-Default /Od), NIE harter exit. Fallback = der bewegliche CEB-Default (O3), NICHT
    // ein O2-Pin. Formale D1-Log-Klassifikation an der Build-Naht folgt INC-29.2/d1-log.
    std::cerr << "[profile_facade] COMDARE_PILOT_OPT_LEVEL='" << level << "' unbekannt; nutze CEB-Default "
              << cm::DefaultOptLevelSubAxis::opt_level_id() << ".\n";
    return pick(cm::DefaultOptLevelSubAxis::gcc_opt_flag(), cm::DefaultOptLevelSubAxis::clang_opt_flag());
}

// H-10 (Bau-INC-1g): die VARIABLEN System-Achsen-Belegungen (Erweiterungshardware-Politik,
// Compiler, opt_level) werden in build_version kodiert — eine unter anderer Belegung gebaute DLL bekommt
// ein eigenes .version-Sidecar (kein falsches Skip via dll_is_current) und die CSV-Spalte
// build_version traegt die Provenienz. Konstante Achsen (Scheduling/Last=Default) bleiben
// weggelassen, bis die CEB-Laufzeit-Permutation sie variabel macht.
[[nodiscard]] std::string system_axes_version_suffix() {
    // A1/OF-2 (Ruling 2026-07-18): KEIN globaler Byte-Anker mehr. Die opt_level-Provenienz wird IMMER emittiert
    // (kein O2-Sonderfall) -> jedes Teil beweglich, keine bevorzugte Referenz-Stufe. Folge: alle Tier-Binaries
    // tragen +opt=<level> (Default +opt=O3) -> dll_is_current sieht sie unter neuer Belegung als neu; die golden-
    // Reihe wird deterministisch unter O3 neu gebaut/gemessen (bewusster Neu-Mess-Lauf, alt-Reihen additiv erhalten).
    std::string suffix = "+ext=" + std::string{active_simd_policy()} + "+cxx=" + cxx_compiler();
    suffix += "+opt=";
    suffix += active_opt_level();
    return suffix;
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
    a.profile_path = args.profile_path;
    a.out_csv      = args.out_csv;
    a.src_dir      = args.src_dir;
    a.dll_dir      = args.dll_dir;
    a.compile      = ex::make_gpp_compile_fn(
        perm_include_dirs(), perm_mess_defines(), cxx_compiler(), perm_link_libs(),
        perm_opt_level_cflags(),           // opt-c: opt_level-Flag (Default O3, beweglich)
        facade_supports_fno_gnu_unique()); // opt-d: Dialekt-Gate als Wert (kein Sniff im Builder)
    a.n_ops                      = args.n_ops;
    a.max_binaries               = args.max_binaries;
    a.build_version              = args.build_version + system_axes_version_suffix();
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
    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());

    // M-CE-12: die REAL vorhandenen load_profiles/-ids enumerieren (gleicher co-lokalisierter Default-Pfad
    // wie der Run: thesis_profiles/../load_profiles, s. run_profile_facade) und als bekannte Workload-Menge
    // hereinreichen — so faellt eine getippte <workloads>-id SCHON hier (rein-lesend) auf, statt erst im
    // teuren E4-Lauf mit exit 4. Existiert das Verzeichnis nicht, bleibt die Menge leer (Pruefung
    // uebersprungen, rueckwaerts-kompatibel).
    std::set<std::string> known_workload_ids;
    if (!profile_path.empty()) {
        std::filesystem::path const load_profile_dir = profile_path.parent_path().parent_path() / "load_profiles";
        for (auto const& idp : wd::discover_load_profiles(load_profile_dir)) known_workload_ids.insert(idp.first);
    }

    tlz::ProfileValidationResult const vr = tlz::validate_profile(*tp, registry, known_workload_ids);
    tlz::print_validation_report(vr, *tp, os);
    os << "(--validate: rein-lesend — es wurde KEINE DLL gebaut und KEINE Messung durchgefuehrt.)\n";
    return vr.ok ? 0 : 1;
}

int validate_experiment_profile_facade(std::filesystem::path const& profile_path,
                                       std::filesystem::path const& ce_registry_path,
                                       std::filesystem::path const& prt_registry_path, std::ostream& os) {
    cx::XmlConfigParser const parser;
    auto const                ep = parser.parse_experiment_profile(profile_path);
    if (!ep) {
        os << "[validate] Experiment-Profil '" << profile_path.string()
           << "' nicht als comdare_experiment lesbar (parse_experiment_profile=nullopt). KEIN Bau ausgefuehrt.\n";
        return 5;
    }

    // Bruecke-I2 (2-Registry-Kanon): je Engine EINE Registry am STATISCHEN Pfad. Die Map wird per Adapter-Typ
    // gebaut (CacheEngineExecutionEngineAdapter→ce, PrtArtExecutionEngineAdapter→prt), mit Fallback per
    // kanonischer engine-id (ee_ce/ee_prt) — so traegt jede deklarierte engine-id ihren statischen Pfad. Der
    // Host reicht BEIDE Pfade herein (die ce-Fassade kennt das prt-art-Repo-Layout nicht — Baseline-Layering).
    std::map<std::string, std::filesystem::path> engine_registry_paths;
    for (auto const& e : ep->engines) {
        if (e.type == "CacheEngineExecutionEngineAdapter")
            engine_registry_paths[e.id] = ce_registry_path;
        else if (e.type == "PrtArtExecutionEngineAdapter")
            engine_registry_paths[e.id] = prt_registry_path;
        else if (e.id == "ee_ce")
            engine_registry_paths[e.id] = ce_registry_path;
        else if (e.id == "ee_prt")
            engine_registry_paths[e.id] = prt_registry_path;
    }

    // Bruecke-I1/M-CE-12: die REAL vorhandenen load_profiles/-ids enumerieren (gleicher co-lokalisierter
    // Default-Pfad wie validate_profile_facade: thesis_profiles/../load_profiles) und als bekannte Workload-
    // Menge hereinreichen — so faellt eine getippte <workloads>-id SCHON hier auf. Existiert das Verzeichnis
    // nicht, bleibt die Menge leer (Pruefung uebersprungen, rueckwaerts-kompatibel).
    std::set<std::string> known_workload_ids;
    if (!profile_path.empty()) {
        std::filesystem::path const load_profile_dir = profile_path.parent_path().parent_path() / "load_profiles";
        for (auto const& idp : wd::discover_load_profiles(load_profile_dir)) known_workload_ids.insert(idp.first);
    }

    tlz::ExperimentValidationResult const vr =
        tlz::validate_experiment_profile(*ep, {}, known_workload_ids, engine_registry_paths);

    os << "=== EXPERIMENT-PROFIL-VALIDAT (rein-lesend; KEIN DLL-Bau, KEINE Messung) ===\n";
    os << "  Experiment id=" << ep->id << " version=" << ep->version << "\n";
    os << "  geprueft: " << vr.engines_checked << " engines, " << vr.phases_checked << " phases, "
       << vr.variants_checked << " allowed_variants, " << vr.categories_checked << " measurement_categories";
    if (vr.workloads_checked > 0) os << ", " << vr.workloads_checked << " workloads";
    os << "\n";
    for (auto const& w : vr.warnings) os << "  [HINWEIS] " << w << "\n";
    for (auto const& e : vr.errors) os << "  [FEHLER]  " << e << "\n";
    if (vr.ok)
        os << "VALIDAT OK: das Experiment-Profil ist gegen die 2-Registry (ce+prt) + MergeStrategy/Kategorien "
              "konsistent.\n";
    else
        os << "VALIDAT FEHLGESCHLAGEN: " << vr.errors.size()
           << " Fehler — Experiment NICHT baubar (Abbruch vor Bau).\n";
    os << "(--validate: rein-lesend — es wurde KEINE DLL gebaut und KEINE Messung durchgefuehrt.)\n";
    return vr.ok ? 0 : 1;
}

ExperimentRunResult run_experiment_profile_facade(ExperimentRunArgs const& args) {
    ExperimentRunResult out;

    // ── (1) Pre-Flight-Validat (I1/I2-Gate) MIT registry_dir + known_workload_ids. Schliesst die Lücke des
    //    ersetzten Parallelstrangs (execute_messreihe validierte mit leerem registry_dir, v32_messreihe_antrieb:264).
    //    Verstoss ⇒ Abbruch VOR jedem Bau (5 = nicht als comdare_experiment lesbar, 1 = Registry-/Struktur-Verstoss). ──
    if (int const vrc = validate_experiment_profile_facade(args.profile_path, args.ce_registry_path,
                                                           args.prt_registry_path, std::cout);
        vrc != 0) {
        std::cerr << "[experiment_facade] Validat fehlgeschlagen (rc=" << vrc << ") -- KEIN Bau, KEINE Messung.\n";
        out.exit_code = vrc;
        return out;
    }

    // ── (2) Experiment-XML fuer die Achse-2-Auswahl (<workloads>) parsen — analog run_profile_facade (tp->workloads). ──
    cx::XmlConfigParser const parser;
    auto const                ep = parser.parse_experiment_profile(args.profile_path);
    if (!ep) { // von (1) bereits ausgeschlossen; defensiv
        out.exit_code = 5;
        return out;
    }

    // ── (3) Achse-2-Lastprofile aufloesen: co-lokalisierter Default (profile/../load_profiles) oder Host-Override,
    //    gefiltert ueber <workloads> — BYTE-gleiches Muster wie run_profile_facade:108-146. 0 gueltige Profile ⇒
    //    exit 4 (Achse 2 darf nicht still entfallen = two_phase_valid=0-Schutz). ──
    std::filesystem::path load_profile_dir = args.load_profile_dir;
    if (load_profile_dir.empty() && !args.profile_path.empty())
        load_profile_dir = args.profile_path.parent_path().parent_path() / "load_profiles";

    std::vector<std::string> const& workload_select = ep->workloads;
    auto const                      is_selected     = [&workload_select](std::string const& id) {
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
        std::cout << "[experiment_facade] Lastprofile (XML, Achse 2, <workloads>-Auswahl): " << workload_values.size()
                  << " aus " << load_profile_dir.string() << "\n";
        if (workload_values.empty()) {
            std::cerr << "[experiment_facade] 0 gueltige Lastprofile fuer die <workloads>-Auswahl in '"
                      << load_profile_dir.string() << "' -- Abbruch (Achse 2 darf nicht still entfallen).\n";
            out.exit_code = 4;
            return out;
        }
    }

    // ── (4) Der EINE Compile-Injektionspunkt (identisch run_profile_facade:153) → Delegation an den umbrella-
    //    schweren Lauf-Unterbau run_experiment_profile (experiment_run_entry.hpp). ──
    tlz::RunExperimentArgs a;
    a.profile_path = args.profile_path;
    a.out_csv      = args.out_csv;
    a.src_dir      = args.src_dir;
    a.dll_dir      = args.dll_dir;
    // opt-g: per-Perm-CompileFn-Fabrik statt EINER festen CompileFn. Der Planer (run_experiment_profile)
    //   permutiert opt_level×simd aus der XML (ep.opt_levels/simd_extensions) und ruft die Fabrik je Perm mit den
    //   aufgelösten Flags. Die include_dirs/defines/cxx/link_libs/fno_gnu_unique-Wahl bleibt Facade-Wissen
    //   (WAS/WIE-Trennung: der Planer permutiert die System-Achsen, die Facade montiert die CompileFn).
    a.compile_for_perm = [inc = perm_include_dirs(), def = perm_mess_defines(), cxx = cxx_compiler(),
                          libs = perm_link_libs(), fno = facade_supports_fno_gnu_unique()](
                             std::string const& opt_flag, std::string const& march_flag) {
        std::string flags = opt_flag; // opt-b-Kanal: eine rsp-Zeile, opt + optional -march (gcc/clang teilen Syntax)
        if (!march_flag.empty()) {
            flags += ' ';
            flags += march_flag;
        }
        return ex::make_gpp_compile_fn(inc, def, cxx, libs, flags, fno);
    };
    a.compiler_tag = cxx_compiler(); // +cxx=-Provenienz im per-Perm-build_version
    // Fallback-Einzel-CompileFn (greift nur, wenn compile_for_perm null wäre) = beweglicher CEB-Default (O3).
    a.compile      = ex::make_gpp_compile_fn(perm_include_dirs(), perm_mess_defines(), cxx_compiler(), perm_link_libs(),
                                             perm_opt_level_cflags(), facade_supports_fno_gnu_unique());
    a.n_ops        = args.n_ops;
    a.max_binaries = args.max_binaries;
    // opt-g: BASIS ohne System-Achsen-Suffix — die Perm-Schleife hängt je opt×simd "+cxx=+opt=+ext=" an
    // (system_axes_version_suffix() bleibt für den Einzel-Pfad run_profile_facade unverändert).
    a.build_version              = args.build_version;
    a.n_repeats                  = args.n_repeats;
    a.cores_per_build            = args.cores_per_build;
    a.min_free_gb                = args.min_free_gb;
    a.resume_override_set        = args.resume_override_set;
    a.resume                     = args.resume;
    a.working_set_override       = args.working_set_override;
    a.platform_override          = args.platform_override;
    a.build_version_tag_override = args.build_version_tag_override;
    a.workload_registry          = std::move(workload_registry);
    a.workload_values            = std::move(workload_values);

    tlz::RunExperimentResult const r = tlz::run_experiment_profile(a);
    out.exit_code                    = r.exit_code;
    out.phases                       = r.phases;
    out.sota_rows                    = r.sota_rows;
    out.sota_binary_ids              = r.sota_binary_ids;
    out.measured                     = r.any_measured;
    out.resumed                      = r.any_resumed;
    return out;
}

} // namespace comdare::cache_engine::builder::profile_facade
