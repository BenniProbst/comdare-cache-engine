// test_measurement_categories — INC-3 Familie A (2026-07-14): ctest-Gate fuer die ADDITIVE E4-Profil-Schema-
// Erweiterung <measurement_categories>/<category name=..> (die dritte W/D/K-Dimension K, Verortungs-Hinweis K
// aus GO-5 Fork 1 nun GEBAUT). Muster: test_wdk_datasets_fairness.cpp (Fork 1 <datasets>).
//
// BEWEIST LITERAL:
//   (a) GUELTIGE KATEGORIEN — das committete Beispiel-Profil wdk_fairness_example.profile.xml traegt den
//       <measurement_categories>-Block; parse_thesis_profile liefert die 4 Namen; validate_profile bestaetigt
//       sie gegen die Single-Source kMeasurementAxisRegistry (categories_checked==4, ok).
//   (b) UNGUELTIGE KATEGORIE — ein getippter Name (LATENCY_P90) ist ein HARTER Validierungs-Fehler.
//   (c) OHNE ELEMENT — Profile ohne <measurement_categories> parsen zu LEEREM Feld + validieren ok; die
//       --validate-Zusammenfassung bleibt byte-identisch (kein "measurement_categories" im Report) →
//       rueckwaerts-kompatibel (Default-Verhaltens-Gate dieses Increments).
//   (d) DUPLIKAT — eine mehrfach genannte Kategorie ist eine WARNUNG (redundante Spalte), NICHT fatal.
//   (e) GOLDEN-NEUTRALITAET — die gepinnte Basis des Beispiel-Profils (MIT measurement_categories) erzeugt
//       weiterhin genau 1 Zelle mit der golden-320-Baseline-binary_id: Kategorien sind eine Spalten-
//       PROJEKTION, KEINE Achse → binary_id-neutral.
//
// TABU-Wache: m3v2_study.profile.xml + golden_fullpilot_320_binary_ids.txt werden NUR GELESEN.

#include "profile_runner.hpp"   // load_thesis_profile / build_profile_basis_levels
#include "validate_profile.hpp" // validate_profile / axis_registry_from_levels / print_validation_report

#include <builder/experiment_tree/registry_to_axis_levels.hpp> // build_all_axis_levels (EnabledStrategies)

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#ifndef COMDARE_THESIS_PROFILES_DIR
#error "COMDARE_THESIS_PROFILES_DIR must point to libs/cache_engine/algorithm_profiles/thesis_profiles"
#endif
#ifndef COMDARE_GOLDEN_320_IDS
#error "COMDARE_GOLDEN_320_IDS must point to tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt"
#endif

namespace {

namespace cx  = comdare::builder::xml;
namespace ex  = comdare::cache_engine::builder::experiment;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace fs  = std::filesystem;

fs::path example_profile_path() { return fs::path{COMDARE_THESIS_PROFILES_DIR} / "wdk_fairness_example.profile.xml"; }

std::string first_golden_id() {
    std::ifstream f{fs::path{COMDARE_GOLDEN_320_IDS}};
    std::string   line;
    while (std::getline(f, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (!line.empty() && line[0] != '#') return line;
    }
    return {};
}

// Temp-Fixture (Muster test_wdk_datasets_fairness::write_temp_profile): Minimal-Profil mit frei
// waehlbarem <measurement_categories>-Block fuer die Fehler-/Default-Faelle.
fs::path write_temp_profile(std::string const& categories_block) {
    fs::path const p = fs::temp_directory_path() /
                       ("comdare_meascat_fixture_" +
                        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".xml");
    std::ofstream  out{p};
    out << R"(<?xml version="1.0" encoding="UTF-8"?>
<comdare_thesis_profile id="meascat_fixture" schema_version="1">
  <base_tiers><tier id="hot" profile_ref="../sota/hot.profile.xml" paper_ref="P02"/></base_tiers>
  <permute_axes><axis ref="search_algo"><value>k_ary</value></axis></permute_axes>
)" << categories_block
        << R"(  <modes><mode name="ce_only" merge="Stufe1_CeOnly" active_axes="search_algo"/></modes>
  <static_axes from="base_tier"/>
</comdare_thesis_profile>
)";
    return p;
}

std::optional<cx::ThesisProfile> parse_temp(std::string const& categories_block) {
    fs::path const                   p = write_temp_profile(categories_block);
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

} // namespace

// (a) GUELTIGE KATEGORIEN — das committete Beispiel-Profil traegt den <measurement_categories>-Block.
TEST(MeasurementCategories, ParserAndValidateOnExampleProfile) {
    auto const tp = tlz::load_thesis_profile(example_profile_path());
    ASSERT_TRUE(tp.has_value()) << "Beispiel-Profil nicht lesbar: " << example_profile_path().string();

    ASSERT_EQ(tp->measurement_categories.size(), 4u);
    EXPECT_EQ(tp->measurement_categories[0], "CLU");
    EXPECT_EQ(tp->measurement_categories[1], "CACHE_MISS_L3");
    EXPECT_EQ(tp->measurement_categories[2], "LATENCY_P99");
    EXPECT_EQ(tp->measurement_categories[3], "THROUGHPUT");

    ex::AxisRegistry const             registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());
    tlz::ProfileValidationResult const vr       = tlz::validate_profile(*tp, registry);
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok);
    EXPECT_EQ(vr.categories_checked, 4u);

    std::ostringstream os;
    tlz::print_validation_report(vr, *tp, os);
    EXPECT_NE(os.str().find("4 measurement_categories"), std::string::npos) << os.str();
}

// (b) UNGUELTIGE KATEGORIE — ein getippter Name ist ein HARTER Fehler (Single-Source kMeasurementAxisRegistry).
TEST(MeasurementCategories, InvalidCategoryNameIsError) {
    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());
    auto const             bad =
        parse_temp("  <measurement_categories><category name=\"LATENCY_P90\"/></measurement_categories>\n");
    ASSERT_TRUE(bad.has_value());
    tlz::ProfileValidationResult const vr = tlz::validate_profile(*bad, registry);
    EXPECT_FALSE(vr.ok);
    EXPECT_EQ(vr.categories_checked, 1u);
    EXPECT_TRUE(any_contains(vr.errors, "UNGUELTIGE Mess-Kategorie"));
}

// (c) OHNE ELEMENT — leeres Feld + ok; die --validate-Zusammenfassung bleibt byte-identisch (kein Report-Wort).
TEST(MeasurementCategories, WithoutElementIsEmptyAndReportUnchanged) {
    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());
    auto const             bare     = parse_temp("");
    ASSERT_TRUE(bare.has_value());
    EXPECT_TRUE(bare->measurement_categories.empty());

    tlz::ProfileValidationResult const vr = tlz::validate_profile(*bare, registry);
    EXPECT_TRUE(vr.ok);
    EXPECT_EQ(vr.categories_checked, 0u);
    std::ostringstream os;
    tlz::print_validation_report(vr, *bare, os);
    EXPECT_EQ(os.str().find("measurement_categories"), std::string::npos)
        << "Report bestehender Profile darf sich nicht aendern:\n"
        << os.str();
}

// (d) DUPLIKAT — eine mehrfach genannte Kategorie ist eine WARNUNG (redundante Spalte), NICHT fatal.
TEST(MeasurementCategories, DuplicateCategoryIsWarningNotError) {
    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());
    auto const             dup      = parse_temp("  <measurement_categories><category name=\"CLU\"/>"
                                                 "<category name=\"CLU\"/></measurement_categories>\n");
    ASSERT_TRUE(dup.has_value());
    tlz::ProfileValidationResult const vr = tlz::validate_profile(*dup, registry);
    EXPECT_TRUE(vr.ok) << "Duplikat ist WARNUNG, kein Fehler";
    EXPECT_EQ(vr.categories_checked, 2u);
    EXPECT_TRUE(any_contains(vr.warnings, "mehrfach deklariert"));
}

// (e) GOLDEN-NEUTRALITAET — die gepinnte Basis (MIT measurement_categories) erzeugt EXAKT die golden-320-
// Baseline-binary_id: Kategorien sind eine Spalten-Projektion, KEINE Achse.
TEST(MeasurementCategories, CategoriesAreBinaryIdNeutralAgainstGolden) {
    std::string const golden0 = first_golden_id();
    ASSERT_FALSE(golden0.empty()) << "golden-Fixture nicht lesbar (TABU-Wache)";

    auto const tp = tlz::load_thesis_profile(example_profile_path());
    ASSERT_TRUE(tp.has_value());
    std::vector<ex::AxisLevel> const basis =
        tlz::build_profile_basis_levels(*tp, "wdk_example_base", /*with_dynamic=*/false);
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(basis);
    ex::StaticBinaryView const view = tree.static_binary_view();
    ASSERT_EQ(view.size(), 1u) << "gepinnte Basis muss genau 1 Zelle ergeben";
    EXPECT_EQ(view[0].binary_id, golden0) << "measurement_categories duerfen die binary_id NIE beeinflussen";
}
