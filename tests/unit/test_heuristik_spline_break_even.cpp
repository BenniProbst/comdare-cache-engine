// PAKET W3-C (Ledger Sec.32-F8): Unit-Test des Heuristik-Spline-Vorbaus.
//
// Beweist:
//   (1) F8-Beispiel gross/klein (Allokator A/B): zwei synthetische Performance-Kurven schneiden sich
//       an GENAU EINEM Break-Even; Position + links/rechts-Besser analytisch verifiziert (x=80, y=210).
//   (2) Monotonie-Erhalt: der monotone kubische Hermite-Spline (Fritsch-Carlson) bleibt auf monotonen
//       Daten monoton + beschraenkt; der natuerliche kubische Spline UEBERSCHWINGT auf denselben Daten
//       (numerische Begruendung der Default-Verfahrenswahl in axis_spline.hpp).
//   (3) HONEST-EMPTY: < 2 Stuetzstellen -> kein Spline (std::nullopt).
//   (4) n/a-toleranter Loader (K-10): Mini-CSV-Fixture (im Test generiert) mit "n/a"/"failed"-Zellen ->
//       die betroffenen Zeilen werden GEZAEHLT uebersprungen, NIE zu Phantom-Punkten.
//   (5) T-9 (2026-08-07): der Min/Max-KATALOG speist den Break-Even-Vergleich. Die Bissprobe
//       MaxAxisReportsHigherCurveAsBetter faellt gegen den Alt-Stand ("besser == kleiner", pauschal) --
//       sie ist die eigentliche Regressions-Wache dieses Pakets. Dazu die Katalog-Struktur-Wachen und
//       der Drift-Beleg Katalog <-> kCompositionAxisNames.

#include "heuristik/axis_optimization_catalog.hpp"
#include "heuristik/axis_spline.hpp"
#include "heuristik/break_even.hpp"
#include "heuristik/measurement_curve_loader.hpp"

#include <builder/experiment_tree/axis_path_serialization.hpp> // kCompositionAxisNames (Single-Source, NUR gelesen)

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace h = ::comdare::cache_engine::heuristik;

namespace {

// F8-Beispiel: Allokator A (klein-optimiert) f_A(x)=50+2x; Allokator B (gross-optimiert) f_B(x)=170+0.5x.
// Analytischer Break-Even: 50+2x == 170+0.5x  =>  1.5x = 120  =>  x = 80, y = 210.
std::vector<h::CurveSample> alloc_a_small() {
    std::vector<h::CurveSample> v;
    for (double x : {10.0, 40.0, 70.0, 100.0, 130.0, 160.0}) v.push_back({x, 50.0 + 2.0 * x});
    return v;
}
std::vector<h::CurveSample> alloc_b_large() {
    std::vector<h::CurveSample> v;
    for (double x : {10.0, 40.0, 70.0, 100.0, 130.0, 160.0}) v.push_back({x, 170.0 + 0.5 * x});
    return v;
}

// -- T-9-Fixture: eine MAX-Zielgroesse --------------------------------------------------------------
// Achse T3 path_compression, Zielgroesse "compression_ratio" -> laut Katalog MAXIMIEREN
// (axis_optimization_catalog.hpp; Quelle BEFUND :86 "MAX Kompressionsrate (Single-Child-Kollaps)").
// x = Praefix-Sharing-Anteil der Schluessel (die vom Katalog benannte Abhaengigkeit ":164-165"),
// y = erreichte Kompressionsrate. Beide Kurven sind LINEAR, damit der monotone Spline sie exakt
// reproduziert und der Schnittpunkt analytisch nachrechenbar bleibt:
//   patricia(x) = 1.0 + 0.020 x   (profitiert stark vom Praefix-Sharing)
//   macro(x)    = 3.4 + 0.005 x   (kollabiert ganze Makro-Knoten, startet hoch, waechst flach)
// Schnitt: 1.0 + 0.020x == 3.4 + 0.005x  =>  0.015x = 2.4  =>  x = 160, y = 4.2.
// LINKS von 160 ist macro HOEHER, also unter MAX BESSER; RECHTS ist patricia hoeher/besser.
std::vector<h::CurveSample> pc_patricia_ratio() {
    std::vector<h::CurveSample> v;
    for (double x : {0.0, 60.0, 120.0, 180.0, 240.0, 300.0}) v.push_back({x, 1.0 + 0.020 * x});
    return v;
}
std::vector<h::CurveSample> pc_macro_ratio() {
    std::vector<h::CurveSample> v;
    for (double x : {0.0, 60.0, 120.0, 180.0, 240.0, 300.0}) v.push_back({x, 3.4 + 0.005 * x});
    return v;
}

// Drift-Beleg Katalog <-> Kompositions-Achsen: die beiden Mengen sind BEWUSST nicht deckungsgleich
// (Katalog-Stand 09.07. mit telemetry/isa; Komposition seit STRUKT-R ORG-18 mit persistence_target).
// Diese consteval-Zaehler halten die Rechnung 17 + 2 + 1 fest; jede kuenftige Verschiebung bricht hier
// compile-time, statt still eine Achse ohne Optimierungsrichtung durchlaufen zu lassen.
namespace ex = ::comdare::cache_engine::builder::experiment;

[[nodiscard]] consteval std::size_t composition_names_with_catalog_row() {
    std::size_t n = 0;
    for (std::string_view name : ex::kCompositionAxisNames)
        if (h::catalog_axis_from_name(name).has_value()) ++n;
    return n;
}
[[nodiscard]] consteval std::size_t composition_names_without_catalog_row() {
    return ex::kCompositionAxisNames.size() - composition_names_with_catalog_row();
}
[[nodiscard]] consteval std::size_t catalog_rows_without_composition_slot() {
    std::size_t n = 0;
    for (h::AxisOptimizationInfo const& a : h::kAxisOptimizationCatalog) {
        bool found = false;
        for (std::string_view name : ex::kCompositionAxisNames)
            if (name == a.name) found = true;
        if (!found) ++n;
    }
    return n;
}

} // namespace

// -- (1) Break-Even: genau EIN Schnittpunkt, Position analytisch verifiziert ------------------------
TEST(HeuristikBreakEven, SingleCrossingAnalyticPosition) {
    auto sa = h::MonotoneAxisSpline::build(alloc_a_small());
    auto sb = h::MonotoneAxisSpline::build(alloc_b_large());
    ASSERT_TRUE(sa.has_value());
    ASSERT_TRUE(sb.has_value());

    std::vector<h::BreakEvenPoint> const bes = h::find_break_even_points(*sa, *sb);
    ASSERT_EQ(bes.size(), 1u) << "F8-Beispiel muss GENAU einen Break-Even liefern";

    h::BreakEvenPoint const& p = bes.front();
    EXPECT_NEAR(p.x, 80.0, 1e-4); // linear -> monotoner Spline reproduziert die Geraden exakt
    EXPECT_NEAR(p.y, 210.0, 1e-3);
    // Links vom Break-Even ist A (=f) besser (niedriger), rechts B (=g).
    EXPECT_EQ(p.links_besser, h::Curve::F);
    EXPECT_EQ(p.rechts_besser, h::Curve::G);
}

// Kontroll-Probe: die punktweise Besser-Aussage stimmt mit der Analytik ueberein.
TEST(HeuristikBreakEven, BetterSideMatchesAnalytics) {
    auto sa = h::MonotoneAxisSpline::build(alloc_a_small());
    auto sb = h::MonotoneAxisSpline::build(alloc_b_large());
    ASSERT_TRUE(sa.has_value() && sb.has_value());
    // x=20 < 80 : A niedriger ; x=140 > 80 : B niedriger.
    EXPECT_LT(sa->eval(20.0), sb->eval(20.0));
    EXPECT_GT(sa->eval(140.0), sb->eval(140.0));
}

// Keine Domaenen-Ueberlappung -> ehrlich kein Break-Even.
TEST(HeuristikBreakEven, NoOverlapNoCrossing) {
    auto left  = h::MonotoneAxisSpline::build({{0.0, 1.0}, {10.0, 2.0}});
    auto right = h::MonotoneAxisSpline::build({{100.0, 1.0}, {110.0, 2.0}});
    ASSERT_TRUE(left.has_value() && right.has_value());
    EXPECT_TRUE(h::find_break_even_points(*left, *right).empty());
}

// -- (2) Monotonie-Segment-Test: Fritsch-Carlson erhaelt Monotonie, natuerlicher Spline schwingt ueber -
TEST(HeuristikSpline, MonotonePreservationVsNaturalOvershoot) {
    // Monoton nicht-fallende Stufen-Daten (klassischer Overshoot-Ausloeser fuer natuerliche Splines).
    std::vector<h::CurveSample> const step = {{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, {3.0, 1.0}, {4.0, 1.0}, {5.0, 1.0}};
    auto                              mono = h::MonotoneAxisSpline::build(step);
    auto                              nat  = h::NaturalAxisSpline::build(step);
    ASSERT_TRUE(mono.has_value() && nat.has_value());
    ASSERT_TRUE(h::MonotoneAxisSpline::preserves_monotonicity());
    ASSERT_FALSE(h::NaturalAxisSpline::preserves_monotonicity());

    double prev  = mono->eval(0.0);
    double m_min = prev, m_max = prev;
    double n_min = nat->eval(0.0), n_max = n_min;
    for (int i = 0; i <= 500; ++i) {
        double const x  = 5.0 * static_cast<double>(i) / 500.0;
        double const vm = mono->eval(x);
        double const vn = nat->eval(x);
        // Monotoner Spline: nie fallend (Segment-Monotonie erhalten).
        EXPECT_GE(vm, prev - 1e-9) << "Monotonie-Bruch bei x=" << x;
        prev  = vm;
        m_min = std::min(m_min, vm);
        m_max = std::max(m_max, vm);
        n_min = std::min(n_min, vn);
        n_max = std::max(n_max, vn);
    }
    // Monotoner Spline bleibt im Datenband [0,1] (kein Overshoot).
    EXPECT_GE(m_min, -1e-9);
    EXPECT_LE(m_max, 1.0 + 1e-9);
    // Natuerlicher Spline VERLAESST das Datenband -> genau der Grund, warum Fritsch-Carlson F8-Default ist.
    EXPECT_TRUE(n_min < -1e-3 || n_max > 1.0 + 1e-3)
        << "erwartet: natuerlicher Spline schwingt ueber; min=" << n_min << " max=" << n_max;
}

// -- (3) HONEST-EMPTY: unterbestimmte Daten -> kein Modell ------------------------------------------
TEST(HeuristikSpline, HonestEmptyBelowTwoPoints) {
    EXPECT_FALSE(h::MonotoneAxisSpline::build({}).has_value());
    EXPECT_FALSE(h::MonotoneAxisSpline::build({{5.0, 1.0}}).has_value());
    // Zwei Samples mit gleichem x kollabieren auf einen Knoten -> unterbestimmt.
    EXPECT_FALSE(h::MonotoneAxisSpline::build({{5.0, 1.0}, {5.0, 2.0}}).has_value());
}

// -- (4) n/a-toleranter Loader mit Mini-CSV-Fixture (synthetisch im Test generiert) -----------------
TEST(HeuristikLoader, NaTolerantSkipsNeverPhantom) {
    // WIDE-Dialekt (Semikolon), reale Spaltennamen (sweep_axis;binary_id;workload;working_set_n;ns_per_op).
    std::string const  csv = "sweep_axis;binary_id;workload;working_set_n;ns_per_op\n"
                             "allocator;A;ycsb_a;10;70\n"
                             "allocator;A;ycsb_a;20;90\n"
                             "allocator;A;ycsb_a;30;n/a\n" // K-10: n/a -> uebersprungen
                             "allocator;B;ycsb_a;10;175\n"
                             "allocator;B;ycsb_a;20;failed\n" // K-10: failed -> uebersprungen
                             "allocator;B;ycsb_a;30;185\n";
    std::istringstream is(csv);
    auto const         groups = h::load_curves(is, h::wide_lazy_spec());

    ASSERT_EQ(groups.size(), 2u);
    h::GroupKey const key_a{"allocator", "A", "ycsb_a"};
    h::GroupKey const key_b{"allocator", "B", "ycsb_a"};
    ASSERT_TRUE(groups.count(key_a) == 1);
    ASSERT_TRUE(groups.count(key_b) == 1);

    auto const& sa = groups.at(key_a);
    auto const& sb = groups.at(key_b);
    EXPECT_EQ(sa.samples.size(), 2u); // (10,70),(20,90) -- die n/a-Zeile fiel weg
    EXPECT_EQ(sa.skipped_rows, 1u);
    EXPECT_EQ(sb.samples.size(), 2u); // (10,175),(30,185) -- die failed-Zeile fiel weg
    EXPECT_EQ(sb.skipped_rows, 1u);
    // KEIN Phantom-Punkt: keine (x,0)-Zelle aus den n/a/failed-Zeilen.
    for (auto const& s : sa.samples) EXPECT_NE(s.y, 0.0);
    for (auto const& s : sb.samples) EXPECT_NE(s.y, 0.0);
}

// Fehlende Pflicht-Spalte -> ehrlich leeres Ergebnis (kein Absturz).
TEST(HeuristikLoader, MissingRequiredColumnHonestEmpty) {
    std::string const  csv = "sweep_axis;binary_id;workload;ns_per_op\n" // kein working_set_n (x-Spalte)
                             "allocator;A;ycsb_a;70\n";
    std::istringstream is(csv);
    EXPECT_TRUE(h::load_curves(is, h::wide_lazy_spec()).empty());
}

// build_axis_splines schliesst Gruppen mit < 2 verwertbaren Punkten aus (HONEST-EMPTY end-to-end).
TEST(HeuristikLoader, BuildSplinesDropsUnderdeterminedGroups) {
    std::string const  csv = "sweep_axis;binary_id;workload;working_set_n;ns_per_op\n"
                             "allocator;A;ycsb_a;10;70\n"
                             "allocator;A;ycsb_a;20;90\n"
                             "allocator;C;ycsb_a;10;100\n"     // nur 1 gueltiger Punkt ...
                             "allocator;C;ycsb_a;20;failed\n"; // ... + ein failed -> Gruppe C unterbestimmt
    std::istringstream is(csv);
    auto const         splines = h::build_axis_splines<>(is, h::wide_lazy_spec());
    EXPECT_EQ(splines.size(), 1u); // nur Gruppe A ueberlebt
    EXPECT_TRUE(splines.count(h::GroupKey{"allocator", "A", "ycsb_a"}) == 1);
    EXPECT_TRUE(splines.count(h::GroupKey{"allocator", "C", "ycsb_a"}) == 0);
}

// -- (5) T-9: DIE BISSPROBE -- fuer eine MAX-Zielgroesse gewinnt die HOEHERE Kurve ------------------
// DIESER TEST FAELLT GEGEN DEN ALT-STAND. Vor T-9 stand in break_even.hpp die pauschale Konvention
// "besser == kleinerer y-Wert" fuer ALLE Achsen; better_at() kannte keine Richtung. Damit haette der
// Finder hier links_besser=F/rechts_besser=G gemeldet -- also fuer eine MAX-Groesse (Kompressionsrate)
// genau die SCHLECHTERE, weil niedrigere Kurve als die bessere. Die Erwartungen unten sind die exakte
// Umkehrung davon; ein Ruecksturz auf die pauschale Konvention bricht sie sofort.
TEST(HeuristikBreakEvenT9, MaxAxisReportsHigherCurveAsBetter) {
    auto patricia = h::MonotoneAxisSpline::build(pc_patricia_ratio());
    auto macro    = h::MonotoneAxisSpline::build(pc_macro_ratio());
    ASSERT_TRUE(patricia.has_value());
    ASSERT_TRUE(macro.has_value());

    // Die Richtung kommt COMPILE-TIME aus dem Katalog, nicht aus einer Annahme im Test.
    constexpr h::OptimizationDirection kDir =
        h::objective_direction(h::CatalogAxis::PathCompression, "compression_ratio");
    static_assert(kDir == h::OptimizationDirection::Maximize,
                  "T3 path_compression / compression_ratio ist laut Katalog eine MAX-Groesse (BEFUND :86).");

    std::vector<h::BreakEvenPoint> const bes = h::find_break_even_points<kDir>(*patricia, *macro);
    ASSERT_EQ(bes.size(), 1u) << "genau ein Break-Even erwartet (analytisch x=160)";

    h::BreakEvenPoint const& p = bes.front();
    EXPECT_NEAR(p.x, 160.0, 1e-4);
    EXPECT_NEAR(p.y, 4.2, 1e-3);

    // Kontroll-Rechnung am Objekt: LINKS ist macro (=g) hoeher, RECHTS ist patricia (=f) hoeher.
    ASSERT_GT(macro->eval(60.0), patricia->eval(60.0));
    ASSERT_GT(patricia->eval(260.0), macro->eval(260.0));

    // ... also ist unter MAXIMIZE links g besser und rechts f besser. Der Alt-Stand haette F/G gemeldet.
    EXPECT_EQ(p.links_besser, h::Curve::G) << "MAX-Groesse: links liegt macro hoeher -> macro ist besser";
    EXPECT_EQ(p.rechts_besser, h::Curve::F) << "MAX-Groesse: rechts liegt patricia hoeher -> patricia ist besser";
}

// Gegenprobe an DENSELBEN Daten: unter MINIMIZE dreht sich die Etikettierung exakt um -- und die
// Schnittpunkt-MENGE bleibt identisch. Das trennt "Richtung wirkt" sauber von "Mathematik veraendert".
TEST(HeuristikBreakEvenT9, DirectionFlipsOnlyTheLabelNotTheCrossing) {
    auto patricia = h::MonotoneAxisSpline::build(pc_patricia_ratio());
    auto macro    = h::MonotoneAxisSpline::build(pc_macro_ratio());
    ASSERT_TRUE(patricia.has_value() && macro.has_value());

    auto const as_max = h::find_break_even_points<h::OptimizationDirection::Maximize>(*patricia, *macro);
    auto const as_min = h::find_break_even_points<h::OptimizationDirection::Minimize>(*patricia, *macro);
    ASSERT_EQ(as_max.size(), 1u);
    ASSERT_EQ(as_min.size(), 1u);
    EXPECT_DOUBLE_EQ(as_max.front().x, as_min.front().x);
    EXPECT_DOUBLE_EQ(as_max.front().y, as_min.front().y);
    EXPECT_EQ(as_max.front().links_besser, h::Curve::G);
    EXPECT_EQ(as_min.front().links_besser, h::Curve::F);
    EXPECT_EQ(as_max.front().rechts_besser, h::Curve::F);
    EXPECT_EQ(as_min.front().rechts_besser, h::Curve::G);
}

// Die Katalog-Abkuerzung ueber die FUEHRENDE Zielgroesse liefert fuer T3 dasselbe MAX-Ergebnis --
// hier faehrt der Aufrufer NUR die Achse, die Richtung zieht der Katalog selbststaendig.
TEST(HeuristikBreakEvenT9, AxisShortcutUsesCatalogLeadingDirection) {
    auto patricia = h::MonotoneAxisSpline::build(pc_patricia_ratio());
    auto macro    = h::MonotoneAxisSpline::build(pc_macro_ratio());
    ASSERT_TRUE(patricia.has_value() && macro.has_value());
    static_assert(h::leading_direction(h::CatalogAxis::PathCompression) == h::OptimizationDirection::Maximize);

    auto const bes = h::find_break_even_points_of_axis<h::CatalogAxis::PathCompression>(*patricia, *macro);
    ASSERT_EQ(bes.size(), 1u);
    EXPECT_EQ(bes.front().links_besser, h::Curve::G);
    EXPECT_EQ(bes.front().rechts_besser, h::Curve::F);
}

// Die alte, defaultete Aufruf-Form bleibt quellkompatibel UND semantisch korrekt: T0 search_algo faehrt
// eine MIN-Groesse (Lookup-Latenz), und genau das ist der Default. Die F8-Allokator-Kurven von oben
// bleiben damit unveraendert gueltig (Test 1).
TEST(HeuristikBreakEvenT9, MinAxisKeepsLowerCurveAsBetter) {
    auto sa = h::MonotoneAxisSpline::build(alloc_a_small());
    auto sb = h::MonotoneAxisSpline::build(alloc_b_large());
    ASSERT_TRUE(sa.has_value() && sb.has_value());
    static_assert(h::leading_direction(h::CatalogAxis::SearchAlgo) == h::OptimizationDirection::Minimize);

    auto const via_axis = h::find_break_even_points_of_axis<h::CatalogAxis::SearchAlgo>(*sa, *sb);
    auto const via_dflt = h::find_break_even_points(*sa, *sb); // Alt-Form, ohne Richtungs-Argument
    ASSERT_EQ(via_axis.size(), 1u);
    ASSERT_EQ(via_dflt.size(), 1u);
    EXPECT_EQ(via_axis.front().links_besser, h::Curve::F);
    EXPECT_EQ(via_axis.front().rechts_besser, h::Curve::G);
    EXPECT_EQ(via_dflt.front().links_besser, via_axis.front().links_besser);
    EXPECT_EQ(via_dflt.front().rechts_besser, via_axis.front().rechts_besser);
}

// -- (5b) Katalog-Struktur: die Richtung haengt an der ZIELGROESSE, nicht an der Achse ---------------
TEST(AxisOptimizationCatalog, DirectionIsPerObjectiveNotPerAxis) {
    // Dieselbe Achse T3 traegt eine MAX- UND zwei MIN-Groessen -- der Beleg, dass eine Tabelle
    // "Achse -> eine Richtung" den Katalog verfaelscht haette.
    static_assert(h::objective_direction(h::CatalogAxis::PathCompression, "compression_ratio") ==
                  h::OptimizationDirection::Maximize);
    static_assert(h::objective_direction(h::CatalogAxis::PathCompression, "tree_height") ==
                  h::OptimizationDirection::Minimize);
    static_assert(h::objective_direction(h::CatalogAxis::PathCompression, "memory_footprint") ==
                  h::OptimizationDirection::Minimize);
    EXPECT_EQ(h::objectives_of(h::CatalogAxis::PathCompression).size(), 3u);

    // Stichproben quer durch die MAX-Achsen des Befunds (T5/T6/T8/T9/T12/T14).
    static_assert(h::objective_direction(h::CatalogAxis::MemoryLayout, "cache_line_utilization") ==
                  h::OptimizationDirection::Maximize);
    static_assert(h::objective_direction(h::CatalogAxis::Allocator, "alloc_throughput") ==
                  h::OptimizationDirection::Maximize);
    static_assert(h::objective_direction(h::CatalogAxis::Concurrency, "multicore_throughput") ==
                  h::OptimizationDirection::Maximize);
    static_assert(h::objective_direction(h::CatalogAxis::Serialization, "compression_ratio") ==
                  h::OptimizationDirection::Maximize);
    static_assert(h::objective_direction(h::CatalogAxis::Isa, "ipc") == h::OptimizationDirection::Maximize);
    static_assert(h::objective_direction(h::CatalogAxis::IoDispatch, "io_throughput") ==
                  h::OptimizationDirection::Maximize);
    // ... und die MIN-Gegenstuecke DERSELBEN Achsen.
    static_assert(h::objective_direction(h::CatalogAxis::MemoryLayout, "memory_footprint") ==
                  h::OptimizationDirection::Minimize);
    static_assert(h::objective_direction(h::CatalogAxis::Allocator, "p99_latency") ==
                  h::OptimizationDirection::Minimize);
    static_assert(h::objective_direction(h::CatalogAxis::Isa, "branch_misses") == h::OptimizationDirection::Minimize);
    SUCCEED();
}

TEST(AxisOptimizationCatalog, CountsMatchTheBefundTable) {
    ASSERT_EQ(h::kAxisOptimizationCatalog.size(), 19u);
    ASSERT_EQ(h::kAxisObjectives.size(), 45u);

    std::size_t max_objectives = 0, min_objectives = 0, axes_with_max = 0, pareto_axes = 0, objective_total = 0;
    h::for_each_axis_objective([&](h::AxisObjective const& o) {
        if (o.direction == h::OptimizationDirection::Maximize)
            ++max_objectives;
        else
            ++min_objectives;
    });
    h::for_each_catalog_axis([&](h::AxisOptimizationInfo const& a) {
        objective_total += a.objective_count;
        if (a.pareto) ++pareto_axes;
        bool has_max = false;
        for (h::AxisObjective const& o : h::objectives_of(a.axis))
            if (o.direction == h::OptimizationDirection::Maximize) has_max = true;
        if (has_max) ++axes_with_max;
    });

    EXPECT_EQ(max_objectives, 17u); // BEFUND :83-:101
    EXPECT_EQ(min_objectives, 28u);
    EXPECT_EQ(objective_total, h::kAxisObjectiveCount);
    EXPECT_EQ(axes_with_max, 15u) << "15 von 19 Achsen tragen eine MAX-Groesse -- der Befund hinter T-9";
    EXPECT_EQ(pareto_axes, 3u);
}

// Pareto-Sonderbehandlung: fuer T5/T6/T18 verweigert die Achsen-Abkuerzung den Dienst (static_assert in
// find_break_even_points_of_axis). Hier wird das Praedikat selbst festgenagelt -- der Compile-Fehler-Pfad
// laesst sich in einem laufenden Test nicht ausloesen, das Praedikat schon.
TEST(AxisOptimizationCatalog, ParetoAxesDemandAnExplicitObjective) {
    static_assert(h::requires_explicit_objective(h::CatalogAxis::MemoryLayout));
    static_assert(h::requires_explicit_objective(h::CatalogAxis::Allocator));
    static_assert(h::requires_explicit_objective(h::CatalogAxis::QueuingQ2));
    static_assert(!h::requires_explicit_objective(h::CatalogAxis::PathCompression));
    static_assert(!h::requires_explicit_objective(h::CatalogAxis::SearchAlgo));
    // Auch auf einer Pareto-Achse ist die Richtung je Zielgroesse eindeutig -- nur EINE davon zu waehlen
    // waere die Falschaussage, nicht die Tabelle.
    static_assert(h::objective_direction(h::CatalogAxis::QueuingQ2, "batching") == h::OptimizationDirection::Maximize);
    static_assert(h::objective_direction(h::CatalogAxis::QueuingQ2, "write_amplification") ==
                  h::OptimizationDirection::Minimize);
    SUCCEED();
}

// Unbekannte Zielgroesse -> honest-empty (weiche Suche). Die HARTE Form (objective_direction) ist
// consteval und wuerde hier gar nicht erst uebersetzen -- genau so ist sie gemeint.
TEST(AxisOptimizationCatalog, UnknownObjectiveIsHonestEmptyNeverAGuess) {
    EXPECT_FALSE(h::find_objective(h::CatalogAxis::PathCompression, "throughput").has_value());
    EXPECT_TRUE(h::find_objective(h::CatalogAxis::PathCompression, "compression_ratio").has_value());
    // T0 misst laut Katalog-Spalte auch THR, nennt dafuer aber KEINE eigene Min/Max-Groesse in der
    // Uebersichtstabelle -> hier steht bewusst nichts, statt eine Richtung zu erfinden.
    EXPECT_FALSE(h::find_objective(h::CatalogAxis::SearchAlgo, "throughput").has_value());
}

// -- (5c) Drift-Beleg: Katalog-Achsen vs. Kompositions-Achsen ---------------------------------------
// 17 gemeinsame Namen + 2 nur im Katalog (telemetry/isa, inzwischen System-Achsen) + 1 nur in der
// Komposition (persistence_target, ohne Katalog-Zeile -> OFFENER Owner-Punkt, keine geratene Richtung).
TEST(AxisOptimizationCatalog, JoinAgainstCompositionAxesIsExplicit) {
    static_assert(ex::kCompositionAxisNames.size() == 18u);
    static_assert(composition_names_with_catalog_row() == 17u,
                  "17 der 18 Kompositions-Achsen haben eine Katalog-Zeile.");
    static_assert(composition_names_without_catalog_row() == 1u,
                  "genau EINE Kompositions-Achse ohne Katalog-Zeile: persistence_target (Owner-Entscheid offen).");
    static_assert(catalog_rows_without_composition_slot() == 2u,
                  "genau ZWEI Katalog-Zeilen ohne Kompositions-Slot: telemetry (T10) und isa (T12).");

    EXPECT_FALSE(h::catalog_axis_from_name("persistence_target").has_value())
        << "persistence_target hat KEINE Katalog-Zeile -- honest-empty statt geratener Richtung";
    EXPECT_TRUE(h::catalog_axis_from_name("telemetry").has_value());
    EXPECT_TRUE(h::catalog_axis_from_name("isa").has_value());
    EXPECT_FALSE(h::catalog_axis_from_name("gibt_es_nicht").has_value());

    // Jede Kompositions-Achse mit Katalog-Zeile traegt eine Richtung -- keine stumme Luecke dazwischen.
    std::size_t mapped = 0;
    for (std::string_view name : ex::kCompositionAxisNames) {
        auto const axis = h::catalog_axis_from_name(name);
        if (!axis.has_value()) continue;
        ++mapped;
        EXPECT_FALSE(h::objectives_of(*axis).empty()) << "Achse ohne Zielgroesse: " << name;
        EXPECT_EQ(h::catalog_axis_info(*axis).name, name);
    }
    EXPECT_EQ(mapped, 17u);
}
