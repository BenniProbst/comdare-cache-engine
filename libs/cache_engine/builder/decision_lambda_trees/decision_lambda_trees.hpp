#pragma once
// decision_lambda_trees -- Entscheidungs-Lambda-Baum-SKELETON (Section 32-F8 Hybrid-Rueckwaerts-Wahl, 2026-07-20).
//
// Verbindet das ENGINE-seitige Spline-Modell (curve_fit.hpp AxisSpline) mit den Break-Even-Schwellen zu
// einem Entscheidungsbaum: an einer Anfrage-Koordinate x (Working-Set-Groesse) waehlt der Baum ueber die
// aus den Spline-SCHNITTPUNKTEN abgeleiteten Schwellen RUECKWAERTS die fuehrende Kandidaten-Binary.
//
// HONEST-EMPTY: ohne gueltige Kurven keine Entscheidung (leere binary_id). SCAFFOLD-GRENZE: die Stuetz-
// stellen sind synthetisch (#156-DATA-gated); das Geruest wird gegen synthetische Kurven entwickelt/
// getestet. Der SELF-CONTAINED best_binary_selector traegt eine std-only Spiegelung derselben Rueckwaerts-
// Wahl (PiecewiseCurve statt AxisSpline) -- dieses Engine-Modul ist die AxisSpline-basierte Variante.
//
// Section 49: KEIN std::variant. Die Haupt-Kommunikation Hybrid<->Tier-Binary-Observer bleibt statisch
// (IObservableTier / Pruef-Dock, zero-cost, Section 9). Die eng begrenzte variant-/Abstract-Factory-
// Mechanik fuer abweichende Unter-Pruef-Dock-Typen ist S7/S9, NICHT dieser Entscheidungsbaum-Vorbau.
//
// @related [[feedback_heuristik_messkurven_typsystem_chain_of_responsibility]]

#include "../curve_fit/curve_fit.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::decision_lambda_trees {

namespace cf = ::comdare::cache_engine::builder::curve_fit;

/// Ein modellierter Kandidat: Binary-Identitaet + Algo-Satz + Achsen-Spline (curve_fit). Kosten kleiner = besser.
struct SplineCandidate {
    std::string    binary_id;
    std::string    algo_set;
    cf::AxisSpline model;
};

/// Ein ROH-Kandidat: Binary-Identitaet + Algo-Satz + noch NICHT gefittete Mess-Kurve. Wird ueber ein
/// waehlbares Fit-Verfahren (Default = monotoner, overshoot-sicherer Fritsch-Carlson-Spline) zu einem
/// SplineCandidate modelliert (DecisionLambdaTree::from_curves).
struct CurveCandidate {
    std::string          binary_id;
    std::string          algo_set;
    cf::MeasurementCurve curve;
};

/// Eine Break-Even-Schwelle im Entscheidungsbaum: unterhalb x fuehrt below_id, oberhalb above_id.
struct DecisionThreshold {
    double      x_working_set_bytes = 0.0;
    std::string below_id;
    std::string above_id;
};

/// DecisionLambdaTree (SKELETON): traegt die Kandidaten + leitet die Schwellen aus den Spline-Schnittpunkten
/// ab und materialisiert die Rueckwaerts-Wahl. Modell-agnostisch ueber cf::AxisSpline::eval().
class DecisionLambdaTree {
public:
    explicit DecisionLambdaTree(std::vector<SplineCandidate> candidates) : candidates_(std::move(candidates)) {}

    /// Baut den Baum aus ROH-Kurven: jede Kurve wird mit `fit` modelliert. Default = fit_monotone_cubic_spline
    /// (Fritsch-Carlson, overshoot-frei), sodass decide()/thresholds() denselben SCHEIN-Break-Even-freien
    /// Spline nutzen wie die heuristik-Familie -- statt des UEBERSCHWINGENDEN natuerlichen Fits. Das Fit-
    /// Verfahren ist compile-time durchgereicht (Template-Callable, kein Runtime-Switch); jeder Aufrufer kann
    /// z.B. &cf::fit_natural_cubic_spline uebergeben, ohne die Klasse zu aendern.
    template <class Fit = cf::AxisSpline (*)(cf::MeasurementCurve const&)>
    [[nodiscard]] static DecisionLambdaTree from_curves(std::vector<CurveCandidate> curves,
                                                        Fit fit = &cf::fit_monotone_cubic_spline) {
        std::vector<SplineCandidate> cands;
        cands.reserve(curves.size());
        for (CurveCandidate& c : curves)
            cands.push_back(SplineCandidate{std::move(c.binary_id), std::move(c.algo_set), fit(c.curve)});
        return DecisionLambdaTree{std::move(cands)};
    }

    /// Waehlt an x die Kandidaten-Binary mit minimalem Spline-Modellwert (nur valid() Kandidaten).
    /// Leere Rueckgabe = keine gueltige Entscheidung (honest-empty).
    [[nodiscard]] std::string decide(std::uint64_t x_working_set_bytes) const {
        std::string best_id;
        double      best = 0.0;
        for (SplineCandidate const& c : candidates_) {
            if (!c.model.valid()) continue;
            double const v = c.model.eval(x_working_set_bytes);
            if (!std::isfinite(v)) continue;
            if (best_id.empty() || v < best) {
                best_id = c.binary_id;
                best    = v;
            }
        }
        return best_id;
    }

    /// Break-Even-Schwellen zwischen JE ZWEI Kandidaten (Spline-Schnittpunkte, curve_fit). Diagnose/Doku;
    /// aufsteigend je Paar. `samples` steuert die Raster-Aufloesung des Schnittpunkt-Finders.
    [[nodiscard]] std::vector<DecisionThreshold> thresholds(std::size_t samples = 256) const {
        std::vector<DecisionThreshold> out;
        for (std::size_t i = 0; i < candidates_.size(); ++i) {
            for (std::size_t j = i + 1; j < candidates_.size(); ++j) {
                cf::AxisSpline const& mi = candidates_[i].model;
                cf::AxisSpline const& mj = candidates_[j].model;
                if (!mi.valid() || !mj.valid()) continue;
                auto const   ix   = cf::spline_intersections(mi, mj, samples);
                double const lo   = std::max(mi.knots.front().x_log2, mj.knots.front().x_log2);
                double const hi   = std::min(mi.knots.back().x_log2, mj.knots.back().x_log2);
                double const side = (hi > lo) ? (hi - lo) * 1e-6 : 0.0; // span-relativer Seiten-Schritt (log2)
                for (cf::SplineIntersection const& p : ix) {
                    // Wer fuehrt knapp UNTERHALB der Schwelle (kleinere Kosten)? Probe span-relativ LINKS in der
                    // log2-Abszisse (nicht der grobe x-1.0-Byte-Schritt, der bei grossen Working-Sets in log2
                    // verschwindet und die Fuehrungs-Bestimmung nahe dem Schnittpunkt mehrdeutig macht).
                    double const      xl_probe = std::max(lo, p.x_log2 - side);
                    bool const        i_below  = mi.eval_log2(xl_probe) <= mj.eval_log2(xl_probe);
                    DecisionThreshold t;
                    t.x_working_set_bytes = p.x_working_set_bytes;
                    t.below_id            = i_below ? candidates_[i].binary_id : candidates_[j].binary_id;
                    t.above_id            = i_below ? candidates_[j].binary_id : candidates_[i].binary_id;
                    out.push_back(t);
                }
            }
        }
        return out;
    }

    [[nodiscard]] std::size_t candidate_count() const noexcept { return candidates_.size(); }

private:
    std::vector<SplineCandidate> candidates_;
};

} // namespace comdare::cache_engine::builder::decision_lambda_trees
