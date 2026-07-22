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

#include "build_type_stamp.hpp"                 // (i) §61-STUFEN: build_type_version_suffix (facade-Naht, Byte-Wache)
#include "planner/experiment_plan_director.hpp" // ExperimentPlanDirector / IPlanBuilder / PlanTextBuilder / RegistryTrio-Annotation
#include "planner/plan_legend.hpp"              // W10-A: das dreistufige Legenden-Namensschema (Single-Source)
#include "validate_profile.hpp"                 // read_axis_registry_trio / RegistryTrio
#include "xml_config_parser/xml_config_parser.hpp" // XmlConfigParser / ThesisProfile / ExperimentProfile

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdlib> // (i) Byte-Wache: setenv/unsetenv (COMDARE_BUILD_TYPE)
#include <filesystem>
#include <optional>
#include <stdexcept> // (R5) EXPECT_THROW std::invalid_argument (exactly-one-Haertung)
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

// CountingBuilder — ConcreteBuilder, der Mess-Kombinationen + Perms + Schritte je Perm zaehlt (strukturelle
// Assertions). W10-A: die aeussere Mess-Achsen-Stufe wird ueber begin_measurement_combo mitgezaehlt.
struct CountingBuilder final : planner::IPlanBuilder {
    planner::PlanHeader                         header;
    std::vector<planner::PlanMeasurementCombo>  combos;
    std::vector<planner::PlanPerm>              perms;
    std::vector<std::vector<planner::PlanStep>> steps_per_perm; // parallel zu perms

    void begin_plan(planner::PlanHeader const& h) override { header = h; }
    void begin_measurement_combo(planner::PlanMeasurementCombo const& c) override { combos.push_back(c); }
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

    // S2/A2 P-SYSREG (2026-07-20): das System-ANGEBOT traegt genau die kanonischen Haupt-Achsen; target_isa ist
    // als EIGENE Haupt-Achse angeboten (INC-2d, 2 Bausteine x86_64/aarch64) und NUMA (7. Achse) ist korrekt
    // ABWESEND (=S11). atomic128 reist als sub_axis unter compiler (nicht als eigener axis-Key) und wird ueber
    // die Parser-/Validat-Consumer-Verdrahtung + den Byte-Roundtrip (test_system_axis_registry_roundtrip) gedeckt.
    EXPECT_EQ(trio->system.axis_names.count("compiler"), 1u);
    EXPECT_EQ(trio->system.axis_names.count("extension_hardware"), 1u);
    EXPECT_EQ(trio->system.axis_names.count("target_isa"), 1u) << "target_isa = eigene Haupt-System-Achse (INC-2d)";
    EXPECT_EQ(tlz::RegistryTrio::baustein_count(trio->system, "target_isa"), 2u) << "x86_64 + aarch64";
    EXPECT_EQ(trio->system.axis_names.count("numa"), 0u) << "NUMA (7. Achse) korrekt abwesend (=S11)";
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

// (B) all_axes_golden: S6-P1 faechert auf 3 Mess-Combos {wallclock/macro/micro} auf -> 3 x (2x2) = 12 Perms x
//     (1 Basis + 17 Sweep) = 18 Sweep-Passes je Perm = 216 Schritte. header.perm_count bleibt 4 (|opt x simd| JE Combo).
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

    ASSERT_EQ(cb.perms.size(), 12u) << "3 Mess-Combos x 2 opt x 2 simd = 12 Perms";
    EXPECT_EQ(cb.header.perm_count, 4u) << "perm_count = |opt x simd| JE Mess-Combo (unveraendert)";
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
    EXPECT_EQ(cb.total_steps(), 216u) << "12 Perms x 18 Sweep-Passes (3 Mess-Combos)";
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

// (D') S6-P1 (Fan-out SCHARF): die Fan-out-Verdrahtung in construct() ist aktiv (measurement_combos_of(cats,
//      ep.measurement_tooling)). Diese Golden-Instanz (experiment_golden.xml) traegt KEIN <measurement_tooling> ->
//      Default {} => 1 Combo [all] (byte-stabil zur heutigen 1-CEB-Strecke; NUR Profile MIT <measurement_tooling>
//      faechern auf). (Der Fan-out-KERN measurement_combos_of ist separat in MeasurementToolingFanOut getestet.)
TEST(ExperimentPlanDirector, MeasurementToolingStaysDefaultOneComboWhenUndeclared) {
    auto const ep = parse_experiment(COMDARE_EXPERIMENT_GOLDEN);
    ASSERT_TRUE(ep.has_value());
    EXPECT_TRUE(ep->measurement_tooling.empty()) << "Golden deklariert KEIN <measurement_tooling> (Default-Pfad)";

    planner::ExperimentPlanDirector const director;
    CountingBuilder                       cb;
    director.construct(*ep, cb);
    ASSERT_EQ(cb.combos.size(), 1u) << "ohne <measurement_tooling> => Default {} => 1 Combo [all] (byte-stabil)";
    EXPECT_EQ(cb.combos[0].legend, "[all]");
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
// (E2/E3) S3 P-RESOLVER (minimal-3-tief, 2026-07-20): mit dem VOLLEN RegistryTrio konstruiert, ruft der Director
// den Resolver (resolve_axis_refs_against_trio) auf den Organ-Position-Refs des Profils und traegt den
// klassifizierten ResolverReport in den Plan-Kopf (sichtbar im --dump-plan). INERT-by-default: OHNE volles Trio
// (Default-/Annotation-Konstruktor) bleibt der Resolver aus (resolved=0). binary_id-neutral -- reine Annotation.
// ─────────────────────────────────────────────────────────────────────────────

// (E2) POSITIV: organ-reines Thesis-Profil (planner_thesis_min: permute_axes = {search_algo}) -> 0 Rejects,
//      resolved=1/ok=1 im Plan-Kopf + im --dump-plan-Text. RegistryTrio 17/5/16 bleibt unveraendert.
TEST(ExperimentPlanDirector, ResolverOrganPureProfileZeroRejectsInPlanHead) {
    auto const trio =
        tlz::read_axis_registry_trio(fs::path{COMDARE_CE_AXIS_REGISTRY}, fs::path{COMDARE_SYSTEM_AXIS_REGISTRY},
                                     fs::path{COMDARE_MEASUREMENT_AXIS_REGISTRY});
    ASSERT_TRUE(trio.has_value());
    ASSERT_EQ(trio->organ_axis_count(), 17u) << "RegistryTrio 17/5/16 unveraendert gruen";
    ASSERT_EQ(trio->system_axis_count(), 5u);
    ASSERT_EQ(trio->measurement_category_count(), 16u);

    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_MIN);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director{*trio}; // VOLLES Trio => Resolver laeuft
    CountingBuilder                       cb;
    director.construct(*tp, cb);
    EXPECT_TRUE(cb.header.resolver.resolved) << "mit vollem Trio LIEF der Resolver";
    EXPECT_TRUE(cb.header.resolver.ok) << "organ-reines Profil => 0 Rejects";
    EXPECT_TRUE(cb.header.resolver.rejects.empty());

    planner::PlanTextBuilder text;
    director.construct(*tp, text);
    EXPECT_NE(text.text().find("resolver resolved=1 ok=1 rejects=0"), std::string::npos) << text.text();
    EXPECT_EQ(text.text().find("reject code="), std::string::npos) << "kein Reject im organ-reinen Profil";
}

// (E3) NEGATIV/ROUTE: eine SYSTEM-Achse (target_isa) in Organ-Position (permute_axes) -> V-CATEGORY-Reject mit
//      Koordinate im Plan-Kopf UND als reject-Zeile im --dump-plan. Die Perm-/Schritt-Topologie bleibt unberuehrt.
TEST(ExperimentPlanDirector, ResolverRoutesMisplacedSystemAxisInDumpPlan) {
    auto const trio =
        tlz::read_axis_registry_trio(fs::path{COMDARE_CE_AXIS_REGISTRY}, fs::path{COMDARE_SYSTEM_AXIS_REGISTRY},
                                     fs::path{COMDARE_MEASUREMENT_AXIS_REGISTRY});
    ASSERT_TRUE(trio.has_value());

    auto tp = parse_thesis(COMDARE_PLANNER_THESIS_MIN);
    ASSERT_TRUE(tp.has_value());
    cx::ThesisAxisSpec bogus;
    bogus.ref = "target_isa"; // System-Achse in Organ-Position => fehlplatziert (V-CATEGORY)
    tp->permute_axes.push_back(bogus);

    planner::ExperimentPlanDirector const director{*trio};
    CountingBuilder                       cb;
    director.construct(*tp, cb);
    EXPECT_TRUE(cb.header.resolver.resolved);
    EXPECT_FALSE(cb.header.resolver.ok) << "fehlplatzierte System-Achse => Reject";
    ASSERT_EQ(cb.header.resolver.rejects.size(), 1u);
    EXPECT_EQ(cb.header.resolver.rejects[0].code, "V-CATEGORY");
    EXPECT_EQ(cb.header.resolver.rejects[0].ref, "target_isa") << "deterministische Koordinate";
    // Die Perm-/Schritt-Topologie ist unberuehrt (permute_axes speist NICHT den Sweep-Walk).
    ASSERT_EQ(cb.perms.size(), 1u) << "keine system_axes => 1 Identitaets-Perm (unberuehrt)";
    EXPECT_EQ(cb.total_steps(), 1u) << "1 Basis-Sweep-Pass (unberuehrt)";

    planner::PlanTextBuilder text;
    director.construct(*tp, text);
    EXPECT_NE(text.text().find("resolver resolved=1 ok=0 rejects=1"), std::string::npos) << text.text();
    EXPECT_NE(text.text().find("reject code=V-CATEGORY ref=target_isa"), std::string::npos) << text.text();
    EXPECT_NE(text.text().find("build_system_axis_levels"), std::string::npos) << "Routing-Ziel im --dump-plan";
}

// (E4) INERT-by-default: OHNE volles Trio (Default-Konstruktor, wie in der Fassade) bleibt der Resolver aus
//      (resolved=0) -- das Vor-S3-Verhalten byte-identisch, kein Reject im --dump-plan.
TEST(ExperimentPlanDirector, ResolverInertWithoutFullTrio) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_MIN);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director; // Default => full_trio_ leer
    CountingBuilder                       cb;
    director.construct(*tp, cb);
    EXPECT_FALSE(cb.header.resolver.resolved) << "ohne volles Trio ist der Resolver INERT";
    EXPECT_TRUE(cb.header.resolver.ok);
    EXPECT_TRUE(cb.header.resolver.rejects.empty());

    planner::PlanTextBuilder text;
    director.construct(*tp, text);
    EXPECT_NE(text.text().find("resolver resolved=0 ok=1 rejects=0"), std::string::npos) << text.text();
    EXPECT_EQ(text.text().find("reject code="), std::string::npos);
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
    ASSERT_EQ(gb.perms().size(), 12u) << "3 Mess-Combos x 2 opt x 2 simd";
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
    // Cross-Check zum PlanTextBuilder: dessen perm_count-Kopfzeile (|opt x simd| JE Combo) bleibt 4.
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

// (H) STUFE 1 (W10-A): je Mess-Kombination EIN ceb:build- + EIN ceb:emit-Target (ceb:build->ceb:emit-Kante) +
//     1 Aggregat. all_axes_golden faechert S6-P1 auf 3 Combos {wallclock/macro/micro} auf => 3 ceb:build- + 3
//     ceb:emit-Targets + 3 Kanten (die 16 Kategorien reisen je Combo als UNTER mit).
TEST(CMakeGraphBuilder, EmitsPerComboCebBuildEmitTargetsAndAggregate) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::CMakeGraphBuilder            gb;
    director.construct(*tp, gb);
    std::string const& cmake = gb.text();

    // DREI Mess-Kombinationen => 3 CEB-Bau- + 3 CEB-Emit-Targets ([wallclock]/[macro]/[micro]).
    EXPECT_EQ(count_occurrences(cmake, "add_custom_target(comdare_ceb_build_"), 3u)
        << "1 CEB-Bau-Target je Kombination (3 Combos)";
    EXPECT_EQ(count_occurrences(cmake, "add_custom_target(comdare_ceb_emit_"), 3u)
        << "1 CEB-Emit-Target je Kombination (3 Combos)";
    EXPECT_EQ(count_occurrences(cmake, "# ceb:build->ceb:emit-Kante"), 3u) << "eine Bau->Emit-Kante je Kombination";
    // Der CEB-Emit-Schritt ruft --emit-tier-cmake (die CEB emittiert SELBST die Stufe-2, §40.b-Hoheit).
    EXPECT_NE(cmake.find("--emit-tier-cmake"), std::string::npos) << "CEB emittiert Stufe-2 selbst (--emit-tier-cmake)";
    EXPECT_NE(cmake.find("add_custom_target(comdare_experiment_plan_all DEPENDS"), std::string::npos);
    // Blaupausen-Treue (catalog_codegen.cmake:27-37): add_custom_command + DEPENDS + VERBATIM.
    EXPECT_NE(cmake.find("add_custom_command("), std::string::npos);
    EXPECT_NE(cmake.find("VERBATIM)"), std::string::npos);
    // STUFE 1 baut KEINE Tier-Binaries (kein provision-only-Kommando) -- das ist Stufe-2 (--emit-tier-cmake).
    EXPECT_EQ(cmake.find("COMDARE_GOLDEN_N_PROVISION_ONLY=true"), std::string::npos)
        << "Stufe 1 (Planer-Rolle) baut keine Tier-Binaries";
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

// (J) STUFE 2 (W10-A/§42.b, TierCmakeGraphBuilder): der tier:build-Schritt traegt echte provision-only-Treiber-
//     Kommandos, chunk-gebuendelt (kTierChunkCount je Perm); der measure:-Schritt ist ab S5-P2 SCHARF (misst real).
TEST(TierCmakeGraphBuilder, TierBuildIsProvisionOnlyChunkedAndMeasureIsSharp) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::TierCmakeGraphBuilder        gb;
    director.construct(*tp, gb);
    std::string const& cmake = gb.text();

    // S6-P1: 3 Mess-Combos x 4 Perms x kTierChunkCount(=4) = 48 Tier-Chunk-Bau-Targets, je mit provision-only. Der
    // measure:-Schritt ist ab S5-P2 SCHARF (eigenes -E env), setzt aber KEIN COMDARE_GOLDEN_N_PROVISION_ONLY -> die
    // PROVISION_ONLY-Zahl bleibt genau 48 (nur die Tier-Bau-Targets provisionieren; die Mess-Targets messen).
    std::size_t const expected_builds = 3u * 4u * planner::kTierChunkCount;
    EXPECT_EQ(count_occurrences(cmake, "COMDARE_GOLDEN_N_PROVISION_ONLY=true"), expected_builds)
        << "je Combo x Perm x chunk ein provision-only-Tier-Bau (die Mess-Targets provisionieren NICHT)";
    EXPECT_EQ(count_occurrences(cmake, "add_custom_target(comdare_tier_build_perm"), expected_builds);
    EXPECT_EQ(count_occurrences(cmake, "add_custom_target(comdare_tier_measure_perm"), 12u)
        << "1 scharfes Mess-Target je Perm (3 Combos x 4 Perms)";
    EXPECT_NE(cmake.find("\"COMDARE_GN_OPT=O2\""), std::string::npos) << "opt/simd als Plan-Konstanten (LITERALE)";
    EXPECT_NE(cmake.find("\"COMDARE_GN_SIMD=avx2\""), std::string::npos);
    // Host-unabhaengig: konfigurierbare Eingaben als CMake-Variablen mit Defaults, KEINE Host-Absolutpfade.
    EXPECT_NE(cmake.find("if(NOT DEFINED COMDARE_PLAN_DRIVER)"), std::string::npos);
    EXPECT_NE(cmake.find("if(NOT DEFINED COMDARE_PLAN_RANGE)"), std::string::npos);
    EXPECT_EQ(cmake.find("/home/"), std::string::npos) << "keine emit-Zeit-Host-Absolutpfade im .cmake";
    // S5-P2 FLIP: der measure:-Schritt ist SCHARF (misst) -- KEIN gegatetes GN-11/320er-Skelett mehr. Realer
    // Treiber-Aufruf nach measure/. §61-MODI: DLL-Bau PARALLEL (COMDARE_PLAN_MEASURE_PARALLEL), Messen 1-Thread.
    EXPECT_EQ(cmake.find("measure GATED (GN-11/320er"), std::string::npos)
        << "kein gegatetes Mess-Skelett mehr (S5-P2 scharf)";
    EXPECT_NE(cmake.find("measure (S5-P2 scharf, misst)"), std::string::npos)
        << "measure:-Target ist scharf (misst real)";
    EXPECT_NE(cmake.find("${COMDARE_PLAN_OUT}/measure/"), std::string::npos)
        << "scharfer Treiber-Aufruf schreibt EIN CSV je Zelle nach measure/";
    // §61-MODI Regressions-Fix: der DLL-Bau des Mess-Targets laeuft PARALLEL (COMDARE_PLAN_MEASURE_PARALLEL, Default
    // ProcessorCount) -- NICHT mehr COMDARE_BUILD_PARALLEL=1. Das Messen selbst bleibt 1-Thread (run_profile-Loop).
    EXPECT_EQ(count_occurrences(cmake, "COMDARE_BUILD_PARALLEL=${COMDARE_PLAN_MEASURE_PARALLEL}"), 12u)
        << "je Mess-Target DLL-Bau parallel (§61-MODI; 3 Combos x 4 Perms)";
    EXPECT_EQ(cmake.find("COMDARE_BUILD_PARALLEL=1\n"), std::string::npos) << "kein serialisierter Bau mehr (§61-MODI)";
    EXPECT_EQ(cmake.find("COMDARE_RUN_MEASURE=true"), std::string::npos)
        << "COMDARE_RUN_MEASURE hat null Konsumenten -> nie emittiert";
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
    ASSERT_EQ(yb.perms().size(), 12u) << "3 Mess-Combos x 2 opt x 2 simd";
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

// (L) STUFE 1 (W10-A/§42): je Mess-Kombination GENAU 1 ceb:build-, 1 ceb:emit- und 1 ceb:trigger-Job +
//     zweistufige stages-Struktur (ceb-build/ceb-emit). all_axes_golden faechert S6-P1 auf 3 Combos auf =>
//     je 3 ceb:build/emit/trigger-Jobs mit den 3 DISTINKTEN Legenden [wallclock]/[macro]/[micro].
TEST(CiYamlBuilder, EmitsPerComboCebJobsWithTwoStages) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::CiYamlBuilder                yb;
    director.construct(*tp, yb);
    std::string const& yaml = yb.text();

    // DREI Mess-Kombinationen => je 3 ceb:build/emit/trigger-Jobs (Marker-Kommentare, kollisionsfrei zu needs).
    EXPECT_EQ(count_occurrences(yaml, "# JOB ceb-build combo "), 3u) << "1 CEB-Bau-Job je Kombination (3 Combos)";
    EXPECT_EQ(count_occurrences(yaml, "# JOB ceb-emit combo "), 3u) << "1 CEB-Emit-Job je Kombination (--emit-tier-ci)";
    EXPECT_EQ(count_occurrences(yaml, "# JOB ceb-trigger combo "), 3u) << "1 Grandchild-Trigger-Job je Kombination";
    // Live-3-Combo-Assert: die 3 DISTINKTEN [a,b,c]-Legenden-Job-Namen {wallclock/macro/micro} sind ALLE praesent.
    EXPECT_NE(yaml.find("\"ceb:build:[wallclock]\":"), std::string::npos) << "ceb:build:[wallclock]-Strecke";
    EXPECT_NE(yaml.find("\"ceb:build:[macro]\":"), std::string::npos) << "ceb:build:[macro]-Strecke";
    EXPECT_NE(yaml.find("\"ceb:build:[micro]\":"), std::string::npos) << "ceb:build:[micro]-Strecke";
    EXPECT_NE(yaml.find("\"ceb:emit:[wallclock]\":"), std::string::npos);
    EXPECT_NE(yaml.find("\"ceb:trigger:[wallclock]\":"), std::string::npos);
    // Die CEB emittiert SELBST die Stufe-2 (§40.b-Hoheit: --emit-tier-ci, nicht --dump-ci).
    EXPECT_NE(yaml.find("--emit-tier-ci"), std::string::npos) << "CEB-Hoheit: die CEB emittiert Stufe-2 selbst";
    // Genau EIN trigger:-Schluessel (Grandchild via include: artifact:) je Kombination.
    EXPECT_EQ(count_occurrences(yaml, "\n  trigger:\n"), 3u) << "je Kombination ein trigger:-Schluessel (3 Combos)";
    EXPECT_EQ(count_occurrences(yaml, "include:\n      - artifact:"), 3u);
    // Zweistufige stages-Struktur (genau einmal, im Kopf).
    EXPECT_EQ(count_occurrences(yaml, "\nstages:\n"), 1u);
    EXPECT_NE(yaml.find("  - ceb-build\n"), std::string::npos);
    EXPECT_NE(yaml.find("  - ceb-emit\n"), std::string::npos);
    // STUFE 1 (Planer-Rolle) baut KEINE Tier-Binaries (kein provision-only) -- das ist Stufe-2 (--emit-tier-ci).
    EXPECT_EQ(yaml.find("COMDARE_GOLDEN_N_PROVISION_ONLY=true"), std::string::npos)
        << "Stufe 1 baut keine Tier-Binaries";
}

// (L2) S1/A1 P-TOTAL (Ledger 46): der emittierte ceb:trigger-Job forwardet COMDARE_GN_TOTAL vererbungssicher als
//      EXPLIZITE Allowlist an die STUFE-2-Grandchild -- self-contained Grandchild-Pipelines erben Pipeline-Variablen
//      NICHT (der Grandchild-Tier-Bau faellt sonst auf ${COMDARE_GN_TOTAL:-16} zurueck statt 131072 zu bauen).
//      KEIN blindes pipeline_variables:true (Isolation + Byte-Determinismus des Bau-Raums).
TEST(CiYamlBuilder, CebTriggerForwardsGnTotalAsExplicitAllowlist) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::CiYamlBuilder                yb;
    director.construct(*tp, yb);
    std::string const& yaml = yb.text();

    // Die explizite Allowlist: der ceb:trigger-Bridge-Job deklariert COMDARE_GN_TOTAL als YAML-Variable (RHS = der
    // aus STUFE 1 geerbte Wert). DREI Kombinationen => genau 3 solche Bloecke (je Combo ein ceb:trigger-Job).
    EXPECT_EQ(count_occurrences(yaml, "  variables:\n    COMDARE_GN_TOTAL: \"$COMDARE_GN_TOTAL\"\n"), 3u)
        << "ceb:trigger deklariert COMDARE_GN_TOTAL als Forward-Allowlist (3 Combos)";
    // S5-P2-Rest (Smoke-Propagation): derselbe Bridge-Job forwardet zusaetzlich COMDARE_MEASURE_PROFILE -- sonst
    // faellt die STUFE-3-Mess-Rule '$COMDARE_MEASURE_PROFILE == "smoke"' im Grandchild aus (Jobs bleiben when:manual
    // statt Auto-Run; Befund CI-Smoke 11840/Grandchild 11858). DREI Kombinationen => genau 3 Bloecke.
    EXPECT_EQ(count_occurrences(yaml, "    COMDARE_MEASURE_PROFILE: \"$COMDARE_MEASURE_PROFILE\"\n"), 3u)
        << "ceb:trigger forwardet COMDARE_MEASURE_PROFILE (Smoke-Propagation an die Grandchild; 3 Combos)";
    // forward:yaml_variables reicht die Allowlist an die Grandchild; pipeline_variables bleibt AUS (Isolation).
    EXPECT_NE(yaml.find("    forward:\n      yaml_variables: true"), std::string::npos)
        << "forward:yaml_variables:true reicht die Allowlist an die Grandchild";
    EXPECT_NE(yaml.find("      pipeline_variables: false"), std::string::npos)
        << "KEIN blindes Erben des gesamten Eltern-Variablenraums";
    EXPECT_EQ(yaml.find("pipeline_variables: true"), std::string::npos)
        << "pipeline_variables:true ist verboten (Modul-Trigger-Isolation)";
    // Der ceb:trigger bleibt der EINZIGE Grandchild-Trigger je Kombination (keine Struktur-Regression; 3 Combos).
    EXPECT_EQ(count_occurrences(yaml, "\n  trigger:\n"), 3u);
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

    // all_axes_golden traegt no_extension + avx2 -> die STUFE-2-YAML (System-Perm-Ebene, TierCiYamlBuilder) routet
    // auf beide Tags; die STUFE-1-YAML (CiYamlBuilder, CEB-Bau) ist compiler-only => broadest amd64.
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::CiYamlBuilder                yb;
    director.construct(*tp, yb);
    EXPECT_NE(yb.text().find("tags: [\"amd64\"]"), std::string::npos) << "Stufe 1: CEB-Bau amd64 (broadest)";
    EXPECT_EQ(yb.text().find("tags: [\"avx2\"]"), std::string::npos) << "Stufe 1 routet NICHT per SIMD";
    planner::TierCiYamlBuilder tb;
    director.construct(*tp, tb);
    EXPECT_NE(tb.text().find("tags: [\"amd64\"]"), std::string::npos) << "Stufe 2: no_extension-Perm -> amd64";
    EXPECT_NE(tb.text().find("tags: [\"avx2\"]"), std::string::npos) << "Stufe 2: avx2-Perm -> avx2";
}

// (N2) A3 (Task #23/#24, Manager-Ruling Weg a): die EHRLICHE flag-granulare Runner-Tag-LISTE leitet die Tags aus
//      den REAL gebauten -march-Flags ab (system_axis_march_of): avx512 -> {"avx512f"} (aus -mavx512f, NICHT der
//      grobe "avx512"-Tag), avx2 -> {"avx2"}, no_extension/leer -> {"amd64"}. GitLab wertet die Liste als
//      UND-Bedingung (der Runner muss ALLE Tags tragen). Die Liste waechst automatisch mit A7/§40.a mit.
TEST(SimdRunnerTags, FlagGranularListDerivesTagsFromRealMarch) {
    using V = std::vector<std::string>;
    EXPECT_EQ(planner::simd_runner_tags("avx512"), (V{"avx512f"})) << "-mavx512f => avx512f (NICHT 'avx512')";
    EXPECT_EQ(planner::simd_runner_tags("avx2"), (V{"avx2"})) << "-mavx2 => avx2";
    EXPECT_EQ(planner::simd_runner_tags("no_extension"), (V{"amd64"}));
    EXPECT_EQ(planner::simd_runner_tags(""), (V{"amd64"})) << "leer = kein SIMD -> broadest amd64";
    // Symmetrie zum Static-Wrapper (CiYamlBuilder-Test-Surface, EINE Routing-Single-Source).
    EXPECT_EQ(planner::CiYamlBuilder::simd_runner_tags("avx512"), (V{"avx512f"}));
    // Unbekannte simd-id (march leer, aber nicht no_extension) => ISA-Name selbst (kein falscher Tag).
    EXPECT_EQ(planner::simd_runner_tags("sse4"), (V{"sse4"}));

    // Stufe-2-YAML: eine avx512-Perm traegt die flag-granulare Liste tags: ["avx512f"]. Da kein committetes Profil
    // avx512 fuehrt (all_axes_golden = no_extension/avx2), wird der TierCiYamlBuilder direkt getrieben (wie A5c).
    planner::TierCiYamlBuilder tb;
    planner::PlanHeader        h;
    h.source_kind             = "thesis";
    h.profile_id              = "avx512probe";
    h.perm_count              = 1;
    h.measurement_combo_count = 1;
    tb.begin_plan(h);
    planner::PlanMeasurementCombo c;
    c.index  = 0;
    c.legend = "[all]";
    tb.begin_measurement_combo(c);
    planner::PlanPerm p;
    p.index   = 0;
    p.opt_id  = "O3";
    p.simd_id = "avx512";
    tb.begin_perm(p);
    tb.end_perm(p);
    tb.end_measurement_combo(c);
    tb.end_plan(h);
    EXPECT_NE(tb.text().find("tags: [\"avx512f\"]"), std::string::npos)
        << "Stufe-2: avx512-Perm -> tags: [\"avx512f\"] (flag-granular)";
    EXPECT_EQ(tb.text().find("tags: [\"avx512\"]"), std::string::npos) << "NICHT der grobe 'avx512'-Tag";
}

// ─────────────────────────────────────────────────────────────────────────────
// PAKET W10-A / §42+§42.b — die DREISTUFIGE Legenden-Kette (CE erhaelt die XML und steuert ALLES). GEPRUEFT:
// (W1) Legenden-Single-Source: [a,b,c]/[d,e,f]/[g,h,i]-Formatierung deterministisch + YAML-quote-sicher, Job-Namen.
// (W2) DREISTUFIGE Topologie im Director-Walk: Mess-Kombination -> System-Perm -> (Chunk); Kombinations-Zahl.
// (W3) Der --dump-plan-Text traegt die Mess-Achsen-Stufe sichtbar (measurement_combo-Zeilen).
// (W4) Stufe 2 (TierCiYamlBuilder) enthaelt NUR die freigegebenen System-Perms + Tier-Chunk-Jobs + gegatete Mess-Jobs.
// (W5) §42.b Bau=Haupt-only-Gate: KEINE Unter-Achse in Bau-Job-Legenden; die Mess-Jobs sind GN-11/320er-gegatet.
// (W6) Legenden-Determinismus: zwei Laeufe beider Stufen -> byte-gleich.
// ─────────────────────────────────────────────────────────────────────────────

namespace lg = comdare::cache_engine::planner::legend;

// (W1) Legenden-Single-Source: die Array-/Combo-/Perm-/Organ-/Job-Formatierung.
TEST(PlanLegend, FormatsShortDeterministicYamlSafeArraysAndJobNames) {
    // Achsen-Array (kurz, kommagetrennt, Klammern).
    EXPECT_EQ(lg::axis_array({"a", "b", "c"}), "[a,b,c]");
    EXPECT_EQ(lg::axis_array({}), "[]");
    // Sanitisierung: Trennzeichen ([ ] , :) werden defensiv auf '_' gefaltet (YAML-quote-sicher).
    EXPECT_EQ(lg::sanitize_token("a:b,c[d]"), "a_b_c_d_");
    // System-Perm [d,e,f] = [opt,simd].
    EXPECT_EQ(lg::system_perm("O2", "avx2"), "[O2,avx2]");
    // Organ-Referenz [g,h,i] = die fuehrenden Organ-Haupt-Achsen (kCompositionAxisNames-Single-Source).
    EXPECT_EQ(lg::organ_reference(), "[search_algo,cache_traversal,mapping]");
    // §47/§54-T2/§55: die [a,b,c]-HAUPT-Kombination kommt aus der Mess-Tooling-HAUPT-Achse {wallclock/macro/micro}
    // (measurement_tooling_combo) — leer/volles Angebot => [all]; echtes Subset => sortiertes Array.
    EXPECT_EQ(lg::measurement_tooling_combo({}), "[all]") << "leer = volles Mess-System";
    EXPECT_EQ(lg::measurement_tooling_combo({"wallclock", "macro", "micro"}), "[all]") << "volles Angebot => [all]";
    EXPECT_EQ(lg::measurement_tooling_combo({"wallclock"}), "[wallclock]") << "Einzel-Tooling-Konfig";
    EXPECT_EQ(lg::measurement_tooling_combo({"micro", "macro"}), "[macro,micro]") << "dedupliziert + sortiert";
    // measurement_combo bleibt der UNTER-Kategorien-Formatter (CSV, §54-T2) — NICHT die HAUPT-Auffaecherung:
    // leer ODER alle 16 => [all]; echtes Subset => sortiertes Array.
    EXPECT_EQ(lg::measurement_combo({}), "[all]");
    EXPECT_EQ(lg::measurement_combo({"THROUGHPUT", "CLU"}), "[CLU,THROUGHPUT]") << "dedupliziert + sortiert";
    // Job-Namen der drei Stufen.
    EXPECT_EQ(lg::ceb_build_job("[all]"), "ceb:build:[all]");
    EXPECT_EQ(lg::ceb_emit_job("[all]"), "ceb:emit:[all]");
    EXPECT_EQ(lg::ceb_trigger_job("[all]"), "ceb:trigger:[all]");
    // §56/§54-T6: die Tier-Bau-Legende ist System-Perm [d,e,f] x Organ-Referenz [g,h,i] + chunk<k> im ORGAN-Slot;
    // die Mess-Kombination [a,b,c] steht NICHT in der Bau-Legende (sie lebt nur in ceb:build:[a,b,c]).
    EXPECT_EQ(lg::tier_build_job("[O2,avx2]", "[g,h,i]", 3), "tier:build:[O2,avx2][g,h,i]:chunk3");
    EXPECT_EQ(lg::measure_job("[all]", "[O2,avx2]", "[g,h,i]"), "measure:[all][O2,avx2][g,h,i]");
}

// (T2) §47/§54-T2/§55: die [a,b,c]-HAUPT-Auffaecherung kommt aus der Mess-Tooling-HAUPT-Achse {wallclock/macro/micro}
//      (NICHT aus den 16 <measurement_categories> = UNTER). Fan-out-KERN: N Tooling-Konfigs => N Combos; Default
//      (keine Konfig deklariert) => 1 Voll-Konfig [all] (Topologie byte-stabil); die Kategorien reisen als UNTER mit.
TEST(MeasurementToolingFanOut, HauptAxisIsToolingNotCategories) {
    namespace mt = comdare::cache_engine::measurement;
    // Das Registry-ANGEBOT (OFFER): 3 Tooling {wallclock/macro/micro}, Index==Tooling (Single-Source).
    ASSERT_EQ(mt::kMeasurementToolingCount, 3u);
    EXPECT_EQ(mt::kMeasurementToolingRegistry[0].id, "wallclock");
    EXPECT_EQ(mt::kMeasurementToolingRegistry[1].id, "macro");
    EXPECT_EQ(mt::kMeasurementToolingRegistry[2].id, "micro");

    std::vector<std::string> const cats{"THROUGHPUT", "CLU"};

    // Default (keine Tooling-Konfig): EINE implizite VOLL-Konfig => HAUPT-Legende [all]; die Kategorien = UNTER.
    auto const def = planner::ExperimentPlanDirector::measurement_combos_of(cats);
    ASSERT_EQ(def.size(), 1u) << "Default = 1 Voll-Konfig (Topologie byte-stabil zur heutigen 1-CEB-Strecke)";
    EXPECT_EQ(def[0].legend, "[all]") << "[a,b,c]-HAUPT = volles Mess-System";
    EXPECT_EQ(def[0].categories, cats) << "die 16 Kategorien reisen als Mess-Tooling-UNTER (CSV) mit";
    EXPECT_TRUE(def[0].tooling.empty()) << "leere Konfig = volles Angebot";

    // Fan-out-KERN: N Tooling-Konfigs => N Combos; die HAUPT-Legende je Konfig kommt aus dem TOOLING.
    auto const fan =
        planner::ExperimentPlanDirector::measurement_combos_of(cats, {{"wallclock"}, {"macro"}, {"micro"}});
    ASSERT_EQ(fan.size(), 3u) << "3 Tooling-Konfigs => 3 ceb:build:[a,b,c]-Strecken (§47/§55)";
    EXPECT_EQ(fan[0].legend, "[wallclock]");
    EXPECT_EQ(fan[1].legend, "[macro]");
    EXPECT_EQ(fan[2].legend, "[micro]");
    EXPECT_EQ(fan[0].index, 0u);
    EXPECT_EQ(fan[2].index, 2u);
    EXPECT_EQ(fan[1].tooling, (std::vector<std::string>{"macro"})) << "die Tooling-KONFIG reist mit";

    // §54-T2: ein Kategorie-Subset (UNTER) faechert den CEB-Typ NICHT auf — die HAUPT-Legende bleibt tooling-bestimmt.
    auto const cat_subset = planner::ExperimentPlanDirector::measurement_combos_of({"CLU"}, {{"wallclock"}});
    ASSERT_EQ(cat_subset.size(), 1u);
    EXPECT_EQ(cat_subset[0].legend, "[wallclock]") << "Kategorie-Subset ist UNTER, keine Auffaecherung (§54-T2)";
}

// (W2) DREISTUFIGE Topologie: die Anwender-XML bestimmt die Mess-Kombination; darunter die System-Perms.
TEST(ExperimentPlanDirector, ThreeStageTopologyMeasurementComboOuterSystemPermInner) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    CountingBuilder                       cb;
    director.construct(*tp, cb);

    // all_axes_golden faechert S6-P1 auf 3 Mess-Tooling-Combos {wallclock/macro/micro} auf; die 16 Kategorien
    // reisen je Combo als UNTER mit (die HAUPT-Legende kommt aus dem Tooling, NICHT aus den Kategorien).
    ASSERT_EQ(cb.combos.size(), 3u) << "3 Mess-Kombinationen (Tooling-Fan-out {wallclock/macro/micro})";
    EXPECT_EQ(cb.header.measurement_combo_count, 3u);
    EXPECT_EQ(cb.combos[0].legend, "[wallclock]") << "erste Tooling-Combo";
    EXPECT_EQ(cb.combos[1].legend, "[macro]");
    EXPECT_EQ(cb.combos[2].legend, "[micro]");
    // Die System-Perms je Kombination bleiben byte-identisch (4 Perms je Combo -> 3 x 4 = 12 Perms total).
    ASSERT_EQ(cb.perms.size(), 12u) << "2 opt x 2 simd je Mess-Kombination x 3 Combos";
    EXPECT_EQ(cb.header.perm_count, 4u) << "perm_count = |opt x simd| JE Mess-Kombination (unveraendert)";
}

// (W3) --dump-plan zeigt die Mess-Achsen-Stufe sichtbar (measurement_combo-Zeile + count-Kopf).
TEST(PlanTextBuilder, DumpPlanShowsMeasurementComboStage) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::PlanTextBuilder              pt;
    director.construct(*tp, pt);
    EXPECT_NE(pt.text().find("measurement_combo_count=3"), std::string::npos);
    EXPECT_NE(pt.text().find("measurement_combo 0 legend=[wallclock]"), std::string::npos);
    EXPECT_NE(pt.text().find("measurement_combo 1 legend=[macro]"), std::string::npos);
    EXPECT_NE(pt.text().find("measurement_combo 2 legend=[micro]"), std::string::npos);
    EXPECT_NE(pt.text().find("perm_count=4"), std::string::npos) << "Perm-Ebene bleibt unter der Kombination";
}

// (W4) Stufe 2 (CEB-Rolle, TierCiYamlBuilder): NUR die freigegebenen System-Perms + Tier-Chunk-Jobs + gegatete
//      Mess-Jobs. all_axes_golden: 4 Perms x kTierChunkCount Chunk-Bau-Jobs + 4 gegatete Mess-Jobs.
TEST(TierCiYamlBuilder, EmitsFreedSystemPermsWithTierChunkAndGatedMeasureJobs) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    std::size_t const expected_chunks = 3u * 4u * planner::kTierChunkCount; // 3 Combos x 4 Perms x kTierChunkCount
    EXPECT_EQ(count_occurrences(yaml, "# JOB tier-build "), expected_chunks)
        << "kTierChunkCount Chunk-Bau-Jobs je Perm (3 Combos x 4 Perms)";
    EXPECT_EQ(count_occurrences(yaml, "# JOB measure "), 12u) << "1 (gegateter) Mess-Job je System-Perm (3 Combos x 4)";
    // Die zwei Stufen-2-stages.
    EXPECT_NE(yaml.find("  - tier-build\n"), std::string::npos);
    EXPECT_NE(yaml.find("  - measure\n"), std::string::npos);
    // KEINE Stufe-1-CEB-Jobs in der Stufe-2-Sicht (die CEB-Jobs gehoeren in --dump-ci).
    EXPECT_EQ(yaml.find("ceb:build:"), std::string::npos) << "Stufe 2 enthaelt KEINE CEB-Bau-Jobs";
    EXPECT_EQ(yaml.find("stage: ceb-build"), std::string::npos);
    // §56/§54-T6: die Tier-Bau-Jobs tragen die Legende [d,e,f][g,h,i]:chunk<k> = System-Perm x Organ-Referenz
    // (organ_reference() = [search_algo,cache_traversal,mapping]); die Mess-Kombination [a,b,c] steht NICHT drin.
    EXPECT_NE(yaml.find("\"tier:build:[O2,no_extension][search_algo,cache_traversal,mapping]:chunk0\":"),
              std::string::npos);
    EXPECT_NE(yaml.find("\"tier:build:[O2,avx2][search_algo,cache_traversal,mapping]:chunk0\":"), std::string::npos);
    EXPECT_NE(yaml.find("\"tier:build:[O3,avx2][search_algo,cache_traversal,mapping]:chunk3\":"), std::string::npos);
    // §56/§54-T6: die needs:-Kante der Mess-Jobs referenziert EXAKT dieselbe korrigierte Bau-Legende (sonst
    // brechen die Bau->Mess-Job-Kanten). Literaler Beleg: die needs-Zeile traegt [d,e,f][g,h,i]:chunk<k>, NICHT
    // die alte [a,b,c][d,e,f]-Fehlform.
    EXPECT_NE(yaml.find("    - \"tier:build:[O2,no_extension][search_algo,cache_traversal,mapping]:chunk0\"\n"),
              std::string::npos)
        << "needs:-Kante referenziert die korrigierte tier:build-Legende";
    // Die Mess-Jobs tragen die volle [a,b,c][d,e,f][g,h,i]-Legende und sind provision-only im Bau (golden-neutral).
    EXPECT_EQ(count_occurrences(yaml, "COMDARE_GOLDEN_N_PROVISION_ONLY=true"), expected_chunks)
        << "je Chunk-Bau-Job provision-only (Bau ohne Messung)";
}

// (W5) §42.b Bau=Haupt-only-Gate: KEINE Unter-Achse (Laufzeit-Parameter) in irgendeiner Bau-Job-Legende; die
//      Mess-Jobs sind ab S5-P2 SCHARF (misst real), gegatet ueber rules (smoke=Auto-Run / sonst when:manual,
//      §41/320er) + resource_group ceb-measurement-exclusive (B14-#3) -- KEIN Skelett mehr.
TEST(TierCiYamlBuilder, BuildLegendsCarryNoSubAxisAndMeasureIsSharp) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    // Die Bau-Job-Schluessel selbst tragen KEINE Unter-Achsen-Namen (reine Laufzeit-Parameter, §42.b). Wir pruefen
    // die Legenden-Zeilen der Bau-Jobs ("tier:build:...) auf die bekannten dynamischen Unter-Achsen-Dimensionen.
    for (char const* sub :
         {"thread_count", "prefetch_distance", "pool_budget", "batch_size", "inline_threshold", "workload"}) {
        // In den Bau-JOB-Namen darf keine Unter-Achse vorkommen: wir scannen jede "tier:build:"-Zeile.
        std::size_t pos = 0;
        while ((pos = yaml.find("\"tier:build:", pos)) != std::string::npos) {
            std::size_t const eol  = yaml.find('\n', pos);
            std::string const line = yaml.substr(pos, eol - pos);
            EXPECT_EQ(line.find(sub), std::string::npos) << "Bau=Haupt-only-Gate (§42.b): Unter-Achse '" << sub
                                                         << "' darf NICHT in der Bau-Legende stehen: " << line;
            pos = eol;
        }
    }
    // S5-P2: die Mess-Jobs sind SCHARF, aber gegatet. rules (§41/320er): je Mess-Job EINE smoke-Auto-Run-Regel +
    // EINE when:manual-Fallback-Regel (Voll-Messlauf erst nach User-Entscheid). Struktur ja, Auto-Voll-Messlauf nein.
    EXPECT_EQ(count_occurrences(yaml, "    - when: manual"), 12u)
        << "je Mess-Job ein when:manual-Fallback (Voll-Messlauf = User-Entscheid; 3 Combos x 4 Perms)";
    EXPECT_EQ(count_occurrences(yaml, "    - if: '$COMDARE_MEASURE_PROFILE == \"smoke\"'"), 12u)
        << "je Mess-Job eine smoke-Auto-Run-Regel (COMDARE_MEASURE_PROFILE==smoke => when:on_success)";
    EXPECT_NE(yaml.find("320er"), std::string::npos) << "Gate-Provenienz (§41/320er) dokumentiert";
    // (h)/(k) §61-KONSOLIDIERUNG: Mess-Exklusivitaet PRO MASCHINE (ceb-measure-<host>). all_axes_golden (no_ext+avx2,
    // 3 Combos): avx2->intel IMMER (6 Jobs); no_ext folgt der Combo (wallclock->amd, macro->intel, micro->amd) =>
    // amd = 2(wallclock)+0(macro)+2(micro) = 4; intel = 6(avx2) + 2(macro-no_ext) = 8. Summe 12.
    EXPECT_EQ(count_occurrences(yaml, "  resource_group: \"ceb-measure-amd\""), 4u)
        << "amd-Lane: no_ext bei wallclock+micro (2+2)";
    EXPECT_EQ(count_occurrences(yaml, "  resource_group: \"ceb-measure-intel\""), 8u)
        << "intel-Lane: alle avx2 (6) + no_ext bei macro (2)";
    EXPECT_EQ(yaml.find("ceb-measurement-exclusive"), std::string::npos)
        << "keine globale Mess-Serialisierung mehr (§61-MODI: prod1+prod2 messen parallel)";
    // S5-P2 FLIP: der reale Mess-Vollzug schreibt EIN CSV je Zelle nach measure_out. §61-MODI: der DLL-Bau laeuft
    // PARALLEL (COMDARE_BUILD_PARALLEL=$(nproc)), NUR das Messen ist 1-Thread (run_profile); OHNE COMDARE_RUN_MEASURE.
    EXPECT_NE(yaml.find("$CI_PROJECT_DIR/Code/measure_out/"), std::string::npos)
        << "scharfer Mess-Aufruf schreibt nach measure_out";
    EXPECT_EQ(count_occurrences(yaml, "export COMDARE_BUILD_PARALLEL=\"$(nproc)\""), 12u)
        << "je Mess-Job DLL-Bau parallel (§61-MODI Regressions-Fix; 3 Combos x 4 Perms)";
    EXPECT_EQ(yaml.find("export COMDARE_BUILD_PARALLEL=1"), std::string::npos)
        << "kein serialisierter Bau mehr (§61-MODI: der alte =1 war eine Regression)";
    EXPECT_EQ(yaml.find("COMDARE_RUN_MEASURE=true"), std::string::npos)
        << "COMDARE_RUN_MEASURE hat null Konsumenten -> nie emittiert";
    EXPECT_EQ(yaml.find("measure GATED (GN-11/320er"), std::string::npos)
        << "kein gegatetes Mess-Skelett mehr (S5-P2 scharf)";
}

// (W6) Legenden-Determinismus beider Stufen: zwei Laeufe -> byte-gleich (Thesis + Experiment).
TEST(TierCiYamlBuilder, StageTwoIsByteDeterministic) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    auto const ep = parse_experiment(COMDARE_EXPERIMENT_GOLDEN);
    ASSERT_TRUE(ep.has_value());

    planner::ExperimentPlanDirector const director;
    {
        planner::TierCiYamlBuilder a, b;
        director.construct(*tp, a);
        director.construct(*tp, b);
        EXPECT_EQ(a.text(), b.text()) << "thesis Stufe-2-YAML byte-deterministisch";
        EXPECT_FALSE(a.text().empty());
        EXPECT_EQ(a.text().find("/home/"), std::string::npos) << "keine emit-Zeit-Host-Absolutpfade";
    }
    {
        planner::TierCmakeGraphBuilder a, b;
        director.construct(*ep, a);
        director.construct(*ep, b);
        EXPECT_EQ(a.text(), b.text()) << "experiment Stufe-2-cmake byte-deterministisch";
    }
}

// (W7) W10-Nacharbeit (Serie-E2E 11562/11566): die self-contained Child-YAMLs erben die Parent-Globals NICHT ->
//      BEIDE CI-Stufen (CiYamlBuilder Stufe 1 + TierCiYamlBuilder Stufe 2) muessen die ccache-/Parallel-Variablen
//      + einen top-level cache:-Block selbst emittieren (sonst ccache-Permission-Fail am Runner). Spiegel des Parent.
TEST(CiYamlBuilder, BothStagesEmitParentMirroredCcacheConfig) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;

    planner::CiYamlBuilder     s1; // Stufe 1
    planner::TierCiYamlBuilder s2; // Stufe 2
    director.construct(*tp, s1);
    director.construct(*tp, s2);

    for (auto const* yaml : {&s1.text(), &s2.text()}) {
        // LITERAL-Variablen bleiben in variables: (Parent-Spiegel). CCACHE_DIR NICHT mehr hier (W10-Nacharbeit 4:
        // $CI_PROJECT_DIR-Werte ausschliesslich per Prolog-Export, s. Test W8/W9).
        EXPECT_NE(yaml->find("  CCACHE_MAXSIZE: \"3G\""), std::string::npos);
        EXPECT_NE(yaml->find("  CMAKE_BUILD_PARALLEL_LEVEL: \"6\""), std::string::npos);
        EXPECT_EQ(yaml->find("  CCACHE_DIR:"), std::string::npos)
            << "CCACHE_DIR steht NICHT mehr in variables: (nur noch Prolog-Export)";
        // top-level cache:-Block mit dem GLEICHEN Key wie der Parent (Warm-ccache zieht auch im Child); paths ist
        // workdir-relativ (.ccache), der Key nutzt $CI_PROJECT_NAME (Cache-System-Expansion, NICHT $CI_PROJECT_DIR).
        EXPECT_NE(yaml->find("\ncache:\n  key: \"ccache-$CI_PROJECT_NAME\"\n  paths: [\".ccache\"]\n"),
                  std::string::npos)
            << "top-level cache:-Block (gleicher Key wie Parent)";
        // genau EIN top-level cache:-Block je Child (kein Duplikat).
        EXPECT_EQ(count_occurrences(*yaml, "\ncache:\n"), 1u);
    }
}

// (W9) W10-Nacharbeit 4 (Serie-E2E Lauf 4): KLASSEN-WACHE -- der variables:-Block beider Stufen enthaelt KEINEN
//      $CI_PROJECT_DIR-Anteil mehr (die gitlab-seitig vorexpandierte, vererbte Parent-Variable expandiert im Child
//      versions-/wegabhaengig LEER -> /.ccache bzw. /Code/...-fehlt). Alle $CI_PROJECT_DIR-Werte kommen
//      ausschliesslich per Runtime-Shell-Export im Prolog. HART, damit die Klasse nie wieder aufmacht.
TEST(CiYamlBuilder, NoCiProjectDirInVariablesBlockBothStages) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::CiYamlBuilder                s1;
    planner::TierCiYamlBuilder            s2;
    director.construct(*tp, s1);
    director.construct(*tp, s2);

    for (auto const* yaml : {&s1.text(), &s2.text()}) {
        // Den variables:-Block isolieren (von "variables:\n" bis zum naechsten top-level Key "cache:\n").
        auto const vbeg = yaml->find("\nvariables:\n");
        ASSERT_NE(vbeg, std::string::npos);
        auto const vend = yaml->find("\ncache:\n", vbeg);
        ASSERT_NE(vend, std::string::npos);
        std::string const variables_block = yaml->substr(vbeg, vend - vbeg);
        EXPECT_EQ(variables_block.find("$CI_PROJECT_DIR"), std::string::npos)
            << "KLASSE: kein $CI_PROJECT_DIR in variables: (nur Literale) -- Block:\n"
            << variables_block;
        // Positiv: der Prolog exportiert die $CI_PROJECT_DIR-Werte zur Laufzeit.
        EXPECT_NE(yaml->find("export COMDARE_GOLDEN_N_PROFILE=\"${CI_PROJECT_DIR}/Code/external/comdare-cache-engine/"),
                  std::string::npos)
            << "COMDARE_GOLDEN_N_PROFILE per Runtime-Export im Prolog";
    }
    // je Job-mit-Klon-Prolog ein GOLDEN_N_PROFILE-Export: Stufe 1 = 2 (ceb:build + ceb:emit); Stufe 2 = 16
    // (tier:build) + 4 (Mess-Jobs, ab S5-P2 SCHARF) = 20 -- JE Mess-Combo (S6-P1: x3 => 6 bzw. 60).
    EXPECT_EQ(count_occurrences(s1.text(), "export COMDARE_GOLDEN_N_PROFILE="), 3u * 2u);
    EXPECT_EQ(count_occurrences(s2.text(), "export COMDARE_GOLDEN_N_PROFILE="),
              3u * (4u * planner::kTierChunkCount + 4u));
}

// (W10) W10-Nacharbeit 5 (Serie-E2E Lauf 5): je chunk<k>-Tier-Bau-Job berechnet sein DISJUNKTES Teilfenster zur
//       Laufzeit (Pilot-Matrix-Chunk-Arithmetik) -- kein globales Fixfenster mehr in der Nutzlast (sonst baute der
//       Voll-Build 4x die volle Range je Perm). Steuerung ueber COMDARE_GN_TOTAL (reine Zahl).
TEST(TierCiYamlBuilder, PerChunkRangeArithmeticNoGlobalFixedWindow) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    // Chunk-Arithmetik-Zeilen in JEDEM der 48 Tier-Bau-Jobs (3 Combos x 4 Perms x kTierChunkCount).
    std::size_t const jobs = 3u * 4u * planner::kTierChunkCount;
    EXPECT_EQ(count_occurrences(yaml, "CHUNK_SIZE=$(( (TOTAL + CCOUNT - 1) / CCOUNT ))"), jobs)
        << "ceil-Arithmetik je Job";
    EXPECT_EQ(count_occurrences(yaml, "START=$(( CHUNK * CHUNK_SIZE ))"), jobs);
    EXPECT_EQ(count_occurrences(yaml, "export COMDARE_GOLDEN_N_RANGE=\"${START}:${COUNT_C}\""), jobs);
    EXPECT_EQ(count_occurrences(yaml, "TOTAL=\"${COMDARE_GN_TOTAL:-16}\""), jobs) << "Default 16 = sicherer 4x4-Test";
    // Je System-Perm 4 UNTERSCHIEDLICHE k-Werte -> je k genau 12x (einmal pro Perm ueber 4 Perms x 3 Combos).
    EXPECT_EQ(count_occurrences(yaml, "; CHUNK=0\n"), 12u);
    EXPECT_EQ(count_occurrences(yaml, "; CHUNK=1\n"), 12u);
    EXPECT_EQ(count_occurrences(yaml, "; CHUNK=2\n"), 12u);
    EXPECT_EQ(count_occurrences(yaml, "; CHUNK=3\n"), 12u);
    // KEIN globales Fixfenster mehr in der Nutzlast (der alte ${COMDARE_GN_RANGE:-0:4}-Fallback ist weg).
    EXPECT_EQ(yaml.find("COMDARE_GN_RANGE:-0:4"), std::string::npos)
        << "kein globales Fixfenster in der Nutzlast (sonst 4x volle Range je Perm im Voll-Build)";
    // jenseits-TOTAL -> No-Op-Exit 0 (golden-neutral).
    EXPECT_EQ(count_occurrences(yaml, "-> No-Op (golden-neutral) =="), jobs);
}

// (W8) W10-Nacharbeit 2 (Serie-E2E 11569/11576): die self-contained Child-YAMLs erben default:before_script NICHT
//      -> JEDER Bau-Job beider Stufen traegt den Submodul-Klon-PROLOG (Deploy-Token via CI-Variablen, NIE Klartext)
//      + global GIT_SUBMODULE_STRATEGY:none (kein Auto-Fetch, der am extraheader failt).
TEST(CiYamlBuilder, BothStagesEmitSubmoduleClonePrologInBuildJobs) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;

    planner::CiYamlBuilder     s1; // Stufe 1: ceb:build + ceb:emit = 2 Bau-Jobs
    planner::TierCiYamlBuilder s2; // Stufe 2: 4 Perms x kTierChunkCount tier:build = 16 Bau-Jobs
    director.construct(*tp, s1);
    director.construct(*tp, s2);
    std::string const& y1 = s1.text();
    std::string const& y2 = s2.text();

    // Prolog-Marker: 1 je Job-mit-Klon-Prolog. Stufe 1 = 2 (ceb:build + ceb:emit); Stufe 2 = 16 (tier:build) +
    // 4 (Mess-Jobs, ab S5-P2 SCHARF: self-clone) = 20. (Vor S5-P2 waren die Mess-Jobs Skelette OHNE Prolog.)
    EXPECT_EQ(count_occurrences(y1, "# CHILD-SUBMODULE-KLON"), 3u * 2u) << "ceb:build + ceb:emit je Combo (3 Combos)";
    EXPECT_EQ(count_occurrences(y2, "# CHILD-SUBMODULE-KLON"), 3u * (4u * planner::kTierChunkCount + 4u))
        << "je tier:build-Job + je scharfem Mess-Job (S5-P2) x 3 Combos";
    // W10-Nacharbeit 3: ccache-Env per RUNTIME-Shell-Export im Prolog (1 je Job-mit-Prolog, immun gegen die vererbte,
    // vorexpandierte Parent-CCACHE_DIR). Stufe 1 = 2, Stufe 2 = 16 tier:build + 4 Mess-Jobs (S5-P2) = 20 -- JE Combo (x3).
    EXPECT_EQ(count_occurrences(y1, "export CCACHE_DIR=\"${CI_PROJECT_DIR}/.ccache\""), 3u * 2u);
    EXPECT_EQ(count_occurrences(y2, "export CCACHE_DIR=\"${CI_PROJECT_DIR}/.ccache\""),
              3u * (4u * planner::kTierChunkCount + 4u));
    for (auto const* yaml : {&y1, &y2}) {
        // Runtime-Export beider ccache-Variablen (VOR jedem cmake-Aufruf in der Job-Shell).
        EXPECT_NE(yaml->find("export CCACHE_DIR=\"${CI_PROJECT_DIR}/.ccache\""), std::string::npos);
        EXPECT_NE(yaml->find("export CCACHE_MAXSIZE=\"3G\""), std::string::npos);
        // global GIT_SUBMODULE_STRATEGY:none (kein Auto-Fetch); KEIN per-Job recursive-Override mehr.
        EXPECT_NE(yaml->find("  GIT_SUBMODULE_STRATEGY: \"none\""), std::string::npos);
        EXPECT_EQ(yaml->find("GIT_SUBMODULE_STRATEGY: recursive"), std::string::npos)
            << "kein Auto-Fetch-Override mehr (der failt am extraheader)";
        // Deploy-Token NUR als CI-Variablen-Referenz, NIE Klartext (Byte-Determinismus + kein Leak).
        EXPECT_NE(yaml->find("${CE_SUBMODULE_USER}:${CE_SUBMODULE_TOKEN}"), std::string::npos)
            << "Token als Variablen-Referenz";
        // Idempotenz-Haertung: --force + sync auf den gepinnten gitlink-SHA; ce + prt-art pfad-geskopt.
        EXPECT_NE(yaml->find("git submodule update --init --recursive --force -- Code/external/comdare-cache-engine "
                             "Code/external/comdare-prt-art"),
                  std::string::npos);
        // die Overleaf-Thesis wird NICHT geklont (C++-Bau braucht sie nicht).
        EXPECT_EQ(yaml->find("20260931-overleaf-diplomarbeit"), std::string::npos)
            << "Thesis-Submodul im Bau-Child nicht geklont";
    }
}

// (A5) §56-T2-FANOUT D4 -- der per-CEB Combo-Selektor an --emit-tier-ci. Der Fan-out-KERN select_measurement_combo ist
//      isoliert testbar: --emit-tier-ci repraesentiert GENAU EINE CEB-Konfig (je ceb:emit-Job eine Konfig); da
//      §56/T6 die Mess-Konfig aus der tier:build-Legende ENTFERNT hat (combo-unabhaengige Job-Namen), wuerden N>1
//      CEB-Konfigs in EINEM Lauf kollidieren -> der Selektor behaelt NUR die Kombination mit cmake_slug(legend) ==
//      Selektor. Leerer Selektor = IDENTITAET (heutige Live-Strecke, byte-stabil). Kein Treffer => ehrlich leer.
TEST(SelectMeasurementCombo, EmptySelectorIsIdentityAndSlugMatchIsExact) {
    // Drei CEB-Konfigs (die HAUPT-Auffaecherung {wallclock/macro/micro}); der Selektor arbeitet auf cmake_slug(legend).
    auto const combos =
        planner::ExperimentPlanDirector::measurement_combos_of({"CLU"}, {{"wallclock"}, {"macro"}, {"micro"}});
    ASSERT_EQ(combos.size(), 3u);

    // Leerer Selektor = IDENTITAET (die heutige Live-Strecke): alle Kombinationen bleiben, Original-index erhalten.
    auto const identity = planner::ExperimentPlanDirector::select_measurement_combo(combos, "");
    ASSERT_EQ(identity.size(), 3u) << "leerer Selektor = Identitaet (byte-stabil)";
    EXPECT_EQ(identity[2].index, 2u) << "KEIN Re-Indexing: Original-index bleibt";

    // Exakter cmake_slug-Match: [macro] => _macro_ => genau EINE Kombination (die repraesentierte CEB-Konfig).
    auto const one = planner::ExperimentPlanDirector::select_measurement_combo(combos, "_macro_");
    ASSERT_EQ(one.size(), 1u) << "genau die EINE repraesentierte CEB-Konfig";
    EXPECT_EQ(one[0].legend, "[macro]");
    EXPECT_EQ(one[0].index, 1u) << "die ueberlebende Kombination behaelt ihren Original-index (Walk-Determinismus)";

    // Kein cmake_slug-Treffer => ehrlich leer (kein Crash, keine Phantom-Kombination).
    auto const none = planner::ExperimentPlanDirector::select_measurement_combo(combos, "_nonexist_");
    EXPECT_TRUE(none.empty()) << "kein Treffer => leer (ehrliche Null-Selektion)";
}

// (A5b) S6-P1: der per-CEB Combo-Selektor auf dem GEFANNTEN all_axes_golden (3 Combos {wallclock/macro/micro}).
//       Die fruehere Byte-Identitaet "default == [all]-Selektor" war ein N=1-Sonderfall (die 1-CEB-Strecke trug
//       [all]); mit N=3 testet die Trichotomie die Selektor-Semantik STAERKER: (i) leerer Selektor = Identitaet
//       (voller 3-Combo-Walk); (ii) EIN realer Selektor "_wallclock_" = echtes nicht-leeres Subset (genau 1/3 der
//       Tier-Bau-Jobs); (iii) Miss = ehrlich leere Stufe-2 (0 tier:build-Jobs), kein Crash.
TEST(SelectMeasurementCombo, SelectorTrichotomyIdentitySubsetMissOnFannedFixture) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;

    // (i) Identitaet: leerer Selektor == der explizite construct("") -- der volle 3-Combo-Walk (byte-gleich, > 0 Jobs).
    planner::TierCiYamlBuilder tb_default, tb_empty;
    director.construct(*tp, tb_default);
    director.construct(*tp, tb_empty, "");
    EXPECT_EQ(tb_default.text(), tb_empty.text()) << "leerer Selektor = Identitaet (voller 3-Combo-Walk)";
    std::size_t const full_jobs = count_occurrences(tb_default.text(), "# JOB tier-build ");
    EXPECT_GT(full_jobs, 0u) << "Default emittiert Tier-Bau-Jobs (3 Combos)";

    // (ii) EIN realer Selektor "_wallclock_" -> echtes nicht-leeres Subset: genau 1/3 der Tier-Bau-Jobs (1 von 3 Combos).
    planner::TierCiYamlBuilder tb_one;
    director.construct(*tp, tb_one, "_wallclock_");
    std::size_t const one_jobs = count_occurrences(tb_one.text(), "# JOB tier-build ");
    EXPECT_GT(one_jobs, 0u) << "der reale Selektor behaelt seine EINE CEB-Konfig";
    EXPECT_EQ(one_jobs * 3u, full_jobs) << "genau 1/3 der Tier-Bau-Jobs (1 von 3 Combos)";

    // (iii) Nicht existierender Selektor -> 0 Kombinationen -> 0 tier:build-Jobs (ehrlich leer, kein Crash).
    planner::TierCiYamlBuilder tb_none;
    director.construct(*tp, tb_none, "_does_not_exist_");
    EXPECT_EQ(count_occurrences(tb_none.text(), "# JOB tier-build "), 0u)
        << "kein Selektor-Treffer => ehrlich leere Stufe-2 (0 Tier-Bau-Jobs)";
}

// (A5c) STUFE 1 (CiYamlBuilder): bei N>1 CEB-Konfigs traegt jeder ceb:emit-Job den distinct --measurement-combo-
//       Selektor (Kollisionsschutz der combo-unabhaengigen tier:build-Job-Namen, §56/T6). count==1 => KEIN Flag
//       (byte-Stabilitaet zur heutigen 1-CEB-Strecke). Der Builder wird direkt getrieben (die Live-construct()-Naht
//       reicht heute {} => 1 Combo; die Fan-out-Deklaration ist XML-gated, D2/D3).
TEST(CiYamlBuilder, PerComboCebEmitCarriesDistinctMeasurementComboSelectorWhenFannedOut) {
    auto const combos =
        planner::ExperimentPlanDirector::measurement_combos_of({"CLU"}, {{"wallclock"}, {"macro"}, {"micro"}});
    ASSERT_EQ(combos.size(), 3u);

    planner::CiYamlBuilder yb;
    planner::PlanHeader    h;
    h.source_kind             = "thesis";
    h.profile_id              = "fanout";
    h.perm_count              = 1;
    h.measurement_combo_count = combos.size(); // N>1 => Selektor-Naht AKTIV
    yb.begin_plan(h);
    for (auto const& c : combos) yb.begin_measurement_combo(c);
    std::string const& yaml = yb.text();

    // Je ceb:emit-Job traegt GENAU seinen distinct Selektor (cmake_slug der [a,b,c]-Legende).
    EXPECT_NE(yaml.find("--measurement-combo=_wallclock_"), std::string::npos);
    EXPECT_NE(yaml.find("--measurement-combo=_macro_"), std::string::npos);
    EXPECT_NE(yaml.find("--measurement-combo=_micro_"), std::string::npos);
    EXPECT_EQ(count_occurrences(yaml, "--measurement-combo="), 3u) << "je ceb:emit-Job genau EIN Selektor";
}

TEST(CiYamlBuilder, SingleComboCebEmitOmitsMeasurementComboSelectorForByteStability) {
    auto const combos = planner::ExperimentPlanDirector::measurement_combos_of({"CLU"}); // 1 Voll-Konfig [all]
    ASSERT_EQ(combos.size(), 1u);

    planner::CiYamlBuilder yb;
    planner::PlanHeader    h;
    h.source_kind             = "thesis";
    h.profile_id              = "single";
    h.perm_count              = 1;
    h.measurement_combo_count = 1; // heutige Live-Strecke => Selektor-Naht DORMANT
    yb.begin_plan(h);
    yb.begin_measurement_combo(combos[0]);
    EXPECT_EQ(yb.text().find("--measurement-combo="), std::string::npos)
        << "count==1 => KEIN --measurement-combo (byte-identisch zu vor A5)";
}

// (A8a) A8(a)-Symmetrie: der combo_selector reist auch bis --emit-tier-cmake durch (TierCmakeGraphBuilder), EXAKT
//       symmetrisch zu --emit-tier-ci (A5b). S6-P1 (gefanntes all_axes_golden, 3 Combos): dieselbe Trichotomie --
//       (i) leerer Selektor = Identitaet (voller 3-Combo-Walk); (ii) EIN realer Selektor "_wallclock_" = echtes
//       nicht-leeres Subset (genau 1/3 der Tier-Bau-Targets); (iii) Miss = ehrlich leere Stufe-2 (0 Targets), kein
//       Crash. Golden-neutral: nur die emittierten .cmake-Strings.
TEST(SelectMeasurementCombo, EmitTierCmakeSelectorTrichotomyIdentitySubsetMiss) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;

    // (i) Identitaet: leerer Selektor == der explizite construct("") -- der volle 3-Combo-Walk (byte-gleich, > 0).
    planner::TierCmakeGraphBuilder cm_default, cm_empty;
    director.construct(*tp, cm_default);
    director.construct(*tp, cm_empty, "");
    EXPECT_EQ(cm_default.text(), cm_empty.text()) << "emit-tier-cmake: leerer Selektor = Identitaet (3-Combo-Walk)";
    std::size_t const full_targets = count_occurrences(cm_default.text(), "add_custom_target(comdare_tier_build_perm");
    EXPECT_GT(full_targets, 0u) << "Default emittiert Tier-Bau-Targets (3 Combos)";

    // (ii) EIN realer Selektor "_wallclock_" -> echtes nicht-leeres Subset: genau 1/3 der Tier-Bau-Targets.
    planner::TierCmakeGraphBuilder cm_one;
    director.construct(*tp, cm_one, "_wallclock_");
    std::size_t const one_targets = count_occurrences(cm_one.text(), "add_custom_target(comdare_tier_build_perm");
    EXPECT_GT(one_targets, 0u) << "der reale Selektor behaelt seine EINE CEB-Konfig";
    EXPECT_EQ(one_targets * 3u, full_targets) << "genau 1/3 der Tier-Bau-Targets (1 von 3 Combos)";

    // (iii) Nicht existierender Selektor -> 0 Kombinationen -> 0 tier:build-Targets (ehrlich leer, kein Crash).
    planner::TierCmakeGraphBuilder cm_none;
    director.construct(*tp, cm_none, "_does_not_exist_");
    EXPECT_EQ(count_occurrences(cm_none.text(), "add_custom_target(comdare_tier_build_perm"), 0u)
        << "kein Selektor-Treffer => ehrlich leere Stufe-2 (0 Tier-Bau-Targets)";
}

// (S6-P1b-d) Env-Bruecke: die Tier-Bau-/Mess-Kommandos exportieren COMDARE_MEASUREMENT_COMBO=<combo-legend> ab N>1
//       (der Director kennt die Combo aus dem gefilterten Walk, combo_legend_). Default/[all] (1-Combo-Profil) =>
//       KEIN Export (byte-stabil). So reist die gewaehlte Combo bis zum Treiber (run_profile stempelt die je-Combo-DLLs).
TEST(MeasurementComboEnvBridge, TierCommandsCarryComboEnvWhenFannedAndOmitForAll) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES); // 3 Combos {wallclock/macro/micro}
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;

    // Stufe-2-YAML (--emit-tier-ci): je-Combo COMDARE_MEASUREMENT_COMBO in den tier:build/measure-Treiber-Kommandos.
    planner::TierCiYamlBuilder tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();
    EXPECT_NE(yaml.find("COMDARE_MEASUREMENT_COMBO=\"[wallclock]\""), std::string::npos);
    EXPECT_NE(yaml.find("COMDARE_MEASUREMENT_COMBO=\"[macro]\""), std::string::npos);
    EXPECT_NE(yaml.find("COMDARE_MEASUREMENT_COMBO=\"[micro]\""), std::string::npos);
    // Der Export sitzt VOR COMDARE_GN_OPT im Treiber-Kommando (Bau + Mess).
    EXPECT_NE(yaml.find("COMDARE_MEASUREMENT_COMBO=\"[wallclock]\" COMDARE_GN_OPT="), std::string::npos);

    // Stufe-2-cmake (--emit-tier-cmake): dieselbe Combo bare-metal-symmetrisch (unquoted -E env-Zeile).
    planner::TierCmakeGraphBuilder cm;
    director.construct(*tp, cm);
    EXPECT_NE(cm.text().find("COMDARE_MEASUREMENT_COMBO=[wallclock]"), std::string::npos);

    // Default/[all] (planner_thesis_min: kein <measurement_tooling> => 1 Combo [all]) => KEIN Export (byte-stabil).
    auto const tp_all = parse_thesis(COMDARE_PLANNER_THESIS_MIN);
    ASSERT_TRUE(tp_all.has_value());
    planner::TierCiYamlBuilder tb_all;
    director.construct(*tp_all, tb_all);
    EXPECT_EQ(tb_all.text().find("COMDARE_MEASUREMENT_COMBO="), std::string::npos)
        << "[all]-Default => kein Combo-Export (byte-stabil zum Vor-S6-P1b-Kommando)";
    planner::TierCmakeGraphBuilder cm_all;
    director.construct(*tp_all, cm_all);
    EXPECT_EQ(cm_all.text().find("COMDARE_MEASUREMENT_COMBO="), std::string::npos);
}

// (S6-P1 g/h) §61-MODI: der Mess-Job traegt (g) den smoke=Debug-Branch (parallel/schnell) + measure=Release (sonst),
//       den §61-Regressions-Fix (DLL-Bau parallel statt =1) und (h) per-Host-Lanes (prod1/amd, prod2/intel; avx512
//       nie intel). Paralleles MESSEN (debug-Ideal) bleibt UNGEBAUT (§16.2-M1) -- hier NICHT getestet (ehrliche Luecke).
TEST(MeasurementModi61, ProfileDrivenModeParallelBuildLanesAndCompileStamp) {
    planner::ExperimentPlanDirector const director;

    // MEASURE-Profil (all_axes_golden, run_methodology=measure per j1): STATISCHER Release-Build (KEIN Runtime-Branch;
    // j2: Methodik aus dem PROFIL, nicht Env), §61-MODI-Regressions-Fix (DLL-Bau parallel), per-Host-Lanes, KEIN +bt.
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    ASSERT_EQ(tp->run_methodology.size(), 1u) << "exactly-one (j1)";
    ASSERT_EQ(tp->run_methodology.front(), "measure");
    planner::TierCiYamlBuilder tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    EXPECT_NE(yaml.find("-DCMAKE_BUILD_TYPE=Release"), std::string::npos) << "measure => statisch Release (j2)";
    EXPECT_EQ(yaml.find("COMDARE_MEASURE_BUILD_TYPE"), std::string::npos) << "kein Runtime-Build-Typ-Branch mehr (j2)";
    EXPECT_EQ(yaml.find("COMDARE_BUILD_TYPE=\"Debug\""), std::string::npos) << "measure=Default => kein +bt-Signal (i)";
    // §61-MODI Regressions-Fix: DLL-Bau parallel ($(nproc)), NICHT =1.
    EXPECT_NE(yaml.find("export COMDARE_BUILD_PARALLEL=\"$(nproc)\""), std::string::npos);
    EXPECT_EQ(yaml.find("export COMDARE_BUILD_PARALLEL=1"), std::string::npos);
    // (h) per-Host-Lanes: amd + intel + Host-Tags (prod1+prod2 messen parallel).
    EXPECT_NE(yaml.find("  resource_group: \"ceb-measure-amd\""), std::string::npos);
    EXPECT_NE(yaml.find("  resource_group: \"ceb-measure-intel\""), std::string::npos);
    EXPECT_NE(yaml.find("  tags: [\"amd\"]"), std::string::npos) << "no_extension-Perm -> amd-Lane";
    EXPECT_NE(yaml.find("  tags: [\"intel\"]"), std::string::npos) << "avx2-Perm -> intel-Lane";

    // (platform-Tag) §61/§62: der Mess-Job exportiert COMDARE_PLATFORM=<lane>@$(hostname) -> CSV-Spalte "platform"
    // traegt die MESSENDE Maschine (compile_time_platform_tag trennt amd/intel-x86_64 NICHT). Beide Lanes praesent.
    EXPECT_NE(yaml.find("export COMDARE_PLATFORM=\"amd@$(hostname)\""), std::string::npos)
        << "amd-Lane-Mess-Job taggt die Plattform (prod1)";
    EXPECT_NE(yaml.find("export COMDARE_PLATFORM=\"intel@$(hostname)\""), std::string::npos)
        << "intel-Lane-Mess-Job taggt die Plattform (prod2)";

    // (k) measure_host_lane(simd, combo): no_extension folgt der Mess-Combo (F-4-Aufloesung); avx512/avx2 sind
    // combo-UNABHAENGIG (Hardware-Zwang bzw. Standard-Routing schlaegt die Combo).
    EXPECT_EQ(planner::measure_host_lane("no_extension", "[wallclock]"), "amd") << "no_ext+wallclock -> amd (prod1)";
    EXPECT_EQ(planner::measure_host_lane("no_extension", "[macro]"), "intel") << "no_ext+macro -> intel (prod2)";
    EXPECT_EQ(planner::measure_host_lane("no_extension", "[micro]"), "amd") << "no_ext+micro -> amd (prod1)";
    EXPECT_EQ(planner::measure_host_lane("avx512", "[macro]"), "amd") << "avx512 zwingend amd (Combo ignoriert)";
    EXPECT_EQ(planner::measure_host_lane("avx2", "[wallclock]"), "intel") << "avx2 -> intel (Combo ignoriert)";
    EXPECT_NE(planner::measure_host_lane("avx512", "[macro]"), "intel") << "avx512 landet NIE auf intel";

    // DEBUG-Profil (j2: Methodik aus dem PROFIL): dieselben Achsen, run_methodology=debug -> STATISCH Debug-Build +
    // (i) COMDARE_BUILD_TYPE=Debug-Signal (Nicht-Default => +bt=Debug an der facade-Suffix-Naht, benannter Folgepunkt).
    auto dbg             = tp;
    dbg->run_methodology = {"debug"};
    planner::TierCiYamlBuilder tb_dbg;
    director.construct(*dbg, tb_dbg);
    std::string const& ydbg = tb_dbg.text();
    EXPECT_NE(ydbg.find("-DCMAKE_BUILD_TYPE=Debug"), std::string::npos) << "debug-Profil => statisch Debug (j2)";
    EXPECT_NE(ydbg.find("COMDARE_BUILD_TYPE=\"Debug\""), std::string::npos) << "(i) Nicht-Default => +bt-Signal";

    // (j3) §61-STUFEN Dual-Compile: der Debug-Mess-Job macht ZWEI Treiber-Aufrufe -- (1) Release provision-only
    // (O2/O3-Reuse-Masse, eigenes _release_provision-Dir, KEIN +bt), (2) Debug-Bau+Messung (-O0/+bt). Blocker #50:
    // ARTEFAKT_TRIES=1. Der measure/Release-Pfad (yaml oben) bleibt byte-stabil (EIN Aufruf, kein provision, kein Override).
    EXPECT_NE(ydbg.find("(j3) Aufruf 1/2: Release provision-only"), std::string::npos)
        << "(j3) Debug: Aufruf 1 = Release provision-only (Reuse-Masse)";
    EXPECT_NE(ydbg.find("(j3) Aufruf 2/2: Debug-Bau+Messung"), std::string::npos)
        << "(j3) Debug: Aufruf 2 = Debug measure";
    EXPECT_NE(ydbg.find("COMDARE_GOLDEN_N_PROVISION_ONLY=true"), std::string::npos)
        << "(j3) Debug-Mess-Job provisioniert die Reuse-Masse (Aufruf 1)";
    EXPECT_NE(ydbg.find("_release_provision\""), std::string::npos)
        << "(j3) Aufruf 1 hat ein EIGENES Ausgabe-Dir (Debug-Bau ueberschreibt die Release-.so nicht)";
    EXPECT_NE(ydbg.find("export COMDARE_ARTEFAKT_TRIES=1"), std::string::npos)
        << "(j3) Blocker #50: smoke/debug begrenzt die measure-drop-Retries (hart)";
    EXPECT_EQ(yaml.find("(j3) Aufruf 1/2"), std::string::npos) << "measure => KEIN (j3)-Dual-Compile (byte-stabil)";
    EXPECT_EQ(yaml.find("_release_provision"), std::string::npos) << "measure => kein Release-Provision-Vorlauf";
    EXPECT_EQ(yaml.find("COMDARE_ARTEFAKT_TRIES"), std::string::npos) << "measure => kein ARTEFAKT_TRIES-Override";

    // Bare-metal (--emit-tier-cmake): DLL-Bau-Pool parallel (ProcessorCount) fuer measure; kein +bt. Debug => +bt.
    planner::TierCmakeGraphBuilder cm;
    director.construct(*tp, cm);
    EXPECT_NE(cm.text().find("COMDARE_BUILD_PARALLEL=${COMDARE_PLAN_MEASURE_PARALLEL}"), std::string::npos);
    EXPECT_NE(cm.text().find("ProcessorCount(_comdare_measure_nproc)"), std::string::npos)
        << "Default = ProcessorCount";
    EXPECT_EQ(cm.text().find("COMDARE_BUILD_PARALLEL=1\n"), std::string::npos);
    EXPECT_EQ(cm.text().find("COMDARE_BUILD_TYPE=Debug"), std::string::npos) << "measure => kein +bt (cmake)";
    planner::TierCmakeGraphBuilder cm_dbg;
    director.construct(*dbg, cm_dbg);
    EXPECT_NE(cm_dbg.text().find("\"COMDARE_BUILD_TYPE=Debug\""), std::string::npos) << "debug => +bt-Signal (cmake)";

    // (j3) cmake-symmetrisch: das Debug-Mess-Target traegt VOR dem Debug-Mess-COMMAND einen Release-Provision-Vorlauf
    // (eigenes _release_provision-Dir, ARTEFAKT_TRIES=1). measure/Release bleibt byte-stabil (kein Vorlauf).
    EXPECT_NE(cm_dbg.text().find("(j3) 1/2: Release provision-only"), std::string::npos)
        << "(j3) Debug cmake: Release-Provision-Vorlauf im Mess-Target";
    EXPECT_NE(cm_dbg.text().find("_release_provision\""), std::string::npos)
        << "(j3) Debug cmake: eigenes Provision-Dir";
    // (j3)/R4 LOCKSTEP: TRIES=1 steht jetzt in BEIDEN -E env-Bloecken je Debug-Perm (Provision-COMMAND UND Debug-Mess-
    // COMMAND), nicht mehr nur im Provision-COMMAND. Perm-anzahl-robuste Invariante: #TRIES == #Provision + #Mess.
    // (VOR R4 war #TRIES == #Provision allein -> der Debug-Mess-Aufruf konnte in measure-drop-Retries grinden.)
    auto const n_tries_dbg = count_occurrences(cm_dbg.text(), "\"COMDARE_ARTEFAKT_TRIES=1\"");
    auto const n_provision = count_occurrences(cm_dbg.text(), "(j3) 1/2: Release provision-only");
    auto const n_mess_dbg  = count_occurrences(cm_dbg.text(), "measure (S5-P2 scharf, misst): [a,b,c][d,e,f][g,h,i]=");
    EXPECT_GT(n_provision, 0u) << "(j3) Debug cmake: mindestens ein Provision-Block je Perm";
    EXPECT_EQ(n_mess_dbg, n_provision) << "(j3) je Perm ein Provision- und ein Mess-COMMAND";
    EXPECT_EQ(n_tries_dbg, n_provision + n_mess_dbg)
        << "(j3)/R4 Debug cmake: TRIES=1 in BEIDEN env-Bloecken je Perm (Blocker #50)";
    // Positions-Pruefung: mindestens eine TRIES-Instanz liegt NACH dem ersten Mess-Echo (= im Debug-Mess-COMMAND,
    // nicht zweimal im Provision-Block).
    auto const first_mess_echo = cm_dbg.text().find("measure (S5-P2 scharf, misst): [a,b,c][d,e,f][g,h,i]=");
    ASSERT_NE(first_mess_echo, std::string::npos) << "(j3) Debug-Mess-Echo-Marker vorhanden";
    EXPECT_NE(cm_dbg.text().find("\"COMDARE_ARTEFAKT_TRIES=1\"", first_mess_echo), std::string::npos)
        << "(j3)/R4 Debug cmake: TRIES=1 auch im Mess-COMMAND (nach dem Mess-Echo)";
    EXPECT_EQ(cm.text().find("(j3) 1/2"), std::string::npos) << "measure cmake => kein (j3)-Vorlauf (byte-stabil)";
    EXPECT_EQ(cm.text().find("_release_provision"), std::string::npos) << "measure cmake => kein Release-Provision-Dir";
    EXPECT_EQ(cm.text().find("COMDARE_ARTEFAKT_TRIES"), std::string::npos)
        << "(j3)/R4 measure cmake => KEIN ARTEFAKT_TRIES (Release-Mess-Target byte-identisch zum Ist-Stand)";
}

// (R5) exactly-one-Haertung (Ledger §61-STUFEN, LED:3190): ein 2-Modi-Profil bricht auf dem tp-Pfad HART ab, statt
// still ids.front() (debug-Semantik inkl. parallelem Mess-Loop) zu nehmen. Zwei Konsum-Ebenen: (1) der Director-Konsum
// build_semantic_of_run_methodology via construct(); (2) der Runtime-Konsum run_methodology_for_ids (Mess-Loop-Naht).
TEST(MeasurementModi61, TwoModeProfileHardFailsExactlyOne) {
    namespace mm = comdare::cache_engine::measurement;
    planner::ExperimentPlanDirector const director;

    auto tp2 = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp2.has_value());
    tp2->run_methodology = {"debug", "measure"}; // 2 Modi = Kontraktbruch (exactly-one verletzt)
    planner::TierCmakeGraphBuilder cm_two;
    EXPECT_THROW(director.construct(*tp2, cm_two), std::invalid_argument)
        << "(R5) tp-Pfad: build_semantic_of_run_methodology bricht bei >1 Modi HART ab (kein stilles front())";

    // Runtime-Konsum (Mess-Loop-Naht, resolve_measure_parallelism -> run_methodology_for_ids): wirft ebenfalls bei >1.
    EXPECT_THROW(mm::run_methodology_for_ids({"debug", "measure"}), std::invalid_argument)
        << "(R5) run_methodology_for_ids bricht bei >1 Methoden HART ab";
    EXPECT_THROW(mm::run_methodology_for_ids({"measure", "release"}), std::invalid_argument);

    // exactly-one bleibt gueltig + byte-neutral (kein Fehlalarm):
    EXPECT_EQ(mm::run_methodology_for_ids({"debug"}).methodology, mm::RunMethodology::Debug);
    EXPECT_EQ(mm::run_methodology_for_ids({}).methodology, mm::RunMethodology::Measure); // leer => measure-Default
    auto tp1 = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp1.has_value());
    tp1->run_methodology = {"measure"};
    planner::TierCmakeGraphBuilder cm_one;
    EXPECT_NO_THROW(director.construct(*tp1, cm_one)) << "(R5) exactly-one-Profil baut normal (byte-neutral)";
}

// (i)-facade §61-STUFEN Byte-Wache: die facade-Suffix-Naht build_type_version_suffix liest COMDARE_BUILD_TYPE und
//       haengt +bt=Debug NUR bei Debug ans build_version. Ungesetzt/Release (Default) => "" => build_version BYTE-
//       IDENTISCH (Sidecar/Resume/golden/dll_is_current unberuehrt). Reuse-Schluessel: Debug-DLL != Release-DLL.
TEST(CompileTypeStamp, BtVersionSuffixOnlyForDebugElseByteStable) {
    namespace tlz = comdare::cache_engine::thesis_lazy;
    ::unsetenv("COMDARE_BUILD_TYPE");
    EXPECT_EQ(tlz::build_type_version_suffix(), "") << "ungesetzt => kein Suffix (build_version byte-identisch)";
    ::setenv("COMDARE_BUILD_TYPE", "Debug", 1);
    EXPECT_EQ(tlz::build_type_version_suffix(), "+bt=Debug") << "Debug => +bt=Debug (Reuse-Schluessel scharf)";
    ::setenv("COMDARE_BUILD_TYPE", "Release", 1);
    EXPECT_EQ(tlz::build_type_version_suffix(), "") << "Release (Default) => kein Suffix (byte-identisch)";
    ::unsetenv("COMDARE_BUILD_TYPE");
}
