// test_experiment_kern_seam -- KERN #48-S4 (2026-07-22): ctest-Gate fuer die Minimal-Parser-/Validator-Naht der
// vier neuen comdare_experiment-v2-Elemente (Section 59/62). ADDITIV: beruehrt KEINEN bestehenden Parser-/
// Projection-/Director-Test-Pin. Fixture = tests/unit/thesis_tiere/experiment_kern_seam_fixture.xml (ce-lokal,
// valide; bis zur Weg-a-Umbenennung am 17.08.2026 hiess sie experiment_golden_kern.xml wie die super-Instanz).
//
// BEWEIST LITERAL (R4: kein Element ohne Test-Beleg):
//   (a) PARSE   -- <machines> (Kern-Identitaet), <axis pruefling=..>, merge="union", <output><storage ..> parsen
//                  in die neuen PODs mit den woertlich erwarteten Feldern.
//   (b) VALIDATE OK -- die valide Fixture => ok==true, mit den neuen Zaehlern (machines_checked==2,
//                  storage_checked==1, axis_pruefling_checked==3).
//   (c) VALIDATE FEHLER -- je neuer Regel ein negativer Beleg: union ohne Phase-3-Bindung; unvollstaendige
//                  Maschinen-Identitaet; ungueltiges storage-backend; unbekannter axis-pruefling.
//   (d) merge_mode_to_strategy("union") == "Verbund3_Union" (Projektion nachgezogen, additiv).
//
// LESE-Schicht: kein Treiber-Lauf, kein DLL-Bau. Die Fixture wird NUR GELESEN.

#include "merge_plan.hpp"                          // merge_mode_to_strategy
#include "validate_profile.hpp"                    // validate_experiment_profile / ExperimentValidationResult
#include "xml_config_parser/xml_config_parser.hpp" // XmlConfigParser / ExperimentProfile

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#ifndef COMDARE_EXPERIMENT_KERN_SEAM_FIXTURE
#error "COMDARE_EXPERIMENT_KERN_SEAM_FIXTURE must point to tests/unit/thesis_tiere/experiment_kern_seam_fixture.xml"
#endif

namespace {

namespace cx  = comdare::builder::xml;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace fs  = std::filesystem;

std::optional<cx::ExperimentProfile> parse_kern() {
    cx::XmlConfigParser const parser;
    return parser.parse_experiment_profile(fs::path{COMDARE_EXPERIMENT_KERN_SEAM_FIXTURE});
}

// Ein minimal-valides ExperimentProfile (2 engines + op_types + Verbund1-Phase): die Basis fuer die negativen
// Regel-Belege, damit GENAU die getestete Regel den Fehler traegt (kein Rauschen aus anderen Pflicht-Checks).
cx::ExperimentProfile make_base_ep() {
    cx::ExperimentProfile ep;
    ep.engines.push_back(
        cx::ExperimentEngine{"ee_ce", "CacheEngineExecutionEngineAdapter", "cache_engine_axis_registry.xml"});
    ep.engines.push_back(cx::ExperimentEngine{"ee_prt", "PrtArtExecutionEngineAdapter", "prt_art_axis_registry.xml"});
    ep.op_types = {"OP-1"};
    cx::ExperimentPhase ph1;
    ph1.name  = "Verbund1_CeOnly";
    ph1.merge = "Verbund1_CeOnly";
    ep.phases.push_back(ph1);
    return ep;
}

bool has_error_containing(tlz::ExperimentValidationResult const& r, std::string const& frag) {
    for (auto const& e : r.errors)
        if (e.find(frag) != std::string::npos) return true;
    return false;
}

bool has_warning_containing(tlz::ExperimentValidationResult const& r, std::string const& frag) {
    for (auto const& w : r.warnings)
        if (w.find(frag) != std::string::npos) return true;
    return false;
}

// O-8 Schritt 5 (O-4b): schreibt ein Minimal-<comdare_experiment> mit frei waehlbarem <machines>-Block in eine
// Temp-Datei und parst es. Beweist die Parser-Naht der neuen target_isa-Glieder end-to-end, OHNE die
// Golden-Instanz anzufassen (golden-neutral; Muster test_experiment_parser.cpp:80-96).
std::optional<cx::ExperimentProfile> parse_experiment_with_machines(std::string const& machines_block) {
    fs::path const p = fs::temp_directory_path() /
                       ("comdare_o4b_machines_" +
                        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".xml");
    {
        std::ofstream out{p};
        out << R"(<?xml version="1.0" encoding="UTF-8"?>
<comdare_experiment version="1" id="o4b_machines_fixture">
)" << machines_block
            << "</comdare_experiment>\n";
    }
    cx::XmlConfigParser const            parser;
    std::optional<cx::ExperimentProfile> ep = parser.parse_experiment_profile(p);
    std::error_code                      ec;
    fs::remove(p, ec);
    return ep;
}

// ── (a) PARSE ────────────────────────────────────────────────────────────────
TEST(ExperimentKernSeam, ParsesMachinesWithCoreIdentity) {
    auto ep = parse_kern();
    ASSERT_TRUE(ep.has_value());
    ASSERT_EQ(ep->machines.size(), 2u);
    EXPECT_EQ(ep->machines[0].id, "prod1");
    EXPECT_EQ(ep->machines[0].cpu_fabrication, "amd_zen5_avx512");
    EXPECT_EQ(ep->machines[0].ram_pair, "ddr5_2x32");
    EXPECT_EQ(ep->machines[0].hostname_hint, "prod1");
    EXPECT_EQ(ep->machines[1].id, "prod2");
    EXPECT_EQ(ep->machines[1].cpu_fabrication, "intel_avx2");

    // O-8 Schritt 5 (O-4b): die Fixture deklariert die zwei RAM-seitigen target_isa-Glieder NICHT. Sie
    // muessen deshalb auf 0 stehen -- das ist die NICHT-DEKLARIERT-Marke, nicht "0 MHz". Diese Zusicherung
    // ist der Byte-Identitaets-Beleg des Schrittes: ein Bestands-Profil ohne die neuen Attribute verhaelt
    // sich exakt wie vorher.
    EXPECT_EQ(ep->machines[0].ram_frequency_mhz, 0);
    EXPECT_EQ(ep->machines[0].cas_latency_cl, 0);
    EXPECT_EQ(ep->machines[1].ram_frequency_mhz, 0);
    EXPECT_EQ(ep->machines[1].cas_latency_cl, 0);
}

// O-8 Schritt 5 (O-4b): die Gegenrichtung -- deklarierte Glieder kommen woertlich als Zahlen an. Ohne diesen
// Beleg waere die 0-Zusicherung oben auch dann erfuellt, wenn der Leser die Attribute gar nicht liest.
TEST(ExperimentKernSeam, ParsesDeclaredTargetIsaMemberValues) {
    auto const ep = parse_experiment_with_machines(
        R"(  <machines>
    <machine id="m_voll" cpu_fabrication="fab_a" ram_pair="ddr5_2x32" ram_frequency_mhz="5600" cas_latency_cl="36"/>
    <machine id="m_teil" cpu_fabrication="fab_b" ram_pair="ddr4_2x32" ram_frequency_mhz="3200"/>
  </machines>
)");
    ASSERT_TRUE(ep.has_value());
    ASSERT_EQ(ep->machines.size(), 2u);
    EXPECT_EQ(ep->machines[0].ram_frequency_mhz, 5600);
    EXPECT_EQ(ep->machines[0].cas_latency_cl, 36);
    // Teil-Deklaration ist erlaubt und bleibt ehrlich: das fehlende Glied ist 0, nicht geraten.
    EXPECT_EQ(ep->machines[1].ram_frequency_mhz, 3200);
    EXPECT_EQ(ep->machines[1].cas_latency_cl, 0);
}

// A14/OS-U2 (Owner-Entscheid E3, 02.08.2026): die drei operating_system-Unter-Achsen als ERWARTUNGS-
// Attribute am <machine>. Bestands-Profile ohne die Attribute bleiben byte-identisch (leer = nicht
// deklariert); deklarierte Werte kommen woertlich an. Ohne diesen zweiten Beleg waere die Leer-
// Zusicherung auch dann erfuellt, wenn der Leser die Attribute gar nicht liest.
TEST(ExperimentKernSeam, ParsesDeclaredOsExpectationValues) {
    auto const ep = parse_experiment_with_machines(
        R"(  <machines>
    <machine id="m_os_voll" cpu_fabrication="fab_a" ram_pair="ddr5_2x32" os_version="debian-13" kernel="6.17.0-35-generic" build="13.1" os_declaration_source="handout infra 2026-08"/>
    <machine id="m_os_teil" cpu_fabrication="fab_b" ram_pair="ddr4_2x32" os_version="ubuntu-24.04"/>
  </machines>
)");
    ASSERT_TRUE(ep.has_value());
    ASSERT_EQ(ep->machines.size(), 2u);
    EXPECT_EQ(ep->machines[0].os_version, "debian-13");
    EXPECT_EQ(ep->machines[0].kernel, "6.17.0-35-generic");
    EXPECT_EQ(ep->machines[0].build, "13.1");
    EXPECT_EQ(ep->machines[0].os_declaration_source, "handout infra 2026-08");
    // Teil-Deklaration ist erlaubt und bleibt ehrlich: die fehlenden Werte sind LEER, nicht geraten.
    EXPECT_EQ(ep->machines[1].os_version, "ubuntu-24.04");
    EXPECT_TRUE(ep->machines[1].kernel.empty());
    EXPECT_TRUE(ep->machines[1].build.empty());
    EXPECT_TRUE(ep->machines[1].os_declaration_source.empty());
}

// A14/OS-U2: die Byte-Identitaets-Zusicherung. Die Kern-Naht-Fixture deklariert KEINE OS-Erwartung --
// alle vier Felder muessen leer bleiben. Ein Bestands-Profil ohne die neuen Attribute verhaelt sich
// exakt wie vorher; die Attribute sind ADDITIV und PASSIV (kein Validator-Zwang, keine Warnung).
TEST(ExperimentKernSeam, MachinesWithoutOsExpectationStayEmpty) {
    auto ep = parse_kern();
    ASSERT_TRUE(ep.has_value());
    ASSERT_EQ(ep->machines.size(), 2u);
    for (auto const& mc : ep->machines) {
        EXPECT_TRUE(mc.os_version.empty()) << mc.id;
        EXPECT_TRUE(mc.kernel.empty()) << mc.id;
        EXPECT_TRUE(mc.build.empty()) << mc.id;
        EXPECT_TRUE(mc.os_declaration_source.empty()) << mc.id;
    }
    // Die Kern-Identitaet ist davon unberuehrt -- validate zaehlt weiterhin beide Maschinen.
    // A2.5-g5 (Fix 15): merge="merge" der Fixture in-memory neutralisieren (Gate hat eigenen Beleg,
    // MergeModeMergeIsDeferredError) -- dieser Test misst die Maschinen-Zaehlung, nicht das Merge-Gate.
    ASSERT_EQ(ep->axes_default_lookup.size(), 3u);
    ep->axes_default_lookup[2].merge_mode    = "replace";
    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_TRUE(vr.ok) << (vr.errors.empty() ? "" : vr.errors.front());
    EXPECT_EQ(vr.machines_checked, 2u);
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
    EXPECT_EQ(ep->axes_default_lookup[1].merge_mode, "union");
    EXPECT_EQ(ep->axes_default_lookup[2].pruefling, "self");
    EXPECT_EQ(ep->axes_default_lookup[2].merge_mode, "merge");
}

TEST(ExperimentKernSeam, ParsesStorageSlot) {
    auto ep = parse_kern();
    ASSERT_TRUE(ep.has_value());
    EXPECT_EQ(ep->output.storage_backend, "minio");
    EXPECT_EQ(ep->output.storage_endpoint, "minio.comdare.local");
}

TEST(ExperimentKernSeam, ParsesPhaseIdNamespace) {
    auto ep = parse_kern();
    ASSERT_TRUE(ep.has_value());
    ASSERT_EQ(ep->phases.size(), 3u);
    EXPECT_TRUE(ep->phases[0].id_namespace.empty()); // Verbund1_CeOnly: kein eigener id-Satz
    EXPECT_EQ(ep->phases[1].id_namespace, "merge_prt_v1");
    EXPECT_EQ(ep->phases[2].id_namespace, "join_prt_v1");
}

// ── (b) VALIDATE OK ──────────────────────────────────────────────────────────
TEST(ExperimentKernSeam, ValidatesOkWithNewCounters) {
    auto ep = parse_kern();
    ASSERT_TRUE(ep.has_value());
    // A2.5-g5 (Review #15, Fix 15): die Fixture traegt <axis merge="merge"> -- der Modus ist bis zur
    // Materialisierung von Verbund2_Hybrid am Validator GESPERRT (eigener Negativ-Beleg unten,
    // MergeModeMergeIsDeferredError). Fuer den GRUENEN Zaehler-Beleg dieses Tests wird die dritte
    // Achse in-memory auf den replace-Default gedreht; axis_merge_checked bleibt 3 (drei nicht-leere
    // Modi: replace, union, replace).
    ASSERT_EQ(ep->axes_default_lookup.size(), 3u);
    ASSERT_EQ(ep->axes_default_lookup[2].merge_mode, "merge");
    ep->axes_default_lookup[2].merge_mode    = "replace";
    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_TRUE(vr.ok) << (vr.errors.empty() ? "" : vr.errors.front());
    EXPECT_TRUE(vr.errors.empty());
    EXPECT_EQ(vr.machines_checked, 2u);
    EXPECT_EQ(vr.storage_checked, 1u);
    EXPECT_EQ(vr.axis_pruefling_checked, 3u);
    EXPECT_EQ(vr.axis_merge_checked, 3u);
    EXPECT_EQ(vr.id_namespace_checked, 2u); // Verbund2 + Verbund3 tragen je einen eigenen id-Satz
}

// ── (c) VALIDATE FEHLER -- je neuer Regel ein negativer Beleg ─────────────────

// A2.5-g5 (Review #15, Fix 15 / D-F4b): "merge" ist bis zur Materialisierung von Verbund2_Hybrid am
// EINGANG gesperrt -- das PrueflingVerbundStrategy-Enum traegt den Wert nicht (MATERIALISIERUNG
// DEFERRED, merge_plan.hpp), eine durchgelassene "merge"-Direktive fiele erst im NACHGELAGERTEN Bau
// des emittierten .cpp mit kryptischem Namensfehler (fail-late). Der Validator ist die frueheste
// Stelle der Kette, darum faellt sie HIER. Die Kern-Fixture traegt genau so eine Achse (mapping/self).
TEST(ExperimentKernSeam, MergeModeMergeIsDeferredError) {
    auto ep = parse_kern();
    ASSERT_TRUE(ep.has_value());
    ASSERT_EQ(ep->axes_default_lookup.size(), 3u);
    ASSERT_EQ(ep->axes_default_lookup[2].merge_mode, "merge") << "Fixture-Vorbedingung (mapping/self)";
    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(*ep);
    EXPECT_FALSE(vr.ok) << "merge=\"merge\" muss bis zur Materialisierung am Validator fallen";
    EXPECT_TRUE(has_error_containing(vr, "nicht materialisiert"));
    EXPECT_TRUE(has_error_containing(vr, "Verbund2_Hybrid"));
    EXPECT_TRUE(has_error_containing(vr, "replace/union")) << "die Meldung nennt den Ausweg";
}

TEST(ExperimentKernSeam, FulljoinWithoutPhase3IsError) {
    cx::ExperimentProfile     ep = make_base_ep(); // nur Verbund1-Phase => keine Phase-3-Bindung
    cx::ExperimentAxisDefault ax;
    ax.ref        = "path_compression";
    ax.merge_mode = "union";
    ep.axes_default_lookup.push_back(ax);
    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(ep);
    EXPECT_FALSE(vr.ok);
    EXPECT_TRUE(has_error_containing(vr, "union"));
    EXPECT_TRUE(has_error_containing(vr, "Phase-3-Bindung"));
}

TEST(ExperimentKernSeam, FulljoinWithPhase3IsAccepted) {
    cx::ExperimentProfile ep = make_base_ep();
    cx::ExperimentPhase   ph3;
    ph3.name  = "Verbund3_Union";
    ph3.merge = "Verbund3_Union";
    ep.phases.push_back(ph3); // jetzt liegt eine Phase-3-Bindung vor
    cx::ExperimentAxisDefault ax;
    ax.ref        = "path_compression";
    ax.merge_mode = "union";
    ep.axes_default_lookup.push_back(ax);
    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(ep);
    // Der union-Bindungs-Check darf hier NICHT feuern (die Union-Phase existiert).
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

TEST(ExperimentKernSeam, IdNamespaceWithoutPrueflingWarns) {
    cx::ExperimentProfile ep                 = make_base_ep();  // Verbund1_CeOnly ohne pruefling
    ep.phases[0].id_namespace                = "orphan_id_set"; // id-Satz auf pruefling-loser Phase
    tlz::ExperimentValidationResult const vr = tlz::validate_experiment_profile(ep);
    EXPECT_TRUE(vr.ok); // Warnung, KEIN Fehler
    EXPECT_EQ(vr.id_namespace_checked, 1u);
    EXPECT_TRUE(has_warning_containing(vr, "ohne pruefling"));
}

// ── (d) merge_plan-Projektion nachgezogen ────────────────────────────────────
TEST(ExperimentKernSeam, UnionProjectsToVerbund3UnionStrategy) {
    EXPECT_EQ(tlz::merge_mode_to_strategy("union"), "Verbund3_Union");  // §59-A(3): kombinierte Union
    EXPECT_EQ(tlz::merge_mode_to_strategy("merge"), "Verbund2_Hybrid"); // R6/§59-A(2): Hybrid, NICHT Union
    EXPECT_EQ(tlz::merge_mode_to_strategy("replace"), "Verbund2_Replace");
}

} // namespace
