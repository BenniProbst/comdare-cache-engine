// test_merge_plan_directive -- KERN-B K5 (Section 59, 2026-07-20): ctest-Gate der DEKLARATIVEN Merge-Naht
// (merge_plan.hpp::merge_plan_from_profile) + des direktiven-getriebenen Emitters (sota_catalog.hpp::
// render_directive_merge_module_source). REINE Daten-/Render-Schritte -- KEIN DLL-Bau, KEINE Messung.
//
// BEWEIST LITERAL:
//   (a) merge_plan: ein Profil OHNE per-Achse <axis merge=..> => LEERER Direktiven-Vektor => der Aufrufer nutzt
//       den KATALOG-Pfad (byte-identisch). Ein per-Achse-merge-Profil => je markierter Achse EINE Direktive mit
//       korrekter Strategie-Zuordnung (replace->Stufe2, merge->Stufe3) + Pruefling-Identitaet (Fork 3 self = leer).
//   (b) Direktiven-Pfad-Emission: eine synthetische per-Achse-merge-Direktive (path_compression/prt_art/Stufe2)
//       => der emittierte Quelltext traegt eine REALE MergeAxis<MergeStrategy::..>-Instanziierung (die
//       Generalisierung der hart aufgelisteten <Host>PrtStufeN-Typen) + den dritten Merge-Stempel.
//   (c) Byte-Additivitaet: render_sota_module_source OHNE merge_line ist byte-identisch zum heutigen Katalog-
//       Quelltext (Default-Argument); mit merge_line haengt es NUR den Merge-Stempel an (append-only).

#include "sota_catalog.hpp" // render_sota_module_source / render_directive_merge_module_source / directive_slot_types
#include "merge_plan.hpp"   // AxisMergeDirective / merge_plan_from_profile / merge_mode_to_strategy
#include "xml_config_parser/xml_config_parser.hpp" // ExperimentProfile / ExperimentAxisDefault / ExperimentPhase

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

namespace cx  = comdare::builder::xml;
namespace tlz = comdare::cache_engine::thesis_lazy;

// (a1) Leeres / heutiges Profil (KEINE per-Achse-Direktive) => LEERE Direktiven => Katalog-Pfad.
TEST(MergePlanDirective, EmptyProfileYieldsNoDirectivesFallsBackToCatalog) {
    cx::ExperimentProfile ep; // frisch, keine axes_default_lookup, keine phases
    EXPECT_TRUE(tlz::merge_plan_from_profile(ep).empty());

    // Auch mit axes_default_lookup OHNE merge-Attribut (leer = replace-Default OHNE Direktive) => leer.
    cx::ExperimentAxisDefault ax;
    ax.ref = "path_compression"; // KEIN merge_mode gesetzt
    ep.axes_default_lookup.push_back(ax);
    EXPECT_TRUE(tlz::merge_plan_from_profile(ep).empty())
        << "leeres merge_mode = replace-Default OHNE Direktive => Katalog-Pfad";
}

// (a2) merge_mode-Zuordnung (Single-Source): replace/""->Stufe2, merge/fulljoin->Stufe3.
TEST(MergePlanDirective, MergeModeToStrategyMapping) {
    EXPECT_EQ(tlz::merge_mode_to_strategy(""), "Stufe2_PrueflingReplace");
    EXPECT_EQ(tlz::merge_mode_to_strategy("replace"), "Stufe2_PrueflingReplace");
    EXPECT_EQ(tlz::merge_mode_to_strategy("merge"), "Stufe3_FullJoin");
    // KERN #48-S4 (Verdikt V-a): "fulljoin" = der EXPLIZITE Phase-3-Token, projiziert wie "merge" auf FullJoin.
    EXPECT_EQ(tlz::merge_mode_to_strategy("fulljoin"), "Stufe3_FullJoin");
}

// (a3) Ein per-Achse-merge-Profil => je markierter Achse EINE Direktive mit korrekter Strategie + Pruefling.
TEST(MergePlanDirective, PerAxisMergeProfileYieldsDirectives) {
    cx::ExperimentProfile ep;
    // Merge-Phase deklariert den Pruefling (nicht self) -> die Direktiven tragen diese Identitaet.
    cx::ExperimentPhase ph;
    ph.name      = "phase_prt";
    ph.merge     = "Stufe2_PrueflingReplace";
    ph.pruefling = "prt_art";
    ep.phases.push_back(ph);
    // Zwei per-Achse-Direktiven: path_compression=replace, node_type=merge.
    cx::ExperimentAxisDefault a1;
    a1.ref              = "path_compression";
    a1.merge_mode       = "replace";
    a1.allowed_variants = {"prt_patricia"};
    ep.axes_default_lookup.push_back(a1);
    cx::ExperimentAxisDefault a2;
    a2.ref        = "node_type";
    a2.merge_mode = "merge";
    ep.axes_default_lookup.push_back(a2);

    std::vector<tlz::AxisMergeDirective> const plan = tlz::merge_plan_from_profile(ep);
    ASSERT_EQ(plan.size(), 2u) << "je markierter Achse genau EINE Direktive (Dokument-Reihenfolge)";
    EXPECT_EQ(plan[0].axis_ref, "path_compression");
    EXPECT_EQ(plan[0].strategy, "Stufe2_PrueflingReplace");
    EXPECT_EQ(plan[0].pruefling_slot, "prt_art");
    ASSERT_EQ(plan[0].allowed_variants.size(), 1u);
    EXPECT_EQ(plan[0].allowed_variants.front(), "prt_patricia");
    EXPECT_EQ(plan[1].axis_ref, "node_type");
    EXPECT_EQ(plan[1].strategy, "Stufe3_FullJoin");
    EXPECT_EQ(plan[1].pruefling_slot, "prt_art");
}

// (a4) Fork 3: identity="CacheEngine"/self-Phase traegt keinen Merge-Pruefling => Slot leer (ce, Stufe1).
TEST(MergePlanDirective, SelfIdentityPhaseYieldsEmptyPrueflingSlot) {
    cx::ExperimentProfile ep;
    cx::ExperimentPhase   ph;
    ph.name     = "phase_self";
    ph.identity = "CacheEngine"; // Fork 3 self-Marker
    ep.phases.push_back(ph);
    cx::ExperimentAxisDefault ax;
    ax.ref        = "path_compression";
    ax.merge_mode = "replace";
    ep.axes_default_lookup.push_back(ax);

    std::vector<tlz::AxisMergeDirective> const plan = tlz::merge_plan_from_profile(ep);
    ASSERT_EQ(plan.size(), 1u);
    EXPECT_TRUE(plan[0].pruefling_slot.empty()) << "self-Phase => kein Merge-Pruefling (ce/Stufe1, leere Slot-Liste)";
}

// (b) Direktiven-Pfad-Emission: eine synthetische per-Achse-merge-Direktive (path_compression/prt_art/Stufe2)
//     => der emittierte Quelltext traegt eine REALE MergeAxis<MergeStrategy::..>-Instanziierung + den dritten
//     Merge-Stempel. directive_slot_types loest den realen (default, slot)-Typ auf (prt_art_merge_reference.hpp).
TEST(MergePlanDirective, DirectivePathEmitsRealMergeAxisInstantiation) {
    std::vector<tlz::AxisMergeDirective> const directives{
        tlz::AxisMergeDirective{"path_compression", "Stufe2_PrueflingReplace", "prt_art", {"prt_patricia"}}};
    // Der Merge-Stempel (K6a) fuer diese Kombination.
    std::string const merge_line = "merge=Stufe2_PrueflingReplace;pruefling=prt_art";
    std::string const src =
        tlz::render_directive_merge_module_source("::comdare::cache_engine::compositions::HotComposition",
                                                  "compositions/hot_reference.hpp", directives, merge_line);

    // Reale MergeAxis<>-Instanziierung ueber die directive-Achse (generalisiert, NICHT hart path_compression im Code).
    EXPECT_NE(src.find("pf::MergeAxis<pf::MergeStrategy::Stufe2_PrueflingReplace,"), std::string::npos)
        << "Direktiven-Pfad ohne MergeAxis-Instanziierung:\n"
        << src;
    EXPECT_NE(src.find("PrtArtPathCompressionSlot"), std::string::npos) << "realer Pruefling-Slot fehlt";
    EXPECT_NE(src.find("DirectiveMerged_path_compression"), std::string::npos);
    // Der reale Modul-Marker + der dritte Merge-Stempel (append-only).
    EXPECT_NE(src.find("COMDARE_DEFINE_ANATOMY_MODULE("), std::string::npos);
    EXPECT_NE(src.find("COMDARE_ANATOMY_VERSION_STAMP_MERGE(\"\", \"\", \"\", \"" + merge_line + "\")"),
              std::string::npos)
        << "dritter Merge-Stempel fehlt im Direktiven-Pfad";

    // directive_slot_types: reale Aufloesung fuer path_compression/prt_art, nullopt sonst (ehrlich).
    EXPECT_TRUE(tlz::directive_slot_types("path_compression", "prt_art").has_value());
    EXPECT_FALSE(tlz::directive_slot_types("node_type", "unknown_pruefling").has_value());
}

// (c) Byte-Additivitaet: render_sota_module_source OHNE merge_line == heutiger Katalog-Quelltext; mit merge_line
//     haengt es NUR den Merge-Stempel an (append-only; der Rest byte-identisch).
TEST(MergePlanDirective, SotaSourceMergeLineIsAppendOnly) {
    std::string const fq       = "::comdare::cache_engine::compositions::HotComposition";
    std::string const header   = "compositions/hot_reference.hpp";
    std::string const catalog  = tlz::render_sota_module_source(fq, header);     // Default merge_line = ""
    std::string const catalog2 = tlz::render_sota_module_source(fq, header, ""); // explizit leer
    EXPECT_EQ(catalog, catalog2) << "leeres merge_line != Default => nicht byte-identisch";
    EXPECT_EQ(catalog.find("COMDARE_ANATOMY_VERSION_STAMP_MERGE"), std::string::npos)
        << "ce-only/Katalog-Pfad darf KEINEN Merge-Stempel tragen (byte-identisch)";

    std::string const with_merge =
        tlz::render_sota_module_source(fq, header, "merge=Stufe2_PrueflingReplace;pruefling=prt_art");
    // Append-only: der Katalog-Quelltext ist ein exaktes Praefix des Merge-Quelltexts.
    EXPECT_EQ(with_merge.rfind(catalog, 0), 0u) << "merge_line ist NICHT append-only (Katalog-Praefix gebrochen)";
    EXPECT_NE(with_merge.find("COMDARE_ANATOMY_VERSION_STAMP_MERGE("), std::string::npos);
}

} // namespace
