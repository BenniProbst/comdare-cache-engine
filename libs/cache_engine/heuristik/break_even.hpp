#pragma once
// AXIS_ALGO_VERSION: 1
// heuristik/break_even.hpp -- Schnittpunkt-Finder zweier AxisSpline f/g DERSELBEN Organ-Achse.
// PAKET W3-C (Ledger Sec.32-F8, "Break-Even-Mathematik"). Header-only, KEINE Fremdbibliothek, kein Python.
//
// F8-MATHEMATIK (verbatim-treu): "Switch-Thresholds = Schnittpunkte zwischen den f(x)-Spline-Funktionen
// der Performance-Modellierungen zweier Algorithmen derselben Achse (Beispiel: Allokatoren fuer grosse
// vs kleine Dateien -- Kurven uebereinandergelegt ergeben den Break-Even-Punkt)."
//
// VERFAHREN: d(x) = f(x) - g(x). Auf dem UEBERLAPPUNGS-Intervall [max(f.x_min,g.x_min),
// min(f.x_max,g.x_max)] wird ueber die Vereinigung beider Knoten-Gitter ein gemeinsames, aufsteigend
// sortiertes Auswerte-Gitter gebildet. Je aufeinanderfolgendem Segment-Paar [a,b] entscheidet der
// Vorzeichenwechsel von d: sign(d(a)) != sign(d(b)) -> genau eine Nullstelle im Inneren -> BISEKTION
// (deterministisch, feste Iterationszahl) auf d==0. Exakte Knoten-Treffer (d(a)==0) werden separat als
// Schnittpunkt erfasst (ohne Doppelzaehlung). ALLE Schnittpunkte, deterministisch, aufsteigend nach x.
//
// KONVENTION "besser" = KLEINERER y-Wert (Performance: niedrigere Latenz/Kosten ist besser). Je
// Schnittpunkt wird geprueft, welche Kurve UNMITTELBAR LINKS und welche UNMITTELBAR RECHTS niedriger
// liegt -> BreakEvenPoint{x, y, links_besser, rechts_besser}.
//
// BENANNTES PATTERN: Strategy (die f/g sind AxisSpline<Strategy> -- der Finder ist generisch ueber
// BELIEBIGE Strategy-Kombination, auch monoton x natuerlich). Die Bisektion ist ein klassisches,
// benanntes Wurzel-Einschluss-Verfahren (bracketing root-find), deterministisch.

#include "axis_spline.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace comdare::cache_engine::heuristik {

/// Welche der beiden Kurven (f oder g) auf einer Seite des Schnittpunkts besser (= niedriger) ist.
enum class Curve : std::uint8_t {
    F   = 0, ///< die erste Kurve ist niedriger (besser)
    G   = 1, ///< die zweite Kurve ist niedriger (besser)
    Tie = 2, ///< innerhalb der Toleranz gleichauf (kein klarer Vorteil)
};

/// Ein Break-Even-Uebergang: bei x kreuzen sich f und g; links/rechts ist jeweils eine Kurve besser.
struct BreakEvenPoint {
    double x = 0.0;               ///< Schnittpunkt-Position (Parameter-Achse)
    double y = 0.0;               ///< Funktionswert am Schnittpunkt (Mittel aus f,g)
    Curve  links_besser = Curve::Tie;  ///< welche Kurve unmittelbar LINKS niedriger liegt
    Curve  rechts_besser = Curve::Tie; ///< welche Kurve unmittelbar RECHTS niedriger liegt
};

namespace detail {

/// Vorzeichen mit Nulltoleranz: -1 / 0 / +1. |v| <= tol gilt als 0 (Knoten-Treffer/Tie-Schutz).
[[nodiscard]] inline int sign_tol(double v, double tol) noexcept {
    if (v > tol) return 1;
    if (v < -tol) return -1;
    return 0;
}

/// Welche Kurve ist bei x niedriger (besser)? tol trennt "gleichauf" (Tie) sauber ab.
template <class SplineF, class SplineG>
[[nodiscard]] Curve better_at(SplineF const& f, SplineG const& g, double x, double tol) {
    double const d = f.eval(x) - g.eval(x);
    if (d < -tol) return Curve::F; // f niedriger
    if (d > tol) return Curve::G;  // g niedriger
    return Curve::Tie;
}

} // namespace detail

/// Findet ALLE Break-Even-Schnittpunkte von f und g auf ihrer Domaenen-Ueberlappung. Deterministisch:
/// festes Gitter (Vereinigung beider Knoten), feste Bisektions-Iterationszahl. `y_tol` toleriert
/// Knoten-Treffer und Tie. Rueckgabe aufsteigend nach x, ohne Duplikate.
template <class SplineF, class SplineG>
[[nodiscard]] std::vector<BreakEvenPoint> find_break_even_points(SplineF const& f, SplineG const& g,
                                                                 double y_tol = 1e-9) {
    std::vector<BreakEvenPoint> out;

    double const lo = std::max(f.x_min(), g.x_min());
    double const hi = std::min(f.x_max(), g.x_max());
    if (!(lo < hi)) return out; // keine Ueberlappung -> kein Break-Even (ehrlich leer)

    // Gemeinsames Auswerte-Gitter: alle Knoten beider Splines innerhalb [lo,hi], plus die Grenzen.
    std::vector<double> grid;
    grid.push_back(lo);
    grid.push_back(hi);
    for (double xv : f.knots_x())
        if (xv > lo && xv < hi) grid.push_back(xv);
    for (double xv : g.knots_x())
        if (xv > lo && xv < hi) grid.push_back(xv);
    std::sort(grid.begin(), grid.end());
    grid.erase(std::unique(grid.begin(), grid.end()), grid.end());

    auto d_at = [&](double x) { return f.eval(x) - g.eval(x); };

    // Kleine, aber gitter-relative Seiten-Distanz fuer die links/rechts-Besser-Bestimmung.
    double const span      = hi - lo;
    double const side_step = span * 1e-6;

    auto record = [&](double xr) {
        // Doppelzaehlung vermeiden (Knoten-Treffer, der auch als Segment-Wechsel erscheint).
        if (!out.empty() && std::fabs(out.back().x - xr) <= span * 1e-9) return;
        double const xl = std::max(lo, xr - side_step);
        double const xh = std::min(hi, xr + side_step);
        BreakEvenPoint p;
        p.x             = xr;
        p.y             = 0.5 * (f.eval(xr) + g.eval(xr));
        p.links_besser  = detail::better_at(f, g, xl, y_tol);
        p.rechts_besser = detail::better_at(f, g, xh, y_tol);
        out.push_back(p);
    };

    for (std::size_t i = 0; i + 1 < grid.size(); ++i) {
        double const a  = grid[i];
        double const b  = grid[i + 1];
        double const da = d_at(a);
        double const db = d_at(b);
        int const    sa = detail::sign_tol(da, y_tol);
        int const    sb = detail::sign_tol(db, y_tol);

        if (sa == 0) { record(a); } // exakter Knoten-Schnitt am linken Rand des Segments

        if (sa != 0 && sb != 0 && sa != sb) {
            // Echter Vorzeichenwechsel -> Bisektion (deterministisch, feste Iterationszahl).
            double aa = a, bb = b, fa = da;
            for (int it = 0; it < 100; ++it) {
                double const mid = 0.5 * (aa + bb);
                double const fm  = d_at(mid);
                if (detail::sign_tol(fm, y_tol) == 0 || (bb - aa) <= span * 1e-12) {
                    aa = bb = mid;
                    break;
                }
                if (detail::sign_tol(fm, y_tol) == detail::sign_tol(fa, y_tol)) {
                    aa = mid;
                    fa = fm;
                } else {
                    bb = mid;
                }
            }
            record(0.5 * (aa + bb));
        }
    }
    // Rechter Gitter-Rand: exakter Knoten-Schnitt bei hi (falls Segment-Schleife ihn nicht erfasste).
    if (!grid.empty() && detail::sign_tol(d_at(grid.back()), y_tol) == 0) record(grid.back());

    return out;
}

} // namespace comdare::cache_engine::heuristik
