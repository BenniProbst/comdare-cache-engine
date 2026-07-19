// test_experiment_plan_director — PAKET W3-B / Phase-1-I1 Contract-Test (2026-07-19).
//
// Beweist LITERAL, dass der ExperimentPlanDirector (GoF Director + Builder, planner/experiment_plan_director.hpp)
// EINEN deterministischen Walk opt x simd x (Thesis: Sweep-Passes / Experiment: Phasen) faehrt, indem er NUR die
// BESTEHENDEN Zerlegungs-Bausteine orchestriert (profile_sweep_passes / project_experiment_to_sota_passes /
// die geteilten system_axis_*-Naht + XML-System-Achsen-Listen). REINER Enumerations-/Render-Schritt — KEIN Bau,
// KEINE Messung, KEIN run_lazy_static_then_dynamic, KEINE CSV (Anti-Phantom, golden-neutral).
//
// GEPRUEFT:
//   (T) RegistryTrio: die 3 Art-Registries (Organ/System/Mess) laden ueber DIESELBE read_axis_registry-Naht —
//       17 Organ-Achsen / 5 System-Achsen / 16 Mess-Kategorien (die "17/5/16"-Zaehlung der 3 XMLs).
//   (A) Thesis-min (keine <system_axes>, keine <axis_sweeps>): GENAU 1 opt x simd Identitaets-Perm (O3/no_extension)
//       x 1 Basis-Sweep-Pass = 1 Plan-Schritt.
//   (B) all_axes_golden (system_axes O2/O3 x no_extension/avx2 + 17 <axis_sweep>): GENAU 2x2 = 4 Perms x
//       (1 Basis + 17 Sweep) = 18 Sweep-Passes je Perm = 72 Schritte, in Dokument-Reihenfolge.
//   (C) Determinismus: zwei Laeufe erzeugen den BYTE-GLEICHEN --dump-plan-Text (Thesis + Experiment).
//   (D) Experiment-Kanal (experiment_golden.xml): je Perm die Bruecke-I3-Phasen-Enumeration (3 Phasen -> 19 Passes).
//   (E) Der Director ANNOTIERT seinen Plan-Kopf mit den 3 Angebots-Quellen (Resolver-Vorstufe).
//
// LESE-Schicht: alle Fixtures + Registries werden NUR GELESEN (per CMake-Define auf die realen/committeten Dateien).

#include "planner/experiment_plan_director.hpp" // ExperimentPlanDirector / IPlanBuilder / PlanTextBuilder / RegistryTrio-Annotation
#include "validate_profile.hpp"                 // read_axis_registry_trio / RegistryTrio
#include "xml_config_parser/xml_config_parser.hpp" // XmlConfigParser / ThesisProfile / ExperimentProfile

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#ifndef COMDARE_PLANNER_THESIS_MIN
#error "COMDARE_PLANNER_THESIS_MIN must point to tests/unit/thesis_tiere/planner_thesis_min.profile.xml"
#endif
#ifndef COMDARE_PLANNER_THESIS_ALL_AXES
#error "COMDARE_PLANNER_THESIS_ALL_AXES must point to thesis_profiles/all_axes_golden.profile.xml"
#endif
#ifndef COMDARE_EXPERIMENT_GOLDEN
#error "COMDARE_EXPERIMENT_GOLDEN must point to tests/unit/thesis_tiere/experiment_golden.xml"
#endif
#ifndef COMDARE_CE_AXIS_REGISTRY
#error "COMDARE_CE_AXIS_REGISTRY must point to cache_engine_axis_registry.xml"
#endif
#ifndef COMDARE_SYSTEM_AXIS_REGISTRY
#error "COMDARE_SYSTEM_AXIS_REGISTRY must point to system_axis_registry.xml"
#endif
#ifndef COMDARE_MEASUREMENT_AXIS_REGISTRY
#error "COMDARE_MEASUREMENT_AXIS_REGISTRY must point to measurement_axis_registry.xml"
#endif

namespace {

namespace cx      = comdare::builder::xml;
namespace tlz     = comdare::cache_engine::thesis_lazy;
namespace planner = comdare::cache_engine::planner;
namespace fs      = std::filesystem;

// CountingBuilder — ConcreteBuilder, der Perms + Schritte je Perm zaehlt (fuer die strukturellen Assertions).
struct CountingBuilder final : planner::IPlanBuilder {
    planner::PlanHeader                         header;
    std::vector<planner::PlanPerm>              perms;
    std::vector<std::vector<planner::PlanStep>> steps_per_perm; // parallel zu perms

    void begin_plan(planner::PlanHeader const& h) override { header = h; }
    void begin_perm(planner::PlanPerm const& p) override {
        perms.push_back(p);
        steps_per_perm.emplace_back();
    }
    void on_step(planner::PlanStep const& s) override { steps_per_perm.back().push_back(s); }
    void end_perm(planner::PlanPerm const&) override {}
    void end_plan(planner::PlanHeader const&) override {}

    [[nodiscard]] std::size_t total_steps() const {
        std::size_t n = 0;
        for (auto const& v : steps_per_perm) n += v.size();
        return n;
    }
};

std::optional<cx::ThesisProfile> parse_thesis(char const* path) {
    cx::XmlConfigParser const parser;
    return parser.parse_thesis_profile(fs::path{path});
}
std::optional<cx::ExperimentProfile> parse_experiment(char const* path) {
    cx::XmlConfigParser const parser;
    return parser.parse_experiment_profile(fs::path{path});
}

} // namespace

// (T) Die 3 Art-Registries laden ueber DIESELBE read_axis_registry-Naht — 17/5/16.
TEST(ExperimentPlanDirector, RegistryTrioLoadsThreeArtRegistriesWith17_5_16) {
    auto const trio =
        tlz::read_axis_registry_trio(fs::path{COMDARE_CE_AXIS_REGISTRY}, fs::path{COMDARE_SYSTEM_AXIS_REGISTRY},
                                     fs::path{COMDARE_MEASUREMENT_AXIS_REGISTRY});
    ASSERT_TRUE(trio.has_value()) << "alle 3 Art-Registries muessen als comdare_axis_registry lesbar sein";

    EXPECT_EQ(trio->organ.engine, "cache_engine");
    EXPECT_EQ(trio->system.engine, "cache_engine_system");
    EXPECT_EQ(trio->measurement.engine, "cache_engine_measurement");

    EXPECT_EQ(trio->organ_axis_count(), 17u) << "Organ-golden: 17 Kompositions-Achsen (isa raus, INC-2d)";
    EXPECT_EQ(trio->system_axis_count(), 5u)
        << "System: compiler/extension_hardware/target_isa/scheduling/load_framework";
    EXPECT_EQ(trio->measurement_category_count(), 16u) << "16 Mess-Kategorien (kMeasurementAxisRegistry)";
}

// (A) Thesis-min: 1 Identitaets-Perm (keine system_axes) x 1 Basis-Pass (keine axis_sweeps) = 1 Schritt.
TEST(ExperimentPlanDirector, ThesisMinYieldsSingleIdentityPermAndSinglePass) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_MIN);
    ASSERT_TRUE(tp.has_value()) << "planner_thesis_min.profile.xml nicht parsbar: " << COMDARE_PLANNER_THESIS_MIN;
    ASSERT_TRUE(tp->compiler.opt_levels.empty()) << "Fixture MUSS ohne <system_axes> sein";
    ASSERT_TRUE(tp->extension_hardware.simd_options.empty());

    planner::ExperimentPlanDirector const director;
    CountingBuilder                       cb;
    director.construct(*tp, cb);

    EXPECT_EQ(cb.header.source_kind, "thesis");
    EXPECT_EQ(cb.header.profile_id, "planner_thesis_min");
    ASSERT_EQ(cb.perms.size(), 1u) << "keine system_axes => EINE Identitaets-Perm (Vor-Wiring-Verhalten)";
    EXPECT_EQ(cb.header.perm_count, 1u);
    EXPECT_EQ(cb.perms[0].opt_id, "O3") << "CEB-Default opt (DefaultOptLevelOption)";
    EXPECT_EQ(cb.perms[0].simd_id, "no_extension") << "CEB-Default simd (DefaultSimdOption)";
    EXPECT_EQ(cb.perms[0].opt_flag, "-O3");
    EXPECT_EQ(cb.perms[0].march_flag, "");
    EXPECT_EQ(cb.perms[0].build_version_suffix, "+opt=O3") << "no_extension traegt kein +ext=";

    ASSERT_EQ(cb.steps_per_perm.size(), 1u);
    ASSERT_EQ(cb.steps_per_perm[0].size(), 1u) << "keine <axis_sweeps> => 1 Basis-Sweep-Pass";
    EXPECT_EQ(cb.steps_per_perm[0][0].kind, "thesis_sweep_pass");
    EXPECT_EQ(cb.steps_per_perm[0][0].label, "") << "Basis-Pass = leere Sweep-Achse";
    EXPECT_EQ(cb.total_steps(), 1u);
}

// (B) all_axes_golden: 2x2 = 4 Perms x (1 Basis + 17 Sweep) = 18 Sweep-Passes je Perm = 72 Schritte.
TEST(ExperimentPlanDirector, ThesisAllAxesGoldenYields2x2PermsTimesSweepPasses) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value()) << "all_axes_golden.profile.xml nicht parsbar";
    // Das committete Golden traegt opt O2/O3 x simd no_extension/avx2 + 17 <axis_sweep>.
    ASSERT_EQ(tp->compiler.opt_levels.size(), 2u);
    ASSERT_EQ(tp->extension_hardware.simd_options.size(), 2u);
    ASSERT_EQ(tp->axis_sweeps.size(), 17u);

    planner::ExperimentPlanDirector const director;
    CountingBuilder                       cb;
    director.construct(*tp, cb);

    ASSERT_EQ(cb.perms.size(), 4u) << "2 opt x 2 simd = 4 Perms";
    EXPECT_EQ(cb.header.perm_count, 4u);
    // Deterministische Perm-Reihenfolge (opt-aussen in Dokument-Reihenfolge, simd-innen).
    EXPECT_EQ(cb.perms[0].opt_id, "O2");
    EXPECT_EQ(cb.perms[0].simd_id, "no_extension");
    EXPECT_EQ(cb.perms[1].opt_id, "O2");
    EXPECT_EQ(cb.perms[1].simd_id, "avx2");
    EXPECT_EQ(cb.perms[1].march_flag, "-mavx2");
    EXPECT_EQ(cb.perms[1].build_version_suffix, "+opt=O2+ext=avx2");
    EXPECT_EQ(cb.perms[2].opt_id, "O3");
    EXPECT_EQ(cb.perms[3].opt_id, "O3");
    EXPECT_EQ(cb.perms[3].simd_id, "avx2");

    // Je Perm: 1 Basis + 17 Sweep = 18 Sweep-Passes; der erste ist der Basis-Pass (leeres label).
    for (auto const& steps : cb.steps_per_perm) {
        ASSERT_EQ(steps.size(), 18u) << "1 Basis + 17 <axis_sweep> Sweep-Passes";
        EXPECT_EQ(steps[0].label, "") << "Basis-Pass immer zuerst (#26/GO-5)";
        EXPECT_EQ(steps[1].label, "search_algo") << "erster deklarierter <axis_sweep> in Dokument-Reihenfolge";
    }
    EXPECT_EQ(cb.total_steps(), 72u) << "4 Perms x 18 Sweep-Passes";
}

// (C) Determinismus (Thesis): zwei Laeufe -> byte-gleicher --dump-plan-Text.
TEST(ExperimentPlanDirector, ThesisDumpPlanIsByteDeterministic) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::PlanTextBuilder              a;
    planner::PlanTextBuilder              b;
    director.construct(*tp, a);
    director.construct(*tp, b);

    EXPECT_EQ(a.text(), b.text()) << "der Director-Walk ist rein/deterministisch => byte-gleicher Plan-Text";
    EXPECT_FALSE(a.text().empty());
    EXPECT_NE(a.text().find("source_kind=thesis"), std::string::npos);
    EXPECT_NE(a.text().find("perm_count=4"), std::string::npos);
}

// (D) Experiment-Kanal: je Perm die Bruecke-I3-Phasen-Enumeration (3 Phasen -> 7+6+6 = 19 Passes).
TEST(ExperimentPlanDirector, ExperimentGoldenWalksPhasesUnderEachPerm) {
    auto const ep = parse_experiment(COMDARE_EXPERIMENT_GOLDEN);
    ASSERT_TRUE(ep.has_value()) << "experiment_golden.xml nicht parsbar: " << COMDARE_EXPERIMENT_GOLDEN;
    ASSERT_EQ(ep->phases.size(), 3u);
    // experiment_golden traegt dieselben System-Achsen wie all_axes_golden: 2 opt x 2 simd.
    ASSERT_EQ(ep->compiler.opt_levels.size(), 2u);
    ASSERT_EQ(ep->extension_hardware.simd_options.size(), 2u);

    planner::ExperimentPlanDirector const director;
    CountingBuilder                       cb;
    director.construct(*ep, cb);

    EXPECT_EQ(cb.header.source_kind, "experiment");
    ASSERT_EQ(cb.perms.size(), 4u) << "2x2 Perms";
    for (auto const& steps : cb.steps_per_perm) {
        ASSERT_EQ(steps.size(), 19u) << "3 Phasen: Stufe1=7 + Stufe2=6 + Stufe3=6 (prt_art degeneriert)";
        EXPECT_EQ(steps[0].kind, "experiment_phase_pass");
        EXPECT_EQ(steps[0].label, "phase2_cache_engine");
        EXPECT_EQ(steps[0].merge, "Stufe1_CeOnly");
        EXPECT_FALSE(steps[0].binary_id.empty());
        EXPECT_NE(steps[0].binary_id.find("sota_tier="), std::string::npos);
    }
    EXPECT_EQ(cb.total_steps(), 76u) << "4 Perms x 19 Passes";
}

// (C') Determinismus (Experiment): zwei Laeufe -> byte-gleicher Plan-Text.
TEST(ExperimentPlanDirector, ExperimentDumpPlanIsByteDeterministic) {
    auto const ep = parse_experiment(COMDARE_EXPERIMENT_GOLDEN);
    ASSERT_TRUE(ep.has_value());

    planner::ExperimentPlanDirector const director;
    planner::PlanTextBuilder              a;
    planner::PlanTextBuilder              b;
    director.construct(*ep, a);
    director.construct(*ep, b);

    EXPECT_EQ(a.text(), b.text());
    EXPECT_NE(a.text().find("source_kind=experiment"), std::string::npos);
}

// (E) Resolver-Vorstufe: der Director annotiert seinen Plan-Kopf mit den 3 Angebots-Quellen.
TEST(ExperimentPlanDirector, DirectorAnnotatesPlanHeaderWithRegistryTrio) {
    auto const trio =
        tlz::read_axis_registry_trio(fs::path{COMDARE_CE_AXIS_REGISTRY}, fs::path{COMDARE_SYSTEM_AXIS_REGISTRY},
                                     fs::path{COMDARE_MEASUREMENT_AXIS_REGISTRY});
    ASSERT_TRUE(trio.has_value());

    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_MIN);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director{planner::make_plan_registry_annotation(*trio)};
    CountingBuilder                       cb;
    director.construct(*tp, cb);

    EXPECT_TRUE(cb.header.registries.loaded) << "der Plan-Kopf traegt die 3 Angebots-Quellen";
    EXPECT_EQ(cb.header.registries.organ.engine, "cache_engine");
    EXPECT_EQ(cb.header.registries.organ.axis_count, 17u);
    EXPECT_EQ(cb.header.registries.system.engine, "cache_engine_system");
    EXPECT_EQ(cb.header.registries.system.axis_count, 5u);
    EXPECT_EQ(cb.header.registries.measurement.engine, "cache_engine_measurement");
    // Der --dump-plan-Text traegt die Annotation sichtbar (loaded=1 + die 3 engine-Namen).
    planner::PlanTextBuilder text;
    director.construct(*tp, text);
    EXPECT_NE(text.text().find("registry_trio loaded=1"), std::string::npos);
    EXPECT_NE(text.text().find("organ=cache_engine"), std::string::npos);
    EXPECT_NE(text.text().find("measurement=cache_engine_measurement"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// PAKET W5-B / I2 — CMakeGraphBuilder: zweiter ConcreteBuilder (GoF Builder) am SELBEN Director-Walk. Emittiert
// ein deterministisches experiment_plan.cmake (Bau-/Mess-Matrix als CMake-Graph). GEPRUEFT: (F/G) Topologie-
// Isomorphie zum Director-Walk (dieselbe Perm-Menge + Schritt-Reihenfolge wie ein CountingBuilder, den auch der
// PlanTextBuilder-Text spiegelt); (H) build:->measure:-Kanten je Perm + Aggregat-Target (Blaupausen-Treue
// add_custom_command/DEPENDS/VERBATIM); (I) Byte-Determinismus des .cmake-Textes (Thesis + Experiment).
// ─────────────────────────────────────────────────────────────────────────────

namespace {
std::size_t count_occurrences(std::string const& hay, std::string const& needle) {
    std::size_t n = 0, pos = 0;
    while ((pos = hay.find(needle, pos)) != std::string::npos) {
        ++n;
        pos += needle.size();
    }
    return n;
}
} // namespace

// (F) Topologie-Isomorphie (Thesis): CMakeGraphBuilder und ein CountingBuilder (Referenz-Struktur, die auch der
//     PlanTextBuilder-Text spiegelt) sehen ueber DASSELBE Profil dieselbe Perm-Menge + Schritt-Reihenfolge.
TEST(CMakeGraphBuilder, ThesisTopologyIsomorphicToDirectorWalk) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    CountingBuilder                       ref;
    planner::CMakeGraphBuilder            gb;
    director.construct(*tp, ref);
    director.construct(*tp, gb);

    ASSERT_EQ(gb.perms().size(), ref.perms.size()) << "gleiche Perm-Menge";
    ASSERT_EQ(gb.perms().size(), 4u) << "2 opt x 2 simd";
    for (std::size_t i = 0; i < ref.perms.size(); ++i) {
        EXPECT_EQ(gb.perms()[i].index, ref.perms[i].index);
        EXPECT_EQ(gb.perms()[i].opt_id, ref.perms[i].opt_id);
        EXPECT_EQ(gb.perms()[i].simd_id, ref.perms[i].simd_id);
        ASSERT_EQ(gb.steps_per_perm()[i].size(), ref.steps_per_perm[i].size()) << "gleiche Schritt-Zahl je Perm";
        for (std::size_t j = 0; j < ref.steps_per_perm[i].size(); ++j) {
            EXPECT_EQ(gb.steps_per_perm()[i][j].kind, ref.steps_per_perm[i][j].kind);
            EXPECT_EQ(gb.steps_per_perm()[i][j].label, ref.steps_per_perm[i][j].label) << "gleiche Reihenfolge";
        }
    }
    // Cross-Check zum PlanTextBuilder: dessen perm_count-Kopfzeile deckt sich mit der Perm-Menge.
    planner::PlanTextBuilder pt;
    director.construct(*tp, pt);
    EXPECT_NE(pt.text().find("perm_count=4"), std::string::npos);
}

// (G) Experiment-Kanal: dieselbe Isomorphie (3 Phasen -> 19 Passes je Perm -> 76 Schritte total).
TEST(CMakeGraphBuilder, ExperimentTopologyIsomorphicToDirectorWalk) {
    auto const ep = parse_experiment(COMDARE_EXPERIMENT_GOLDEN);
    ASSERT_TRUE(ep.has_value());

    planner::ExperimentPlanDirector const director;
    CountingBuilder                       ref;
    planner::CMakeGraphBuilder            gb;
    director.construct(*ep, ref);
    director.construct(*ep, gb);

    ASSERT_EQ(gb.perms().size(), ref.perms.size());
    std::size_t gb_total = 0, ref_total = 0;
    for (std::size_t i = 0; i < ref.perms.size(); ++i) {
        ASSERT_EQ(gb.steps_per_perm()[i].size(), ref.steps_per_perm[i].size());
        gb_total += gb.steps_per_perm()[i].size();
        ref_total += ref.steps_per_perm[i].size();
    }
    EXPECT_EQ(gb_total, ref_total);
    EXPECT_EQ(gb_total, 76u) << "4 Perms x 19 Passes";
}

// (H) build:->measure:-Kanten: N Perms => N build- + N measure-Targets + N build->measure-DEPENDS + 1 Aggregat.
TEST(CMakeGraphBuilder, EmitsPerPermBuildMeasureEdgesAndAggregate) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::CMakeGraphBuilder            gb;
    director.construct(*tp, gb);
    std::string const& cmake = gb.text();

    EXPECT_EQ(count_occurrences(cmake, "add_custom_target(comdare_experiment_plan_build_perm"), 4u);
    EXPECT_EQ(count_occurrences(cmake, "add_custom_target(comdare_experiment_plan_measure_perm"), 4u);
    EXPECT_EQ(count_occurrences(cmake, "# build:->measure:-Kante"), 4u) << "eine Bau->Mess-Kante je Perm";
    EXPECT_NE(cmake.find("add_custom_target(comdare_experiment_plan_all DEPENDS"), std::string::npos);
    // Blaupausen-Treue (catalog_codegen.cmake:27-37): add_custom_command + DEPENDS + VERBATIM.
    EXPECT_NE(cmake.find("add_custom_command("), std::string::npos);
    EXPECT_NE(cmake.find("VERBATIM)"), std::string::npos);
}

// (I) Byte-Determinismus des .cmake-Textes (Thesis + Experiment): zwei Laeufe -> byte-gleich.
TEST(CMakeGraphBuilder, CMakeTextIsByteDeterministic) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    auto const ep = parse_experiment(COMDARE_EXPERIMENT_GOLDEN);
    ASSERT_TRUE(ep.has_value());

    planner::ExperimentPlanDirector const director;
    {
        planner::CMakeGraphBuilder a;
        planner::CMakeGraphBuilder b;
        director.construct(*tp, a);
        director.construct(*tp, b);
        EXPECT_EQ(a.text(), b.text()) << "thesis .cmake byte-deterministisch";
        EXPECT_FALSE(a.text().empty());
    }
    {
        planner::CMakeGraphBuilder a;
        planner::CMakeGraphBuilder b;
        director.construct(*ep, a);
        director.construct(*ep, b);
        EXPECT_EQ(a.text(), b.text()) << "experiment .cmake byte-deterministisch";
    }
}

// (J) SCHARF (W7-B/§40.c): der build:-Schritt traegt echte provision-only-Treiber-Kommandos (kein No-Op mehr),
//     der measure:-Schritt ist GN-11-gegatet (kein Auto-Messlauf). Treiber/Profil/Range/Out = CMake-Variablen.
TEST(CMakeGraphBuilder, SharpenedBuildCommandIsProvisionOnlyAndMeasureIsGated) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::CMakeGraphBuilder            gb;
    director.construct(*tp, gb);
    std::string const& cmake = gb.text();

    // Echter Bau: je Perm ein cmake -E env ... $DRIVER experiment_config-Kommando mit provision-only. Der measure:-
    // Schritt nutzt -E echo (kein -E env) -> das aktive env-getriebene Bau-COMMAND zaehlt genau 1x je Perm.
    EXPECT_EQ(count_occurrences(cmake, "COMDARE_GOLDEN_N_PROVISION_ONLY=true"), 4u) << "je Perm ein provision-only-Bau";
    EXPECT_EQ(count_occurrences(cmake, "        COMMAND \"${CMAKE_COMMAND}\" -E env\n"), 4u)
        << "je Perm ein aktives env-getriebenes Treiber-Bau-COMMAND (measure nutzt -E echo)";
    EXPECT_NE(cmake.find("\"COMDARE_GN_OPT=O2\""), std::string::npos) << "opt/simd als Plan-Konstanten (LITERALE)";
    EXPECT_NE(cmake.find("\"COMDARE_GN_SIMD=avx2\""), std::string::npos);
    // Host-unabhaengig: konfigurierbare Eingaben als CMake-Variablen mit Defaults, KEINE Host-Absolutpfade.
    EXPECT_NE(cmake.find("if(NOT DEFINED COMDARE_PLAN_DRIVER)"), std::string::npos);
    EXPECT_NE(cmake.find("if(NOT DEFINED COMDARE_PLAN_RANGE)"), std::string::npos);
    EXPECT_EQ(cmake.find("/home/"), std::string::npos) << "keine emit-Zeit-Host-Absolutpfade im .cmake";
    // Messen bleibt GN-11-gegatet: der measure:-Schritt ist ein Skelett (kein aktiver COMDARE_RUN_MEASURE-Aufruf).
    EXPECT_NE(cmake.find("measure GATED (GN-11"), std::string::npos) << "measure:-Schritt ist gegatetes Skelett";
    EXPECT_EQ(cmake.find("COMMAND \"${CMAKE_COMMAND}\" -E env \"COMDARE_THESIS_PROFILE=${COMDARE_PLAN_PROFILE}\" "
                         "\"COMDARE_GN_OPT=O2\" \"COMDARE_GN_SIMD=no_extension\" COMDARE_RUN_MEASURE=true"),
              std::string::npos)
        << "das echte Mess-Kommando steht NUR als Kommentar-Skelett, nie als aktiver COMMAND";
}

// ─────────────────────────────────────────────────────────────────────────────
// PAKET W7-A / I3 (§40.b) — CiYamlBuilder: vierter ConcreteBuilder (GoF Builder) am SELBEN Director-Walk.
// Emittiert eine deterministische GitLab-Child-Pipeline-YAML (dynamische, Planer-gesteuerte Folge-CI). GEPRUEFT:
// (K) Topologie-Isomorphie zum Director-Walk (dieselbe Perm-Menge + Schritt-Reihenfolge wie ein CountingBuilder);
// (L) je Perm GENAU 1 ceb:build- + 1 Trigger-Job (STUFE 1 + STUFE 2b) + die zweistufige stages-Struktur;
// (M) Byte-Determinismus der YAML (Thesis + Experiment); (N) SIMD-Capability-Routing (no_extension->amd64, ...).
// ─────────────────────────────────────────────────────────────────────────────

// (K) Topologie-Isomorphie (Thesis): CiYamlBuilder und ein CountingBuilder sehen ueber DASSELBE Profil dieselbe
//     Perm-Menge + Schritt-Reihenfolge (strukturelle Synchronie zu PlanText/CMakeGraph).
TEST(CiYamlBuilder, ThesisTopologyIsomorphicToDirectorWalk) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    CountingBuilder                       ref;
    planner::CiYamlBuilder                yb;
    director.construct(*tp, ref);
    director.construct(*tp, yb);

    ASSERT_EQ(yb.perms().size(), ref.perms.size()) << "gleiche Perm-Menge";
    ASSERT_EQ(yb.perms().size(), 4u) << "2 opt x 2 simd";
    for (std::size_t i = 0; i < ref.perms.size(); ++i) {
        EXPECT_EQ(yb.perms()[i].index, ref.perms[i].index);
        EXPECT_EQ(yb.perms()[i].opt_id, ref.perms[i].opt_id);
        EXPECT_EQ(yb.perms()[i].simd_id, ref.perms[i].simd_id);
        ASSERT_EQ(yb.steps_per_perm()[i].size(), ref.steps_per_perm[i].size()) << "gleiche Schritt-Zahl je Perm";
        for (std::size_t j = 0; j < ref.steps_per_perm[i].size(); ++j) {
            EXPECT_EQ(yb.steps_per_perm()[i][j].kind, ref.steps_per_perm[i][j].kind);
            EXPECT_EQ(yb.steps_per_perm()[i][j].label, ref.steps_per_perm[i][j].label) << "gleiche Reihenfolge";
        }
    }
    // Cross-Check: dieselbe Perm-Menge, die auch der PlanTextBuilder-Text spiegelt.
    planner::PlanTextBuilder pt;
    director.construct(*tp, pt);
    EXPECT_NE(pt.text().find("perm_count=4"), std::string::npos);
}

// (G') Experiment-Kanal: dieselbe Isomorphie (4 Perms x 19 Passes = 76 Schritte).
TEST(CiYamlBuilder, ExperimentTopologyIsomorphicToDirectorWalk) {
    auto const ep = parse_experiment(COMDARE_EXPERIMENT_GOLDEN);
    ASSERT_TRUE(ep.has_value());

    planner::ExperimentPlanDirector const director;
    CountingBuilder                       ref;
    planner::CiYamlBuilder                yb;
    director.construct(*ep, ref);
    director.construct(*ep, yb);

    ASSERT_EQ(yb.perms().size(), ref.perms.size());
    std::size_t yb_total = 0, ref_total = 0;
    for (std::size_t i = 0; i < ref.perms.size(); ++i) {
        ASSERT_EQ(yb.steps_per_perm()[i].size(), ref.steps_per_perm[i].size());
        yb_total += yb.steps_per_perm()[i].size();
        ref_total += ref.steps_per_perm[i].size();
    }
    EXPECT_EQ(yb_total, ref_total);
    EXPECT_EQ(yb_total, 76u) << "4 Perms x 19 Passes";
}

// (L) Je Perm GENAU 1 ceb:build- + 1 Trigger-Job (+ 1 tier:emit-Helfer) + zweistufige stages-Struktur.
TEST(CiYamlBuilder, EmitsPerPermCebBuildAndTriggerJobsWithTwoStages) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::CiYamlBuilder                yb;
    director.construct(*tp, yb);
    std::string const& yaml = yb.text();

    // STUFE 1: genau 1 CEB-Bau-Job je Perm (Marker-Kommentar, kollisionsfrei zu needs:-Referenzen).
    EXPECT_EQ(count_occurrences(yaml, "# JOB ceb-build perm "), 4u) << "1 CEB-Bau-Job je Perm";
    // STUFE 2b: genau 1 Trigger-Job je Perm.
    EXPECT_EQ(count_occurrences(yaml, "# JOB tier-trigger perm "), 4u) << "1 Grandchild-Trigger-Job je Perm";
    EXPECT_EQ(count_occurrences(yaml, "# JOB tier-emit perm "), 4u) << "1 Tier-Emitter-Job je Perm (STUFE 2a)";
    // Jeder Trigger-Job traegt genau EINEN trigger:-Schluessel (Grandchild via include: artifact:).
    EXPECT_EQ(count_occurrences(yaml, "\n  trigger:\n"), 4u) << "je Perm ein trigger:-Schluessel";
    EXPECT_EQ(count_occurrences(yaml, "include:\n      - artifact:"), 4u)
        << "je Perm ein include: artifact:-Grandchild";
    // Zweistufige stages-Struktur (genau einmal, im Kopf).
    EXPECT_EQ(count_occurrences(yaml, "\nstages:\n"), 1u);
    EXPECT_NE(yaml.find("  - ceb-build\n"), std::string::npos);
    EXPECT_NE(yaml.find("  - tier-emit\n"), std::string::npos);
    // provision-only im STUFE-1-Skript (Pilot-Matrix-UMSCHALT-ZEILE), kein Messlauf.
    EXPECT_EQ(count_occurrences(yaml, "COMDARE_GOLDEN_N_PROVISION_ONLY=true"), 4u);
}

// (M) Byte-Determinismus der YAML (Thesis + Experiment): zwei Laeufe -> byte-gleich.
TEST(CiYamlBuilder, YamlIsByteDeterministic) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    auto const ep = parse_experiment(COMDARE_EXPERIMENT_GOLDEN);
    ASSERT_TRUE(ep.has_value());

    planner::ExperimentPlanDirector const director;
    {
        planner::CiYamlBuilder a, b;
        director.construct(*tp, a);
        director.construct(*tp, b);
        EXPECT_EQ(a.text(), b.text()) << "thesis child-pipeline YAML byte-deterministisch";
        EXPECT_FALSE(a.text().empty());
        EXPECT_EQ(a.text().find("/home/"), std::string::npos) << "keine emit-Zeit-Host-Absolutpfade in der YAML";
    }
    {
        planner::CiYamlBuilder a, b;
        director.construct(*ep, a);
        director.construct(*ep, b);
        EXPECT_EQ(a.text(), b.text()) << "experiment child-pipeline YAML byte-deterministisch";
    }
}

// (N) SIMD-Capability-Routing (Pilot-Matrix §36.3): no_extension->amd64, avx2->avx2, avx512->avx512.
TEST(CiYamlBuilder, SimdRunnerTagRoutingMatchesPilotMatrix) {
    EXPECT_EQ(planner::CiYamlBuilder::simd_runner_tag("no_extension"), "amd64");
    EXPECT_EQ(planner::CiYamlBuilder::simd_runner_tag(""), "amd64") << "leer = kein SIMD -> broadest amd64";
    EXPECT_EQ(planner::CiYamlBuilder::simd_runner_tag("avx2"), "avx2");
    EXPECT_EQ(planner::CiYamlBuilder::simd_runner_tag("avx512"), "avx512");

    // all_axes_golden traegt no_extension + avx2 -> die YAML routet auf beide Tags.
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::CiYamlBuilder                yb;
    director.construct(*tp, yb);
    EXPECT_NE(yb.text().find("tags: [\"amd64\"]"), std::string::npos);
    EXPECT_NE(yb.text().find("tags: [\"avx2\"]"), std::string::npos);
}
