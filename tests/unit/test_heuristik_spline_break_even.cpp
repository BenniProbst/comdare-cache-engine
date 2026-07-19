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

#include "heuristik/axis_spline.hpp"
#include "heuristik/break_even.hpp"
#include "heuristik/measurement_curve_loader.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <string>
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
