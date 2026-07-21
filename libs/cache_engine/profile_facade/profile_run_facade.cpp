// profile_run_facade.cpp -- die einzige umbrella-ziehende Uebersetzungseinheit
// der produktiven run_profile-Fassade.

#include "profile_run_facade.hpp"

#include "profile_run_entry.hpp"
#include "experiment_run_entry.hpp" // Brücke-I4: run_experiment_profile (comdare_experiment-Lauf-Unterbau)
#include "validate_profile.hpp"     // P5: axis_registry_from_levels / validate_profile / print_validation_report

#include "xml_config_parser/xml_config_parser.hpp" // Bruecke-I2: XmlConfigParser / ExperimentProfile
#include "planner/experiment_plan_director.hpp" // W5-B: ExperimentPlanDirector/PlanTextBuilder (katalog-schwer -> NUR hier)

#include <cache_engine/measurement/compiler_system_axis.hpp> // INC-1h: Compiler-System-Achse (gcc|clang)
#include <cache_engine/measurement/simd_sub_axis.hpp> // F-SIMD: simd-Unter-Achse (Flag-Quelle), parent=extension_hardware
#include <cache_engine/measurement/extension_hardware_family_axis.hpp> // GN-1: aktiver extension_hardware-Familien-Knoten
#include <cache_engine/measurement/optimization_level_sub_axis.hpp> // INC-2c.opt-c: opt_level-Unter-Achse (Flag-Quelle)
#include <cache_engine/measurement/compiler_atomic_sub_axis.hpp>    // INC-0: atomic128-Unter-Achse (Cx16Option, -mcx16)
#include <cache_engine/measurement/target_isa_system_axis.hpp>      // INC-2d: target_isa-System-Achse (Cross-Compile)
#include <cache_engine/measurement/simd_build_gate.hpp> // Section 40.a-E4: flag-genaues Bau-Gate (Pruef-Dock, default-permissiv)
#include <axes/alloc/axis_06_allocator_snmalloc.hpp> // INC-0: SnmallocAllocator::vendor_compile_defs() (Organ-Vertrag)
#include <axes/alloc/axis_06_allocator_flags.hpp>    // INC-0: COMDARE_AXIS_06_USE_SNMALLOC (globales Umbrella-Gate)

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // Bauplan §4: ceb_contract_version (+ceb= in build_version)
#include <builder/build_orchestrator/build_orchestrator.hpp>
#include <builder/experiment_tree/axis_variant_version_table.hpp> // Bauplan §4/§5: AlgoSigFn aus compose_algo_signature
#include <builder/experiment_tree/registry_to_axis_levels.hpp>    // P5: build_all_axis_levels (EnabledStrategies)
#include <builder/workload_driver/load_profile_parser.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream> // W5-B: --dump-plan Root-Tag-Sniff (ifstream)
#include <iostream>
#include <map>
#include <memory>
#include <optional> // GN-3: std::optional<cx::ThesisProfile> fuer den <system_axes>-Deklarations-Check
#include <set>
#include <sstream> // W5-B: --dump-plan Root-Tag-Sniff (ostringstream)
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::profile_facade {

namespace ex      = ::comdare::cache_engine::builder::experiment;
namespace tlz     = ::comdare::cache_engine::thesis_lazy;
namespace wd      = ::comdare::cache_engine::builder::workload_driver;
namespace cx      = ::comdare::builder::xml;          // Bruecke-I2: XmlConfigParser / ExperimentProfile
namespace planner = ::comdare::cache_engine::planner; // W5-B: ExperimentPlanDirector / PlanTextBuilder

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

// DEPRECATED (INC-0, 2026-07-18): der flache CMake-String-Bake COMDARE_FACADE_PERM_EXTRA_CFLAGS mischte snmalloc-
// Organ-Defs + -mcx16-Compiler-Flag in EINEN Kanal. Abgeloest durch perm_alloc_organ_cflags() (Organ-Achse) +
// perm_compiler_isa_cflags() (Compiler-atomic128-Achse), die die Werte single-source aus den Achsen ziehen. NICHT
// mehr konsumiert (perm_compile_flags nutzt die getrennten Kanaele); Bake + diese Funktion bleiben additiv bis zur
// Aufraeum-Absprache. [[maybe_unused]] haelt -Werror gruen.
[[maybe_unused]] [[nodiscard]] std::vector<std::string> baked_perm_extra_cflags() {
#ifdef COMDARE_FACADE_PERM_EXTRA_CFLAGS
    return split_on(COMDARE_FACADE_PERM_EXTRA_CFLAGS, '|');
#else
    return {};
#endif
}

// simd-Unter-Achse der extension_hardware-Haupt-Achse (F-SIMD, Q2-Option-C): die -march-Flag-QUELLE ist die
// SimdSubAxis-Option (compile-time-Reflexion), der ORT ist diese CompileFn-Flag-Kette. Default = no_extension
// (SimdNoExtOption, keine Flags, Ist-Verhalten byte-identisch); die CEB-Laufzeit-Permutation aller Auspraegungen
// kommt mit dem Planer-Strang. Single-XML (9dim-G3, Sec.50): die Einzelpfad-Wahl kommt aus dem Profil
// (<system_axes><extension_hardware><simd>, GENAU EINER), nicht mehr aus COMDARE_PILOT_SIMD_POLICY-Env.
// GN-1-Anker (opt-g-Facade): die hier gezogenen simd-Optionen haengen unter dem AKTIVEN extension_hardware-
// Familien-Knoten (extension_hardware_family_axis.hpp, analog CompilerSystemAxis) -- Label-Drift bricht compile-time.
static_assert(::comdare::cache_engine::measurement::SimdNoExtOption::parent_axis_label() ==
                  ::comdare::cache_engine::measurement::SimdExtensionHardwareFamily::axis_label(),
              "GN-1: die opt-g-Facade zieht simd-Flags nur ueber den aktiven extension_hardware-Knoten.");
[[nodiscard]] std::string_view active_simd_policy(cx::ThesisProfile const* tp = nullptr) {
    namespace cm = ::comdare::cache_engine::measurement;
    // Single-XML (9dim-G3): der EINE deklarierte <simd>-Wert des Profils ist die Quelle (GENAU EINER = Einzelpfad).
    // 0 deklariert -> benannter Achsen-Default no_extension (byte-identisch, keine Flags); >1 traegt der
    // Permutations-Pfad (run_profile), nicht diese Facade-Naht.
    if (tp != nullptr && tp->extension_hardware.simd_options.size() == 1)
        return tp->extension_hardware.simd_options.front();
    return cm::SimdNoExtOption::simd_id();
}

[[nodiscard]] std::vector<std::string> perm_extension_hardware_cflags(cx::ThesisProfile const* tp = nullptr) {
    namespace cm                  = ::comdare::cache_engine::measurement;
    std::string_view const policy = active_simd_policy(tp);
    std::string_view       flag;
    if (policy == cm::SimdAvx2Option::simd_id()) {
        flag = cm::SimdAvx2Option::gcc_march_flag();
    } else if (policy == cm::SimdAvx512Option::simd_id()) {
        flag = cm::SimdAvx512Option::gcc_march_flag();
    } else if (policy != cm::SimdNoExtOption::simd_id()) {
        std::cerr << "[profile_facade] simd-Politik '" << policy << "' unbekannt; nutze no_extension (generisch).\n";
    }
    if (flag.empty()) return {};
    return {std::string{flag}};
}

// INC-2d: target_isa-System-Achse -- die ZIEL-ISA fuer Cross-Compile (x86->ARM64). Die Cross-Flags (-target/-march)
// kommen single-source aus der TargetIsaSystemAxis-Auspraegung; Default X86_64TargetIsa = host==target = KEINE Flags
// (golden byte-identisch). Der echte aarch64-Lauf braucht zusaetzlich den Cross-Treiber (Toolchain-Handover).
// Single-XML (9dim-G3, Sec.50): der Einzelpfad-Wert kommt aus dem Profil (<system_axes><target_isa>, GENAU EINER),
// nicht mehr aus COMDARE_PILOT_TARGET_ISA-Env; target_isa wird NICHT permutiert (eigene Haupt-Achse, host-weite
// System-Config).
[[nodiscard]] std::string_view active_target_isa(cx::ThesisProfile const* tp = nullptr) {
    namespace cm = ::comdare::cache_engine::measurement;
    if (tp != nullptr && tp->target_isa.isa.size() == 1) return tp->target_isa.isa.front();
    return cm::X86_64TargetIsa::target_isa_id();
}

[[nodiscard]] std::vector<std::string> perm_target_isa_cflags(cx::ThesisProfile const* tp = nullptr) {
    namespace cm                  = ::comdare::cache_engine::measurement;
    std::string_view const target = active_target_isa(tp);
    if (target == cm::Aarch64TargetIsa::target_isa_id()) {
        // "-target aarch64-linux-gnu" ist ZWEI Tokens -> auf Whitespace splitten (je Element ein Compiler-Arg).
        std::vector<std::string> out = split_on(std::string{cm::Aarch64TargetIsa::target_triple()}, ' ');
        if (!cm::Aarch64TargetIsa::target_march().empty()) out.emplace_back(cm::Aarch64TargetIsa::target_march());
        return out;
    }
    if (target != cm::X86_64TargetIsa::target_isa_id())
        std::cerr << "[profile_facade] target_isa '" << target
                  << "' unbekannt; nutze x86_64 (host==target, kein Cross).\n";
    return {}; // x86_64 (Default) = native = KEINE Cross-Flags (golden byte-identisch)
}

// H-10 (Bau-INC-2c, W9.1-Konformitaets-Nachzug): die telemetry-System-Achse (TelemetryConfig Active/Silent,
// telemetry_mode.hpp) ist eine CEB-System-Achsen-Belegung -- ihre Wahl gehoert in die per-Binary-Provenienz
// (.version-Sidecar, kein binary_id-Segment; registry_to_axis_levels.hpp:109). A9.3 (Sec.50, 9dim-G3): der
// COMDARE_PILOT_TELEMETRY-Env-Schalter entfaellt; Rueckgabe bleibt der benannte Default Active (== false),
// UNVERAENDERT. BEWUSST KEIN Profil-Wiring hier: telemetry IST eine Registry-System-Achse (build_system_axis_
// levels, T10_telemetry), UND die golden-320-Profile deklarieren <telemetry silent="true"> -- ein Durchreichen
// von telemetry_silent haenge +tel=silent ans golden-build_version und BRECHE die Byte-Identitaet (das Alt-
// Verhalten env-unset=>Active hat den XML-Wert nie getragen). Das telemetry_silent-Wiring ist damit ein bewusst
// golden-BRECHENDER Folge-Schritt (eigene Absprache), NICHT Teil der golden-neutralen A9.3. Default Active =>
// KEIN +tel=-Token => build_version byte-identisch.
[[nodiscard]] bool active_telemetry_is_silent() {
    return false; // Default = Active (TelemetryMode::Active); A9.3 golden-neutral: kein Profil-Wiring (s.o.)
}

// INC-0: Allokator-ORGAN-Kanal -- der snmalloc-Vendor-Build-Vertrag (SNMALLOC_*-INTERFACE-Defs). Werte single-source
// aus der Organ-Achse (SnmallocAllocator::vendor_compile_defs()), NICHT mehr CMake-string-gebacken. Gate = das GLOBALE
// COMDARE_AXIS_06_USE_SNMALLOC (NIE per-Tier: der Umbrella zieht snmalloc.h in JEDE TU -> alle Tiers brauchen den
// Vertrag, sonst ds/aba.h-#error). In conf/go2 (USE_SNMALLOC=0) inert -> byte-neutral zum Ist-Zustand.
[[nodiscard]] std::vector<std::string> perm_alloc_organ_cflags() {
#if defined(COMDARE_AXIS_06_USE_SNMALLOC) && COMDARE_AXIS_06_USE_SNMALLOC
    std::vector<std::string> out;
    for (auto const sv : ::comdare::cache_engine::alloc::SnmallocAllocator::vendor_compile_defs()) out.emplace_back(sv);
    return out;
#else
    return {};
#endif
}

// INC-0: Compiler-SYSTEM-Kanal -- -mcx16 (atomic128-Unter-Achse, Cx16Option). Gate = USE_SNMALLOC && x86_64 (snmallocs
// ds/aba.h verlangt CMPXCHG16B; -mcx16 ist x86_64-only). Freigabe-Prinzip: die Compiler-Achse GIBT -mcx16 frei, das
// snmalloc-Organ SETZT es durch. Wert single-source aus der Achse (gcc/clang teilen -mcx16). In conf/go2 inert.
[[nodiscard]] std::vector<std::string> perm_compiler_isa_cflags() {
#if defined(COMDARE_AXIS_06_USE_SNMALLOC) && COMDARE_AXIS_06_USE_SNMALLOC && defined(COMDARE_ARCH_X86_64)
    std::string_view const flag = ::comdare::cache_engine::measurement::Cx16Option::gcc_flag(); // == clang_flag()
    if (flag.empty()) return {};
    return {std::string{flag}};
#else
    return {};
#endif
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
    // INC-0-Entmischung: perm_mess_defines() traegt NUR noch Mess-/OS-/Arch-/Cache-Line-Defines. Die Achsen-Flags
    // (Allokator-Organ, Compiler-atomic128, extension_hardware-SIMD) montiert perm_compile_flags() getrennt.
    return d;
}

// INC-0: der EINE Compile-Flag-Assembler fuer die Tier-Binary-Subprozesse -- macht die WAS/WIE-Schicht-Trennung
// SICHTBAR statt eines flachen Misch-Vektors: (1) Mess-/OS-/Arch-Defines (perm_mess_defines), (2) Allokator-ORGAN-
// Defs (snmalloc-Vertrag), (3) Compiler-SYSTEM-Flag -mcx16 (atomic128-Achse), (4) extension_hardware-SIMD -march.
[[nodiscard]] std::vector<std::string> perm_compile_flags(cx::ThesisProfile const* tp = nullptr) {
    std::vector<std::string> d = perm_mess_defines();
    for (auto& f : perm_alloc_organ_cflags()) d.push_back(std::move(f));
    for (auto& f : perm_compiler_isa_cflags()) d.push_back(std::move(f));
    for (auto& f : perm_extension_hardware_cflags(tp)) d.push_back(std::move(f));
    for (auto& f : perm_target_isa_cflags(tp)) d.push_back(std::move(f)); // INC-2d: Ziel-ISA (Cross-Compile)
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
// der benannten Single-Source DefaultOptLevelOption (=O3); Single-XML (9dim-G3, Sec.50) + XML/Planer (A3) bewegen
// JEDES Teil (nicht mehr COMDARE_PILOT_OPT_LEVEL-Env). Ofast/O0/O1/O2 leben additiv als +opt=-Sidecar-Extreme.
[[nodiscard]] std::string_view active_opt_level(cx::ThesisProfile const* tp = nullptr) {
    // Single-XML (9dim-G3): GENAU EIN deklarierter <opt_level> -> dieser Wert; sonst die benannte Achsen-Single-
    // Source (kein rohes Literal, kein Pin) = "O3". Mehrere opt_levels traegt der Permutations-Pfad (run_profile).
    if (tp != nullptr && tp->compiler.opt_levels.size() == 1) return tp->compiler.opt_levels.front();
    return ::comdare::cache_engine::measurement::DefaultOptLevelOption::opt_level_id();
}

[[nodiscard]] std::string perm_opt_level_cflags(cx::ThesisProfile const* tp = nullptr) {
    namespace cm                 = ::comdare::cache_engine::measurement;
    std::string_view const level = active_opt_level(tp);
    bool const             clang = cxx_compiler().find("clang") != std::string::npos;
    auto pick = [&](std::string_view gcc, std::string_view cl) { return std::string{clang ? cl : gcc}; };
    if (level == cm::OptO0Option::opt_level_id())
        return pick(cm::OptO0Option::gcc_opt_flag(), cm::OptO0Option::clang_opt_flag());
    if (level == cm::OptO1Option::opt_level_id())
        return pick(cm::OptO1Option::gcc_opt_flag(), cm::OptO1Option::clang_opt_flag());
    if (level == cm::OptO2Option::opt_level_id())
        return pick(cm::OptO2Option::gcc_opt_flag(), cm::OptO2Option::clang_opt_flag());
    if (level == cm::OptO3Option::opt_level_id())
        return pick(cm::OptO3Option::gcc_opt_flag(), cm::OptO3Option::clang_opt_flag());
    if (level == cm::OptOfastOption::opt_level_id())
        return pick(cm::OptOfastOption::gcc_opt_flag(), cm::OptOfastOption::clang_opt_flag());
    // Fehlerklasse (INC-29.0, KonfigXmlParse-Nachbar): unbekannter Smoke-Wert -> sichtbar degradiert, NIE leer
    // (kein impliziter Compiler-Default /Od), NIE harter exit. Fallback = der bewegliche CEB-Default (O3), NICHT
    // ein O2-Pin. Formale D1-Log-Klassifikation an der Build-Naht folgt INC-29.2/d1-log.
    std::cerr << "[profile_facade] opt_level '" << level << "' unbekannt; nutze CEB-Default "
              << cm::DefaultOptLevelOption::opt_level_id() << ".\n";
    return pick(cm::DefaultOptLevelOption::gcc_opt_flag(), cm::DefaultOptLevelOption::clang_opt_flag());
}

// H-10 (Bau-INC-1g): die VARIABLEN System-Achsen-Belegungen (Erweiterungshardware-Politik,
// Compiler, opt_level) werden in build_version kodiert — eine unter anderer Belegung gebaute DLL bekommt
// ein eigenes .version-Sidecar (kein falsches Skip via dll_is_current) und die CSV-Spalte
// build_version traegt die Provenienz. Konstante Achsen (Scheduling/Last=Default) bleiben
// weggelassen, bis die CEB-Laufzeit-Permutation sie variabel macht.
[[nodiscard]] std::string system_axes_version_suffix(cx::ThesisProfile const* tp = nullptr) {
    // A1/OF-2 (Ruling 2026-07-18): KEIN globaler Byte-Anker mehr. Die opt_level-Provenienz wird IMMER emittiert
    // (kein O2-Sonderfall) -> jedes Teil beweglich, keine bevorzugte Referenz-Stufe. Folge: alle Tier-Binaries
    // tragen +opt=<level> (Default +opt=O3) -> dll_is_current sieht sie unter neuer Belegung als neu; die golden-
    // Reihe wird deterministisch unter O3 neu gebaut/gemessen (bewusster Neu-Mess-Lauf, alt-Reihen additiv erhalten).
    // Single-XML (9dim-G3, Sec.50): die vier active_*-Aufloeser ziehen die Einzelpfad-Wahl aus dem Profil (tp), nicht
    // mehr aus Env; kein Profil / keine Deklaration -> benannte Defaults -> Suffix byte-identisch (golden-neutral).
    std::string suffix = "+ext=" + std::string{active_simd_policy(tp)} + "+cxx=" + cxx_compiler();
    suffix += "+opt=";
    suffix += active_opt_level(tp);
    // Bauplan §4 (inkrementeller Cache): die CEB-Contract-Version (Framework/System-Ebene) faltet sich in die
    // build_version -> jeder Bump (ABI-Major AUTOMATISCH ueber COMDARE_ANATOMY_ABI_MAJOR, codegen-Minor manuell/
    // CI-Tripwire) laesst jede perm.dll.version mismatchen -> ALLE Tier-Binaries neu ("CEB-Aenderung betrifft alle").
    // Konsistent zum Loader-host_compatible_with-Major-Backstop. Organ-Provenienz bleibt STRIKT getrennt (perm.algos).
    suffix += "+ceb=" + std::to_string(COMDARE_ANATOMY_ABI_MAJOR) + "." +
              std::to_string(::comdare::cache_engine::abi::kCebContractCodegenMinor);
    // INC-2d: Cross-Compile-Provenienz NUR wenn Ziel != Host (native x86_64 = kein Suffix -> build_version
    // byte-identisch, golden-neutral). Ziel-ISA ist system_config -> .version-Sidecar, NIE binary_id.
    if (std::string_view const t = active_target_isa(tp);
        t != ::comdare::cache_engine::measurement::X86_64TargetIsa::target_isa_id())
        suffix += "+target=" + std::string{t};
    // H-10 (W9.1): telemetry-System-Achsen-Provenienz. REGISTRY-GEGATED -- der Token wird NUR emittiert, wenn
    // "telemetry" wirklich als System-Achse gefuehrt wird. Damit ist build_system_axis_levels() (bislang ausser der
    // Byte-Identitaets-Fold in build_all_axis_levels() ohne echten Konsumenten -- Audit-Auflage A, 2026-07-17) ein
    // ECHTER Produktions-Konsument: verschwaende die telemetry-System-Achse aus der Registry, entfiele der Token
    // automatisch (Anti-Drift). NUR bei Silent (!= Default Active) -> Default byte-identisch (golden-neutral).
    if (active_telemetry_is_silent()) { // A9.3: Default Active (kein Profil-Wiring) -> golden byte-identisch
        auto const system_levels            = ex::build_system_axis_levels();
        bool const telemetry_is_system_axis = std::any_of(system_levels.begin(), system_levels.end(),
                                                          [](ex::AxisLevel const& l) { return l.axis == "telemetry"; });
        if (telemetry_is_system_axis) suffix += "+tel=silent";
    }
    // (i) §61-STUFEN Compile-Kennzeichnung: +bt=Debug NUR bei Debug-Build (COMDARE_BUILD_TYPE=Debug, Emissions-Seite
    // im Director). Release/Default => "" => build_version byte-identisch (golden/Sidecar/Resume unberuehrt).
    suffix += tlz::build_type_version_suffix();
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
    std::optional<cx::ThesisProfile> const tp_opt = tlz::load_thesis_profile(args.profile_path);
    std::vector<std::string>               workload_select;
    if (tp_opt) workload_select = tp_opt->workloads;
    // Single-XML (9dim-G3, Sec.50): der Einzelpfad loest die System-Achsen-Wahl (opt/simd/target_isa/telemetry) aus
    // dem GEPARSTEN Profil auf, nicht mehr aus COMDARE_PILOT_*-Env. Nullbarer Zeiger an die active_*-/perm_*-
    // Aufloeser; kein/leeres Profil => benannte Achsen-Defaults (byte-identisch). Der Permutations-Pfad unten
    // liest opt/simd ohnehin direkt aus dem Profil (compile_for_perm) -- daher hier NUR die Einzelpfad-Naht.
    cx::ThesisProfile const* const tp_ptr = tp_opt ? &*tp_opt : nullptr;
    // GN-3 (§33 Systembeweis-Traeger, 2026-07-19): deklariert das Profil <system_axes> (opt_level/simd), permutiert
    // run_profile sie SELBST (opt×simd-Walk) und haengt je Kombination das +cxx=+opt=+ext=-Suffix ans build_version.
    // Dann darf die BASIS-build_version den system_axes_version_suffix() NICHT tragen (sonst doppelte Provenienz) —
    // exakt wie run_experiment_profile_facade. OHNE <system_axes> bleibt der Einzel-Pfad byte-identisch.
    bool const profile_has_system_axes =
        tp_opt && (!tp_opt->compiler.opt_levels.empty() || !tp_opt->extension_hardware.simd_options.empty());
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
        perm_include_dirs(), perm_compile_flags(tp_ptr), cxx_compiler(), perm_link_libs(),
        perm_opt_level_cflags(tp_ptr),     // opt-c: opt_level-Flag (Default O3, beweglich; Single-XML aus tp)
        facade_supports_fno_gnu_unique()); // opt-d: Dialekt-Gate als Wert (kein Sniff im Builder)
    // Bauplan §5/§7: die AlgoSigFn aus der compile-time Versions-Tabelle (axis_variant_version_table). Der
    // Orchestrator berechnet damit je Binary die Organ-Signatur (perm.algos) und gated Rebuild + Neu-Messung. Die
    // Tabelle wird EINMAL gebaut (dieser TU zieht ohnehin alle 17 Registries) und per shared_ptr in der Closure
    // gehalten. Leer waere Organ-Gate aus; hier IMMER gesetzt -> der produktive Mess-Pfad cached organ-genau.
    {
        auto algo_table = std::make_shared<std::vector<ex::AxisVariantVersion>>(ex::build_axis_variant_version_table());
        a.algo_sig      = [algo_table](std::vector<std::pair<std::string, std::string>> const& axes) {
            return ex::compose_algo_signature(axes, *algo_table);
        };
    }
    a.n_ops        = args.n_ops;
    a.max_binaries = args.max_binaries;
    // GN-3 (§33, 2026-07-19): build_version + opt×simd-Kanal je nach <system_axes>-Deklaration (profile_has_system_axes).
    if (profile_has_system_axes) {
        // Die opt×simd-Perm-Schleife in run_profile haengt je Kombination +cxx=+opt=+ext= an → BASIS OHNE
        // system_axes_version_suffix() (Spiegel run_experiment_profile_facade). compile_for_perm montiert je Perm
        // die CompileFn aus den aufgeloesten Flags (WAS/WIE-Trennung: run_profile permutiert, die Facade montiert;
        // include_dirs/defines/cxx/link_libs/fno_gnu_unique bleiben Facade-Wissen).
        a.build_version = args.build_version;
        a.compiler_tag  = cxx_compiler(); // +cxx=-Provenienz im per-Perm-build_version
        a.compile_for_perm =
            [inc = perm_include_dirs(), def = perm_compile_flags(), cxx = cxx_compiler(), libs = perm_link_libs(),
             fno = facade_supports_fno_gnu_unique()](std::string const& opt_flag, std::string const& march_flag) {
                std::string flags =
                    opt_flag; // opt-b-Kanal: eine rsp-Zeile, opt + optional -march (gcc/clang teilen Syntax)
                if (!march_flag.empty()) {
                    flags += ' ';
                    flags += march_flag;
                }
                // Section 40.a-E4: flag-genaues Bau-Gate an der CompileFn-Naht. Default-permissiv -- solange kein
                // Organ required-Flags deklariert, ist die aktive Anforderung leer -> Pruef-Dock NotApplicable ->
                // KEINE Zusatz-Flags (byte-identisch zum Ist). Aktiviert, sobald Organe required-Flags erklaeren.
                for (auto const& mf : ::comdare::cache_engine::measurement::gate_extra_march_flags_for_build(
                         ::comdare::cache_engine::measurement::route_of_march_flag(march_flag))) {
                    flags += ' ';
                    flags += mf;
                }
                return ex::make_gpp_compile_fn(inc, def, cxx, libs, flags, fno);
            };
    } else {
        a.build_version = args.build_version + system_axes_version_suffix(tp_ptr); // Einzel-Pfad byte-identisch
    }
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
    a.golden_range_start         = args.golden_range_start; // INC-G6: Chunk-Fenster durchreichen (inert bei count==0)
    a.golden_range_count         = args.golden_range_count;
    a.provision_only             = args.provision_only; // INC-G6: provision-only durchreichen (inert bei false)
    a.build_parallelism   = args.build_parallelism; // W6 (§32-F7): Bau-Pool-Override durchreichen (0 = byte-neutral)
    a.gn_cell_opt         = args.gn_cell_opt;       // W5-C+ (§36.1): GN-Zellen-Filter (leer = kein Filter)
    a.gn_cell_simd        = args.gn_cell_simd;      // W5-C+ (§36.1): GN-Zellen-Filter (leer = kein Filter)
    a.workload_registry   = std::move(workload_registry);
    a.workload_values     = std::move(workload_values);
    a.cache_push          = args.cache_push;          // Storage #51: No-Op-Naht durchreichen (byte-neutral)
    a.measurement_sink    = args.measurement_sink;    // Storage #51: perm.dll->Store (B) / CSV->measure-drop (C)
    a.partial_marker_sink = args.partial_marker_sink; // W11 (§43.c): BAU-Modus Teil-Marker durchreichen (No-Op-Default)
    a.chunk_part_size     = args.chunk_part_size;     // W11 (§43.c): Teil-Marker-Intervall N (0 = keine)
    a.progress_sink =
        args.progress_sink; // Welle 5 (E-W5-2): §38-Fortschritts-Rueck-Kanal (No-Op-Default => byte-neutral)

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

namespace {
// GETEILTE Naht der --dump-plan/--dump-ci/--dump-cmake-Fassaden (W5-B/W7-A/W7-B): Root-Tag-Sniff ueber den
// common-DOM (analog main.cpp:675-680) + Parse + EINER Director-Walk in den uebergebenen ConcreteBuilder.
// <comdare_thesis_profile> -> Thesis-Kanal, <comdare_experiment> -> Experiment-Kanal. Beide Parser liefern
// nullopt bei Fremd-Tag, daher ist der reine Root-Tag-Read gefahrlos. KEIN DLL-Bau, KEINE Messung, KEINE CSV
// (Anti-Phantom, golden-neutral). Ohne Registry-Trio-Annotation (loaded=0): host-/registry-pfad-unabhaengig
// reproduzierbar. Rueckgabe: 0 = Walk in den Builder gefahren, 5 = Profil nicht als bekannte Wurzel lesbar.
int construct_plan_into(std::filesystem::path const& profile_path, planner::IPlanBuilder& builder, std::ostream& os,
                        char const* what, std::string const& combo_selector = {}) {
    std::string root_tag;
    if (std::ifstream in{profile_path, std::ios::binary}; in) {
        std::ostringstream ss;
        ss << in.rdbuf();
        if (auto const root = ::comdare::common::xml::parse_document(ss.str())) root_tag = root->tag;
    }

    // S3 P-RESOLVER (A1 Produktions-Aktivierung, 2026-07-20): das RegistryTrio (Organ/System/Mess) aus den per
    // CMake-Interface hereingereichten STATISCHEN ce-Registry-Pfaden lesen (feedback_ceb_config_cmake_interface_
    // static_registry_paths_prt_module). Erfolg -> der Director traegt den VOLLEN Trio (Full-Trio-Ctor): der
    // Resolver LAEUFT und annotiert den Plan-Kopf (resolved=1, Organ-Position-Refs klassifiziert). Fehler
    // (fehlende/unlesbare Registry) -> graceful Default-Ctor (INERT-Annotation resolved=0), NIE Crash. E-1: reine
    // Plan-Kopf-Annotation, KEIN Exit!=0 im run/build-Pfad (kein harter --validate-Gate hier) -> golden-neutral.
    // Guaranteed copy elision (C++17): der IIFE-Prvalue initialisiert `director` direkt, kein Move noetig.
    auto const director = []() -> planner::ExperimentPlanDirector {
#if defined(COMDARE_CE_AXIS_REGISTRY) && defined(COMDARE_SYSTEM_AXIS_REGISTRY) &&                                      \
    defined(COMDARE_MEASUREMENT_AXIS_REGISTRY)
        if (auto trio = tlz::read_axis_registry_trio(COMDARE_CE_AXIS_REGISTRY, COMDARE_SYSTEM_AXIS_REGISTRY,
                                                     COMDARE_MEASUREMENT_AXIS_REGISTRY))
            return planner::ExperimentPlanDirector{std::move(*trio)};
#endif
        return planner::ExperimentPlanDirector{};
    }();
    if (root_tag == "comdare_thesis_profile") {
        auto const tp = tlz::load_thesis_profile(profile_path);
        if (!tp) {
            os << "[" << what << "] Thesis-Profil '" << profile_path.string()
               << "' nicht lesbar (parse_thesis_profile=nullopt). KEIN Plan emittiert.\n";
            return 5;
        }
        director.construct(*tp, builder, combo_selector); // A5: leer => Identitaet (byte-stabil), s. emit_tier_ci
        return 0;
    }
    if (root_tag == "comdare_experiment") {
        cx::XmlConfigParser const parser;
        auto const                ep = parser.parse_experiment_profile(profile_path);
        if (!ep) {
            os << "[" << what << "] Experiment-Profil '" << profile_path.string()
               << "' nicht als comdare_experiment lesbar (parse_experiment_profile=nullopt). KEIN Plan emittiert.\n";
            return 5;
        }
        director.construct(*ep, builder, combo_selector); // A5: leer => Identitaet (byte-stabil), s. emit_tier_ci
        return 0;
    }
    os << "[" << what << "] '" << profile_path.string() << "': unbekannte/unlesbare Wurzel"
       << (root_tag.empty() ? "" : " '" + root_tag + "'")
       << " -- weder <comdare_thesis_profile> noch <comdare_experiment>. KEIN Plan emittiert.\n";
    return 5;
}
} // namespace

int dump_experiment_plan_facade(std::filesystem::path const& profile_path, std::ostream& os) {
    // W5-B (--dump-plan): der deterministische PlanTextBuilder-Traeger am geteilten Director-Walk.
    planner::PlanTextBuilder builder;
    int const                rc = construct_plan_into(profile_path, builder, os, "dump-plan");
    if (rc == 0) os << builder.text();
    return rc;
}

int dump_experiment_ci_facade(std::filesystem::path const& profile_path, std::ostream& os) {
    // W7-A (--dump-ci, §40.b): der CiYamlBuilder-Traeger am geteilten Director-Walk. Emittiert die dynamische,
    // Planer-gesteuerte GitLab-Child-Pipeline-YAML (STUFE 1 CEB-Bau-Jobs + STUFE 2 Tier-Emit/Grandchild-Trigger).
    planner::CiYamlBuilder builder;
    int const              rc = construct_plan_into(profile_path, builder, os, "dump-ci");
    if (rc == 0) os << builder.text();
    return rc;
}

int dump_experiment_cmake_facade(std::filesystem::path const& profile_path, std::ostream& os) {
    // W7-B/W10-A (--dump-cmake, §40.c/§42): der CMakeGraphBuilder-Traeger (STUFE 1, Planer-Rolle) am geteilten
    // Director-Walk. Emittiert das Bare-Metal-experiment_plan.cmake der Mess-Achsen-Stufe (CEB-Bau + CEB-Emit).
    planner::CMakeGraphBuilder builder;
    int const                  rc = construct_plan_into(profile_path, builder, os, "dump-cmake");
    if (rc == 0) os << builder.text();
    return rc;
}

int emit_tier_ci_facade(std::filesystem::path const& profile_path, std::ostream& os,
                        std::string const& combo_selector) {
    // W10-A (--emit-tier-ci, §42/§42.b): der TierCiYamlBuilder-Traeger (STUFE 2, CEB-Rolle) am geteilten
    // Director-Walk. Emittiert NUR die Stufe-2-Sicht des freigegebenen CEB-Raums (System-Perms + Tier-Chunk-Jobs
    // + GN-11/320er-gegatete Mess-Jobs). CEB-Hoheit (§40.b-Praezisierung); heute EINE Binary in zwei Rollen.
    // A5 (§56-T2-FANOUT D4): der combo_selector reicht bis zum Director-Walk durch (leer => Identitaet, byte-stabil).
    planner::TierCiYamlBuilder builder;
    int const                  rc = construct_plan_into(profile_path, builder, os, "emit-tier-ci", combo_selector);
    if (rc == 0) os << builder.text();
    return rc;
}

int emit_tier_cmake_facade(std::filesystem::path const& profile_path, std::ostream& os,
                           std::string const& combo_selector) {
    // W10-A (--emit-tier-cmake, §42/§42.b): der TierCmakeGraphBuilder-Traeger (STUFE 2, CEB-Rolle) am geteilten
    // Director-Walk. Emittiert das Bare-Metal-tier_plan.cmake (reale provision-only-Tier-Chunk-Bau-Targets +
    // GN-11/320er-gegatetes measure:-Skelett) -- der Ort des Tier-Baus in der dreistufigen Bare-Metal-Kette.
    // A8(a)-Symmetrie: der combo_selector reist bis zum Director-Walk durch, exakt wie in emit_tier_ci_facade
    // (leer => Identitaet, byte-stabil zur heutigen 1-CEB-Strecke).
    planner::TierCmakeGraphBuilder builder;
    int const rc = construct_plan_into(profile_path, builder, os, "emit-tier-cmake", combo_selector);
    if (rc == 0) os << builder.text();
    return rc;
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
    a.compile_for_perm = [inc = perm_include_dirs(), def = perm_compile_flags(), cxx = cxx_compiler(),
                          libs = perm_link_libs(), fno = facade_supports_fno_gnu_unique()](
                             std::string const& opt_flag, std::string const& march_flag) {
        std::string flags = opt_flag; // opt-b-Kanal: eine rsp-Zeile, opt + optional -march (gcc/clang teilen Syntax)
        if (!march_flag.empty()) {
            flags += ' ';
            flags += march_flag;
        }
        // Section 40.a-E4: flag-genaues Bau-Gate an der CompileFn-Naht. Default-permissiv -- solange kein Organ
        // required-Flags deklariert, ist die aktive Anforderung leer -> Pruef-Dock NotApplicable -> KEINE
        // Zusatz-Flags (byte-identisch zum Ist). Aktiviert, sobald Organe required-Flags erklaeren.
        for (auto const& mf : ::comdare::cache_engine::measurement::gate_extra_march_flags_for_build(
                 ::comdare::cache_engine::measurement::route_of_march_flag(march_flag))) {
            flags += ' ';
            flags += mf;
        }
        return ex::make_gpp_compile_fn(inc, def, cxx, libs, flags, fno);
    };
    a.compiler_tag = cxx_compiler(); // +cxx=-Provenienz im per-Perm-build_version
    // Bauplan §5/§7: dieselbe AlgoSigFn wie der Profile-Pfad -> auch der XML-Experiment-Lauf cached organ-genau.
    {
        auto algo_table = std::make_shared<std::vector<ex::AxisVariantVersion>>(ex::build_axis_variant_version_table());
        a.algo_sig      = [algo_table](std::vector<std::pair<std::string, std::string>> const& axes) {
            return ex::compose_algo_signature(axes, *algo_table);
        };
    }
    // Fallback-Einzel-CompileFn (greift nur, wenn compile_for_perm null wäre) = beweglicher CEB-Default (O3).
    a.compile = ex::make_gpp_compile_fn(perm_include_dirs(), perm_compile_flags(), cxx_compiler(), perm_link_libs(),
                                        perm_opt_level_cflags(), facade_supports_fno_gnu_unique());
    a.n_ops   = args.n_ops;
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
    a.build_parallelism   = args.build_parallelism; // W6 (§32-F7): Bau-Pool-Override durchreichen (0 = byte-neutral)
    a.gn_cell_opt         = args.gn_cell_opt;       // W5-C+ (§36.1): GN-Zellen-Filter (Spiegel; leer = kein Filter)
    a.gn_cell_simd        = args.gn_cell_simd;      // W5-C+ (§36.1): GN-Zellen-Filter (Spiegel; leer = kein Filter)
    a.workload_registry   = std::move(workload_registry);
    a.workload_values     = std::move(workload_values);
    a.cache_push          = args.cache_push;          // Storage #51: No-Op-Naht durchreichen (byte-neutral)
    a.measurement_sink    = args.measurement_sink;    // Storage #51: perm.dll->Store (B) / CSV->measure-drop (C)
    a.partial_marker_sink = args.partial_marker_sink; // W11 (§43.c): BAU-Modus Teil-Marker durchreichen (No-Op-Default)
    a.chunk_part_size     = args.chunk_part_size;     // W11 (§43.c): Teil-Marker-Intervall N (0 = keine)
    a.progress_sink =
        args.progress_sink; // Welle 5 (E-W5-2): §38-Fortschritts-Rueck-Kanal (No-Op-Default => byte-neutral)

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
