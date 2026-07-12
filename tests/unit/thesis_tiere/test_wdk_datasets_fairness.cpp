// test_wdk_datasets_fairness — GO-5 Forks 1/2/6 (2026-07-12, Dossier 20260712-go5 A.1/A.2/A.6):
// ctest-Gate fuer die ADDITIVEN E4-Profil-Schema-Erweiterungen <datasets> (Fork 1, Mess-Input D;
// Single-Source = die test_data-AKTEN, Fork 2/R2) + <sota_series fairness=..> (Fork 6, Thesis
// §sec:fairness common_denominator|native).
//
// BEWEIST LITERAL:
//   (1) PARSE-ROUNDTRIP: das Beispiel-Profil wdk_fairness_example.profile.xml liefert die 3
//       <dataset>-Eintraege (id/akte_ref/loader) + die fairness-Attribute beider Modi.
//   (2) DEFAULT BYTE-IDENTISCH: Profile OHNE die neuen Tags (m3v2_study TABU/NUR GELESEN + ein
//       synthetisches Minimal-Profil) parsen zu LEEREN Feldern; die --validate-Zusammenfassung
//       bestehender Profile traegt KEINE datasets-Erwaehnung; der golden-Roundtrip ist unberuehrt
//       (Beispiel-Profil-Basis == golden[0] — die neuen Tags sind binary_id-NEUTRAL).
//   (3) VALIDATE-FEHLERFAELLE: ungueltiger fairness-Wert / akte_ref ohne .test_data.xml-Suffix /
//       fehlende+doppelte dataset-id / fehlender loader ⇒ FEHLER; fremder loader + fairness-
//       Varianten desselben (lebewesen,merge)-Paars ⇒ WARNUNG (nicht fatal, dokumentiert ehrlich).
//   (4) KONSUM-ANKUNFT (kein totes Feld): fairness kommt in der Runner-Spec an
//       (build_sota_passes → SotaPass.fairness_mode) und wird exportiert (lazy_csv_header-Endspalte
//       fairness_mode + format_csv_row); datasets kommen als deterministische Signatur
//       (profile_datasets_signature) im Resume-Stamp an (resume-v5) — verschiedene fairness-/
//       datasets-Deklarationen ergeben VERSCHIEDENE Stamps (kein stales Cross-Resume).
//       Der Loader-MESS-Konsum der datasets ist lauf-gated (dokumentiert; Loader-Slot #184).
//
// TABU-Wache: m3v2_study.profile.xml + golden_fullpilot_320_binary_ids.txt werden NUR GELESEN.

#include "profile_runner.hpp"   // load_thesis_profile / profile_datasets_signature / build_profile_basis_levels
#include "sota_catalog.hpp"     // build_sota_passes (Runner-Spec: SotaPass.fairness_mode)
#include "validate_profile.hpp" // validate_profile / axis_registry_from_levels / print_validation_report

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // LazyRunConfig / lazy_resume_stamp_prefix /
                                                                     // lazy_csv_header / format_csv_row
#include <builder/experiment_tree/registry_to_axis_levels.hpp>       // build_all_axis_levels (EnabledStrategies)

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
fs::path m3v2_profile_path() { return fs::path{COMDARE_THESIS_PROFILES_DIR} / "m3v2_study.profile.xml"; }

std::string first_golden_id() {
    std::ifstream f{fs::path{COMDARE_GOLDEN_320_IDS}};
    std::string   line;
    while (std::getline(f, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (!line.empty() && line[0] != '#') return line;
    }
    return {};
}

// Temp-Fixture (Muster test_e4_contract_xml_to_axislevels::write_negative_fixture): ein Minimal-Profil
// mit frei waehlbarem <datasets>-/<sota_series>-Block fuer die validate-Fehlerfaelle.
fs::path write_temp_profile(std::string const& datasets_block, std::string const& series_block) {
    fs::path const p = fs::temp_directory_path() /
                       ("comdare_wdk_fairness_fixture_" +
                        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".xml");
    std::ofstream  out{p};
    out << R"(<?xml version="1.0" encoding="UTF-8"?>
<comdare_thesis_profile id="wdk_fixture" schema_version="1">
  <base_tiers><tier id="hot" profile_ref="../sota/hot.profile.xml" paper_ref="P02"/></base_tiers>
  <permute_axes><axis ref="search_algo"><value>k_ary</value></axis></permute_axes>
)" << datasets_block
        << series_block << R"(  <modes><mode name="ce_only" merge="Stufe1_CeOnly" active_axes="search_algo"/></modes>
  <static_axes from="base_tier"/>
</comdare_thesis_profile>
)";
    return p;
}

std::optional<cx::ThesisProfile> parse_temp(std::string const& datasets_block, std::string const& series_block) {
    fs::path const                   p = write_temp_profile(datasets_block, series_block);
    cx::XmlConfigParser const        parser;
    std::optional<cx::ThesisProfile> tp = parser.parse_thesis_profile(p);
    std::error_code                  ec;
    fs::remove(p, ec);
    return tp;
}

std::size_t count_char(std::string const& s, char c) {
    return static_cast<std::size_t>(std::count(s.begin(), s.end(), c));
}

bool any_contains(std::vector<std::string> const& msgs, std::string const& needle) {
    for (auto const& m : msgs)
        if (m.find(needle) != std::string::npos) return true;
    return false;
}

} // namespace

// (1) PARSE-ROUNDTRIP — das committete Beispiel-Profil traegt beide neuen Konstrukte vollstaendig.
TEST(WdkDatasetsFairness, ParserRoundtripOnExampleProfile) {
    auto const tp = tlz::load_thesis_profile(example_profile_path());
    ASSERT_TRUE(tp.has_value()) << "Beispiel-Profil nicht lesbar: " << example_profile_path().string();
    EXPECT_EQ(tp->id, "wdk_fairness_example");

    ASSERT_EQ(tp->datasets.size(), 3u);
    EXPECT_EQ(tp->datasets[0].id, "url");
    EXPECT_EQ(tp->datasets[0].akte_ref, "test_data_xml/url.test_data.xml");
    EXPECT_EQ(tp->datasets[0].loader, "string_corpus");
    EXPECT_EQ(tp->datasets[2].id, "sosd_books_200M");
    EXPECT_EQ(tp->datasets[2].loader, "sosd_uint64");

    ASSERT_EQ(tp->sota_series.size(), 3u);
    EXPECT_EQ(tp->sota_series[0].fairness, "") << "prt_art-Reihe ist bewusst UNGESETZT (Default)";
    EXPECT_EQ(tp->sota_series[1].fairness, "common_denominator");
    EXPECT_EQ(tp->sota_series[2].fairness, "native");

    // Die deterministische <datasets>-Signatur (Dokument-Reihenfolge) — Quelle des Resume-Stamp-Konsums.
    EXPECT_EQ(tlz::profile_datasets_signature(*tp),
              "url:test_data_xml/url.test_data.xml:string_corpus;"
              "english_words:test_data_xml/english_words.test_data.xml:string_corpus;"
              "sosd_books_200M:test_data_xml/sosd_books_200M.test_data.xml:sosd_uint64;");
}

// (2a) DEFAULT BYTE-IDENTISCH — Profile OHNE die neuen Tags parsen zu LEEREN Feldern (m3v2 TABU/NUR GELESEN
// + synthetisches Minimal-Profil); die --validate-Zusammenfassung bestehender Profile bleibt unveraendert.
TEST(WdkDatasetsFairness, DefaultsWithoutNewTagsAreEmptyAndReportUnchanged) {
    auto const m3v2 = tlz::load_thesis_profile(m3v2_profile_path());
    ASSERT_TRUE(m3v2.has_value());
    EXPECT_TRUE(m3v2->datasets.empty());
    for (auto const& s : m3v2->sota_series) EXPECT_EQ(s.fairness, "") << "m3v2 traegt kein fairness-Attribut";
    EXPECT_EQ(tlz::profile_datasets_signature(*m3v2), "");

    auto const bare = parse_temp("", "");
    ASSERT_TRUE(bare.has_value());
    EXPECT_TRUE(bare->datasets.empty());
    EXPECT_TRUE(bare->sota_series.empty());

    // Report-Neutralitaet: OHNE <datasets> erwaehnt die Zusammenfassung KEINE datasets (byte-identisches
    // Default-Verhalten der --validate-Ausgabe bestehender Profile).
    ex::AxisRegistry const             registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());
    tlz::ProfileValidationResult const vr       = tlz::validate_profile(*m3v2, registry);
    EXPECT_TRUE(vr.ok);
    EXPECT_EQ(vr.datasets_checked, 0u);
    std::ostringstream os;
    tlz::print_validation_report(vr, *m3v2, os);
    EXPECT_EQ(os.str().find("datasets"), std::string::npos) << "Report bestehender Profile darf sich nicht aendern:\n"
                                                            << os.str();
}

// (2b) GOLDEN-NEUTRALITAET — die gepinnte Basis des Beispiel-Profils (MIT datasets+fairness) erzeugt EXAKT
// die golden-320-Baseline-binary_id: die neuen Tags sind Mess-Input-/Reihen-METADATEN, keine Achsen.
TEST(WdkDatasetsFairness, NewTagsAreBinaryIdNeutralAgainstGolden) {
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
    EXPECT_EQ(view[0].binary_id, golden0) << "datasets/fairness duerfen die binary_id NIE beeinflussen";
}

// (3) VALIDATE — Beispiel-Profil OK (inkl. dokumentierter fairness-Duplikat-WARNUNG) + die Fehlerfaelle.
TEST(WdkDatasetsFairness, ValidateExampleOkAndErrorCases) {
    ex::AxisRegistry const registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());

    // Beispiel-Profil: OK (Exit-0-Aequivalent), 3 datasets geprueft, fairness-Duplikat-WARNUNG sichtbar.
    auto const tp = tlz::load_thesis_profile(example_profile_path());
    ASSERT_TRUE(tp.has_value());
    tlz::ProfileValidationResult const vr = tlz::validate_profile(*tp, registry);
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok);
    EXPECT_EQ(vr.datasets_checked, 3u);
    EXPECT_EQ(vr.series_checked, 3u);
    EXPECT_TRUE(any_contains(vr.warnings, "fairness-Varianten"))
        << "die binary_id-Gleichheit der fairness-Varianten (DATEN-gated Pinnung) muss als WARNUNG sichtbar sein";
    std::ostringstream os;
    tlz::print_validation_report(vr, *tp, os);
    EXPECT_NE(os.str().find("3 datasets"), std::string::npos) << os.str();

    // Fehlerfall fairness: nur common_denominator|native (oder weglassen) ist erlaubt.
    auto const bad_fair =
        parse_temp("", "  <sota_series_set><sota_series id=\"A\" lebewesen=\"hot\" merge=\"Stufe1_CeOnly\" "
                       "fairness=\"fastest\"/></sota_series_set>\n");
    ASSERT_TRUE(bad_fair.has_value());
    tlz::ProfileValidationResult const vf = tlz::validate_profile(*bad_fair, registry);
    EXPECT_FALSE(vf.ok);
    EXPECT_TRUE(any_contains(vf.errors, "UNGUELTIGER Fairness-Modus"));

    // Fehlerfaelle datasets: kein .test_data.xml-Suffix / fehlende id / doppelte id / fehlender loader.
    auto const bad_ds = parse_temp(
        "  <datasets>\n"
        "    <dataset id=\"url\" akte_ref=\"test_data_xml/url.xml\" loader=\"string_corpus\"/>\n"
        "    <dataset akte_ref=\"test_data_xml/protein.test_data.xml\" loader=\"string_corpus\"/>\n"
        "    <dataset id=\"trec\" akte_ref=\"test_data_xml/trec-terms.test_data.xml\" loader=\"\"/>\n"
        "    <dataset id=\"trec\" akte_ref=\"test_data_xml/trec-terms.test_data.xml\" loader=\"string_corpus\"/>\n"
        "  </datasets>\n",
        "");
    ASSERT_TRUE(bad_ds.has_value());
    tlz::ProfileValidationResult const vd = tlz::validate_profile(*bad_ds, registry);
    EXPECT_FALSE(vd.ok);
    EXPECT_EQ(vd.datasets_checked, 4u);
    EXPECT_TRUE(any_contains(vd.errors, "UNGUELTIGE Akten-Referenz"));
    EXPECT_TRUE(any_contains(vd.errors, "DATASET ohne id"));
    EXPECT_TRUE(any_contains(vd.errors, "DOPPELTE Dataset-id"));
    EXPECT_TRUE(any_contains(vd.errors, "DATASET ohne loader"));

    // Warnfall loader: fremde loader_id ist NICHT fatal (Registry laufzeit-offen), aber sichtbar.
    auto const odd_loader = parse_temp("  <datasets><dataset id=\"url\" akte_ref=\"test_data_xml/url.test_data.xml\" "
                                       "loader=\"string_korpus\"/></datasets>\n",
                                       "");
    ASSERT_TRUE(odd_loader.has_value());
    tlz::ProfileValidationResult const vl = tlz::validate_profile(*odd_loader, registry);
    EXPECT_TRUE(vl.ok) << "fremder loader ist WARNUNG, kein Fehler";
    EXPECT_TRUE(any_contains(vl.warnings, "keine Repo-Loader-id"));
}

// (4a) KONSUM: fairness kommt in der Runner-Spec an — build_sota_passes traegt den Modus je Pass.
TEST(WdkDatasetsFairness, FairnessArrivesInSotaPassSpec) {
    auto const tp = tlz::load_thesis_profile(example_profile_path());
    ASSERT_TRUE(tp.has_value());
    std::vector<tlz::SotaPass> const passes = tlz::build_sota_passes(*tp);
    ASSERT_EQ(passes.size(), 3u) << "alle 3 deklarierten Reihen sind real baubar (prt_art + 2x hot St1)";
    EXPECT_EQ(passes[0].lebewesen, "prt_art");
    EXPECT_EQ(passes[0].fairness_mode, "-") << "ungesetztes fairness ⇒ '-' (heutiges Verhalten)";
    EXPECT_EQ(passes[1].lebewesen, "hot");
    EXPECT_EQ(passes[1].fairness_mode, "common_denominator");
    EXPECT_EQ(passes[2].fairness_mode, "native");
    // EHRLICH dokumentierte Grenze (DATEN-gated Kompositions-Pinnung): beide hot-Modi teilen HEUTE
    // dieselbe view-binary_id — die Trennung lebt in fairness_mode-Tag + Resume-Stamp, nicht in der DLL.
    EXPECT_EQ(passes[1].view_binary_id, passes[2].view_binary_id);
}

// (4b) EXPORT: CSV-Endspalte fairness_mode + Resume-Stamp (resume-v5) konsumieren beide Felder —
// verschiedene Deklarationen ⇒ verschiedene Stamps (kein stales Cross-Resume), Default bleibt "-"/leer.
TEST(WdkDatasetsFairness, FairnessAndDatasetsAreExportedInCsvAndResumeStamp) {
    // CSV-Header: fairness_mode ist die LETZTE Spalte (additiv; alte Leser header-getrieben unberuehrt).
    std::string const header = ex::lazy_csv_header();
    ASSERT_GE(header.size(), 15u);
    EXPECT_EQ(header.substr(header.size() - 15), ";fairness_mode\n");

    // Row-Export: Default "-" und gesetzter Modus, Spaltenzahl == Header-Spaltenzahl (Schema-Identitaet).
    ex::LazyMeasuredRow row;
    row.binary_id         = "sota_tier=smoke";
    std::string const def = ex::format_csv_row(row);
    EXPECT_EQ(def.substr(def.size() - 3), ";-\n");
    row.fairness_mode        = "native";
    std::string const native = ex::format_csv_row(row);
    EXPECT_EQ(native.substr(native.size() - 8), ";native\n");
    EXPECT_EQ(count_char(def, ';'), count_char(header, ';'));
    EXPECT_EQ(count_char(native, ';'), count_char(header, ';'));

    // Resume-Stamp: v5 + fair=/datasets=-Segmente; Aenderung eines der beiden Felder aendert den Stamp.
    auto const tp = tlz::load_thesis_profile(example_profile_path());
    ASSERT_TRUE(tp.has_value());
    ex::LazyRunConfig cfg;
    cfg.row_fairness_mode      = "common_denominator";
    cfg.profile_datasets       = tlz::profile_datasets_signature(*tp);
    std::string const stamp_cd = ex::lazy_resume_stamp_prefix(cfg, {});
    EXPECT_EQ(stamp_cd.rfind("resume-v5|", 0), 0u) << stamp_cd;
    EXPECT_NE(stamp_cd.find("|fair=common_denominator|"), std::string::npos) << stamp_cd;
    EXPECT_NE(stamp_cd.find("|datasets=url:test_data_xml/url.test_data.xml:string_corpus;"), std::string::npos)
        << stamp_cd;

    ex::LazyRunConfig cfg_native = cfg;
    cfg_native.row_fairness_mode = "native";
    EXPECT_NE(ex::lazy_resume_stamp_prefix(cfg_native, {}), stamp_cd)
        << "common_denominator- und native-Reihe duerfen sich NIE gegenseitig resumen";

    ex::LazyRunConfig cfg_no_ds = cfg;
    cfg_no_ds.profile_datasets.clear();
    EXPECT_NE(ex::lazy_resume_stamp_prefix(cfg_no_ds, {}), stamp_cd)
        << "eine geaenderte <datasets>-Deklaration invalidiert alte Staende (konservativ)";

    // Default-Config (kein fairness, keine datasets): ehrliche Leer-Segmente.
    ex::LazyRunConfig const def_cfg;
    std::string const       def_stamp = ex::lazy_resume_stamp_prefix(def_cfg, {});
    EXPECT_NE(def_stamp.find("|fair=-|"), std::string::npos) << def_stamp;
    EXPECT_NE(def_stamp.find("|datasets=|"), std::string::npos) << def_stamp;
}
