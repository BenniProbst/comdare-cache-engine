// test_validate_profile — M-CE-12 (Voll-Review 2026-07-13): das ctest-Gate der rein-lesenden
// <workloads>-id-Pruefung in validate_profile (SCHEMA.md:73 wieder wahr; die Negativ-Pfade der
// --validate-Preflight sind damit getestet, statt erst im teuren E4-Lauf mit exit 4 aufzufallen).
//
// HINTERGRUND (M-CE-11/12): die <workloads>-Auswahl im comdare_thesis_profile ist die AUTORITATIVE
// Achse-2-Auswahl (profile_run_facade.cpp) — es sind die ids der Lastprofile in
// algorithm_profiles/load_profiles/ (ycsb_a..ycsb_f, lp_*, coco_*, ih, lh). Eine getippte id (z.B. das
// alte "A C" statt "ycsb_a ycsb_c") matcht 0 Lastprofile → der Lauf braeche mit exit 4 ab. validate_profile
// bekam bis M-CE-12 die real vorhandenen ids GAR NICHT und liess solche Profile durch --validate passieren.
//
// BEWEIST LITERAL:
//   (a) UNBEKANNTE Workload-id (z.B. "ycsb_zzz" / das alte "A") wird ABGELEHNT (ok=false + klare Meldung).
//   (b) ein ycsb_*-Profil wird AKZEPTIERT (ok=true, workloads_checked == Anzahl der ids).
//   (c) LEERE <workloads> = "alle Lastprofile" = OK (workloads_checked==0, nichts zu pruefen).
//   (d) RUECKWAERTS-KOMPATIBEL: der 2-arg-Aufruf (ohne Host-enumerierte ids) UEBERSPRINGT die Pruefung
//       (workloads_checked==0) — auch bei einem Profil mit kaputten ids; die Pruefung ist rein additiv.
//   (e) REGRESSIONS-WACHE M-CE-11: die real committeten m3v2_smoke/m3v2_sota_pilot-Profile tragen jetzt
//       gueltige ycsb_*-ids und validieren gegen die REAL entdeckten load_profiles/-ids mit ok=true;
//       das frueher dort stehende "A"/"A B C..." wuerde von derselben Pruefung ABGELEHNT.
//
// TABU-Wache: m3v2_study.profile.xml wird NICHT angefasst (byte-frozen); dieser Test liest die
// load_profiles/-Verzeichnis-ids nur (kein Schreiben, kein Bau, keine Messung).

#include "profile_run_facade.hpp" // S2-NACHT-3: resolve_methodik_profile_path (Basename-Haertung)
#include "profile_runner.hpp"     // load_thesis_profile
#include "validate_profile.hpp"   // validate_profile / axis_registry_from_levels / print_validation_report

#include <builder/experiment_tree/registry_to_axis_levels.hpp> // build_all_axis_levels (EnabledStrategies)
#include <builder/workload_driver/load_profile_parser.hpp>     // discover_load_profiles (real load_profiles/-ids)

#include "xml_config_parser/xml_config_parser.hpp" // XmlConfigParser (Temp-Fixture-Parse)

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#ifndef COMDARE_THESIS_PROFILES_DIR
#error "COMDARE_THESIS_PROFILES_DIR must point to libs/cache_engine/algorithm_profiles/thesis_profiles"
#endif
#ifndef COMDARE_CE_AXIS_REGISTRY
#error "COMDARE_CE_AXIS_REGISTRY must point to cache_engine_axis_registry.xml (S3 P-RESOLVER)"
#endif
#ifndef COMDARE_SYSTEM_AXIS_REGISTRY
#error "COMDARE_SYSTEM_AXIS_REGISTRY must point to system_axis_registry.xml (S3 P-RESOLVER)"
#endif
#ifndef COMDARE_MEASUREMENT_AXIS_REGISTRY
#error "COMDARE_MEASUREMENT_AXIS_REGISTRY must point to measurement_axis_registry.xml (S3 P-RESOLVER)"
#endif

namespace {

namespace cx  = comdare::builder::xml;
namespace ex  = comdare::cache_engine::builder::experiment;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace wd  = comdare::cache_engine::builder::workload_driver;
namespace fs  = std::filesystem;

fs::path thesis_profiles_dir() { return fs::path{COMDARE_THESIS_PROFILES_DIR}; }
// load_profiles/ ist der Schwesterordner von thesis_profiles/ (co-lokalisiert, wie in der Fassade).
fs::path load_profiles_dir() { return thesis_profiles_dir().parent_path() / "load_profiles"; }

// Die REAL im Repo vorhandenen load_profiles/-ids (dieselbe Quelle wie der Host: discover_load_profiles).
std::set<std::string> real_workload_ids() {
    std::set<std::string> ids;
    for (auto const& idp : wd::discover_load_profiles(load_profiles_dir())) ids.insert(idp.first);
    return ids;
}

// Temp-Fixture (Muster test_wdk_datasets_fairness::write_temp_profile): ein Minimal-Profil mit frei
// waehlbarem <compile_dims><workloads>-Inhalt. `workloads_block` leer = kein <compile_dims> (= leere Auswahl).
fs::path write_temp_profile(std::string const& workloads_block) {
    fs::path const p = fs::temp_directory_path() /
                       ("comdare_validate_workloads_fixture_" +
                        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".xml");
    std::ofstream  out{p};
    out << R"(<?xml version="1.0" encoding="UTF-8"?>
<comdare_thesis_profile id="validate_wl_fixture" schema_version="1">
  <base_tiers><tier id="hot" profile_ref="../sota/hot.profile.xml" paper_ref="P02"/></base_tiers>
  <permute_axes><axis ref="search_algo"><value>k_ary</value></axis></permute_axes>
)" << workloads_block
        << R"(  <modes><mode name="ce_only" merge="Stufe1_CeOnly" active_axes="search_algo"/></modes>
  <static_axes from="base_tier"/>
</comdare_thesis_profile>
)";
    return p;
}

std::optional<cx::ThesisProfile> parse_temp(std::string const& workloads_block) {
    fs::path const                   p = write_temp_profile(workloads_block);
    cx::XmlConfigParser const        parser;
    std::optional<cx::ThesisProfile> tp = parser.parse_thesis_profile(p);
    std::error_code                  ec;
    fs::remove(p, ec);
    return tp;
}

bool any_contains(std::vector<std::string> const& msgs, std::string const& needle) {
    for (auto const& m : msgs)
        if (m.find(needle) != std::string::npos) return true;
    return false;
}

std::string compile_dims(std::string const& ids) {
    return "  <compile_dims>\n    <workloads>" + ids +
           "</workloads>\n    <telemetry mode=\"on\" silent=\"true\"/>\n"
           "  </compile_dims>\n";
}

} // namespace

// (a) UNBEKANNTE Workload-id ⇒ HARTER Fehler (ok=false), klare Meldung mit id + gueltigen ids.
TEST(ValidateProfileWorkloads, UnknownWorkloadIdIsRejected) {
    ex::AxisRegistry const      registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());
    std::set<std::string> const known    = {"ycsb_a", "ycsb_c"};

    auto const tp = parse_temp(compile_dims("ycsb_a ycsb_zzz"));
    ASSERT_TRUE(tp.has_value());
    tlz::ProfileValidationResult const vr = tlz::validate_profile(*tp, registry, known);
    EXPECT_FALSE(vr.ok);
    EXPECT_EQ(vr.workloads_checked, 2u);
    EXPECT_TRUE(any_contains(vr.errors, "UNBEKANNTE Workload-id"));
    EXPECT_TRUE(any_contains(vr.errors, "ycsb_zzz"));
    // die klassische M-CE-11-Fehlerform "A"/"C" (AchsenNAME statt load_profile-id) wird ebenso gefangen.
    auto const tp_old = parse_temp(compile_dims("A C"));
    ASSERT_TRUE(tp_old.has_value());
    tlz::ProfileValidationResult const vold = tlz::validate_profile(*tp_old, registry, known);
    EXPECT_FALSE(vold.ok);
    EXPECT_EQ(vold.workloads_checked, 2u);
}

// (b) ein ycsb_*-Profil wird AKZEPTIERT; workloads_checked == Anzahl der ids; Report nennt "2 workloads".
TEST(ValidateProfileWorkloads, YcsbWorkloadsAreAccepted) {
    ex::AxisRegistry const      registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());
    std::set<std::string> const known    = {"ycsb_a", "ycsb_b", "ycsb_c", "ycsb_d", "ycsb_e", "ycsb_f"};

    auto const tp = parse_temp(compile_dims("ycsb_a ycsb_c"));
    ASSERT_TRUE(tp.has_value());
    tlz::ProfileValidationResult const vr = tlz::validate_profile(*tp, registry, known);
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok);
    EXPECT_EQ(vr.workloads_checked, 2u);
    std::ostringstream os;
    tlz::print_validation_report(vr, *tp, os);
    EXPECT_NE(os.str().find("2 workloads"), std::string::npos) << os.str();
}

// (c) LEERE <workloads> (kein <compile_dims>) = "alle Lastprofile" = OK, nichts zu pruefen.
TEST(ValidateProfileWorkloads, EmptyWorkloadsIsOk) {
    ex::AxisRegistry const      registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());
    std::set<std::string> const known    = {"ycsb_a", "ycsb_c"};

    auto const tp = parse_temp(""); // kein <compile_dims> → tp.workloads leer
    ASSERT_TRUE(tp.has_value());
    ASSERT_TRUE(tp->workloads.empty());
    tlz::ProfileValidationResult const vr = tlz::validate_profile(*tp, registry, known);
    EXPECT_TRUE(vr.ok);
    EXPECT_EQ(vr.workloads_checked, 0u);
    std::ostringstream os;
    tlz::print_validation_report(vr, *tp, os);
    EXPECT_EQ(os.str().find("workloads"), std::string::npos) << "leere Auswahl darf keine workloads-Zeile erzeugen:\n"
                                                             << os.str();
}

// (d) RUECKWAERTS-KOMPATIBEL: der 2-arg-Aufruf (Default-leere known_workload_ids) UEBERSPRINGT die Pruefung —
// auch bei kaputten ids bleibt workloads_checked==0 (die Pruefung ist strikt additiv/opt-in ueber den Host).
TEST(ValidateProfileWorkloads, TwoArgCallSkipsWorkloadCheck) {
    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());

    auto const tp = parse_temp(compile_dims("A C")); // kaputte ids
    ASSERT_TRUE(tp.has_value());
    tlz::ProfileValidationResult const vr = tlz::validate_profile(*tp, registry); // 2-arg (known leer)
    EXPECT_TRUE(vr.ok) << "ohne hereingereichte ids wird die <workloads>-Pruefung uebersprungen";
    EXPECT_EQ(vr.workloads_checked, 0u);
}

// (e) REGRESSIONS-WACHE M-CE-11: die real committeten m3v2_smoke/m3v2_sota_pilot tragen jetzt gueltige
// ycsb_*-ids und validieren gegen die REAL entdeckten load_profiles/-ids OK; das alte "A"/"A B C.." fiele.
TEST(ValidateProfileWorkloads, RealFixedProfilesValidateAgainstRealLoadProfiles) {
    ex::AxisRegistry const      registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());
    std::set<std::string> const known    = real_workload_ids();
    ASSERT_FALSE(known.empty()) << "load_profiles/ nicht gefunden: " << load_profiles_dir().string();
    EXPECT_TRUE(known.count("ycsb_a") == 1u && known.count("ycsb_f") == 1u) << "erwartete ycsb_a..f in load_profiles/";

    for (char const* name : {"m3v2_smoke.profile.xml", "m3v2_sota_pilot.profile.xml"}) {
        auto const tp = tlz::load_thesis_profile(thesis_profiles_dir() / name);
        ASSERT_TRUE(tp.has_value()) << name;
        ASSERT_FALSE(tp->workloads.empty()) << name << ": traegt eine explizite <workloads>-Auswahl";
        for (auto const& w : tp->workloads)
            EXPECT_EQ(w.rfind("ycsb_", 0), 0u)
                << name << ": <workloads> muss load_profile-ids (ycsb_*) tragen, nicht '" << w << "'";
        tlz::ProfileValidationResult const vr = tlz::validate_profile(*tp, registry, known);
        for (auto const& e : vr.errors) ADD_FAILURE() << name << " [validate] " << e;
        EXPECT_TRUE(vr.ok) << name;
        EXPECT_EQ(vr.workloads_checked, tp->workloads.size()) << name;
    }

    // Gegenprobe: die alte kaputte Auswahl "A C" wuerde von derselben Pruefung ABGELEHNT.
    auto const bad = parse_temp(compile_dims("A C"));
    ASSERT_TRUE(bad.has_value());
    tlz::ProfileValidationResult const vbad = tlz::validate_profile(*bad, registry, known);
    EXPECT_FALSE(vbad.ok) << "die vor-M-CE-11-Auswahl 'A C' muss gegen die realen ids scheitern";
}

// ─────────────────────────────────────────────────────────────────────────────
// S2/A2 P-SYSREG (2026-07-20): die zwei NEU verdrahteten System-Achsen-Consumer im Thesis-Kanal —
// compiler/atomic128 (Single-Source kAllAtomic128Ids) + target_isa (kAllTargetIsaIds). Struct-Injektion
// (die geteilte parse_system_axes-Naht ist end-to-end in test_experiment_parser bewiesen); hier die
// Validat-Bloecke + Report-Zeilen. binary_id-neutral (system_config) -> golden-Roundtrip unberuehrt.
// ─────────────────────────────────────────────────────────────────────────────

// (f) gueltige atomic128 + target_isa werden AKZEPTIERT; die Zaehler + Report-Zeilen stimmen.
TEST(ValidateProfileSystemAxes, ValidAtomic128AndTargetIsaAccepted) {
    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());

    auto tp = parse_temp(""); // Minimal-Profil, keine <workloads>
    ASSERT_TRUE(tp.has_value());
    tp->compiler.atomic128 = {"no_cx16", "cx16"};
    tp->target_isa.isa     = {"x86_64", "aarch64"};

    tlz::ProfileValidationResult const vr = tlz::validate_profile(*tp, registry);
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok);
    EXPECT_EQ(vr.atomic128_checked, 2u);
    EXPECT_EQ(vr.target_isa_checked, 2u);
    std::ostringstream os;
    tlz::print_validation_report(vr, *tp, os);
    EXPECT_NE(os.str().find("2 atomic128"), std::string::npos) << os.str();
    EXPECT_NE(os.str().find("2 target_isa"), std::string::npos) << os.str();
}

// (g) ein Bogus-atomic128-Wert (Tippfehler cx32) ist ein HARTER Fehler (erlaubt: no_cx16/cx16).
TEST(ValidateProfileSystemAxes, BogusAtomic128Rejected) {
    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());

    auto tp = parse_temp("");
    ASSERT_TRUE(tp.has_value());
    tp->compiler.atomic128 = {"cx32"};

    tlz::ProfileValidationResult const vr = tlz::validate_profile(*tp, registry);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "atomic128"));
    EXPECT_TRUE(any_contains(vr.errors, "cx32"));
    EXPECT_EQ(vr.atomic128_checked, 1u);
}

// (h) ein Bogus-target_isa-Wert (Tippfehler riscv) ist ein HARTER Fehler (erlaubt: x86_64/aarch64).
TEST(ValidateProfileSystemAxes, BogusTargetIsaRejected) {
    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());

    auto tp = parse_temp("");
    ASSERT_TRUE(tp.has_value());
    tp->target_isa.isa = {"riscv"};

    tlz::ProfileValidationResult const vr = tlz::validate_profile(*tp, registry);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "target_isa"));
    EXPECT_TRUE(any_contains(vr.errors, "riscv"));
    EXPECT_EQ(vr.target_isa_checked, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// S3 P-RESOLVER (minimal-3-tief, 2026-07-20): resolve_axis_refs_against_trio macht das heute STILLE Verwerfen
// fehlplatzierter Organ-Position-Achsen (profile_to_tree.hpp is_organ_composition_axis -> `continue`) zu einem
// KLASSIFIZIERTEN, LAUTEN Reject/Route. Gegen das REALE 3-Art-Angebot (17 Organ / 5 System / 16 Mess) aufgeloest
// -- Single-Source, keine hartkodierte Achsenliste. Je Fehlerklasse >=1 Negativ-Test mit DETERMINISTISCHER Meldung
// MIT Koordinate (STUFE §4.4-I1); der Positiv-Fall (organ-reines Profil) belegt 0 Rejects. READ-ONLY, binary_id-neutral.
// ─────────────────────────────────────────────────────────────────────────────
namespace {
// Das REALE RegistryTrio (committete Art-Registries) -- dieselbe Naht wie der Host/Director (read_axis_registry_trio).
std::optional<tlz::RegistryTrio> real_trio() {
    return tlz::read_axis_registry_trio(fs::path{COMDARE_CE_AXIS_REGISTRY}, fs::path{COMDARE_SYSTEM_AXIS_REGISTRY},
                                        fs::path{COMDARE_MEASUREMENT_AXIS_REGISTRY});
}
} // namespace

// (P0) Das reale Trio traegt 17/5/16 und die kanonischen Achsen (Grundlage der Klassifikation) -- unveraendert gruen.
TEST(ResolveAxisRefsAgainstTrio, RealTrioIs17_5_16WithCanonicalAxes) {
    auto const trio = real_trio();
    ASSERT_TRUE(trio.has_value()) << "die 3 committeten Art-Registries muessen als comdare_axis_registry lesbar sein";
    EXPECT_EQ(trio->organ_axis_count(), 17u) << "Organ-golden: 17 Kompositions-Achsen (isa raus, INC-2d)";
    EXPECT_EQ(trio->system_axis_count(), 5u);
    EXPECT_EQ(trio->measurement_category_count(), 16u);
    EXPECT_EQ(trio->organ.axis_names.count("search_algo"), 1u) << "search_algo ist eine Organ-Achse";
    EXPECT_EQ(trio->system.axis_names.count("target_isa"), 1u) << "target_isa ist eine System-Achse (INC-2d)";
    EXPECT_EQ(trio->organ.axis_names.count("target_isa"), 0u) << "target_isa ist KEINE Organ-Achse";
}

// (P1) NEGATIV V-UNREG-AXIS: eine in KEINER Angebots-Registry registrierte Achse -> harter Reject mit Koordinate.
TEST(ResolveAxisRefsAgainstTrio, UnregisteredAxisIsVUnregAxisWithCoordinate) {
    auto const trio = real_trio();
    ASSERT_TRUE(trio.has_value());

    tlz::ResolverReport const rep = tlz::resolve_axis_refs_against_trio({"total_bogus_axis"}, *trio);
    EXPECT_TRUE(rep.resolved) << "der Resolver LIEF (gegen ein Trio aufgeloest)";
    EXPECT_FALSE(rep.ok);
    ASSERT_EQ(rep.rejects.size(), 1u);
    EXPECT_EQ(rep.rejects[0].code, "V-UNREG-AXIS");
    EXPECT_EQ(rep.rejects[0].ref, "total_bogus_axis") << "die Ref-Koordinate steht im Reject";
    // Deterministische Meldung MIT Koordinate + stabilem Fehlerklassen-Etikett (axis_error error_class_label).
    EXPECT_NE(rep.rejects[0].message.find("V-UNREG-AXIS"), std::string::npos);
    EXPECT_NE(rep.rejects[0].message.find("total_bogus_axis"), std::string::npos) << "Koordinate in der Meldung";
    EXPECT_NE(rep.rejects[0].message.find("konfig_xml_parse"), std::string::npos) << "Fehlerklassen-Etikett (D1)";
}

// (P2) NEGATIV V-CATEGORY: eine registrierte SYSTEM-Achse (target_isa) in Organ-Position -> Route-Meldung mit
//      Koordinate (gehoert ueber build_system_axis_levels, nicht den 17-Slot-Organ-Pfad).
TEST(ResolveAxisRefsAgainstTrio, MisplacedSystemAxisIsVCategoryWithCoordinate) {
    auto const trio = real_trio();
    ASSERT_TRUE(trio.has_value());

    tlz::ResolverReport const rep = tlz::resolve_axis_refs_against_trio({"target_isa"}, *trio);
    EXPECT_FALSE(rep.ok);
    ASSERT_EQ(rep.rejects.size(), 1u);
    EXPECT_EQ(rep.rejects[0].code, "V-CATEGORY");
    EXPECT_EQ(rep.rejects[0].ref, "target_isa");
    EXPECT_NE(rep.rejects[0].message.find("V-CATEGORY"), std::string::npos);
    EXPECT_NE(rep.rejects[0].message.find("target_isa"), std::string::npos) << "Koordinate in der Meldung";
    EXPECT_NE(rep.rejects[0].message.find("build_system_axis_levels"), std::string::npos) << "Routing-Ziel benannt";
    EXPECT_NE(rep.rejects[0].message.find("cache_engine_system"), std::string::npos) << "Angebots-Registry benannt";
}

// (P3) POSITIV: ein organ-reines Profil (nur Organ-Achsen + Organ-Sonderzweig-Unter-Achsen) -> 0 Rejects, ok=true.
//      Belegt die Golden-Neutralitaet (die golden/m3v2-Profile sind organ-rein => keine Reject-Regression).
TEST(ResolveAxisRefsAgainstTrio, OrganPureRefsYieldZeroRejects) {
    auto const trio = real_trio();
    ASSERT_TRUE(trio.has_value());

    // Organ-Achsen + die 3 compile-time Organ-SUB-Achsen (cacheline/node_width/alloc_hw sind KEINE eigene
    // Registry-<axis>, werden aber wie in build_axis_levels separat behandelt => KEIN Reject).
    std::vector<std::string> const organ_pure = {"search_algo", "cache_traversal", "mapping",
                                                 "cacheline",   "node_width",      "alloc_hw"};
    tlz::ResolverReport const      rep        = tlz::resolve_axis_refs_against_trio(organ_pure, *trio);
    for (auto const& rj : rep.rejects) ADD_FAILURE() << "[resolver] " << rj.message;
    EXPECT_TRUE(rep.resolved);
    EXPECT_TRUE(rep.ok);
    EXPECT_TRUE(rep.rejects.empty());
}

// (P4) organ_position_refs zieht die Refs aus dem geparsten Thesis-Profil (permute_axes.ref) -- die Ref-Quelle
//      des Resolvers. Das Minimal-Fixture traegt genau search_algo (organ-rein) => Resolver 0 Rejects end-to-end.
TEST(ResolveAxisRefsAgainstTrio, ThesisPermuteAxesRefsResolveClean) {
    auto const trio = real_trio();
    ASSERT_TRUE(trio.has_value());
    auto const tp = parse_temp(""); // permute_axes = {search_algo}
    ASSERT_TRUE(tp.has_value());

    std::vector<std::string> const refs = tlz::organ_position_refs(*tp);
    ASSERT_EQ(refs.size(), 1u);
    EXPECT_EQ(refs[0], "search_algo");
    tlz::ResolverReport const rep = tlz::resolve_axis_refs_against_trio(refs, *trio);
    EXPECT_TRUE(rep.ok);
    EXPECT_TRUE(rep.rejects.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// A9.1 (S4-Delta B9 Mess-Schema-Kern, 2026-07-20): die drei PASSIVEN Mess-UNTER-Achsen im comdare_thesis_profile-
// Kanal (run_methodology/measurement_framework/writeback_methods). Struct-Injektion (die Parser-Naht ist
// end-to-end in test_experiment_parser bewiesen); hier die Validat-Bloecke gegen die 3 constexpr-Registries +
// die Report-Zeilen + das SKIP-Gate. binary_id-neutral (Planer-delegiert); der Fan-out/Vollzug gehoert S5.
// ─────────────────────────────────────────────────────────────────────────────

// (a9a) gueltige A9.1-Werte (exactly-one Modus, §61-STUFEN/j1) werden AKZEPTIERT; die Zaehler + Report-Zeilen stimmen.
TEST(ValidateProfileMeasurementSubAxes, ValidA9SubAxesAccepted) {
    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());

    auto tp = parse_temp("");
    ASSERT_TRUE(tp.has_value());
    tp->run_methodology       = {"measure"}; // §61-STUFEN/(j1): GENAU EIN aktiver Modus je Profil
    tp->measurement_framework = "ycsb";
    tp->writeback_methods     = {"csv", "latex_table", "comparison_metrics"};

    tlz::ProfileValidationResult const vr = tlz::validate_profile(*tp, registry);
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok);
    EXPECT_EQ(vr.run_methodology_checked, 1u);
    EXPECT_EQ(vr.measurement_framework_checked, 1u);
    EXPECT_EQ(vr.writeback_methods_checked, 3u);
    std::ostringstream os;
    tlz::print_validation_report(vr, *tp, os);
    EXPECT_NE(os.str().find("1 run_methodology"), std::string::npos) << os.str();
    EXPECT_NE(os.str().find("1 measurement_framework"), std::string::npos) << os.str();
    EXPECT_NE(os.str().find("3 writeback_methods"), std::string::npos) << os.str();
}

// (a9a-exactly-one) §61-STUFEN/(j1): MEHR als ein run_methodology-Modus ist ein HARTER Fehler (exactly-one je
//     Profil/Call; die A9-"sweepbar"-Auslegung ist supersediert). Genau EIN Modus (measure/debug/release) = gueltig.
TEST(ValidateProfileMeasurementSubAxes, RunMethodologyMustBeExactlyOne) {
    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());
    auto                   tp       = parse_temp("");
    ASSERT_TRUE(tp.has_value());
    tp->measurement_framework = "ycsb";
    tp->writeback_methods     = {"csv"};

    // >1 Modus => harter Fehler (exactly-one).
    tp->run_methodology                      = {"debug", "measure"};
    tlz::ProfileValidationResult const multi = tlz::validate_profile(*tp, registry);
    EXPECT_FALSE(multi.ok) << "zwei Modi muessen abgelehnt werden (exactly-one)";
    bool found = false;
    for (auto const& e : multi.errors)
        if (e.find("GENAU EINE") != std::string::npos) found = true;
    EXPECT_TRUE(found) << "exactly-one-Fehlermeldung erwartet";

    // Genau EIN Modus (debug) => gueltig.
    tp->run_methodology                    = {"debug"};
    tlz::ProfileValidationResult const one = tlz::validate_profile(*tp, registry);
    EXPECT_TRUE(one.ok) << "ein Modus (debug) ist gueltig";
    EXPECT_EQ(one.run_methodology_checked, 1u);
}

// (a9b) ein Bogus run_methodology-Wert (profiling) ist ein HARTER Fehler ({debug,measure,release}).
TEST(ValidateProfileMeasurementSubAxes, BogusRunMethodologyRejected) {
    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());

    auto tp = parse_temp("");
    ASSERT_TRUE(tp.has_value());
    tp->run_methodology = {"measure", "profiling"}; // ausserhalb {debug,measure,release}

    tlz::ProfileValidationResult const vr = tlz::validate_profile(*tp, registry);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "run_methodology"));
    EXPECT_TRUE(any_contains(vr.errors, "profiling"));
    EXPECT_EQ(vr.run_methodology_checked, 2u);
}

// (a9c) ein Bogus measurement_framework-Wert (tpcc) ist ein HARTER Fehler ({ycsb}, honest-1).
TEST(ValidateProfileMeasurementSubAxes, BogusMeasurementFrameworkRejected) {
    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());

    auto tp = parse_temp("");
    ASSERT_TRUE(tp.has_value());
    tp->measurement_framework = "tpcc"; // ausserhalb {ycsb}

    tlz::ProfileValidationResult const vr = tlz::validate_profile(*tp, registry);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "measurement_framework"));
    EXPECT_TRUE(any_contains(vr.errors, "tpcc"));
    EXPECT_EQ(vr.measurement_framework_checked, 1u);
}

// (a9d) ein Bogus writeback_methods-Wert (pdf) ist ein HARTER Fehler -- pdf ist honest-0 KEIN Rueckschrieb-Kanal.
TEST(ValidateProfileMeasurementSubAxes, BogusWritebackMethodRejected) {
    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());

    auto tp = parse_temp("");
    ASSERT_TRUE(tp.has_value());
    tp->writeback_methods = {"csv", "pdf"}; // pdf ausserhalb {csv,latex_table,comparison_metrics}

    tlz::ProfileValidationResult const vr = tlz::validate_profile(*tp, registry);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(any_contains(vr.errors, "writeback_methods"));
    EXPECT_TRUE(any_contains(vr.errors, "pdf"));
    EXPECT_EQ(vr.writeback_methods_checked, 2u);
}

// (a9e) SKIP-Gate: leere A9.1-Achsen = nichts geprueft (Zaehler 0), ok=true, KEINE Report-Zeilen (byte-identisch
//       zum heutigen Verhalten -- bestehende Profile ohne diese Elemente validieren unveraendert).
TEST(ValidateProfileMeasurementSubAxes, EmptyA9SubAxesSkipAndByteIdentical) {
    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());

    auto tp = parse_temp(""); // keine A9.1-Elemente -> alle Felder leer
    ASSERT_TRUE(tp.has_value());

    tlz::ProfileValidationResult const vr = tlz::validate_profile(*tp, registry);
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok);
    EXPECT_EQ(vr.run_methodology_checked, 0u);
    EXPECT_EQ(vr.measurement_framework_checked, 0u);
    EXPECT_EQ(vr.writeback_methods_checked, 0u);
    std::ostringstream os;
    tlz::print_validation_report(vr, *tp, os);
    EXPECT_EQ(os.str().find("run_methodology"), std::string::npos) << os.str();
    EXPECT_EQ(os.str().find("measurement_framework"), std::string::npos) << os.str();
    EXPECT_EQ(os.str().find("writeback_methods"), std::string::npos) << os.str();
}

// S2-NACHT-3 (2026-07-23) Facade-Basename-Haertung: ein COMDARE_PLAN_METHODIK_PROFILE-BARE-BASENAME wird gegen
// thesis_profiles/ (das Verzeichnis des Haupt-Profils) aufgeloest und ist dann ladbar; ein Pfad-behafteter Wert
// (absolut oder mit Verzeichnisanteil) bleibt UNVERAENDERT gueltig. So kann die super-YAML den KLASSEN-konformen
// Basename setzen (der zugleich am ceb:trigger weitergereicht wird), waehrend die Facade ihn zur --dump-ci-
// Emissionszeit laedt (resolve_methodik_override nutzt exakt diesen Helfer + tlz::load_thesis_profile).
TEST(FacadeMethodikBasename, BareBasenameResolvesUnderThesisProfilesAndLoads) {
    namespace pf        = comdare::cache_engine::builder::profile_facade;
    fs::path const main = thesis_profiles_dir() / "all_axes_golden.profile.xml";

    // (a) BARE-BASENAME => gegen thesis_profiles/ aufgeloest, und der aufgeloeste Pfad LAEDT m3_smoke_coverage.
    fs::path const resolved = pf::resolve_methodik_profile_path("m3_smoke_coverage.profile.xml", main);
    EXPECT_EQ(resolved, thesis_profiles_dir() / "m3_smoke_coverage.profile.xml");
    EXPECT_TRUE(tlz::load_thesis_profile(resolved).has_value())
        << "aufgeloester Basename laedt m3_smoke_coverage: " << resolved.string();

    // (Negativ) nicht-existenter Basename => aufgeloest, aber load_thesis_profile scheitert HART (nullopt).
    fs::path const missing = pf::resolve_methodik_profile_path("does_not_exist_ce_test.profile.xml", main);
    EXPECT_EQ(missing, thesis_profiles_dir() / "does_not_exist_ce_test.profile.xml");
    EXPECT_FALSE(tlz::load_thesis_profile(missing).has_value())
        << "nicht-existenter Basename => harter Fehler (nullopt)";

    // Pfad-behafteter Wert bleibt UNVERAENDERT: absoluter Pfad ...
    EXPECT_EQ(pf::resolve_methodik_profile_path(main, main), main) << "absoluter Pfad unangetastet";
    // ... und ein relativer Pfad MIT Verzeichnisanteil.
    fs::path const with_dir{"sub/x.profile.xml"};
    EXPECT_EQ(pf::resolve_methodik_profile_path(with_dir, main), with_dir) << "Pfad mit Verzeichnisanteil unangetastet";
    // Leeres Haupt-Profil (degenerate) => Basename unveraendert (keine Aufloesung moeglich).
    EXPECT_EQ(pf::resolve_methodik_profile_path("m3_smoke_coverage.profile.xml", fs::path{}),
              fs::path{"m3_smoke_coverage.profile.xml"});
}
