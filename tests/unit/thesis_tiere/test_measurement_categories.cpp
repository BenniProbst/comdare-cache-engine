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
//   (f) TEILMENGEN-GARANTIE / Koeder (Paket #11, 2026-08-09) -- ein bei JEDEM Lauf NEU GEWUERFELTER Name
//       ausserhalb des Angebots wird hart abgelehnt; die Vorbedingung "liegt wirklich ausserhalb" ist
//       selbst ein ASSERT. Der Bericht nennt dabei BEIDE Zahlen (Auswahl von Grundgesamtheit).
//   (g) TEILMENGEN-GARANTIE / Gegeneingang (Paket #11) -- die VOLLE Auswahl (alle Registry-Namen) geht
//       glatt durch, Zaehler == Nenner. (f) ohne (g) waere wertlos: eine Wache, die alles ablehnt,
//       bestuende (f) ebenfalls und ist von der richtigen nicht zu unterscheiden.
//
// TABU-Wache: m3v2_study.profile.xml + golden_fullpilot_320_binary_ids.txt werden NUR GELESEN.

#include "profile_runner.hpp"   // load_thesis_profile / build_profile_basis_levels
#include "validate_profile.hpp" // validate_profile / axis_registry_from_levels / print_validation_report

#include <builder/experiment_tree/registry_to_axis_levels.hpp> // build_all_axis_levels (EnabledStrategies)

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#ifndef COMDARE_THESIS_PROFILES_DIR
#error "COMDARE_THESIS_PROFILES_DIR must point to libs/cache_engine/algorithm_profiles/thesis_profiles"
#endif
#ifndef COMDARE_GOLDEN_320_BYTE_GUARD_IDS
#error "COMDARE_GOLDEN_320_BYTE_GUARD_IDS must point to tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt"
#endif

namespace {

namespace cx  = comdare::builder::xml;
namespace ex  = comdare::cache_engine::builder::experiment;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace fs  = std::filesystem;
namespace ms  = comdare::cache_engine::measurement; // Paket #11: kMeasurementAxisRegistry = die Grundgesamtheit

// -- Paket #11 (2026-08-09): die TEILMENGEN-GARANTIE. Die Erwartungen unten leiten die Grundgesamtheit AUS
//    kMeasurementAxisRegistry ab statt "16" zu tippen -- waechst die Registry auf 17, wandert das Gate mit,
//    statt eine tote Zahl zu behaupten. --
std::vector<std::string> offered_categories() {
    std::vector<std::string> v;
    for (auto const& info : ms::kMeasurementAxisRegistry) v.emplace_back(info.name);
    return v;
}

bool is_offered(std::string const& name) {
    for (auto const& info : ms::kMeasurementAxisRegistry)
        if (info.name == name) return true;
    return false;
}

std::string categories_block_of(std::vector<std::string> const& names) {
    std::string block = "  <measurement_categories>";
    for (auto const& n : names) block += "<category name=\"" + n + "\"/>";
    block += "</measurement_categories>\n";
    return block;
}

// FRISCH GEWUERFELT (K13): der Koeder entsteht bei JEDEM Lauf neu. Ein fest verdrahteter Koeder verliert
// seine Beweiskraft in dem Moment, in dem er versehentlich ins Angebot wandert -- dann prueft der Test still
// den Gutfall weiter und sieht aus wie vorher. Die Schleife wuerfelt daher so lange, bis der Koeder
// NACHWEISLICH AUSSERHALB des Angebots liegt; der Test macht diese Vorbedingung anschliessend hart.
std::string fresh_koeder() {
    std::mt19937_64 rng{std::random_device{}() ^
                        static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count())};
    std::uniform_int_distribution<int> letter{0, 25};
    for (int attempt = 0; attempt < 64; ++attempt) {
        std::string name = "KOEDER_";
        for (int i = 0; i < 12; ++i) name += static_cast<char>('A' + letter(rng));
        if (!is_offered(name)) return name;
    }
    return {};
}

fs::path example_profile_path() { return fs::path{COMDARE_THESIS_PROFILES_DIR} / "wdk_fairness_example.profile.xml"; }

std::string first_golden_id() {
    std::ifstream f{fs::path{COMDARE_GOLDEN_320_BYTE_GUARD_IDS}};
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
    // Paket #11: der NENNER wird mitgefuehrt -- und zwar der, den die Wache wirklich befragt hat.
    EXPECT_EQ(vr.categories_offered, ms::kMeasurementAxisRegistry.size());

    std::ostringstream os;
    tlz::print_validation_report(vr, *tp, os);
    // BEIDE Zahlen stehen in der Ausgabe: Auswahl "von" Grundgesamtheit. "4 measurement_categories" allein
    // liesse offen, wogegen geprueft wurde -- und waere von einem falschen Nenner nicht zu unterscheiden.
    std::string const expect_both =
        "4 von " + std::to_string(ms::kMeasurementAxisRegistry.size()) + " measurement_categories";
    EXPECT_NE(os.str().find(expect_both), std::string::npos) << os.str();
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

// -----------------------------------------------------------------------------
// Paket #11 (2026-08-09) -- DIE TEILMENGEN-GARANTIE ALS WACHE, mit ihrem Gegeneingang.
//
// (f) und (g) gehoeren ZUSAMMEN und duerfen nie getrennt gelesen werden. (f) allein wuerde auch von einer
// Wache bestanden, die AUSNAHMSLOS alles ablehnt -- und die blinde Wache sieht von aussen genauso aus wie
// die richtige. Erst (g) zeigt, dass die Wache UNTERSCHEIDET: die vollstaendige, legitime Auswahl muss
// glatt durchgehen. Zusammen belegen sie, dass "Teilmenge des Angebots" gemessen und nicht behauptet wird.
// -----------------------------------------------------------------------------

// (f) GEGENEINGANG "Auswahl nennt etwas Unbekanntes" -- der FRISCH GEWUERFELTE Koeder MUSS BEISSEN.
TEST(MeasurementCategories, FreshKoederIsRejectedAndDenominatorIsReported) {
    std::string const koeder = fresh_koeder();
    ASSERT_FALSE(koeder.empty()) << "kein Koeder ausserhalb des Angebots wuerfelbar";
    // VORBEDINGUNG, hart: ein Koeder INNERHALB des Angebots ist kein Koeder -- der Test pruefte dann still
    // den Gutfall und meldete trotzdem Erfolg. Genau diese Vertauschung soll hier auffallen.
    ASSERT_FALSE(is_offered(koeder)) << "Koeder liegt IM Angebot, ist also keiner: " << koeder;

    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());
    auto const             bad      = parse_temp(categories_block_of({koeder}));
    ASSERT_TRUE(bad.has_value());
    ASSERT_EQ(bad->measurement_categories.size(), 1u) << "der Koeder muss den Parser ueberhaupt erreichen";

    tlz::ProfileValidationResult const vr = tlz::validate_profile(*bad, registry);
    EXPECT_FALSE(vr.ok) << "Koeder \"" << koeder << "\" blieb unbemerkt -- die Wache deckt nichts";
    EXPECT_TRUE(any_contains(vr.errors, "UNGUELTIGE Mess-Kategorie")) << "Koeder: " << koeder;
    EXPECT_EQ(vr.categories_checked, 1u);
    // BEIDE Zahlen: auch im Fehlerfall muss der Nenner stehen, sonst bleibt unklar, wogegen geprueft wurde.
    EXPECT_EQ(vr.categories_offered, ms::kMeasurementAxisRegistry.size());
}

// (g) GEGENEINGANG "Auswahl = Gesamtmenge" -- die volle Auswahl geht glatt durch, Zaehler == Nenner.
// Damit ist zugleich der NENNER selbst gegengeprueft: das gemeldete Angebot ist nicht nur eine Zahl, jedes
// seiner Elemente ist auch wirklich waehlbar. Ein Nenner, der Namen mitzaehlt, die die Wache dann ablehnt,
// faellt hier auf (und keine der Zahlen allein wuerde ihn verraten).
TEST(MeasurementCategories, FullSelectionEqualsOfferAndPasses) {
    std::vector<std::string> const all = offered_categories();
    ASSERT_EQ(all.size(), ms::kMeasurementAxisRegistry.size());

    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());
    auto const             full     = parse_temp(categories_block_of(all));
    ASSERT_TRUE(full.has_value());
    ASSERT_EQ(full->measurement_categories.size(), all.size());

    tlz::ProfileValidationResult const vr = tlz::validate_profile(*full, registry);
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok) << "die Gesamtmenge IST eine Teilmenge ihrer selbst";
    EXPECT_EQ(vr.categories_checked, all.size());
    EXPECT_EQ(vr.categories_offered, all.size());
    // Kein Duplikat-Hinweis: die Registry-Namen sind paarweise verschieden (registry_names_are_unique()).
    EXPECT_FALSE(any_contains(vr.warnings, "mehrfach deklariert"));

    std::ostringstream os;
    tlz::print_validation_report(vr, *full, os);
    std::string const expect_both =
        std::to_string(all.size()) + " von " + std::to_string(all.size()) + " measurement_categories";
    EXPECT_NE(os.str().find(expect_both), std::string::npos) << os.str();
}
