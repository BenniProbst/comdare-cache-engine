// test_merge_plan_directive -- KERN-B K5 (Section 59, 2026-07-20): ctest-Gate der DEKLARATIVEN Merge-Naht
// (merge_plan.hpp::merge_plan_from_profile) + des direktiven-getriebenen Emitters (sota_catalog.hpp::
// render_directive_merge_module_source). REINE Daten-/Render-Schritte -- KEIN DLL-Bau, KEINE Messung.
//
// BEWEIST LITERAL:
//   (a) merge_plan: ein Profil OHNE per-Achse <axis merge=..> => LEERER Direktiven-Vektor => der Aufrufer nutzt
//       den KATALOG-Pfad (byte-identisch). Ein per-Achse-merge-Profil => je markierter Achse EINE Direktive mit
//       korrekter Strategie-Zuordnung (replace->Stufe2_PrueflingReplace, merge->Stufe2_Hybrid, fulljoin->Stufe3_
//       FullJoin; R6/§59-A(2)+A(3)) + Pruefling-Identitaet (Fork 3 self = leer).
//   (b) Direktiven-Pfad-Emission: eine synthetische per-Achse-merge-Direktive (path_compression/prt_art/Stufe2)
//       => der emittierte Quelltext traegt eine REALE MergeAxis<MergeStrategy::..>-Instanziierung (die
//       Generalisierung der hart aufgelisteten <Host>PrtStufeN-Typen).
//   (c) Byte-Additivitaet: render_sota_module_source OHNE Stempel ist byte-identisch zum heutigen Katalog-
//       Quelltext (Default-Argument); mit Stempel haengt es NUR die Stempel-Zeile an (append-only).
//   (d) A13-M3/C1 (K-3): der SOTA-Emitter reicht die VOLLEN organ/system/measurement-Zeilen durch -- inklusive
//       der EHRLICHEN Feststellung, dass die Organ-Zeile fuer SOTA-binary_ids leer BLEIBT (kein 17-Achsen-Pfad).
//
// A13-M3/C3 (Owner-E2 02.08.2026): die MERGE-ZEILE existiert nicht mehr -- alle frueheren merge_line-/
// _MERGE-Erwartungen sind hier entfallen. Was BLEIBT und hier weiter scharf geprueft wird, ist die
// Merge-DURCHFUEHRUNG (Owner-Q2): merge_plan_from_profile, die Strategie-Zuordnung und die reale
// MergeAxis<>-Instanziierung des Direktiven-Emitters. Nur ihr STEMPEL-Bezug ist weg.

#include "sota_catalog.hpp" // render_sota_module_source / render_directive_merge_module_source / directive_slot_types
#include "merge_plan.hpp"   // AxisMergeDirective / merge_plan_from_profile / merge_mode_to_strategy
#include "xml_config_parser/xml_config_parser.hpp" // ExperimentProfile / ExperimentAxisDefault / ExperimentPhase

#include <cache_engine/abi/anatomy_version_stamp.hpp> // A13-M3/C1: abi::system_stamp_line (die REALE System-Zeile)

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

namespace cx  = comdare::builder::xml;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace cea = comdare::cache_engine::abi;

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

// (a2) merge_mode-Zuordnung (Single-Source): replace/""->Stufe2_PrueflingReplace, merge->Stufe2_Hybrid,
//      fulljoin->Stufe3_FullJoin. R6/§59-A(2)+A(3): "merge" != "fulljoin" (nicht mehr vermischt).
TEST(MergePlanDirective, MergeModeToStrategyMapping) {
    EXPECT_EQ(tlz::merge_mode_to_strategy(""), "Stufe2_PrueflingReplace");
    EXPECT_EQ(tlz::merge_mode_to_strategy("replace"), "Stufe2_PrueflingReplace");
    // R6 (§59-A(2)): "merge" = CE+Pruefling-Hybrid je Pruefling => eigener Name Stufe2_Hybrid (NICHT FullJoin).
    EXPECT_EQ(tlz::merge_mode_to_strategy("merge"), "Stufe2_Hybrid");
    // R6 (§59-A(3)): "fulljoin" = kombinierte Union, der EXPLIZITE Phase-3-Token => Stufe3_FullJoin.
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
    EXPECT_EQ(plan[1].strategy, "Stufe2_Hybrid"); // R6 (§59-A(2)): node_type=merge => Hybrid (NICHT FullJoin)
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
//     => der emittierte Quelltext traegt eine REALE MergeAxis<MergeStrategy::..>-Instanziierung.
//     directive_slot_types loest den realen (default, slot)-Typ auf (prt_art_merge_reference.hpp).
TEST(MergePlanDirective, DirectivePathEmitsRealMergeAxisInstantiation) {
    std::vector<tlz::AxisMergeDirective> const directives{
        tlz::AxisMergeDirective{"path_compression", "Stufe2_PrueflingReplace", "prt_art", {"prt_patricia"}}};
    std::string const src = tlz::render_directive_merge_module_source(
        "::comdare::cache_engine::compositions::HotComposition", "compositions/hot_reference.hpp", directives);

    // Reale MergeAxis<>-Instanziierung ueber die directive-Achse (generalisiert, NICHT hart path_compression im Code).
    EXPECT_NE(src.find("pf::MergeAxis<pf::MergeStrategy::Stufe2_PrueflingReplace,"), std::string::npos)
        << "Direktiven-Pfad ohne MergeAxis-Instanziierung:\n"
        << src;
    EXPECT_NE(src.find("PrtArtPathCompressionSlot"), std::string::npos) << "realer Pruefling-Slot fehlt";
    EXPECT_NE(src.find("DirectiveMerged_path_compression"), std::string::npos);
    EXPECT_NE(src.find("COMDARE_DEFINE_ANATOMY_MODULE("), std::string::npos);
    // A13-M3/C3 (Owner-E2): die Merge-DURCHFUEHRUNG steht oben, aber KEINE Merge-Stempel-Zeile mehr -- die
    // gibt es nicht mehr, und ohne gereichte Stempel-Zeilen emittiert der Pfad gar kein Stempel-Makro.
    EXPECT_EQ(src.find("COMDARE_ANATOMY_VERSION_STAMP"), std::string::npos)
        << "ohne gereichte Stempel-Zeilen darf KEIN Stempel-Makro emittiert werden:\n"
        << src;

    // directive_slot_types: reale Aufloesung fuer path_compression/prt_art, nullopt sonst (ehrlich).
    EXPECT_TRUE(tlz::directive_slot_types("path_compression", "prt_art").has_value());
    EXPECT_FALSE(tlz::directive_slot_types("node_type", "unknown_pruefling").has_value());
}

// (c) Byte-Additivitaet: render_sota_module_source OHNE Stempel == heutiger Katalog-Quelltext; mit Stempel
//     haengt es NUR die Stempel-Zeile an (append-only; der Rest byte-identisch).
TEST(MergePlanDirective, SotaSourceStampIsAppendOnly) {
    std::string const fq       = "::comdare::cache_engine::compositions::HotComposition";
    std::string const header   = "compositions/hot_reference.hpp";
    std::string const catalog  = tlz::render_sota_module_source(fq, header); // Default: leerer Stempel
    std::string const catalog2 = tlz::render_sota_module_source(fq, header, tlz::SotaStampLines{});
    EXPECT_EQ(catalog, catalog2) << "leerer SotaStampLines != Default => nicht byte-identisch";
    EXPECT_EQ(catalog.find("COMDARE_ANATOMY_VERSION_STAMP"), std::string::npos)
        << "ce-only/Katalog-Pfad darf KEINE Stempel-Zeile tragen (byte-identisch)";

    tlz::SotaStampLines const stamp{"", "target_isa=code@1.0.0c", ""};
    std::string const         with_stamp = tlz::render_sota_module_source(fq, header, stamp);
    // Append-only: der Katalog-Quelltext ist ein exaktes Praefix des Stempel-Quelltexts.
    EXPECT_EQ(with_stamp.rfind(catalog, 0), 0u) << "Stempel ist NICHT append-only (Katalog-Praefix gebrochen)";
    EXPECT_NE(with_stamp.find("COMDARE_ANATOMY_VERSION_STAMP_M("), std::string::npos);
}

// (d) A13-M3/C1 (K-3): die VOLLEN Stempel-Zeilen reisen ueber die _M-Vollform. Vor C1 emittierte der SOTA-Pfad
//     GAR KEINEN Stempel -- jede SOTA-Binary trug damit denselben Leer-Fingerprint und war fuer das
//     SHA512-Skip-Gate identitaetslos.
TEST(MergePlanDirective, C1SotaQuelleTraegtVolleStempelZeilen) {
    std::string const fq     = "::comdare::cache_engine::compositions::HotComposition";
    std::string const header = "compositions/hot_reference.hpp";

    // (d1) Der stempel-lose Default bleibt exakt die heutige Emission (Byte-Additivitaet der neuen Naht).
    EXPECT_EQ(tlz::render_sota_module_source(fq, header).find("COMDARE_ANATOMY_VERSION_STAMP"), std::string::npos)
        << "der Default-Aufruf darf KEINE Stempel-Zeile emittieren (byte-identisch zum Katalog-Stand)";

    // (d2) Die Makro-SLOT-Ordnung ist organ, system, measurement -- literal gepinnt, damit kein Slot still
    //      verrutscht. A13-M3/C3: der vierte (merge-)Slot ist ersatzlos entfallen.
    tlz::SotaStampLines const stamp{"search_algo=k_ary@1.0.0c", "target_isa=code@1.0.0c",
                                    "measurement_tooling=wallclock@1.0.0c"};
    std::string const         with_stamp = tlz::render_sota_module_source(fq, header, stamp);
    EXPECT_NE(with_stamp.find("COMDARE_ANATOMY_VERSION_STAMP_M(\"search_algo=k_ary@1.0.0c\", "
                              "\"target_isa=code@1.0.0c\", \"measurement_tooling=wallclock@1.0.0c\")"),
              std::string::npos)
        << with_stamp;

    // (d3) Auch der direktiven-getriebene Zwilling reicht dieselben Zeilen durch (KEIN zweiter Emitter-Stand).
    std::vector<tlz::AxisMergeDirective> const directives{
        tlz::AxisMergeDirective{"path_compression", "Stufe2_PrueflingReplace", "prt_art", {"prt_patricia"}}};
    std::string const directive_src = tlz::render_directive_merge_module_source(fq, header, directives, stamp);
    EXPECT_NE(directive_src.find("COMDARE_ANATOMY_VERSION_STAMP_M(\"search_algo=k_ary@1.0.0c\", "
                                 "\"target_isa=code@1.0.0c\", \"measurement_tooling=wallclock@1.0.0c\")"),
              std::string::npos)
        << directive_src;

    // (d4) Der PRODUKTIONS-Pfad: die reale System-Zeile + die gereichte Mess-Zeile stehen im emittierten
    //      Quelltext. Die ORGAN-Zeile bleibt EHRLICH LEER -- der SOTA-binary_id ("sota_tier=sota::A::...") ist
    //      kein 17-Achsen-Pfad, ceb_parse_path liefert keine Achsen-Paare, und die SOTA-Kompositions-Slots
    //      tragen kein name()/algo_version (Metadaten-BLOCKER, abi/anatomy_version_stamp.hpp-Kopf). Das ist der
    //      BENANNTE K-3-Rest; er wird hier festgeschrieben, damit er nicht als stiller Ausfall durchgeht.
    std::string const meas = "measurement_tooling=wallclock@1.0.0c";
    auto const        by_id =
        tlz::build_sota_view_source_map(std::vector<tlz::SotaMergeLebewesen>{{"Stufe1_CeOnly", "hot"}}, meas);
    ASSERT_EQ(by_id.size(), 1u);
    std::string const& src = by_id.begin()->second;
    EXPECT_FALSE(cea::system_stamp_line().empty()) << "die System-Zeile ist die reale Identitaets-Quelle von C1";
    EXPECT_NE(src.find("COMDARE_ANATOMY_VERSION_STAMP_M(\"\", \"" + cea::system_stamp_line() + "\", \"" + meas + "\")"),
              std::string::npos)
        << src;
}

} // namespace
