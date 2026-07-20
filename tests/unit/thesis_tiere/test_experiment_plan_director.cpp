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
#include "planner/plan_legend.hpp"              // W10-A: das dreistufige Legenden-Namensschema (Single-Source)
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

// (D') KERN-A (S4 Mess-Schema, 2026-07-20): das measurement_tooling-Feld ist im Schema ADDITIV (Parser fuellt es),
//      aber die Fan-out-VERDRAHTUNG in construct() gehoert dem Schwester-Paket P-MESSTOOL. KERN-A-Beleg: die
//      LIVE-Call-Site bleibt bei Default 1 Combo [all] (byte-stabil zur heutigen 1-CEB-Strecke); die Golden-
//      Instanz traegt kein <measurement_tooling>. (Der Fan-out-KERN measurement_combos_of ist separat in
//      MeasurementToolingFanOut getestet.)
TEST(ExperimentPlanDirector, MeasurementToolingStaysDefaultOneComboInKernA) {
    auto const ep = parse_experiment(COMDARE_EXPERIMENT_GOLDEN);
    ASSERT_TRUE(ep.has_value());
    EXPECT_TRUE(ep->measurement_tooling.empty()) << "Golden deklariert KEIN <measurement_tooling> (passiv, Default)";

    planner::ExperimentPlanDirector const director;
    CountingBuilder                       cb;
    director.construct(*ep, cb);
    ASSERT_EQ(cb.combos.size(), 1u) << "KERN-A reicht measurement_tooling NICHT durch => 1 Combo [all] (byte-stabil)";
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

// (H) STUFE 1 (W10-A): je Mess-Kombination EIN ceb:build- + EIN ceb:emit-Target (ceb:build->ceb:emit-Kante) +
//     1 Aggregat. all_axes_golden traegt EINE Kombination ([all], alle 16 Kategorien) => 1+1 Targets.
TEST(CMakeGraphBuilder, EmitsPerComboCebBuildEmitTargetsAndAggregate) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::CMakeGraphBuilder            gb;
    director.construct(*tp, gb);
    std::string const& cmake = gb.text();

    // EINE Mess-Kombination => 1 CEB-Bau- + 1 CEB-Emit-Target ([all]).
    EXPECT_EQ(count_occurrences(cmake, "add_custom_target(comdare_ceb_build_"), 1u)
        << "1 CEB-Bau-Target je Kombination";
    EXPECT_EQ(count_occurrences(cmake, "add_custom_target(comdare_ceb_emit_"), 1u)
        << "1 CEB-Emit-Target je Kombination";
    EXPECT_EQ(count_occurrences(cmake, "# ceb:build->ceb:emit-Kante"), 1u) << "eine Bau->Emit-Kante je Kombination";
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

    // 4 Perms x kTierChunkCount(=4) = 16 Tier-Chunk-Bau-Targets, je mit provision-only. Der measure:-Schritt ist ab
    // S5-P2 SCHARF (eigenes -E env), setzt aber KEIN COMDARE_GOLDEN_N_PROVISION_ONLY -> die PROVISION_ONLY-Zahl
    // bleibt genau 16 (nur die Tier-Bau-Targets provisionieren; die Mess-Targets messen).
    std::size_t const expected_builds = 4u * planner::kTierChunkCount;
    EXPECT_EQ(count_occurrences(cmake, "COMDARE_GOLDEN_N_PROVISION_ONLY=true"), expected_builds)
        << "je Perm x chunk ein provision-only-Tier-Bau (die Mess-Targets provisionieren NICHT)";
    EXPECT_EQ(count_occurrences(cmake, "add_custom_target(comdare_tier_build_perm"), expected_builds);
    EXPECT_EQ(count_occurrences(cmake, "add_custom_target(comdare_tier_measure_perm"), 4u)
        << "1 scharfes Mess-Target je Perm";
    EXPECT_NE(cmake.find("\"COMDARE_GN_OPT=O2\""), std::string::npos) << "opt/simd als Plan-Konstanten (LITERALE)";
    EXPECT_NE(cmake.find("\"COMDARE_GN_SIMD=avx2\""), std::string::npos);
    // Host-unabhaengig: konfigurierbare Eingaben als CMake-Variablen mit Defaults, KEINE Host-Absolutpfade.
    EXPECT_NE(cmake.find("if(NOT DEFINED COMDARE_PLAN_DRIVER)"), std::string::npos);
    EXPECT_NE(cmake.find("if(NOT DEFINED COMDARE_PLAN_RANGE)"), std::string::npos);
    EXPECT_EQ(cmake.find("/home/"), std::string::npos) << "keine emit-Zeit-Host-Absolutpfade im .cmake";
    // S5-P2 FLIP: der measure:-Schritt ist SCHARF (misst) -- KEIN gegatetes GN-11/320er-Skelett mehr. Realer
    // Treiber-Aufruf nach measure/, 1-Thread-deterministisch (COMDARE_BUILD_PARALLEL=1), OHNE COMDARE_RUN_MEASURE.
    EXPECT_EQ(cmake.find("measure GATED (GN-11/320er"), std::string::npos)
        << "kein gegatetes Mess-Skelett mehr (S5-P2 scharf)";
    EXPECT_NE(cmake.find("measure (S5-P2 scharf, misst)"), std::string::npos)
        << "measure:-Target ist scharf (misst real)";
    EXPECT_NE(cmake.find("${COMDARE_PLAN_OUT}/measure/"), std::string::npos)
        << "scharfer Treiber-Aufruf schreibt EIN CSV je Zelle nach measure/";
    EXPECT_EQ(count_occurrences(cmake, "COMDARE_BUILD_PARALLEL=1"), 4u)
        << "je Mess-Target 1-Thread-deterministisch (COMDARE_BUILD_PARALLEL=1, §38.b)";
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

// (L) STUFE 1 (W10-A/§42): je Mess-Kombination GENAU 1 ceb:build-, 1 ceb:emit- und 1 ceb:trigger-Job +
//     zweistufige stages-Struktur (ceb-build/ceb-emit). all_axes_golden => 1 Kombination ([all]).
TEST(CiYamlBuilder, EmitsPerComboCebJobsWithTwoStages) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::CiYamlBuilder                yb;
    director.construct(*tp, yb);
    std::string const& yaml = yb.text();

    // EINE Mess-Kombination => je 1 ceb:build/emit/trigger-Job (Marker-Kommentare, kollisionsfrei zu needs).
    EXPECT_EQ(count_occurrences(yaml, "# JOB ceb-build combo "), 1u) << "1 CEB-Bau-Job je Kombination";
    EXPECT_EQ(count_occurrences(yaml, "# JOB ceb-emit combo "), 1u) << "1 CEB-Emit-Job je Kombination (--emit-tier-ci)";
    EXPECT_EQ(count_occurrences(yaml, "# JOB ceb-trigger combo "), 1u) << "1 Grandchild-Trigger-Job je Kombination";
    // Die Legenden-Job-Namen tragen die [a,b,c]-Kurzform ([all] = alle 16 Kategorien).
    EXPECT_NE(yaml.find("\"ceb:build:[all]\":"), std::string::npos) << "ceb:build:[a,b,c]-Legende (all)";
    EXPECT_NE(yaml.find("\"ceb:emit:[all]\":"), std::string::npos);
    EXPECT_NE(yaml.find("\"ceb:trigger:[all]\":"), std::string::npos);
    // Die CEB emittiert SELBST die Stufe-2 (§40.b-Hoheit: --emit-tier-ci, nicht --dump-ci).
    EXPECT_NE(yaml.find("--emit-tier-ci"), std::string::npos) << "CEB-Hoheit: die CEB emittiert Stufe-2 selbst";
    // Genau EIN trigger:-Schluessel (Grandchild via include: artifact:) je Kombination.
    EXPECT_EQ(count_occurrences(yaml, "\n  trigger:\n"), 1u) << "je Kombination ein trigger:-Schluessel";
    EXPECT_EQ(count_occurrences(yaml, "include:\n      - artifact:"), 1u);
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
    // aus STUFE 1 geerbte Wert). Genau EINE Kombination ([all]) => genau EIN solcher Block.
    EXPECT_EQ(count_occurrences(yaml, "  variables:\n    COMDARE_GN_TOTAL: \"$COMDARE_GN_TOTAL\"\n"), 1u)
        << "ceb:trigger deklariert COMDARE_GN_TOTAL als Forward-Allowlist";
    // forward:yaml_variables reicht die Allowlist an die Grandchild; pipeline_variables bleibt AUS (Isolation).
    EXPECT_NE(yaml.find("    forward:\n      yaml_variables: true"), std::string::npos)
        << "forward:yaml_variables:true reicht die Allowlist an die Grandchild";
    EXPECT_NE(yaml.find("      pipeline_variables: false"), std::string::npos)
        << "KEIN blindes Erben des gesamten Eltern-Variablenraums";
    EXPECT_EQ(yaml.find("pipeline_variables: true"), std::string::npos)
        << "pipeline_variables:true ist verboten (Modul-Trigger-Isolation)";
    // Der ceb:trigger bleibt der EINZIGE Grandchild-Trigger je Kombination (keine Struktur-Regression).
    EXPECT_EQ(count_occurrences(yaml, "\n  trigger:\n"), 1u);
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

    // all_axes_golden deklariert alle 16 <measurement_categories> => EINE Kombination mit Legende [all].
    ASSERT_EQ(cb.combos.size(), 1u) << "heute typisch EINE Mess-Kombination (ersetzt GN_MSYS=default)";
    EXPECT_EQ(cb.header.measurement_combo_count, 1u);
    EXPECT_EQ(cb.combos[0].legend, "[all]") << "alle 16 Kategorien => [all]-Sentinel";
    // Die System-Perms bleiben byte-identisch zum Vor-W10-Verhalten (4 Perms je Kombination).
    ASSERT_EQ(cb.perms.size(), 4u) << "2 opt x 2 simd je Mess-Kombination";
    EXPECT_EQ(cb.header.perm_count, 4u) << "perm_count = |opt x simd| JE Mess-Kombination";
}

// (W3) --dump-plan zeigt die Mess-Achsen-Stufe sichtbar (measurement_combo-Zeile + count-Kopf).
TEST(PlanTextBuilder, DumpPlanShowsMeasurementComboStage) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::PlanTextBuilder              pt;
    director.construct(*tp, pt);
    EXPECT_NE(pt.text().find("measurement_combo_count=1"), std::string::npos);
    EXPECT_NE(pt.text().find("measurement_combo 0 legend=[all]"), std::string::npos);
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

    std::size_t const expected_chunks = 4u * planner::kTierChunkCount;
    EXPECT_EQ(count_occurrences(yaml, "# JOB tier-build "), expected_chunks)
        << "kTierChunkCount Chunk-Bau-Jobs je Perm";
    EXPECT_EQ(count_occurrences(yaml, "# JOB measure "), 4u) << "1 (gegateter) Mess-Job je System-Perm";
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
    EXPECT_EQ(count_occurrences(yaml, "    - when: manual"), 4u)
        << "je Mess-Job ein when:manual-Fallback (Voll-Messlauf = User-Entscheid)";
    EXPECT_EQ(count_occurrences(yaml, "    - if: '$COMDARE_MEASURE_PROFILE == \"smoke\"'"), 4u)
        << "je Mess-Job eine smoke-Auto-Run-Regel (COMDARE_MEASURE_PROFILE==smoke => when:on_success)";
    EXPECT_NE(yaml.find("320er"), std::string::npos) << "Gate-Provenienz (§41/320er) dokumentiert";
    // B14-#3: je Mess-Job eine exklusive resource_group (kein paralleler Messlauf auf demselben Runner).
    EXPECT_EQ(count_occurrences(yaml, "  resource_group: \"ceb-measurement-exclusive\""), 4u)
        << "je Mess-Job resource_group ceb-measurement-exclusive (Run-to-Run-Stabilitaet)";
    // S5-P2 FLIP: der reale Mess-Vollzug schreibt EIN CSV je Zelle nach measure_out, 1-Thread-deterministisch
    // (COMDARE_BUILD_PARALLEL=1), OHNE COMDARE_GOLDEN_N_PROVISION_ONLY (die Abwesenheit IST das Mess-Signal) und
    // OHNE COMDARE_RUN_MEASURE (null Konsumenten).
    EXPECT_NE(yaml.find("$CI_PROJECT_DIR/Code/measure_out/"), std::string::npos)
        << "scharfer Mess-Aufruf schreibt nach measure_out";
    EXPECT_EQ(count_occurrences(yaml, "export COMDARE_BUILD_PARALLEL=1"), 4u)
        << "je Mess-Job 1-Thread-deterministisch (§38.b)";
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
    // (tier:build) + 4 (Mess-Jobs, ab S5-P2 SCHARF: self-clone + self-build + misst) = 20.
    EXPECT_EQ(count_occurrences(s1.text(), "export COMDARE_GOLDEN_N_PROFILE="), 2u);
    EXPECT_EQ(count_occurrences(s2.text(), "export COMDARE_GOLDEN_N_PROFILE="), 4u * planner::kTierChunkCount + 4u);
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

    // Chunk-Arithmetik-Zeilen in JEDEM der 16 Tier-Bau-Jobs.
    std::size_t const jobs = 4u * planner::kTierChunkCount;
    EXPECT_EQ(count_occurrences(yaml, "CHUNK_SIZE=$(( (TOTAL + CCOUNT - 1) / CCOUNT ))"), jobs)
        << "ceil-Arithmetik je Job";
    EXPECT_EQ(count_occurrences(yaml, "START=$(( CHUNK * CHUNK_SIZE ))"), jobs);
    EXPECT_EQ(count_occurrences(yaml, "export COMDARE_GOLDEN_N_RANGE=\"${START}:${COUNT_C}\""), jobs);
    EXPECT_EQ(count_occurrences(yaml, "TOTAL=\"${COMDARE_GN_TOTAL:-16}\""), jobs) << "Default 16 = sicherer 4x4-Test";
    // Je System-Perm 4 UNTERSCHIEDLICHE k-Werte -> je k genau 4x (einmal pro Perm) ueber die 4 Perms.
    EXPECT_EQ(count_occurrences(yaml, "; CHUNK=0\n"), 4u);
    EXPECT_EQ(count_occurrences(yaml, "; CHUNK=1\n"), 4u);
    EXPECT_EQ(count_occurrences(yaml, "; CHUNK=2\n"), 4u);
    EXPECT_EQ(count_occurrences(yaml, "; CHUNK=3\n"), 4u);
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
    EXPECT_EQ(count_occurrences(y1, "# CHILD-SUBMODULE-KLON"), 2u) << "ceb:build + ceb:emit";
    EXPECT_EQ(count_occurrences(y2, "# CHILD-SUBMODULE-KLON"), 4u * planner::kTierChunkCount + 4u)
        << "je tier:build-Job + je scharfem Mess-Job (S5-P2)";
    // W10-Nacharbeit 3: ccache-Env per RUNTIME-Shell-Export im Prolog (1 je Job-mit-Prolog, immun gegen die vererbte,
    // vorexpandierte Parent-CCACHE_DIR). Stufe 1 = 2, Stufe 2 = 16 tier:build + 4 Mess-Jobs (S5-P2) = 20.
    EXPECT_EQ(count_occurrences(y1, "export CCACHE_DIR=\"${CI_PROJECT_DIR}/.ccache\""), 2u);
    EXPECT_EQ(count_occurrences(y2, "export CCACHE_DIR=\"${CI_PROJECT_DIR}/.ccache\""),
              4u * planner::kTierChunkCount + 4u);
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

// (A5b) golden-Neutralitaet: der Default-Walk (leerer Selektor) und der explizite [all]-Selektor (_all_ =
//       cmake_slug("[all]")) liefern eine BYTE-GLEICHE Stufe-2 (die heutige 1-CEB-Strecke traegt die Legende [all]).
//       Ein nicht existierender Selektor liefert eine EHRLICH leere Stufe-2 (0 tier:build-Jobs), keinen Crash.
TEST(SelectMeasurementCombo, GoldenNeutralDefaultEqualsAllSelectorAndMissIsEmpty) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;

    // Default (kein Selektor) vs expliziter [all]-Selektor -> byte-identische Stufe-2 (golden-neutral).
    planner::TierCiYamlBuilder tb_default, tb_all;
    director.construct(*tp, tb_default);
    director.construct(*tp, tb_all, "_all_");
    EXPECT_EQ(tb_default.text(), tb_all.text()) << "leerer Selektor == [all]-Selektor (heutige Live-Strecke)";
    EXPECT_GT(count_occurrences(tb_default.text(), "# JOB tier-build "), 0u) << "Default emittiert Tier-Bau-Jobs";

    // Nicht existierender Selektor -> 0 Kombinationen -> 0 tier:build-Jobs (ehrlich leer, kein Crash).
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
//       symmetrisch zu --emit-tier-ci (A5b). Leerer Selektor = Identitaet (byte-gleich zum expliziten [all]-Selektor
//       _all_ = cmake_slug("[all]"), der die heutige 1-CEB-Strecke traegt); ein nicht existierender Selektor =>
//       ehrlich leere Stufe-2 (0 tier:build-Targets), kein Crash. Golden-neutral: nur die emittierten .cmake-Strings.
TEST(SelectMeasurementCombo, EmitTierCmakeDefaultEqualsAllSelectorAndMissIsEmpty) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;

    // Default (kein Selektor) vs expliziter [all]-Selektor -> byte-identische Stufe-2-cmake (golden-neutral).
    planner::TierCmakeGraphBuilder cm_default, cm_all;
    director.construct(*tp, cm_default);
    director.construct(*tp, cm_all, "_all_");
    EXPECT_EQ(cm_default.text(), cm_all.text()) << "emit-tier-cmake: leerer Selektor == [all]-Selektor (byte-gleich)";
    EXPECT_GT(count_occurrences(cm_default.text(), "add_custom_target(comdare_tier_build_perm"), 0u)
        << "Default emittiert Tier-Bau-Targets";

    // Nicht existierender Selektor -> 0 Kombinationen -> 0 tier:build-Targets (ehrlich leer, kein Crash).
    planner::TierCmakeGraphBuilder cm_none;
    director.construct(*tp, cm_none, "_does_not_exist_");
    EXPECT_EQ(count_occurrences(cm_none.text(), "add_custom_target(comdare_tier_build_perm"), 0u)
        << "kein Selektor-Treffer => ehrlich leere Stufe-2 (0 Tier-Bau-Targets)";
}
