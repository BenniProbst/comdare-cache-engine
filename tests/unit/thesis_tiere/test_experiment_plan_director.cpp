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
