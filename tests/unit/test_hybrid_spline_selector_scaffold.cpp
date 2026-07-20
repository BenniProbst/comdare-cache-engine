// Hybrid-Break-Even-Vorbau (S1/A5, Section 32-F8 / Section 49): SYNTHETISCHE Verifikation des Spline-Fit-
// Geruests (curve_fit.hpp AxisSpline), des self-contained Break-Even-/Selektor-Geruests (best_binary_
// selector.hpp) und des Entscheidungsbaums (decision_lambda_trees.hpp). GOLDEN-NEUTRAL: rein synthetische
// Kurven, KEINE #156-Messdaten, kein binary_id/CRC/golden-Bezug. Honest-Empty-Doktrin wird mitgeprueft.
//
// K-5 (Daten-Korrektheit): zusaetzliches Paritaets-Gate am NEUEN Selektor-Rand — der self-contained ABI-
// Spiegel des Tools MUSS mit dem autoritativen Decl-Header uebereinstimmen (sonst falsche Manifest-
// Provenienz beim Versand realer Saetze in S7). Der Vergleich lebt compile-time HIER (Tool bleibt engine-
// include-frei). Relative Includes (Decl-Schliessung ist relativ/std-only) -> keine tests-CMake-Sonderpfade.

#include "../../libs/cache_engine/builder/curve_fit/curve_fit.hpp"
#include "../../libs/cache_engine/builder/decision_lambda_trees/decision_lambda_trees.hpp"

#include "best_binary_selector.hpp"

#include "../../libs/cache_engine/include/cache_engine/abi/anatomy_module_abi_v1_decl.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace cf  = ::comdare::cache_engine::builder::curve_fit;
namespace dlt = ::comdare::cache_engine::builder::decision_lambda_trees;
namespace bb  = ::comdare::cache_engine::best_binary;

// ── K-5-Paritaets-Gate (compile-time): Selektor-Spiegel == autoritativer Host-ABI (anatomy_module_abi_v1) ──
static_assert(bb::kAbiMajor == COMDARE_ANATOMY_ABI_MAJOR,
              "K-5: kAbiMajor-Spiegel (best_binary_selector.hpp) driftet vom Decl-Header -- syncen!");
static_assert(bb::kAbiMinor == COMDARE_ANATOMY_ABI_MINOR,
              "K-5: kAbiMinor-Spiegel (best_binary_selector.hpp) driftet vom Decl-Header -- syncen!");
static_assert(bb::kAbiMagic == COMDARE_ANATOMY_ABI_MAGIC,
              "K-5: kAbiMagic-Spiegel (best_binary_selector.hpp) driftet vom Decl-Header -- syncen!");

namespace {

int g_fail = 0;

void check(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

// Kurve aus (log2-)linearer Funktion y = slope*log2(x) + off ueber die gegebenen Working-Set-Groessen.
cf::MeasurementCurve line_curve(std::vector<std::uint64_t> const& xs, double slope, double off) {
    cf::MeasurementCurve c;
    for (std::uint64_t x : xs) c.points.push_back({x, slope * std::log2(static_cast<double>(x)) + off, 1});
    return c;
}

} // namespace

int main() {
    std::cout << "==== Hybrid-Break-Even-Vorbau: Spline + Selektor + Decision-Tree (synthetisch) ====\n";

    // ── (1) AxisSpline honest-empty (wie fit_log_linear) ──────────────────────────────────────────
    check("Spline: 0 Punkte => NoData",
          cf::fit_natural_cubic_spline(cf::MeasurementCurve{}).status == cf::FitStatus::NoData);
    {
        cf::MeasurementCurve one;
        one.points.push_back({1024, 1.0, 1});
        check("Spline: 1 Punkt => InsufficientPoints",
              cf::fit_natural_cubic_spline(one).status == cf::FitStatus::InsufficientPoints);
        cf::MeasurementCurve zero_x;
        zero_x.points.push_back({0, 5.0, 1});
        zero_x.points.push_back({1024, 10.0, 1});
        check("Spline: x==0 => InvalidData (NIE Ok/NaN)",
              cf::fit_natural_cubic_spline(zero_x).status == cf::FitStatus::InvalidData);
    }

    // ── (2) AxisSpline reproduziert eine kollineare (log2-)Gerade EXAKT (m2==0) ────────────────────
    {
        cf::AxisSpline const s =
            cf::fit_natural_cubic_spline(line_curve({1u << 10, 1u << 14, 1u << 17, 1u << 23}, 2.0, 3.0));
        check("Spline: 4-Punkte-Gerade => Ok+valid", s.status == cf::FitStatus::Ok && s.valid());
        check("Spline: eval am Knoten 2^17 == 2*17+3 == 37", std::fabs(s.eval(1u << 17) - 37.0) < 1e-9);
        check("Spline: eval interpoliert 2^12 == 27 (kollinear exakt)", std::fabs(s.eval(1u << 12) - 27.0) < 1e-9);
        check("Spline: eval extrapoliert 2^25 == 53 (lineare Rand-Steigung)",
              std::fabs(s.eval(1u << 25) - 53.0) < 1e-9);
    }

    // ── (3) AxisSpline interpoliert nicht-lineare Stuetzstellen exakt AN den Knoten ────────────────
    {
        cf::MeasurementCurve parab; // y = (log2(x) - 14)^2, konvex
        for (std::uint64_t x : {1u << 10, 1u << 12, 1u << 14, 1u << 16, 1u << 18}) {
            double const xl = std::log2(static_cast<double>(x));
            parab.points.push_back({x, (xl - 14.0) * (xl - 14.0), 1});
        }
        cf::AxisSpline const s = cf::fit_natural_cubic_spline(parab);
        check("Spline: konvexe 5-Punkte-Kurve => Ok", s.status == cf::FitStatus::Ok);
        check("Spline: eval am Scheitel-Knoten 2^14 == 0 (Interpolation)", std::fabs(s.eval(1u << 14)) < 1e-9);
        check("Spline: eval am Rand-Knoten 2^10 == 16 (Interpolation)", std::fabs(s.eval(1u << 10) - 16.0) < 1e-9);
    }

    // ── (4) spline_intersections = Break-Even (Section 32-F8): zwei Geraden schneiden sich einmal ──
    {
        cf::AxisSpline const a =
            cf::fit_natural_cubic_spline(line_curve({1u << 5, 1u << 10, 1u << 15, 1u << 20}, 1.0, 0.0));
        cf::AxisSpline const b =
            cf::fit_natural_cubic_spline(line_curve({1u << 5, 1u << 10, 1u << 15, 1u << 20}, -1.0, 20.0));
        auto const ix = cf::spline_intersections(a, b);
        check("Break-Even(Spline): genau 1 Schnittpunkt", ix.size() == 1);
        check("Break-Even(Spline): x == 2^10 == 1024 (log2-Kreuzung bei 10)",
              ix.size() == 1 && std::fabs(ix[0].x_working_set_bytes - 1024.0) < 1.0);
        check("Break-Even(Spline): gemeinsamer Wert y == 10", ix.size() == 1 && std::fabs(ix[0].y - 10.0) < 1e-6);
        check("Break-Even(Spline): kein Ueberlapp => leer", cf::spline_intersections(a, cf::AxisSpline{}).empty());
    }

    // ── (5) PiecewiseCurve honest-empty + lineare eval (self-contained Tool-Modell) ────────────────
    {
        check("Piecewise: 0 Stuetzstellen => !valid", !bb::PiecewiseCurve{}.valid());
        check("Piecewise: 1 Stuetzstelle => !valid + eval NaN",
              !bb::PiecewiseCurve{std::vector<bb::CurveSample>{{5.0, 1.0}}}.valid() &&
                  std::isnan(bb::PiecewiseCurve{std::vector<bb::CurveSample>{{5.0, 1.0}}}.eval(5.0)));
        bb::PiecewiseCurve const c{std::vector<bb::CurveSample>{{0.0, 0.0}, {100.0, 100.0}}};
        check("Piecewise: valid + eval(50)==50 (linear)", c.valid() && std::fabs(c.eval(50.0) - 50.0) < 1e-12);
        check("Piecewise: eval extrapoliert links eval(-10)==-10", std::fabs(c.eval(-10.0) + 10.0) < 1e-12);
    }

    // ── (6) find_break_evens: zwei Kandidaten-Kostenkurven kreuzen sich einmal ─────────────────────
    bb::AlgoSetCandidate cand_a{
        "cand_a", "algo_set_a",
        bb::PiecewiseCurve{std::vector<bb::CurveSample>{{0.0, 0.0}, {100.0, 100.0}}}}; // cost = x (steigt)
    bb::AlgoSetCandidate cand_b{
        "cand_b", "algo_set_b",
        bb::PiecewiseCurve{std::vector<bb::CurveSample>{{0.0, 100.0}, {100.0, 0.0}}}}; // cost = 100-x (faellt)
    {
        auto const be = bb::find_break_evens(cand_a, cand_b);
        check("Break-Even(Selektor): genau 1 Punkt", be.size() == 1);
        check("Break-Even(Selektor): x == 50", be.size() == 1 && std::fabs(be[0].x - 50.0) < 1e-6);
        check("Break-Even(Selektor): cost == 50", be.size() == 1 && std::fabs(be[0].cost - 50.0) < 1e-6);
        check("Break-Even(Selektor): unterhalb fuehrt cand_a, oberhalb cand_b",
              be.size() == 1 && be[0].winner_below == "cand_a" && be[0].winner_above == "cand_b");
    }

    // ── (7) HybridBinarySelector: Rueckwaerts-Wahl links/rechts des Break-Even + Tabelle ───────────
    {
        bb::HybridBinarySelector const sel{std::vector<bb::AlgoSetCandidate>{cand_a, cand_b}};
        auto const                     lo = sel.select(25.0);
        auto const                     hi = sel.select(75.0);
        check("Selektor: query 25 => cand_a (kleinere Kosten)", lo.has_value() && lo->binary_id == "cand_a");
        check("Selektor: query 25 traegt Algo-Satz", lo.has_value() && lo->algo_set == "algo_set_a");
        check("Selektor: query 75 => cand_b", hi.has_value() && hi->binary_id == "cand_b");
        auto const table = sel.break_even_table(0.0, 100.0);
        check("Selektor: Break-Even-Tabelle = 1 Fuehrungswechsel", table.size() == 1);
        check("Selektor: Wechsel bei x==50 (cand_a -> cand_b)",
              table.size() == 1 && std::fabs(table[0].x - 50.0) < 1e-3 && table[0].winner_below == "cand_a" &&
                  table[0].winner_above == "cand_b");
        bb::HybridBinarySelector const empty_sel{std::vector<bb::AlgoSetCandidate>{
            {"only", "a", bb::PiecewiseCurve{std::vector<bb::CurveSample>{{5.0, 1.0}}}}}};
        check("Selektor: nur ungueltige Kurve => select == nullopt (honest-empty)", !empty_sel.select(5.0).has_value());
    }

    // ── (8) DecisionLambdaTree (curve_fit-Spline-basiert): decide + thresholds ─────────────────────
    {
        dlt::SplineCandidate sc_a{
            "sp_small", "algo_a",
            cf::fit_natural_cubic_spline(line_curve({1u << 5, 1u << 10, 1u << 15, 1u << 20}, 1.0, 0.0))};
        dlt::SplineCandidate sc_b{
            "sp_large", "algo_b",
            cf::fit_natural_cubic_spline(line_curve({1u << 5, 1u << 10, 1u << 15, 1u << 20}, -1.0, 20.0))};
        dlt::DecisionLambdaTree const tree{std::vector<dlt::SplineCandidate>{sc_a, sc_b}};
        check("Decision-Tree: 2 Kandidaten", tree.candidate_count() == 2);
        check("Decision-Tree: decide(2^7) => sp_small (unterhalb Break-Even)", tree.decide(1u << 7) == "sp_small");
        check("Decision-Tree: decide(2^15) => sp_large (oberhalb Break-Even)", tree.decide(1u << 15) == "sp_large");
        auto const th = tree.thresholds();
        check("Decision-Tree: 1 Schwelle aus Spline-Schnittpunkt", th.size() == 1);
        check("Decision-Tree: Schwelle x==1024, below=sp_small, above=sp_large",
              th.size() == 1 && std::fabs(th[0].x_working_set_bytes - 1024.0) < 1.0 && th[0].below_id == "sp_small" &&
                  th[0].above_id == "sp_large");
    }

    // ── (9) Monotone Stufen-Daten => KEIN Schein-Break-Even (spiegelt heuristik MonotonePreservation...) ──
    // Klassischer Overshoot-Ausloeser: nicht-fallende Stufen. In der log2-Abszisse liegen die Knoten bei
    // {0,1,2,3,4,5} (Working-Sets 2^0..2^5) mit y {0,0,0,1,1,1} -- IDENTISCH zur heuristik-Assertion
    // (test_heuristik_spline_break_even.cpp MonotonePreservationVsNaturalOvershoot), nur in der builder-
    // Familie (curve_fit.hpp). Der monotone Fit (Fritsch-Carlson) bleibt im Datenband [0,1] und erzeugt
    // KEINE Schein-Schnitte; der natuerliche Fit UEBERSCHWINGT und erzeugt sie.
    {
        std::vector<std::uint64_t> const xs_step = {1u << 0, 1u << 1, 1u << 2, 1u << 3, 1u << 4, 1u << 5};
        std::vector<double> const        ys_step = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
        cf::MeasurementCurve             step;
        for (std::size_t k = 0; k < xs_step.size(); ++k) step.points.push_back({xs_step[k], ys_step[k], 1});

        cf::AxisSpline const mono = cf::fit_monotone_cubic_spline(step);
        cf::AxisSpline const nat  = cf::fit_natural_cubic_spline(step);
        check("Stufen: monotoner Fit => Ok+valid", mono.status == cf::FitStatus::Ok && mono.valid());
        check("Stufen: natuerlicher Fit => Ok+valid", nat.status == cf::FitStatus::Ok && nat.valid());
        check("Stufen: monotoner Fit traegt MonotoneHermite-Auswertung",
              mono.interp == cf::AxisSpline::Interp::MonotoneHermite);
        check("Stufen: natuerlicher Fit bleibt NaturalCubic (Default unveraendert)",
              nat.interp == cf::AxisSpline::Interp::NaturalCubic);

        // Dichte Abtastung ueber log2 [0,5]: Monotonie-Erhalt + Band-Containment (monotoner Fit),
        // Band-Verlassen (natuerlicher Fit).
        double prev  = mono.eval_log2(0.0);
        double m_min = prev, m_max = prev;
        double n_min = nat.eval_log2(0.0), n_max = n_min;
        bool   mono_nondecreasing = true;
        for (int s = 0; s <= 500; ++s) {
            double const xl = 5.0 * static_cast<double>(s) / 500.0;
            double const vm = mono.eval_log2(xl);
            double const vn = nat.eval_log2(xl);
            if (vm < prev - 1e-9) mono_nondecreasing = false;
            prev  = vm;
            m_min = std::min(m_min, vm);
            m_max = std::max(m_max, vm);
            n_min = std::min(n_min, vn);
            n_max = std::max(n_max, vn);
        }
        check("Stufen: monotoner Fit ist nicht-fallend (Segment-Monotonie erhalten)", mono_nondecreasing);
        check("Stufen: monotoner Fit bleibt im Datenband [0,1] (kein Overshoot)",
              m_min >= -1e-9 && m_max <= 1.0 + 1e-9);
        check("Stufen: natuerlicher Fit VERLAESST das Band (Ueberschwingen -> Grund fuer Fritsch-Carlson)",
              n_min < -1e-3 || n_max > 1.0 + 1e-3);

        // Schein-Break-Even: Referenz-Konstante c ECHT im Ueberschwing-Bereich (1 < c < n_max, aus dem
        // gemessenen Overshoot abgeleitet -> kein Magic-Threshold). Der monotone Fit bleibt <= 1 < c => 0
        // Schnitte; der natuerliche Fit schwingt ueber c und faellt zurueck => >= 2 SCHEIN-Schnitte.
        double const         c        = 1.0 + (n_max - 1.0) * 0.5;
        cf::AxisSpline const flat_ref = cf::fit_monotone_cubic_spline(line_curve(xs_step, 0.0, c)); // y == c
        auto const           ix_mono  = cf::spline_intersections(mono, flat_ref);
        auto const           ix_nat   = cf::spline_intersections(nat, flat_ref);
        check("Schein-Break-Even: monotoner Fit => 0 Schnitte oberhalb des Bandes (kein Schein)", ix_mono.empty());
        check("Schein-Break-Even: natuerlicher Fit => >= 2 Schein-Schnitte (Ueberschwingen ueber c)",
              ix_nat.size() >= 2);

        // Entscheidungsbaum aus ROH-Kurven (Section-32-F8 Schritt 3): Default-Fit ist der monotone -> keine
        // Schein-Schwellen; das explizit durchgereichte natuerliche Fit-Verfahren erzeugt sie sehr wohl.
        dlt::DecisionLambdaTree const tree_mono = dlt::DecisionLambdaTree::from_curves(std::vector<dlt::CurveCandidate>{
            {"mono_bin", "algo_mono", step}, {"flat_bin", "algo_flat", line_curve(xs_step, 0.0, c)}});
        check("from_curves: Default-Fit modelliert 2 Kandidaten (monoton)", tree_mono.candidate_count() == 2u);
        check("from_curves: monotone Modelle => KEINE Schein-Schwellen ueber der Referenz",
              tree_mono.thresholds().empty());
        dlt::DecisionLambdaTree const tree_nat = dlt::DecisionLambdaTree::from_curves(
            std::vector<dlt::CurveCandidate>{{"nat_bin", "algo_nat", step},
                                             {"flat_bin", "algo_flat", line_curve(xs_step, 0.0, c)}},
            &cf::fit_natural_cubic_spline);
        check("from_curves: natuerliches Fit-Verfahren durchgereicht => Schein-Schwellen entstehen",
              !tree_nat.thresholds().empty());
    }

    std::cout << "\n==== Hybrid-Break-Even-Vorbau: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
