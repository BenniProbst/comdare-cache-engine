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

#include "profile_runner.hpp"   // load_thesis_profile
#include "validate_profile.hpp" // validate_profile / axis_registry_from_levels / print_validation_report

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
