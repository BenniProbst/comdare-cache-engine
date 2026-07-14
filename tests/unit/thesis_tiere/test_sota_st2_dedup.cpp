// test_sota_st2_dedup — M-CE-10 (Voll-Review 2026-07-13; per-Host-Auffächerung 2026-07-14, Ledger :1134
// "per-Host-Stufe2-Kompositionen nötig", bau-relevant mit F/G): ctest-Gate der Stufe2_PrueflingReplace-
// Materialisierung im sota_catalog.
//
// HISTORIE + KORREKTUR: FRÜHER (2026-07-13) war Stufe2 KONZEPTIONELL EINE Binary (der HOT-Pilot,
// HotPrtStufe2ReplaceComposition), weil nur der HOT-Host als Stufe2-Komposition existierte — N <sota_series
// merge="Stufe2_.." lebewesen=X> materialisierten daher DIESELBE HOT-Messung und wurden per (view_binary_id,
// fairness_mode)-Dedup zu 1 Pass kollabiert. Der damalige Anti-Phantom-Vorbehalt ("lebewesen in die binary_id
// zu ziehen = N FAKE-distinkte-ids für byte-identischen HOT-Code") galt, WEIL nur HOT existierte. JETZT faechert
// Stufe2 je SOTA-Host eine EIGENE reale <Host>PrtStufe2ReplaceComposition (prt_art_merge_reference.hpp,
// symmetrisch zu Stufe3) — die binary_ids sind damit GENUINE per-Host distinkt (verschiedene Hosts = verschiedene
// Kompositionen, KEIN Fake-id), und die H2-Attribution ist host-dominant mit host == angefragtem lebewesen.
//
// BEWEIST LITERAL (NEUE, korrigierte Semantik):
//   (1) PER-HOST-DISTINKT: mehrere St2-Deklarationen mit verschiedenen lebewesen ⇒ je Host EIN distinkter Pass
//       (build_sota_passes), NICHT mehr zu 1 kollabiert.
//   (2) H2-ATTRIBUTION host-dominant, host == lebewesen: der art-St2-Pass trägt h2_lebewesen == "art" (sein realer
//       Host), NICHT "hot"; über die committete Akte aufgelöst ergibt das den ART-Score.
//   (3) DISTINKT-ZÄHLUNG: das gepinnte m3v2_study (7 St1 + 7 St2 + 7 St3-Deklarationen) liefert 19 Pässe mit
//       19 DISTINKTEN view_binary_ids (7 St1 + 6 St2 [prt_art degeneriert] + 6 St3 [prt_art degeneriert]).
//   (4) KEINE ÜBER-KOLLABIERUNG: legitime fairness-Varianten (gleiche binary_id, verschiedener fairness_mode)
//       bleiben getrennte Pässe — der Dedup-Schlüssel ist (view_binary_id, fairness_mode), nicht nur binary_id.
//   (5) ECHTE DEDUP bleibt: zwei St2-Deklarationen mit DEMSELBEN lebewesen (gleicher Host) ⇒ 1 Pass.
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
// <sota_series_set>-Block. NUR für die synthetischen Distinkt-/Attributions-Fälle — die committeten Profile TABU.
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

// Ein Stufe2-Pass (per-Host): die view_binary_id trägt den <Host>PrtStufe2ReplaceComposition-Marker.
bool is_st2(tlz::SotaPass const& p) { return p.view_binary_id.find("PrtStufe2Replace") != std::string::npos; }

tlz::SotaPass const* find_st2_for_host(std::vector<tlz::SotaPass> const& passes, std::string const& host) {
    for (auto const& p : passes)
        if (is_st2(p) && p.h2_lebewesen == host) return &p;
    return nullptr;
}

} // namespace

// (1)+(2) PER-HOST-DISTINKT + HOST-DOMINANTE H2-ATTRIBUTION (host == lebewesen) — der Kern der M-CE-10-per-Host-
// Auffächerung über ein synthetisches Profil mit 3 verschiedenen St2-Lebewesen (art/masstree/surf).
TEST(SotaSt2Dedup, MultipleSt2DeclarationsArePerHostDistinctWithOwnHostH2) {
    auto const tp = parse_temp("  <sota_series_set>\n"
                               "    <sota_series id=\"A\" lebewesen=\"art\"      merge=\"Stufe2_PrueflingReplace\"/>\n"
                               "    <sota_series id=\"A\" lebewesen=\"masstree\" merge=\"Stufe2_PrueflingReplace\"/>\n"
                               "    <sota_series id=\"A\" lebewesen=\"surf\"     merge=\"Stufe2_PrueflingReplace\"/>\n"
                               "  </sota_series_set>\n");
    ASSERT_TRUE(tp.has_value());
    ASSERT_EQ(tp->sota_series.size(), 3u) << "3 St2-Deklarationen im Fixture";

    std::vector<tlz::SotaPass> const passes = tlz::build_sota_passes(*tp);
    ASSERT_EQ(passes.size(), 3u) << "M-CE-10 per-Host: 3 verschiedene St2-Lebewesen = 3 distinkte Pässe";
    EXPECT_EQ(distinct_view_binary_ids(passes), 3u) << "jeder Host trägt eine DISTINKTE view_binary_id";
    for (auto const& p : passes) {
        EXPECT_TRUE(is_st2(p)) << p.view_binary_id;
        EXPECT_EQ(p.series, "A");
        EXPECT_EQ(p.pruefling_type, "abstract");
        // host-dominant: der Host IST das angefragte lebewesen (kein FIX "hot" mehr).
        EXPECT_EQ(p.h2_lebewesen, p.lebewesen) << "St2 trägt seinen eigenen Host (host == lebewesen)";
    }

    // Über die committete Akte aufgelöst: der art-Pass trägt den ART-Score, NICHT den HOT-Score (der M-CE-10-Bug).
    auto const akte = tlz::parse_h2_score_akte(akte_path());
    ASSERT_TRUE(akte.has_value());
    tlz::H2CodeQualityEntry const* hot = akte->entry("hot");
    tlz::H2CodeQualityEntry const* art = akte->entry("art");
    ASSERT_NE(hot, nullptr);
    ASSERT_NE(art, nullptr);
    ASSERT_NE(hot->score, art->score) << "Fixture-Voraussetzung: hot- und art-Score unterscheiden sich";
    tlz::SotaPass const* art_pass = find_st2_for_host(passes, "art");
    ASSERT_NE(art_pass, nullptr) << "der art-St2-Pass existiert (per-Host)";
    EXPECT_EQ(tlz::h2_score_for(*akte, art_pass->h2_lebewesen), art->score) << "art-St2-Score == ART-Host-Score";
    EXPECT_NE(tlz::h2_score_for(*akte, art_pass->h2_lebewesen), hot->score)
        << "der art-St2-Pass darf NIE den HOT-Score tragen (die M-CE-10-Fehl-Attribution ist behoben)";
}

// (3) DISTINKT-ZÄHLUNG am gepinnten m3v2_study (7 St1 + 7 St2 + 7 St3): 19 Pässe / 19 distinkte view_binary_ids.
TEST(SotaSt2Dedup, M3v2StudyYieldsNineteenDistinctSotaBinaries) {
    auto const tp = tlz::load_thesis_profile(m3v2_profile_path());
    ASSERT_TRUE(tp.has_value()) << "m3v2_study nicht lesbar (TABU-Wache): " << m3v2_profile_path().string();

    std::vector<tlz::SotaPass> const passes = tlz::build_sota_passes(*tp);
    // 7 St1 (prt_art + 6 SOTA isoliert) + 6 St2 (per-Host; prt_art degeneriert = nullopt) + 6 St3 (prt_art
    // degeneriert = nullopt) = 19 distinkte reale Kompositionen (M-CE-10 per-Host: vor der Auffächerung 14).
    EXPECT_EQ(passes.size(), 19u) << "M-CE-10 per-Host: 7 St1 + 6 St2 + 6 St3 = 19 Pässe";
    EXPECT_EQ(distinct_view_binary_ids(passes), 19u) << "jeder Pass trägt eine DISTINKTE view_binary_id";
    EXPECT_EQ(passes.size(), distinct_view_binary_ids(passes))
        << "kein Pass dupliziert die Messung eines anderen (== res.sota_binary_ids-Semantik im Treiber)";

    // GENAU 6 St2-Pässe (art/hot/masstree/surf/start/wormhole), je host-dominant auf DEN EIGENEN Host attribuiert.
    std::set<std::string> st2_hosts;
    for (auto const& p : passes)
        if (is_st2(p)) {
            st2_hosts.insert(p.h2_lebewesen);
            EXPECT_EQ(p.h2_lebewesen, p.lebewesen) << "jeder St2-Pass trägt seinen eigenen Host-Score";
            EXPECT_EQ(p.pruefling_type, "abstract");
            EXPECT_NE(p.h2_lebewesen, "prt_art") << "prt_art-Host ist in St2 degeneriert (nullopt)";
        }
    EXPECT_EQ(st2_hosts.size(), 6u) << "die 7 St2-Deklarationen ergeben 6 reale per-Host-Pässe (prt_art degeneriert)";
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

// (5) ECHTE DEDUP bleibt: zwei St2-Deklarationen mit DEMSELBEN lebewesen (gleicher Host, gleicher fairness_mode)
// kollabieren weiterhin zu GENAU 1 Pass — der Dedup fängt nur WIRKLICH identische Messungen ab.
TEST(SotaSt2Dedup, SameHostSt2DeclarationsStillDedupToOne) {
    auto const tp = parse_temp("  <sota_series_set>\n"
                               "    <sota_series id=\"A\" lebewesen=\"art\" merge=\"Stufe2_PrueflingReplace\"/>\n"
                               "    <sota_series id=\"A\" lebewesen=\"art\" merge=\"Stufe2_PrueflingReplace\"/>\n"
                               "  </sota_series_set>\n");
    ASSERT_TRUE(tp.has_value());
    ASSERT_EQ(tp->sota_series.size(), 2u);

    std::vector<tlz::SotaPass> const passes = tlz::build_sota_passes(*tp);
    ASSERT_EQ(passes.size(), 1u) << "gleicher Host (art) + gleicher fairness_mode ⇒ echte Dedup zu 1 Pass";
    EXPECT_TRUE(is_st2(passes[0]));
    EXPECT_EQ(passes[0].h2_lebewesen, "art");
}
