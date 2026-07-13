// test_sota_st2_dedup — M-CE-10 (Voll-Review 2026-07-13, docs/sessions/backups/20260712-full-line-review/
// REVIEW-BERICHT.md:104-107; correctness / Anti-Phantom): ctest-Gate der KORREKTUR der
// Stufe2_PrueflingReplace-Materialisierung im sota_catalog.
//
// DER BEFUND: Stufe2 ist KONZEPTIONELL EINE Binary — der HOT-Pilot (HotPrtStufe2ReplaceComposition: HOT-Host,
// PRT-ART ersetzt den path_compression-Slot). Die binary_id trägt bewusst KEIN lebewesen (der Wert existiert
// real nicht per-lebewesen). Vor dem Fix erzeugten N <sota_series merge="Stufe2_.."> mit verschiedenen
// lebewesen N Pässe auf DERSELBEN Binary ⇒ N× dieselben Messzeilen in EINE CSV, Über-Zählung der DLL-Zahl und
// H2-Score des FALSCHEN Lebewesens. Anti-Phantom: lebewesen in die binary_id zu ziehen wäre FALSCH (N
// FAKE-distinkte-ids für byte-identischen HOT-Code). Der saubere Fix ist: St2 = 1 Binary.
//
// BEWEIST LITERAL:
//   (1) DEDUP: mehrere St2-Deklarationen mit verschiedenen lebewesen ⇒ GENAU 1 Pass (build_sota_passes).
//   (2) H2-ATTRIBUTION host-dominant: der St2-Pass trägt h2_lebewesen == "hot" (der reale Host), NICHT das
//       angefragte lebewesen. Über die committete Akte aufgelöst ergibt das den HOT-Score, NICHT den des
//       angefragten Lebewesens (der vor dem Fix falsch angehängt wurde).
//   (3) DISTINKT-ZÄHLUNG: das gepinnte m3v2_study (7 St1 + 7 St2 + 7 St3-Deklarationen) liefert 14 Pässe mit
//       14 DISTINKTEN view_binary_ids (7 St1 + 1 St2-Pilot + 6 St3; St3/prt_art degeneriert = nullopt) — die
//       Zahl, die res.sota_binary_ids im Treiber (distinkter Set-Guard) zählt.
//   (4) KEINE ÜBER-KOLLABIERUNG: legitime fairness-Varianten (gleiche binary_id, verschiedener fairness_mode)
//       bleiben getrennte Pässe — der Dedup-Schlüssel ist (view_binary_id, fairness_mode), nicht nur binary_id.
//
// TABU-Wache: m3v2_study.profile.xml + sota_h2_scores.xml werden NUR GELESEN (byte-unberührt).

#include "h2_score_akte.hpp"  // parse_h2_score_akte / h2_score_for
#include "profile_runner.hpp" // load_thesis_profile
#include "sota_catalog.hpp"   // build_sota_passes / SotaPass

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <string>
#include <system_error>
#include <vector>

#ifndef COMDARE_THESIS_PROFILES_DIR
#error "COMDARE_THESIS_PROFILES_DIR must point to libs/cache_engine/algorithm_profiles/thesis_profiles"
#endif
#ifndef COMDARE_SOTA_PROFILES_DIR
#error "COMDARE_SOTA_PROFILES_DIR must point to libs/cache_engine/algorithm_profiles/sota"
#endif

namespace {

namespace tlz = comdare::cache_engine::thesis_lazy;
namespace fs  = std::filesystem;

fs::path m3v2_profile_path() { return fs::path{COMDARE_THESIS_PROFILES_DIR} / "m3v2_study.profile.xml"; }
fs::path akte_path() { return fs::path{COMDARE_SOTA_PROFILES_DIR} / "sota_h2_scores.xml"; }

// Temp-Fixture (Muster test_wdk_datasets_fairness::write_temp_profile): ein Minimal-Profil mit frei wählbarem
// <sota_series_set>-Block. NUR für die synthetischen Dedup-/Attributions-Fälle — die committeten Profile TABU.
fs::path write_temp_profile(std::string const& series_block) {
    fs::path const p = fs::temp_directory_path() /
                       ("comdare_sota_st2_dedup_fixture_" +
                        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".xml");
    std::ofstream  out{p};
    out << R"(<?xml version="1.0" encoding="UTF-8"?>
<comdare_thesis_profile id="st2_fixture" schema_version="1">
  <base_tiers><tier id="hot" profile_ref="../sota/hot.profile.xml" paper_ref="P02"/></base_tiers>
  <permute_axes><axis ref="search_algo"><value>k_ary</value></axis></permute_axes>
)" << series_block
        << R"(  <modes><mode name="ce_only" merge="Stufe1_CeOnly" active_axes="search_algo"/></modes>
  <static_axes from="base_tier"/>
</comdare_thesis_profile>
)";
    return p;
}

std::optional<comdare::builder::xml::ThesisProfile> parse_temp(std::string const& series_block) {
    fs::path const  p  = write_temp_profile(series_block);
    auto const      tp = tlz::load_thesis_profile(p);
    std::error_code ec;
    fs::remove(p, ec);
    return tp;
}

std::size_t distinct_view_binary_ids(std::vector<tlz::SotaPass> const& passes) {
    std::set<std::string> ids;
    for (auto const& p : passes) ids.insert(p.view_binary_id);
    return ids.size();
}

bool is_st2_pilot(tlz::SotaPass const& p) {
    return p.view_binary_id.find("HotPrtStufe2ReplaceComposition") != std::string::npos;
}

} // namespace

// (1)+(2) DEDUP + HOST-DOMINANTE H2-ATTRIBUTION — der Kern des M-CE-10-Fixes über ein synthetisches Profil,
// dessen 3 St2-Deklarationen BEWUSST NICHT "hot" anfragen (art/masstree/surf): trotzdem 1 Pass, h2_lebewesen "hot".
TEST(SotaSt2Dedup, MultipleSt2DeclarationsCollapseToOneHotPilotWithHostH2) {
    auto const tp = parse_temp("  <sota_series_set>\n"
                               "    <sota_series id=\"A\" lebewesen=\"art\"      merge=\"Stufe2_PrueflingReplace\"/>\n"
                               "    <sota_series id=\"A\" lebewesen=\"masstree\" merge=\"Stufe2_PrueflingReplace\"/>\n"
                               "    <sota_series id=\"A\" lebewesen=\"surf\"     merge=\"Stufe2_PrueflingReplace\"/>\n"
                               "  </sota_series_set>\n");
    ASSERT_TRUE(tp.has_value());
    ASSERT_EQ(tp->sota_series.size(), 3u) << "3 St2-Deklarationen im Fixture";

    std::vector<tlz::SotaPass> const passes = tlz::build_sota_passes(*tp);
    ASSERT_EQ(passes.size(), 1u) << "M-CE-10 (a): 3 St2-Deklarationen = 1 realer HOT-Pilot-Pass (Dedup)";
    tlz::SotaPass const& st2 = passes[0];
    EXPECT_TRUE(is_st2_pilot(st2)) << st2.view_binary_id;
    EXPECT_EQ(st2.series, "A");
    EXPECT_EQ(st2.pruefling_type, "abstract");

    // M-CE-10 (c): host-dominant. Der Host ist FIX "hot" — NIE das angefragte lebewesen (art/masstree/surf).
    EXPECT_EQ(st2.h2_lebewesen, "hot") << "St2 trägt den HOST (hot), nicht das angefragte lebewesen";
    EXPECT_NE(st2.h2_lebewesen, "art");
    EXPECT_NE(st2.h2_lebewesen, "surf");

    // Über die committete Akte aufgelöst: der HOT-Score, NICHT der des (vor dem Fix falsch attribuierten) art.
    auto const akte = tlz::parse_h2_score_akte(akte_path());
    ASSERT_TRUE(akte.has_value());
    tlz::H2CodeQualityEntry const* hot = akte->entry("hot");
    tlz::H2CodeQualityEntry const* art = akte->entry("art");
    ASSERT_NE(hot, nullptr);
    ASSERT_NE(art, nullptr);
    ASSERT_NE(hot->score, art->score) << "Fixture-Voraussetzung: hot- und art-Score unterscheiden sich";
    EXPECT_EQ(tlz::h2_score_for(*akte, st2.h2_lebewesen), hot->score) << "St2-Score == HOT-Host-Score";
    EXPECT_NE(tlz::h2_score_for(*akte, st2.h2_lebewesen), art->score)
        << "St2 darf NIE den H2-Score des angefragten (art) Lebewesens tragen (der M-CE-10-Bug)";
}

// (3) DISTINKT-ZÄHLUNG am gepinnten m3v2_study (7 St1 + 7 St2 + 7 St3): 14 Pässe / 14 distinkte view_binary_ids.
TEST(SotaSt2Dedup, M3v2StudyYieldsFourteenDistinctSotaBinaries) {
    auto const tp = tlz::load_thesis_profile(m3v2_profile_path());
    ASSERT_TRUE(tp.has_value()) << "m3v2_study nicht lesbar (TABU-Wache): " << m3v2_profile_path().string();

    std::vector<tlz::SotaPass> const passes = tlz::build_sota_passes(*tp);
    // 7 St1 (prt_art + 6 SOTA isoliert) + 1 St2-Pilot (7 Deklarationen dedupliziert) + 6 St3 (prt_art degeneriert
    // = nullopt). Das ist die "real 14 distinkte DLLs"-Zahl des Befunds (vor dem Fix zählte der Treiber 20).
    EXPECT_EQ(passes.size(), 14u) << "M-CE-10: 20 St1/St2/St3-Module minus 6 St2-Duplikate = 14 Pässe";
    EXPECT_EQ(distinct_view_binary_ids(passes), 14u) << "jeder Pass trägt eine DISTINKTE view_binary_id";
    EXPECT_EQ(passes.size(), distinct_view_binary_ids(passes))
        << "kein Pass dupliziert die Messung eines anderen (== res.sota_binary_ids-Semantik im Treiber)";

    // GENAU 1 St2-Pilot-Pass, host-dominant auf "hot" attribuiert.
    std::size_t st2_count = 0;
    for (auto const& p : passes)
        if (is_st2_pilot(p)) {
            ++st2_count;
            EXPECT_EQ(p.h2_lebewesen, "hot") << "der einzige St2-Pilot trägt den HOT-Host-Score";
            EXPECT_EQ(p.pruefling_type, "abstract");
        }
    EXPECT_EQ(st2_count, 1u) << "die 7 St2-Deklarationen kollabieren zu GENAU 1 HOT-Pilot";
}

// (4) KEINE ÜBER-KOLLABIERUNG — legitime fairness-Varianten (gleiche binary_id, verschiedener fairness_mode)
// bleiben getrennte Pässe. Sonst würde der Dedup die dokumentierte #156-Pinnungs-Semantik zerstören.
TEST(SotaSt2Dedup, FairnessVariantsAreNotCollapsedBySameBinaryId) {
    auto const tp = parse_temp(
        "  <sota_series_set>\n"
        "    <sota_series id=\"A\" lebewesen=\"hot\" merge=\"Stufe1_CeOnly\" fairness=\"common_denominator\"/>\n"
        "    <sota_series id=\"A\" lebewesen=\"hot\" merge=\"Stufe1_CeOnly\" fairness=\"native\"/>\n"
        "  </sota_series_set>\n");
    ASSERT_TRUE(tp.has_value());

    std::vector<tlz::SotaPass> const passes = tlz::build_sota_passes(*tp);
    ASSERT_EQ(passes.size(), 2u) << "gleiche binary_id, aber verschiedener fairness_mode ⇒ 2 Pässe (nicht dedupt)";
    EXPECT_EQ(passes[0].view_binary_id, passes[1].view_binary_id) << "beide teilen HEUTE dieselbe view-binary_id";
    EXPECT_NE(passes[0].fairness_mode, passes[1].fairness_mode);
    EXPECT_EQ(passes[0].fairness_mode, "common_denominator");
    EXPECT_EQ(passes[1].fairness_mode, "native");
    // Distinkte view_binary_ids == 1 (EINE reale DLL) — genau das zählt res.sota_binary_ids im Treiber, obwohl
    // 2 Pässe laufen (die Trennung lebt im fairness_mode-Tag + Resume-Stamp, nicht in der DLL).
    EXPECT_EQ(distinct_view_binary_ids(passes), 1u);
}
