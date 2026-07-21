// test_experiment_kern_seam -- KERN #48-S4 (2026-07-22): ctest-Gate fuer die Minimal-Parser-/Validator-Naht der
// vier neuen comdare_experiment-v2-Elemente (Section 59/62). ADDITIV: beruehrt KEINEN bestehenden Parser-/
// Projection-/Director-Test-Pin. Fixture = tests/unit/thesis_tiere/experiment_golden_kern.xml (ce-lokal, valide).
//
// BEWEIST LITERAL (R4: kein Element ohne Test-Beleg):
//   (a) PARSE   -- <machines> (Kern-Identitaet), <axis pruefling=..>, merge="fulljoin", <output><storage ..> parsen
//                  in die neuen PODs mit den woertlich erwarteten Feldern.
//   (b) VALIDATE OK -- die valide Fixture => ok==true, mit den neuen Zaehlern (machines_checked==2,
//                  storage_checked==1, axis_pruefling_checked==3).
//   (c) VALIDATE FEHLER -- je neuer Regel ein negativer Beleg: fulljoin ohne Phase-3-Bindung; unvollstaendige
//                  Maschinen-Identitaet; ungueltiges storage-backend; unbekannter axis-pruefling.
//   (d) merge_mode_to_strategy("fulljoin") == "Stufe3_FullJoin" (Projektion nachgezogen, additiv).
//
// LESE-Schicht: kein Treiber-Lauf, kein DLL-Bau. Die Fixture wird NUR GELESEN.

#include "merge_plan.hpp"                          // merge_mode_to_strategy
#include "validate_profile.hpp"                    // validate_experiment_profile / ExperimentValidationResult
#include "xml_config_parser/xml_config_parser.hpp" // XmlConfigParser / ExperimentProfile

#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#ifndef COMDARE_EXPERIMENT_GOLDEN_KERN
#error "COMDARE_EXPERIMENT_GOLDEN_KERN must point to tests/unit/thesis_tiere/experiment_golden_kern.xml"
#endif

namespace {

namespace cx  = comdare::builder::xml;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace fs  = std::filesystem;

std::optional<cx::ExperimentProfile> parse_kern() {
    cx::XmlConfigParser const parser;
    return parser.parse_experiment_profile(fs::path{COMDARE_EXPERIMENT_GOLDEN_KERN});
}

// Ein minimal-valides ExperimentProfile (2 engines + op_types + Stufe1-Phase): die Basis fuer die negativen
// Regel-Belege, damit GENAU die getestete Regel den Fehler traegt (kein Rauschen aus anderen Pflicht-Checks).
cx::ExperimentProfile make_base_ep() {
    cx::ExperimentProfile ep;
    ep.engines.push_back(
        cx::ExperimentEngine{"ee_ce", "CacheEngineExecutionEngineAdapter", "cache_engine_axis_registry.xml"});
    ep.engines.push_back(cx::ExperimentEngine{"ee_prt", "PrtArtExecutionEngineAdapter", "prt_art_axis_registry.xml"});
    ep.op_types = {"OP-1"};
    cx::ExperimentPhase ph1;
    ph1.name  = "Stufe1_CeOnly";
    ph1.merge = "Stufe1_CeOnly";
    ep.phases.push_back(ph1);
    return ep;
}

bool has_error_containing(tlz::ExperimentValidationResult const& r, std::string const& frag) {
    for (auto const& e : r.errors)
        if (e.find(frag) != std::string::npos) return true;
    return false;
}

// ── (a) PARSE ────────────────────────────────────────────────────────────────
TEST(ExperimentKernSeam, ParsesMachinesWithCoreIdentity) {
    auto ep = parse_kern();
    ASSERT_TRUE(ep.has_value());
    ASSERT_EQ(ep->machines.size(), 2u);
    EXPECT_EQ(ep->machines[0].id, "prod1");
    EXPECT_EQ(ep->machines[0].cpu_fabrication, "amd_zen4_avx512");
    EXPECT_EQ(ep->machines[0].ram_pair, "ddr5_2x32");
    EXPECT_EQ(ep->machines[0].hostname_hint, "prod1");
    EXPECT_EQ(ep->machines[1].id, "prod2");
    EXPECT_EQ(ep->machines[1].cpu_fabrication, "intel_avx2");
}

TEST(ExperimentKernSeam, ParsesAxisPrueflingAndFulljoinToken) {
    auto ep = parse_kern();
    ASSERT_TRUE(ep.has_value());
    ASSERT_EQ(ep->axes_default_lookup.size(), 3u);
    EXPECT_EQ(ep->axes_default_lookup[0].ref, "search_algo");
    EXPECT_EQ(ep->axes_default_lookup[0].pruefling, "ee_prt");
    EXPECT_EQ(ep->axes_default_lookup[0].merge_mode, "replace");
    EXPECT_EQ(ep->axes_default_lookup[1].ref, "path_compression");
    EXPECT_EQ(ep->axes_default_lookup[1].pruefling, "prt_art");
    EXPECT_EQ(ep->axes_default_lookup[1].merge_mode, "fulljoin");
    EXPECT_EQ(ep->axes_default_lookup[2].pruefling, "self");
    EXPECT_EQ(ep->axes_default_lookup[2].merge_mode, "merge");
}

TEST(ExperimentKernSeam, ParsesStorageSlot) {
    auto ep = parse_kern();
    ASSERT_TRUE(ep.has_value());
    EXPECT_EQ(ep->output.storage_backend, "minio");
    EXPECT_EQ(ep->output.storage_endpoint, "minio.comdare.local");
}

// ── (b) VALIDATE OK ──────────────────────────────────────────────────────────
TEST(ExperimentKernSeam, ValidatesOkWithNewCounters) {
    auto ep = parse_kern();
    ASSERT_TRUE(ep.has_value());
    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_TRUE(vr.ok) << (vr.errors.empty() ? "" : vr.errors.front());
    EXPECT_TRUE(vr.errors.empty());
    EXPECT_EQ(vr.machines_checked, 2u);
    EXPECT_EQ(vr.storage_checked, 1u);
    EXPECT_EQ(vr.axis_pruefling_checked, 3u);
    EXPECT_EQ(vr.axis_merge_checked, 3u);
}

// ── (c) VALIDATE FEHLER -- je neuer Regel ein negativer Beleg ─────────────────
TEST(ExperimentKernSeam, FulljoinWithoutPhase3IsError) {
    cx::ExperimentProfile     ep = make_base_ep(); // nur Stufe1-Phase => keine Phase-3-Bindung
    cx::ExperimentAxisDefault ax;
    ax.ref        = "path_compression";
    ax.merge_mode = "fulljoin";
    ep.axes_default_lookup.push_back(ax);
    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(has_error_containing(vr, "fulljoin"));
    EXPECT_TRUE(has_error_containing(vr, "Phase-3-Bindung"));
}

TEST(ExperimentKernSeam, FulljoinWithPhase3IsAccepted) {
    cx::ExperimentProfile ep = make_base_ep();
    cx::ExperimentPhase   ph3;
    ph3.name  = "Stufe3_FullJoin";
    ph3.merge = "Stufe3_FullJoin";
    ep.phases.push_back(ph3); // jetzt liegt eine Phase-3-Bindung vor
    cx::ExperimentAxisDefault ax;
    ax.ref        = "path_compression";
    ax.merge_mode = "fulljoin";
    ep.axes_default_lookup.push_back(ax);
    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(ep);
    // Der fulljoin-Bindungs-Check darf hier NICHT feuern (die Union-Phase existiert).
    EXPECT_FALSE(has_error_containing(vr, "Phase-3-Bindung"));
}

TEST(ExperimentKernSeam, IncompleteMachineIdentityIsError) {
    cx::ExperimentProfile ep = make_base_ep();
    ep.machines.push_back(cx::ExperimentMachine{"prod1", "", "ddr5_2x32", "prod1"}); // cpu_fabrication leer
    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(has_error_containing(vr, "Maschinen-Identitaet"));
}

TEST(ExperimentKernSeam, InvalidStorageBackendIsError) {
    cx::ExperimentProfile ep                 = make_base_ep();
    ep.output.storage_backend                = "s3"; // kein {local,minio}
    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(has_error_containing(vr, "storage backend=\"s3\""));
}

TEST(ExperimentKernSeam, UnknownAxisPrueflingIsError) {
    cx::ExperimentProfile     ep = make_base_ep();
    cx::ExperimentAxisDefault ax;
    ax.ref       = "search_algo";
    ax.pruefling = "does_not_exist"; // keine engine-id / lebewesen-id / self-Marker
    ep.axes_default_lookup.push_back(ax);
    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(has_error_containing(vr, "UNBEKANNTER Pruefling"));
}

// ── (d) merge_plan-Projektion nachgezogen ────────────────────────────────────
TEST(ExperimentKernSeam, FulljoinProjectsToFullJoinStrategy) {
    EXPECT_EQ(tlz::merge_mode_to_strategy("fulljoin"), "Stufe3_FullJoin");
    EXPECT_EQ(tlz::merge_mode_to_strategy("merge"), "Stufe3_FullJoin");
    EXPECT_EQ(tlz::merge_mode_to_strategy("replace"), "Stufe2_PrueflingReplace");
}

} // namespace
