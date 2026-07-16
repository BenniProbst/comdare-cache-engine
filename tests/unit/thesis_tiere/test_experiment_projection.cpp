// test_experiment_projection — BRÜCKE I3 (2026-07-16): ctest-Gate der Projektion ExperimentProfile →
// (merge×lebewesen)-Pässe/Quellen-Map (sota_catalog.hpp::project_experiment_to_sota_passes). Es ist der
// REINE Enumerations-/Render-Schritt der Fork-A-Brücke — KEIN DLL-Bau, KEINE Messung, KEIN
// run_lazy_static_then_dynamic. Muster: test_experiment_parser.cpp (INC-D) + test_sota_st2_dedup.cpp (M-CE-10).
//
// BEWEIST LITERAL (für die committete Golden-Instanz experiment_golden.xml — 3 Phasen Stufe1/2/3, 7 lebewesen):
//   (a) je Phase die WÖRTLICH erwarteten view-binary_id-Keys "sota_tier=sota::<Reihe>::<Composition>"
//       (Stufe1→Reihe A, Stufe2→Reihe A, Stufe3→Reihe B — mechanisch via stufe_to_reihe), in Reihenfolge;
//   (b) je erzeugtem Pass ein NICHT-LEERER Quelltext (render_sota_module_source), 1:1 über view_binary_id
//       mit der Pass-Liste verknüpft, mit dem realen COMDARE_DEFINE_ANATOMY_MODULE(<FQ-Typ>)-Marker;
//   (c) nullopt-Paare EHRLICH ausgelassen — prt_art ist unter Stufe2/Stufe3 degeneriert ⇒ KEIN Phantom-Key/-Pass
//       (6 statt 7 Pässe, kein PrtArtComposition-Key in den Merge-Phasen).
//
// LESE-Schicht: die Fixture experiment_golden.xml wird NUR GELESEN; kein Treiber-Lauf, keine #156-Messdaten.

#include "sota_catalog.hpp" // project_experiment_to_sota_passes / SotaPass / ExperimentPhaseProjection
#include "xml_config_parser/xml_config_parser.hpp" // XmlConfigParser / ExperimentProfile

#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <vector>

#ifndef COMDARE_EXPERIMENT_GOLDEN
#error "COMDARE_EXPERIMENT_GOLDEN must point to tests/unit/thesis_tiere/experiment_golden.xml"
#endif

namespace {

namespace cx  = comdare::builder::xml;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace fs  = std::filesystem;

std::optional<cx::ExperimentProfile> parse_golden() {
    cx::XmlConfigParser const parser;
    return parser.parse_experiment_profile(fs::path{COMDARE_EXPERIMENT_GOLDEN});
}

// Die view_binary_ids der Pass-Liste EINER Phase, in Pass-Reihenfolge.
std::vector<std::string> pass_view_ids(tlz::ExperimentPhaseProjection const& pp) {
    std::vector<std::string> v;
    v.reserve(pp.passes.size());
    for (auto const& p : pp.passes) v.push_back(p.view_binary_id);
    return v;
}

// Erwartete Stufe1-Keys (Reihe A, isolierte Lebewesen) — golden-lebewesen-Reihenfolge (prt_art zuerst).
std::vector<std::string> const kStufe1Keys = {
    "sota_tier=sota::A::PrtArtComposition",  "sota_tier=sota::A::ArtComposition",
    "sota_tier=sota::A::HotComposition",     "sota_tier=sota::A::MasstreeComposition",
    "sota_tier=sota::A::SurfComposition",    "sota_tier=sota::A::StartComposition",
    "sota_tier=sota::A::WormholeComposition"};

// Erwartete Stufe2-Keys (Reihe A, per-Host-Replace) — prt_art degeneriert (ausgelassen).
std::vector<std::string> const kStufe2Keys = {
    "sota_tier=sota::A::ArtPrtStufe2ReplaceComposition",      "sota_tier=sota::A::HotPrtStufe2ReplaceComposition",
    "sota_tier=sota::A::MasstreePrtStufe2ReplaceComposition", "sota_tier=sota::A::SurfPrtStufe2ReplaceComposition",
    "sota_tier=sota::A::StartPrtStufe2ReplaceComposition",    "sota_tier=sota::A::WormholePrtStufe2ReplaceComposition"};

// Erwartete Stufe3-Keys (Reihe B, per-Host-FullJoin) — prt_art degeneriert (ausgelassen).
std::vector<std::string> const kStufe3Keys = {"sota_tier=sota::B::ArtPrtStufe3FullJoinComposition",
                                              "sota_tier=sota::B::HotPrtStufe3FullJoinComposition",
                                              "sota_tier=sota::B::MasstreePrtStufe3FullJoinComposition",
                                              "sota_tier=sota::B::SurfPrtStufe3FullJoinComposition",
                                              "sota_tier=sota::B::StartPrtStufe3FullJoinComposition",
                                              "sota_tier=sota::B::WormholePrtStufe3FullJoinComposition"};

} // namespace

// (a) je Phase die WÖRTLICH erwarteten sota_tier=sota::<S>::<name>-Keys (in Reihenfolge) + series/pruefling_type.
TEST(ExperimentProjection, ProjectsGoldenPhasesToLiteralSotaKeys) {
    auto const ep = parse_golden();
    ASSERT_TRUE(ep.has_value()) << "experiment_golden.xml nicht parsbar: " << COMDARE_EXPERIMENT_GOLDEN;
    ASSERT_EQ(ep->lebewesen.size(), 7u);
    ASSERT_EQ(ep->phases.size(), 3u);

    std::vector<tlz::ExperimentPhaseProjection> const proj = tlz::project_experiment_to_sota_passes(*ep);
    ASSERT_EQ(proj.size(), 3u) << "je <phase> genau EINE Projektion";

    // Phase 0 — phase2_cache_engine / Stufe1_CeOnly → Reihe A, 7 isolierte Lebewesen.
    EXPECT_EQ(proj[0].phase_name, "phase2_cache_engine");
    EXPECT_EQ(proj[0].merge, "Stufe1_CeOnly");
    ASSERT_EQ(proj[0].passes.size(), 7u) << "Stufe1: alle 7 Lebewesen real baubar";
    EXPECT_EQ(pass_view_ids(proj[0]), kStufe1Keys);
    for (auto const& p : proj[0].passes) {
        EXPECT_EQ(p.series, "A");
        EXPECT_EQ(p.pruefling_type, "full");
    }

    // Phase 1 — phase1_prt_art / Stufe2_PrueflingReplace → Reihe A (mechanisch via stufe_to_reihe), 6 per-Host.
    EXPECT_EQ(proj[1].phase_name, "phase1_prt_art");
    EXPECT_EQ(proj[1].merge, "Stufe2_PrueflingReplace");
    ASSERT_EQ(proj[1].passes.size(), 6u) << "Stufe2: prt_art degeneriert ⇒ 6 per-Host-Pässe";
    EXPECT_EQ(pass_view_ids(proj[1]), kStufe2Keys);
    for (auto const& p : proj[1].passes) {
        EXPECT_EQ(p.series, "A");
        EXPECT_EQ(p.pruefling_type, "abstract");
    }

    // Phase 2 — phase3_kombiniert / Stufe3_FullJoin → Reihe B, 6 per-Host.
    EXPECT_EQ(proj[2].phase_name, "phase3_kombiniert");
    EXPECT_EQ(proj[2].merge, "Stufe3_FullJoin");
    ASSERT_EQ(proj[2].passes.size(), 6u) << "Stufe3: prt_art degeneriert ⇒ 6 per-Host-Pässe";
    EXPECT_EQ(pass_view_ids(proj[2]), kStufe3Keys);
    for (auto const& p : proj[2].passes) {
        EXPECT_EQ(p.series, "B");
        EXPECT_EQ(p.pruefling_type, "abstract");
    }

    // Gesamt-Bilanz: 7 + 6 + 6 = 19 Pässe (spiegelt m3v2_study 7 St1 + 6 St2 + 6 St3, test_sota_st2_dedup).
    EXPECT_EQ(proj[0].passes.size() + proj[1].passes.size() + proj[2].passes.size(), 19u);
}

// (b) je erzeugtem Pass ein NICHT-LEERER Quelltext, 1:1 über view_binary_id verknüpft, mit dem realen Modul-Marker.
TEST(ExperimentProjection, EveryPassHasNonEmptyRenderedSource) {
    auto const ep = parse_golden();
    ASSERT_TRUE(ep.has_value());

    std::vector<tlz::ExperimentPhaseProjection> const proj = tlz::project_experiment_to_sota_passes(*ep);
    ASSERT_EQ(proj.size(), 3u);

    for (auto const& pp : proj) {
        // 1:1 — jeder Pass hat GENAU einen Quellen-Eintrag, keine verwaisten Keys (innerhalb einer Phase keine Dedup).
        EXPECT_EQ(pp.passes.size(), pp.source_by_view_id.size())
            << "Phase " << pp.phase_name << ": Pass-Zahl == Quellen-Map-Größe";
        for (auto const& p : pp.passes) {
            auto const it = pp.source_by_view_id.find(p.view_binary_id);
            ASSERT_NE(it, pp.source_by_view_id.end())
                << "Phase " << pp.phase_name << ": Quelle fehlt für " << p.view_binary_id;
            EXPECT_FALSE(it->second.empty()) << "leerer Quelltext für " << p.view_binary_id;
            EXPECT_NE(it->second.find("COMDARE_DEFINE_ANATOMY_MODULE("), std::string::npos)
                << "Quelltext ohne Modul-Marker für " << p.view_binary_id;
        }
    }
}

// (c) nullopt-Paare EHRLICH ausgelassen — prt_art ist unter Stufe2/Stufe3 degeneriert (KEIN Phantom-Key/-Pass).
TEST(ExperimentProjection, DegeneratePrtArtPairsHonestlyOmitted) {
    auto const ep = parse_golden();
    ASSERT_TRUE(ep.has_value());
    // prt_art ist das ERSTE deklarierte Lebewesen (das Kreuzprodukt fragt es je Phase ab).
    ASSERT_EQ(ep->lebewesen.front(), "prt_art");

    std::vector<tlz::ExperimentPhaseProjection> const proj = tlz::project_experiment_to_sota_passes(*ep);
    ASSERT_EQ(proj.size(), 3u);

    // Stufe1: prt_art IST baubar (isoliert) ⇒ es existiert genau ein PrtArtComposition-Key + prt_art-Pass.
    {
        std::vector<std::string> const vids = pass_view_ids(proj[0]);
        std::set<std::string> const    ids{vids.begin(), vids.end()};
        EXPECT_TRUE(ids.count("sota_tier=sota::A::PrtArtComposition")) << "Stufe1: prt_art isoliert baubar (Reihe A)";
        bool has_prt_pass = false;
        for (auto const& p : proj[0].passes)
            if (p.lebewesen == "prt_art") has_prt_pass = true;
        EXPECT_TRUE(has_prt_pass);
    }

    // Stufe2 + Stufe3: prt_art-als-Host degeneriert ⇒ 6 (nicht 7) Pässe, KEIN PrtArtComposition-Key,
    // KEIN prt_art-Pass, KEIN prt_art-Quellen-Key (nullopt ehrlich ausgelassen, kein Phantom).
    for (std::size_t i : {std::size_t{1}, std::size_t{2}}) {
        EXPECT_EQ(proj[i].passes.size(), 6u)
            << "Phase " << proj[i].phase_name << ": 7 Lebewesen - prt_art degeneriert = 6";
        for (auto const& p : proj[i].passes) {
            EXPECT_NE(p.lebewesen, "prt_art") << "kein prt_art-Pass in einer Merge-Phase";
            EXPECT_EQ(p.view_binary_id.find("PrtArtComposition"), std::string::npos)
                << "kein isolierter PrtArtComposition-Key in einer Merge-Phase: " << p.view_binary_id;
        }
        for (auto const& [view_id, src] : proj[i].source_by_view_id)
            EXPECT_EQ(view_id.find("PrtArtComposition"), std::string::npos)
                << "kein Phantom-prt_art-Quellen-Key in Phase " << proj[i].phase_name << ": " << view_id;
    }
}
