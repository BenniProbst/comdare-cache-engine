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
#include "xml_config_parser/xml_config_parser.hpp"  // XmlConfigParser / ThesisProfile / ExperimentProfile
#include <builder/experiment_tree/slice_marker.hpp> // E-04-P1: kSliceMarkerTraceMarken (Tee-Filter-Single-Source)

#include <gtest/gtest.h>

#include <algorithm> // (W0b-3) std::find ueber die aus dem Plan GELESENE Marken-Menge
#include <array>     // (W0b-3) kTestatKlassifikation
#include <cstddef>
#include <cstdlib> // (i) Byte-Wache: setenv/unsetenv (COMDARE_BUILD_TYPE)
#include <filesystem>
#include <fstream> // I-PMC-2: /proc/cpuinfo als FREMDER Nenner der Real-Probe (T-3)
#include <optional>
#include <stdexcept> // (R5) EXPECT_THROW std::invalid_argument (exactly-one-Haertung)
#include <string>
#include <string_view> // (W0b-3) Marken-Namen der Klassifikation
#include <utility>     // (W0b-3) std::pair der Klassifikation
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
namespace bex     = comdare::cache_engine::builder::experiment; // E-04-P1: Marken der Marker-Familie v2
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
TEST(ExperimentPlanDirector, RegistryTrioLoadsThreeArtRegistriesWith18_3_16) {
    auto const trio =
        tlz::read_axis_registry_trio(fs::path{COMDARE_CE_AXIS_REGISTRY}, fs::path{COMDARE_SYSTEM_AXIS_REGISTRY},
                                     fs::path{COMDARE_MEASUREMENT_AXIS_REGISTRY});
    ASSERT_TRUE(trio.has_value()) << "alle 3 Art-Registries muessen als comdare_axis_registry lesbar sein";

    EXPECT_EQ(trio->organ.engine, "cache_engine");
    EXPECT_EQ(trio->system.engine, "cache_engine_system");
    EXPECT_EQ(trio->measurement.engine, "cache_engine_measurement");

    EXPECT_EQ(trio->organ_axis_count(), 18u)
        << "Organ-golden: 18 Kompositions-Achsen (isa raus INC-2d, persistence_target rein STRUKT-R ORG-18)";
    EXPECT_EQ(trio->system_axis_count(), 3u)
        << "System nach O-8 Schritt 4 (A3-Kern): target_isa/operating_system/external_utils";
    EXPECT_EQ(trio->measurement_category_count(), 16u) << "16 Mess-Kategorien (kMeasurementAxisRegistry)";

    // S2/A2 P-SYSREG (2026-07-20): das System-ANGEBOT traegt genau die kanonischen Haupt-Achsen; target_isa ist
    // als EIGENE Haupt-Achse angeboten (INC-2d, 2 Bausteine x86_64/aarch64) und NUMA (7. Achse) ist korrekt
    // ABWESEND (=S11). atomic128 reist als sub_axis unter compiler (nicht als eigener axis-Key) und wird ueber
    // die Parser-/Validat-Consumer-Verdrahtung + den Byte-Roundtrip (test_system_axis_registry_roundtrip) gedeckt.
    // O-8 Schritt 4/6: compiler ist KEINE System-HAUPT-Achse mehr, sondern Sub-Achse der Komplex-Achse
    // build_target_complex (system_axis_registry.xml: <sub_axis id="compiler" parent="build_target_complex">);
    // scheduling ist analog unter target_isa gewandert. Beide sind damit aus axis_names verschwunden -- die
    // 0u-Erwartungen sind der Beleg dafuer, dass der Umbau vollzogen und nicht nur die Zaehlung gesenkt wurde.
    EXPECT_EQ(trio->system.axis_names.count("compiler"), 0u) << "compiler = Sub-Achse des build_target_complex";
    EXPECT_EQ(trio->system.axis_names.count("scheduling"), 0u) << "scheduling = Sub-Achse unter target_isa";
    EXPECT_EQ(trio->system.axis_names.count("load_framework"), 0u) << "load_framework = Mess-Realm (K1)";
    EXPECT_EQ(trio->system.axis_names.count("operating_system"), 1u);
    EXPECT_EQ(trio->system.axis_names.count("external_utils"), 1u);
    EXPECT_EQ(trio->system.axis_names.count("target_isa"), 1u) << "target_isa = eigene Haupt-System-Achse (INC-2d)";
    EXPECT_EQ(tlz::RegistryTrio::baustein_count(trio->system, "target_isa"), 2u) << "x86_64 + aarch64";
    EXPECT_EQ(trio->system.axis_names.count("numa"), 0u) << "NUMA (7. Achse) korrekt abwesend (=S11)";
}

// (A) Thesis-min: 1 Identitaets-Perm (keine system_axes) x 1 Basis-Pass (keine axis_sweeps) = 1 Schritt.
TEST(ExperimentPlanDirector, ThesisMinYieldsSingleIdentityPermAndSinglePass) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_MIN);
    ASSERT_TRUE(tp.has_value()) << "planner_thesis_min.profile.xml nicht parsbar: " << COMDARE_PLANNER_THESIS_MIN;
    ASSERT_TRUE(tp->compiler.opt_levels.empty()) << "Fixture MUSS ohne <system_axes> sein";
    ASSERT_TRUE(tp->external_utils.simd_options.empty());

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
    ASSERT_EQ(tp->external_utils.simd_options.size(), 2u);
    ASSERT_EQ(tp->axis_sweeps.size(), 17u);

    planner::ExperimentPlanDirector const director;
    CountingBuilder                       cb;
    director.construct(*tp, cb);

    ASSERT_EQ(cb.perms.size(), 4u) << "§64: 1 [all]-Combo x 2 opt x 2 simd = 4 Perms (frueher 3 separate Combos = 12)";
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
    EXPECT_EQ(cb.total_steps(), 72u) << "§64: 4 Perms x 18 Sweep-Passes (1 [all]-Combo, frueher 12 Perms=216)";
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
    ASSERT_EQ(ep->external_utils.simd_options.size(), 2u);

    planner::ExperimentPlanDirector const director;
    CountingBuilder                       cb;
    director.construct(*ep, cb);

    EXPECT_EQ(cb.header.source_kind, "experiment");
    ASSERT_EQ(cb.perms.size(), 4u) << "2x2 Perms";
    for (auto const& steps : cb.steps_per_perm) {
        ASSERT_EQ(steps.size(), 19u) << "3 Phasen: Verbund1=7 + Verbund2=6 + Verbund3=6 (prt_art degeneriert)";
        EXPECT_EQ(steps[0].kind, "experiment_phase_pass");
        EXPECT_EQ(steps[0].label, "phase2_cache_engine");
        EXPECT_EQ(steps[0].merge, "Verbund1_CeOnly");
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
    EXPECT_EQ(cb.header.registries.organ.axis_count, 18u);
    EXPECT_EQ(cb.header.registries.system.engine, "cache_engine_system");
    EXPECT_EQ(cb.header.registries.system.axis_count, 3u); // A3-Kern: 5 -> 3 System-Haupt-Achsen
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
    ASSERT_EQ(trio->organ_axis_count(), 18u) << "RegistryTrio 18/3/16 nach A3-Kern";
    ASSERT_EQ(trio->system_axis_count(), 3u);
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
    ASSERT_EQ(gb.perms().size(), 4u) << "§64: 1 [all]-Combo x 2 opt x 2 simd (frueher 3 Combos=12)";
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
//     1 Aggregat. §64: all_axes_golden traegt jetzt die EINE Vollmengen-Combo [all] {wallclock,macro,micro} => 1
//     ceb:build- + 1 ceb:emit-Target + 1 Kante (die 16 Kategorien reisen je Combo als UNTER mit; die N>1-Auffaecherung
//     bleibt XML-Option, s. MeasurementToolingFanOut/SelectMeasurementCombo mit explizitem 3-Tool-Override).
TEST(CMakeGraphBuilder, EmitsPerComboCebBuildEmitTargetsAndAggregate) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::CMakeGraphBuilder            gb;
    director.construct(*tp, gb);
    std::string const& cmake = gb.text();

    // §64: EINE Vollmengen-Combo [all] => 1 CEB-Bau- + 1 CEB-Emit-Target (frueher 3 separate [wallclock]/[macro]/[micro]).
    EXPECT_EQ(count_occurrences(cmake, "add_custom_target(comdare_ceb_build_"), 1u)
        << "1 CEB-Bau-Target (1 [all]-Combo)";
    EXPECT_EQ(count_occurrences(cmake, "add_custom_target(comdare_ceb_emit_"), 1u)
        << "1 CEB-Emit-Target (1 [all]-Combo)";
    EXPECT_EQ(count_occurrences(cmake, "# ceb:build->ceb:emit-Kante"), 1u) << "eine Bau->Emit-Kante (1 [all]-Combo)";
    // Der CEB-Emit-Schritt ruft "tier cmake" (die CEB emittiert SELBST die Stufe-2, §40.b-Hoheit).
    // P8-REST-ZWILLING (27.07.): geprueft wird die REALE COMMAND-Zeile, nicht eine blosse Erwaehnung -- der Wortlaut
    // steht sonst auch im COMMENT und im Kopf-Kommentar, und der Test waere gegen eine Regression blind.
    EXPECT_NE(cmake.find("\"${COMDARE_PLAN_DRIVER}\" tier cmake "), std::string::npos)
        << "CEB emittiert Stufe-2 selbst, in der Subkommando-Form";
    // Gegenrichtung, die den Zustand ERZWINGT (Zwilling der CI-Wache): das DEPRECATED-Alt-Flag darf NIRGENDS mehr
    // im emittierten Bare-Metal-Plan stehen. Faellt der Alias im Abschluss-Aufraeumpass (Ledger §75), braeche eine
    // Alias-Emission sonst erst beim realen Bare-Metal-Bau.
    EXPECT_EQ(cmake.find("--emit-tier-cmake"), std::string::npos)
        << "Alt-Flag im emittierten Plan: bricht, sobald §75 die Aliase entfernt";
    EXPECT_NE(cmake.find("add_custom_target(comdare_experiment_plan_all DEPENDS"), std::string::npos);
    // Blaupausen-Treue (catalog_codegen.cmake:27-37): add_custom_command + DEPENDS + VERBATIM.
    EXPECT_NE(cmake.find("add_custom_command("), std::string::npos);
    EXPECT_NE(cmake.find("VERBATIM)"), std::string::npos);
    // STUFE 1 baut KEINE Tier-Binaries (kein provision-only-Kommando) -- das ist Stufe-2 ("tier cmake").
    EXPECT_EQ(cmake.find("COMDARE_GOLDEN_N_PROVISION_ONLY=true"), std::string::npos)
        << "Stufe 1 (Planer-Rolle) baut keine Tier-Binaries";
}

// (OP-9) Der Plan weist je step den REAL gebauten Datei-Stem aus -- damit der Auswerter ihn LIEST
//        statt ihn nachzubauen. Additiv unter UNVERAENDERTEM v1.1-Kopf (supers Formatwache pinnt den
//        Anker hart; ein v1.2-Bump waere ohne Zwei-Schritt-Koordination ein rotes CI).
TEST(PlanTextBuilder, StepTraegtDenRealGebautenStemUndDerKopfBleibtV11) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::PlanTextBuilder              tb;
    director.construct(*tp, tb);
    std::string const& txt = tb.text();

    // Der Kopf-Anker MUSS v1.1 bleiben -- das ist die Koordinations-Zusage an super.
    EXPECT_NE(txt.find("# comdare-experiment-plan v1.1"), std::string::npos)
        << "Kopf-Anker gebumpt: supers 2c-Formatwache (kPlanAnchor) schlaegt hart fehl";
    EXPECT_EQ(txt.find("# comdare-experiment-plan v1.2"), std::string::npos);
    // Jede step-Zeile traegt das Feld -- kein Feld bleibt leer (nz-Doktrin der Zeilenform).
    EXPECT_EQ(count_occurrences(txt, " step "), count_occurrences(txt, " built_stem="))
        << "je step genau ein built_stem-Feld";
    // THESIS fuehrt keine binary_ids ("-") -> der Stem ist ehrlich "-", NIE ein erfundener Name.
    EXPECT_NE(txt.find("binary_id=- "), std::string::npos);
    EXPECT_NE(txt.find("built_stem=-"), std::string::npos) << "ohne binary_id darf kein Stem behauptet werden";
}

// (OP-9b) Die Stem-Bildung ist EIN Kanal: der emittierte Wert ist byte-gleich dem, was der
//         Orchestrator beim echten Bau erzeugt -- geprueft gegen orch_make_stem SELBST, nicht
//         gegen eine im Test nachgebaute Regel.
TEST(PlanTextBuilder, BuiltStemIstByteGleichDerOrchestratorFunktion) {
    auto const ep = parse_experiment(COMDARE_EXPERIMENT_GOLDEN);
    ASSERT_TRUE(ep.has_value());
    planner::ExperimentPlanDirector const director;
    planner::PlanTextBuilder              tb;
    director.construct(*ep, tb);
    std::string const& txt = tb.text();

    // Aus dem emittierten Text die Paare (binary_id, built_stem) ziehen und JEDES gegen die reale
    // Orchestrator-Funktion pruefen. Die realen Plan-ids sind kurz (gemessen max. 55 Zeichen), also
    // greift dort der index-unabhaengige Zweig von orch_make_stem -- genau deshalb ist der Wert im
    // Plan ueberhaupt bestimmbar.
    std::size_t geprueft = 0;
    for (std::size_t pos = txt.find("binary_id="); pos != std::string::npos; pos = txt.find("binary_id=", pos + 1)) {
        std::size_t const id_end = txt.find(' ', pos);
        std::string const id     = txt.substr(pos + 10, id_end - pos - 10);
        std::size_t const st     = txt.find("built_stem=", id_end);
        if (id == "-" || st == std::string::npos) continue;
        std::size_t const st_end = txt.find_first_of(" \n", st);
        std::string const stem   = txt.substr(st + 11, st_end - st - 11);
        ASSERT_LE(::comdare::cache_engine::builder::experiment::orch_sanitize(id).size(),
                  ::comdare::cache_engine::builder::experiment::kStemMax)
            << "unerwartet lange Plan-id: dann darf der Plan KEINEN Stem behaupten";
        EXPECT_EQ(stem, ::comdare::cache_engine::builder::experiment::orch_make_stem(id, 0))
            << "Zweit-Implementierung: der Plan-Stem weicht vom Orchestrator-Stem ab (id=" << id << ")";
        ++geprueft;
    }
    EXPECT_GT(geprueft, 0u) << "das Experiment-Profil muss echte binary_ids fuehren, sonst prueft der Test nichts";
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

// (J) STUFE 2 (W10-A/§42.b + S4-§62-B-Batch, TierCmakeGraphBuilder): je-Host-Aggregat-Targets comdare_tier_batch_
//     <host> (per-Perm Provision- + S3-Pruef-Kommandos, SCHARF) + je-Host-Mess-Target comdare_tier_measure_<host>
//     (misst real). Bare-Metal-Spiegel des CI-Batch (Dual-Weg §61).
TEST(TierCmakeGraphBuilder, TierBatchIsProvisionPruefPerHostAndMeasureIsSharp) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::TierCmakeGraphBuilder        gb;
    director.construct(*tp, gb);
    std::string const& cmake = gb.text();

    // §62-B-Batch: all_axes_golden [all] -> 4 Perms; measure_host_lane -> amd={O2/O3,no_ext} (2), intel={O2/O3,avx2}
    // (2). Je Host EIN Build+Pruef-Aggregat comdare_tier_batch_<host> (2) + EIN Mess-Target comdare_tier_measure_
    // <host> (2). Provision-only-Kommandos = 1 je Perm (Release) = 4; S3-Pruef-Kommandos = 1 je Perm = 4.
    EXPECT_EQ(count_occurrences(cmake, "add_custom_target(comdare_tier_batch_"), 2u)
        << "je Host-Lane EIN Build+Pruef-Aggregat-Target (amd + intel)";
    EXPECT_EQ(count_occurrences(cmake, "add_custom_target(comdare_tier_measure_"), 2u)
        << "je Host-Lane EIN scharfes Mess-Target (amd + intel)";
    EXPECT_EQ(count_occurrences(cmake, "COMDARE_GOLDEN_N_PROVISION_ONLY=true"), 4u)
        << "je Perm EIN provision-only-Kommando (Release; 4 Perms) -- die Mess-Kommandos provisionieren NICHT";
    EXPECT_EQ(count_occurrences(cmake, "\"COMDARE_PRUEF_ONLY=true\""), 4u)
        << "je Perm EIN S3-Konformitaets-Gate-Kommando (COMDARE_PRUEF_ONLY=true; 4 Perms)";
    EXPECT_NE(cmake.find("\"COMDARE_GN_OPT=O2\""), std::string::npos) << "opt/simd als Plan-Konstanten (LITERALE)";
    EXPECT_NE(cmake.find("\"COMDARE_GN_SIMD=avx2\""), std::string::npos);
    // Host-unabhaengig: konfigurierbare Eingaben als CMake-Variablen mit Defaults, KEINE Host-Absolutpfade.
    EXPECT_NE(cmake.find("if(NOT DEFINED COMDARE_PLAN_DRIVER)"), std::string::npos);
    EXPECT_NE(cmake.find("if(NOT DEFINED COMDARE_PLAN_RANGE)"), std::string::npos);
    EXPECT_EQ(cmake.find("/home/"), std::string::npos) << "keine emit-Zeit-Host-Absolutpfade im .cmake";
    // DEPRECATED: keine per-Perm-Chunk-Targets mehr (§56/§57 Chunk-Konzept in S4 abgeloest).
    EXPECT_EQ(cmake.find("add_custom_target(comdare_tier_build_perm"), std::string::npos)
        << "keine per-(Perm x chunk)-Targets mehr (§62-B-Batch)";
    // S5-P2 SCHARF: realer Treiber-Aufruf nach measure/. §61-MODI: DLL-Bau PARALLEL (COMDARE_PLAN_MEASURE_PARALLEL),
    // Messen 1-Thread. Je Perm EIN Mess-Kommando (4 Perms).
    EXPECT_NE(cmake.find("measure (S5-P2 scharf, misst)"), std::string::npos) << "Mess-Target ist scharf (misst real)";
    EXPECT_NE(cmake.find("${COMDARE_PLAN_OUT}/measure/"), std::string::npos)
        << "scharfer Treiber-Aufruf schreibt EIN CSV je Zelle nach measure/";
    EXPECT_EQ(count_occurrences(cmake, "COMDARE_BUILD_PARALLEL=${COMDARE_PLAN_MEASURE_PARALLEL}"), 4u)
        << "je Perm-Mess-Kommando DLL-Bau parallel (§61-MODI; 4 Perms)";
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
    ASSERT_EQ(yb.perms().size(), 4u) << "§64: 1 [all]-Combo x 2 opt x 2 simd (frueher 3 Combos=12)";
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

    // §64: EINE Vollmengen-Combo [all] => je 1 ceb:build/emit/trigger-Job (Marker-Kommentare, kollisionsfrei zu needs).
    EXPECT_EQ(count_occurrences(yaml, "# JOB ceb-build combo "), 1u) << "1 CEB-Bau-Job (1 [all]-Combo)";
    EXPECT_EQ(count_occurrences(yaml, "# JOB ceb-emit combo "), 1u) << "1 CEB-Emit-Job ('tier ci', 1 [all]-Combo)";
    EXPECT_EQ(count_occurrences(yaml, "# JOB ceb-trigger combo "), 1u) << "1 Grandchild-Trigger-Job (1 [all]-Combo)";
    // §64: die Vollmenge kollabiert auf die Legende [all] -> der EINE ceb:build/emit/trigger:[all]-Job. Die 3 distinkten
    // [wallclock]/[macro]/[micro]-Legenden bleiben XML-Option (s. MeasurementToolingFanOut/SelectMeasurementCombo).
    EXPECT_NE(yaml.find("\"ceb:build:[all]\":"), std::string::npos) << "ceb:build:[all]-Strecke (Vollmenge vereint)";
    EXPECT_NE(yaml.find("\"ceb:emit:[all]\":"), std::string::npos);
    EXPECT_NE(yaml.find("\"ceb:trigger:[all]\":"), std::string::npos);
    EXPECT_EQ(yaml.find("[wallclock]"), std::string::npos)
        << "§64: keine separaten Tool-Lanen im Default (vereint als [all])";
    // Die CEB emittiert SELBST die Stufe-2 (§40.b-Hoheit: 'tier ci', nicht 'plan ci').
    // P8-REST (27.07.): geprueft wird der REALE Aufruf, nicht eine blosse Erwaehnung -- 'tier ci' kommt sonst
    // auch in den beschreibenden Kommentarzeilen vor und der Test waere gegen eine Regression blind.
    EXPECT_NE(yaml.find("\"$DRIVER\" tier ci "), std::string::npos)
        << "CEB-Hoheit: die CEB emittiert Stufe-2 selbst, in der Subkommando-Form";
    // Und die Gegenrichtung, die den Zustand ERZWINGT: das DEPRECATED-Alt-Flag darf NIRGENDS mehr im
    // emittierten YAML stehen. Faellt der Alias im Abschluss-Aufraeumpass (Ledger §75), braeche eine
    // Alias-Emission genau dann -- und zwar erst im CI. Dieser Test faengt es vorher.
    EXPECT_EQ(yaml.find("--emit-tier-ci"), std::string::npos)
        << "Alt-Flag im emittierten Child-YAML: bricht, sobald §75 die Aliase entfernt";
    // Genau EIN trigger:-Schluessel (Grandchild via include: artifact:) je Kombination.
    EXPECT_EQ(count_occurrences(yaml, "\n  trigger:\n"), 1u)
        << "je Kombination ein trigger:-Schluessel (1 [all]-Combo)";
    EXPECT_EQ(count_occurrences(yaml, "include:\n      - artifact:"), 1u);
    // Zweistufige stages-Struktur (genau einmal, im Kopf).
    EXPECT_EQ(count_occurrences(yaml, "\nstages:\n"), 1u);
    EXPECT_NE(yaml.find("  - ceb-build\n"), std::string::npos);
    EXPECT_NE(yaml.find("  - ceb-emit\n"), std::string::npos);
    // STUFE 1 (Planer-Rolle) baut KEINE Tier-Binaries (kein provision-only) -- das ist Stufe-2 ('tier ci').
    EXPECT_EQ(yaml.find("COMDARE_GOLDEN_N_PROVISION_ONLY=true"), std::string::npos)
        << "Stufe 1 baut keine Tier-Binaries";
}

// (L2) S1/A1 P-TOTAL (Ledger 46) + S2-NACHT-2 (2026-07-23, Zirkularitaets-Fix): der ceb:trigger-Bridge-Job forwardet
//      COMDARE_GN_TOTAL/COMDARE_MEASURE_PROFILE/COMDARE_PLAN_METHODIK_PROFILE als EXPLIZITE Allowlist an die STUFE-2-
//      Grandchild (self-contained Grandchild-Pipelines erben Pipeline-Variablen NICHT). Die RHS wird zur EMISSIONSZEIT
//      LITERAL aus der Planer-Env eingebrannt -- NICHT NAME: "$NAME": diese Selbst-Referenz wertete GitLab im Child als
//      'circular variable reference' aus (config_error, leeres failed-Child; Befund Struktur-Smoke 12628/12663). Leer/
//      ungesetzt => Zeile ENTFAELLT (eine leere YAML-Variable ueberschriebe die Grandchild-'nicht gesetzt'-Semantik).
//      BEWUSSTE Aenderung ggue. dem alten $VAR-Pin (S2-NACHT-2). Env via setenv/unsetenv gesteuert (Hygiene am Ende).
// G4a (2026-07-26): die sechs ZUSAETZLICH forwardeten Opt-in-Variablen (Storage + Lager/Gate). Sie muessen in JEDEM
// Allowlist-Test aktiv weggeraeumt werden -- eine geerbte Umgebung (Entwickler-Shell, Runner mit gesetztem
// COMDARE_BESTANDSLOG) wuerde sonst einen variables:-Block erzeugen und den Absenz-Zweig (3b) unten falsch rot faerben.
inline constexpr char const* kG4aForwardedOptIns[] = {"COMDARE_STORAGE_CACHE",        "COMDARE_BESTANDSLOG",
                                                      "COMDARE_BESTANDSLOG_DOC_KEY",  "COMDARE_BESTANDSLOG_OWNER_UUID",
                                                      "COMDARE_BESTANDSLOG_MASCHINE", "COMDARE_VARIANT_GATE"};

inline void unset_g4a_forwarded_opt_ins() {
    for (char const* n : kG4aForwardedOptIns) ::unsetenv(n);
}

TEST(CiYamlBuilder, CebTriggerForwardsAllowlistAsEmissionTimeLiteralsNoSelfRef) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    unset_g4a_forwarded_opt_ins(); // G4a: geerbte Opt-ins duerfen diesen Test nicht verfaelschen

    // (2) Gesetzte Env (smoke-typisch) => die drei LITERALE, KEIN $-Ref. §64: EINE Vollmengen-Combo => genau 1 Block.
    ::setenv("COMDARE_GN_TOTAL", "131072", 1);
    ::setenv("COMDARE_MEASURE_PROFILE", "smoke", 1);
    ::setenv("COMDARE_PLAN_METHODIK_PROFILE", "m3_smoke_coverage.profile.xml", 1);
    {
        planner::CiYamlBuilder yb;
        director.construct(*tp, yb);
        std::string const& yaml = yb.text();
        EXPECT_EQ(count_occurrences(yaml, "  variables:\n    COMDARE_GN_TOTAL: \"131072\"\n"), 1u)
            << "GN_TOTAL LITERAL eingebrannt (kein $-Ref; 1 [all]-Combo)";
        EXPECT_NE(yaml.find("    COMDARE_MEASURE_PROFILE: \"smoke\"\n"), std::string::npos)
            << "MEASURE_PROFILE LITERAL (Smoke-Auto-Run-Propagation)";
        EXPECT_NE(yaml.find("    COMDARE_PLAN_METHODIK_PROFILE: \"m3_smoke_coverage.profile.xml\"\n"),
                  std::string::npos)
            << "METHODIK-Basename LITERAL (smoke=>debug-Entkopplung)";
        // (1) KEINE Selbst-Referenz-Form NAME: "$NAME" mehr (der Zirkularitaets-Ausloeser).
        EXPECT_EQ(yaml.find("COMDARE_GN_TOTAL: \"$COMDARE_GN_TOTAL\""), std::string::npos)
            << "keine Selbst-Referenz mehr (Zirkularitaets-Fix)";
        EXPECT_EQ(yaml.find("COMDARE_MEASURE_PROFILE: \"$COMDARE_MEASURE_PROFILE\""), std::string::npos);
        EXPECT_EQ(yaml.find("COMDARE_PLAN_METHODIK_PROFILE: \"$COMDARE_PLAN_METHODIK_PROFILE\""), std::string::npos);
        // forward-Mechanismus unveraendert: yaml_variables:true, pipeline_variables:false, genau 1 trigger.
        EXPECT_NE(yaml.find("    forward:\n      yaml_variables: true"), std::string::npos)
            << "forward:yaml_variables:true reicht die Allowlist an die Grandchild";
        EXPECT_NE(yaml.find("      pipeline_variables: false"), std::string::npos)
            << "KEIN blindes Erben des gesamten Eltern-Variablenraums";
        EXPECT_EQ(yaml.find("pipeline_variables: true"), std::string::npos)
            << "pipeline_variables:true ist verboten (Modul-Trigger-Isolation)";
        EXPECT_EQ(count_occurrences(yaml, "\n  trigger:\n"), 1u) << "EIN Grandchild-Trigger (1 [all]-Combo)";
    }

    // (3) MEASURE_PROFILE + METHODIK ungesetzt => deren Zeilen FEHLEN komplett; GN_TOTAL bleibt (gesetzt).
    ::unsetenv("COMDARE_MEASURE_PROFILE");
    ::unsetenv("COMDARE_PLAN_METHODIK_PROFILE");
    {
        planner::CiYamlBuilder yb;
        director.construct(*tp, yb);
        std::string const& yaml = yb.text();
        EXPECT_NE(yaml.find("    COMDARE_GN_TOTAL: \"131072\"\n"), std::string::npos) << "GN_TOTAL bleibt (gesetzt)";
        // Absenz-Check auf die YAML-Variablen-Zeilenform "    NAME: \"" (4-Space-Indent) -- die kommt NUR im Trigger-
        // variables-Block vor (die Prolog-Shell-Nutzung ${..:-} bzw. die $VAR-Rule tragen KEIN "    NAME: \"").
        EXPECT_EQ(yaml.find("    COMDARE_MEASURE_PROFILE: \""), std::string::npos)
            << "ungesetzt => Zeile entfaellt komplett (keine leere Variable)";
        EXPECT_EQ(yaml.find("    COMDARE_PLAN_METHODIK_PROFILE: \""), std::string::npos)
            << "ungesetzt => Zeile entfaellt komplett (keine leere Variable)";
    }

    // (3b) ALLE drei ungesetzt => KEIN Trigger-variables-Block (leerer Block ausgelassen); trigger-Struktur bleibt.
    ::unsetenv("COMDARE_GN_TOTAL");
    {
        planner::CiYamlBuilder yb;
        director.construct(*tp, yb);
        std::string const& yaml = yb.text();
        EXPECT_EQ(yaml.find("\n  variables:\n"), std::string::npos)
            << "alle ungesetzt => kein (2-space-indentierter) Trigger-variables-Block";
        EXPECT_EQ(count_occurrences(yaml, "\n  trigger:\n"), 1u) << "trigger-Struktur bleibt (keine Regression)";
    }

    // Env-Hygiene: alle drei wieder entfernt (kein Leak in Folgetests).
    ::unsetenv("COMDARE_GN_TOTAL");
    ::unsetenv("COMDARE_MEASURE_PROFILE");
    ::unsetenv("COMDARE_PLAN_METHODIK_PROFILE");
    unset_g4a_forwarded_opt_ins();
}

// (L2-G4a) Die G4a-Erweiterung derselben Allowlist: der Storage-Schalter (P-A) und die vier Lager- plus die
// Variant-Gate-Variable reisen nach DEMSELBEN emissions-gateten Muster ueber die zweite Trigger-Grenze. Ungesetzt =>
// Zeile entfaellt => die emittierte YAML ist byte-identisch zum Stand vor G4a (das ist die Byte-Neutralitaets-Zusage).
// Zusaetzlich die KLASSEN-Wache: kein Credential-Name darf je an dieser Naht auftauchen.
TEST(CiYamlBuilder, CebTriggerForwardsStorageAndLagerGateOptInsEmissionGated) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;

    // (a) ALLE sechs ungesetzt (und die drei Alt-Variablen ebenso) => KEIN variables:-Block, YAML wie vor G4a.
    ::unsetenv("COMDARE_GN_TOTAL");
    ::unsetenv("COMDARE_MEASURE_PROFILE");
    ::unsetenv("COMDARE_PLAN_METHODIK_PROFILE");
    unset_g4a_forwarded_opt_ins();
    std::string baseline;
    {
        planner::CiYamlBuilder yb;
        director.construct(*tp, yb);
        baseline = yb.text();
        EXPECT_EQ(baseline.find("\n  variables:\n"), std::string::npos)
            << "alle Opt-ins ungesetzt => kein Trigger-variables-Block (byte-identisch zu vor G4a)";
        for (char const* n : kG4aForwardedOptIns)
            EXPECT_EQ(baseline.find(std::string{"    "} + n + ": \""), std::string::npos)
                << "ungesetzt => Zeile entfaellt komplett: " << n;
    }

    // (b) Alle sechs gesetzt => je genau EINE Literal-Zeile, kein $-Selbstbezug.
    ::setenv("COMDARE_STORAGE_CACHE", "true", 1);
    ::setenv("COMDARE_BESTANDSLOG", "true", 1);
    ::setenv("COMDARE_BESTANDSLOG_DOC_KEY", "lager/bestand.xml", 1);
    ::setenv("COMDARE_BESTANDSLOG_OWNER_UUID", "6f1c2b3a-0000-4444-8888-abcdefabcdef", 1);
    ::setenv("COMDARE_BESTANDSLOG_MASCHINE", "prod1", 1);
    ::setenv("COMDARE_VARIANT_GATE", "true", 1);
    {
        planner::CiYamlBuilder yb;
        director.construct(*tp, yb);
        std::string const& yaml = yb.text();
        EXPECT_EQ(count_occurrences(yaml, "    COMDARE_STORAGE_CACHE: \"true\"\n"), 1u);
        EXPECT_EQ(count_occurrences(yaml, "    COMDARE_BESTANDSLOG: \"true\"\n"), 1u);
        EXPECT_EQ(count_occurrences(yaml, "    COMDARE_BESTANDSLOG_DOC_KEY: \"lager/bestand.xml\"\n"), 1u);
        EXPECT_EQ(
            count_occurrences(yaml, "    COMDARE_BESTANDSLOG_OWNER_UUID: \"6f1c2b3a-0000-4444-8888-abcdefabcdef\"\n"),
            1u);
        EXPECT_EQ(count_occurrences(yaml, "    COMDARE_BESTANDSLOG_MASCHINE: \"prod1\"\n"), 1u);
        EXPECT_EQ(count_occurrences(yaml, "    COMDARE_VARIANT_GATE: \"true\"\n"), 1u);
        for (char const* n : kG4aForwardedOptIns)
            EXPECT_EQ(yaml.find(std::string{n} + ": \"$"), std::string::npos) << "keine Selbst-Referenz: " << n;
    }

    // (c) KLASSEN-Wache (§ Credential-Verbot): die Geheimnis-Namen duerfen an KEINER Stelle der YAML stehen -- weder
    // als Forward-Zeile noch sonstwo. Sie werden ausschliesslich maschinenlokal von comdare_storage_activation.sh
    // gefaltet. Auch mit gesetzter Umgebung geprueft, damit ein spaeteres versehentliches Forwarden sofort rot wird.
    {
        ::setenv("MINIO_ACCESS_KEY", "AKIA-TESTONLY", 1);
        ::setenv("MINIO_SECRET_KEY", "secret-testonly", 1);
        ::setenv("COMDARE_NFS_DROP_TOKEN", "token-testonly", 1);
        planner::CiYamlBuilder yb;
        director.construct(*tp, yb);
        std::string const& yaml = yb.text();
        for (char const* secret : {"MINIO_ACCESS_KEY", "MINIO_SECRET_KEY", "COMDARE_NFS_DROP_TOKEN"})
            EXPECT_EQ(yaml.find(secret), std::string::npos) << "Credential-Name darf NIE in die YAML: " << secret;
        for (char const* val : {"AKIA-TESTONLY", "secret-testonly", "token-testonly"})
            EXPECT_EQ(yaml.find(val), std::string::npos) << "Credential-WERT darf NIE in die YAML: " << val;
        ::unsetenv("MINIO_ACCESS_KEY");
        ::unsetenv("MINIO_SECRET_KEY");
        ::unsetenv("COMDARE_NFS_DROP_TOKEN");
    }

    unset_g4a_forwarded_opt_ins(); // Env-Hygiene
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

    // all_axes_golden traegt no_extension + avx2. Die STUFE-1-YAML (CiYamlBuilder, CEB-Bau) ist compiler-only =>
    // broadest amd64. Die STUFE-2-YAML (S4-§62-B-Batch, TierCiYamlBuilder) taggt je HOST-LANE (amd/intel, s.
    // measure_host_lane) statt der reinen simd-Faehigkeit -- die Lane-Zuordnung deckt die avx512->amd-Zwangsregel ab.
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::CiYamlBuilder                yb;
    director.construct(*tp, yb);
    EXPECT_NE(yb.text().find("tags: [\"amd64\"]"), std::string::npos) << "Stufe 1: CEB-Bau amd64 (broadest)";
    EXPECT_EQ(yb.text().find("tags: [\"avx2\"]"), std::string::npos) << "Stufe 1 routet NICHT per SIMD";
    planner::TierCiYamlBuilder tb;
    director.construct(*tp, tb);
    EXPECT_NE(tb.text().find("tags: [\"amd\"]"), std::string::npos) << "Stufe 2 Batch: no_extension-Perm -> amd-Lane";
    EXPECT_NE(tb.text().find("tags: [\"intel\"]"), std::string::npos) << "Stufe 2 Batch: avx2-Perm -> intel-Lane";
    EXPECT_EQ(tb.text().find("tags: [\"avx2\"]"), std::string::npos)
        << "Stufe 2 Batch taggt je Host-Lane (amd/intel), nicht per flag-granularem simd-Tag";
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

    // Stufe-2-YAML (S4-§62-B-Batch): eine avx512-Perm wird via measure_host_lane ZWINGEND in den amd-Bucket geroutet
    // (nur prod1/Zen5 traegt avx512) -> der Build+Pruef-Batch traegt tags: ["amd"] (Host-Lane), NICHT den flag-
    // granularen "avx512f"-Tag (der galt der abgeloesten Einzel-Job-Ebene). Das Treiber-ISA-Gate ist die zweite Wache.
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
    EXPECT_NE(tb.text().find("tags: [\"amd\"]"), std::string::npos)
        << "Stufe-2 Batch: avx512-Perm -> amd-Lane (Hardware-Zwang, measure_host_lane)";
    EXPECT_EQ(tb.text().find("tags: [\"avx512"), std::string::npos)
        << "kein flag-granularer avx512-Tag mehr in der Batch-Emission (Host-Lane-Tag)";
    EXPECT_NE(tb.text().find("\"tier:build-batch:amd\":"), std::string::npos)
        << "die avx512-Perm landet im amd-Build-Batch";
    EXPECT_EQ(tb.text().find("\"tier:build-batch:intel\":"), std::string::npos)
        << "Leere-Lane-Regel: kein intel-Batch (avx512 nie intel)";
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
    // §62-B-Batch (S4): die Job-Namen sind O(Maschinen). Der Build+Pruef-Batch traegt NUR die Host-Lane; der
    // Mess-Batch traegt CEB-Identitaet [a,b,c] + Lane. (Der fruehere tier_build_job-Chunk-Helper entfiel in S4.)
    EXPECT_EQ(lg::tier_batch_build_job("amd"), "tier:build-batch:amd");
    EXPECT_EQ(lg::tier_batch_build_job("intel"), "tier:build-batch:intel");
    EXPECT_EQ(lg::measure_batch_job("[all]", "amd"), "measure:[all]:batch:amd");
    // measure_job bleibt der Legenden-/Doku-Helper der vollen Mess-Zelle (drei Klammern).
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

    // §64: all_axes_golden traegt die EINE Vollmengen-Combo [all] {wallclock,macro,micro}; die 16 Kategorien reisen je
    // Combo als UNTER mit (die HAUPT-Legende kommt aus dem Tooling, NICHT aus den Kategorien).
    ASSERT_EQ(cb.combos.size(), 1u) << "§64: EINE Vollmengen-Combo (frueher 3 separate {wallclock/macro/micro})";
    EXPECT_EQ(cb.header.measurement_combo_count, 1u);
    EXPECT_EQ(cb.combos[0].legend, "[all]") << "die Vollmengen-Combo kollabiert auf die Legende [all]";
    // Die System-Perms bleiben byte-identisch (4 Perms je Combo -> 1 x 4 = 4 Perms total).
    ASSERT_EQ(cb.perms.size(), 4u) << "2 opt x 2 simd je Mess-Kombination x 1 [all]-Combo (frueher x3 = 12)";
    EXPECT_EQ(cb.header.perm_count, 4u) << "perm_count = |opt x simd| JE Mess-Kombination (unveraendert)";
}

// (W3) --dump-plan zeigt die Mess-Achsen-Stufe sichtbar (measurement_combo-Zeile + count-Kopf).
TEST(PlanTextBuilder, DumpPlanShowsMeasurementComboStage) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::PlanTextBuilder              pt;
    director.construct(*tp, pt);
    EXPECT_NE(pt.text().find("measurement_combo_count=1"), std::string::npos);
    EXPECT_NE(pt.text().find("measurement_combo 0 legend=[all]"), std::string::npos)
        << "§64: EINE Vollmengen-Combo [all]";
    EXPECT_NE(pt.text().find("perm_count=4"), std::string::npos) << "Perm-Ebene bleibt unter der Kombination";
}

// (W4) Stufe 2 (CEB-Rolle, TierCiYamlBuilder, S4-§62-B-Batch): je Host-Lane GENAU EIN Build+Pruef-Batch + EIN
//      Mess-Batch, O(Maschinen). all_axes_golden [all]: amd={no_ext-Perms}, intel={avx2-Perms} => exakt 4 Jobs.
TEST(TierCiYamlBuilder, EmitsOneBuildBatchAndOneMeasureBatchPerHost) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    // O(Maschinen): je Host EIN Build+Pruef-Batch + EIN Mess-Batch => bei 2 nicht-leeren Lanes exakt 4 Jobs.
    EXPECT_EQ(count_occurrences(yaml, "# JOB tier-build-batch "), 2u)
        << "je Host-Lane EIN Build+Pruef-Batch (amd+intel)";
    EXPECT_EQ(count_occurrences(yaml, "# JOB measure-batch "), 2u) << "je Host-Lane EIN Mess-Batch (amd+intel)";
    // Die Job-Namen-Literale (Build+Pruef-Batch traegt NUR die Host-Lane; Mess-Batch traegt CEB-Identitaet + Lane).
    EXPECT_NE(yaml.find("\"tier:build-batch:amd\":"), std::string::npos);
    EXPECT_NE(yaml.find("\"tier:build-batch:intel\":"), std::string::npos);
    EXPECT_NE(yaml.find("\"measure:[all]:batch:amd\":"), std::string::npos);
    EXPECT_NE(yaml.find("\"measure:[all]:batch:intel\":"), std::string::npos);
    // needs: EINE Bau->Mess-Kante je Mess-Batch auf den Build-Batch derselben Lane (statt 4 Chunk-Kanten).
    EXPECT_NE(yaml.find("    - \"tier:build-batch:amd\"\n"), std::string::npos)
        << "needs:-Kante des amd-Mess-Batch referenziert den amd-Build-Batch";
    EXPECT_NE(yaml.find("    - \"tier:build-batch:intel\"\n"), std::string::npos);
    // Die zwei Stufen-2-stages.
    EXPECT_NE(yaml.find("  - tier-build\n"), std::string::npos);
    EXPECT_NE(yaml.find("  - measure\n"), std::string::npos);
    // KEINE Stufe-1-CEB-Jobs in der Stufe-2-Sicht (die CEB-Jobs gehoeren in --dump-ci).
    EXPECT_EQ(yaml.find("ceb:build:"), std::string::npos) << "Stufe 2 enthaelt KEINE CEB-Bau-Jobs";
    EXPECT_EQ(yaml.find("stage: ceb-build"), std::string::npos);
    // §62-B-Batch: KEINE Chunk-Job-Namen mehr in der Stufe-2-YAML (die per-(Perm x chunk<k>)-Kette entfiel).
    EXPECT_EQ(yaml.find("chunk"), std::string::npos) << "kein chunk-Text in der Batch-Stufe-2 (§62-B)";
    // provision-only-Kommando 1 je Perm (im Batch-Script, in der Fenster-Schleife); 4 Perms total.
    EXPECT_EQ(count_occurrences(yaml, "COMDARE_GOLDEN_N_PROVISION_ONLY=true"), 4u)
        << "je Perm EIN provision-only-Kommando (4 Perms; frueher 16)";
    // S3-PRUEF-Schritt 1 je Perm (unbedingt nach der Fenster-Schleife emittiert).
    EXPECT_EQ(count_occurrences(yaml, "COMDARE_PRUEF_ONLY=true"), 4u) << "je Perm EIN S3-Konformitaets-Gate";
}

// (W5) §42.b Bau=Haupt-only-Gate (S4-§62-B-Batch): KEINE Unter-Achse (Laufzeit-Parameter) in irgendeiner [BAU]-
//      Schritt-Legende; §62-B-NACHTRAG: die [BAU]-/[PRUEF]-Testate tragen zelle=[d,e,f][g,h,i] (ZWEI Klammern, kein
//      [a,b,c]); nur die [MESS]-Testate tragen alle drei Klammern. Mess ist SCHARF (misst real), gegatet ueber rules.
TEST(TierCiYamlBuilder, BuildLegendsCarryNoSubAxisAndMeasureIsSharp) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    // §42.b Bau=Haupt-only-Gate: KEINE Unter-Achse in irgendeiner [BAU]-Schritt-Legende (reine Laufzeit-Parameter).
    // Wir scannen jede "[BAU] zelle="-Echo-Zeile auf die bekannten dynamischen Unter-Achsen-Dimensionen.
    std::size_t bau_lines = 0;
    for (char const* sub :
         {"thread_count", "prefetch_distance", "pool_budget", "batch_size", "inline_threshold", "workload"}) {
        std::size_t pos = 0;
        while ((pos = yaml.find("[BAU] zelle=", pos)) != std::string::npos) {
            std::size_t const eol  = yaml.find('\n', pos);
            std::string const line = yaml.substr(pos, eol - pos);
            EXPECT_EQ(line.find(sub), std::string::npos) << "Bau=Haupt-only-Gate (§42.b): Unter-Achse '" << sub
                                                         << "' darf NICHT in der [BAU]-Legende stehen: " << line;
            // §62-B-NACHTRAG: die [BAU]-Zelle traegt KEINE Mess-Kombination [a,b,c] (Layer NIE verschmolzen).
            EXPECT_EQ(line.find("[all]"), std::string::npos)
                << "[BAU]-Schritt-Legende traegt KEINE CEB-Kombination [a,b,c] (nur KOPF): " << line;
            ++bau_lines;
            pos = eol;
        }
    }
    EXPECT_GT(bau_lines, 0u) << "es gibt [BAU]-Schritt-Echos je Perm";
    // §62-B-NACHTRAG: der Batch-KOPF (einmal je Job) traegt die CEB-Identitaet [a,b,c] + Lane; die MESS-Testate
    // tragen alle drei Klammern. Beide Ebenen sind praesent, aber die Layer bleiben getrennt.
    EXPECT_NE(yaml.find("[BATCH-BAU] ceb=[all] lane=amd"), std::string::npos) << "Batch-KOPF traegt [a,b,c] + Lane";
    EXPECT_NE(yaml.find("[MESS] zelle=[all][O2,no_extension][search_algo,cache_traversal,mapping]"), std::string::npos)
        << "MESS-Testat traegt alle drei Klammern [a,b,c][d,e,f][g,h,i]";
    // rules (§41/320er): je Mess-Batch EINE smoke-Auto-Run-Regel + EINE when:manual-Fallback-Regel. 2 Mess-Batches.
    EXPECT_EQ(count_occurrences(yaml, "    - when: manual"), 2u)
        << "je Mess-Batch ein when:manual-Fallback (2 Host-Lanes)";
    EXPECT_EQ(count_occurrences(yaml, "    - if: '$COMDARE_MEASURE_PROFILE == \"smoke\"'"), 2u)
        << "je Mess-Batch eine smoke-Auto-Run-Regel";
    EXPECT_NE(yaml.find("320er"), std::string::npos) << "Gate-Provenienz (§41/320er) dokumentiert";
    // P4 (§62-B): resource_group ceb-measure-<host> je Maschine geteilt zwischen Build-Batch UND Mess-Batch -> je
    // Host genau 2 Vorkommen (1 Build-Batch + 1 Mess-Batch), NICHT umbenannt.
    EXPECT_EQ(count_occurrences(yaml, "  resource_group: \"ceb-measure-amd\""), 2u)
        << "amd-Lane: Build-Batch + Mess-Batch teilen ceb-measure-amd (P4)";
    EXPECT_EQ(count_occurrences(yaml, "  resource_group: \"ceb-measure-intel\""), 2u)
        << "intel-Lane: Build-Batch + Mess-Batch teilen ceb-measure-intel (P4)";
    EXPECT_EQ(yaml.find("ceb-measurement-exclusive"), std::string::npos)
        << "keine globale Mess-Serialisierung mehr (§61-MODI: prod1+prod2 messen parallel)";
    // S5-P2 FLIP: der reale Mess-Vollzug schreibt EIN CSV je Zelle nach measure_out.
    EXPECT_NE(yaml.find("$CI_PROJECT_DIR/Code/measure_out/"), std::string::npos)
        << "scharfer Mess-Aufruf schreibt nach measure_out";
    // §62-B Lane-Budget (T-Wert, NICHT $(nproc)): der Mess-Batch exportiert COMDARE_BUILD_PARALLEL als Lane-Literal.
    EXPECT_EQ(yaml.find("$(nproc)"), std::string::npos) << "§62-B Lane-Budget-T-Wert-Literale ersetzen $(nproc)";
    EXPECT_EQ(yaml.find("export COMDARE_BUILD_PARALLEL=1"), std::string::npos)
        << "kein serialisierter Bau mehr (§61-MODI: der alte =1 war eine Regression)";
    EXPECT_EQ(yaml.find("COMDARE_RUN_MEASURE=true"), std::string::npos)
        << "COMDARE_RUN_MEASURE hat null Konsumenten -> nie emittiert";
}

// -- #278 / OV-16: KEIN allow_failure in irgendeiner EMITTIERTEN Job-YAML (beide CI-Stufen). -----------------
// SELBSTCHECK: dieser Test beisst genau dann, wenn ein Emissionsblock das Wort 'allow_failure' in die Job-YAML
//   schreibt, und NUR dann -- kein anderer Test dieses Baums nennt es (gegengeprueft 2026-08-09:
//   `git grep -c allow_failure -- tests/` = 0 Treffer, rc=1). Vor dem Entfernen von
//   experiment_plan_director.hpp emit_batch_measure_job:"  allow_failure: true\n" war er ROT (2 Treffer,
//   1 je Host-Lane); danach gruen. Ein Wiedereinbau macht ihn sofort wieder rot -- der mutierte Zweig ist
//   also beobachtbar und wird von keiner anderen Zusicherung verdeckt.
// PROVENIENZ: Owner 2026-07-06 14:16:43 UTC "bei einer harten Pipeline darf es kein allow failure geben";
//   Verschaerfung 2026-07-17 "die gesamte Pipeline IMMER hart gruen"; 2026-07-26 "Pipelines muessen hart gruen
//   durchlaufen, da wird nichts unterbrochen"; Bestaetigung 2026-08-09 "Allow failure war schon IMMER verboten.
//   Wenn dann muss ein Fehler sauber mit einer Warnung an den Anwender angezeigt und die Messung uebersprungen
//   werden, aber der CI-Job failed immer hart."
// ZWEI EBENEN, hier BEIDE geprueft (sie duerfen nie wieder verschmelzen -- genau daran entstand der Defekt:
//   Commit b5e64a51c 2026-07-19 fuegte das Flag auf JOB-Ebene ein, 0d91dc1e3 2026-07-20 klebte den Kommentar
//   "Sichtbarkeits-Doktrin" darueber, obwohl die Owner-Aussage vom 2026-07-16 der CSV-ZELLE galt):
//   JOB   -> kein allow_failure; der Batch endet auf `exit $FAIL` = hartes Verdikt.
//   ZELLE -> je gescheiterter Zelle [FEHLER-TESTAT] + FAIL=1, der Batch MISST DURCH (Schleife bleibt intakt).
TEST(TierCiYamlBuilder, KeinAllowFailureInEmittierterJobYamlBeideStufenUndZellEbeneIntakt) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb; // Stufe 2: die Batch-Jobs (Bau + Mess) je Host-Lane
    planner::CiYamlBuilder                cb; // Stufe 1: die CEB-Jobs (build/emit/trigger)
    director.construct(*tp, tb);
    director.construct(*tp, cb);
    std::string const& stufe2 = tb.text();
    std::string const& stufe1 = cb.text();

    // GEGENPROBE VOR DEM NICHTFUND: dass die Suche auf GENAU diesen Texten ueberhaupt greift, wird an einem
    // Literal belegt, das in derselben Emission steht (sonst waere eine 0 nur eine stille Null).
    std::size_t const measure_batches = count_occurrences(stufe2, "# JOB measure-batch ");
    ASSERT_EQ(measure_batches, 2u) << "Nenner: 2 Host-Lanes (amd+intel) => 2 Mess-Batches";
    // (Der GN-11-`timeout: 7d` taugt hier NICHT als Nenner-Probe: er steht auch im Bau-Batch, also 4x statt 2x --
    //  am Objekt gemessen 2026-08-09. Genommen wird ein Literal, das NUR der Mess-Batch traegt.)
    ASSERT_EQ(count_occurrences(stufe2, "      ctest --test-dir build -L pmc --no-tests=error --output-on-failure\n"),
              measure_batches)
        << "Gegenprobe: die Suche findet in DIESEM Text (PMC-Preflight nur im Mess-Batch, je Lane einer)";
    ASSERT_GT(count_occurrences(stufe1, "  stage: ceb-build\n"), 0u) << "Gegenprobe: Stufe-1-Text ist nicht leer";

    // (1) JOB-EBENE: 0 von 2 Mess-Batches (und 0 in der gesamten Emission BEIDER Stufen) traegt allow_failure.
    EXPECT_EQ(count_occurrences(stufe2, "allow_failure"), 0u)
        << "#278: die Stufe-2-Emission traegt 0 allow_failure (Nenner: " << measure_batches << " Mess-Batches)";
    EXPECT_EQ(count_occurrences(stufe1, "allow_failure"), 0u) << "#278: auch die Stufe-1-Emission traegt 0";

    // (2)+(3) werden PRO MESS-BATCH-BLOCK geprueft, nicht global. Grund, am Objekt gemessen (2026-08-09): global
    //     gezaehlt traegt die Stufe-2-YAML 4x `exit $FAIL`, 16x [FEHLER-TESTAT] und 12x "; FAIL=1" -- weil der
    //     BAU-Batch dieselben Marken fuer seine Bau-/Pruef-Schritte fuehrt. Ein globaler Nenner haette hier also
    //     einen Bau-Befund als Mess-Befund ausgegeben. Geschnitten wird wie im Nachbar-Test G4a...InCorrectOrder.
    auto measure_bloecke = [&stufe2] {
        std::vector<std::string> blocks;
        std::size_t              pos = stufe2.find("# JOB measure-batch ");
        while (pos != std::string::npos) {
            std::size_t const next = stufe2.find("\n# JOB ", pos + 1);
            blocks.push_back(stufe2.substr(pos, next == std::string::npos ? std::string::npos : next - pos));
            pos = stufe2.find("# JOB measure-batch ", pos + 1);
        }
        return blocks;
    }();
    ASSERT_EQ(measure_bloecke.size(), measure_batches);

    for (std::string const& blk : measure_bloecke) {
        // (2) JOB-EBENE, positiv: das harte Verdikt steht wirklich da. Ohne diese Zusicherung koennte ein
        //     spaeterer Umbau `exit $FAIL` durch `exit 0` ersetzen -- und das Entfernen von allow_failure waere
        //     folgenlos, ohne dass irgendetwas rot wuerde.
        EXPECT_EQ(blk.find("allow_failure"), std::string::npos) << "kein allow_failure IN DIESEM Mess-Batch";
        EXPECT_EQ(count_occurrences(blk, "      exit $FAIL"), 1u) << "genau ein hartes Schluss-Verdikt je Batch";
        EXPECT_EQ(blk.find("exit 0"), std::string::npos) << "kein weich gemachter Schluss im Mess-Batch";

        // (3) ZELL-EBENE, unangetastet: je Zelle GENAU ein Testat (die beiden schliessen einander aus, D3-5),
        //     die Fehler-Zelle setzt FAIL=1 statt abzubrechen -- der Batch misst durch.
        std::size_t const zellen = count_occurrences(blk, "      echo \"== [MESS] zelle=");
        ASSERT_GT(zellen, 0u) << "Nenner: die Mess-Koepfe dieser Lane";
        // Gezaehlt wird die ECHO-Emission, nicht das blosse Vorkommen der Marke: die Schluss-Zeile
        // `exit $FAIL` traegt "[FEHLER-TESTAT]" im KOMMENTAR mit, und ein naiver Marken-Zaehler laege je
        // Batch um genau 1 daneben (am Objekt gemessen 2026-08-09: 3 statt 2).
        EXPECT_EQ(count_occurrences(blk, "echo \"[FEHLER-TESTAT]"), zellen)
            << "je Zelle ein Fehler-Zweig ([FEHLER-TESTAT] + FAIL=1) -- die Warnung an den Anwender";
        EXPECT_EQ(count_occurrences(blk, "echo \"[MESS-TESTAT]"), zellen) << "je Zelle ein Erfolgs-Zweig";
        EXPECT_EQ(count_occurrences(blk, "; FAIL=1"), zellen)
            << "die gescheiterte Zelle setzt FAIL=1 und der Batch laeuft weiter (kein exit im Fehler-Zweig)";
    }
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
    // je Job-mit-Klon-Prolog ein GOLDEN_N_PROFILE-Export: Stufe 1 = 2 (ceb:build + ceb:emit); Stufe 2 (S4-§62-B-
    // Batch) = 2 Build-Batches + 2 Mess-Batches = 4 (§64: EINE [all]-Combo, 2 nicht-leere Host-Lanes).
    EXPECT_EQ(count_occurrences(s1.text(), "export COMDARE_GOLDEN_N_PROFILE="), 1u * 2u);
    EXPECT_EQ(count_occurrences(s2.text(), "export COMDARE_GOLDEN_N_PROFILE="), 4u);
}

// (S2-NACHT, 2026-07-23) PROFIL-DURCHREICHE: der emittierte Child-Prolog exportiert COMDARE_GOLDEN_N_PROFILE mit dem
//     BASENAME des AKTIVEN Profils (facade: profile_path.filename()), NICHT mehr hart all_axes_golden. Sonst
//     exerzierten die von der CEB emittierten Stufe-1/2-Jobs (ceb:emit --emit-tier-ci liest genau diese Variable)
//     trotz Smoke-Scope den vollen all_axes-Katalog. KLASSEN-Regel bleibt: Re-Derive mit frischem ${CI_PROJECT_DIR},
//     nur der Basename ist dynamisch. LOCKSTEP: (a) Nicht-Default-Profil => richtiger Basename, KEIN all_axes_golden;
//     (b) golden-/Legacy-Pfad byte-unveraendert all_axes_golden.
TEST(CiYamlBuilder, ChildPrologForwardsActiveProfileBasenameNotHardAllAxes) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;

    // (a) Nicht-Default-Profil-Basename => beide Stufen tragen GENAU diesen Basename, KEIN all_axes_golden-Literal.
    //     (Der Katalog-Inhalt ist fuer die Basename-Durchreiche irrelevant -- getestet wird die Export-Verdrahtung.)
    for (auto const* basename : {"m3v2_smoke.profile.xml", "cacheline_study.profile.xml"}) {
        planner::CiYamlBuilder     s1;
        planner::TierCiYamlBuilder s2;
        director.construct(*tp, s1, "", {}, basename);
        director.construct(*tp, s2, "", {}, basename);
        for (auto const* yaml : {&s1.text(), &s2.text()}) {
            EXPECT_NE(yaml->find(std::string("/thesis_profiles/") + basename + "\""), std::string::npos)
                << "COMDARE_GOLDEN_N_PROFILE zeigt auf das AKTIVE Profil (" << basename << ")";
            EXPECT_EQ(yaml->find("all_axes_golden.profile.xml"), std::string::npos)
                << "kein hartes all_axes_golden mehr bei Nicht-Default-Profil (" << basename << ")";
        }
    }

    // (b) golden-/Legacy-Pfad: expliziter all_axes-Basename UND leerer Ctor (kein profile_path) => beide byte-gleich
    //     und beide tragen das all_axes_golden-Literal (byte-identisch zu HEAD => bestehende Byte-Wachen bleiben gruen).
    planner::CiYamlBuilder golden_explicit;
    planner::CiYamlBuilder golden_empty;
    director.construct(*tp, golden_explicit, "", {}, "all_axes_golden.profile.xml");
    director.construct(*tp, golden_empty); // leer => Prolog-Fallback all_axes_golden (byte-identisch zu HEAD)
    EXPECT_EQ(golden_explicit.text(), golden_empty.text())
        << "golden-Basename explizit == Legacy-Ctor-Fallback (byte-identisch)";
    EXPECT_NE(golden_empty.text().find("/thesis_profiles/all_axes_golden.profile.xml\""), std::string::npos)
        << "Default-/golden-Pfad bleibt all_axes_golden";
}

// (W10) S4-§62-B: der Build+Pruef-Batch durchlaeuft je Perm das [0,COMDARE_GN_TOTAL)-Fenster in kGnBatchSlice=4096er-
//       Scheiben (Bestandslog-Korn). Die Scheiben-Arithmetik ist eine Shell-while-Schleife MIT dem 4096er-Literal;
//       KEIN Env-Override (COMDARE_GN_BATCH_SLICE), KEIN Chunk-Konzept mehr.
TEST(TierCiYamlBuilder, PerBatchSliceArithmetic4096) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    // Das 4096er-Bestandslog-Korn als HARTE Konstante (SLICE-Var je Build-Batch, 2 Host-Lanes).
    EXPECT_EQ(count_occurrences(yaml, "SLICE=4096"), 2u) << "kGnBatchSlice=4096 je Build-Batch (2 Host-Lanes)";
    // Fenster-Schleife + Klemm-Arithmetik je Perm (4 Perms ueber die 2 Build-Batches).
    EXPECT_EQ(count_occurrences(yaml, "while [ \"$START\" -lt \"$TOTAL\" ]; do"), 4u) << "Scheiben-Schleife je Perm";
    EXPECT_EQ(count_occurrences(yaml, "COMDARE_GOLDEN_N_RANGE=\"${START}:${COUNT}\""), 4u) << "Inline-Range je Perm";
    // C-10/E-10 (21.08.2026): der Scheiben-Fortschritt steht seit der window_belongs_to-Verdrahtung ZWEIMAL
    // je Perm -- einmal am Schleifenende (gebautes Fenster) und einmal im [FENSTER-FREMD]-Skip-Zweig
    // (fremdes Fenster wird uebersprungen, der Zaehler muss trotzdem voranschreiten, sonst Endlosschleife).
    EXPECT_EQ(count_occurrences(yaml, "START=$(( START + COUNT ))"), 8u)
        << "Scheiben-Fortschritt je Perm (Bau-Pfad + [FENSTER-FREMD]-Pfad, C-10/E-10)";
    // TOTAL-Default je Build-Batch (2 Host-Lanes); Voll-Bau: COMDARE_GN_TOTAL=131072.
    EXPECT_EQ(count_occurrences(yaml, "TOTAL=\"${COMDARE_GN_TOTAL:-16}\""), 2u) << "Default 16 je Build-Batch";
    // NEGATIV: KEIN Env-Override des 4096er-Korns (§61-Verstoss verworfen) und KEIN Chunk-Konzept mehr.
    EXPECT_EQ(yaml.find("COMDARE_GN_BATCH_SLICE"), std::string::npos)
        << "das 4096er-Korn ist eine harte Konstante, KEIN Env-Override (§61)";
    EXPECT_EQ(yaml.find("CHUNK_SIZE"), std::string::npos) << "kein Chunk-Konzept mehr (§62-B-Batch)";
    EXPECT_EQ(yaml.find("; CHUNK="), std::string::npos) << "keine CHUNK-Nummer mehr";
    EXPECT_EQ(yaml.find("COMDARE_GN_RANGE:-0:4"), std::string::npos) << "kein globales Fixfenster in der Nutzlast";
}

// (S4-a) PruefStepEmittedPerPermWithContractEnv: der Build+Pruef-Batch emittiert je Perm UNBEDINGT den S3-Pruef-
//        Schritt (COMDARE_PRUEF_ONLY=true, COMDARE_GN_OPT/SIMD, COMDARE_GOLDEN_N_RANGE=0:${TOTAL}, gleiches dll_dir)
//        mit [PRUEF]-Legende + [PRUEF-TESTAT] (kein Existenz-Guard, S3-Vertrag fixiert).
TEST(TierCiYamlBuilder, PruefStepEmittedPerPermWithContractEnv) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    EXPECT_EQ(count_occurrences(yaml, "COMDARE_PRUEF_ONLY=true"), 4u) << "je Perm EIN S3-Pruef-Schritt (4 Perms)";
    EXPECT_EQ(count_occurrences(yaml, "COMDARE_GOLDEN_N_RANGE=\"0:${TOTAL}\""), 4u)
        << "der Pruef-Schritt faehrt ueber das VOLLE Perm-Fenster 0:${TOTAL}";
    EXPECT_EQ(count_occurrences(yaml, "[PRUEF] zelle="), 4u) << "je Perm eine [PRUEF]-Schritt-Legende";
    EXPECT_EQ(count_occurrences(yaml, "[PRUEF-TESTAT]"), 4u) << "je Perm ein [PRUEF-TESTAT] (Erfolgsfall)";
    // Der Pruef-Aufruf traegt dieselbe Perm-Selektion (COMDARE_GN_OPT/SIMD) wie der Bau -> gleiches dll_dir.
    EXPECT_NE(yaml.find("COMDARE_PRUEF_ONLY=true COMDARE_RUN_SOTA=0"), std::string::npos)
        << "S3-Vertrag: COMDARE_PRUEF_ONLY + COMDARE_RUN_SOTA=0";
}

// (S4-b) SoftFailGuardAndFinalExit: jeder Treiber-Aufruf ist set-e-sicher if-guarded ([FEHLER-TESTAT] + FAIL=1),
//        der Batch endet mit exit $FAIL (Fehler je Zelle sichtbar, der Batch laeuft durch).
TEST(TierCiYamlBuilder, SoftFailGuardAndFinalExit) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    EXPECT_NE(yaml.find("set -euo pipefail"), std::string::npos) << "Batch-Script ist set -euo pipefail";
    EXPECT_NE(yaml.find("if ! COMDARE_THESIS_PROFILE="), std::string::npos)
        << "if-!-Guard je Treiber-Aufruf (set-e-vertraeglich)";
    EXPECT_GT(count_occurrences(yaml, "[FEHLER-TESTAT]"), 0u) << "Fehler-Testat-Zweig je Aufruf";
    EXPECT_GT(count_occurrences(yaml, "FAIL=1"), 0u) << "Sammel-Fehler-Flag je Aufruf";
    EXPECT_EQ(count_occurrences(yaml, "exit $FAIL"), 4u) << "je Batch-Job (2 Build + 2 Mess) ein Sammel-Exit";
}

// ---------------------------------------------------------------------------
// (D3-5 / W0b-3, 2026-08-08) WERKZEUG DER TESTAT-BINDUNGS-WACHE.
//
// Ein Shell-Testat behauptet "dieser Schritt IST getan". Steht sein echo HINTER dem schliessenden fi
// eines SOFT-FAIL-Guards -- `if ! <cmd>; then echo "[FEHLER-TESTAT] ..."; FAIL=1; fi` -- dann wird es
// UNBEDINGT gedruckt, auch fuer die gescheiterte Zelle. Der Guard ist WEICH: er setzt FAIL=1, statt
// abzubrechen, die Zeile darunter laeuft also wirklich. Wer die Testat-Zeilen als "erledigte Zellen"
// zaehlt -- der naheliegendste Gebrauch --, zaehlt die Fehlschlaege mit; der Nenner ist um genau die
// Fehlerzahl zu gross und schoent sich, je mehr schiefgeht.
//
// D3-5 heilte das [MESS-TESTAT]. W0b-3 ist die SCHWESTERSTELLE: dasselbe im Bau-Batch ([TESTAT]).
//
// WARUM DIE VORIGE FASSUNG DIESER WACHE DIE SCHWESTER NICHT SAH: sie lief ueber EINEN handgenannten
// Marker. Die Klasse hatte aber zwei Stellen, und ein dritter Marker haette sie wieder blind
// erwischt. Deshalb liest diese Fassung die Marken-Menge AUS DEM EMITTIERTEN PLAN und verlangt fuer
// jede eine Klassifikation -- eine neue Marke faellt mit ihrem Namen auf, statt stumm durchzurutschen.
//
// NICHT GEMEINT sind die unter `set -euo pipefail` gefuehrten Testate ([CEB-TESTAT], [PMC-TESTAT]):
// dort bricht ein Fehler den ganzen Job ab, das Testat wird nie erreicht. Das ist eine andere Bauart,
// kein Defekt -- sie steht deshalb namentlich in der Klassifikation und nicht stillschweigend im
// Ausnahmefall.
//
// GRENZEN (T-9), ausdruecklich benannt statt verschwiegen -- drei Stueck:
//  1. Die Wache liest den EMITTIERTEN Shell-Text STRUKTURELL, sie fuehrt ihn nicht aus. Die Bindung
//     erkennt sie an else/fi und an FAIL=1, nicht an der Semantik eines beliebigen Konstrukts: ein
//     Testat, das ueber `&&`/`||` oder eine Shell-Funktion gebunden wird, gilt ihr als "kein
//     Soft-Fail-Guard" und bleibt ungeprueft.
//  2. Als Testat zaehlt nur, was "TESTAT" IM NAMEN traegt -- das ist die Grammatik der Familie
//     (Marke, ts=, dann Felder), aber eine Erfolgsmeldung, die anders hiesse, faellt durch. Die
//     Alternative (jedes echo "[...]") war verworfen: die Schritt-KOEPFE [BAU]/[MESS]/[PRUEF] sind
//     legitim unbedingt und erzeugten lauter Falschklagen.
//  3. Geprueft werden die zwei CI-Emissionen, seit dem Nachsatz W0b-3 in BEIDEN Methodiken
//     (measure UND debug, alle_wachen_plaene -- der Debug-Zweig mit seinem (j3)-Dual-Compile war
//     vorher strukturell ungeprueft). Der bare-metal-Spiegel (TierCmakeGraphBuilder,
//     Abschnitt 61 Dual-Weg) emittiert heute UEBERHAUPT keine Testate und ist damit nicht
//     Gegenstand dieser Wache -- am Objekt nachgesehen (2026-08-08), nicht angenommen.
// ---------------------------------------------------------------------------
namespace {

[[nodiscard]] std::string ohne_rand(std::string const& s) {
    std::size_t const a = s.find_first_not_of(" \t");
    if (a == std::string::npos) return {};
    return s.substr(a, s.find_last_not_of(" \t") - a + 1);
}

[[nodiscard]] std::vector<std::string> zeilen_von(std::string const& text) {
    std::vector<std::string> out;
    std::size_t              a = 0;
    while (true) {
        std::size_t const e = text.find('\n', a);
        if (e == std::string::npos) {
            out.push_back(text.substr(a));
            break;
        }
        out.push_back(text.substr(a, e - a));
        a = e + 1;
    }
    return out;
}

// Die Marke eines Testat-ECHOS, sonst "". Nur echte Emissionen zaehlen: eine Zeile, die eine Marke
// bloss NENNT (`exit $FAIL   # Fehler je Zelle sichtbar ([FEHLER-TESTAT] + Log-Artefakt)`), ist kein
// Testat und darf den Nenner nicht fuellen. Genau daran waere ein reiner Zeichenketten-Zaehler
// vorbeigelaufen -- der Plan emittiert solche Kommentare wirklich.
[[nodiscard]] std::string testat_marke_der_zeile(std::string const& zeile) {
    std::string const anker = "echo \"[";
    std::size_t const p     = zeile.find(anker);
    if (p == std::string::npos) return {};
    std::size_t const a = p + anker.size();
    std::size_t const e = zeile.find(']', a);
    if (e == std::string::npos) return {};
    std::string marke = zeile.substr(a, e - a);
    if (marke.find("TESTAT") == std::string::npos) return {};
    return marke;
}

// Die Marken-Menge, aus dem Plan GELESEN statt handgelistet (Reihenfolge des ersten Auftretens).
[[nodiscard]] std::vector<std::string> emittierte_testat_marken(std::vector<std::string> const& z) {
    std::vector<std::string> marken;
    for (std::string const& zeile : z) {
        std::string const marke = testat_marke_der_zeile(zeile);
        if (marke.empty()) continue;
        if (std::find(marken.begin(), marken.end(), marke) == marken.end()) marken.push_back(marke);
    }
    return marken;
}

[[nodiscard]] bool ist_kontrollfluss(std::string const& roh) {
    std::string const t = ohne_rand(roh);
    return t == "fi" || t == "else" || t == "done" || t.rfind("if ", 0) == 0 || t.rfind("elif ", 0) == 0 ||
           t.rfind("while ", 0) == 0 || t.rfind("for ", 0) == 0;
}

// Die Feldnamen einer Testat-Zeile in Reihenfolge. Getrennt wird am '=', der Name davor gelesen --
// NICHT am Leerzeichen: die Werte enthalten selbst welche ($(date -u +%FT%TZ),
// $(( TOTAL - START - COUNT ))), ein Split am Leerzeichen zerlegte sie falsch.
//
// GELESEN WIRD NUR DIE ECHO-NUTZLAST zwischen den aeusseren Anfuehrungszeichen. Ohne diese Klammer
// zaehlte die angehaengte Shell-Anweisung `; FAIL=1` als Feld "FAIL" mit -- im ersten roten Lauf
// meldete die Wache genau das, und die Grammatik der Zeile ist nicht, was hinter ihr noch steht.
[[nodiscard]] std::vector<std::string> feldnamen(std::string const& roh) {
    std::size_t const auf = roh.find('"');
    std::size_t const zu  = roh.rfind('"');
    if (auf == std::string::npos || zu <= auf) return {};
    std::string const        zeile = roh.substr(auf, zu - auf + 1);
    std::vector<std::string> felder;
    for (std::size_t i = 0; i < zeile.size(); ++i) {
        if (zeile[i] != '=') continue;
        std::size_t j = i;
        while (j > 0) {
            char const c    = zeile[j - 1];
            bool const teil = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
            if (!teil) break;
            --j;
        }
        if (j == i) continue;                        // '=' ohne Namen davor
        if (j == 0 || zeile[j - 1] != ' ') continue; // nur freistehende Felder, kein ${X:-Y}-Innenleben
        felder.push_back(zeile.substr(j, i - j));
    }
    return felder;
}

[[nodiscard]] std::string verkettet(std::vector<std::string> const& v) {
    std::string s;
    for (std::string const& e : v) {
        if (!s.empty()) s += ' ';
        s += e;
    }
    return s;
}

// BEIDE emittierten Plaene, nicht nur der eine. Der erste rote Lauf klagte [CEB-TESTAT] als
// "wird nicht mehr emittiert" ein -- die Marke steht naemlich in der STUFE-1-Emission
// (CiYamlBuilder), waehrend die Batch-Testate aus der STUFE-2-Emission (TierCiYamlBuilder)
// kommen. Eine Wache, die nur eine Stufe liest, ist auf der anderen blind: genau die
// Schwesterstellen-Blindheit, gegen die dieses Paket gebaut ist, eine Ebene hoeher.
struct EmittierterPlan {
    std::string              name;
    std::vector<std::string> zeilen;
};

// (Nachsatz W0b-3, 08.08.2026) BEIDE Methodiken, nicht nur der eine golden-measure-Release-Pfad:
// der Review fand alle drei Wachen-Tests auf demselben Pfad -- der Debug-Zweig ((j3) Dual-Compile,
// ZWEI Treiber-Aufrufe je Mess-Fenster, eigene Testat-Zeilen) war strukturell ungeprueft. Die
// Debug-Plaene entstehen wie in MeasurementModi61 (j2): dieselben Achsen, run_methodology={"build"}.
// Vier Plaene = {Stufe 1, Stufe 2} x {measure, debug}; jede Wache laeuft ueber ALLE vier.
[[nodiscard]] std::vector<EmittierterPlan> alle_wachen_plaene(cx::ThesisProfile const& tp) {
    planner::ExperimentPlanDirector const director;
    planner::CiYamlBuilder                b1m;
    planner::TierCiYamlBuilder            b2m;
    director.construct(tp, b1m);
    director.construct(tp, b2m);
    cx::ThesisProfile dbg = tp;
    dbg.run_methodology   = {"build"};
    planner::CiYamlBuilder     b1d;
    planner::TierCiYamlBuilder b2d;
    director.construct(dbg, b1d);
    director.construct(dbg, b2d);
    return {{"CiYamlBuilder (Stufe 1, measure)", zeilen_von(b1m.text())},
            {"TierCiYamlBuilder (Stufe 2, measure)", zeilen_von(b2m.text())},
            {"CiYamlBuilder (Stufe 1, debug)", zeilen_von(b1d.text())},
            {"TierCiYamlBuilder (Stufe 2, debug)", zeilen_von(b2d.text())}};
}

// ---------------------------------------------------------------------------
// (Nachsatz W0b-3) GRAMMATIK DER CTEST-ERKENNUNG, als benannte Helfer ueber eine MENGE geprueft
// (Test CtestGrammatikHelfer...). Die vorige Fassung verglich den Zeilenanfang woertlich mit
// "ctest " und suchte "--no-tests=error" als Substring der GANZEN Zeile. Das war in beide
// Richtungen falsch: zu ENGER Nenner (uebersehen: /usr/bin/ctest, `if ! ctest`, `VAR=1 ctest`,
// `cd build && ctest`, Tab-Einrueckung, nacktes `ctest` ohne Argument) und zu WEITE Annahme
// (akzeptiert: der Schalter im Kommentar hinter `#`, in einem ANDEREN Kommando derselben Zeile
// hinter `;`/`&&`, oder als Praefix eines anderen Tokens wie --no-tests=error-x).
// RICHTUNG DER RESTUNSICHERHEIT: die Erkennung zaehlt jedes ctest-Token in Kommando-Position
// LIEBER EINMAL ZU VIEL (z.B. `echo ctest` unquotiert) -- ein Zuviel erzwingt den Schalter und
// faellt laut auf, ein Zuwenig waere die stille Null. Quotes werden NICHT geparst; die
// Emissionen tragen kein '#' und kein 'ctest' in Anfuehrungszeichen (Mengen-Test dokumentiert das).
// ---------------------------------------------------------------------------

// Schneidet den Kommentarteil ab: '#' am Zeilenanfang oder nach Leerraum beginnt den Kommentar.
[[nodiscard]] std::string ohne_kommentar(std::string const& roh) {
    for (std::size_t i = 0; i < roh.size(); ++i) {
        if (roh[i] != '#') continue;
        if (i == 0 || roh[i - 1] == ' ' || roh[i - 1] == '\t') return roh.substr(0, i);
    }
    return roh;
}

// Zerlegt die (kommentarfreie) Zeile in Kommando-Segmente an den Trennern && / || / ; / | und
// jedes Segment in Whitespace-Token. Die Trenner werden auch OHNE umgebende Leerzeichen erkannt.
[[nodiscard]] std::vector<std::vector<std::string>> kommando_segmente(std::string const& roh) {
    std::string const                     z = ohne_kommentar(roh);
    std::vector<std::vector<std::string>> segmente(1);
    std::string                           token;
    auto const                            token_abschliessen = [&] {
        if (!token.empty()) segmente.back().push_back(token);
        token.clear();
    };
    auto const segment_abschliessen = [&] {
        token_abschliessen();
        if (!segmente.back().empty()) segmente.emplace_back();
    };
    for (std::size_t i = 0; i < z.size(); ++i) {
        char const c = z[i];
        if (c == ' ' || c == '\t') {
            token_abschliessen();
        } else if (c == ';') {
            segment_abschliessen();
        } else if (c == '&' && i + 1 < z.size() && z[i + 1] == '&') {
            segment_abschliessen();
            ++i;
        } else if (c == '|') {
            segment_abschliessen();
            if (i + 1 < z.size() && z[i + 1] == '|') ++i;
        } else {
            token += c;
        }
    }
    token_abschliessen();
    if (segmente.back().empty()) segmente.pop_back();
    return segmente;
}

// Ein Token ruft ctest, wenn es "ctest" IST oder auf "/ctest" ENDET (Pfad-Aufruf).
[[nodiscard]] bool ist_ctest_token(std::string const& t) {
    if (t == "ctest") return true;
    static constexpr std::string_view suffix = "/ctest";
    return t.size() > suffix.size() && t.compare(t.size() - suffix.size(), suffix.size(), suffix.data()) == 0;
}

struct CtestZeilenBefund {
    std::size_t aufrufe     = 0; // Segmente der Zeile mit ctest in Kommando-Position
    std::size_t ohne_schutz = 0; // davon ohne das EXAKTE Argument-Token --no-tests=error
};

// Je Kommando-Segment: erstes ctest-Token gesucht; geschuetzt ist der Aufruf nur, wenn im SELBEN
// Segment NACH dem ctest-Token das exakte Token --no-tests=error steht (kein Substring, kein
// Kommentar, kein Nachbar-Kommando).
[[nodiscard]] CtestZeilenBefund ctest_befund_der_zeile(std::string const& roh) {
    CtestZeilenBefund b;
    for (std::vector<std::string> const& seg : kommando_segmente(roh)) {
        std::size_t ctest_pos = seg.size();
        for (std::size_t j = 0; j < seg.size(); ++j) {
            if (ist_ctest_token(seg[j])) {
                ctest_pos = j;
                break;
            }
        }
        if (ctest_pos == seg.size()) continue;
        ++b.aufrufe;
        bool geschuetzt = false;
        for (std::size_t j = ctest_pos + 1; j < seg.size(); ++j)
            if (seg[j] == "--no-tests=error") geschuetzt = true;
        if (!geschuetzt) ++b.ohne_schutz;
    }
    return b;
}

// ---------------------------------------------------------------------------
// (Nachsatz W0b-3) SET-E-BINDUNG STRUKTURELL: die Klassifikation nannte [CEB-TESTAT]/[PMC-TESTAT]
// SetEAbbruch, aber KEIN Test verifizierte, dass die Marke wirklich set-e-gebunden ist. Diese
// Helfer lesen die Emission strukturell: die Marke muss in einem YAML-'- |'-Block stehen, in dem
// VOR ihr `set -euo pipefail` laeuft, zwischen set -e und Marke darf nichts aufweichen (`set +e`,
// `|| true`), und mindestens EIN hartes (Nicht-echo-, Nicht-Kommentar-)Kommando muss dazwischen
// stehen -- sonst gaebe es nichts, dessen Fehlschlag das Testat verhindert.
// GRENZE (T-9): strukturell, nicht ausgefuehrt. Ein Kommando, das seinen Fehlschlag selbst
// verschluckt (`cmd || echo weiter`), saehe diese Pruefung nur ueber das `|| `-Verbot; exotische
// Formen (Funktionsdefinitionen, `if`-gebundene harte Kommandos) sind nicht gemeint.
// ---------------------------------------------------------------------------
struct SetEBindung {
    bool        im_pipe_block   = false; // Marke steht in einem '- |'-Block
    bool        set_e_davor     = false; // `set -euo pipefail` VOR der Marke im selben Block
    bool        aufgeweicht     = false; // `set +e` oder `|| true` zwischen set -e und Marke
    bool        hartes_kommando = false; // >=1 Nicht-echo-Kommando zwischen set -e und Marke
    std::string beanstandet;             // erste beanstandete Zeile (Diagnose)
};

[[nodiscard]] SetEBindung pruefe_set_e_bindung(std::vector<std::string> const& z, std::size_t i) {
    SetEBindung b;
    // Die Marken-Zeile MUSS selbst Blockinhalt sein (>= 6 Leerzeichen). Ohne diese Pruefung fand
    // die erste Fassung dieses Helfers fuer eine als EIGENES '- '-Listen-Element ausgekoppelte
    // Marke den '- |' der VORGAENGER-Zeilen und meldete gruen -- der M2-Koeder (09.08., [CEB-TESTAT]
    // als '    - echo ...') biss NICHT und hat genau diesen Defekt im Test aufgedeckt (T-9/K13:
    // der Koeder muss erst beissen, sonst ist der Test der Defekt).
    if (z[i].rfind("      ", 0) != 0) {
        b.beanstandet = ohne_rand(z[i]);
        return b;
    }
    std::size_t start = i;
    while (start > 0) {
        std::string const t = ohne_rand(z[start - 1]);
        if (t == "- |") {
            b.im_pipe_block = true;
            break;
        }
        // Blockinhalt der Emissionen ist mit >= 6 Leerzeichen eingerueckt; alles andere
        // (naechstes '- '-Listenelement, 'script:', Job-Schluessel) beendet die Suche.
        if (z[start - 1].rfind("      ", 0) != 0) break;
        --start;
    }
    if (!b.im_pipe_block) {
        b.beanstandet = ohne_rand(z[i]);
        return b;
    }
    std::size_t set_e = i;
    for (std::size_t k = start; k < i; ++k) {
        if (ohne_rand(z[k]) == "set -euo pipefail") {
            b.set_e_davor = true;
            set_e         = k;
            break;
        }
    }
    if (!b.set_e_davor) {
        b.beanstandet = ohne_rand(z[i]);
        return b;
    }
    for (std::size_t k = set_e + 1; k < i; ++k) {
        std::string const t = ohne_rand(z[k]);
        if (t.empty() || t.rfind("#", 0) == 0) continue;
        if (t.find("set +e") != std::string::npos || t.find("|| true") != std::string::npos) {
            b.aufgeweicht = true;
            if (b.beanstandet.empty()) b.beanstandet = t;
        }
        if (t.rfind("echo ", 0) != 0) b.hartes_kommando = true;
    }
    return b;
}

// Ergebnis MIT NENNER: `geprueft` ist die Grundgesamtheit (alle echo-Emissionen der Marke),
// `unbedingt` die Zahl derer, denen KEIN else unmittelbar vorausgeht.
struct TestatBindung {
    std::size_t geprueft  = 0;
    std::size_t unbedingt = 0;
    std::string erste_vorzeile;
};

// DIE INVARIANTE FUER EINEN MARKER (die Hilfsfunktion, die D3-5 nur inline kannte): jedem
// Testat-echo geht ein "else" unmittelbar voraus. Ein "fi" davor ist genau der Defekt.
[[nodiscard]] TestatBindung testat_haengt_am_else(std::vector<std::string> const& z, std::string const& marker) {
    TestatBindung b;
    for (std::size_t i = 0; i < z.size(); ++i) {
        if (testat_marke_der_zeile(z[i]) != marker) continue;
        ++b.geprueft;
        std::string const vorzeile = (i == 0) ? std::string{} : ohne_rand(z[i - 1]);
        if (vorzeile != "else") {
            ++b.unbedingt;
            if (b.erste_vorzeile.empty()) b.erste_vorzeile = vorzeile;
        }
    }
    return b;
}

struct SoftFailGuard {
    std::size_t fehler_zeile = 0;
    bool        hat_else     = false;
    bool        hat_erfolg   = false;
    std::size_t erfolg_zeile = 0;
    std::string unbedingtes_testat; // Marke eines Testats HINTER dem fi ("" = keins)
    std::string unbedingte_zeile;
};

// DIE KLASSEN-WACHE, eine Ebene UEBER der Marken-Liste: jeder Soft-Fail-Guard wird an seiner
// "[FEHLER-TESTAT] ... FAIL=1"-Zeile erkannt und von seinem fi aus nach vorn gelesen. Ein Testat
// zwischen dem fi und dem naechsten Kontrollfluss-Schluesselwort wird unbedingt gedruckt -- das
// gilt fuer JEDE Marke, auch fuer eine, die es heute noch nicht gibt.
[[nodiscard]] std::vector<SoftFailGuard> soft_fail_guards(std::vector<std::string> const& z) {
    std::vector<SoftFailGuard> guards;
    for (std::size_t i = 0; i < z.size(); ++i) {
        if (testat_marke_der_zeile(z[i]) != "FEHLER-TESTAT") continue;
        SoftFailGuard g;
        g.fehler_zeile    = i;
        bool        weich = false;
        std::size_t k     = i;
        for (; k < z.size(); ++k) {
            if (z[k].find("FAIL=1") != std::string::npos) weich = true;
            std::string const t = ohne_rand(z[k]);
            if (k > i && (t == "else" || t == "fi")) break;
        }
        // Kein FAIL=1 im then-Zweig => HARTER Guard (set -e / exit). Andere Bauart, hier nicht gemeint.
        if (!weich || k >= z.size()) continue;
        std::size_t fi          = k;
        bool        fi_gefunden = (ohne_rand(z[k]) == "fi");
        if (ohne_rand(z[k]) == "else") {
            g.hat_else = true;
            for (std::size_t m = k + 1; m < z.size(); ++m) {
                if (ohne_rand(z[m]) == "fi") {
                    fi          = m;
                    fi_gefunden = true;
                    break;
                }
                if (g.hat_erfolg || testat_marke_der_zeile(z[m]).empty()) continue;
                g.hat_erfolg   = true;
                g.erfolg_zeile = m;
            }
        }
        // HINTER dem schliessenden fi laeuft alles wieder unbedingt -- AUCH bei einem Guard MIT
        // else-Zweig. Ein dort ergaenztes zweites Testat waere derselbe Defekt eine Zeile spaeter,
        // und die erste Fassung dieser Wache sah genau dorthin nicht (beim Entwurf des dritten
        // Koeders aufgefallen: sie prueft sonst nur den Guard OHNE else).
        for (std::size_t m = fi + 1; fi_gefunden && m < z.size() && !ist_kontrollfluss(z[m]); ++m) {
            std::string const marke = testat_marke_der_zeile(z[m]);
            if (marke.empty()) continue;
            g.unbedingtes_testat = marke;
            g.unbedingte_zeile   = ohne_rand(z[m]);
            break;
        }
        guards.push_back(g);
    }
    return guards;
}

enum class Bindung {
    SoftFailElse, // Erfolgs-Zwilling eines Soft-Fail-Guards -- MUSS im else-Zweig stehen
    SetEAbbruch,  // unter `set -euo pipefail` gefuehrt -- ein Fehler bricht den Job ab
    FehlerAnker   // die Fehler-Marke selbst
};

// WER EINE NEUE SHELL-TESTAT-MARKE EMITTIERT, TRAEGT SIE HIER EIN. Der Test faellt sonst mit ihrem
// Namen -- und zwar in BEIDE Richtungen (unklassifiziert emittiert / klassifiziert aber verschwunden).
constexpr std::array<std::pair<std::string_view, Bindung>, 7> kTestatKlassifikation{{
    {"FEHLER-TESTAT", Bindung::FehlerAnker},
    {"CEB-TESTAT", Bindung::SetEAbbruch},    // STUFE 1: CEB gebaut (harter cmake --build davor)
    {"TESTAT", Bindung::SoftFailElse},       // Bau-Fenster je Perm
    {"PRUEF-TESTAT", Bindung::SoftFailElse}, // S3-Konformitaets-Gate je Perm
    {"PMC-TESTAT", Bindung::SetEAbbruch},    // PMC-Preflight (harter ctest davor)
    {"MESS-TESTAT", Bindung::SoftFailElse},  // Mess-Zelle
    {"DUAL-TESTAT", Bindung::SetEAbbruch},   // CI-DUAL clang-Zwilling (harte cmake/ctest-Gates davor)
}};

} // namespace

// (W0b-3) Die Marken-Menge BEIDER Stufen ist klassifiziert, und jede Soft-Fail-Marke haengt am else.
TEST(TierCiYamlBuilder, TestatMarkenSindKlassifiziertUndHaengenAmElse) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    // (Nachsatz W0b-3) measure UND debug durch beide Builder -- der Debug-Zweig war ungeprueft.
    std::vector<EmittierterPlan> const pl = alle_wachen_plaene(*tp);

    // (1) Die Marken werden aus den emittierten Plaenen GELESEN, nicht handgelistet.
    std::vector<std::string> gefunden;
    for (EmittierterPlan const& p : pl)
        for (std::string const& marke : emittierte_testat_marken(p.zeilen))
            if (std::find(gefunden.begin(), gefunden.end(), marke) == gefunden.end()) gefunden.push_back(marke);
    ASSERT_FALSE(gefunden.empty()) << "keine Testat-Marke in den Plaenen -- die Suche griff nicht";

    // (2) Jede emittierte Marke ist klassifiziert. Eine NEUE Marke faellt hier mit ihrem Namen auf.
    for (std::string const& marke : gefunden) {
        bool bekannt = false;
        for (auto const& eintrag : kTestatKlassifikation)
            if (eintrag.first == marke) bekannt = true;
        EXPECT_TRUE(bekannt) << "unklassifizierte Testat-Marke [" << marke
                             << "] -- in kTestatKlassifikation eintragen (SoftFailElse oder SetEAbbruch)";
    }
    // (3) und umgekehrt: keine klassifizierte Marke ist lautlos aus den Plaenen verschwunden.
    for (auto const& eintrag : kTestatKlassifikation)
        EXPECT_NE(std::find(gefunden.begin(), gefunden.end(), std::string{eintrag.first}), gefunden.end())
            << "klassifizierte Marke [" << eintrag.first << "] wird nicht mehr emittiert -- Klassifikation nachziehen";
    EXPECT_EQ(gefunden.size(), kTestatKlassifikation.size())
        << gefunden.size() << " emittierte Marken gegen " << kTestatKlassifikation.size() << " klassifizierte ("
        << verkettet(gefunden) << ")";

    // (4) DIE INVARIANTE je Soft-Fail-Marke, ueber die benannte Hilfsfunktion, ueber BEIDE Stufen.
    std::size_t soft_marken = 0;
    for (auto const& eintrag : kTestatKlassifikation) {
        if (eintrag.second != Bindung::SoftFailElse) continue;
        ++soft_marken;
        std::size_t geprueft = 0, unbedingt = 0;
        std::string erste;
        for (EmittierterPlan const& p : pl) {
            TestatBindung const b = testat_haengt_am_else(p.zeilen, std::string{eintrag.first});
            geprueft += b.geprueft;
            unbedingt += b.unbedingt;
            if (erste.empty()) erste = b.erste_vorzeile;
        }
        EXPECT_GT(geprueft, 0u) << "[" << eintrag.first << "] kam nicht vor -- daran prueft der Test nichts";
        EXPECT_EQ(unbedingt, 0u) << unbedingt << " von " << geprueft << " [" << eintrag.first
                                 << "]-Zeilen haengen nicht am else-Zweig; erste beanstandete Vorzeile: '" << erste
                                 << "'";
    }
    EXPECT_EQ(soft_marken, 3u) << "erwartet: [TESTAT] (bau), [PRUEF-TESTAT], [MESS-TESTAT]";
}

// (W0b-3) Marker-AGNOSTISCH: kein Testat steht hinter dem fi eines Soft-Fail-Guards. Faengt auch eine
// Marke, die es heute noch nicht gibt -- der Test kennt hier keine Namen, nur die Struktur.
TEST(TierCiYamlBuilder, KeinTestatUnbedingtHinterEinemSoftFailGuard) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    // (Nachsatz W0b-3) measure UND debug durch beide Builder -- der Debug-Zweig war ungeprueft.
    std::vector<EmittierterPlan> const pl = alle_wachen_plaene(*tp);

    std::size_t gesamt = 0, mit_else = 0, unbedingt = 0;
    for (EmittierterPlan const& p : pl) {
        std::vector<SoftFailGuard> const guards = soft_fail_guards(p.zeilen);
        gesamt += guards.size();
        for (SoftFailGuard const& g : guards) {
            if (g.hat_else) ++mit_else;
            if (g.unbedingtes_testat.empty()) continue;
            ++unbedingt;
            ADD_FAILURE() << p.name << ": [" << g.unbedingtes_testat << "] steht HINTER dem fi eines Soft-Fail-"
                          << "Guards und wird damit auch fuer die gescheiterte Zelle gedruckt: '" << g.unbedingte_zeile
                          << "'";
        }
    }
    ASSERT_GT(gesamt, 0u) << "kein Soft-Fail-Guard in den Plaenen -- die Suche griff nicht";
    EXPECT_EQ(unbedingt, 0u) << unbedingt << " von " << gesamt << " Soft-Fail-Guards drucken ihr Testat unbedingt";
    EXPECT_EQ(mit_else, gesamt) << mit_else << " von " << gesamt << " Soft-Fail-Guards haben einen else-Zweig";
}

// (W0b-3, per T-6 gefunden) DIESELBE KLASSE an einer anderen Naht: ein Marker, der mehr behauptet, als er
// weiss. `ctest -L <label>` liefert bei NULL passenden Tests rc=0 ("No tests were found!!!"), `set -e` greift
// also nicht, und das [PMC-TESTAT] meldet ungeruehrt pmc=ok -- ein Preflight, der nichts gefunden hat, ist von
// einem bestandenen nicht zu unterscheiden. Geprueft wird ueber ALLE emittierten ctest-Aufrufe, nicht ueber
// den einen von heute: ein zweiter Aufruf ohne den Schalter faellt damit von selbst auf.
TEST(TierCiYamlBuilder, JederEmittierteCtestAufrufMachtDenLeerlaufZumFehler) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    // (Nachsatz W0b-3) measure UND debug durch beide Builder -- der Debug-Zweig war ungeprueft.
    std::vector<EmittierterPlan> const pl = alle_wachen_plaene(*tp);

    // (Nachsatz W0b-3) Erkennung ueber die benannten Grammatik-Helfer statt ueber den woertlichen
    // Zeilenanfang "ctest " -- die alte Form uebersah Praefixe (/usr/bin/ctest, if !, VAR=1,
    // cd build &&) und akzeptierte den Schalter als Substring irgendwo in der Zeile (Kommentar,
    // Nachbar-Kommando). Die Helfer selbst stehen unter dem Mengen-Test CtestGrammatikHelfer...
    std::size_t aufrufe = 0, ohne_schalter = 0;
    for (EmittierterPlan const& p : pl) {
        for (std::string const& roh : p.zeilen) {
            CtestZeilenBefund const b = ctest_befund_der_zeile(roh);
            aufrufe += b.aufrufe;
            ohne_schalter += b.ohne_schutz;
            if (b.ohne_schutz > 0)
                ADD_FAILURE() << p.name << ": ctest-Aufruf ohne exaktes Argument --no-tests=error -- ein Lauf "
                              << "ohne passende Tests meldet rc=0 und faerbt das folgende Testat gruen: '"
                              << ohne_rand(roh) << "'";
        }
    }
    ASSERT_GT(aufrufe, 0u) << "kein ctest-Aufruf in den Plaenen -- die Suche griff nicht";
    EXPECT_EQ(ohne_schalter, 0u) << ohne_schalter << " von " << aufrufe
                                 << " emittierten ctest-Aufrufen lassen den Leerlauf als Erfolg durchgehen";
}

// (Nachsatz W0b-3) DIE GRAMMATIK SELBST, ueber eine MENGE von Formen geprueft statt ueber eine
// Annahme (T-4 Gegeneingang): jede Zeile traegt ihre erwartete Zaehlung; der Fehltext nennt die
// Zeile. Die Menge dokumentiert auch die BEWUSSTE Restunsicherheit (unquotiertes `echo ctest`
// zaehlt mit -- fail-closed: ein Zuviel erzwingt den Schalter, ein Zuwenig waere still).
TEST(TierCiYamlBuilder, CtestGrammatikHelferErkennenPraefixeUndExakteSchreibweise) {
    struct Form {
        char const* zeile;
        std::size_t aufrufe;
        std::size_t ohne_schutz;
        char const* warum;
    };
    constexpr std::array<Form, 18> formen{{
        // erkannt UND geschuetzt:
        {"      ctest --test-dir build -L pmc --no-tests=error --output-on-failure", 1, 0, "der Regelfall"},
        {"cd build && ctest -L pmc --no-tests=error", 1, 0, "Praefix-Kommando && ctest"},
        // erkannt, NICHT geschuetzt -- die Formen, die die alte Wache UEBERSAH:
        {"ctest -L pmc", 1, 1, "nackter Aufruf ohne Schalter"},
        {"\tctest\t-L pmc", 1, 1, "Tab-Einrueckung/-Trennung (alte Wache: rfind 'ctest ')"},
        {"/usr/bin/ctest -L pmc", 1, 1, "Pfad-Aufruf (alte Wache sah nur Zeilenanfang 'ctest ')"},
        {"if ! ctest -L pmc; then", 1, 1, "if-!-Guard ersetzt den Schalter NICHT (Leerlauf ist rc=0)"},
        {"COMDARE_X=1 ctest -L pmc", 1, 1, "Env-Praefix"},
        {"cd build && ctest -L pmc", 1, 1, "Kommando-Praefix"},
        {"ctest", 1, 1, "nacktes ctest ohne Argumente (alte Wache verlangte 'ctest ')"},
        // erkannt, NICHT geschuetzt -- die Schreibweisen, die die alte Wache faelschlich AKZEPTIERTE:
        {"ctest -L pmc --no-tests=ignore", 1, 1, "ignore ist das Gegenteil von error"},
        {"ctest -L pmc # --no-tests=error", 1, 1, "Schalter nur im Kommentar"},
        {"ctest -L pmc; echo --no-tests=error", 1, 1, "Schalter im NACHBAR-Kommando hinter ';'"},
        {"ctest -L pmc --no-tests=error-x", 1, 1, "Praefix-Token ist nicht das exakte Argument"},
        // NICHT erkannt (kein Aufruf):
        {"# ctest -L pmc", 0, 0, "reiner Kommentar"},
        {"      # Par. 66: ctest -L pmc kommt spaeter", 0, 0, "eingerueckter Kommentar"},
        {"ctest_registrierung foo", 0, 0, "anderes Wort mit ctest-Praefix"},
        {"myctest -L x", 0, 0, "anderes Wort mit ctest-Suffix ohne '/'"},
        // BEWUSSTE Restunsicherheit, dokumentiert statt verschwiegen:
        {"echo ctest ohne Anfuehrungszeichen", 1, 1, "fail-closed: unquotiertes Wort zaehlt mit"},
    }};
    for (Form const& f : formen) {
        CtestZeilenBefund const b = ctest_befund_der_zeile(f.zeile);
        EXPECT_EQ(b.aufrufe, f.aufrufe) << "Aufruf-Zaehlung fuer '" << f.zeile << "' (" << f.warum << ")";
        EXPECT_EQ(b.ohne_schutz, f.ohne_schutz) << "Schutz-Wertung fuer '" << f.zeile << "' (" << f.warum << ")";
    }
    // Nenner der Menge selbst: 18 Formen, davon 2 geschuetzt, 11 ungeschuetzt erkannt, 4 keine
    // Aufrufe, 1 dokumentierte Restunsicherheit -- eine leere Menge kann nicht gruen sein.
    static_assert(formen.size() == 18u);
}

// (Nachsatz W0b-3) DIE SetEAbbruch-KLASSIFIKATION BEKOMMT IHREN PRUEFER. Bisher war
// "[CEB-TESTAT]/[PMC-TESTAT] stehen unter set -euo pipefail hinter einem harten Kommando" eine
// Behauptung der Klassifikationstabelle -- KEIN Test verifizierte sie (Review-Mangel). Jetzt
// strukturell erzwungen, je Marke und je Emission: (1) die Marke steht in einem '- |'-Block,
// (2) `set -euo pipefail` laeuft VOR ihr im selben Block, (3) nichts weicht dazwischen auf
// (`set +e`, `|| true`), (4) mindestens ein hartes (Nicht-echo-)Kommando steht dazwischen --
// sonst gaebe es nichts, dessen Fehlschlag das Testat verhindert. Wird eine SetEAbbruch-Marke
// eines Tages hinter das fi eines Soft-Fail-Guards verschoben oder ihr Block entschaerft,
// faellt sie hier mit Zeilentext auf.
TEST(TierCiYamlBuilder, SetEAbbruchMarkenSindWirklichSetEGebunden) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    // (Nachsatz W0b-3) measure UND debug durch beide Builder -- der Debug-Zweig war ungeprueft.
    std::vector<EmittierterPlan> const pl = alle_wachen_plaene(*tp);

    std::size_t geprueft = 0;
    for (EmittierterPlan const& p : pl) {
        for (std::size_t i = 0; i < p.zeilen.size(); ++i) {
            std::string const marke = testat_marke_der_zeile(p.zeilen[i]);
            if (marke.empty()) continue;
            bool set_e_marke = false;
            for (auto const& eintrag : kTestatKlassifikation)
                if (eintrag.first == marke && eintrag.second == Bindung::SetEAbbruch) set_e_marke = true;
            if (!set_e_marke) continue;
            ++geprueft;
            SetEBindung const b = pruefe_set_e_bindung(p.zeilen, i);
            EXPECT_TRUE(b.im_pipe_block) << p.name << ": [" << marke
                                         << "] steht in keinem '- |'-Block -- als eigenes Listen-Element "
                                         << "truege es keinen set-e-Kontext: '" << b.beanstandet << "'";
            EXPECT_TRUE(b.set_e_davor) << p.name << ": [" << marke
                                       << "] ohne `set -euo pipefail` VOR der Marke im selben Block";
            EXPECT_FALSE(b.aufgeweicht) << p.name << ": [" << marke
                                        << "] Block zwischen set -e und Marke aufgeweicht: '" << b.beanstandet << "'";
            EXPECT_TRUE(b.hartes_kommando)
                << p.name << ": [" << marke << "] ohne hartes Kommando zwischen set -e und Marke -- "
                << "es gibt nichts, dessen Fehlschlag das Testat verhindert";
        }
    }
    ASSERT_GT(geprueft, 0u) << "keine SetEAbbruch-Marke in den 4 Plaenen -- daran prueft der Test nichts";
}

// (W0b-3) Die else-Bindung nimmt dem Fehlerpfad seine Fortschrittsinformation, wenn das Erfolgs-Testat
// Felder traegt, die das [FEHLER-TESTAT] nicht hat: `offen=` stand bisher NUR am Bau-Testat, das vor der
// Heilung auch nach einem Fehlschlag lief. Ab der else-Bindung faellt es fuer die gescheiterte Zelle
// ersatzlos weg -- genau dort, wo "wie viele noch offen" am meisten zaehlt. Beide Zeilen eines Guards
// tragen deshalb DIESELBEN Felder; das ist die Deckung dieser Forderung, nicht bloss ihre Absicht.
TEST(TierCiYamlBuilder, FehlerTestatTraegtDieselbenFelderWieSeinErfolgsTestat) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    // (Nachsatz W0b-3) measure UND debug durch beide Builder -- der Debug-Zweig war ungeprueft.
    std::vector<EmittierterPlan> const pl = alle_wachen_plaene(*tp);

    std::size_t gesamt = 0, paare = 0, ungleich = 0;
    for (EmittierterPlan const& p : pl) {
        std::vector<SoftFailGuard> const guards = soft_fail_guards(p.zeilen);
        gesamt += guards.size();
        for (SoftFailGuard const& g : guards) {
            if (!g.hat_else || !g.hat_erfolg) continue;
            ++paare;
            std::vector<std::string> const f_fehler = feldnamen(p.zeilen[g.fehler_zeile]);
            std::vector<std::string> const f_erfolg = feldnamen(p.zeilen[g.erfolg_zeile]);
            if (f_fehler == f_erfolg) continue;
            ++ungleich;
            ADD_FAILURE() << p.name << ": Guard-Paar mit ungleichem Feldsatz -- [FEHLER-TESTAT] traegt '"
                          << verkettet(f_fehler) << "', [" << testat_marke_der_zeile(p.zeilen[g.erfolg_zeile])
                          << "] traegt '" << verkettet(f_erfolg) << "'";
        }
    }
    // Der NENNER: jeder Guard muss ein vergleichbares Paar bilden, sonst prueft der Test weniger,
    // als er zu pruefen vorgibt.
    ASSERT_GT(gesamt, 0u) << "kein Soft-Fail-Guard in den Plaenen -- die Suche griff nicht";
    ASSERT_EQ(paare, gesamt) << paare << " vergleichbare Guard-Paare von " << gesamt << " Guards";
    EXPECT_EQ(ungleich, 0u) << ungleich << " von " << paare << " Guard-Paaren tragen ungleiche Feldsaetze";
}

// (S4-c) TraceHygieneAndTimeout: Treiber-Detail je Aufruf nach $LOGDIR-Artefakt-Datei (>...log 2>&1); artifacts
//        when:always mit den logs/-Verzeichnissen + expire_in 4 weeks; timeout: 7d an ALLEN Batch-Jobs.
TEST(TierCiYamlBuilder, TraceHygieneAndTimeout) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    // Detail-Output je Aufruf in Log-Artefakt-Dateien (im Job-Trace stehen NUR Testate).
    EXPECT_GT(count_occurrences(yaml, "2>&1; then"), 0u) << "if-guarded Log-Umleitung je Aufruf (Trace-Hygiene)";
    EXPECT_NE(yaml.find("_bau_${START}.log"), std::string::npos) << "Bau-Scheiben-Log-Datei";
    EXPECT_NE(yaml.find("_pruef.log"), std::string::npos) << "Pruef-Log-Datei";
    EXPECT_NE(yaml.find("_mess.log"), std::string::npos) << "Mess-Log-Datei";
    // artifacts when:always fuer die logs/-Verzeichnisse, expire_in 4 weeks (je Batch-Job).
    EXPECT_EQ(count_occurrences(yaml, "    when: always\n"), 4u) << "je Batch-Job artifacts when:always";
    EXPECT_EQ(count_occurrences(yaml, "    expire_in: 4 weeks\n"), 4u) << "je Batch-Job expire_in 4 weeks";
    EXPECT_NE(yaml.find("      - Code/gn_out/"), std::string::npos) << "Build-Batch logs/ als Artefakt";
    EXPECT_NE(yaml.find("      - Code/measure_out/"), std::string::npos) << "Mess-Batch logs/ als Artefakt";
    // timeout: 7d an allen Batch-Jobs (GN-11-Mehrtaegigkeit; Runner-maximum_timeout ist die Infra-Vorbedingung).
    EXPECT_EQ(count_occurrences(yaml, "  timeout: 7d"), 4u) << "timeout: 7d an allen 4 Batch-Jobs";
}

// (S4-d) LaneBudgetLiteralsNoNproc: beide Batch-Typen exportieren COMDARE_BUILD_PARALLEL als Lane-Budget-Literal
//        (User-Drosselung 23.07. mit GO: amd von 32 auf 24 wegen RAM-Bound/Swap-Thrashing-Evidenz, intel bleibt 24 =>
//        BEIDE Lanes 24), NIE $(nproc). Die fruehere 32/24-T-Wert-Lesart ist RAM-korrigiert.
TEST(TierCiYamlBuilder, LaneBudgetLiteralsNoNproc) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    // BEIDE Lanes (amd+intel) x 2 Batch-Typen (Build + Mess) => 4x das Lane-Literal "24".
    EXPECT_EQ(count_occurrences(yaml, "export COMDARE_BUILD_PARALLEL=\"24\""), 4u)
        << "beide Lanes 24 (amd von 32 gedrosselt, RAM-Bound) x Build+Mess = 4";
    EXPECT_EQ(yaml.find("$(nproc)"), std::string::npos) << "kein $(nproc) (harte Lane-Budget-Literale)";
    EXPECT_EQ(yaml.find("COMDARE_BUILD_PARALLEL=\"32\""), std::string::npos)
        << "amd 32 auf 24 gedrosselt (RAM-Bound/Swap-Evidenz 23.07.)";
    EXPECT_EQ(yaml.find("COMDARE_BUILD_PARALLEL=\"16\""), std::string::npos)
        << "der alte konservative K-Wert 16 ist ersetzt";
}

// (DRINGEND, 2026-07-23 Resume-CI-Fix) BatchJobsCarryGnOutPersistenceFlags: BEIDE STUFE-2-Batch-Typen (Build + Mess)
//     tragen je GIT_CLEAN_FLAGS mit -e-Ausnahme fuer Code/gn_out + Code/build UND GIT_STRATEGY:fetch. Sonst loescht der
//     GitLab-Checkout-Default 'git clean -ffdx' die Bau-Artefakte (.so + .version-Sidecar) am Job-Start und der
//     per-Binary-Resume (dll_is_current) ist ueber Job-Grenzen wirkungslos. Der Flag-Count ist an die Batch-Job-Zahl
//     gebunden (Build + Mess je Host) -- so beweist er, dass BEIDE Typen die Flags tragen (kein hartkodierter Wert).
TEST(TierCiYamlBuilder, BatchJobsCarryGnOutPersistenceFlags) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    // §64/§62-B: je Host EIN Build-Batch + EIN Mess-Batch; beide Typen sind vorhanden (all_axes_golden: amd + intel).
    std::size_t const build_batches   = count_occurrences(yaml, "# JOB tier-build-batch ");
    std::size_t const measure_batches = count_occurrences(yaml, "# JOB measure-batch ");
    ASSERT_GT(build_batches, 0u) << "es gibt Build-Batch-Jobs";
    ASSERT_GT(measure_batches, 0u) << "es gibt Mess-Batch-Jobs";
    std::size_t const total_batches = build_batches + measure_batches;

    // Jeder Batch-Job (Build + Mess, je Host) traegt beide Flags GENAU EINMAL -> Count == Batch-Job-Zahl (=> BEIDE
    // Typen tragen sie; waeren sie nur im Build, waere der Count == build_batches < total_batches).
    // G4a P-C: der Literal-Pin wandert um "-e Code/measure_out" (Mess-CSV ueberleben den Checkout-Clean). Die
    // Zaehl-Semantik (== total_batches, also BEIDE Batch-Typen) bleibt unveraendert -- es ist bewusst DIESELBE Zeile
    // in Bau- und Mess-Batch (im Bau-Batch existiert measure_out nicht, der Ausschluss ist dort folgenlos).
    EXPECT_EQ(
        count_occurrences(yaml, "    GIT_CLEAN_FLAGS: \"-ffdx -e Code/gn_out -e Code/build -e Code/measure_out\"\n"),
        total_batches)
        << "beide Batch-Typen je Host tragen den gn_out/build/measure_out-Clean-Ausschluss (dll_is_current ueberlebt, "
           "Messdaten ueberleben)";
    EXPECT_EQ(yaml.find("    GIT_CLEAN_FLAGS: \"-ffdx -e Code/gn_out -e Code/build\"\n"), std::string::npos)
        << "die Vor-G4a-Form ohne measure_out darf nirgends mehr stehen (sonst haette ein Batch den Schutz nicht)";
    EXPECT_EQ(count_occurrences(yaml, "    GIT_STRATEGY: \"fetch\"\n"), total_batches)
        << "beide Batch-Typen je Host tragen GIT_STRATEGY:fetch (kein Voll-Clone, Workdir erhalten)";
    // KLASSEN-Regel: die neuen Job-variables tragen KEIN $CI_PROJECT_DIR (workdir-relative Pfade).
    EXPECT_EQ(yaml.find("GIT_CLEAN_FLAGS: \"-ffdx -e $CI_PROJECT_DIR"), std::string::npos)
        << "kein $CI_PROJECT_DIR in den Persistenz-Flags (KLASSE)";
}

// (G4a P-A/P-B/#37) Die vier unbedingten Emissions-Ergaenzungen der G4a-Scheibe, jeweils mit ihrer REIHENFOLGE-
// Bedingung -- die Platzierung ist bei dreien korrektheits-relevant, nicht kosmetisch:
//   Storage-Aktivierung: in BEIDEN Batches, vor jedem Treiber-Aufruf (sonst laeuft der Lauf storage-inert);
//   PMC-Preflight: im Mess-Batch VOR der ersten Messung (sonst misst eine kaputte Lane tagelang Nullen);
//   PRUNE: im Mess-Batch NACH der letzten Messung (sonst Netz-Rueckweg, s. D1);
//   CSV-Artefakt: zusaetzlich zu den Logs (sonst ueberleben die Messdaten keinen Runner-Verlust).
TEST(TierCiYamlBuilder, G4aStorageActivationPmcPreflightAndPruneAreEmittedInCorrectOrder) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb; // die Batch-Jobs stehen in der STUFE-2-Tier-YAML, nicht in der Child-1
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    std::size_t const build_batches   = count_occurrences(yaml, "# JOB tier-build-batch ");
    std::size_t const measure_batches = count_occurrences(yaml, "# JOB measure-batch ");
    ASSERT_GT(build_batches, 0u);
    ASSERT_GT(measure_batches, 0u);
    std::size_t const total_batches = build_batches + measure_batches;

    // (a) P-A Storage-Aktivierung: EINE Literal-Quelle, genau einmal je Batch (Bau UND Mess).
    EXPECT_EQ(count_occurrences(yaml, "      . external/comdare-cache-engine/scripts/comdare_storage_activation.sh\n"),
              total_batches)
        << "das Aktivierungs-Skript wird in JEDEM Batch gesourct (inert ohne COMDARE_STORAGE_CACHE)";
    EXPECT_EQ(count_occurrences(yaml, "        export COMDARE_ARTEFAKT_TRIES=\"${COMDARE_ARTEFAKT_TRIES:-2}\"\n"),
              total_batches)
        << "der Blackhole-Deckel steht in jedem Batch";
    // set -u-Haerte: der Schalter wird NIE unquoted/ungeschuetzt expandiert (der Block laeuft unter set -euo pipefail).
    EXPECT_EQ(yaml.find("[ \"$COMDARE_STORAGE_CACHE\" ="), std::string::npos)
        << "unter set -u waere die ungeschuetzte Expansion ein Abbruch -- immer ${VAR:-}";
    // KLASSE: workdir-relativer Skript-Pfad, kein $CI_PROJECT_DIR.
    EXPECT_EQ(yaml.find("$CI_PROJECT_DIR/Code/external/comdare-cache-engine/scripts/"), std::string::npos)
        << "Skript-Pfad bleibt workdir-relativ (KLASSE)";

    // (b) #37 PMC-Preflight: je Mess-Batch genau einmal, und VOR der ersten Messung dieses Batches.
    //     --no-tests=error ist seit W0b-3 Teil der erwarteten Bytes und KEIN Beiwerk: ohne den Schalter meldet
    //     ctest bei null passenden Tests rc=0, `set -e` greift nicht, und das [PMC-TESTAT] darunter faerbt einen
    //     Preflight gruen, der nichts ausgefuehrt hat.
    EXPECT_EQ(count_occurrences(yaml, "      ctest --test-dir build -L pmc --no-tests=error --output-on-failure\n"),
              measure_batches)
        << "je Mess-Batch ein PMC-Preflight, der den Leerlauf zum Fehler macht";
    EXPECT_EQ(count_occurrences(yaml, "      cmake --build build --target m3v2_pmc_smoke linux_perf_pmc_smoke\n"),
              measure_batches);
    // Die Reihenfolge-Invarianten gelten PRO JOB, nicht global: die YAML traegt mehrere Mess-Batches (je Lane einen),
    // und deren Bloecke stehen hintereinander. Global geprueft waere "letzte Messung vor erstem Prune" falsch (der
    // Prune der amd-Lane steht vor der letzten Messung der intel-Lane), obwohl innerhalb jedes Jobs alles stimmt.
    auto measure_job_blocks = [&yaml] {
        std::vector<std::string> blocks;
        std::size_t              pos = yaml.find("# JOB measure-batch ");
        while (pos != std::string::npos) {
            std::size_t const next = yaml.find("\n# JOB ", pos + 1);
            blocks.push_back(yaml.substr(pos, next == std::string::npos ? std::string::npos : next - pos));
            pos = yaml.find("# JOB measure-batch ", pos + 1);
        }
        return blocks;
    }();
    ASSERT_EQ(measure_job_blocks.size(), measure_batches);

    for (std::string const& blk : measure_job_blocks) {
        std::size_t const pmc        = blk.find("[PMC-PREFLIGHT]");
        std::size_t const first_mess = blk.find("== [MESS] zelle=");
        ASSERT_NE(pmc, std::string::npos) << "jeder Mess-Batch hat einen Preflight";
        ASSERT_NE(first_mess, std::string::npos);
        EXPECT_LT(pmc, first_mess) << "der PMC-Preflight steht VOR der ersten Messung DIESES Jobs";
    }
    // Hart in BEIDEN Profilen (D4): kein allow-failure-Kunstgriff am Preflight.
    EXPECT_EQ(yaml.find("ctest --test-dir build -L pmc --output-on-failure || true"), std::string::npos)
        << "der Preflight darf nicht weich gemacht werden (§66-N2: beide hart)";

    // (c) P-B PRUNE: guarded, je Perm, NICHT-fatal, und NACH der letzten Messung.
    EXPECT_NE(yaml.find("        COMDARE_PRUNE_ONLY=true \"$DRIVER\" experiment_config "), std::string::npos)
        << "der Prune-Schritt wird emittiert";
    EXPECT_EQ(count_occurrences(yaml, "COMDARE_PRUNE_ONLY=true"),
              count_occurrences(yaml, "        echo \"== [PRUNE] zelle="))
        << "je Prune-Aufruf genau ein [PRUNE]-Kopf (je Perm einer)";
    EXPECT_NE(yaml.find("experiment_config \"$CI_PROJECT_DIR/Code/measure_out/"), std::string::npos);
    EXPECT_EQ(count_occurrences(yaml, "COMDARE_PRUNE_ONLY=true \"$DRIVER\" experiment_config \""),
              count_occurrences(yaml, "\" || true\n"))
        << "jeder Prune-Aufruf ist NICHT-fatal (|| true; D2)";
    for (std::string const& blk : measure_job_blocks) {
        std::size_t const last_mess   = blk.rfind("[MESS-TESTAT]");
        std::size_t const first_prune = blk.find("COMDARE_PRUNE_ONLY=true");
        ASSERT_NE(last_mess, std::string::npos);
        ASSERT_NE(first_prune, std::string::npos) << "jeder Mess-Batch raeumt am Ende lokal auf";
        EXPECT_LT(last_mess, first_prune)
            << "PRUNE laeuft NACH der letzten Messung DIESES Jobs (D1: sonst muesste der Mess-Batch alles "
               "ueber das Netz zurueckziehen)";
    }
    // Der Prune darf in KEINEM Bau-Batch stehen -- genau das war die verworfene Variante (D1).
    {
        std::size_t pos = yaml.find("# JOB tier-build-batch ");
        ASSERT_NE(pos, std::string::npos);
        while (pos != std::string::npos) {
            std::size_t const next = yaml.find("\n# JOB ", pos + 1);
            std::string const blk  = yaml.substr(pos, next == std::string::npos ? std::string::npos : next - pos);
            EXPECT_EQ(blk.find("COMDARE_PRUNE_ONLY"), std::string::npos)
                << "kein Prune im Bau-Batch (die Binaries muessen fuer den Mess-Batch lokal liegen bleiben)";
            pos = yaml.find("# JOB tier-build-batch ", pos + 1);
        }
    }

    // (d) P-C: die Mess-CSV reisen als Artefakt mit (bisher nur die Logs).
    EXPECT_EQ(count_occurrences(yaml, "/**/*.csv\n"), measure_batches) << "je Mess-Batch ein CSV-Artefakt-Glob";
    EXPECT_NE(yaml.find("      - Code/measure_out/"), std::string::npos);
}

// (G4a-7) Die Planer-Vorreservierung als WERT: rein, uhrfrei, deterministisch. Der ce liefert sie, der Host schreibt
// sie -- deshalb ist sie hier vollstaendig testbar, ohne Transport und ohne die Emitter-Reinheit anzutasten.
TEST(PlanerBlockReservation, ProFormaShapeIsExactAndClockFree) {
    auto const r = planner::make_planer_block_reservation("6f1c2b3a-0000-4444-8888-abcdefabcdef", 7, "prod1", 24,
                                                          "2026-07-26T05:00:00Z", "2026-07-26T05:30:00Z");
    namespace bl = ::comdare::cache_engine::builder::bestandslog;

    EXPECT_EQ(r.id, "6f1c2b3a-0000-4444-8888-abcdefabcdef/7") << "id = owner_uuid/seq";
    EXPECT_EQ(r.typ, bl::BatchTyp::planer_block) << "der Typ trennt die Planer-Sperre vom tier-Slice";
    EXPECT_EQ(r.maschine, "prod1");
    EXPECT_EQ(r.threads, 24u);
    // Ein planer_block reserviert KEINE Datenscheibe -- er blockiert die Strecke als Ganzes.
    EXPECT_EQ(r.slice_begin, 0u);
    EXPECT_EQ(r.slice_count, 0u);
    EXPECT_EQ(r.reserviert_utc, "2026-07-26T05:00:00Z");
    EXPECT_EQ(r.pro_forma_bis_utc, "2026-07-26T05:30:00Z") << "30min pro-forma (B7), vom Aufrufer geliefert";
    // OHNE ETA: die kommt erst mit der Kalibrierung (apply_calibration), nie schon bei der Vorreservierung.
    EXPECT_EQ(r.eta_s, "");
    EXPECT_EQ(r.avg_size_bytes, "");
    EXPECT_EQ(r.status, bl::BatchStatus::offen);

    // Uhrfrei => zwei Aufrufe mit denselben Argumenten sind identisch (kein versteckter now()-Zugriff).
    auto const again = planner::make_planer_block_reservation("6f1c2b3a-0000-4444-8888-abcdefabcdef", 7, "prod1", 24,
                                                              "2026-07-26T05:00:00Z", "2026-07-26T05:30:00Z");
    EXPECT_EQ(r, again);

    // Der Typ-Name serialisiert stabil (das Dokument-Format kennt ihn).
    EXPECT_EQ(bl::to_string(bl::BatchTyp::planer_block), "planer_block");
    EXPECT_EQ(bl::batch_typ_from_string("planer_block"), bl::BatchTyp::planer_block);
}

// (G4b-2/E4) Der Wert-Kern liegt jetzt in bestandslog/planer_block_value.hpp und nimmt eine EXPLIZITE id; der
// Director-Helfer ist nur noch eine Huelle, die `owner_uuid + "/" + seq` als id einsetzt. Diese Wache haelt beides
// fest: die Delegation ist wertgleich, und der Kern akzeptiert die zweite legitime id-Form (E2, owner + "/planer"),
// die der Helfer strukturell nicht erzeugen kann.
TEST(PlanerBlockReservation, DelegatesToValueCoreAndAcceptsExplicitId) {
    namespace bl = ::comdare::cache_engine::builder::bestandslog;

    // (a) Wertgleichheit: Huelle == Kern mit derselben, von der Huelle gebildeten id.
    auto const via_huelle = planner::make_planer_block_reservation("6f1c2b3a-0000-4444-8888-abcdefabcdef", 7, "prod1",
                                                                   24, "2026-07-26T05:00:00Z", "2026-07-26T05:30:00Z");
    auto const via_kern = bl::make_planer_block_reservation_value("6f1c2b3a-0000-4444-8888-abcdefabcdef/7", "prod1", 24,
                                                                  /*ceb_legende=*/"", /*ceb_key_sha512=*/"",
                                                                  "2026-07-26T05:00:00Z", "2026-07-26T05:30:00Z");
    EXPECT_EQ(via_huelle, via_kern) << "der Director-Helfer ist eine reine Delegation, kein zweiter Wert-Aufbau";
    EXPECT_TRUE(via_huelle.ceb_legende.empty()) << "die Huelle meldet keine CEB-Bindung -- das tut der Emissions-Pfad";
    EXPECT_TRUE(via_huelle.ceb_key_sha512.empty());

    // (b) Die E2-id-Form: eine Sperre je LAUF, nicht je Sequenz. Ueber die seq-Huelle nicht erzeugbar.
    auto const planer = bl::make_planer_block_reservation_value("prod1-job-4711@prod1/planer", "prod1", 24,
                                                                /*ceb_legende=*/"", /*ceb_key_sha512=*/"",
                                                                "2026-07-26T05:00:00Z", "2026-07-26T05:30:00Z");
    EXPECT_EQ(planer.id, "prod1-job-4711@prod1/planer");
    EXPECT_EQ(planer.typ, bl::BatchTyp::planer_block);
    EXPECT_EQ(planer.maschine, "prod1") << "maschine ist ein EIGENES Feld, nicht aus der id zurueckgelesen";
    EXPECT_EQ(planer.slice_begin, 0u) << "ein planer_block sperrt die Strecke, keine Datenscheibe";
    EXPECT_EQ(planer.slice_count, 0u);
    EXPECT_EQ(planer.status, bl::BatchStatus::offen);
    EXPECT_TRUE(planer.eta_s.empty()) << "pro forma == ohne ETA";

    // (c) Uhrfrei auch im Kern: zwei Aufrufe mit denselben Argumenten sind identisch.
    auto const again = bl::make_planer_block_reservation_value("prod1-job-4711@prod1/planer", "prod1", 24,
                                                               /*ceb_legende=*/"", /*ceb_key_sha512=*/"",
                                                               "2026-07-26T05:00:00Z", "2026-07-26T05:30:00Z");
    EXPECT_EQ(planer, again);
}

// (G4b-2/2.4-(7)) ceb:emit war der EINZIGE Job ohne Storage-Scharfschaltung -- und zugleich der Job, der
// --emit-tier-ci faehrt. Ohne den Aufruf waere dort Push/Pull inert und eine Bestandslog-Reservierung Fiktion.
// Die Aktivierung steht jetzt in JEDEM ceb:emit-Job der Stufe-1-YAML, aus DERSELBEN Literal-Quelle wie in Stufe 2.
TEST(CiYamlBuilder, CebEmitJobCarriesStorageActivation) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::ExperimentPlanDirector const director;
    planner::CiYamlBuilder                yb;
    director.construct(*tp, yb);
    std::string const& yaml = yb.text();

    std::size_t const ceb_emit_jobs = count_occurrences(yaml, "# JOB ceb-emit combo ");
    ASSERT_GT(ceb_emit_jobs, 0u);
    EXPECT_EQ(count_occurrences(yaml, "      . external/comdare-cache-engine/scripts/comdare_storage_activation.sh\n"),
              ceb_emit_jobs)
        << "genau einmal je ceb:emit-Job -- und in keinem anderen Stufe-1-Job";
    EXPECT_EQ(count_occurrences(yaml, "        export COMDARE_ARTEFAKT_TRIES=\"${COMDARE_ARTEFAKT_TRIES:-2}\"\n"),
              ceb_emit_jobs)
        << "der Blackhole-Deckel reist mit derselben Literal-Quelle";
    // set -u-Haerte und KLASSE-Pfad gelten hier genauso wie in Stufe 2 (eine Quelle, ein Verhalten).
    EXPECT_EQ(yaml.find("[ \"$COMDARE_STORAGE_CACHE\" ="), std::string::npos);
    EXPECT_EQ(yaml.find("$CI_PROJECT_DIR/Code/external/comdare-cache-engine/scripts/"), std::string::npos);
    // Die Aktivierung steht NACH der DRIVER-Ermittlung (sie braucht das gebaute Binary nicht, aber die
    // Reihenfolge ist an den drei Aufrufstellen dieselbe -- eine Quelle, ein Muster).
    auto const pos_driver = yaml.find("      DRIVER=$(find build -type f -name \"comdare-messung-driver\"");
    auto const pos_activ  = yaml.find("      . external/comdare-cache-engine/scripts/comdare_storage_activation.sh\n");
    ASSERT_NE(pos_driver, std::string::npos);
    ASSERT_NE(pos_activ, std::string::npos);
    EXPECT_LT(pos_driver, pos_activ);
}

// (#29+#27, 2026-07-23) BatchJobsCarryCancelTrapAndHeartbeatTee: jeder STUFE-2-Batch-Job (Build + Mess je Host) beginnt
//     seinen work-Block mit dem #29-Cancel-Trap (SIGTERM/INT -> eigene Prozessgruppe beenden, keine Waisen) UND leitet
//     jede LOG-umgeleitete Treiber-Invocation ueber die #27-tee-Naht (Detail nach <log>, [heartbeat]-Zeilen zusaetzlich
//     line-gepuffert in den Trace via Process-Substitution -> Treiber-Exit-Code unangetastet).
TEST(TierCiYamlBuilder, BatchJobsCarryCancelTrapAndHeartbeatTee) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    std::size_t const build_batches   = count_occurrences(yaml, "# JOB tier-build-batch ");
    std::size_t const measure_batches = count_occurrences(yaml, "# JOB measure-batch ");
    ASSERT_GT(build_batches, 0u);
    ASSERT_GT(measure_batches, 0u);
    std::size_t const total_batches = build_batches + measure_batches;

    // #29 Cancel-Trap: genau EINER je Batch-Job (work-Block) -> Count == Batch-Job-Zahl (Build + Mess je Host). Der Trap
    // demontiert sich zuerst (kein Re-Entry-Loop), dann kill -- -$$ (eigene Gruppe: bash-Loop+Driver+Compiler).
    EXPECT_EQ(count_occurrences(yaml, "trap 'trap - TERM INT; kill -- -$$ 2>/dev/null' TERM INT"), total_batches)
        << "jeder Batch-Job beginnt den work-Block mit dem #29-Cancel-Trap (keine Waisen)";
    // #27 Heartbeat-tee: Process-Substitution 2> >(tee -a <log> | grep -F -e ... >&2) -- an den LOG-umgeleiteten
    // Treiber-Invocations (Build-Window + Pruef im Build-Batch, Release-Mess im Mess-Batch). Presence-Wache (>=1).
    std::size_t const tee_count = count_occurrences(yaml, "2> >(tee -a \"$LOGDIR/perm");
    EXPECT_GT(tee_count, 0u)
        << "Treiber-Detail via Process-Substitution (nicht Pipe -> Exit-Code des Treibers bleibt der Wahrheitswert)";
    // E-04-P1 (Teil 1c, MIGRIERT): der Filter laesst jetzt [heartbeat] PLUS die Marken der Marker-Familie v2
    // durch. Die Erwartung wird aus bex::kSliceMarkerTraceMarken ABGELEITET, nicht handgelistet -- eine vierte
    // Marke bricht diesen Pin sofort, statt still am Job-Trace vorbeizulaufen (Lehre: gruene Tests zementieren
    // alte Ordnung). Der Filter-Anfang (-F -e '[heartbeat]') bleibt der unveraenderte Trace-Hygiene-Kern.
    std::string erwarteter_filter = "grep --line-buffered -F -e '[heartbeat]'";
    for (std::string_view const marke : bex::kSliceMarkerTraceMarken)
        erwarteter_filter += " -e '[" + std::string{marke} + "]'";
    erwarteter_filter += " >&2";
    EXPECT_GT(count_occurrences(yaml, erwarteter_filter), 0u)
        << "der Tee-Filter fuehrt [heartbeat] + JEDE Marke der Marker-Familie v2 (sonst waere der Kanal stumm)";
    // Symmetrie: so viele Filter wie tee-Redirects (jede getee'te Invocation traegt beide Haelften).
    EXPECT_EQ(tee_count, count_occurrences(yaml, erwarteter_filter))
        << "jede tee-Invocation traegt genau EINEN (vollstaendigen) Marker-Filter";
    // NEGATIV: der alte, marker-blinde Filter darf NICHT mehr vorkommen -- sonst liefe eine Invocation weiter
    // mit der Vor-E-04-P1-Liste und ihre Fortschritts-Zeilen blieben im Artefakt-Log haengen.
    EXPECT_EQ(count_occurrences(yaml, "grep --line-buffered -F '[heartbeat]' >&2"), 0u)
        << "kein Rest des marker-blinden Ein-Muster-Filters";
    // Review-Fix (Offset-Kollaps): JEDE getee'te Invocation truncated ihr LOG ZUERST explizit (: > <log>) und schreibt
    // DANN im APPEND-Modus (>> <log> + tee -a). O_APPEND-Writes sind atomar ans Dateiende -> stdout ueberschreibt keine
    // per tee angehaengten stderr-/[FEHLER-TESTAT]-Zeilen mehr. Truncate-Zahl == Append-Zahl == tee-Zahl.
    EXPECT_EQ(count_occurrences(yaml, ": > \"$LOGDIR/perm"), tee_count)
        << "jede getee'te Treiber-Invocation truncated ihr LOG genau einmal explizit (Retry-fest)";
    EXPECT_EQ(count_occurrences(yaml, ">> \"$LOGDIR/perm"), tee_count)
        << "die Treiber-Invocation schreibt stdout im APPEND-Modus (>>), NICHT O_TRUNC (kein Offset-Kollaps mit tee "
           "-a)";
}

// (E-04-P1) SliceKanalMarkerFamilyV2Emission: die Emissions-Seite des Live-Fortschritts-Kanals.
//   Der Owner-KERN verlangt "live sehen ... ob der CacheEngineBuilder Orchestrator gebaut wird und exakt WELCHE
//   Tier-Binary Rekombinationen und WIE VIELE davon noch offen sind". Der Treiber liefert die Fenster-Zahlen
//   ([PLAN-TESTAT]/[BILANZ-TESTAT], eigener Iterator-Anker); die Emission liefert die drei Bezugsgroessen, ohne
//   die eine Fenster-Zeile nicht einordenbar waere: die Lane als Vertrag (COMDARE_LANE), die Gesamt-Zahl der
//   Fenster + Perms im Batch-KOPF und den Rest-Zaehler offen= je Schritt-Testat.
TEST(TierCiYamlBuilder, SliceKanalMarkerFamilyV2Emission) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    // (a) LANE-VERTRAG: je Batch-Job (2 Build + 2 Mess) genau EIN export COMDARE_LANE mit dem Lane-Namen des
    //     Jobs. Der Treiber raet die Lane nie -- der emittierende Planer sagt sie.
    EXPECT_EQ(count_occurrences(yaml, "export COMDARE_LANE=\"amd\""), 2u)
        << "amd: Build-Batch + Mess-Batch exportieren ihre Lane";
    EXPECT_EQ(count_occurrences(yaml, "export COMDARE_LANE=\"intel\""), 2u)
        << "intel: Build-Batch + Mess-Batch exportieren ihre Lane";

    // (b) BATCH-KOPF: die Fenster-Gesamtzahl ist Shell-arithmetisch (byte-deterministisch, kein Planer-Literal --
    //     TOTAL ist erst zur Laufzeit bekannt), die Perm-Zahl ein Planer-Literal (er kennt seine Lane-Perms).
    EXPECT_EQ(count_occurrences(yaml, "fenster_gesamt=$(( (TOTAL + SLICE - 1) / SLICE ))"), 2u)
        << "je Build-Batch EINE Fenster-Gesamtzahl im KOPF (Aufrundung, kein Ganzzahl-Abschnitt)";
    EXPECT_NE(yaml.find("[BATCH-BAU] ceb=[all] lane=amd"), std::string::npos) << "KOPF-Grammatik unveraendert";
    EXPECT_EQ(count_occurrences(yaml, " perms="), 2u) << "je Build-Batch EINE Perm-Zahl im KOPF";

    // (c) OFFEN-ZAEHLER: jede Zeile der Bau-Fenster-Schleife traegt den Rest der Perm. Seit W0b-3 sind das ZWEI
    //     je Fenster: das [TESTAT] im else-Zweig UND sein [FEHLER-TESTAT]-Zwilling. Vorher lief das [TESTAT] auch
    //     nach einem Fehlschlag und trug den Zaehler dabei mit; mit der else-Bindung faellt es fuer das
    //     gescheiterte Fenster weg, und ohne offen= auf der Fehler-Zeile verloere genau der Fehlerpfad die
    //     Fortschrittsinformation -- also dort, wo "wie viele davon noch offen" am meisten zaehlt.
    std::size_t const testate = count_occurrences(yaml, "[TESTAT] ts=");
    EXPECT_GT(testate, 0u) << "es gibt Schritt-Testate der Fenster-Schleife";
    EXPECT_EQ(count_occurrences(yaml, " offen=$(( TOTAL - START - COUNT ))"), 2u * testate)
        << "je Bau-Fenster ZWEI Zeilen mit Rest-Zaehler (Erfolg + Fehler), kein zaehlerloser Pfad mehr";
    // Der Zwilling namentlich, damit die 2u oben nicht bloss eine Zahl ist, die irgendwie aufgeht.
    EXPECT_NE(yaml.find("[FEHLER-TESTAT] ts=$(date -u +%FT%TZ) lane=amd zelle=[O2,no_extension]"
                        "[search_algo,cache_traversal,mapping] phase=bau fenster=${START}:${COUNT} "
                        "offen=$(( TOTAL - START - COUNT ))"),
              std::string::npos)
        << "das Bau-[FEHLER-TESTAT] traegt den Rest-Zaehler (W0b-3)";

    // (d) STUFE 1: das CEB-Bau-Ereignis maschinenlesbar (der Owner-Teil "ob der CEB gebaut wird"). Genau EINES je
    //     CEB-Strecke -- hier ist die Stufe-1-YAML eine andere Builder-Ausgabe, deshalb separat erhoben.
    planner::CiYamlBuilder cb;
    director.construct(*tp, cb);
    std::string const& stufe1 = cb.text();
    EXPECT_EQ(count_occurrences(stufe1, "[CEB-TESTAT] ts="), 1u) << "je CEB-Strecke EIN maschinenlesbares Bau-Testat";
    EXPECT_NE(stufe1.find("[CEB-TESTAT] ts=$(date -u +%FT%TZ) ceb=[all] status=gebaut"), std::string::npos)
        << "Testat-Grammatik: Marke, ts=, dann Felder";
}

// (E-04-P1, Section 61-Dual-Weg) SliceKanalLaneReachesBareMetalPath: der bare-metal-Weg (CMake-Targets) traegt
//   DIESELBE Lane-Aussage wie die CI-Emission. Fehlte sie dort, stuende der lokale Dual-Weg-Beweis mit
//   lane=unbelegt gegen die CI-Zeilen -- die beiden Wege waeren nicht mehr vergleichbar (Section 61 verlangt
//   genau diese Vergleichbarkeit).
TEST(TierCmakeGraphBuilder, SliceKanalLaneReachesBareMetalPath) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::TierCmakeGraphBuilder        cm;
    director.construct(*tp, cm);
    std::string const& txt = cm.text();

    // Je Treiber-COMMAND genau eine Lane-Zeile: die Zahl muss der Zahl der COMDARE_GN_SIMD-Zeilen entsprechen
    // (abgeleitet, nicht handgezaehlt -- kommt eine Perm/ein COMMAND dazu, faellt der Pin).
    std::size_t const simd_zeilen = count_occurrences(txt, "\"COMDARE_GN_SIMD=");
    EXPECT_GT(simd_zeilen, 0u) << "der bare-metal-Weg emittiert Treiber-COMMANDs mit Perm-Selektion";
    EXPECT_EQ(count_occurrences(txt, "\"COMDARE_LANE="), simd_zeilen)
        << "JEDER Treiber-COMMAND des bare-metal-Wegs traegt seine Lane (Section 61-Dual-Weg-Symmetrie)";
    EXPECT_NE(txt.find("\"COMDARE_LANE=amd\""), std::string::npos) << "amd-Lane im bare-metal-Weg benannt";
    EXPECT_NE(txt.find("\"COMDARE_LANE=intel\""), std::string::npos) << "intel-Lane im bare-metal-Weg benannt";
}

// (S4-e) EmptyLaneEmitsNoJobPair (Leere-Lane-Regel): ein Profil mit NUR avx2-simd routet alle Perms nach intel =>
//        NUR das intel-Job-Paar, kein amd-Batch (kein toter needs-Verweis).
TEST(TierCiYamlBuilder, EmptyLaneEmitsNoJobPair) {
    auto tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    tp->external_utils.simd_options = {"avx2"}; // nur avx2 -> measure_host_lane immer intel -> amd-Bucket leer
    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    EXPECT_NE(yaml.find("\"tier:build-batch:intel\":"), std::string::npos) << "intel-Batch vorhanden";
    EXPECT_NE(yaml.find("\"measure:[all]:batch:intel\":"), std::string::npos) << "intel-Mess-Batch vorhanden";
    EXPECT_EQ(yaml.find("\"tier:build-batch:amd\":"), std::string::npos) << "kein amd-Build-Batch (Leere-Lane-Regel)";
    EXPECT_EQ(yaml.find("\"measure:[all]:batch:amd\":"), std::string::npos) << "kein amd-Mess-Batch";
    EXPECT_EQ(count_occurrences(yaml, "# JOB tier-build-batch "), 1u) << "nur EIN Build-Batch (intel)";
    EXPECT_EQ(count_occurrences(yaml, "# JOB measure-batch "), 1u) << "nur EIN Mess-Batch (intel)";
}

// (W8) W10-Nacharbeit 2 (Serie-E2E 11569/11576): die self-contained Child-YAMLs erben default:before_script NICHT
//      -> JEDER Bau-Job beider Stufen traegt den Submodul-Klon-PROLOG (Deploy-Token via CI-Variablen, NIE Klartext)
//      + global GIT_SUBMODULE_STRATEGY:none (kein Auto-Fetch, der am extraheader failt).
TEST(CiYamlBuilder, BothStagesEmitSubmoduleClonePrologInBuildJobs) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;

    planner::CiYamlBuilder     s1; // Stufe 1: ceb:build + ceb:emit = 2 Bau-Jobs
    planner::TierCiYamlBuilder s2; // Stufe 2 (S4-§62-B-Batch): 2 Build-Batches + 2 Mess-Batches = 4 Jobs
    director.construct(*tp, s1);
    director.construct(*tp, s2);
    std::string const& y1 = s1.text();
    std::string const& y2 = s2.text();

    // Prolog-Marker: 1 je Job-mit-Klon-Prolog. Stufe 1 = 2 (ceb:build + ceb:emit); Stufe 2 (S4-§62-B-Batch) =
    // 2 Build-Batches + 2 Mess-Batches = 4 (je Batch-Job self-clone).
    EXPECT_EQ(count_occurrences(y1, "# CHILD-SUBMODULE-KLON"), 1u * 2u) << "ceb:build + ceb:emit (§64: 1 [all]-Combo)";
    EXPECT_EQ(count_occurrences(y2, "# CHILD-SUBMODULE-KLON"), 4u)
        << "je Build-Batch + je Mess-Batch (2 Host-Lanes = 4 Batch-Jobs)";
    // W10-Nacharbeit 3: ccache-Env per RUNTIME-Shell-Export im Prolog (1 je Job-mit-Prolog). Stufe 1 = 2, Stufe 2 = 4.
    EXPECT_EQ(count_occurrences(y1, "export CCACHE_DIR=\"${CI_PROJECT_DIR}/.ccache\""), 1u * 2u);
    EXPECT_EQ(count_occurrences(y2, "export CCACHE_DIR=\"${CI_PROJECT_DIR}/.ccache\""), 4u);
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
//       S4-§62-B-Batch: die Trichotomie testet die Selektor-Semantik ueber die je-Host-Batch-Emission je Combo:
//       (i) leerer Selektor = Identitaet (voller 3-Combo-Walk); (ii) EIN realer Selektor "_wallclock_" = echtes
//       nicht-leeres Subset (< voller Walk); (iii) Miss = ehrlich leere Stufe-2 (0 Batch-Jobs), kein Crash. Die
//       Batch-Zahl je Combo ist NICHT uniform (measure_host_lane routet no_extension bei [macro] nach intel => nur
//       EIN nicht-leerer Bucket), daher kein *3-Verhaeltnis, sondern deterministische Batch-Zahlen.
TEST(SelectMeasurementCombo, SelectorTrichotomyIdentitySubsetMissOnFannedFixture) {
    // §64: der all_axes-Default ist jetzt EINE [all]-Combo (Klasse-A-bewiesen). Dieser Test stellt die 3-Tool-XML-
    // OPTION EXPLIZIT nach (measurement_tooling-Override) -> die N>1-Selektor-Trichotomie bleibt als Absicherung.
    auto tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    tp->measurement_tooling = {{"wallclock"}, {"macro"}, {"micro"}};
    planner::ExperimentPlanDirector const director;

    // (i) Identitaet: leerer Selektor == der explizite construct("") -- der volle 3-Combo-Walk (byte-gleich). Je Combo:
    // [wallclock] -> amd+intel (2 Build-Batches), [macro] -> nur intel (1; no_ext+macro->intel), [micro] -> amd+intel
    // (2). Summe = 5 Build-Batches.
    planner::TierCiYamlBuilder tb_default, tb_empty;
    director.construct(*tp, tb_default);
    director.construct(*tp, tb_empty, "");
    EXPECT_EQ(tb_default.text(), tb_empty.text()) << "leerer Selektor = Identitaet (voller 3-Combo-Walk)";
    std::size_t const full_jobs = count_occurrences(tb_default.text(), "# JOB tier-build-batch ");
    EXPECT_EQ(full_jobs, 5u) << "3 Combos: [wallclock]=2 + [macro]=1 (intel only) + [micro]=2 Build-Batches";

    // (ii) EIN realer Selektor "_wallclock_" -> echtes nicht-leeres Subset: NUR die wallclock-Combo (amd+intel = 2).
    planner::TierCiYamlBuilder tb_one;
    director.construct(*tp, tb_one, "_wallclock_");
    std::size_t const one_jobs = count_occurrences(tb_one.text(), "# JOB tier-build-batch ");
    EXPECT_EQ(one_jobs, 2u) << "der reale Selektor behaelt seine EINE CEB-Konfig (wallclock: amd+intel)";
    EXPECT_LT(one_jobs, full_jobs) << "echtes Subset des vollen Walks";

    // (iii) Nicht existierender Selektor -> 0 Kombinationen -> 0 Batch-Jobs (ehrlich leer, kein Crash).
    planner::TierCiYamlBuilder tb_none;
    director.construct(*tp, tb_none, "_does_not_exist_");
    EXPECT_EQ(count_occurrences(tb_none.text(), "# JOB tier-build-batch "), 0u)
        << "kein Selektor-Treffer => ehrlich leere Stufe-2 (0 Batch-Jobs)";
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
//       symmetrisch zu --emit-tier-ci (A5b). S4-§62-B-Batch (gefanntes all_axes_golden, 3 Combos): dieselbe
//       Trichotomie ueber die je-Host-Build+Pruef-Aggregat-Targets comdare_tier_batch_<host> -- (i) Identitaet, (ii)
//       echtes Subset "_wallclock_", (iii) Miss = 0 Targets. Nicht-uniform je Combo (measure_host_lane), daher
//       deterministische Ziel-Zahlen statt *3. Golden-neutral: nur die emittierten .cmake-Strings.
TEST(SelectMeasurementCombo, EmitTierCmakeSelectorTrichotomyIdentitySubsetMiss) {
    // §64: der all_axes-Default ist jetzt EINE [all]-Combo (Klasse-A-bewiesen). Dieser Test stellt die 3-Tool-XML-
    // OPTION EXPLIZIT nach -> die N>1-Selektor-Trichotomie (bare-metal-cmake) bleibt als XML-Options-Absicherung.
    auto tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    tp->measurement_tooling = {{"wallclock"}, {"macro"}, {"micro"}};
    planner::ExperimentPlanDirector const director;

    // (i) Identitaet: leerer Selektor == der explizite construct("") -- der volle 3-Combo-Walk (byte-gleich). Je Combo:
    // [wallclock]=amd+intel (2), [macro]=intel (1), [micro]=amd+intel (2) => 5 Build+Pruef-Aggregat-Targets.
    planner::TierCmakeGraphBuilder cm_default, cm_empty;
    director.construct(*tp, cm_default);
    director.construct(*tp, cm_empty, "");
    EXPECT_EQ(cm_default.text(), cm_empty.text()) << "emit-tier-cmake: leerer Selektor = Identitaet (3-Combo-Walk)";
    std::size_t const full_targets = count_occurrences(cm_default.text(), "add_custom_target(comdare_tier_batch_");
    EXPECT_EQ(full_targets, 5u) << "3 Combos: [wallclock]=2 + [macro]=1 + [micro]=2 Build+Pruef-Aggregat-Targets";

    // (ii) EIN realer Selektor "_wallclock_" -> echtes nicht-leeres Subset: NUR die wallclock-Combo (amd+intel = 2).
    planner::TierCmakeGraphBuilder cm_one;
    director.construct(*tp, cm_one, "_wallclock_");
    std::size_t const one_targets = count_occurrences(cm_one.text(), "add_custom_target(comdare_tier_batch_");
    EXPECT_EQ(one_targets, 2u) << "der reale Selektor behaelt seine EINE CEB-Konfig (wallclock: amd+intel)";
    EXPECT_LT(one_targets, full_targets) << "echtes Subset des vollen Walks";

    // (iii) Nicht existierender Selektor -> 0 Kombinationen -> 0 Batch-Targets (ehrlich leer, kein Crash).
    planner::TierCmakeGraphBuilder cm_none;
    director.construct(*tp, cm_none, "_does_not_exist_");
    EXPECT_EQ(count_occurrences(cm_none.text(), "add_custom_target(comdare_tier_batch_"), 0u)
        << "kein Selektor-Treffer => ehrlich leere Stufe-2 (0 Batch-Targets)";
}

// (S6-P1b-d) Env-Bruecke: die Tier-Bau-/Mess-Kommandos exportieren COMDARE_MEASUREMENT_COMBO=<combo-legend> ab N>1
//       (der Director kennt die Combo aus dem gefilterten Walk, combo_legend_). Default/[all] (1-Combo-Profil) =>
//       KEIN Export (byte-stabil). So reist die gewaehlte Combo bis zum Treiber (run_profile stempelt die je-Combo-DLLs).
TEST(MeasurementComboEnvBridge, TierCommandsCarryComboEnvWhenFannedAndOmitForAll) {
    // §64: der all_axes-Default ist jetzt EINE [all]-Combo (Klasse-A-bewiesen). Dieser Test stellt die 3-Tool-XML-
    // OPTION EXPLIZIT nach -> die N>1-Env-Bruecke (Combo-Export bei Fanned) bleibt als XML-Options-Absicherung; der
    // [all]-Zweig unten nutzt weiterhin planner_thesis_min (kein <measurement_tooling> => [all] => KEIN Export).
    auto tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    tp->measurement_tooling = {{"wallclock"}, {"macro"}, {"micro"}};
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

// (W2, 2026-08-05, Owner-GO mittag-6 R1) Der CT-EINBAU der Mess-Combo an den VIER CEB-Compile-Stellen der
//       Emission: ceb:build + ceb:emit (Stufe 1, CiYamlBuilder) und tier-build-batch + measure-batch (Stufe 2,
//       TierCiYamlBuilder). Das sind genau die Stellen, an denen der comdare-messung-driver=CEB kompiliert wird
//       -- dort ENTSTEHEN die Mess-Stempel real. ADDITIV zu den S6-P1b-Env-Pins darueber: der Env-Export bleibt
//       (er speist +mtool/Bestandslog, W-11-Flaeche), der -D-Zusatz kommt HINZU.
//       [all] (die gesamte heutige Live-/golden-Strecke) => KEINE Zuweisung, sondern seit F-B1 die EXPLIZITE
//       LOESCHUNG der Cache-Variablen (-U). Die frueher hier zugesagte Byte-Identitaet der [all]-Emission ist
//       damit BEWUSST aufgegeben -- Begruendung an der F-B1-Pin-Stelle unten.
TEST(MeasurementComboCtDefine, CebCompileSitesCarryDefineWhenFannedAndOmitForAll) {
    auto tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    tp->measurement_tooling = {{"wallclock"}, {"macro"}, {"micro"}};
    planner::ExperimentPlanDirector const director;

    // Stufe 1 (plan ci): ceb:build UND ceb:emit konfigurieren die CEB mit der einkompilierten Combo.
    planner::CiYamlBuilder cb;
    director.construct(*tp, cb);
    std::string const& s1 = cb.text();
    EXPECT_NE(s1.find("-DCMAKE_BUILD_TYPE=Release \"-DCOMDARE_MEASUREMENT_COMBO=[wallclock]\""), std::string::npos)
        << "ceb:build/ceb:emit tragen den CT-Define direkt hinter dem Build-Typ";
    // CI-DUAL (Owner 14.08.): DREI Stellen je Combo -- ceb:build + ceb:emit + der clang-22-Zwilling
    // des ceb:build (E2: der Zwilling traegt DIESELBEN Defines, sonst pruefte er eine ANDERE CEB).
    EXPECT_EQ(count_occurrences(s1, "\"-DCOMDARE_MEASUREMENT_COMBO=[wallclock]\""), 3u)
        << "genau drei CEB-Compile-Stellen je Combo in Stufe 1 (ceb:build + ceb:emit + clang-Zwilling)";
    EXPECT_NE(s1.find("\"-DCOMDARE_MEASUREMENT_COMBO=[macro]\""), std::string::npos);
    EXPECT_NE(s1.find("\"-DCOMDARE_MEASUREMENT_COMBO=[micro]\""), std::string::npos);

    // Stufe 2 (tier ci): der CEB-NEUBAU im Build-Batch UND im Mess-Batch traegt denselben Define.
    planner::TierCiYamlBuilder tb;
    director.construct(*tp, tb);
    std::string const& s2 = tb.text();
    EXPECT_NE(s2.find("\"-DCOMDARE_MEASUREMENT_COMBO=[wallclock]\""), std::string::npos);
    // Der Env-Export bleibt DANEBEN bestehen (W-11 unangetastet) -- beide Traeger, ein Wert.
    EXPECT_NE(s2.find("COMDARE_MEASUREMENT_COMBO=\"[wallclock]\" COMDARE_GN_OPT="), std::string::npos);

    // [all] => KEIN Define in KEINER Stufe.
    auto const tp_all = parse_thesis(COMDARE_PLANNER_THESIS_MIN);
    ASSERT_TRUE(tp_all.has_value());
    planner::CiYamlBuilder cb_all;
    director.construct(*tp_all, cb_all);
    EXPECT_EQ(cb_all.text().find("-DCOMDARE_MEASUREMENT_COMBO="), std::string::npos)
        << "[all] => kein CT-Define (Stufe 1)";
    planner::TierCiYamlBuilder tb_all;
    director.construct(*tp_all, tb_all);
    EXPECT_EQ(tb_all.text().find("-DCOMDARE_MEASUREMENT_COMBO="), std::string::npos)
        << "[all] => kein CT-Define (Stufe 2)";

    // F-B1 (Codex-Nachreview W1/W2, Ledger-Nachtrag 05.08.2026 nachmittag-7): [all] emittiert nicht mehr
    // SCHWEIGEN, sondern die EXPLIZITE LOESCHUNG der CMake-Cache-Variablen. Grund: die Cache-Variable ist
    // STICKY -- ein Build-Verzeichnis, das zuvor mit -DCOMDARE_MEASUREMENT_COMBO=<spezifisch> konfiguriert
    // wurde (und die emittierten Jobs halten `build` per gn_out-Persistenz ueber den Checkout-Clean hinweg),
    // behielte das CT-Define im [all]-Folgelauf. Damit ist die [all]-Emission BEWUSST NICHT MEHR byte-identisch
    // zur Vor-F-B1-Form -- die W1-Byte-Identitaets-Aussage galt dem Stand VOR diesem Fix. Kein Tier-Fingerprint
    // haengt daran (reine Job-Text-Flaeche).
    // CI-DUAL (Owner 14.08.): die Loeschung steht nicht mehr am Zeilenende -- der Compiler-Pin ist
    // ANGEFUEGT (E1: " -UCOMDARE_MEASUREMENT_COMBO -DCMAKE_C_COMPILER=..."); Anker entsprechend.
    EXPECT_NE(cb_all.text().find(" -UCOMDARE_MEASUREMENT_COMBO -DCMAKE_C_COMPILER="), std::string::npos)
        << "[all] => explizite Cache-Loeschung statt Schweigen (Stufe 1)";
    EXPECT_EQ(count_occurrences(cb_all.text(), "-UCOMDARE_MEASUREMENT_COMBO"), 3u)
        << "genau drei CEB-Compile-Stellen in Stufe 1 (ceb:build + ceb:emit + clang-Zwilling) tragen die Loeschung";
    EXPECT_NE(tb_all.text().find(" -UCOMDARE_MEASUREMENT_COMBO -DCMAKE_C_COMPILER="), std::string::npos)
        << "[all] => explizite Cache-Loeschung statt Schweigen (Stufe 2)";
    EXPECT_GE(count_occurrences(tb_all.text(), "-UCOMDARE_MEASUREMENT_COMBO"), 2u)
        << "Stufe 2: Build-Batch UND Mess-Batch je Lane tragen die Loeschung";
    // Die gefannte Strecke traegt die ZUWEISUNG, nie die Loeschung (die beiden Formen schliessen sich aus).
    EXPECT_EQ(s1.find("-UCOMDARE_MEASUREMENT_COMBO"), std::string::npos)
        << "spezifische Combo => -D-Zuweisung, KEIN -U (Stufe 1)";
    EXPECT_EQ(s2.find("-UCOMDARE_MEASUREMENT_COMBO"), std::string::npos)
        << "spezifische Combo => -D-Zuweisung, KEIN -U (Stufe 2)";
    // Bare-metal-Gegenpart (plan cmake): der aeussere Configure baut den Treiber -> nur ein Hinweis-Echo,
    // und auch das NUR ausserhalb von [all].
    planner::CMakeGraphBuilder gm;
    director.construct(*tp, gm);
    EXPECT_NE(gm.text().find("aeusserer Configure braucht -DCOMDARE_MEASUREMENT_COMBO=[wallclock]"), std::string::npos);
    planner::CMakeGraphBuilder gm_all;
    director.construct(*tp_all, gm_all);
    EXPECT_EQ(gm_all.text().find("COMDARE_MEASUREMENT_COMBO"), std::string::npos)
        << "[all] => kein Hinweis-Echo (Stufe-1-cmake byte-identisch)";
}

// =============================================================================
// M-2 / B2 (P-PMC-1, 2026-08-06) -- DIE PMC-INVARIANTE. Der KERN dieses Pakets.
//
// WURZEL, und deshalb genau DIESE Form: die PMC-Pflicht (F9, Owner 2026-07-16 "PFLICHT fuer die
// Vollstaendigkeit aller perf-Messwerte") wurde am 16.07. an ZWEI JOB-NAMEN geheftet (measure:smoke +
// measure:golden-320). Als die Mess-Arbeit in den Planer wanderte, wanderte die Pflicht nicht mit --
// niemand merkte es, weil keine Wache die SACHE prueft. Ein Test, der stattdessen die Zahl "4" hart
// verdrahtet, wiederholt denselben Fehler eine Ebene hoeher: er ist gruen, sobald jemand eine FUENFTE
// Emissionsstelle baut, und er ist rot aus dem falschen Grund, sobald eine Stelle legitim entfaellt.
//
// DIE INVARIANTE, die hier gepinnt wird -- eine Vollstaendigkeits-Bedingung, keine Zahl:
//
//     Zu JEDER emittierten `cmake -B build`-Zeile, deren FOLGEZEILE den comdare-messung-driver baut,
//     gehoert -DCOMDARE_ENABLE_PMC=ON.
//
// Der Selektor ist die EIGENSCHAFT "dieser Configure traegt einen Mess-Treiber-Bau", nicht der Job-Name
// und nicht die Anzahl. Eine Emission OHNE Treiber-Bau bleibt bewusst flaglos (super .gitlab-ci.yml haelt
// einen Auswertungs-Job ausdruecklich ohne COMDARE_ENABLE_PMC) -- die Invariante sagt darueber nichts und
// soll das auch nicht.
//
// GESCHLOSSEN GEGEN DAS LEERLAUFEN (Regel 6 in Testform): geprueft wird nicht nur die Implikation, sondern
// die DECKUNG. Es wird von den TREIBER-BAU-Zeilen aus gezaehlt (das ist der Nenner) und verlangt, dass jede
// von ihnen unmittelbar hinter einem geflaggten Configure steht. Damit faellt der Test auch dann rot, wenn
// ein Umbau die Nachbarschaft aufbricht (Zeile dazwischen) oder die Emission umbenennt -- er kann nicht
// still gruen leer laufen. Eine nackte 0 ist kein Befund: der Nenner wird mit ausgegeben.
//
// GEGENPROBE: der Kern-Pruefer wird unten zuerst gegen einen HANDGEBAUTEN Text gefahren, in dem genau eine
// Stelle das Flag NICHT traegt -- findet er dort nichts, taugt das Verfahren nicht und der Test sagt es.
// =============================================================================
namespace {

std::vector<std::string> split_lines(std::string const& text) {
    std::vector<std::string> lines;
    std::size_t              start = 0;
    while (start <= text.size()) {
        std::size_t const nl = text.find('\n', start);
        if (nl == std::string::npos) {
            if (start < text.size()) lines.push_back(text.substr(start));
            break;
        }
        lines.push_back(text.substr(start, nl - start));
        start = nl + 1;
    }
    return lines;
}

// Der Befund einer Emission. Alle Zahlen beziehen sich auf DENSELBEN Nenner (driver_builds).
// I-PMC-2 (10.08.2026): `vendor_flagged` ist neu -- seit der Erkennung ist "ON" allein keine vollstaendige
// Aussage mehr. Eine Konfiguration mit ON aber OHNE Vendor baut die Messfuehler ein, ohne dass der
// CEB-Selbst-Stempel die Hardwareform nennt: die Zellen dieser CEB sind gegen vendor-gestempelte nicht
// zuordenbar. Der Zaehler macht genau diesen Zwischenzustand sichtbar, statt ihn unter "flagged" zu
// verstecken.
struct PmcInvariantReport {
    std::size_t              driver_builds  = 0; // NENNER: Zeilen, die den Mess-Treiber bauen
    std::size_t              configured     = 0; // davon: unmittelbar hinter einer `cmake -B build`-Zeile
    std::size_t              flagged        = 0; // davon: deren Configure -DCOMDARE_ENABLE_PMC=ON traegt
    std::size_t              vendor_flagged = 0; // davon: deren Configure ZUSAETZLICH einen Vendor nennt
    std::vector<std::string> violations;         // STRUKTURELLE Brueche (Nachbarschaft), woertlich
};

// Der EINE Pruefer. Ausgangspunkt ist die Treiber-Bau-Zeile (der Nenner), nicht die Configure-Zeile --
// so kann keine Stelle dadurch verschwinden, dass ihr Configure umformuliert wird.
PmcInvariantReport pmc_invariant(std::string const& emitted) {
    static constexpr char const* kDriverBuild = "cmake --build build --target comdare-messung-driver";
    static constexpr char const* kConfigure   = "cmake -B build";
    static constexpr char const* kPmcFlag     = "-DCOMDARE_ENABLE_PMC=ON";
    static constexpr char const* kVendorFlag  = "-DCOMDARE_PMC_VENDOR=";

    PmcInvariantReport             rep;
    std::vector<std::string> const lines = split_lines(emitted);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].find(kDriverBuild) == std::string::npos) continue;
        ++rep.driver_builds;
        if (i == 0 || lines[i - 1].find(kConfigure) == std::string::npos) {
            rep.violations.push_back("kein `cmake -B build` unmittelbar VOR dem Treiber-Bau: " + lines[i]);
            continue;
        }
        ++rep.configured;
        // I-PMC-2: das FEHLENDE Flag ist seit dem 10.08. KEIN struktureller Bruch mehr -- auf einem Host
        // ohne benutzbare PMU ist "ohne PMC" die ehrliche Permutation. Ob die Zahl stimmt, entscheidet der
        // Aufrufer GEGEN DEN BEFUND; der Pruefer zaehlt nur noch.
        if (lines[i - 1].find(kPmcFlag) == std::string::npos) continue;
        ++rep.flagged;
        if (lines[i - 1].find(kVendorFlag) != std::string::npos) ++rep.vendor_flagged;
    }
    return rep;
}

// Erzwungene Befunde -- die drei Lagen, auf JEDER Maschine herstellbar. Genau das ist der Punkt der
// Strategy-Form der Probe: die intel-Seite ist auf prod1 (AuthenticAMD) real nicht erzeugbar, als Wert
// aber sehr wohl -- und die Emission haengt am WERT, nicht an der Hardware.
planner::PmcHostBefund befund_mit(planner::PmcLage lage, char const* vendor) {
    planner::PmcHostBefund b;
    b.lage            = lage;
    b.probe_gefahren  = true;
    b.cpuid_vendor    = vendor;
    b.events_geprueft = 4;
    b.events_gebissen = lage == planner::PmcLage::Unbrauchbar ? 0u : 3u;
    b.biss_vektor     = lage == planner::PmcLage::Unbrauchbar
                            ? "cache_misses_l1=0;cache_misses_l3_ll=0;dtlb_misses=0;branch_misses=0"
                            : "cache_misses_l1=1;cache_misses_l3_ll=0;dtlb_misses=1;branch_misses=1";
    if (lage == planner::PmcLage::Unbrauchbar) b.fehlgrund = "koeder_hat_nicht_gebissen";
    return b;
}

// Die EINE Abnahme je Emission. `wo` benennt Kanal + Builder, damit ein roter Lauf sofort sagt, WELCHE
// der Emissionen gebrochen ist.
//
// I-PMC-2 (Owner 10.08.2026): die Abnahme prueft ab jetzt die UEBEREINSTIMMUNG MIT DEM BEFUND, nicht mehr
// ein unbedingtes ON. F9 gilt darin unveraendert weiter -- fuer jeden Host mit benutzbarer PMU ist die
// Deckung wieder 100%. Neu ist nur, dass die Wache auch die GEGENRICHTUNG haelt: ein Host ohne PMU darf
// KEIN Flag bekommen. Ohne diese zweite Haelfte waere die Erkennung gebaut und unbewacht -- man koennte
// sie entfernen, und der Test bliebe gruen.
void expect_pmc_invariant(std::string const& emitted, char const* wo, planner::PmcHostBefund const& befund) {
    PmcInvariantReport const rep = pmc_invariant(emitted);
    std::string              verstoesse;
    for (auto const& v : rep.violations) verstoesse += "\n    - " + v;
    bool const        erwartet_pmc = befund.lage != planner::PmcLage::Unbrauchbar;
    std::size_t const soll         = erwartet_pmc ? rep.driver_builds : 0u;

    EXPECT_GT(rep.driver_builds, 0u)
        << wo << ": Wache leer gelaufen -- kein `" << "cmake --build build --target comdare-messung-driver"
        << "` in der Emission gefunden. Entweder die Emission wurde umbenannt (dann ist DIESER Test "
           "nachzuziehen) oder sie ist verschwunden (dann ist die Kette gebrochen).";
    EXPECT_EQ(rep.configured, rep.driver_builds)
        << wo << ": " << rep.driver_builds << " Treiber-Bau-Zeilen geprueft, davon " << rep.configured
        << " mit unmittelbar vorangehendem `cmake -B build`." << verstoesse;
    EXPECT_EQ(rep.flagged, soll)
        << wo << " [befund=" << befund.lage_label() << "]: " << rep.driver_builds
        << " Treiber-Bau-Zeilen geprueft, davon " << rep.flagged << " mit -DCOMDARE_ENABLE_PMC=ON (SOLL " << soll
        << "). Bei brauchbarer PMU gilt F9 unveraendert (Deckung 100%); bei unbrauchbarer MUSS die Emission "
           "flaglos bleiben -- ein ON auf einem Host ohne PMU baut den vollen Messfuehler-Overhead ein und "
           "misst nichts (pmc_available=0), und genau diese CEB ist gegen eine echte nicht vergleichbar."
        << verstoesse;
    EXPECT_EQ(rep.vendor_flagged, soll)
        << wo << " [befund=" << befund.lage_label() << "]: " << rep.flagged << " geflaggte Configure-Zeilen, davon "
        << rep.vendor_flagged << " mit -DCOMDARE_PMC_VENDOR= (SOLL " << soll
        << "). Owner 10.08.2026: 'JEDE HARDWAREFORM EINER PMC AMD/INTEL IST ZU UNTERSCHEIDEN, ES SIND 2 "
           "VERSCHIEDENE HARDWARE KOMPONENTEN NICHT EIN PMC.' Ein ON ohne Vendor nennt die Komponente nicht.";
    if (erwartet_pmc) {
        std::string const erwartet = std::string{"-DCOMDARE_PMC_VENDOR="} + std::string{befund.vendor_id()};
        EXPECT_NE(emitted.find(erwartet), std::string::npos)
            << wo << ": die Emission nennt nicht die GEMESSENE Hardwareform (" << erwartet << ").";
    }
}

} // namespace

// (M-2/B2) Die Invariante ueber ALLE Emissionen, die einen Mess-Treiber bauen -- BEIDE Builder (Stufe 1
//       CiYamlBuilder = ceb:build + ceb:emit; Stufe 2 TierCiYamlBuilder = tier-build-batch + measure-batch)
//       und BEIDE Kanaele (Thesis + Experiment). Ein Test, der nur EINEN Builder konstruiert, saehe die
//       beiden teuersten Stellen (die 7d-Batches der Stufe 2) NICHT.
TEST(PmcPflichtInvariante, JedeTreiberKonfigurationTraegtDasPmcFlag) {
    // (0) GEGENPROBE ZUERST: findet das Verfahren ueberhaupt? Handgebauter Text, DREI Sorten Zeile.
    //     Ohne diesen Schritt waere ein spaeteres "0 Verstoesse" nicht unterscheidbar von "der Pruefer
    //     sucht am falschen Ort". Seit I-PMC-2 traegt der Kunsttext auch die ON-OHNE-VENDOR-Zeile: sie
    //     MUSS als geflaggt, aber NICHT als vendor_flagged gezaehlt werden -- genau dieser Zwischenzustand
    //     ist der heutige Super-Altaufrufer, und ein Pruefer, der ihn nicht sieht, uebersieht ihn spaeter
    //     auch in der echten Emission.
    std::string const kunstlich =
        std::string{} +
        "    - cmake -B build -G Ninja -DCOMDARE_ENABLE_PMC=ON -DCOMDARE_PMC_VENDOR=amd -DCMAKE_BUILD_TYPE=Release\n"
        "    - cmake --build build --target comdare-messung-driver\n"
        "    - cmake -B build -G Ninja -DCOMDARE_ENABLE_PMC=ON -DCMAKE_BUILD_TYPE=Release\n"
        "    - cmake --build build --target comdare-messung-driver\n"
        "    - cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release\n"
        "    - cmake --build build --target comdare-messung-driver\n"
        "    - cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release\n"
        "    - cmake --build build --target comdare_experiment_planner\n";
    PmcInvariantReport const probe = pmc_invariant(kunstlich);
    ASSERT_EQ(probe.driver_builds, 3u) << "Gegenprobe: der Pruefer muss GENAU die 3 Treiber-Bau-Zeilen sehen "
                                          "(die vierte Konfiguration baut ein anderes Target)";
    ASSERT_EQ(probe.configured, 3u) << "Gegenprobe: alle drei stehen unmittelbar hinter einer Konfiguration";
    ASSERT_EQ(probe.flagged, 2u) << "Gegenprobe: GENAU zwei der drei tragen -DCOMDARE_ENABLE_PMC=ON";
    ASSERT_EQ(probe.vendor_flagged, 1u) << "Gegenprobe: der Pruefer BEISST am Zwischenzustand -- nur EINE der "
                                           "beiden geflaggten nennt auch die Hardwareform";

    // (1) Thesis-Kanal, all_axes_golden (die Live-/golden-Strecke, Combo [all]) -- mit einem BRAUCHBAREN
    //     amd-Befund. F9 gilt hier unveraendert: Deckung 100%.
    planner::PmcHostBefund const    amd = befund_mit(planner::PmcLage::Amd, "AuthenticAMD");
    planner::ExperimentPlanDirector director;
    director.set_pmc_befund(amd);

    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::CiYamlBuilder cb;
    director.construct(*tp, cb);
    expect_pmc_invariant(cb.text(), "Thesis/Verbund1 CiYamlBuilder (ceb:build + ceb:emit)", amd);
    planner::TierCiYamlBuilder tb;
    director.construct(*tp, tb);
    expect_pmc_invariant(tb.text(), "Thesis/Verbund2 TierCiYamlBuilder (tier-build-batch + measure-batch)", amd);

    // (2) Derselbe Thesis-Kanal GEFANNT (drei Mess-Combos): der Fanout vervielfacht die Stufe-1-Stellen.
    //     Die Invariante darf davon nicht abhaengen -- genau das ist ihr Punkt gegenueber einer festen Zahl.
    auto tp_fan = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp_fan.has_value());
    tp_fan->measurement_tooling = {{"wallclock"}, {"macro"}, {"micro"}};
    planner::CiYamlBuilder cb_fan;
    director.construct(*tp_fan, cb_fan);
    expect_pmc_invariant(cb_fan.text(), "Thesis/Verbund1 GEFANNT (3 Combos)", amd);
    planner::TierCiYamlBuilder tb_fan;
    director.construct(*tp_fan, tb_fan);
    expect_pmc_invariant(tb_fan.text(), "Thesis/Verbund2 GEFANNT (3 Combos)", amd);

    // (3) Minimal-Profil: die kleinste Emission ueberhaupt. Auch sie baut den Treiber.
    auto const tp_min = parse_thesis(COMDARE_PLANNER_THESIS_MIN);
    ASSERT_TRUE(tp_min.has_value());
    planner::CiYamlBuilder cb_min;
    director.construct(*tp_min, cb_min);
    expect_pmc_invariant(cb_min.text(), "Thesis-min/Verbund1", amd);
    planner::TierCiYamlBuilder tb_min;
    director.construct(*tp_min, tb_min);
    expect_pmc_invariant(tb_min.text(), "Thesis-min/Verbund2", amd);

    // (4) Experiment-Kanal (eigene Schrittzahl, eigener Zwilling -- im Haus schon einmal als "Fix fehlt im
    //     Experiment-Zwilling" aufgefallen).
    auto const ep = parse_experiment(COMDARE_EXPERIMENT_GOLDEN);
    ASSERT_TRUE(ep.has_value()) << "experiment_golden.xml nicht parsbar: " << COMDARE_EXPERIMENT_GOLDEN;
    planner::CiYamlBuilder cb_exp;
    director.construct(*ep, cb_exp);
    expect_pmc_invariant(cb_exp.text(), "Experiment/Verbund1", amd);
    planner::TierCiYamlBuilder tb_exp;
    director.construct(*ep, tb_exp);
    expect_pmc_invariant(tb_exp.text(), "Experiment/Verbund2", amd);

    // (5) DIE GEGENRICHTUNG, die es vor dem 10.08. nicht gab: ein Host OHNE benutzbare PMU bekommt KEIN
    //     Flag -- in ALLEN vier Emissionen. Ohne diese Haelfte koennte man die Erkennung wieder ausbauen
    //     (unbedingtes ON) und der Test bliebe gruen.
    planner::PmcHostBefund const    ohne = befund_mit(planner::PmcLage::Unbrauchbar, "SomeOtherVendor");
    planner::ExperimentPlanDirector director_ohne;
    director_ohne.set_pmc_befund(ohne);
    planner::CiYamlBuilder cb_ohne;
    director_ohne.construct(*tp, cb_ohne);
    expect_pmc_invariant(cb_ohne.text(), "Thesis/Verbund1 OHNE PMU", ohne);
    planner::TierCiYamlBuilder tb_ohne;
    director_ohne.construct(*tp, tb_ohne);
    expect_pmc_invariant(tb_ohne.text(), "Thesis/Verbund2 OHNE PMU", ohne);
}

// (M-2/B2, Zusatz) Die Invariante haengt an der SACHE, nicht an der Konfigurations-Nachbarschaft: der
//       PMC-Zusatz sitzt VOR -DCMAKE_BUILD_TYPE und laesst damit die W2-Nachbarschaft (BUILD_TYPE direkt
//       gefolgt vom Combo-Define) unberuehrt. Das ist keine Kosmetik -- die W2-Pins oben lesen genau diese
//       Nachbarschaft und wuerden bei einer Einschiebung dazwischen still ihre Aussage verlieren.
//       I-PMC-2: der Vendor haengt sich HINTER das ON und bleibt damit innerhalb desselben Blocks; die
//       Nachbarschaft BUILD_TYPE/Combo ist unveraendert.
TEST(PmcPflichtInvariante, FlagStehtVorDemBuildTypUndLaesstDieComboNachbarschaftIntakt) {
    auto tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    tp->measurement_tooling = {{"wallclock"}};
    planner::ExperimentPlanDirector director;
    director.set_pmc_befund(befund_mit(planner::PmcLage::Amd, "AuthenticAMD"));
    planner::CiYamlBuilder cb;
    director.construct(*tp, cb);
    std::string const& s1 = cb.text();

    EXPECT_NE(s1.find("-DCOMDARE_V32_ENABLE=ON -DCOMDARE_ENABLE_PMC=ON -DCOMDARE_PMC_VENDOR=amd "
                      "-DCMAKE_BUILD_TYPE=Release"),
              std::string::npos)
        << "PMC-Zusatz steht zwischen V32-Schalter und Build-Typ (Reihenfolge der beiden super-Mess-Jobs)";
    EXPECT_NE(s1.find("-DCMAKE_BUILD_TYPE=Release \"-DCOMDARE_MEASUREMENT_COMBO=[wallclock]\""), std::string::npos)
        << "die W2-Nachbarschaft (Build-Typ direkt gefolgt vom Combo-Define) bleibt unangetastet";
}

// =============================================================================
// PMC ALS META-META-MESS-ACHSE (Owner 10.08.2026) -- DIE ERKENNUNG, DIE HEUTE FEHLT.
// =============================================================================
// OWNER-WORTLAUT (10.08.2026, verbatim): "Dabei erkennt jeder Planer auf jeder Maschine fuer sich, ob PMC
// existiert und ob daher das PMC in der CEB verbaut wird."
// OWNER-PRAEZISIERUNG (10.08.2026): "Moment mal JEDER Planer? Es gibt ja nur einen Planer" -- am Objekt
// gezaehlt gibt es GENAU EINEN (apps/experiment_planner, ein add_executable). "Jeder Planer auf jeder
// Maschine" meint also jeden LAUF dieses EINEN Programms auf einem Host: DIESELBE Binary muss auf prod1
// und prod2 zu VERSCHIEDENEN Ergebnissen kommen.
//
// DER DEFEKT IN EINEM SATZ: ceb_pmc_compile_define() nahm KEIN Argument und lieferte unbedingt
// " -DCOMDARE_ENABLE_PMC=ON" -- der Planer ERKANNTE also nichts, er BEHAUPTETE. Auf einem Host ohne
// benutzbare PMU emittierte er exakt dieselben Bytes wie auf prod1, und die dort gebaute CEB meldete
// pmc_available=0 bei vollem Messfuehler-Overhead. Zwei nicht vergleichbare Ergebnisse unter einem Namen.
//
// DIESE WACHE PRUEFT DIE AUSSAGE, NICHT DIE ANWESENHEIT (T-2): sie zaehlt von den TREIBER-BAU-Zeilen aus
// (derselbe Nenner wie die F9-Invariante darueber) und verlangt, dass JEDE von ihnen eine PMC-BEFUND-Zeile
// traegt, die die gemessene Lage NENNT -- auch und gerade im Fall "unbrauchbar" (V-1: der Nenner gehoert in
// die Ausgabe; eine Emission, die zu PMC schweigt, ist von einer ungeprueften nicht unterscheidbar).
namespace {

// Der Befund der Befund-Annotation. Beide Zahlen beziehen sich auf DENSELBEN Nenner (driver_builds).
struct PmcBefundReport {
    std::size_t              driver_builds = 0; // NENNER: Zeilen, die den Mess-Treiber bauen
    std::size_t              annotiert     = 0; // davon: mit PMC-BEFUND-Zeile im Block davor
    std::vector<std::string> fehlstellen;       // die unannotierten Treiber-Bau-Zeilen, woertlich
};

// Der EINE Pruefer. Ausgangspunkt ist wieder die Treiber-Bau-Zeile (der Nenner). Die Befund-Zeile wird im
// FENSTER der drei Zeilen davor gesucht -- sie steht als YAML-Kommentar unmittelbar vor dem Configure,
// darf aber nicht daran kleben (sonst braeche jede Umformulierung des Configure die Wache aus dem
// falschen Grund).
PmcBefundReport pmc_befund_annotation(std::string const& emitted) {
    static constexpr char const* kDriverBuild = "cmake --build build --target comdare-messung-driver";
    static constexpr char const* kBefund      = "PMC-BEFUND";

    PmcBefundReport                rep;
    std::vector<std::string> const lines = split_lines(emitted);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].find(kDriverBuild) == std::string::npos) continue;
        ++rep.driver_builds;
        bool              gefunden = false;
        std::size_t const von      = i >= 3 ? i - 3 : 0;
        for (std::size_t j = von; j < i; ++j)
            if (lines[j].find(kBefund) != std::string::npos) gefunden = true;
        if (gefunden)
            ++rep.annotiert;
        else
            rep.fehlstellen.push_back("Treiber-Bau ohne PMC-BEFUND-Zeile: " + lines[i]);
    }
    return rep;
}

void expect_pmc_befund_annotation(std::string const& emitted, char const* wo) {
    PmcBefundReport const rep = pmc_befund_annotation(emitted);
    std::string           fehl;
    for (auto const& f : rep.fehlstellen) fehl += "\n    - " + f;
    EXPECT_GT(rep.driver_builds, 0u) << wo << ": Wache leer gelaufen -- kein Treiber-Bau in der Emission.";
    EXPECT_EQ(rep.annotiert, rep.driver_builds)
        << wo << ": " << rep.driver_builds << " Treiber-Bau-Zeilen geprueft, davon " << rep.annotiert
        << " mit einer PMC-BEFUND-Zeile. Ohne sie behauptet die Emission ihre PMC-Lage, statt sie zu "
           "nennen -- der Leser der Kind-Pipeline kann nicht unterscheiden, ob der Planer gemessen hat "
           "oder ob er geraten hat (Owner 10.08.2026: 'erkennt ... fuer sich')."
        << fehl;
}

} // namespace

// GEGENPROBE ZUERST (Regel 6 in Testform): findet das Verfahren ueberhaupt? Ohne diesen Schritt waere ein
// spaeteres "0 Fehlstellen" nicht von "der Pruefer sucht am falschen Ort" zu unterscheiden.
TEST(PmcMetaMetaAchse, DerBefundPrueferBeisstAmKunsttext) {
    std::string const     kunstlich = std::string{} + "    # PMC-BEFUND lage=amd quelle=perf_event_open+cpuid\n"
                                                      "    - cmake -B build -G Ninja -DCOMDARE_ENABLE_PMC=ON\n"
                                                      "    - cmake --build build --target comdare-messung-driver\n"
                                                      "    - cmake -B build -G Ninja\n"
                                                      "    - cmake --build build --target comdare-messung-driver\n";
    PmcBefundReport const probe     = pmc_befund_annotation(kunstlich);
    ASSERT_EQ(probe.driver_builds, 2u) << "Gegenprobe: der Pruefer muss GENAU die 2 Treiber-Bau-Zeilen sehen";
    ASSERT_EQ(probe.annotiert, 1u) << "Gegenprobe: GENAU eine der beiden traegt die Befund-Zeile";
    ASSERT_EQ(probe.fehlstellen.size(), 1u) << "Gegenprobe: der Pruefer BEISST -- er meldet die stumme Stelle";
}

TEST(PmcMetaMetaAchse, JedeTreiberEmissionNenntDenGemessenenHostBefund) {
    planner::ExperimentPlanDirector const director;

    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::CiYamlBuilder cb;
    director.construct(*tp, cb);
    expect_pmc_befund_annotation(cb.text(), "Thesis/Verbund1 CiYamlBuilder (ceb:build + ceb:emit)");
    planner::TierCiYamlBuilder tb;
    director.construct(*tp, tb);
    expect_pmc_befund_annotation(tb.text(), "Thesis/Verbund2 TierCiYamlBuilder (tier-build-batch + measure-batch)");

    auto const ep = parse_experiment(COMDARE_EXPERIMENT_GOLDEN);
    ASSERT_TRUE(ep.has_value());
    planner::CiYamlBuilder cb_exp;
    director.construct(*ep, cb_exp);
    expect_pmc_befund_annotation(cb_exp.text(), "Experiment/Verbund1");
    planner::TierCiYamlBuilder tb_exp;
    director.construct(*ep, tb_exp);
    expect_pmc_befund_annotation(tb_exp.text(), "Experiment/Verbund2");
}

// ================================================================================================
// DER DEFEKT IN EINEM SATZ -- und sein Gegenbeweis.
// ================================================================================================
// VOR DEM 10.08.2026 emittierte DIESELBE Planer-Binary auf einem Host MIT PMU und auf einem Host OHNE
// PMU BYTE-GLEICH dasselbe: ceb_pmc_compile_define() nahm kein Argument. Die beiden entstehenden CEBs
// unterschieden sich im Messfuehler-Overhead, hiessen aber gleich -- Owner 10.08.2026: "was die
// Ergebnisse wiederum NICHT VERGLEICHBAR macht".
// DIESER TEST IST DER GEGENBEWEIS: drei Lagen, drei paarweise VERSCHIEDENE Emissionen. Er ist die
// Wache, die verhindert, dass die Erkennung wieder still zu einer Behauptung zurueckgebaut wird.
TEST(PmcMetaMetaAchse, DreiBefundeErzeugenDreiVerschiedeneEmissionen) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    auto emission = [&tp](planner::PmcHostBefund const& b) {
        planner::ExperimentPlanDirector d;
        d.set_pmc_befund(b);
        planner::CiYamlBuilder cb;
        d.construct(*tp, cb);
        return cb.text();
    };
    std::string const amd   = emission(befund_mit(planner::PmcLage::Amd, "AuthenticAMD"));
    std::string const intel = emission(befund_mit(planner::PmcLage::Intel, "GenuineIntel"));
    std::string const ohne  = emission(befund_mit(planner::PmcLage::Unbrauchbar, "SomeOtherVendor"));

    // PAARWEISE verschieden -- nicht nur "irgendwie anders". Zwei gleiche Emissionen aus zwei Lagen
    // waeren exakt der Alt-Zustand, nur an einer anderen Stelle.
    EXPECT_NE(amd, intel) << "amd und intel emittieren gleich -- die ZWEI Hardware-Komponenten sind zu "
                             "einer kollabiert (Owner 10.08.2026: 'ES SIND 2 VERSCHIEDENE HARDWARE "
                             "KOMPONENTEN NICHT EIN PMC').";
    EXPECT_NE(amd, ohne) << "PMU-Host und PMU-loser Host emittieren gleich -- DAS ist der Defekt, gegen den "
                            "diese Achse gebaut ist.";
    EXPECT_NE(intel, ohne) << "PMU-Host und PMU-loser Host emittieren gleich (intel-Seite).";

    // Und die Aussage, nicht nur die Ungleichheit: WAS steht drin.
    EXPECT_NE(amd.find("-DCOMDARE_PMC_VENDOR=amd"), std::string::npos);
    EXPECT_EQ(amd.find("-DCOMDARE_PMC_VENDOR=intel"), std::string::npos);
    EXPECT_NE(intel.find("-DCOMDARE_PMC_VENDOR=intel"), std::string::npos);
    EXPECT_EQ(intel.find("-DCOMDARE_PMC_VENDOR=amd"), std::string::npos);
    EXPECT_EQ(ohne.find("-DCOMDARE_ENABLE_PMC=ON"), std::string::npos) << "der PMU-lose Host darf KEIN ON emittieren";
    // ... und er schweigt trotzdem nicht (T-2: Aussage statt Abwesenheit).
    EXPECT_NE(ohne.find("PMC-BEFUND lage=unbrauchbar"), std::string::npos)
        << "der PMU-lose Host muss seine Lage NENNEN -- sonst ist er von einem ungeprueften nicht zu "
           "unterscheiden";
}

// --dump-plan: dieselbe Sache eine Ebene hoeher. Der Plan-Text ist die Form, in der ein Mensch den Plan
// liest; wenn DORT die Lage fehlt, ist die Erkennung fuer den Leser nicht existent.
TEST(PmcMetaMetaAchse, DumpPlanNenntDieLageUndUnterscheidetSichGenauDarin) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_MIN);
    ASSERT_TRUE(tp.has_value());
    auto plan = [&tp](planner::PmcHostBefund const& b) {
        planner::ExperimentPlanDirector d;
        d.set_pmc_befund(b);
        planner::PlanTextBuilder pb;
        d.construct(*tp, pb);
        return pb.text();
    };
    std::string const amd  = plan(befund_mit(planner::PmcLage::Amd, "AuthenticAMD"));
    std::string const ohne = plan(befund_mit(planner::PmcLage::Unbrauchbar, "SomeOtherVendor"));
    EXPECT_NE(amd.find("pmc_befund=amd events=3/4 erhoben=1\n"), std::string::npos) << "amd-Plan:\n" << amd;
    EXPECT_NE(ohne.find("pmc_befund=unbrauchbar events=0/4 erhoben=1\n"), std::string::npos) << "ohne-Plan:\n" << ohne;

    // GENAU DARIN unterschiedlich: alle uebrigen Zeilen bleiben host-unabhaengig (Format-Zusage des
    // PlanTextBuilder-Kopfs). Der Nenner steht in der Diagnose -- eine nackte "1" waere kein Befund.
    std::vector<std::string> const la = split_lines(amd);
    std::vector<std::string> const lo = split_lines(ohne);
    ASSERT_EQ(la.size(), lo.size()) << "Zeilenzahl unterscheidet sich -- die Aenderung ist nicht EINE Zeile";
    std::size_t              abweichungen = 0;
    std::vector<std::string> welche;
    for (std::size_t i = 0; i < la.size(); ++i)
        if (la[i] != lo[i]) {
            ++abweichungen;
            welche.push_back(la[i] + "   |   " + lo[i]);
        }
    std::string sicht;
    for (auto const& w : welche) sicht += "\n    - " + w;
    EXPECT_EQ(abweichungen, 1u) << la.size() << " Plan-Zeilen verglichen, " << abweichungen
                                << " abweichend (SOLL 1: nur pmc_befund=)." << sicht;
}

// ================================================================================================
// DIE HOST-PROBE SELBST (I-PMC-2) -- vier Fake-Lagen + eine REALE.
// ================================================================================================
// Die Syscall-Schicht ist ein statischer Strategy-Parameter. Genau deshalb sind alle Lagen auf JEDER
// Maschine pruefbar -- auch die, die hier physisch nicht herstellbar ist (intel auf prod1/AuthenticAMD).
namespace {

// (a) Der Zaehler oeffnet gar nicht -- Rechte, Container, paranoid=3.
struct FakeOeffnetNicht {
    static std::string cpuid_vendor() { return "AuthenticAMD"; }
    static bool event_beisst(::comdare::cache_engine::measurement::PmcEventSpec const&) noexcept { return false; }
};
// (b) Der Zaehler oeffnet, liefert aber nichts (t_running==0 / Multiplexing / Kern-Migration).
//     FUER DEN TEST IST DAS DASSELBE SIGNAL WIE (a): der Koeder hat nicht gebissen. Die Strategie
//     unterscheidet die beiden Ursachen bewusst NICHT -- sie sind fuer die Frage "kann diese Maschine
//     PMC messen" dieselbe Antwort, und eine Unterscheidung waere eine Behauptung ueber die Ursache.
using FakeOeffnetOhneWert = FakeOeffnetNicht;
// (c) Alles beisst, Vendor Intel.
struct FakeIntelBeisst {
    static std::string cpuid_vendor() { return "GenuineIntel"; }
    static bool        event_beisst(::comdare::cache_engine::measurement::PmcEventSpec const&) noexcept { return true; }
};
// (d) Alles beisst -- aber die Hardwareform ist unbekannt.
struct FakeFremdeHardware {
    static std::string cpuid_vendor() { return "SomeRiscVThing"; }
    static bool        event_beisst(::comdare::cache_engine::measurement::PmcEventSpec const&) noexcept { return true; }
};
// Teilverfuegbarkeit: NUR der L1D-Zaehler beisst. Das ist die reale prod1-Lage (Zen 5 kennt den
// generischen Last-Level-Zaehler nicht) und MUSS brauchbar sein -- ein UND-Kriterium erklaerte hier
// ausgerechnet die Maschine fuer PMC-los, auf der PMC beweisbar laeuft.
struct FakeNurL1dBeisst {
    static std::string cpuid_vendor() { return "AuthenticAMD"; }
    static bool        event_beisst(::comdare::cache_engine::measurement::PmcEventSpec const& ev) noexcept {
        return ev.name == std::string_view{"cache_misses_l1"};
    }
};

// FREMDER NENNER (T-3): der Vendor kommt fuer die Real-Probe NICHT aus derselben cpuid-Quelle, die die
// Probe benutzt, sondern aus /proc/cpuinfo -- vom Test selbst gelesen. Sonst prueft die Probe sich selbst.
std::string vendor_aus_proc_cpuinfo() {
    std::ifstream f{"/proc/cpuinfo"};
    std::string   zeile;
    while (std::getline(f, zeile)) {
        if (zeile.rfind("vendor_id", 0) != 0) continue;
        std::size_t const dp = zeile.find(':');
        if (dp == std::string::npos) return {};
        std::string v = zeile.substr(dp + 1);
        while (!v.empty() && (v.front() == ' ' || v.front() == '\t')) v.erase(v.begin());
        while (!v.empty() && (v.back() == ' ' || v.back() == '\t' || v.back() == '\r')) v.pop_back();
        return v;
    }
    return {};
}

} // namespace

TEST(PmcHostProbe, KeinBissIstUnbrauchbarUndNenntDenGrund) {
    auto const b = planner::probe_pmc_host<FakeOeffnetNicht>();
    EXPECT_EQ(b.lage, planner::PmcLage::Unbrauchbar);
    EXPECT_TRUE(b.probe_gefahren) << "die Probe LIEF -- das ist etwas anderes als 'nicht erhoben'";
    EXPECT_EQ(b.events_gebissen, 0u);
    EXPECT_GT(b.events_geprueft, 0u) << "der Nenner darf nie 0 sein, sonst ist die 0 im Zaehler bedeutungslos";
    EXPECT_FALSE(b.fehlgrund.empty()) << "fail-loud: ein unbrauchbarer Befund MUSS seinen Grund nennen";
    EXPECT_EQ(b.fehlgrund, std::string{"koeder_hat_nicht_gebissen"});
    EXPECT_TRUE(b.vendor_id().empty()) << "es gibt keinen Default-Vendor";
}

// K13 IN TESTFORM: oeffnen genuegt NICHT. Wer hier lockert (Biss-Pruefung raus), bekommt auf einer
// Maschine mit Zaehlern-ohne-Hardware-Zeit ein ON und misst strukturell Nullen.
TEST(PmcHostProbe, OeffnenOhneWertZaehltNichtAlsBiss) {
    auto const b = planner::probe_pmc_host<FakeOeffnetOhneWert>();
    EXPECT_EQ(b.lage, planner::PmcLage::Unbrauchbar);
    EXPECT_EQ(b.events_gebissen, 0u);
}

TEST(PmcHostProbe, IntelMitBissWirdIntel) {
    auto const b = planner::probe_pmc_host<FakeIntelBeisst>();
    EXPECT_EQ(b.lage, planner::PmcLage::Intel);
    EXPECT_EQ(b.vendor_id(), std::string_view{"intel"});
    EXPECT_EQ(b.events_gebissen, b.events_geprueft);
    EXPECT_TRUE(b.fehlgrund.empty());
    EXPECT_NE(b.nenner_zeile().find("lage=intel"), std::string::npos) << b.nenner_zeile();
    EXPECT_NE(b.nenner_zeile().find("beweist-nicht=runner-host"), std::string::npos)
        << "die Zeile MUSS ihre eigene Grenze nennen: der Planer-Host ist nicht der Runner-Host";
}

TEST(PmcHostProbe, UnbekannteHardwareformIstUnbrauchbarObwohlDieZaehlerBeissen) {
    auto const b = planner::probe_pmc_host<FakeFremdeHardware>();
    EXPECT_EQ(b.lage, planner::PmcLage::Unbrauchbar) << "fail-closed: eine Hardwareform, die die Registry "
                                                        "nicht kennt, kann im CEB-Stempel nicht benannt werden";
    EXPECT_EQ(b.events_gebissen, b.events_geprueft) << "die Zaehler HABEN gebissen -- der Grund ist der Vendor";
    EXPECT_EQ(b.fehlgrund, std::string{"unbekannte_hardwareform"});
}

// DIE KORREKTUR AM ENTWURF, als Test: Teilverfuegbarkeit ist BRAUCHBAR. Die reale Quelle definiert ihre
// Ehrlichkeit als ODER (ready_ = l1d || ll || dtlb || branch), und prod1 (Zen 5) hat strukturell keinen
// generischen Last-Level-Zaehler -- misst aber nachweislich PMC (Job 189916).
TEST(PmcHostProbe, TeilverfuegbarkeitIstBrauchbarUndDerBissVektorSagtWelche) {
    auto const b = planner::probe_pmc_host<FakeNurL1dBeisst>();
    EXPECT_EQ(b.lage, planner::PmcLage::Amd);
    EXPECT_EQ(b.events_gebissen, 1u);
    EXPECT_EQ(b.events_geprueft, 4u);
    EXPECT_NE(b.biss_vektor.find("cache_misses_l1=1"), std::string::npos) << b.biss_vektor;
    EXPECT_NE(b.biss_vektor.find("cache_misses_l3_ll=0"), std::string::npos) << b.biss_vektor;
    // Der Vektor nennt ALLE vier -- eine Teilaussage waere schlimmer als keine.
    EXPECT_EQ(std::count(b.biss_vektor.begin(), b.biss_vektor.end(), '='), 4);
}

// (e) DIE REALE PROBE auf DIESER Maschine. Sie darf nicht behaupten, was der fremde Nenner nicht deckt.
TEST(PmcHostProbe, RealeProbeStimmtMitProcCpuinfoUeberein) {
    auto const        b     = planner::probe_pmc_host<>();
    std::string const fremd = vendor_aus_proc_cpuinfo();
    ASSERT_FALSE(fremd.empty()) << "Gegenprobe: /proc/cpuinfo liefert keinen vendor_id -- der fremde Nenner "
                                   "existiert nicht, der Test kann nichts aussagen";
    EXPECT_TRUE(b.probe_gefahren);
    EXPECT_EQ(b.events_geprueft, 4u) << "der Event-Satz ist die EINE Liste (measurement::kPmcEvents)";
    if (b.lage == planner::PmcLage::Unbrauchbar) {
        EXPECT_FALSE(b.fehlgrund.empty()) << "unbrauchbar OHNE Grund waere eine stille Null";
        GTEST_SKIP() << "diese Maschine meldet PMC unbrauchbar (" << b.fehlgrund << "); die Vendor-Aussage "
                     << "ist dann gegenstandslos. Nenner-Zeile: " << b.nenner_zeile();
    }
    EXPECT_EQ(b.cpuid_vendor, fremd) << "die cpuid-Probe widerspricht /proc/cpuinfo -- eine der beiden luegt";
    EXPECT_GT(b.events_gebissen, 0u) << "brauchbar OHNE Biss ist konstruktiv unmoeglich; wenn das hier "
                                        "faellt, ist das Kriterium gebrochen";
    std::cout << "[REALE PROBE] " << b.nenner_zeile() << "\n";
}

namespace {

/// P1/A-05 (18.08.2026) -- DER STATE-DIREKTE ZUGANG ZUR (j3)-MECHANIK.
///
/// LAGE: der Emitter verzweigt ausschliesslich auf header_.build_semantic.cmake_build_type == "Debug"
/// (TierCiYamlBuilder, (j3)-Zweig). Bis zum work_mode-Umbau kam dieser Zustand aus dem Profil-Token
/// "debug"; den gibt es nicht mehr (V-12), und der EINGANG, der ihn kuenftig setzt (--debug bis in den
/// Director), ist ein eigener W2-Posten (Board #22/OD-7, S-8) -- er wird dort MIT eigenem
/// End-zu-Ende-Test gebaut. Die MECHANIK aber lebt, und ohne diesen Zugang waere sie unbeobachtbar:
/// ein Zweig ohne Test ist ein Zweig, der still verrotten darf.
///
/// WARUM EIN DECORATOR UND NICHT EINE VORAB-INJEKTION: eine Vorab-Injektion (begin_plan am Builder
/// VOR construct) traegt nicht -- construct ruft begin_plan selbst mit dem intern abgeleiteten Header
/// und ueberschreibt sie; das wurde am Objekt gemessen (Revert-Beleg ca26044e). Dieser Decorator sitzt
/// statt dessen ZWISCHEN Director und ConcreteBuilder: der Director ruft SEINE begin_plan, und er
/// reicht den Kopf gedreht nach innen. Es gibt keine Reihenfolge, die das ueberholen koennte, weil er
/// im Aufrufweg steht statt davor.
///
/// WAS DER TEST DAMIT WEITER PRUEFT -- und was nicht (ehrlich getrennt): der VOLLE Director-Walk laeuft
/// (construct -> walk_perms_ -> alle Perms/Steps/Combos), und geprueft wird, was der Director bei
/// Debug-Zustand EMITTIERT: (j3)-Dual-Compile, +bt-Signal, provision-only-Vorlauf. NICHT geprueft wird,
/// woher der Zustand kommt -- die ABLEITUNG eines Debug-Zustands aus einem Eingang existiert heute
/// nicht und gehoert zu S-8. Die Trennung ist die Aussage, nicht ihre Umgehung.
class DebugSemantikInjektor final : public planner::IPlanBuilder {
public:
    explicit DebugSemantikInjektor(planner::IPlanBuilder& inner) : inner_(inner) {}

    void begin_plan(planner::PlanHeader const& h) override { inner_.begin_plan(auf_debug_gedreht(h)); }
    void begin_perm(planner::PlanPerm const& p) override { inner_.begin_perm(p); }
    void on_step(planner::PlanStep const& s) override { inner_.on_step(s); }
    void end_perm(planner::PlanPerm const& p) override { inner_.end_perm(p); }
    void end_plan(planner::PlanHeader const& h) override { inner_.end_plan(auf_debug_gedreht(h)); }
    void begin_measurement_combo(planner::PlanMeasurementCombo const& c) override { inner_.begin_measurement_combo(c); }
    void end_measurement_combo(planner::PlanMeasurementCombo const& c) override { inner_.end_measurement_combo(c); }

private:
    /// Die VOLLE Zeile, die der ausgebaute work_mode "debug" trug: {Debug, misst, NICHT 1-Thread}.
    /// Nur cmake_build_type hat heute Leser (s. PlanBuildSemantic-Struct-Doku); die anderen beiden
    /// werden trotzdem wahrheitsgemaess gesetzt, damit der injizierte Zustand kein Drittel-Zustand
    /// ist, den es so nie gab.
    [[nodiscard]] static planner::PlanHeader auf_debug_gedreht(planner::PlanHeader h) {
        h.build_semantic.cmake_build_type = "Debug";
        h.build_semantic.measurement_on   = true;
        h.build_semantic.single_thread    = false;
        return h;
    }

    planner::IPlanBuilder& inner_;
};

} // namespace

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
    // S4-§62-B Lane-Budget (User-Drosselung 23.07. mit GO: amd 32->24 wegen RAM-Bound/Swap, intel bleibt 24 => beide
    // Lanes 24): DLL-Bau parallel mit dem Lane-Literal (24), NICHT mehr $(nproc) und NICHT =1. Beide Lanen praesent.
    EXPECT_NE(yaml.find("export COMDARE_BUILD_PARALLEL=\"24\""), std::string::npos) << "Lane-Budget 24 (beide Lanes)";
    EXPECT_EQ(yaml.find("export COMDARE_BUILD_PARALLEL=\"32\""), std::string::npos) << "amd 32 auf 24 gedrosselt";
    EXPECT_EQ(yaml.find("$(nproc)"), std::string::npos) << "§62-B: Lane-Budget-T-Wert-Literale ersetzen $(nproc)";
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

    // DEBUG-AUSPRAEGUNG -- STATE-DIREKT GETRIEBEN (A-05/V-12, 18.08.2026).
    //
    // Bis zum work_mode-Umbau stand hier `run_methodology = {"debug"}`: die Debug-Bau-Semantik kam aus
    // dem PROFIL-Token. Diesen Token gibt es nicht mehr -- debug war der einzige work_mode, der MASS UND
    // DABEI PARALLEL LIEF, und ist deshalb ausgebaut. Der work_mode bleibt hier `build` (die Ordnung ist
    // unberuehrt); die AUSPRAEGUNG dreht der DebugSemantikInjektor am Zustandsfeld (s. dessen Doku oben).
    //
    // GEPRUEFT WIRD UNVERAENDERT DIESELBE FAEHIGKEIT: (j3)-Dual-Compile, +bt-Signal, provision-only-
    // Vorlauf. Was fehlt und benannt bleibt: der EINGANG (--debug bis in den Director) kommt mit S-8/W2
    // und bekommt dort seinen eigenen End-zu-Ende-Test. Ohne diesen Zugang haette der (j3)-Zweig heute
    // GAR keine Deckung -- das waere ein verdeckter Zweig, kein bestandener Test.
    auto dbg             = tp;
    dbg->run_methodology = {"build"};
    planner::TierCiYamlBuilder tb_dbg;
    DebugSemantikInjektor      inj_dbg{tb_dbg};
    director.construct(*dbg, inj_dbg);
    std::string const& ydbg = tb_dbg.text();
    EXPECT_NE(ydbg.find("-DCMAKE_BUILD_TYPE=Debug"), std::string::npos)
        << "Debug-Zustand => statisch Debug (Auspraegung, nicht Modus)";
    EXPECT_NE(ydbg.find("COMDARE_BUILD_TYPE=\"Debug\""), std::string::npos) << "(i) Nicht-Default => +bt-Signal";
    // GEGENPROBE, damit der Injektor nicht selbst die Aussage ist: DERSELBE Profil-Zustand OHNE Injektor
    // muss den Debug-Zweig NICHT nehmen. Faellt diese Erwartung, dreht nicht der Zustand die Emission,
    // sondern irgendetwas anderes -- und der Test oben waere eine Tautologie.
    planner::TierCiYamlBuilder tb_ohne_inj;
    director.construct(*dbg, tb_ohne_inj);
    EXPECT_EQ(tb_ohne_inj.text().find("-DCMAKE_BUILD_TYPE=Debug"), std::string::npos)
        << "work_mode 'build' allein traegt KEIN Debug -- die Auspraegung kommt aus dem Zustandsfeld";

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
    // G4a: dieser Pin war zu WEIT gefasst. Gemeint war immer der (j3)-DEBUG-Override "TRIES=1", nicht jede Erwaehnung
    // der Variablen -- seit P-A traegt JEDER Batch zusaetzlich den guarded Blackhole-Deckel. Der Pin wird deshalb auf
    // seine tatsaechliche Absicht VERSCHAERFT (exakte Debug-Form verboten) statt aufgeweicht, und der neue Deckel
    // bekommt eine eigene positive Wache.
    EXPECT_EQ(yaml.find("export COMDARE_ARTEFAKT_TRIES=1"), std::string::npos)
        << "measure => kein (j3)-Debug-Override auf 1";
    EXPECT_NE(yaml.find("export COMDARE_ARTEFAKT_TRIES=\"${COMDARE_ARTEFAKT_TRIES:-2}\""), std::string::npos)
        << "P-A: der guarded Blackhole-Deckel ist auch im measure-Profil da (nur unter STORAGE_CACHE scharf)";

    // Bare-metal (--emit-tier-cmake): DLL-Bau-Pool parallel (ProcessorCount) fuer measure; kein +bt. Debug => +bt.
    planner::TierCmakeGraphBuilder cm;
    director.construct(*tp, cm);
    EXPECT_NE(cm.text().find("COMDARE_BUILD_PARALLEL=${COMDARE_PLAN_MEASURE_PARALLEL}"), std::string::npos);
    EXPECT_NE(cm.text().find("ProcessorCount(_comdare_measure_nproc)"), std::string::npos)
        << "Default = ProcessorCount";
    EXPECT_EQ(cm.text().find("COMDARE_BUILD_PARALLEL=1\n"), std::string::npos);
    EXPECT_EQ(cm.text().find("COMDARE_BUILD_TYPE=Debug"), std::string::npos) << "measure => kein +bt (cmake)";
    // cmake-Zweig: derselbe state-direkte Zugang (TierCmakeGraphBuilder setzt header_ ebenfalls in begin_plan).
    planner::TierCmakeGraphBuilder cm_dbg;
    DebugSemantikInjektor          inj_cm_dbg{cm_dbg};
    director.construct(*dbg, inj_cm_dbg);
    EXPECT_NE(cm_dbg.text().find("\"COMDARE_BUILD_TYPE=Debug\""), std::string::npos)
        << "Debug-Zustand => +bt-Signal (cmake)";

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
    tp2->run_methodology = {"build", "measure"}; // 2 Modi = Kontraktbruch (exactly-one verletzt)
    planner::TierCmakeGraphBuilder cm_two;
    EXPECT_THROW(director.construct(*tp2, cm_two), std::invalid_argument)
        << "(R5) tp-Pfad: build_semantic_of_run_methodology bricht bei >1 Modi HART ab (kein stilles front())";

    // Runtime-Konsum (Mess-Loop-Naht, resolve_measure_parallelism -> run_methodology_for_ids): wirft ebenfalls bei >1.
    EXPECT_THROW((void)mm::run_methodology_for_ids({"build", "measure"}), std::invalid_argument)
        << "(R5) run_methodology_for_ids bricht bei >1 Methoden HART ab";
    EXPECT_THROW((void)mm::run_methodology_for_ids({"measure", "release"}), std::invalid_argument);

    // exactly-one bleibt gueltig + byte-neutral (kein Fehlalarm):
    EXPECT_EQ(mm::run_methodology_for_ids({"build"}).methodology, mm::WorkMode::Build);
    EXPECT_EQ(mm::run_methodology_for_ids({}).methodology, mm::WorkMode::Measure); // leer => measure-Default
    auto tp1 = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp1.has_value());
    tp1->run_methodology = {"measure"};
    planner::TierCmakeGraphBuilder cm_one;
    EXPECT_NO_THROW(director.construct(*tp1, cm_one)) << "(R5) exactly-one-Profil baut normal (byte-neutral)";
}

// (Welle B/1, 2026-08-07) FAIL-CLOSED beim UNBEKANNTEN Modus-Token. Bis heute fiel ein Tippfehler ("mesure") auf
// BEIDEN Konsum-Ebenen STILL auf measure zurueck -- der Planer emittierte daraufhin eine vollstaendige measure-
// Mess-Strecke, also eine MESSUNG, die niemand angefordert hat. Leer bleibt der Default (Abwesenheit ist eine
// Aussage), ein falsch geschriebener Wunsch ist es NICHT.
TEST(MeasurementModi61, UnknownModeTokenFailsClosedInsteadOfSilentMeasure) {
    namespace mm = comdare::cache_engine::measurement;
    planner::ExperimentPlanDirector const director;

    // (1) Runtime-Konsum (Mess-Loop-Naht, run_methodology_for_ids). Der (void)-Cast: die Funktion ist [[nodiscard]],
    // und die Rueckgabe interessiert im Wurf-Fall nicht (sonst -Wunused-result).
    EXPECT_THROW((void)mm::run_methodology_for_ids({"mesure"}), std::invalid_argument);
    EXPECT_THROW((void)mm::run_methodology_for_ids({"profiling"}), std::invalid_argument);
    EXPECT_THROW((void)mm::run_methodology_for_ids({""}), std::invalid_argument) << "leeres TOKEN != leere Liste";
    EXPECT_THROW((void)mm::run_methodology_for_ids({"Measure"}), std::invalid_argument)
        << "Tokens sind klein geschrieben";
    // SPRECHEND, nicht nur hart: die Meldung nennt das Token UND die gueltige Menge aus der Registry.
    try {
        (void)mm::run_methodology_for_ids({"mesure"});
        ADD_FAILURE() << "unbekannter Token muss werfen";
    } catch (std::invalid_argument const& e) {
        std::string const msg = e.what();
        EXPECT_NE(msg.find("mesure"), std::string::npos) << msg;
        // A-05/V-12 (18.08.2026): die gueltige Menge steht hier NICHT mehr als Literal. Sie stand es bis
        // zum work_mode-Umbau ("debug, measure, release, compare") und war damit genau das, was die
        // Meldung selbst vermeiden will: eine zweite, handgepflegte Wissensquelle, die bei jeder
        // Enum-Bewegung still veraltet. Geprueft wird jetzt gegen DIESELBE Ableitung, aus der die Meldung
        // gebaut wird -- damit kann der Test die Menge nicht mehr verfehlen, egal wie sie sich aendert.
        EXPECT_NE(msg.find(mm::detail::run_methodology_known_ids()), std::string::npos) << msg;
        // Gegenprobe, damit die Zeile darueber keine Tautologie ist (leerer Teilstring findet immer):
        // die Menge ist nicht leer und nennt wirklich die Tokens.
        ASSERT_FALSE(mm::detail::run_methodology_known_ids().empty());
        EXPECT_NE(msg.find("measure"), std::string::npos) << msg;
    }

    // (2) Planer-Konsum (build_semantic_of_run_methodology via construct()) -- KEINE Emission mehr aus Tippfehlern.
    auto tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    tp->run_methodology = {"mesure"};
    planner::TierCmakeGraphBuilder cm_bogus;
    EXPECT_THROW(director.construct(*tp, cm_bogus), std::invalid_argument)
        << "tp-Pfad: unbekannter Token bricht HART ab (kein stiller measure-Ersatz)";

    // Gegenprobe (byte-neutral, kein Fehlalarm): die VIER gueltigen Tokens und die leere Liste tragen unveraendert.
    EXPECT_EQ(mm::run_methodology_for_ids({"build"}).methodology, mm::WorkMode::Build);
    EXPECT_EQ(mm::run_methodology_for_ids({"measure"}).methodology, mm::WorkMode::Measure);
    EXPECT_EQ(mm::run_methodology_for_ids({"release"}).methodology, mm::WorkMode::Release);
    EXPECT_EQ(mm::run_methodology_for_ids({"compare"}).methodology, mm::WorkMode::Compare);
    EXPECT_EQ(mm::run_methodology_for_ids({}).methodology, mm::WorkMode::Measure) << "leer => measure-Default";
    auto tp_ok = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp_ok.has_value());
    tp_ok->run_methodology = {"compare"};
    planner::TierCmakeGraphBuilder cm_ok;
    EXPECT_NO_THROW(director.construct(*tp_ok, cm_ok)) << "gueltiger Token baut normal";
}

// (Welle B/3, 2026-08-07) DER PLAN-SEITIGE SPIEGEL DER REGISTRY-ZEILE.
// Befund am Objekt: PlanBuildSemantic::measurement_on und ::single_thread haben NULL produktive Leser -- alle acht
// Zugriffe der Form header_.build_semantic.* im Repo lesen cmake_build_type. Beide Felder bleiben stehen (S6-Konsum,
// s. Struct-Doku); ein stiller Rueckbau waere schlimmer als ein totes Feld. Damit ein ungelesenes Feld aber nicht
// STILL falsch werden kann -- niemand merkt es ja -- nagelt diese Wache den Spiegel je Modus an die Registry-Zeile,
// aus der er stammt. Wer die Registry aendert und den Planer vergisst (oder umgekehrt), faellt hier auf, statt dem
// spaeteren S6-Fanout einen falschen Wert zu vererben.
TEST(MeasurementModi61, PlanBuildSemanticSpiegeltDieRegistryZeileFuerJedenModus) {
    namespace mm = comdare::cache_engine::measurement;
    planner::ExperimentPlanDirector const director;

    auto semantik_fuer = [&director](std::vector<std::string> const& methodik) {
        auto tp = parse_thesis(COMDARE_PLANNER_THESIS_MIN);
        EXPECT_TRUE(tp.has_value());
        tp->run_methodology = methodik;
        CountingBuilder b;
        director.construct(*tp, b);
        return b.header.build_semantic;
    };

    for (std::size_t i = 0; i < mm::kWorkModeCount; ++i) {
        auto const& zeile = mm::kWorkModeRegistry[i];
        auto const  sem   = semantik_fuer({std::string(zeile.id)});
        EXPECT_EQ(sem.cmake_build_type, std::string(zeile.cmake_build_type)) << zeile.id;
        EXPECT_EQ(sem.measurement_on, zeile.measurement_on) << zeile.id;
        EXPECT_EQ(sem.single_thread, zeile.single_thread) << zeile.id;
    }

    // Leer => die measure-Zeile (Abwesenheit ist der Default, kein eigener Zustand).
    auto const& measure = mm::work_mode_info(mm::WorkMode::Measure);
    auto const  leer    = semantik_fuer({});
    EXPECT_EQ(leer.cmake_build_type, std::string(measure.cmake_build_type));
    EXPECT_EQ(leer.measurement_on, measure.measurement_on);
    EXPECT_EQ(leer.single_thread, measure.single_thread);

    // Und die Unterscheidungs-Probe, ohne die der Spiegel-Test nichts messen wuerde: measurement_on traegt NICHT
    // ueber alle Modi denselben Wert. Ein Feld, das immer gleich ist, waere auch als Annotation wertlos.
    // A-05/V-12 (18.08.2026): der TRUE-Fall stand hier auf {"build"} -- das war die alte debug-Zeile
    // unter neuem Namen. Er ist FALSCH: `build` BAUT, es misst nicht (Registry-Zeile
    // {Build,"build","Build","Release",false,false}). Der einzige messende Modus ist heute `measure`.
    // Die ABSICHT des Blocks bleibt unberuehrt -- es geht darum, dass die Felder ueber die Modi
    // WIRKLICH variieren; ein Feld, das immer gleich ist, waere auch als Annotation wertlos.
    EXPECT_TRUE(semantik_fuer({"measure"}).measurement_on);
    EXPECT_FALSE(semantik_fuer({"build"}).measurement_on);
    EXPECT_FALSE(semantik_fuer({"release"}).measurement_on);
    EXPECT_FALSE(semantik_fuer({"compare"}).measurement_on);
    EXPECT_TRUE(semantik_fuer({"measure"}).single_thread);
    EXPECT_FALSE(semantik_fuer({"build"}).single_thread);
    // Und die Variations-Aussage selbst, statt sie den Einzelzeilen oben nur zu entnehmen: BEIDE Felder
    // tragen ueber die Registry mindestens einen true- UND einen false-Fall. Faellt das, ist der
    // Spiegel-Test oben ein Spiegel ohne Bild -- er verglaeche dann eine Konstante mit sich selbst.
    bool mess_gesehen[2]{}, thread_gesehen[2]{};
    for (std::size_t i = 0; i < mm::kWorkModeCount; ++i) {
        mess_gesehen[mm::kWorkModeRegistry[i].measurement_on ? 1 : 0]  = true;
        thread_gesehen[mm::kWorkModeRegistry[i].single_thread ? 1 : 0] = true;
    }
    EXPECT_TRUE(mess_gesehen[0] && mess_gesehen[1]) << "measurement_on variiert nicht mehr ueber die Modi";
    EXPECT_TRUE(thread_gesehen[0] && thread_gesehen[1]) << "single_thread variiert nicht mehr ueber die Modi";
}

// (smoke=>debug-Entkopplung 2026-07-22): der Director-Methodik-Override entkoppelt Bau-Profil != Methodik-Profil.
// Ein all_axes_golden-Katalog (run_methodology=measure) emittiert MIT Override {"build"} die DEBUG-Methodik
// ((j3)-Dual-Compile + COMDARE_BUILD_TYPE=Debug + TRIES=1 + Methodik-Profil-Forward), WAEHREND Achsen/Perms/Lanes
// aus tp UNVERAENDERT bleiben (nur die Methodik wechselt, nicht der Katalog). Leerer Override => byte-identisch.
TEST(MeasurementModi61, MethodikOverrideDecouplesCatalogFromMethodik) {
    planner::ExperimentPlanDirector const director;
    auto const                            tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    ASSERT_EQ(tp->run_methodology.front(), "measure") << "Katalog-Profil traegt run_methodology=measure";

    planner::TierCiYamlBuilder tb_ref; // ohne Override => measure-Emission (Referenz)
    director.construct(*tp, tb_ref);
    std::string const& yref = tb_ref.text();
    EXPECT_EQ(yref.find("(j3) Aufruf 1/2"), std::string::npos) << "measure-Katalog ohne Override => KEIN Dual-Compile";
    // Der Prolog-Re-Derive (Methodik-Selektor Basename->Voll-Pfad) ist IMMER emittiert (spiegelt COMDARE_GOLDEN_N_
    // PROFILE; No-Op bei unset => measure byte-neutral). ABER der emit_measure_job-FORWARD (nur (j3)-Debug-Zweig) fehlt.
    EXPECT_NE(yref.find("case \"${COMDARE_PLAN_METHODIK_PROFILE:-}\" in"), std::string::npos)
        << "Prolog-Re-Derive des Methodik-Selektors ist immer emittiert (No-Op bei unset)";
    EXPECT_NE(yref.find("thesis_profiles/${COMDARE_PLAN_METHODIK_PROFILE}\""), std::string::npos)
        << "Prolog-Re-Derive Basename-Fall: montiert ${CI_PROJECT_DIR}/.../thesis_profiles/<Basename> (KLASSEN-Regel)";
    EXPECT_EQ(yref.find("COMDARE_PLAN_METHODIK_PROFILE=\"${COMDARE_PLAN_METHODIK_PROFILE:-}\""), std::string::npos)
        << "measure ohne Override => KEIN emit_measure_job-Methodik-Forward (measure byte-identisch)";

    // -- A-05/V-12 (18.08.2026): WAS AN DIESEM TEST HEUTE NOCH BEWEISBAR IST -- und was nicht ----------
    //
    // Bis zum work_mode-Umbau bewies dieser Block die Entkopplung an der EMISSION: Override {"debug"}
    // drehte den Katalog auf Debug-Bau, und der (j3)-Zweig wurde sichtbar. Das geht nicht mehr, und der
    // Grund ist keine Test-Schwaeche, sondern eine Eigenschaft des neuen Enums: ALLE VIER work_modes
    // tragen cmake_build_type "Release" (Registry-Zeilen build/measure/compare/release). Sie
    // unterscheiden sich nur noch in measurement_on/single_thread -- und die haben im Emitter NULL
    // Leser (s. PlanBuildSemantic-Struct-Doku). Ein Methodik-Override kann die Emission also
    // GAR NICHT mehr bewegen; jede Erwartung auf einen sichtbaren Unterschied waere ab heute eine
    // Erwartung an etwas, das es nicht gibt.
    //
    // DER TEST BEHAUPTET DESHALB NICHT MEHR, WAS ER NICHT MEHR ZEIGEN KANN. Er beweist statt dessen die
    // NAHT selbst -- dass der Override ueberhaupt GELESEN wird und den Katalog-Wert verdraengt --, und
    // zwar an der einzigen Stelle, an der das heute beobachtbar ist: an der fail-closed-Wache. Ein
    // ungueltiger Override muss werfen, OBWOHL das Katalog-Profil (measure) voellig gueltig ist. Wuerde
    // der Override ignoriert, liefe der Aufruf still durch.
    {
        bool geworfen = false;
        try {
            planner::TierCiYamlBuilder tb_bogus;
            director.construct(*tp, tb_bogus, /*combo_selector=*/{}, /*methodik_run_methodology=*/{"mesure"});
        } catch (std::invalid_argument const&) { geworfen = true; }
        EXPECT_TRUE(geworfen) << "der Methodik-Override wird NICHT gelesen -- ein ungueltiger Override lief still "
                                 "durch, waehrend das Katalog-Profil ihn verdeckte";
    }

    // Und die Gegenrichtung, damit die Wache oben nicht bloss 'irgendetwas wirft': ein GUELTIGER
    // Override laeuft durch und laesst den KATALOG unberuehrt -- Perm-Lanes und Mess-Job-Zahl bleiben
    // exakt die aus tp. Das ist die Entkopplungs-Aussage, soweit sie heute traegt.
    planner::TierCiYamlBuilder tb_ovr;
    director.construct(*tp, tb_ovr, /*combo_selector=*/{}, /*methodik_run_methodology=*/{"build"});
    std::string const& yovr = tb_ovr.text();
    EXPECT_NE(yovr.find("  tags: [\"amd\"]"), std::string::npos) << "no_extension-Perm-Lane aus tp erhalten";
    EXPECT_NE(yovr.find("  tags: [\"intel\"]"), std::string::npos) << "avx2-Perm-Lane aus tp erhalten";
    EXPECT_GT(count_occurrences(yref, "# JOB measure-batch "), 0u) << "Katalog emittiert Mess-Jobs";
    EXPECT_EQ(count_occurrences(yovr, "# JOB measure-batch "), count_occurrences(yref, "# JOB measure-batch "))
        << "Perm-/Mess-Job-Zahl (Katalog) unveraendert -- Entkopplung Bau != Methodik";

    // DIE (j3)-HAELFTE, ehrlich getrennt: sie haengt heute am ZUSTAND, nicht mehr am Override. Getrieben
    // wird sie state-direkt (DebugSemantikInjektor) -- so bleibt der Zweig unter Beobachtung, ohne dass
    // der Test vorgibt, der Override habe ihn ausgeloest. Sobald der --debug-Eingang aus S-8/W2 steht,
    // gehoert die Verbindung Override/Flag -> Zustand DORT hin und wird DORT geprueft.
    planner::TierCiYamlBuilder tb_dbg;
    DebugSemantikInjektor      inj_ovr{tb_dbg};
    director.construct(*tp, inj_ovr, /*combo_selector=*/{}, /*methodik_run_methodology=*/{"build"});
    std::string const& ydbg = tb_dbg.text();
    EXPECT_NE(ydbg.find("(j3) Aufruf 1/2: Release provision-only"), std::string::npos)
        << "Debug-Zustand => (j3)-Dual-Compile trotz measure-Katalog";
    EXPECT_NE(ydbg.find("COMDARE_BUILD_TYPE=\"Debug\""), std::string::npos) << "Debug-Zustand => +bt-Signal";
    EXPECT_NE(ydbg.find("export COMDARE_ARTEFAKT_TRIES=1"), std::string::npos) << "Debug-Zustand => Blocker #50";
    EXPECT_NE(ydbg.find("COMDARE_PLAN_METHODIK_PROFILE=\"${COMDARE_PLAN_METHODIK_PROFILE:-}\""), std::string::npos)
        << "Debug-Zustand => emit_measure_job-Methodik-Forward an den Grandchild-Mess-Run";
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

// =============================================================================
// CI-DUAL (Owner 14.08.2026 verbatim: "... dass in ALLEN pipelines aller C++ Projekte, die binaries
// nicht wie geplant mit gcc und clang dual in compile Release und Debug gebaut werden, um ganz sicher
// mit 2 verschiedenen compilern die Kompatibilitaet zu beweisen ... unter anderem in jeder
// Traegerstufe der Cache Engine") -- E1-E4 der Design-Fassung 14.08. (A2.5-Fix, FUND-1):
//   E1  gcc-15/g++-15-Pin HART an JEDER emittierten Treiber-Konfiguration (F3-Default: die GEPLANTEN
//       Versionen, KON55-01; der bisherige Host-Default war ungepinnt und damit unehrlich).
//   E2  clang-22-ZWILLING je BAU-Stufe (Stufe 1 ceb:build, Stufe 2 tier-build-batch): ZWEITE Sequenz
//       IM SELBEN Job (sequentiell = EIN Bau-Slot, T-11b), gleiche Defines (PMC/Combo/BUILD_TYPE --
//       sonst pruefte der Zwilling eine ANDERE CEB), eigenes build-clang, Bau + ctest-Gate der
//       UNBEDINGT registrierten Treiber-Tests (--no-tests=error: Leerlauf ist Fehler, W0b-3-Klasse).
//   E3  Mess-Batch: NUR der Pin (Spiegel-Configure des geteilten Code/build), KEIN Zwilling -- in den
//       Mess-Gliedern wird NIE mit zwei Compilern gemessen (N2/Auflage 5b).
//   E4  bare-metal-Spiegel SAGT die Pins (W2-Muster :577ff.: der aeussere Configure baut, die
//       Emission weist an) -- CI und bare-metal fahren dieselben Zellen (V-5, beide literal).
// KEINE Stempel-/Preimage-Beruehrung: der Compiler wird erst mit S-9/S-11 Achsen-Wert (N1).
// =============================================================================
namespace {

struct CompilerPinReport {
    std::size_t              gcc_builds       = 0; // NENNER: Treiber-Bau-Zeilen im gcc-Verzeichnis `build`
    std::size_t              gcc_configured   = 0; // davon unmittelbar hinter `cmake -B build -G Ninja`
    std::size_t              gcc_pinned       = 0; // davon: Configure traegt den gcc-15/g++-15-Pin
    std::size_t              twin_configs     = 0; // `cmake -B build-clang -G Ninja`-Zeilen (Zwillings-Configure)
    std::size_t              twin_builds      = 0; // Treiber-Bau-Zeilen im Zwillings-Verzeichnis build-clang
    std::size_t              twin_configured  = 0; // davon unmittelbar hinter dem Zwillings-Configure
    std::size_t              twin_pinned      = 0; // Zwillings-Configures mit clang-22-Pin
    std::size_t              twin_ctest_gates = 0; // ctest-Gates des Stufen-Binaries (build-clang/02_messung_driver)
    std::size_t              twin_bt_riss     = 0; // Zwillinge, deren BUILD_TYPE vom letzten gcc-Configure abweicht
    std::vector<std::string> violations;
};

std::string build_type_von(std::string const& zeile) {
    static constexpr char const* kBt = "-DCMAKE_BUILD_TYPE=";
    std::size_t const            p   = zeile.find(kBt);
    if (p == std::string::npos) return {};
    std::size_t const start = p + std::string_view{kBt}.size();
    std::size_t const end   = zeile.find(' ', start);
    return zeile.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

// Der EINE Pruefer (Muster pmc_invariant: vom TREIBER-BAU aus gezaehlt, damit keine Stelle durch
// Configure-Umformulierung verschwindet; Gegenprobe unten zuerst).
CompilerPinReport compiler_pin_report(std::string const& emitted) {
    static constexpr char const* kDrvGcc   = "cmake --build build --target comdare-messung-driver";
    static constexpr char const* kDrvTwin  = "cmake --build build-clang --target comdare-messung-driver";
    static constexpr char const* kConfGcc  = "cmake -B build -G Ninja";
    static constexpr char const* kConfTwin = "cmake -B build-clang -G Ninja";
    static constexpr char const* kPinGcc   = "-DCMAKE_C_COMPILER=gcc-15 -DCMAKE_CXX_COMPILER=g++-15";
    static constexpr char const* kPinClang = "-DCMAKE_C_COMPILER=clang-22 -DCMAKE_CXX_COMPILER=clang++-22";
    static constexpr char const* kGate     = "ctest --test-dir build-clang/02_messung_driver --no-tests=error";

    CompilerPinReport              rep;
    std::vector<std::string> const lines = split_lines(emitted);
    std::string                    letzter_gcc_bt; // BUILD_TYPE des letzten gcc-Configures (Stufen-Kontext)
    for (std::size_t i = 0; i < lines.size(); ++i) {
        std::string const& z = lines[i];
        if (z.find(kConfTwin) != std::string::npos) {
            ++rep.twin_configs;
            if (z.find(kPinClang) != std::string::npos) ++rep.twin_pinned;
            if (!letzter_gcc_bt.empty() && build_type_von(z) != letzter_gcc_bt) {
                ++rep.twin_bt_riss;
                rep.violations.push_back("Zwillings-BUILD_TYPE weicht von der Stufe ab: " + z);
            }
        } else if (z.find(kConfGcc) != std::string::npos) {
            letzter_gcc_bt = build_type_von(z);
        }
        if (z.find(kGate) != std::string::npos) ++rep.twin_ctest_gates;
        if (z.find(kDrvTwin) != std::string::npos) {
            ++rep.twin_builds;
            if (i > 0 && lines[i - 1].find(kConfTwin) != std::string::npos)
                ++rep.twin_configured;
            else
                rep.violations.push_back("kein Zwillings-Configure unmittelbar VOR dem Zwillings-Bau: " + z);
            continue; // kDrvTwin enthaelt kDrvGcc NICHT (verschiedene Verzeichnisse), aber explizit halten
        }
        if (z.find(kDrvGcc) != std::string::npos) {
            ++rep.gcc_builds;
            if (i > 0 && lines[i - 1].find(kConfGcc) != std::string::npos) {
                ++rep.gcc_configured;
                if (lines[i - 1].find(kPinGcc) != std::string::npos) ++rep.gcc_pinned;
            } else {
                rep.violations.push_back("kein `cmake -B build -G Ninja` unmittelbar VOR dem Treiber-Bau: " + z);
            }
        }
    }
    return rep;
}

void expect_compiler_pin_invariante(std::string const& emitted, char const* wo, std::size_t soll_zwillinge) {
    CompilerPinReport const rep = compiler_pin_report(emitted);
    std::string             verstoesse;
    for (auto const& v : rep.violations) verstoesse += "\n    - " + v;
    EXPECT_GT(rep.gcc_builds, 0u) << wo << ": Wache leer gelaufen -- kein gcc-Treiber-Bau in der Emission.";
    EXPECT_EQ(rep.gcc_configured, rep.gcc_builds) << wo << ": Nachbarschaft Configure->Bau gebrochen." << verstoesse;
    EXPECT_EQ(rep.gcc_pinned, rep.gcc_builds)
        << wo << ": " << rep.gcc_builds << " gcc-Treiber-Konfigurationen, davon nur " << rep.gcc_pinned
        << " mit dem HARTEN gcc-15/g++-15-Pin (E1/F3: die geplanten Versionen, KON55-01; Host-Default ist "
           "ungepinnt und damit je Runner verschieden -- genau die Unehrlichkeit, die der Pin beseitigt)."
        << verstoesse;
    EXPECT_EQ(rep.twin_configs, soll_zwillinge)
        << wo << ": clang-Zwillings-Configures (SOLL = eine je BAU-Stufen-Emission dieses Builders; "
        << "Mess-Batches tragen KEINEN Zwilling, N2)." << verstoesse;
    EXPECT_EQ(rep.twin_builds, soll_zwillinge) << wo << ": Zwillings-Treiber-Bau-Zeilen." << verstoesse;
    EXPECT_EQ(rep.twin_configured, rep.twin_builds)
        << wo << ": Zwillings-Nachbarschaft Configure->Bau gebrochen." << verstoesse;
    EXPECT_EQ(rep.twin_pinned, rep.twin_configs)
        << wo << ": jeder Zwillings-Configure traegt den clang-22-Pin (E2)." << verstoesse;
    EXPECT_EQ(rep.twin_ctest_gates, soll_zwillinge)
        << wo << ": je Zwilling GENAU EIN ctest-Gate des Stufen-Binaries "
        << "(build-clang/02_messung_driver, --no-tests=error -- Leerlauf ist Fehler, kein Schein-Gate).";
    EXPECT_EQ(rep.twin_bt_riss, 0u)
        << wo << ": der Zwilling faehrt den BUILD_TYPE seiner Stufe (Release; im (j3)-Debug-Profil Debug)."
        << verstoesse;
}

} // namespace

TEST(CompilerPinInvariante, GegenprobeDerPruefer) {
    // Handgebauter Text: (1) gepinnter gcc-Bau + voller Zwilling, (2) UNGEPINNTER gcc-Bau,
    // (3) Zwilling OHNE clang-Pin und mit BUILD_TYPE-Riss. Findet der Pruefer das nicht, taugt er nicht.
    std::string const kunstlich =
        std::string{} + "    - cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release"
                        " -DCMAKE_C_COMPILER=gcc-15 -DCMAKE_CXX_COMPILER=g++-15\n"
                        "    - cmake --build build --target comdare-messung-driver\n"
                        "    - cmake -B build-clang -G Ninja -DCMAKE_BUILD_TYPE=Release"
                        " -DCMAKE_C_COMPILER=clang-22 -DCMAKE_CXX_COMPILER=clang++-22\n"
                        "    - cmake --build build-clang --target comdare-messung-driver\n"
                        "      ctest --test-dir build-clang/02_messung_driver --no-tests=error --output-on-failure\n"
                        "    - cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release\n"
                        "    - cmake --build build --target comdare-messung-driver\n"
                        "    - cmake -B build-clang -G Ninja -DCMAKE_BUILD_TYPE=Debug\n"
                        "    - cmake --build build-clang --target comdare-messung-driver\n";
    CompilerPinReport const probe = compiler_pin_report(kunstlich);
    ASSERT_EQ(probe.gcc_builds, 2u);
    ASSERT_EQ(probe.gcc_configured, 2u);
    ASSERT_EQ(probe.gcc_pinned, 1u) << "Gegenprobe: GENAU einer der beiden gcc-Configures traegt den Pin";
    ASSERT_EQ(probe.twin_configs, 2u);
    ASSERT_EQ(probe.twin_builds, 2u);
    ASSERT_EQ(probe.twin_configured, 2u);
    ASSERT_EQ(probe.twin_pinned, 1u) << "Gegenprobe: der zweite Zwilling ist UNGEPINNT und muss auffallen";
    ASSERT_EQ(probe.twin_ctest_gates, 1u) << "Gegenprobe: nur der erste Zwilling traegt das ctest-Gate";
    ASSERT_EQ(probe.twin_bt_riss, 1u) << "Gegenprobe: der zweite Zwilling reisst den BUILD_TYPE (Debug vs Release)";
}

TEST(CompilerPinInvariante, JedeTreiberKonfigurationGepinntUndJedeBauStufeMitClangZwilling) {
    planner::PmcHostBefund const    amd = befund_mit(planner::PmcLage::Amd, "AuthenticAMD");
    planner::ExperimentPlanDirector director;
    director.set_pmc_befund(amd);

    // (1) Thesis-Kanal all_axes_golden: Stufe 1 = je Combo ceb:build (Zwilling) + ceb:emit (nur Pin);
    //     Stufe 2 = je Host-Lane tier-build-batch (Zwilling) + measure-batch (nur Pin, N2).
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::CiYamlBuilder cb;
    director.construct(*tp, cb);
    std::size_t const combos_s1 = count_occurrences(cb.text(), "# JOB ceb-build combo");
    ASSERT_GT(combos_s1, 0u);
    expect_compiler_pin_invariante(cb.text(), "Thesis/Verbund1 CiYamlBuilder", combos_s1);
    planner::TierCiYamlBuilder tb;
    director.construct(*tp, tb);
    std::size_t const batches_s2 = count_occurrences(tb.text(), "# JOB tier-build-batch host=");
    ASSERT_GT(batches_s2, 0u);
    expect_compiler_pin_invariante(tb.text(), "Thesis/Verbund2 TierCiYamlBuilder", batches_s2);

    // (2) GEFANNT (3 Combos): die Invariante haengt nicht an der Stellenzahl.
    auto tp_fan = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp_fan.has_value());
    tp_fan->measurement_tooling = {{"wallclock"}, {"macro"}, {"micro"}};
    planner::CiYamlBuilder cb_fan;
    director.construct(*tp_fan, cb_fan);
    expect_compiler_pin_invariante(cb_fan.text(), "Thesis/Verbund1 GEFANNT",
                                   count_occurrences(cb_fan.text(), "# JOB ceb-build combo"));
    planner::TierCiYamlBuilder tb_fan;
    director.construct(*tp_fan, tb_fan);
    expect_compiler_pin_invariante(tb_fan.text(), "Thesis/Verbund2 GEFANNT",
                                   count_occurrences(tb_fan.text(), "# JOB tier-build-batch host="));

    // (3) Experiment-Kanal (eigener Zwilling -- die "Fix fehlt im Experiment-Zwilling"-Klasse).
    auto const ep = parse_experiment(COMDARE_EXPERIMENT_GOLDEN);
    ASSERT_TRUE(ep.has_value());
    planner::CiYamlBuilder cb_exp;
    director.construct(*ep, cb_exp);
    expect_compiler_pin_invariante(cb_exp.text(), "Experiment/Verbund1",
                                   count_occurrences(cb_exp.text(), "# JOB ceb-build combo"));
    planner::TierCiYamlBuilder tb_exp;
    director.construct(*ep, tb_exp);
    expect_compiler_pin_invariante(tb_exp.text(), "Experiment/Verbund2",
                                   count_occurrences(tb_exp.text(), "# JOB tier-build-batch host="));

    // (4) OHNE PMU: der Pin haengt NICHT am PMC-Befund (zwei orthogonale Invarianten).
    planner::PmcHostBefund const    ohne = befund_mit(planner::PmcLage::Unbrauchbar, "SomeOtherVendor");
    planner::ExperimentPlanDirector director_ohne;
    director_ohne.set_pmc_befund(ohne);
    planner::CiYamlBuilder cb_ohne;
    director_ohne.construct(*tp, cb_ohne);
    expect_compiler_pin_invariante(cb_ohne.text(), "Thesis/Verbund1 OHNE PMU",
                                   count_occurrences(cb_ohne.text(), "# JOB ceb-build combo"));
}

// (j3)-Debug-Profil: der Stufe-2-Zwilling folgt dem BUILD_TYPE der Stufe (SOLL-Matrix: "clang x Debug
// NUR im (j3)-Debug-Profil"); der Mess-Zweig ((j3)-Dual) bleibt UNVERAENDERT single-compiler.
TEST(CompilerPinInvariante, ZwillingFolgtDemBuildTypDesProfils) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector director;
    director.set_pmc_befund(befund_mit(planner::PmcLage::Amd, "AuthenticAMD"));
    // A-05/V-12 (18.08.2026): der (j3)-Zweig haengt am build_semantic-ZUSTAND, nicht mehr an einem
    // Profil-Token -- alle vier work_modes tragen heute cmake_build_type "Release" (s. die ausfuehrliche
    // Begruendung an MethodikOverrideDecouplesCatalogFromMethodik). Der Zustand wird deshalb
    // state-direkt gesetzt; die SOLL-Matrix-Aussage dieses Tests -- der Zwilling FOLGT dem BUILD_TYPE
    // der Stufe -- ist davon unberuehrt, denn genau dieses Folgen wird hier geprueft.
    planner::TierCiYamlBuilder tb_dbg;
    DebugSemantikInjektor      inj_pin{tb_dbg};
    director.construct(*tp, inj_pin, /*combo_selector=*/{}, /*methodik_run_methodology=*/{"build"});
    std::string const& ydbg = tb_dbg.text();
    expect_compiler_pin_invariante(ydbg, "Thesis/Verbund2 (j3)-Debug",
                                   count_occurrences(ydbg, "# JOB tier-build-batch host="));
    EXPECT_NE(ydbg.find("cmake -B build-clang -G Ninja -DCOMDARE_V32_ENABLE=ON"), std::string::npos);
    EXPECT_NE(ydbg.find("-DCMAKE_BUILD_TYPE=Debug"), std::string::npos)
        << "(j3): der Zwilling baut Debug -- die clang-x-Debug-Zelle der SOLL-Matrix";
    EXPECT_NE(ydbg.find("(j3) Aufruf 1/2"), std::string::npos) << "(j3)-Mess-Zweig bleibt UNVERAENDERT";
}

// E4: der bare-metal-Spiegel SAGT die Pins (W2-Muster -- der aeussere Configure baut den Treiber,
// die Emission weist an; beide Wege fahren dieselben Zellen, V-5 "beide literal").
TEST(CompilerPinInvariante, BareMetalSpiegelSagtDiePins) {
    planner::PmcHostBefund const    amd = befund_mit(planner::PmcLage::Amd, "AuthenticAMD");
    planner::ExperimentPlanDirector director;
    director.set_pmc_befund(amd);
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());

    planner::CMakeGraphBuilder s1;
    director.construct(*tp, s1);
    std::size_t const combos = count_occurrences(s1.text(), "# --- measurement_combo ");
    ASSERT_GT(combos, 0u);
    EXPECT_EQ(count_occurrences(s1.text(), "CI-DUAL (Owner 14.08.): aeusserer Configure pinnt "
                                           "-DCMAKE_C_COMPILER=gcc-15 -DCMAKE_CXX_COMPILER=g++-15"),
              combos)
        << "Stufe-1-Spiegel: je ceb:build-Target EINE Pin-Ansage an den Bediener (W2-Muster)";
    EXPECT_EQ(count_occurrences(s1.text(), "clang-22-Zwilling (build-clang) = Bau+Test-Gate der Stufe"), combos);

    planner::TierCmakeGraphBuilder s2;
    director.construct(*tp, s2);
    EXPECT_NE(s2.text().find("# CI-DUAL (Owner 14.08.): Treiber vorgebaut (COMDARE_PLAN_DRIVER); der aeussere "
                             "Configure pinnt gcc-15/g++-15,"),
              std::string::npos)
        << "Stufe-2-Spiegel: Kopf-Ansage der Pins";
    EXPECT_NE(s2.text().find("clang-22-Zwilling (build-clang) ist Bau+Test-Gate je Stufe; Mess-Targets bleiben "
                             "single-compiler (N2)"),
              std::string::npos);
}

// (E-10/C-10, 21.08.2026) WindowBelongsTo-VERDRAHTUNG, Generator-Haelfte (Gen-2-Kampagnenbetrieb KON29-04):
// die Bau-Fenster-Schleife traegt die Paritaets-Zuteilung des Bestandslogs. Topologie kommt aus
// COMDARE_MACHINE_RANK/COMDARE_MACHINES (Default 0/1 = heutiges Verhalten: alle Fenster der Lane-Maschine);
// rank >= maschinen ist FAIL-CLOSED (sonst baute die Maschine STILL nichts und der Batch endete gruen leer).
// Der Mess-Batch traegt BEWUSST keine Fenster-Teilung ("Messung selbst nicht zweilanig" = W3-Vorstaffel).
// Geprueft wird (a) die Emission literal, (b) die SEMANTIK-PARITAET der gespiegelten Shell-Formel gegen
// bestandslog::window_belongs_to selbst (dieselbe Funktion, die BatchPlanner/plan_batch_slices nutzt) und
// (c) die Partitions-Vollstaendigkeit (jedes Fenster gehoert GENAU einer Maschine).
TEST(TierCiYamlBuilder, WindowBelongsToVerdrahtungGeneratorHaelfte) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    // (a) Topologie-Kopf je Build-Batch (2 Host-Lanes), Defaults 0/1 = heutiges Verhalten.
    EXPECT_EQ(count_occurrences(yaml, "RANK=\"${COMDARE_MACHINE_RANK:-0}\"; MASCHINEN=\"${COMDARE_MACHINES:-1}\""), 2u)
        << "je Build-Batch EIN Topologie-Kopf mit den inerten Defaults 0/1";
    EXPECT_EQ(count_occurrences(yaml, "== [FENSTER-TOPOLOGIE] lane="), 2u)
        << "die Topologie wird je Batch LAUT gesagt (rank/maschinen/Semantik-Quelle)";
    // fail-closed: rank >= maschinen bricht VOR der ersten Perm ab -- kein stiller leerer Batch.
    EXPECT_EQ(count_occurrences(yaml, "COMDARE_MACHINE_RANK=$RANK >= COMDARE_MACHINES=$MASCHINEN"), 2u)
        << "je Build-Batch der harte rank>=maschinen-Abbruch (Abbruch statt leerem Gruen)";
    // (b) der Paritaets-Guard je Perm-Fenster-Schleife (4 Perms) -- WOERTLICH die window_belongs_to-Formel.
    EXPECT_EQ(count_occurrences(yaml, "FENSTER=$(( START / SLICE ))"), 4u) << "Fenster-Index im 4096er-Korn je Perm";
    EXPECT_EQ(
        count_occurrences(yaml, "if [ \"$MASCHINEN\" -gt 0 ] && [ $(( FENSTER % MASCHINEN )) -ne \"$RANK\" ]; then"),
        4u)
        << "je Perm der gespiegelte window_belongs_to-Guard ((w % n) != rank -> skip; n == 0 -> alle)";
    EXPECT_EQ(count_occurrences(yaml, "[FENSTER-FREMD]"), 4u)
        << "je Perm die laute Skip-Zeile (BEWUSST ohne TESTAT-Namen und ohne offen= -- zaehlt nie als gebaut)";
    // Der Mess-Batch bleibt fensterteilungs-frei: alle 4 Guards liegen in den 2 Build-Batches (je 2 Perms),
    // und die Mess-Jobs exportieren die Topologie-Variablen nicht.
    EXPECT_EQ(count_occurrences(yaml, "[FENSTER-FREMD] ts=$(date -u +%FT%TZ) lane=amd zelle=[O2,no_extension]"), 1u)
        << "der Guard sitzt in der Perm-Schleife des BUILD-Batches (Stichprobe amd-Perm)";
    // (c) SEMANTIK-PARITAET: die gespiegelte Shell-Formel gegen die Bestandslog-Funktion selbst, inkl. der
    // Raender n==0 (alle) und rank>=n (nie -- genau der fail-closed-Fall der Emission).
    namespace bl = comdare::cache_engine::builder::bestandslog;
    for (unsigned n = 1; n <= 3; ++n) {
        for (unsigned rank = 0; rank < n; ++rank) {
            for (std::uint64_t w = 0; w < 12; ++w) {
                bool const shell_baut = !((w % n) != rank); // der emittierte Guard, in C++ nachgerechnet
                EXPECT_EQ(bl::window_belongs_to(w, rank, n), shell_baut)
                    << "Formel-Divergenz bei w=" << w << " rank=" << rank << " n=" << n;
            }
        }
    }
    EXPECT_TRUE(bl::window_belongs_to(7, 3, 0)) << "n==0 heisst ALLE (beide Seiten; Shell-Guard kurzschliesst)";
    EXPECT_FALSE(bl::window_belongs_to(5, 4, 2)) << "rank>=n hiesse NIE -- exakt der emittierte Abbruchgrund";
    for (std::uint64_t w = 0; w < 32; ++w) {
        int eigentuemer = 0;
        for (unsigned rank = 0; rank < 2; ++rank) eigentuemer += bl::window_belongs_to(w, rank, 2) ? 1 : 0;
        EXPECT_EQ(eigentuemer, 1) << "Partitions-Vollstaendigkeit verletzt bei w=" << w
                                  << " (jedes Fenster gehoert GENAU einer von 2 Maschinen)";
    }
}

// (E-20/R-15, 21.08.2026) PIN-PFLICHT-DEKLARATION im Mess-Emissionspfad: prod1/amd traegt die 96/32-MiB-
// L3-Asymmetrie (2 CCDs; /sys am Objekt gemessen) -- ungepinnt wandert der 1-Thread-Messlauf zwischen den
// CCDs und die Cache-Messwerte sind NICHT reproduzierbar. Die Emission deklariert die SOLL-Menge (CCD0)
// als COMDARE_MEASURE_PIN_CPUS und sagt den IST-Stand EHRLICH dazu (pin_ist=UNGEPINNT, Aktuator
// ScopedThreadPin im run_profile-Loop unverdrahtet = W3); intel bleibt laut UNVERMESSEN statt einer
// erfundenen Maske. KEIN taskset im Batch: dieselbe Invokation baut parallel und misst 1-Thread.
TEST(TierCiYamlBuilder, PinPflichtDeklarationR15ImMessBatch) {
    auto const tp = parse_thesis(COMDARE_PLANNER_THESIS_ALL_AXES);
    ASSERT_TRUE(tp.has_value());
    planner::ExperimentPlanDirector const director;
    planner::TierCiYamlBuilder            tb;
    director.construct(*tp, tb);
    std::string const& yaml = tb.text();

    // amd: SOLL-Menge (CCD0 = 96-MiB-L3-Haelfte, cpu 0-7,16-23) GENAU EINMAL -- nur der amd-MESS-Batch.
    EXPECT_EQ(count_occurrences(yaml, "export COMDARE_MEASURE_PIN_CPUS=\"0-7,16-23\""), 1u)
        << "die SOLL-Pin-Menge steht genau im amd-Mess-Batch (nicht im Bau-Batch, nicht bei intel)";
    EXPECT_NE(yaml.find("[PIN-DEKLARATION] lane=amd pin_soll=0-7,16-23 pin_ist=UNGEPINNT"), std::string::npos)
        << "amd deklariert SOLL und den ehrlichen IST-Stand in EINER Zeile";
    EXPECT_NE(yaml.find("folge=ungepinnt-nicht-reproduzierbar (R-15)"), std::string::npos)
        << "die R-15-Folge (nicht reproduzierbar) steht woertlich in der Deklaration";
    // intel: laut UNVERMESSEN -- keine erfundene Maske fuer eine nicht vermessene Topologie.
    EXPECT_NE(yaml.find("[PIN-DEKLARATION] lane=intel pin_soll=UNVERMESSEN pin_ist=UNGEPINNT"), std::string::npos)
        << "intel deklariert die Luecke, statt eine Maske zu raten";
    EXPECT_EQ(count_occurrences(yaml, "[PIN-DEKLARATION] lane="), 2u) << "genau die 2 Mess-Batches deklarieren";
    // NIE ein falscher Vollzug: pin_ist traegt nirgends die Maske, und kein taskset drosselt den Bau.
    EXPECT_EQ(yaml.find("pin_ist=0-7,16-23"), std::string::npos)
        << "pin_ist darf den SOLL-Wert nicht behaupten, solange der Aktuator unverdrahtet ist";
    EXPECT_EQ(yaml.find("taskset"), std::string::npos)
        << "kein job-weites taskset (der parallele DLL-Bau liefe sonst auf der Pin-Menge)";
    EXPECT_EQ(yaml.find("numactl"), std::string::npos) << "kein numactl-Praefix (prod1 ist 1 NUMA-Node)";
}
