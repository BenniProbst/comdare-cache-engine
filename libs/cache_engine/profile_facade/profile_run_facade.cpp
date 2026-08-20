// profile_run_facade.cpp -- die einzige umbrella-ziehende Uebersetzungseinheit
// der produktiven run_profile-Fassade.

#include "profile_run_facade.hpp"

#include "profile_run_entry.hpp"
#include "experiment_run_entry.hpp"    // Brücke-I4: run_experiment_profile (comdare_experiment-Lauf-Unterbau)
#include "validate_profile.hpp"        // P5: axis_registry_from_levels / validate_profile / print_validation_report
#include "g1_binary_version_stamp.hpp" // K7b-4/G1: g1_binary_version_block (Je-Binary-Selbst-Stempel, --version)

#include "xml_config_parser/xml_config_parser.hpp" // Bruecke-I2: XmlConfigParser / ExperimentProfile
#include "planner/experiment_plan_director.hpp" // W5-B: ExperimentPlanDirector/PlanTextBuilder (katalog-schwer -> NUR hier)
#include "planner/planner_cli_env.hpp"          // check-size: env_trimmed (COMDARE_GN_TOTAL)
#include "planner/planner_mengen_types.hpp"     // check-size: MengenEingang (die flache POD-Naht)
#include <harness/drift_gated_cell.hpp>         // check-size: DriftGateConfig -- die PRODUKTIVEN Drift-Defaults

#include <system_axes/compiler_system_axis.hpp>        // INC-1h: Compiler-System-Achse (gcc|clang)
#include <system_axes/simd_sub_axis.hpp>               // F-SIMD: simd-Unter-Achse (Flag-Quelle), parent=external_utils
#include <system_axes/external_utils_family_axis.hpp>  // GN-1: aktiver external_utils-Familien-Knoten
#include <system_axes/optimization_level_sub_axis.hpp> // INC-2c.opt-c: opt_level-Unter-Achse (Flag-Quelle)
#include <system_axes/compiler_atomic_sub_axis.hpp>    // INC-0: atomic128-Unter-Achse (Cx16Option, -mcx16)
#include <system_axes/target_isa_system_axis.hpp>      // INC-2d: target_isa-System-Achse (Cross-Compile)
#include <cache_engine/measurement/simd_build_gate.hpp> // Section 40.a-E4: flag-genaues Bau-Gate (Pruef-Dock, default-permissiv)

#include "system_version_suffix.hpp"       // Lane F R3: die EINE Suffix-Quelle (Segment-Ordnung deklarativ)
#include "maschinen_deklarations_naht.hpp" // S-3c: DER EINE Aufruf der aktiven Maschinen-Belegung (hint -> Tupel)
#include "overlay_source_hash_naht.hpp"    // E-E: der LIVE-Wert des Preimage-Glieds [7] + sein Define-Argument
#include "system_cell_values_naht.hpp"     // W10-C4: Zellwert-Aufloesung + Define-Argument (die EINE Wertform)
#include "system_axes_entscheidung.hpp"    // T-1: die EINE <system_axes>-Entscheidung + ihre Zwei-Parse-Wache
#include "toolchain_stamp_naht.hpp"        // NB/CX-4: die LIVE-Werte der Preimage-Glieder [5]/[6] + ihre Define-Args
#include "mess_achsen_naht.hpp"            // M-1/D-1: die Mess-Achse -> Tier-Defines (Stufe-2->Stufe-3-Naht)
// INC-0: SnmallocAllocator::vendor_compile_defs() (Organ-Vertrag)
#include <organ_axes/alloc/axis_06_allocator_snmalloc.hpp>
#include <organ_axes/alloc/axis_06_allocator_flags.hpp> // INC-0: COMDARE_AXIS_06_USE_SNMALLOC (globales Umbrella-Gate)

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>  // Bauplan §4: ceb_contract_version (+ceb= in build_version)
#include <builder/bestandslog/artifact_cache_transport.hpp> // G4b-1 (#46b I1): make_bestand_transport -- NUR hier, die
                                                            // eine Umbrella-TU (zieht den ce-XML-DOM nach)
#include <builder/bestandslog/builder_registration.hpp>     // G4b-2: store_reservation_locked / make_lock_owner /
// default_lock_ttl_s / utc_iso_from_epoch (der gelockte Weg)
#include <builder/bestandslog/planer_block_value.hpp> // G4b-2/E4: make_planer_block_reservation_value (der Wert-Kern)
#include <builder/ceb_version_stamp.hpp> // G4b-2/E3: kCebFingerprint (die 128-hex-CEB-SHA512 fuer ceb_key_sha512)
#include <builder/build_orchestrator/build_orchestrator.hpp>
#include <builder/experiment_tree/axis_variant_version_table.hpp> // Bauplan §4/§5: AlgoSigFn aus compose_algo_signature
#include <builder/experiment_tree/registry_to_axis_levels.hpp>    // P5: build_all_axis_levels (EnabledStrategies)
#include <builder/workload_driver/load_profile_parser.hpp>

#include <algorithm>
#include <cerrno> // check-size: strtoull-Fehlerpruefung fuer COMDARE_GN_TOTAL
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

namespace bl      = ::comdare::cache_engine::builder::bestandslog; // G4b-1: make_bestand_transport (die T1-Kante)
namespace ex      = ::comdare::cache_engine::builder::experiment;
namespace tlz     = ::comdare::cache_engine::thesis_lazy;
namespace wd      = ::comdare::cache_engine::builder::workload_driver;
namespace cx      = ::comdare::builder::xml;          // Bruecke-I2: XmlConfigParser / ExperimentProfile
namespace planner = ::comdare::cache_engine::planner; // W5-B: ExperimentPlanDirector / PlanTextBuilder

namespace {

// smoke=>debug-Entkopplung (2026-07-22): das METHODIK-Profil (COMDARE_PLAN_METHODIK_PROFILE) resolven -> dessen
// run_methodology (exactly-one HART validiert). Ein PROFIL-SELEKTOR analog COMDARE_THESIS_PROFILE -- §61: die Methodik
// bleibt profil-getrieben, die Env traegt NICHT den Methodik-Wert (COMDARE_MEASURE_PROFILE bleibt reiner rules-
// Trigger). Rueckgabe {ok, run_methodology}: unset => {true, {}} (kein Override => byte-identisch); unlesbar / >1
// Methoden => {false, {}} (der Aufrufer bricht ab). SINGLE-SOURCE fuer den EMIT-Pfad (construct_plan_into) UND den
// RUNTIME-Pfad (run_profile_facade), damit Bau-Typ/Dual-Compile (Emit) und Mess-Loop-Parallelitaet (Runtime) DIESELBE
// Methodik-Quelle tragen.
struct MethodikOverride {
    bool                     ok = true;
    std::vector<std::string> run_methodology;
};
[[nodiscard]] MethodikOverride resolve_methodik_override(std::filesystem::path const& main_profile_path,
                                                         std::ostream&                os) {
    char const* const mp = std::getenv("COMDARE_PLAN_METHODIK_PROFILE");
    if (mp == nullptr || *mp == '\0') return {}; // unset => kein Override (byte-identisch)
    // S2-NACHT-3 (2026-07-23): ein BARE-BASENAME wird gegen thesis_profiles/ (= main_profile_path.parent_path())
    // aufgeloest -- so kann die super-YAML den KLASSEN-konformen Basename setzen; ein Pfad-behafteter Wert bleibt gueltig.
    std::filesystem::path const mp_path = resolve_methodik_profile_path(mp, main_profile_path);
    auto const                  mtp     = tlz::load_thesis_profile(mp_path);
    if (!mtp) {
        os << "[methodik] COMDARE_PLAN_METHODIK_PROFILE '" << mp << "' nicht als Thesis-Profil lesbar (aufgeloest: '"
           << mp_path.string() << "').\n";
        return {false, {}};
    }
    if (mtp->run_methodology.size() > 1) { // exactly-one HART (Ledger 61-STUFEN) auf dem METHODIK-Profil
        os << "[methodik] METHODIK-Profil '" << mp << "': " << mtp->run_methodology.size()
           << " Methoden deklariert -- GENAU EINE erlaubt (exactly-one je Call, Ledger 61-STUFEN).\n";
        return {false, {}};
    }
    return {true, mtp->run_methodology};
}

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

// simd-Unter-Achse der external_utils-Haupt-Achse (F-SIMD, Q2-Option-C): die -march-Flag-QUELLE ist die
// SimdSubAxis-Option (compile-time-Reflexion), der ORT ist diese CompileFn-Flag-Kette. Default = no_extension
// (SimdNoExtOption, keine Flags, Ist-Verhalten byte-identisch); die CEB-Laufzeit-Permutation aller Auspraegungen
// kommt mit dem Planer-Strang. Single-XML (9dim-G3, Sec.50): die Einzelpfad-Wahl kommt aus dem Profil
// (<system_axes><external_utils><simd>, GENAU EINER), nicht mehr aus COMDARE_PILOT_SIMD_POLICY-Env.
// GN-1-Anker (opt-g-Facade): die hier gezogenen simd-Optionen haengen unter dem AKTIVEN external_utils-
// Familien-Knoten (external_utils_family_axis.hpp, analog CompilerSystemAxis) -- Label-Drift bricht compile-time.
static_assert(::comdare::cache_engine::measurement::SimdNoExtOption::parent_axis_label() ==
                  ::comdare::cache_engine::measurement::SimdExternalUtilsFamily::axis_label(),
              "GN-1: die opt-g-Facade zieht simd-Flags nur ueber den aktiven external_utils-Knoten.");
[[nodiscard]] std::string_view active_simd_policy(cx::ThesisProfile const* tp = nullptr) {
    namespace cm = ::comdare::cache_engine::measurement;
    // Single-XML (9dim-G3): der EINE deklarierte <simd>-Wert des Profils ist die Quelle (GENAU EINER = Einzelpfad).
    // 0 deklariert -> benannter Achsen-Default no_extension (byte-identisch, keine Flags); >1 traegt der
    // Permutations-Pfad (run_profile), nicht diese Facade-Naht.
    if (tp != nullptr && tp->external_utils.simd_options.size() == 1) return tp->external_utils.simd_options.front();
    return cm::SimdNoExtOption::simd_id();
}

[[nodiscard]] std::vector<std::string> perm_external_utils_cflags(cx::ThesisProfile const* tp = nullptr) {
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
//
// T2-B: DIE ENTSCHEIDUNG WOHNT AB HIER IN DER NAHT, NICHT ZWEIMAL. Das #if stand wortgleich hier UND in
// toolchain_stamp_naht.hpp, sobald das Glied [5] die atomic128-Achse traegt. Zwei Orte, EINE Entscheidung
// -- also genau die Konstellation, aus der die W-6/W-13-Divergenz entstand: der Bau haengt -mcx16 an,
// waehrend das Glied "no_cx16" behauptet (oder umgekehrt), und die Identitaets-Aussage ist falsch, ohne
// dass irgendwo etwas bricht. Diese Funktion liest jetzt dieselbe Quelle wie der Stempel.
[[nodiscard]] std::vector<std::string> perm_compiler_isa_cflags() {
    std::string_view const flag = ::comdare::cache_engine::profile_facade::active_atomic128_wahl().flags;
    if (flag.empty()) return {};
    return {std::string{flag}};
}

// M-1/D-1 (06.08.2026) -- DIE STUFE-2->STUFE-3-NAHT DER MESS-ACHSE.
//
// WAS HIER VORHER STAND: die drei Mess-Defines als LITERALE in der Initialisierungsliste --
// -DCOMDARE_MEASUREMENT_ON=1, -DCOMDARE_CE_ENABLE_STATISTICS=1, -DCOMDARE_EXPERIMENT_MODE_ON=1,
// unabhaengig von jeder Tooling-Wahl. Ein vollstaendiger Scan nach MeasurementTooling|measurement_tooling
// ueber libs/ und apps/ fand 105 Treffer in 14 Dateien -- Plan-Legende, Job-Fan-out, Stempel-Renderer,
// XML-Trage-Pfad -- und KEINEN EINZIGEN, der Messcode ein- oder ausschaltet. Das Preimage-Glied [3]
// deklarierte also "die CT-Mess-AUSSTATTUNG dieser Tier-Binary", waehrend jede Tier-Binary die volle
// Ausstattung bekam. Der Stempel log, und nichts brach.
//
// AB HIER: der Mess-Teil des Define-Vektors ist eine FUNKTION der einkompilierten Mess-Achse
// (profile_facade/mess_achsen_naht.hpp). Die Naht loest die Combo-Legende GENAU EINMAL auf; dieselbe
// Aufloesung speist den Stempel (lazy_adhoc_source_gen measurement_stamp_from_env -> Glied [3]).
// Deshalb kann der Bau nicht mehr etwas anderes einbauen, als der Stempel behauptet -- strukturell,
// nicht per Absprache. Die Begruendung JEDER Zuordnung (welches Tooling welches Gate zieht) steht am
// Objekt belegt im Kopf von mess_achsen_naht.hpp, ebenso die ehrliche Grenze (macro und micro teilen
// heute EIN Gate) und die PMC-Entscheidung.
//
// WAS BEWUSST NICHT AUS DER MESS-ACHSE KOMMT:
//   -DCOMDARE_ANATOMY_MODULE_BUILD=1  -- die Modul-Bau-Markierung. Sie sagt "dies ist eine Tier-Binary",
//                                        nicht "so wird gemessen"; sie gilt fuer JEDE Ausstattung.
//   -DCOMDARE_EXPERIMENT_MODE_ON=1    -- die Experiment-Kompilat-Markierung (Wurzel-CMakeLists:446
//                                        "AKTIVIERT selbst keine Hooks"). Sie VERLANGT ihrerseits
//                                        MEASUREMENT_ON (abi_adapter.hpp:9-10, #error) -- und weil die
//                                        Naht MEASUREMENT_ON fuer jede nicht-leere Menge setzt, ist die
//                                        Invariante erhalten.
[[nodiscard]] std::vector<std::string> perm_mess_defines() {
    std::vector<std::string> d = {"-DCOMDARE_ANATOMY_MODULE_BUILD=1"};
    for (auto& f : ::comdare::cache_engine::profile_facade::live_mess_achsen_defines()) d.push_back(std::move(f));
    d.emplace_back("-DCOMDARE_EXPERIMENT_MODE_ON=1");
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
    // (Allokator-Organ, Compiler-atomic128, external_utils-SIMD) montiert perm_compile_flags() getrennt.
    return d;
}

// INC-0: der EINE Compile-Flag-Assembler fuer die Tier-Binary-Subprozesse -- macht die WAS/WIE-Schicht-Trennung
// SICHTBAR statt eines flachen Misch-Vektors: (1) Mess-/OS-/Arch-Defines (perm_mess_defines), (2) Allokator-ORGAN-
// Defs (snmalloc-Vertrag), (3) Compiler-SYSTEM-Flag -mcx16 (atomic128-Achse), (4) external_utils-SIMD -march.
// NB/CX-4: DIE BAU-NAHT DER PREIMAGE-GLIEDER [5] UND [6].
//
// WARUM GENAU HIER UND NICHT AN DEN VIER CompileFn-BAUSTELLEN: perm_compile_flags() ist der EINE
// Flag-Assembler, aus dem ALLE vier make_gpp_compile_fn-Aufrufe dieser TU ihren defines-Kanal ziehen
// (Einzel-Pfad, Profil-Perm-Fabrik, Experiment-Perm-Fabrik, Experiment-Fallback). Vier Einhaengungen
// waeren vier Orte, an denen eine vergessen werden kann -- und eine vergessene Stelle hiesse: eine
// Tier-Binary mit ANDEREM einkompiliertem Fingerprint als der, den die CEB fuer sie erwartet.
//
// DIE ZWEITE HAELFTE DER NAHT liegt im Laufzeit-Zwilling (lazy_adhoc_source_gen.hpp,
// make_lazy_adhoc_fingerprint_fn_from_env). Beide Seiten rufen DIESELBEN argumentlosen, reinen
// Komposition-Funktionen der Toolchain-Naht -- deshalb koennen sie nicht auseinanderlaufen. Wer hier
// etwas aendert, MUSS es dort mitaendern; die Naht ist genau deshalb EINE Funktion und kein Argument.
//
// T2-B: DIE ZWEI GLIEDER TRENNEN SICH HIER. Glied [6] (bvset) ist run-konstant -- es haengt an der
// Enable-Menge des Treibers, nicht an der Permutation. Glied [5] ist es NICHT mehr: seit T2-B traegt es
// opt/opt_flags/ext/gate dieser Permutation. Wuerde diese Funktion weiter BEIDE liefern, bekaeme der
// Perm-Pfad ZWEI -DCOMDARE_TOOLCHAIN_STAMP_GLIED-Argumente (den run-konstanten aus dem Basis-Kanal und
// den per-Perm-Wert), und welches gewinnt, entschiede die Argument-Reihenfolge in der Response-Datei --
// eine Identitaets-Aussage per Zufall. Deshalb: der Parameter sagt, ob das Glied [5] mitkommt. Der
// Einzel-Pfad (kein <system_axes>) nimmt es mit und ist damit byte-identisch zum Vor-T2-B-Stand; der
// Perm-Pfad laesst es weg und haengt seinen eigenen Wert an EINER Stelle an (compile_for_perm).
// B-9/golden-102: build_version_basis ist die BASIS der build_version dieses Laufs (args.build_version)
// -- sie speist das Preimage-Glied [10] (-DCOMDARE_BUILD_VERSION_GLIED). Sie ist KEINE Prozess-Konstante
// (anders als Toolchain/bvset/Overlay), deshalb reist sie als Parameter von den vier CompileFn-Baustellen
// herein statt aus einer live_*()-Komposition. Leer == Identitaet (kein Define, Bestands-Aufrufer
// byte-identisch).
[[nodiscard]] std::vector<std::string> perm_stamp_glied_defines(bool             mit_toolchain_glied = true,
                                                                std::string_view build_version_basis = {}) {
    namespace pfn = ::comdare::cache_engine::profile_facade;
    std::vector<std::string> d;
    if (mit_toolchain_glied) {
        if (std::string arg = pfn::toolchain_stamp_glied_define_arg(pfn::compose_live_toolchain_stamp_glied());
            !arg.empty())
            d.push_back(std::move(arg));
    }
    if (std::string arg = pfn::build_variant_set_signature_define_arg(pfn::live_build_variant_set_signature_glied());
        !arg.empty())
        d.push_back(std::move(arg));
    // B-9/golden-102: das Glied [10]. Run-konstant wie das Overlay-Glied (die Basis haengt am LAUF, nicht
    // an der Permutation) -- die Glied-[5]-Falle zweier konkurrierender Defines kann hier nicht entstehen.
    if (std::string arg = pfn::build_version_glied_define_arg(build_version_basis); !arg.empty())
        d.push_back(std::move(arg));
    // E-E: das Glied [7] (Overlay-Quell-Hash). Es steht hier und nicht bei den per-Perm-Fabriken, weil es
    // RUN-KONSTANT ist: es traegt den Quelltext-Stand DIESER CEB, nicht eine Eigenschaft der Permutation.
    // Damit kann die Falle von Glied [5] hier gar nicht erst entstehen (zwei konkurrierende Defines, deren
    // Gewinner die Argument-Reihenfolge in der Response-Datei entschiede). Der Wert ist derselbe, den der
    // CEB-Laufzeit-Zwilling liest -- beide Seiten ziehen abi::kOverlaySourceHash, es gibt keinen Parameter,
    // ueber den eine zweite Wahrheit hereinkaeme (overlay_source_hash_naht.hpp).
    if (std::string arg = pfn::overlay_source_hash_define_arg(pfn::live_overlay_source_hash_glied()); !arg.empty())
        d.push_back(std::move(arg));
    // EINMALIGE Ehrlichkeits-Zeile, wenn die CT-Realversion den Tier-Treiber NICHT deckt (verschiedene
    // Compiler fuer CEB und Tier-Bau). Dann faellt aus dem Glied genau EIN Bestandteil weg -- die
    // Versions-Behauptung; Dialekt UND Treiber-Tag bleiben drin. Das soll im Trace stehen und nicht stumm
    // passieren (Praezedenz der C-3a-Auflage: eine Identitaets-Entscheidung muss sichtbar sein). Der
    // Hinweis haengt NICHT am Wert -- die Naht selbst bleibt rein.
    //
    // NB-3/T2-D: DIESER TEXT WAR NACH NB2-1 FALSCH GEWORDEN. Er stammt aus NB/CX-4, wo das cxx-Feld
    // tatsaechlich nur `<dialekt>[-<realversion>]` trug -- ohne Deckung blieb also wirklich "nur der
    // Dialekt" uebrig, und genau daraus entstand der Fail-open-Fall (ii) des Codex-Zweitreviews: g++-17
    // und g++-18 kollabierten auf dasselbe `cxx=gcc`. NB2-1 (R1) hat das geheilt -- der Treiber-Tag steht
    // seither IMMER im Feld (`<dialekt>[-<realversion>]:<treiber-tag>`, abi/toolchain_stamp_glied.hpp).
    // Der alte Satz haette einen Leser also ausgerechnet zu der Sorge zurueckgefuehrt, die der Code
    // bereits ausgeraeumt hat: er las sich wie ein Verlust der Treiber-Unterscheidbarkeit. Eine
    // Diagnose-Zeile, die den Ist-Stand falsch beschreibt, ist schlimmer als keine -- sie ist die einzige
    // Quelle, aus der jemand im Trace ueberhaupt erfaehrt, WAS das Glied gerade behauptet.
    //
    // T2-C: DIE ZEILE BERICHTET AB JETZT DIE SONDE, NICHT DIE DECKUNG. Die Frage "deckt die CEB-Version
    // den Tier-Treiber?" war ein Ersatz fuer eine Messung, die es nicht gab. Seit T2-C gibt es sie: der
    // Tier-Treiber nennt seine Version selbst. Damit hat die Zeile genau zwei Faelle zu melden -- erhoben
    // (mit dem Wert, damit er im Trace steht und nicht nur im Binary) oder NICHT erhoben (dann ist der
    // Lauf nicht skip-faehig, und das ist die wichtigste Zeile des ganzen Laufs).
    static bool gemeldet = false;
    if (!gemeldet) {
        gemeldet                  = true;
        std::string const treiber = pfn::active_cxx_driver_tag(); // dieselbe EINE Quelle wie cxx_compiler()
        std::string const real    = pfn::active_tier_realversion();
        if (real.empty()) {
            std::cerr << "[profile_facade] T2-C: die REALVERSION des Tier-Treibers '" << treiber
                      << "' konnte NICHT erhoben werden (Sonde ohne brauchbare Antwort oder Tag nicht "
                         "sondierbar). Das Toolchain-Glied [5] traegt deshalb keine Versions-Behauptung -- es "
                         "traegt weiterhin Dialekt UND Treiber-Tag (cxx=<dialekt>:<treiber-tag>, NB2-1 Regel "
                         "R1), verschiedene Treiber bleiben also unterscheidbar. FOLGE: dieser Lauf ist NICHT "
                         "skip-faehig (kein Fingerprint-Sidecar, kein Lager-Rueckschrieb) -- eine unbestimmte "
                         "Identitaet darf keinen Skip tragen.\n";
        } else {
            std::cerr << "[profile_facade] T2-C: Tier-Treiber '" << treiber << "' meldet Realversion '" << real
                      << "' (am Treiber erhoben, nicht von der CEB geerbt). Das Toolchain-Glied [5] traegt sie "
                         "als cxx=<dialekt>-"
                      << real << ":" << treiber << ". Zum Vergleich die CEB-Toolchain: '"
                      << ::comdare::cache_engine::abi::kDetectedCompilerDialect << " "
                      << ::comdare::cache_engine::abi::kDetectedCompilerRealVersion << "'"
                      << (pfn::ct_realversion_deckt_treiber(treiber) ? " (deckungsgleich)."
                                                                     : " (anderer Compiler -- unschaedlich, die "
                                                                       "Version stammt ab T2-C vom Tier-Treiber).")
                      << "\n";
        }
    }
    return d;
}

// T2-B: `mit_toolchain_glied=false` liefert denselben Kanal OHNE das Glied [5] -- die Basis der
// per-Perm-Fabriken, die ihren eigenen Glied-Wert anhaengen (s. perm_stamp_glied_defines oben).
[[nodiscard]] std::vector<std::string> perm_compile_flags(cx::ThesisProfile const* tp                  = nullptr,
                                                          bool                     mit_toolchain_glied = true,
                                                          std::string_view         build_version_basis = {}) {
    std::vector<std::string> d = perm_mess_defines();
    for (auto& f : perm_alloc_organ_cflags()) d.push_back(std::move(f));
    for (auto& f : perm_compiler_isa_cflags()) d.push_back(std::move(f));
    for (auto& f : perm_external_utils_cflags(tp)) d.push_back(std::move(f));
    for (auto& f : perm_target_isa_cflags(tp)) d.push_back(std::move(f)); // INC-2d: Ziel-ISA (Cross-Compile)
    // Glieder [5]/[6] + B-9: das Basis-Glied [10]
    for (auto& f : perm_stamp_glied_defines(mit_toolchain_glied, build_version_basis)) d.push_back(std::move(f));
    return d;
}

// NB/CX-4: die Entscheidung selbst (Env-Override, sonst der Achsen-Default) ist in die Toolchain-Naht
// gewandert und wird hier nur noch DURCHGEREICHT. Grund: das Preimage-Glied [5] urteilt ueber genau diesen
// Treiber (Dialekt + Deckung der CT-Realversion). Stuende die Entscheidung an zwei Orten, koennte das Glied
// ueber einen ANDEREN Treiber urteilen als den, der wirklich compiliert -- dieselbe Klasse Divergenz wie
// W-6/W-13. INC-1h bleibt woertlich gueltig, nur sein Ort ist jetzt die Naht.
[[nodiscard]] std::string cxx_compiler() { return ::comdare::cache_engine::profile_facade::active_cxx_driver_tag(); }

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

// Scheibe 2b (Ledger 61/62, §62-G (4)): Build-Typ-Entscheidung fuer den Compile-Flag-Kanal. SINGLE-SOURCE
// mit dem (i)-Stempel -- dieselbe COMDARE_BUILD_TYPE-Env-Lesung (tlz::build_type_version_suffix(), nicht
// eine zweite getenv-Quelle), damit -O0-g-Maschinencode und +bt=Debug-Stempel IMMER GEMEINSAM auftreten
// (keine Divergenz zwischen Bytes und Stempel). Die FACADE liest die Env und reicht der achsen-blinde
// Builder nur den Flag-WERT herunter (Muster (2), wie facade_supports_fno_gnu_unique). Ungesetzt/Release
// => false => byte-identischer Compile-Kanal zum Ist-Stand.
[[nodiscard]] bool facade_build_type_is_debug() { return !tlz::build_type_version_suffix().empty(); }

// H-10 (Bau-INC-1g): die VARIABLEN System-Achsen-Belegungen (Erweiterungshardware-Politik,
// Compiler, opt_level) werden in build_version kodiert — eine unter anderer Belegung gebaute DLL bekommt
// ein eigenes .version-Sidecar und die CSV-Spalte
// build_version traegt die Provenienz. Konstante Achsen (Scheduling/Last=Default) bleiben
// weggelassen, bis die CEB-Laufzeit-Permutation sie variabel macht.
// A2-EICHUNG (2026-08-05, GATE 5): der Schutz vor dem falschen Skip laeuft seither NICHT mehr ueber einen
// .version-Stringvergleich in dll_is_current (der vergleicht nur noch den `.fingerprint`), sondern ueber das
// build_version-Glied IM Fingerprint-Preimage. Wirkung identisch, Traeger anders: eine unter anderer Belegung
// gebaute DLL hat einen anderen erwarteten Fingerprint und faellt am EINEN Vergleich durch.
[[nodiscard]] std::string system_axes_version_suffix(cx::ThesisProfile const* tp = nullptr) {
    // A1/OF-2 (Ruling 2026-07-18): KEIN globaler Byte-Anker mehr. Die opt_level-Provenienz wird IMMER emittiert
    // (kein O2-Sonderfall) -> jedes Teil beweglich, keine bevorzugte Referenz-Stufe. Folge: alle Tier-Binaries
    // tragen +opt=<level> (Default +opt=O3) -> sie gelten unter neuer Belegung als neu; die golden-
    // Reihe wird deterministisch unter O3 neu gebaut/gemessen (bewusster Neu-Mess-Lauf, alt-Reihen additiv erhalten).
    // Single-XML (9dim-G3, Sec.50): die vier active_*-Aufloeser ziehen die Einzelpfad-Wahl aus dem Profil (tp), nicht
    // mehr aus Env; kein Profil / keine Deklaration -> benannte Defaults -> Suffix byte-identisch (golden-neutral).
    // Lane F R3 (O-8 Schritt 10): die Glieder werden GESAMMELT und von der EINEN Suffix-Quelle
    // zusammengesetzt (system_version_suffix.hpp). Die frueher hier gebaute Ordnung "+ext+cxx+opt"
    // faellt damit auf die bindende Form "+cxx+opt+ext" -- eine bewusste, golden-veraendernde
    // Vereinheitlichung der Divergenz W-6/W-13, kein Umformatieren.
    namespace pf = ::comdare::cache_engine::profile_facade;
    namespace cm = ::comdare::cache_engine::measurement;
    pf::SystemVersionSuffixParts parts;
    std::string const            cxx_tag = cxx_compiler();
    std::string const            opt_id  = std::string{active_opt_level(tp)};
    parts.cxx                            = cxx_tag;
    parts.opt                            = opt_id;
    // D2.8(ii): no_extension emittiert KEIN Segment -- das leere Glied ist die Aussage.
    std::string_view const simd_policy = active_simd_policy(tp);
    if (simd_policy != ::comdare::cache_engine::measurement::SimdNoExtOption::simd_id()) parts.simd = simd_policy;
    // Bauplan §4 (inkrementeller Cache): die CEB-Contract-Version (Framework/System-Ebene) faltet sich in die
    // build_version -> jeder Bump (ABI-Major AUTOMATISCH ueber COMDARE_ANATOMY_ABI_MAJOR, codegen-Minor manuell/
    // CI-Tripwire) laesst jede perm.dll.version mismatchen -> ALLE Tier-Binaries neu ("CEB-Aenderung betrifft alle").
    // Konsistent zum Loader-host_compatible_with-Major-Backstop. Organ-Provenienz bleibt STRIKT getrennt (perm.algos).
    // W10-C4: der Wert kommt aus der EINEN Zusammensetzung neben der Segment-Ordnung (ceb_contract_version_text);
    // vorher stand dieselbe Konkatenation hier, im --version-Block und in der Cache-Key-Naht getrennt.
    std::string const ceb_version = pf::ceb_contract_version_text();
    parts.ceb                     = ceb_version;
    // INC-2d: Cross-Compile-Provenienz NUR wenn Ziel != Host (native x86_64 = kein Suffix -> build_version
    // byte-identisch, golden-neutral). Ziel-ISA ist system_config -> .version-Sidecar, NIE binary_id.
    if (std::string_view const t = active_target_isa(tp);
        t != ::comdare::cache_engine::measurement::X86_64TargetIsa::target_isa_id())
        parts.target_isa = t;
    // H-10 (W9.1): telemetry-System-Achsen-Provenienz. REGISTRY-GEGATED -- der Token wird NUR emittiert, wenn
    // "telemetry" wirklich als System-Achse gefuehrt wird. Damit ist build_system_axis_levels() (bislang ausser der
    // Byte-Identitaets-Fold in build_all_axis_levels() ohne echten Konsumenten -- Audit-Auflage A, 2026-07-17) ein
    // ECHTER Produktions-Konsument: verschwaende die telemetry-System-Achse aus der Registry, entfiele der Token
    // automatisch (Anti-Drift). NUR bei Silent (!= Default Active) -> Default byte-identisch (golden-neutral).
    if (active_telemetry_is_silent()) { // A9.3: Default Active (kein Profil-Wiring) -> golden byte-identisch
        auto const system_levels            = ex::build_system_axis_levels();
        bool const telemetry_is_system_axis = std::any_of(system_levels.begin(), system_levels.end(),
                                                          [](ex::AxisLevel const& l) { return l.axis == "telemetry"; });
        if (telemetry_is_system_axis) parts.telemetry = "silent";
    }
    // (i) §61-STUFEN Compile-Kennzeichnung: +bt=Debug NUR bei Debug-Build (COMDARE_BUILD_TYPE=Debug, Emissions-Seite
    // im Director). Release/Default => "" => build_version byte-identisch (golden/Sidecar/Resume unberuehrt).
    std::string const bt = tlz::build_type_version_value();
    parts.build_type     = bt;
    // Ledger 70.9 (Lane F R3, Einhaengung am O-8-PUNKT): die Gate-Beitraege MUESSEN bei der
    // Scharfschaltung in der Identitaet sichtbar werden. C-3a ist scharf, der Text existierte fertig
    // und hatte NULL Produktions-Konsumenten -- hier ist er. Leerer Beitrag => KEIN Segment (heute der
    // Normalfall, also byte-neutral); OP-7: das Segment steht am ENDE der Ordnung.
    std::string const gate =
        cm::gate_contribution_identity_text(cm::route_of_simd_id(simd_policy), cm::SimdDialect::Gpp);
    parts.gate_contribution = gate;
    return pf::compose_system_version_suffix(parts);
}

} // namespace

ProfileRunResult run_profile_facade(ProfileRunArgs const& args) {
    namespace pf = ::comdare::cache_engine::profile_facade; // W10-C4: Zellwert-Naht + Suffix-Single-Source
    ProfileRunResult out;

    // -- (R5) Pre-Flight-Validat (exactly-one-Gate) SYMMETRISCH zum ep-Pfad (run_experiment_profile_facade oben):
    //    der tp-Validator erzwingt run_methodology exactly-one (validate_profile: >1 Methoden => nicht ok) sowie die
    //    Achsen-/<workloads>-Struktur -- rein-lesend, KEIN Bau. Verstoss => Abbruch VOR jedem Bau/Messen, damit ein
    //    2-Modi-Profil NICHT still mit ids.front()-Semantik (debug-Mess-Loop) durchlaeuft (Ledger §61-STUFEN, LED:3190).
    //    Byte-neutral fuer valide Profile (rc=0). Der DLL-Bau ist teuer -- der Fehler faellt hier statt spaeter.
    if (int const vrc = validate_profile_facade(args.profile_path, std::cout); vrc != 0) {
        std::cerr << "[profile_facade] Pre-Flight-Validat fehlgeschlagen (rc=" << vrc
                  << ") -- KEIN Bau, KEINE Messung.\n";
        out.exit_code = vrc;
        return out;
    }

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
    // T-1 (2026-08-09): der Ausdruck stand hier frueher AUSGESCHRIEBEN und ein zweites Mal (De-Morgan-dual)
    // in profile_run_entry.hpp -- zwei Parses derselben Datei, die dasselbe sagen MUSSTEN, ohne dass es
    // jemand erzwang. Jetzt faellt die Entscheidung an EINER Stelle und reist als Wert mit (a.system_achsen
    // unten); run_profile prueft sie gegen seinen eigenen Parse. Details: system_axes_entscheidung.hpp.
    bool const profile_has_system_axes = tp_opt && pf::profil_deklariert_system_achsen(*tp_opt);
    auto const is_selected             = [&workload_select](std::string const& id) {
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

    // smoke=>debug-Entkopplung (2026-07-22): das METHODIK-Profil (COMDARE_PLAN_METHODIK_PROFILE) speist -- falls
    // gesetzt -- die Mess-Loop-Methodik (resolve_measure_parallelism, profile_run_entry); der Bau-/Mess-Katalog bleibt
    // args.profile_path. Single-Source-Resolver (identisch zum Emit-Pfad construct_plan_into). Unlesbar/>1 => Abbruch
    // VOR dem Bau; unset => leer => byte-neutral.
    auto const methodik =
        resolve_methodik_override(args.profile_path, std::cerr); // S2-NACHT-3: Basename gg. thesis_profiles/
    if (!methodik.ok) {
        out.exit_code = 1;
        return out;
    }

    tlz::RunProfileArgs a;
    a.methodik_run_methodology = methodik.run_methodology;
    a.profile_path             = args.profile_path;
    a.out_csv                  = args.out_csv;
    a.src_dir                  = args.src_dir;
    a.dll_dir                  = args.dll_dir;
    a.compile                  = ex::make_gpp_compile_fn(
        perm_include_dirs(), perm_compile_flags(tp_ptr, /*mit_toolchain_glied=*/true, args.build_version),
        cxx_compiler(), perm_link_libs(),
        // opt-c: opt_level-Flag (Default O3, beweglich; Single-XML aus tp). Scheibe 2b: bei Build-Typ Debug
        // ersetzt -O0 -g die Optimierung (echter Debug-DLL-Bau); ungesetzt/Release => byte-identisch zum Ist.
        facade_build_type_is_debug() ? ex::debug_flags_for_toolchain() : perm_opt_level_cflags(tp_ptr),
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
    // T-1: DIE ENTSCHEIDUNG REIST MIT. Sie ist oben EINMAL gefallen; run_profile darf sie nicht neu
    // erfinden, sondern haelt seinen eigenen Parse dagegen. Genau die Belegung von compile_for_perm
    // unten haengt an ihr -- driftete sie, liefe die Perm-Schleife ohne per-Perm-Bau und ohne
    // per-Perm-Fingerprint. Bei nicht lesbarem Profil bleibt sie ungetragen (der Entry bricht ohnehin ab).
    a.system_achsen = tp_opt ? pf::system_achsen_entscheidung_von(profile_has_system_axes)
                             : pf::SystemAchsenEntscheidung::NichtGetragen;
    // GN-3 (§33, 2026-07-19): build_version + opt×simd-Kanal je nach <system_axes>-Deklaration (profile_has_system_axes).
    if (profile_has_system_axes) {
        // Die opt×simd-Perm-Schleife in run_profile haengt je Kombination +cxx=+opt=+ext= an → BASIS OHNE
        // system_axes_version_suffix() (Spiegel run_experiment_profile_facade). compile_for_perm montiert je Perm
        // die CompileFn aus den aufgeloesten Flags (WAS/WIE-Trennung: run_profile permutiert, die Facade montiert;
        // include_dirs/defines/cxx/link_libs/fno_gnu_unique bleiben Facade-Wissen).
        a.build_version = args.build_version;
        // B-9/golden-102: die reine BASIS fuer das Preimage-Glied [10] -- derselbe Wert, den
        // perm_compile_flags oben als -DCOMDARE_BUILD_VERSION_GLIED in die Tier-Uebersetzung haengt.
        a.build_version_basis = args.build_version;
        a.compiler_tag        = cxx_compiler(); // +cxx=-Provenienz im per-Perm-build_version
        // W10-C4 (Dossier Sektion 1): die beiden LAUF-KONSTANTEN System-Zellen. Die Ziel-ISA kommt aus der
        // Profil-Deklaration, falls es eine gibt (Cross-Bau), sonst aus der CT-Zelle der Bau-Plattform
        // (nativer Bau); die OS-FAMILIE ist rein CT (es gibt keinen Werte-Kanal fuer sie -- die
        // <machine>-os-Attribute sind ERWARTUNG, nicht Quelle). Die Perm-Schleife ergaenzt nur simd_id.
        a.system_cell_target_isa       = std::string{pf::resolve_system_cell_target_isa(
            tp_ptr != nullptr && tp_ptr->target_isa.isa.size() == 1 ? std::string_view{tp_ptr->target_isa.isa.front()}
                                                                    : std::string_view{})};
        a.system_cell_operating_system = std::string{pf::kSystemCellBuildOsFamily};
        // T2-B: die Basis-Defines kommen OHNE das Glied [5] herein -- der per-Perm-Wert wird unten an
        // genau EINER Stelle angehaengt (sonst zwei konkurrierende Defines, s. perm_stamp_glied_defines).
        a.compile_for_perm =
            [inc = perm_include_dirs(),
             def = perm_compile_flags(nullptr, /*mit_toolchain_glied=*/false, args.build_version), cxx = cxx_compiler(),
             libs = perm_link_libs(), fno = facade_supports_fno_gnu_unique(),
             dbg = facade_build_type_is_debug()](std::string const& opt_flag, std::string const& march_flag,
                                                 ::comdare::cache_engine::abi::SystemCellValues cell_values,
                                                 pf::PermToolchainGliedWert const&              toolchain_glied) {
                // Scheibe 2b: Build-Typ Debug ersetzt die Optimierung (opt_flag) durch -O0 -g; -march (die
                // [d,e,f]-ISA-Identitaet) und die Gate-Flags bleiben erhalten. dbg==false => flags==opt_flag =>
                // byte-identisch zum Ist-Compile-Kanal.
                std::string flags =
                    dbg ? ex::debug_flags_for_toolchain() : opt_flag; // opt-b-Kanal: eine rsp-Zeile (opt + -march)
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
                // W10-C4: das Zellwert-Define reist als EIGENES Argument im defines-Kanal (eine rsp-Zeile), nicht
                // in der opt/-march-Zeile -- es ist eine Praeprozessor-Definition, keine Codegen-Flag. Leere
                // Wertform => leeres Argument => gar kein Define => byte-identischer Bau.
                std::vector<std::string> perm_defines = def;
                if (std::string arg = pf::system_cell_values_define_arg(cell_values.value); !arg.empty())
                    perm_defines.push_back(std::move(arg));
                // T2-B: das PER-PERM-Glied [5]. Derselbe String, den der Laufzeit-Zwilling dieser Iteration
                // bekommt (die Schleife bildet ihn EINMAL und reicht ihn zweimal weiter) -- deshalb kann die
                // Tier-Binary keinen anderen Fingerprint einkompiliert bekommen als den, den die CEB erwartet.
                if (std::string arg = pf::toolchain_stamp_glied_define_arg(toolchain_glied.value); !arg.empty())
                    perm_defines.push_back(std::move(arg));
                return ex::make_gpp_compile_fn(inc, std::move(perm_defines), cxx, libs, flags, fno);
            };
    } else {
        a.build_version = args.build_version + system_axes_version_suffix(tp_ptr); // Einzel-Pfad byte-identisch
        // B-9/golden-102: auch hier die REINE Basis (der statische Suffix ist via Glied [5] Identitaet).
        a.build_version_basis = args.build_version;
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
    a.pruef_only                 = args.pruef_only;     // S3 (§62-B): pruef-only durchreichen (inert bei false)
    // D3-7b: die beiden Schalter reisen hier BEWUSST ungeprueft weiter. Die Fassade urteilt nicht ueber
    // den Auftrag (gleiche Linie wie beim Env-Doppel-Gate, s. Kommentar unten bei bestand_transport);
    // durchgesetzt wird die gegenseitige Ausschliessung EINMAL, in run_profile (der Aufruf steht unten;
    // fail-closed mit exit_code 7) -- und noch einmal eine Schicht tiefer in run_lazy_static_then_dynamic,
    // fuer jeden Aufrufer, der ohne diese Fassade direkt eine LazyRunConfig baut.
    // Ein dritter Abgleich hier waere eine ABSCHRIFT derselben Regel -- sie kann driften, der Aufruf nicht.
    a.build_parallelism   = args.build_parallelism; // W6 (§32-F7): Bau-Pool-Override durchreichen (0 = byte-neutral)
    a.gn_cell_opt         = args.gn_cell_opt;       // W5-C+ (§36.1): GN-Zellen-Filter (leer = kein Filter)
    a.gn_cell_simd        = args.gn_cell_simd;      // W5-C+ (§36.1): GN-Zellen-Filter (leer = kein Filter)
    a.workload_registry   = std::move(workload_registry);
    a.workload_values     = std::move(workload_values);
    a.cache_push          = args.cache_push;          // Storage #51: No-Op-Naht durchreichen (byte-neutral)
    a.cache_pull          = args.cache_pull;          // S2 (#46a): BATCH-Warm-Cache-Hydrierung durchreichen (No-Op)
    a.measurement_sink    = args.measurement_sink;    // Storage #51: perm.dll->Store (B) / CSV->measure-drop (C)
    a.partial_marker_sink = args.partial_marker_sink; // W11 (§43.c): BAU-Modus Teil-Marker durchreichen (No-Op-Default)
    a.chunk_part_size     = args.chunk_part_size;     // W11 (§43.c): Teil-Marker-Intervall N (0 = keine)
    a.progress_sink =
        args.progress_sink; // Welle 5 (E-W5-2): §38-Fortschritts-Rueck-Kanal (No-Op-Default => byte-neutral)
    // G4b-1 (#46b I1): die Bestandslog-Naht. Diese TU ist die EINE, die die Umbrella-Welt sehen darf (s. Kopf von
    // profile_run_facade.hpp) -- deshalb wird der BestandTransport GENAU HIER aus dem uebergebenen ArtifactCache
    // gebunden und nicht im Host. KEIN Gate hier: das harte Doppel-Gate sitzt beim Host (messung_driver, AUF-B3);
    // die Fassade urteilt nicht ueber Env. Ist bestand_cache leer (Default), bleibt bestand_transport unbelegt =>
    // bestandslog_active (iterator:927-929) false => keine Registrierung/Dedup, der Bau bleibt auf provision_all =>
    // byte-neutral. LEBENSDAUER (AUF-B5): der shared_ptr des Hosts ueberlebt diesen Aufruf; make_bestand_transport
    // haelt eine const& auf genau diese Instanz und wird ausschliesslich innerhalb von run_profile benutzt.
    // NUR dieser pa-Pfad (run_profile_facade): der xa-Pfad (run_experiment_facade, weiter unten) bleibt bewusst
    // ohne bestand_*-Felder -- AUF-B2, ausgewiesen inert.
    if (args.bestand_cache) a.bestand_transport = bl::make_bestand_transport(*args.bestand_cache);
    a.bestand_key_of     = args.bestand_key_of;
    a.bestand_doc_key    = args.bestand_doc_key;
    a.bestand_owner_uuid = args.bestand_owner_uuid;
    a.bestand_maschine   = args.bestand_maschine;
    // LAG-P2: das MESSWERT-Genus -- reines Durchreichen wie die vier Zeilen darueber, KEIN Gate hier.
    // Der Transport ist DERSELBE (oben aus bestand_cache gebunden); getrennt sind nur die Dokumente
    // (eigener doc_key je Realm, D-05). Alle leer (Default) => mess_bestandslog_active false =>
    // byte-neutral.
    a.mess_bestand_key_of   = args.mess_bestand_key_of;
    a.mess_bestand_doc_key  = args.mess_bestand_doc_key;
    a.mess_bestand_versions = args.mess_bestand_versions;
    // T2-A/F4: die Plan-Ablage -- reines Durchreichen wie die vier Zeilen darueber, KEIN Gate hier (leerer
    // Pfad => PlanPersistenz inert => byte-neutral). Sie braucht den Umbrella-Umweg nicht: ein Pfad ist ein
    // Pfad, es gibt nichts aus einem ArtifactCache zu binden.
    a.batch_plan_datei = args.batch_plan_datei;

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
       << vr.variants_checked << " allowed_variants, " << vr.categories_checked;
    // Paket #11 (2026-08-09): der NENNER der Teilmengen-Garantie gehoert in die Ausgabe -- aber NUR, wenn die
    // Wache auch lief. categories_offered==0 heisst "keine <measurement_categories> deklariert" (= alle
    // Kategorien, KERN-A); dann waere ein "0 von 0" eine Falschaussage ueber ein Angebot, das es sehr wohl
    // gibt. Ohne Auswahl bleibt die Zeile daher byte-identisch zum Ist-Stand.
    if (vr.categories_offered > 0) os << " von " << vr.categories_offered;
    os << " measurement_categories";
    if (vr.workloads_checked > 0) os << ", " << vr.workloads_checked << " workloads";
    os << "\n";
    for (auto const& w : vr.warnings) os << "  [HINWEIS] " << w << "\n";
    for (auto const& e : vr.errors) os << "  [FEHLER]  " << e << "\n";
    if (vr.ok)
        os << "VALIDAT OK: das Experiment-Profil ist gegen die 2-Registry (ce+prt) + "
              "PrueflingVerbundStrategy/Kategorien "
              "konsistent.\n";
    else
        os << "VALIDAT FEHLGESCHLAGEN: " << vr.errors.size()
           << " Fehler — Experiment NICHT baubar (Abbruch vor Bau).\n";
    os << "(--validate: rein-lesend — es wurde KEINE DLL gebaut und KEINE Messung durchgefuehrt.)\n";
    return vr.ok ? 0 : 1;
}

namespace {
// ---------------------------------------------------------------------------------------------------------------
// G4b-2 (#46b I1b / E1+E2+E4): der planer_block-LEBENSZYKLUS um eine Emission.
//
//   store(offen) -> Emission -> rc==0 ? mark_done + store + commit : PromiseGuard laeuft in den Release
//
// Er liegt HIER und nicht im Treiber, weil der gelockte Schreibweg (with_document_lock_retry /
// store_reservation_locked / make_lock_owner, builder_registration.hpp) ueber bestandslog_document.hpp:49 den
// ce-XML-DOM zieht und libs/common nicht im Include-Satz des messung_driver-Targets liegt. Diese TU ist die eine,
// die die Umbrella-Welt sehen darf -- und zugleich die, in der die Emission ohnehin passiert. Angenehme Folge:
// 2.4-(2) ("rc in eine LOKALE Variable, nicht return-im-Ausdruck") ist hier strukturell erfuellt, der Guard lebt
// garantiert laenger als die Berechnung des Rueckgabewerts.
//
// BUCHHALTUNG, NIE EMISSIONS-GATE: scheitert einer der drei Stores, laeuft die Emission TROTZDEM und es gibt EINE
// Warnzeile auf cerr. Das Bestandslog informiert, es filtert nicht -- ein Lager-Ausfall darf keine CI-Emission
// verhindern. Alle Zeilen gehen auf cerr, NIE auf cout: cout ist hier der YAML-Kanal.
//
// FRIST: reserviert_utc und pro_forma_bis_utc entstehen aus DEMSELBEN now_epoch, die Frist ueber
// pro_forma_deadline_epoch_s -- exakt das Muster von make_slice_reservation (builder_registration.hpp:193-200),
// damit die 30 Minuten echt sind und nicht eine Sekunde spaeter ablaufen.
//
// LOCK: ttl aus default_lock_ttl_s() (der LockRecord-Default ist die EINE Heimat der Zahl), Owner aus
// make_lock_owner(owner_uuid, maschine). Ein planer_block ist der Millisekunden-Fall -- er berechnet keine ETA.
// ---------------------------------------------------------------------------------------------------------------
// GoF DECORATOR um einen IPlanBuilder: reicht das ganze Protokoll unveraendert an den inneren Builder weiter und
// merkt sich NUR die [a,b,c]-Legenden, die auf der Mess-Achsen-Ebene vorbeikommen. Zweck: der planer_block soll die
// CEB-Bindung (ceb_legende, E3) melden, und die Legende entsteht erst IM Director-Walk. Der Decorator holt sie aus
// DEMSELBEN Walk -- kein zweites Parsen, keine neue Director-API, kein Eingriff in die Emitter-Reinheit.
class LegendCollectingBuilder final : public planner::IPlanBuilder {
public:
    explicit LegendCollectingBuilder(planner::IPlanBuilder& inner) : inner_{inner} {}

    void begin_plan(planner::PlanHeader const& h) override { inner_.begin_plan(h); }
    void begin_perm(planner::PlanPerm const& p) override { inner_.begin_perm(p); }
    void on_step(planner::PlanStep const& s) override { inner_.on_step(s); }
    void end_perm(planner::PlanPerm const& p) override { inner_.end_perm(p); }
    void end_plan(planner::PlanHeader const& h) override { inner_.end_plan(h); }
    void begin_measurement_combo(planner::PlanMeasurementCombo const& c) override {
        legenden_.push_back(c.legend);
        inner_.begin_measurement_combo(c);
    }
    void end_measurement_combo(planner::PlanMeasurementCombo const& c) override { inner_.end_measurement_combo(c); }

    // Die EINE Legende dieser Emission -- oder leer. Leer bei null Kombinationen (nichts emittiert) UND bei
    // mehreren: ein planer_block sperrt EINE Strecke, und fuer eine MENGE von Klammern gibt es keine Wire-Form.
    // Statt hier ein Trennzeichen zu ERFINDEN, das kein Leser kennt, bleibt das Feld "nicht gemeldet" -- es ist
    // ausdruecklich optional. Der Mehr-Combo-Fall wird sichtbar gemeldet, nicht verschwiegen.
    [[nodiscard]] std::string eine_legende() const { return legenden_.size() == 1 ? legenden_.front() : std::string{}; }
    [[nodiscard]] std::size_t combo_count() const noexcept { return legenden_.size(); }

private:
    planner::IPlanBuilder&   inner_;
    std::vector<std::string> legenden_;
};

// Das Ergebnis einer Emission: ihr Rueckgabe-Code und die CEB-Bindung, die der planer_block danach melden kann.
struct EmissionErgebnis {
    int         rc = 1;
    std::string ceb_legende; // leer = nicht gemeldet (0 oder >1 Kombinationen)
    std::size_t combo_count = 0;
};

[[nodiscard]] int run_with_planer_block(PlanerBlockContext const& pb, std::function<EmissionErgebnis()> const& emit) {
    if (!pb.aktiv()) return emit().rc; // AUS (Default) => byte-identisch zum Stand vor G4b-2

    auto const      transport = bl::make_bestand_transport(*pb.cache);
    auto const      me        = bl::make_lock_owner(pb.owner_uuid, pb.maschine);
    int const       ttl_s     = bl::default_lock_ttl_s();
    bl::NowFn const now_fn{&bl::system_now_epoch_s};

    // ceb_key_sha512 steht schon VOR der Emission fest (die CEB ist diese Binary) und reist deshalb bereits am
    // offen-Record mit. ceb_legende kann erst die Emission liefern -- sie kommt mit dem done-Record.
    std::string const ceb_key{::comdare::cache_engine::builder::kCebFingerprint};
    auto const        now_epoch = now_fn();
    auto              res       = bl::make_planer_block_reservation_value(
        pb.id, pb.maschine, pb.threads, /*ceb_legende=*/std::string{}, ceb_key, bl::utc_iso_from_epoch(now_epoch),
        bl::utc_iso_from_epoch(bl::pro_forma_deadline_epoch_s(now_epoch)));

    auto const store = [&transport, &pb, &me, ttl_s, &now_fn](bl::BatchReservierung const& r, char const* was) {
        if (!bl::store_reservation_locked(transport, pb.doc_key, me, ttl_s, now_fn, r, was)) {
            std::cerr << "[bestandslog] WARNUNG fehlerklasse=reservierung_nicht_gespeichert: planer_block id='" << r.id
                      << "' (" << was << ") steht NICHT im Store -- Emission laeuft trotzdem weiter.\n";
        }
    };

    store(res, "planer_block offen");
    // Terminalitaet: der Guard schliesst die Reservierung AUCH im Fehler- und Wurf-Pfad -- released statt ewig
    // offen, damit eine andere Maschine die Strecke uebernehmen kann.
    bl::PromiseGuard guard{[&res, &store]() {
        bl::mark_released(res);
        store(res, "planer_block Release");
    }};

    EmissionErgebnis const erg = emit(); // LOKALE Variable (2.4-(2)): der Guard lebt hier noch
    int const              rc  = erg.rc;
    if (rc == 0) {
        // Die CEB-Bindung steht erst jetzt fest (E3): die Legende kommt aus dem gelaufenen Walk.
        if (!erg.ceb_legende.empty()) {
            res.ceb_legende = erg.ceb_legende;
        } else if (erg.combo_count > 1) {
            std::cerr << "[bestandslog] WARNUNG fehlerklasse=ceb_legende_nicht_eindeutig: die Emission traegt "
                      << erg.combo_count << " Mess-Kombinationen -- ceb_legende bleibt ungemeldet (ceb_key_sha512 "
                      << "identifiziert die CEB weiterhin eindeutig).\n";
        }
        bl::mark_done(res);
        store(res, "planer_block done");
        guard.commit(); // Erfolg => kein Release
    }
    return rc;
}
} // namespace

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
    auto director = []() -> planner::ExperimentPlanDirector {
#if defined(COMDARE_CE_AXIS_REGISTRY) && defined(COMDARE_SYSTEM_AXIS_REGISTRY) &&                                      \
    defined(COMDARE_MEASUREMENT_AXIS_REGISTRY)
        if (auto trio = tlz::read_axis_registry_trio(COMDARE_CE_AXIS_REGISTRY, COMDARE_SYSTEM_AXIS_REGISTRY,
                                                     COMDARE_MEASUREMENT_AXIS_REGISTRY))
            return planner::ExperimentPlanDirector{std::move(*trio)};
#endif
        return planner::ExperimentPlanDirector{};
    }();
    // ============================================================================================
    // I-PMC-2 (Owner 10.08.2026) -- DIE EINE ERHEBUNG JE PLANER-LAUF.
    // ============================================================================================
    // OWNER-WORTLAUT (verbatim): "Dabei erkennt jeder Planer auf jeder Maschine fuer sich, ob PMC
    // existiert und ob daher das PMC in der CEB verbaut wird."
    // OWNER-PRAEZISIERUNG (verbatim): "Moment mal JEDER Planer? Es gibt ja nur einen Planer" -- richtig,
    // GENAU EINER (apps/experiment_planner). "Auf jeder Maschine" meint deshalb jeden LAUF dieses einen
    // Programms: HIER, an dieser Zeile, entscheidet sich, ob dieselbe Binary auf prod1 und prod2
    // Verschiedenes emittiert. Ohne diesen Aufruf waere die gesamte Erkennung toter Code.
    //
    // GENAU EINMAL, und nicht je Emissionsstelle: vier Erhebungen koennten vier Antworten geben (die
    // Rechte-Lage kann sich zwischen zwei Syscalls aendern) und die Emission in sich widerspruechlich
    // machen. Der Befund reist ab hier als Wert im Plan-Kopf.
    //
    // KOSTEN: ein Pointer-Chase ueber 64 MiB je Event, einmal je Planer-Lauf (gemessen ~20 ms auf prod1
    // fuer alle vier). Gegen einen Planer-Lauf, der danach eine mehrtaegige Messstrecke emittiert, ist
    // das nichts -- und es ist der Preis dafuer, dass der Koeder wirklich BEISST statt nur zu oeffnen.
    {
        planner::PmcHostBefund const befund = planner::probe_pmc_host<>();
        // Die Zeile geht ins Planer-Log, nicht nur in die Emission: wer den Planer-Lauf liest, sieht die
        // Grundlage seiner Entscheidung, samt Nenner und samt der Grenze der Aussage.
        os << "[" << what << "] " << befund.nenner_zeile() << "\n";
        director.set_pmc_befund(befund);
    }
    // smoke=>debug-Entkopplung (2026-07-22): das METHODIK-Profil (COMDARE_PLAN_METHODIK_PROFILE) liefert -- falls
    // gesetzt -- die run_methodology (steuert Bau-Typ + (j3)-Dual-Compile im emittierten Mess-Job), waehrend
    // profile_path den Bau-Katalog (Achsen/Perms) stellt. Single-Source-Resolver (resolve_methodik_override, oben);
    // unlesbar/>1-Methoden => KEIN Plan emittiert (harter Abbruch); unset => leer => byte-identisch.
    auto const methodik = resolve_methodik_override(profile_path, os); // S2-NACHT-3: Basename gg. thesis_profiles/
    if (!methodik.ok) return 1;
    if (root_tag == "comdare_thesis_profile") {
        auto const tp = tlz::load_thesis_profile(profile_path);
        if (!tp) {
            os << "[" << what << "] Thesis-Profil '" << profile_path.string()
               << "' nicht lesbar (parse_thesis_profile=nullopt). KEIN Plan emittiert.\n";
            return 5;
        }
        // S2-NACHT (2026-07-23): der Datei-Basename des AKTIVEN Profils reist bis in den Child-Prolog durch
        // (COMDARE_GOLDEN_N_PROFILE zeigt auf GENAU dieses Profil, nicht hart all_axes_golden). Quelle ist die DATEI
        // (profile_path.filename()), nicht tp.id (id != Basename moeglich, z.B. m3v2_sota_pilot.profile.xml id="C").
        director.construct(*tp, builder, combo_selector, methodik.run_methodology,
                           profile_path.filename().string()); // A5 combo leer=>Identitaet; methodik leer=>aus tp
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
        // S2-NACHT (2026-07-23): der Profil-Basename reist bis in den Child-Prolog durch (s. Thesis-Kanal oben).
        director.construct(*ep, builder, combo_selector, methodik.run_methodology,
                           profile_path.filename().string()); // A5 combo leer=>Identitaet; methodik leer=>aus ep
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

int assert_plan_nonempty_facade(std::filesystem::path const& profile_path, std::ostream& os) {
    // V-2/2a: derselbe Walk wie --dump-plan, nur mit einem zaehlenden statt einem textenden Builder.
    // Kein zweiter Mechanismus, keine zweite Wahrheit (§73.1).
    planner::PlanSizeBuilder builder;
    if (int const rc = construct_plan_into(profile_path, builder, os, "startgate"); rc != 0) return rc;
    if (builder.empty()) {
        os << "FATAL [V-2 Startgate]: Kein Experiment moeglich -- der Plan dieses Profils ist leer.\n"
           << "  Profil:        " << profile_path.string() << "\n"
           << "  Profil-Achsen: " << builder.profile_axis_count() << " (Werte: " << builder.profile_value_count()
           << ")\n"
           << "  Perms:         " << builder.perm_count() << "\n"
           << "  Schritte:      " << builder.step_count() << "\n"
           << "  Bedeutung:     0 Achsen = das Profil deklariert nichts zu permutieren; 0 Perms = keine\n"
           << "                 Rekombination geplant; Perms ohne Schritte = nichts zu messen.\n"
           << "  Naechstes:     Profil pruefen (--validate) bzw. --dump-plan fuer den vollen Plan-Text.\n";
        return 2;
    }
    os << "[V-2 Startgate] Plan traegt " << builder.profile_axis_count() << " Profil-Achse(n), " << builder.perm_count()
       << " Perm(s) mit " << builder.step_count() << " Schritt(en) -- Lauf startet.\n";
    return 0;
}

int dump_experiment_ci_facade(std::filesystem::path const& profile_path, std::ostream& os,
                              PlanerBlockContext const& pb) {
    // W7-A (--dump-ci, §40.b): der CiYamlBuilder-Traeger am geteilten Director-Walk. Emittiert die dynamische,
    // Planer-gesteuerte GitLab-Child-Pipeline-YAML (STUFE 1 CEB-Bau-Jobs + STUFE 2 Tier-Emit/Grandchild-Trigger).
    // G4b-2/E1: DIES ist eine der beiden Strecken, die real in einen CEB-Compile muenden -- deshalb haengt der
    // planer_block hier (und an --dump-cmake), NICHT an --dump-plan oder den --emit-tier-*-Befehlen.
    return run_with_planer_block(pb, [&]() -> EmissionErgebnis {
        planner::CiYamlBuilder  builder;
        LegendCollectingBuilder sammler{builder}; // Decorator: gleicher Walk, Legende fuer die CEB-Bindung
        int const               rc = construct_plan_into(profile_path, sammler, os, "dump-ci");
        if (rc == 0) os << builder.text();
        return {rc, sammler.eine_legende(), sammler.combo_count()};
    });
}

int dump_experiment_cmake_facade(std::filesystem::path const& profile_path, std::ostream& os,
                                 PlanerBlockContext const& pb) {
    // W7-B/W10-A (--dump-cmake, §40.c/§42): der CMakeGraphBuilder-Traeger (STUFE 1, Planer-Rolle) am geteilten
    // Director-Walk. Emittiert das Bare-Metal-experiment_plan.cmake der Mess-Achsen-Stufe (CEB-Bau + CEB-Emit).
    // G4b-2/E1: die zweite CEB-Compile-Strecke -- derselbe planer_block-Lebenszyklus wie bei --dump-ci.
    return run_with_planer_block(pb, [&]() -> EmissionErgebnis {
        planner::CMakeGraphBuilder builder;
        LegendCollectingBuilder    sammler{builder}; // Decorator wie im --dump-ci-Zweig
        int const                  rc = construct_plan_into(profile_path, sammler, os, "dump-cmake");
        if (rc == 0) os << builder.text();
        return {rc, sammler.eine_legende(), sammler.combo_count()};
    });
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
    namespace pf = ::comdare::cache_engine::profile_facade; // W10-C4: Zellwert-Naht + Suffix-Single-Source
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

    // -- (3b) S-3c (13.08.2026): DER EINE AUFRUF der aktiven Maschinen-Belegung -- NACH Parse+validate
    //    ((1)/(2) oben), VOR der Perm-Schleife (die lebt hinter der Delegation in (4)). live_hostname()
    //    ist NUR der Lookup-Schluessel gegen <machine hostname_hint=..>; belegt wird das Eigenschafts-
    //    Tupel (cpu_fabrication, ram_pair) -- O-4-Kette, KEIN zweiter Kanal, KEINE Heuristik, KEIN
    //    CMake-Schalter. Kein Treffer / keine Menge / kein Hostname => NICHTS belegen (natuerlicher
    //    Kill-Switch: das Gate bleibt inert, der ehrliche CPUID-Fallback in
    //    system_axis_host_supports_simd traegt weiter). AbgelehntAbweichend wird in der Naht als D1
    //    klassifiziert geloggt (konfig_xml_parse) -- KEIN Abbruch, die Erstbelegung bleibt. --
    {
        pf::MaschinenDeklarationsBefund const mb = pf::belege_aktive_maschinen_deklaration(
            ::comdare::cache_engine::measurement::live_hostname(), ep->machines, std::cerr);
        if (mb.belegt)
            std::cout << "[experiment_facade] S-3c: aktive Maschinen-Deklaration belegt (Treffer=" << mb.treffer
                      << ", verdict="
                      << ::comdare::cache_engine::measurement::machine_identity_verdict_label(
                             ::comdare::cache_engine::measurement::active_machine_verdict())
                      << ").\n";
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
    // T2-B (SPIEGEL der Profil-Naht): Basis-Defines ohne Glied [5], der per-Perm-Wert kommt unten dazu.
    a.compile_for_perm = [inc = perm_include_dirs(),
                          def = perm_compile_flags(nullptr, /*mit_toolchain_glied=*/false, args.build_version),
                          cxx = cxx_compiler(), libs = perm_link_libs(), fno = facade_supports_fno_gnu_unique(),
                          dbg =
                              facade_build_type_is_debug()](std::string const& opt_flag, std::string const& march_flag,
                                                            ::comdare::cache_engine::abi::SystemCellValues cell_values,
                                                            pf::PermToolchainGliedWert const& toolchain_glied) {
        // Scheibe 2b: Build-Typ Debug ersetzt die Optimierung (opt_flag) durch -O0 -g; -march ([d,e,f]-ISA-
        // Identitaet) und Gate-Flags bleiben. dbg==false => flags==opt_flag => byte-identisch zum Ist-Kanal.
        std::string flags =
            dbg ? ex::debug_flags_for_toolchain() : opt_flag; // opt-b-Kanal: eine rsp-Zeile, opt + optional -march
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
        // W10-C4 (SPIEGEL der Profil-Naht): das Zellwert-Define als eigenes Argument im defines-Kanal.
        std::vector<std::string> perm_defines = def;
        if (std::string arg = pf::system_cell_values_define_arg(cell_values.value); !arg.empty())
            perm_defines.push_back(std::move(arg));
        // T2-B (SPIEGEL): das PER-PERM-Glied [5] aus derselben Schleifen-Iteration wie der Zwilling.
        if (std::string arg = pf::toolchain_stamp_glied_define_arg(toolchain_glied.value); !arg.empty())
            perm_defines.push_back(std::move(arg));
        return ex::make_gpp_compile_fn(inc, std::move(perm_defines), cxx, libs, flags, fno);
    };
    a.compiler_tag = cxx_compiler(); // +cxx=-Provenienz im per-Perm-build_version
    // W10-C4: die beiden lauf-konstanten System-Zellen (SPIEGEL der Profil-Naht). Dieser Pfad kennt keine
    // Ziel-ISA-Deklaration -- die Aufloesung faellt damit auf die CT-Zelle der Bau-Plattform.
    a.system_cell_target_isa       = std::string{pf::resolve_system_cell_target_isa(std::string_view{})};
    a.system_cell_operating_system = std::string{pf::kSystemCellBuildOsFamily};
    // Bauplan §5/§7: dieselbe AlgoSigFn wie der Profile-Pfad -> auch der XML-Experiment-Lauf cached organ-genau.
    {
        auto algo_table = std::make_shared<std::vector<ex::AxisVariantVersion>>(ex::build_axis_variant_version_table());
        a.algo_sig      = [algo_table](std::vector<std::pair<std::string, std::string>> const& axes) {
            return ex::compose_algo_signature(axes, *algo_table);
        };
    }
    // Fallback-Einzel-CompileFn (greift nur, wenn compile_for_perm null wäre) = beweglicher CEB-Default (O3).
    // Scheibe 2b: bei Build-Typ Debug -O0 -g statt der Optimierung; ungesetzt/Release => byte-identisch zum Ist.
    a.compile = ex::make_gpp_compile_fn(
        perm_include_dirs(), perm_compile_flags(nullptr, /*mit_toolchain_glied=*/true, args.build_version),
        cxx_compiler(), perm_link_libs(),
        facade_build_type_is_debug() ? ex::debug_flags_for_toolchain() : perm_opt_level_cflags(),
        facade_supports_fno_gnu_unique());
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
    a.build_parallelism   = args.build_parallelism; // W6 (§32-F7): Bau-Pool-Override durchreichen (0 = byte-neutral)
    a.gn_cell_opt         = args.gn_cell_opt;       // W5-C+ (§36.1): GN-Zellen-Filter (Spiegel; leer = kein Filter)
    a.gn_cell_simd        = args.gn_cell_simd;      // W5-C+ (§36.1): GN-Zellen-Filter (Spiegel; leer = kein Filter)
    a.workload_registry   = std::move(workload_registry);
    a.workload_values     = std::move(workload_values);
    a.cache_push          = args.cache_push;          // Storage #51: No-Op-Naht durchreichen (byte-neutral)
    a.cache_pull          = args.cache_pull;          // S2 (#46a): BATCH-Warm-Cache-Hydrierung durchreichen (No-Op)
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

// Cache-Resthygiene-2 (2026-07-21): das PRE-IMAGE nach os (die CI sha256summt es zu COMDARE_GN_ALGO_SIG). Reine
// Katalog-Ableitung (kein DLL-Bau); Range/Empty-Verhalten in tlz::chunk_organ_fingerprint_preimage.
int chunk_organ_fingerprint_facade(std::filesystem::path const& profile_path, std::size_t range_start,
                                   std::size_t range_count, std::ostream& os) {
    os << tlz::chunk_organ_fingerprint_preimage(profile_path, range_start, range_count);
    return 0;
}

// R8 (Nacht-Audit 2026-07-22): --print-cache-key -- druckt den VOLLEN ce-Objekt-Cache-Key-Praefix
// cache_key_prefix(perm_build_version) fuer die per Env gepinnte GN-Zelle nach os (EINE Zeile). So konsumiert die CI
// (.golden_n_build) den Key LITERAL statt ihn in bash nachzubilden -> kein Key-Drift: die Segmente +bt/+ceb/+mtool/
// +mrg UND die Perm-Suffix-Reihenfolge kommen Single-Source aus dem Treiber. Der Perm-Suffix wird EXAKT in der
// perm-loop-Reihenfolge montiert (profile_run_entry.hpp: +cxx +opt +ext[!=no_extension] +bt); die single-XML-Naht
// system_axes_version_suffix nutzt eine ANDERE Reihenfolge (+ext+cxx+opt) -- fuer die GN-Cluster-Zellen und die
// YAML-GN_PREFIX ist die perm-loop-Reihenfolge autoritativ. Env: COMDARE_GN_OPT (opt), COMDARE_GN_SIMD (simd),
// COMDARE_CXX (via cxx_compiler(), +cxx), COMDARE_BUILD_TYPE (+bt), COMDARE_MEASUREMENT_COMBO (+mtool via
// ArtifactCache::from_env). Baut KEINE DLL, liest keinen Katalog. Rueckgabe 0.
int print_cache_key_facade(std::string const& base_build_version, std::ostream& os) {
    namespace cm   = ::comdare::cache_engine::measurement;
    namespace at   = ::comdare::cache_engine::builder::artifact_transport;
    auto const env = [](char const* key) -> std::string {
        char const* const v = std::getenv(key);
        return (v != nullptr) ? std::string{v} : std::string{};
    };
    std::string const opt  = env("COMDARE_GN_OPT");
    std::string const simd = env("COMDARE_GN_SIMD");
    // Lane F R3 (O-8 Schritt 10): auch dieser dritte Beitragsort liest jetzt die EINE Suffix-Quelle.
    // Er baute die bindende Form bereits richtig nach -- "richtig nachgebaut" ist aber genau der
    // Zustand, aus dem Divergenz entsteht.
    namespace pf = ::comdare::cache_engine::profile_facade;
    pf::SystemVersionSuffixParts parts;
    std::string const            cxx_tag = cxx_compiler();
    parts.cxx                            = cxx_tag;
    parts.opt                            = opt;
    if (simd != std::string{cm::SimdNoExtOption::simd_id()}) parts.simd = simd; // no_extension => KEIN +ext
    // W10-M2: seit C4 traegt die Perm-build_version das +ceb=-Glied SELBST (in der bindenden
    // kSuffixSegmentOrder-Position). Dieser CI-Key-Druck bildet die Perm-Reihenfolge nach und MUSS es deshalb
    // ebenfalls setzen -- sonst faltete cache_key_prefix es hier ans ENDE und die CI zeigte auf einen Bucket,
    // den kein Push je befuellt (Key-Drift genau der Klasse, gegen die dieser Druck ueberhaupt gebaut wurde).
    std::string const ceb   = pf::ceb_contract_version_text();
    parts.ceb               = ceb;
    std::string const bt    = tlz::build_type_version_value(); // (i) +bt=Debug nur bei COMDARE_BUILD_TYPE=Debug
    parts.build_type        = bt;
    std::string const gate  = cm::gate_contribution_identity_text(cm::route_of_simd_id(simd), cm::SimdDialect::Gpp);
    parts.gate_contribution = gate; // OP-7: am ENDE; leer => kein Segment
    std::string const       suffix = pf::compose_system_version_suffix(parts);
    at::ArtifactCache const cache = at::ArtifactCache::from_env(); // +mtool aus COMDARE_MEASUREMENT_COMBO, +ceb aus ABI
    os << cache.cache_key_prefix(base_build_version + suffix) << "\n";
    return 0;
}

// G1 (K7b-4, Section 62-B, B6-Auflage): --version -- druckt den Je-Binary-Selbst-Stempel des Treiber-Binary (Planer- +
// CEB-Rolle, EIN Binary) nach os. Vier gelabelte non-empty Zeilen (planner-Selbst-Stempel / ceb-contract / build-type /
// build-version). system_axes_version_suffix() ist die Single-Source der System-Achsen-build_version (enthaelt bereits
// +ceb/+bt/+ext/+cxx/+opt); die vier gelabelten Zeilen komponiert der header-only g1_binary_version_block. Rein-lesend,
// baut KEINE DLL, liest keinen Katalog. Rueckgabe 0.
int print_version_facade(std::ostream& os) {
    os << g1_binary_version_block(system_axes_version_suffix());
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------
// W5 (2026-08-05, Owner-R5): die beiden Naehte des status-Rueck-Lesers.
// ---------------------------------------------------------------------------------------------------------------

// Die Mess-Datei-Format-Fakten AUS DER SUBSTANZ, nicht nachgebaut: lazy_csv_header ist die EINE Schema-Wahrheit
// der per-Binary-CSV, kLazyResumeRowsKey der EINE Feld-Schluessel des Resume-Stempel-Schwanzes. Der Status-Leser
// bekommt sie hier durchgereicht, statt sie zu kopieren -- eine Schema-Aenderung zieht damit automatisch mit.
// T2-A/F4-NB2 (Befund 4): dazu kommt kLazyResumeStampFormat -- die FORMAT-MARKE am Kopf der Resume-Zeile,
// aus derselben Substanz und aus demselben Grund. Ohne sie las der Status-Leser einen Alt-Format-Stamp als
// gueltigen Messstand, waehrend der Runner ihn verwirft.
planner::MessFormatFakten mess_format_fakten_facade() {
    planner::MessFormatFakten f{};
    f.csv_header   = ex::lazy_csv_header();
    f.rows_key     = ex::kLazyResumeRowsKey;
    f.stamp_format = ex::kLazyResumeStampFormat;
    return f;
}

namespace {

// W5: der SAMMELNDE ConcreteBuilder des status-Kommandos. Er emittiert nichts -- er traegt den Walk in die
// flache SOLL-Sicht (GoF Builder: derselbe Director, andere Syntax). Der Zell-Schluessel entsteht aus DENSELBEN
// Legenden-Funktionen wie die Emission (plan_legend.hpp): ceb = [a,b,c] als EIGENES Feld, zelle =
// [d,e,f][g,h,i] -- die Layer werden NIE verschmolzen (Marker-v2-Gesetz). ceb_slug == cmake_slug(legend) ist
// genau das Verzeichnis, unter das der Mess-Batch nach <root>/<slug>/perm<idx> emittiert.
class StatusSollBuilder final : public planner::IPlanBuilder {
public:
    explicit StatusSollBuilder(planner::PlanSollSicht& out) noexcept : out_(out) {}

    void begin_plan(planner::PlanHeader const& h) override {
        out_.source_kind             = h.source_kind;
        out_.profile_id              = h.profile_id;
        out_.profile_basename        = h.profile_basename;
        out_.perm_count              = h.perm_count;
        out_.measurement_combo_count = h.measurement_combo_count;
    }
    void begin_measurement_combo(planner::PlanMeasurementCombo const& c) override {
        ceb_      = c.legend;
        ceb_slug_ = planner::legend::cmake_slug(c.legend);
    }
    void end_measurement_combo(planner::PlanMeasurementCombo const&) override {
        ceb_.clear();
        ceb_slug_.clear();
    }
    void begin_perm(planner::PlanPerm const& p) override {
        planner::PlanZelleSoll z{};
        z.ceb        = ceb_.empty() ? std::string{planner::kMarkerUnbelegt} : ceb_;
        z.ceb_slug   = ceb_slug_;
        z.perm_index = p.index;
        z.zelle      = planner::legend::system_perm(p.opt_id, p.simd_id) + planner::legend::organ_reference();
        out_.zellen.push_back(std::move(z));
    }
    void on_step(planner::PlanStep const&) override {
        if (!out_.zellen.empty()) ++out_.zellen.back().plan_schritte;
    }
    void end_perm(planner::PlanPerm const&) override {}
    void end_plan(planner::PlanHeader const&) override {}

private:
    planner::PlanSollSicht& out_;
    std::string             ceb_;
    std::string             ceb_slug_;
};

} // namespace

int collect_plan_soll_facade(std::filesystem::path const& profile_path, planner::PlanSollSicht& out, std::ostream& os) {
    out = planner::PlanSollSicht{};
    StatusSollBuilder builder{out};
    // DERSELBE Walk wie plan dump/ci/cmake -- ein Kanal, eine Wahrheit. KEIN Bau, KEINE Messung, KEINE Emission.
    int const rc = construct_plan_into(profile_path, builder, os, "status");
    if (rc != 0) {
        out.erhoben = false;
        out.grund =
            "Profil '" + profile_path.string() + "' nicht als bekannte Wurzel lesbar (rc " + std::to_string(rc) + ")";
        return rc;
    }
    out.erhoben = true;
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------
// check-size (2026-08-09): die MENGEN-ERHEBUNG. Derselbe Director-Walk, ein dritter sammelnder Builder.
// ---------------------------------------------------------------------------------------------------------------
namespace {

// Der MENGEN-Sammler -- GoF Builder wie StatusSollBuilder, andere Ernte. Er emittiert nichts, baut nichts,
// misst nichts. Er zaehlt die Zellen und liest die drei Kopf-Zahlen, die die Mengenrechnung braucht.
//
// WORST CASE UEBER DIE KOMBINATIONEN, mit Absicht: jede Mess-Kombination ist eine EIGENE CEB und damit ein
// EIGENER Mess-Prozess mit einer EIGENEN Arena. Bemessen werden muss die GROESSTE davon -- deshalb ODER
// ueber die aktiven Ebenen und nicht etwa ein Mittelwert. Eine Arena, die nur im Mittel passt, laeuft in
// der Haelfte der Faelle ueber.
class MengenSollBuilder final : public planner::IPlanBuilder {
public:
    explicit MengenSollBuilder(planner::MengenEingang& out) noexcept : out_(out) {}

    void begin_plan(planner::PlanHeader const& h) override {
        out_.perm_count         = static_cast<std::uint64_t>(h.perm_count);
        out_.combo_count        = static_cast<std::uint64_t>(h.measurement_combo_count);
        out_.profile_axis_count = static_cast<std::uint64_t>(h.profile_axis_count);
        out_.profile_id         = h.profile_id;
        out_.source_kind        = h.source_kind;
        out_.tooling_leer       = false; // wird wahr, sobald EINE Kombination leer ist (== volles Angebot)
    }
    void begin_measurement_combo(planner::PlanMeasurementCombo const& c) override {
        combo_gesehen_ = true;
        if (c.tooling.empty()) {
            out_.tooling_leer = true; // leer == volles Angebot (experiment_plan_director.hpp:109)
            return;
        }
        for (auto const& t : c.tooling) {
            if (t == "macro") out_.ebene_macro = true;
            if (t == "micro") out_.ebene_micro = true;
        }
    }
    void end_measurement_combo(planner::PlanMeasurementCombo const&) override {}
    void begin_perm(planner::PlanPerm const&) override { ++out_.zellen_gezaehlt; }
    void on_step(planner::PlanStep const&) override {}
    void end_perm(planner::PlanPerm const&) override {}
    void end_plan(planner::PlanHeader const&) override {
        // Kein einziger begin_measurement_combo => es gibt keine Kombinations-Stufe. Dann gilt das volle
        // Angebot, nicht "keine Ebene aktiv" -- fail-closed in Richtung der GROESSEREN Arena.
        if (!combo_gesehen_) out_.tooling_leer = true;
    }

private:
    planner::MengenEingang& out_;
    bool                    combo_gesehen_ = false;
};

// Der SIMULATIONS-Sammler (S-19 #7, 2026-08-20) -- UMHUELLT den Mengen-Sammler am SELBEN Walk (ein
// Kanal, eine Wahrheit: jede Callback-Zeile wird delegiert, nichts doppelt gezaehlt) und erntet
// zusaetzlich, was die Simulation braucht und der Mengen-Sammler bewusst ignoriert:
//   - den PMC-Befund des Plan-Kopfs (I-PMC-2: die EINE Erhebung dieses Laufs -- ein zweiter Walk
//     hiesse eine zweite Probe, und zwei Proben koennen zwei Antworten geben);
//   - die System-Perm-IDENTITAETEN (opt+simd) fuer die Kampagnen-Union (Zaehl-Wahrheit bleibt
//     mengen.perm_count -- die Ids dienen dem FULL JOIN, nicht der Zaehlung);
//   - die BREITE der groessten Tooling-Kombination (leer == volles Angebot, dieselbe Regel wie im
//     Mengen-Sammler; die Angebots-Breite kommt aus kMeasurementToolingCount, keinem Literal).
class SimulationsSollBuilder final : public planner::IPlanBuilder {
public:
    explicit SimulationsSollBuilder(planner::SimulationsEingang& out) noexcept : out_(out), mengen_(out.mengen) {}

    void begin_plan(planner::PlanHeader const& h) override {
        mengen_.begin_plan(h);
        namespace cm        = ::comdare::cache_engine::measurement;
        out_.pmc_tor        = (h.pmc_befund.lage != planner::PmcLage::Unbrauchbar);
        out_.pmc_lage_label = std::string{h.pmc_befund.lage_label()};
        out_.pmc_nenner     = h.pmc_befund.nenner_zeile();
        // Vorbelegung "volles Angebot": gilt, bis eine NICHT-leere Kombination etwas anderes sagt --
        // fail-closed in Richtung der GROESSEREN Geraete-Zahl (Spiegel der tooling_leer-Logik).
        out_.messgeraete_basis  = static_cast<std::uint64_t>(cm::kMeasurementToolingCount);
        out_.messgeraete_art    = planner::MengenArt::Konstante;
        out_.messgeraete_nenner = "volles Angebot -- kMeasurementToolingRegistry "
                                  "(mess_axes/measurement_tooling_registry.hpp)";
    }
    void begin_measurement_combo(planner::PlanMeasurementCombo const& c) override {
        mengen_.begin_measurement_combo(c);
        namespace cm               = ::comdare::cache_engine::measurement;
        std::uint64_t const breite = c.tooling.empty() ? static_cast<std::uint64_t>(cm::kMeasurementToolingCount)
                                                       : static_cast<std::uint64_t>(c.tooling.size());
        if (!explizite_combo_gesehen_ || breite > out_.messgeraete_basis) {
            out_.messgeraete_basis = breite;
            if (c.tooling.empty()) {
                out_.messgeraete_art    = planner::MengenArt::Konstante;
                out_.messgeraete_nenner = "Kombination LEER == volles Angebot (kMeasurementToolingRegistry)";
            } else {
                out_.messgeraete_art    = planner::MengenArt::Xml;
                out_.messgeraete_nenner = "groesste <measurement_tooling><combo>-Konfiguration des Walks";
            }
        }
        explizite_combo_gesehen_ = true;
    }
    void end_measurement_combo(planner::PlanMeasurementCombo const& c) override { mengen_.end_measurement_combo(c); }
    void begin_perm(planner::PlanPerm const& p) override {
        mengen_.begin_perm(p);
        perm_ids_.insert(p.opt_id + "+" + p.simd_id);
    }
    void on_step(planner::PlanStep const& s) override {
        mengen_.on_step(s);
        ++out_.walk_schritte;
    }
    void end_perm(planner::PlanPerm const& p) override { mengen_.end_perm(p); }
    void end_plan(planner::PlanHeader const& h) override {
        mengen_.end_plan(h);
        out_.system_perm_ids.assign(perm_ids_.begin(), perm_ids_.end());
    }

private:
    planner::SimulationsEingang& out_;
    MengenSollBuilder            mengen_;
    std::set<std::string>        perm_ids_;
    bool                         explizite_combo_gesehen_ = false;
};

// Die PROFIL-/ENV-NACHLESE der Mengen-Erhebung -- aus collect_mess_menge_facade EXTRAHIERT (S-19,
// 2026-08-20), weil die Simulations-Erhebung DIESELBEN Zahlen braucht: n_ops/drift (nur am
// Thesis-Profil; comdare_experiment hat die Felder nicht -- ehrlicher Befund), das Korn und
// COMDARE_GN_TOTAL. EIN Ort statt zweier, damit die beiden Kommandos nie auseinanderlaufen.
void mengen_profil_und_env_nachlese(std::filesystem::path const& profile_path, planner::MengenEingang& out) {
    if (out.source_kind == "thesis") {
        if (auto const tp = tlz::load_thesis_profile(profile_path)) {
            // n_ops sitzt NICHT direkt am ThesisProfile, sondern an dessen <run_options>-Unterstruktur
            // (ThesisRunOptions::n_ops, xml_config_parser.hpp:194). 0 == ungesetzt => Treiber-Default.
            if (tp->run_options.n_ops != 0u) {
                out.n_ops         = tp->run_options.n_ops;
                out.n_ops_aus_xml = true;
            }
            if (tp->drift_gate_declared) {
                out.drift_reps       = static_cast<std::uint64_t>(tp->drift_gate_reps);
                out.drift_max_reruns = static_cast<std::uint64_t>(tp->drift_gate_max_reruns);
                out.drift_aus_xml    = true;
            }
        }
    }
    // Der Rueckfall ist DERSELBE, den der Mess-Pfad nimmt -- keine zweite Wahrheit.
    if (out.n_ops == 0u) {
        out.n_ops         = ExperimentRunArgs{}.n_ops; // 10000 (profile_run_facade.hpp:195)
        out.n_ops_aus_xml = false;
    }
    if (!out.drift_aus_xml) {
        ex::DriftGateConfig const d{}; // die PRODUKTIVEN Defaults (drift_gated_cell.hpp:74)
        out.drift_reps       = d.reps;
        out.drift_max_reruns = d.max_reruns;
    }

    // Das Korn und die Kampagnen-Breite. Das Korn kommt aus planner::kGnBatchSlice -- KEIN viertes
    // 4096-Literal (die Korn-Wache haelt drei static_assert auf genau diese Konstante).
    out.batch_korn = static_cast<std::uint64_t>(planner::kGnBatchSlice);
    if (std::string const gn = planner::env_trimmed("COMDARE_GN_TOTAL"); !gn.empty()) {
        errno           = 0;
        char*      ende = nullptr;
        auto const wert = std::strtoull(gn.c_str(), &ende, 10);
        if (errno == 0 && ende != nullptr && *ende == '\0' && wert != 0u) {
            out.binaries_je_perm = wert;
            out.binaries_aus_env = true;
        }
    }
}

} // namespace

int collect_mess_menge_facade(std::filesystem::path const& profile_path, planner::MengenEingang& out,
                              std::ostream& os) {
    // Frischer Stand wie bei collect_plan_soll_facade: der Aufrufer setzt SEINE Zutaten (sekunden_je_op,
    // deckel_*) erst NACH dieser Funktion, ein Reset hier kann sie darum nicht loeschen.
    out = planner::MengenEingang{};

    // 1. Der Walk -- die exakten Zahlen (Perms, Kombinationen, Zellen, Achsen).
    MengenSollBuilder builder{out};
    if (int const rc = construct_plan_into(profile_path, builder, os, "check-size"); rc != 0) return rc;

    // 2.+3. Die PROFIL-Zahlen, die der Walk nicht traegt (n_ops/Drift-Gate -- NUR am Thesis-Profil,
    // comdare_experiment hat die Felder nicht, ehrlicher Befund am Helfer) + Korn/Kampagnen-Breite.
    // EXTRAHIERT in mengen_profil_und_env_nachlese (S-19, 2026-08-20): die Simulations-Erhebung braucht
    // DIESELBEN Zahlen -- ein Ort, damit die Kommandos nie auseinanderlaufen.
    mengen_profil_und_env_nachlese(profile_path, out);
    return 0;
}

int collect_simulation_eingang_facade(std::filesystem::path const& profile_path, planner::SimulationsEingang& out,
                                      std::ostream& os) {
    out = planner::SimulationsEingang{};

    // 1. EIN Walk fuer beide Ernten (der Simulations-Sammler delegiert an den Mengen-Sammler) --
    //    damit gibt es auch nur EINE PMC-Probe (I-PMC-2) und EINE Diagnose-Zeile.
    {
        SimulationsSollBuilder builder{out};
        if (int const rc = construct_plan_into(profile_path, builder, os, "simulate"); rc != 0) return rc;
    }

    // 2. Dieselbe Profil-/env-Nachlese wie check-size (ein Ort, eine Wahrheit).
    mengen_profil_und_env_nachlese(profile_path, out.mengen);

    // 3. Die S-19-Zusatzernte aus dem Profil: Freigabe je Organ-Achse + dynamische Dimensionen.
    //    Die Freigabe-Regel ist EXAKT die build_axis_levels-Regel (profile_to_tree.hpp:133-139):
    //    nur Organ-Kompositions-Achsen zaehlen; leere Werteliste == volle Registry-Liste.
    if (out.mengen.source_kind == "thesis") {
        auto const tp = tlz::load_thesis_profile(profile_path);
        if (!tp) return 5; // Walk las das Profil eben noch -- Verschwinden ist ein harter Fehler.
        ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());
        // Die FREIGABE-Regel wird VOLLSTAENDIG gespiegelt (build_axis_levels, profile_to_tree.hpp:59-139):
        //   (1) Modus-Freigabe: mit Modi zaehlt eine Achse nur, wenn IRGENDEIN Modus sie in active_axes
        //       fuehrt (die Kampagne baut die Union der Modi; ohne Modi sind alle frei);
        //   (2) <axis active="false"> nimmt die Achse aus der Ebene (A9b/P6);
        //   (3) die KF-3-/FF2-/F-B-UNTERACHSEN (cacheline/node_width/alloc_hw) erzeugen EIGENE statische,
        //       binary_id-tragende Ebenen VOR dem Organ-Guard -- sie zaehlen darum als EIGENE Faktoren
        //       (m3v2/golden deklarieren keine; der Zweig ist fuer Profile, die sie fuehren).
        auto modus_frei = [&tp](std::string const& ref) -> bool {
            if (tp->modes.empty()) return true; // kein Modus -> alle frei (build_axis_levels:64)
            for (auto const& m : tp->modes)
                for (auto const& a : m.active_axes)
                    if (a == ref) return true;
            return false;
        };
        auto sub_achse = [&out](char const* name, std::vector<std::string> const& werte) {
            if (werte.empty()) return;
            out.organ_achsen.push_back(planner::SimAchse{name, werte, planner::MengenArt::Xml,
                                                         "statische Unterachsen-Ebene (binary_id-tragend, "
                                                         "profile_to_tree.hpp:104-125)"});
        };
        for (auto const& ax : tp->permute_axes) {
            if (!modus_frei(ax.ref) || !ax.active) continue;
            if (ax.ref == "cacheline") {
                sub_achse("cacheline.line_size", ax.line_sizes);
                sub_achse("cacheline.alignment", ax.alignments);
                sub_achse("cacheline.sw_hint", ax.sw_prefetch_hints);
                continue;
            }
            if (ax.ref == "node_width") {
                sub_achse("node_width.width_in_lines", ax.width_in_lines);
                continue;
            }
            if (ax.ref == "alloc_hw") {
                sub_achse("alloc_hw.numa_node", ax.alloc_numa_nodes);
                sub_achse("alloc_hw.page", ax.alloc_pages);
                continue;
            }
            if (!ex::is_organ_composition_axis(ax.ref)) continue;
            planner::SimAchse a;
            a.name = ax.ref;
            if (!ax.values.empty()) {
                a.werte  = ax.values;
                a.art    = planner::MengenArt::Xml;
                a.nenner = "<permute_axes><axis ref> -- explizite Freigabe-Werte";
            } else if (auto const it = registry.find(ax.ref); it != registry.end()) {
                a.werte  = it->second;
                a.art    = planner::MengenArt::Default;
                a.nenner = "leere Werteliste == volle Registry-Liste (build_axis_levels-Regel)";
            } else {
                a.art    = planner::MengenArt::Unbestimmbar;
                a.nenner = "leere Werteliste und Achse nicht in der Registry -- 0 Werte (fail-closed)";
            }
            out.organ_achsen.push_back(std::move(a));
        }
        // Dynamische Laufzeit-Dimensionen -- gespiegelte build_axis_levels-Emission (profile_to_tree.hpp:
        // 149-161) OHNE repetition (KF-10 zaehlt genau einmal, s. planner_simulation.hpp Kopf) PLUS die
        // zwei Schleifen, die NICHT aus build_axis_levels kommen: workload (eigene DynDim-Injektion,
        // profile_run_entry.hpp:499-501) und working_set (aeussere N-Schleife, profile_run_entry.hpp:744).
        auto dyn = [&out](char const* name, std::size_t n, char const* nenner) {
            if (n == 0u) return;
            out.dyn_dims.push_back(
                planner::SimDynDim{name, static_cast<std::uint64_t>(n), planner::MengenArt::Xml, nenner});
        };
        dyn("thread_count", tp->thread_counts.size(), "<runtime_dynamic><thread_count> (build_axis_levels)");
        dyn("hw_prefetcher", tp->hw_prefetcher.size(), "<runtime_dynamic><hw_prefetcher> (build_axis_levels)");
        dyn("prefetch_distance", tp->prefetch_distances.size(), "<runtime_dynamic> (build_axis_levels)");
        dyn("pool_budget_bytes", tp->pool_budgets_bytes.size(), "<runtime_dynamic> (build_axis_levels)");
        dyn("batch_size", tp->batch_sizes.size(), "<runtime_dynamic> (build_axis_levels)");
        dyn("inline_threshold_bytes", tp->inline_thresholds_bytes.size(), "<runtime_dynamic> (build_axis_levels)");
        dyn("workload", tp->workloads.size(),
            "<compile_dims><workloads> -- eigene DynDim-Injektion (profile_run_entry.hpp:499-501), Achse-2");
        dyn("working_set", tp->working_set_sweep.size(),
            "<working_set_sweep> -- aeussere N-Schleife des Laufs (profile_run_entry.hpp:744-760)");
        out.kf10_repetitions = static_cast<std::uint64_t>((tp->repetitions <= 0) ? 1 : tp->repetitions);
        out.kf10_art         = planner::MengenArt::Default;
        out.kf10_nenner      = "ThesisProfile::repetitions -- XML <repetitions count> ODER Parser-Default 3; "
                               "die Deklaration ist am POD nicht unterscheidbar (xml_config_parser.hpp:265)";
    } else if (out.mengen.source_kind == "experiment") {
        cx::XmlConfigParser const parser;
        auto const                ep = parser.parse_experiment_profile(profile_path);
        if (!ep) return 5;
        ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());
        for (auto const& ax : ep->axes_default_lookup) {
            if (!ex::is_organ_composition_axis(ax.ref)) continue;
            planner::SimAchse a;
            a.name = ax.ref;
            if (!ax.allowed_variants.empty()) {
                a.werte  = ax.allowed_variants;
                a.art    = planner::MengenArt::Xml;
                a.nenner = "<axes_default_lookup><axis allowed_variants> -- explizite Freigabe";
            } else if (auto const it = registry.find(ax.ref); it != registry.end()) {
                a.werte  = it->second;
                a.art    = planner::MengenArt::Default;
                a.nenner = "leere Variantenliste == volle Registry-Liste (Limit-Schicht ohne Limit)";
            } else {
                a.art    = planner::MengenArt::Unbestimmbar;
                a.nenner = "leere Variantenliste und Achse nicht in der Registry -- 0 Werte (fail-closed)";
            }
            out.organ_achsen.push_back(std::move(a));
        }
        if (!ep->workloads.empty()) {
            out.dyn_dims.push_back(planner::SimDynDim{"workload", static_cast<std::uint64_t>(ep->workloads.size()),
                                                      planner::MengenArt::Xml, "<workloads> (comdare_experiment)"});
        }
        // EHRLICHER BEFUND (Spiegel n_ops/drift): comdare_experiment traegt KEIN <repetitions> --
        // die Wiederholungszahl ist fuer diese Wurzel nicht erhebbar und wird nicht erfunden.
        out.kf10_repetitions = 0u;
        out.kf10_art         = planner::MengenArt::Unbestimmbar;
        out.kf10_nenner      = "comdare_experiment traegt kein <repetitions>-Feld (xml_config_parser.hpp:467-495)";
    }
    return 0;
}

} // namespace comdare::cache_engine::builder::profile_facade
