#pragma once
// AXIS_ALGO_VERSION: 1
// heuristik/axis_spline.hpp -- Spline-Interpolation ueber (x,y)-Stuetzstellen einer Organ-Achsen-Variante.
// PAKET W3-C (Ledger Sec.32-F8, Abgabe-Pflicht). Header-only, KEINE Fremdbibliothek, kein Python.
//
// ZWECK (F8): je Organ-Achsen-Variante wird aus den realen Messpunkten (x = Mess-Parameter, z.B.
// working_set_n / value_size; y = Messwert, z.B. ns_per_op / total_cycles) eine glatte Modell-Funktion
// f(x) gebildet. Diese f(x) dient als Heuristik-Abschaetzung (Break-Even gegen die f(x) einer anderen
// Variante DERSELBEN Achse -> siehe break_even.hpp).
//
// BENANNTES PATTERN: Strategy (GoF) fuer das Interpolations-VERFAHREN. Die Wahl des Verfahrens ist
// COMPILE-TIME (Template-Parameter Strategy, per Concept gehaertet -- CT-Doktrin: die Methoden-Auswahl
// ist statisch, kein Runtime-Switch, kein std::variant). Die Stuetzstellen selbst sind Laufzeit-Messdaten.
//
// -- VERFAHRENS-WAHL (numerisch begruendet): DEFAULT = monotone kubische Hermite (Fritsch-Carlson 1980) --
// Performance-Kurven (Latenz ueber Working-Set / Value-Size) sind auf ihren Segmenten typisch MONOTON.
// Ein NATUERLICHER kubischer Spline erzwingt C2-Glattheit global und kann zwischen zwei Stuetzstellen
// UEBERSCHWINGEN (Oszillation / kuenstliche lokale Extrema), obwohl die Daten dort monoton sind. Diese
// falschen Extrema erzeugen SCHEIN-Schnittpunkte im Break-Even-Finder -> falsche Switch-Thresholds.
// Fritsch-Carlson begrenzt die Hermite-Steigungen (alpha^2+beta^2 <= 9 je Segment) und GARANTIERT damit
// die Monotonie-Erhaltung auf jedem Segment, auf dem die Daten monoton sind (Fritsch & Carlson, "Monotone
// Piecewise Cubic Interpolation", SIAM J. Numer. Anal. 17(2), 1980). Fuer Break-Even-Mathematik ist das
// die korrekte Wahl: kein kuenstlicher Nulldurchgang von f-g. Der natuerliche kubische Spline bleibt als
// alternative Strategy verfuegbar (glatter, aber overshoot-anfaellig) -- die Strategy-Naht macht die Wahl
// explizit und austauschbar.
//
// HONEST-EMPTY-Doktrin (analog builder/curve_fit/curve_fit.hpp): < 2 verwertbare Stuetzstellen -> KEIN
// Spline (build() liefert std::nullopt). Nie ein Phantom-Modell aus unzureichenden Daten.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::heuristik {

/// Eine (x,y)-Stuetzstelle einer Mess-Kurve. x = Parameter-Achse, y = Messwert. Single-Source des
/// Sample-Typs (auch der measurement_curve_loader.hpp und break_even.hpp nutzen ihn).
struct CurveSample {
    double x = 0.0;
    double y = 0.0;
};

namespace detail {

/// Sortiert Stuetzstellen nach x aufsteigend, aggregiert Duplikate (gleiches x, z.B. Wiederholungs-
/// Messungen) deterministisch per Mittelwert, verwirft nicht-endliche Werte. Ergebnis: streng
/// aufsteigende, endliche Knoten. KEINE Phantom-Punkte -- ungueltige Samples fallen heraus.
[[nodiscard]] inline std::vector<CurveSample> normalize_samples(std::vector<CurveSample> in) {
    std::vector<CurveSample> clean;
    clean.reserve(in.size());
    for (CurveSample const& s : in) {
        if (std::isfinite(s.x) && std::isfinite(s.y)) clean.push_back(s);
    }
    std::stable_sort(clean.begin(), clean.end(), [](CurveSample const& a, CurveSample const& b) { return a.x < b.x; });
    std::vector<CurveSample> out;
    out.reserve(clean.size());
    std::size_t i = 0;
    while (i < clean.size()) {
        std::size_t j     = i;
        double      sum_y = 0.0;
        while (j < clean.size() && clean[j].x == clean[i].x) {
            sum_y += clean[j].y;
            ++j;
        }
        out.push_back(CurveSample{clean[i].x, sum_y / static_cast<double>(j - i)});
        i = j;
    }
    return out;
}

/// Thomas-Algorithmus (Tridiagonal-Loeser) fuer a[i]*x[i-1] + b[i]*x[i] + c[i]*x[i+1] = d[i].
/// Deterministisch, in-place-Ruecksubstitution; keine Pivotisierung noetig (diagonal dominant fuer die
/// Spline-Systeme). Liefert leer bei degeneriertem b (Schutznetz gegen Division durch ~0).
[[nodiscard]] inline std::vector<double> solve_tridiagonal(std::vector<double> a, std::vector<double> b,
                                                           std::vector<double> c, std::vector<double> d) {
    std::size_t const n = b.size();
    if (n == 0 || a.size() != n || c.size() != n || d.size() != n) return {};
    for (std::size_t i = 1; i < n; ++i) {
        if (std::fabs(b[i - 1]) < 1e-300) return {}; // degeneriert -> ehrlich leer
        double const w = a[i] / b[i - 1];
        b[i] -= w * c[i - 1];
        d[i] -= w * d[i - 1];
    }
    if (std::fabs(b[n - 1]) < 1e-300) return {};
    std::vector<double> x(n, 0.0);
    x[n - 1] = d[n - 1] / b[n - 1];
    for (std::size_t k = n - 1; k-- > 0;) { x[k] = (d[k] - c[k] * x[k + 1]) / b[k]; }
    return x;
}

} // namespace detail

// -- Strategy 1: monotone kubische Hermite (Fritsch-Carlson) -- DEFAULT -----------------------------
/// Berechnet die Knoten-Steigungen m_i so, dass der stueckweise kubische Hermite-Interpolant die
/// Monotonie der Daten erhaelt (kein Overshoot). Referenz: Fritsch & Carlson 1980; Standard-Rezept.
struct MonotoneCubicHermiteStrategy {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "monotone_cubic_hermite_fritsch_carlson"; }
    [[nodiscard]] static constexpr bool             preserves_monotonicity() noexcept { return true; }

    [[nodiscard]] static std::vector<double> compute_slopes(std::vector<double> const& x,
                                                            std::vector<double> const& y) {
        std::size_t const   n = x.size();
        std::vector<double> m(n, 0.0);
        if (n < 2) return m;
        std::vector<double> delta(n - 1, 0.0); // Sekanten-Steigungen
        for (std::size_t i = 0; i + 1 < n; ++i) { delta[i] = (y[i + 1] - y[i]) / (x[i + 1] - x[i]); }
        // Start-Init: Enden = Randsekante, innen = Mittel der beiden Nachbarsekanten.
        m[0]     = delta[0];
        m[n - 1] = delta[n - 2];
        for (std::size_t i = 1; i + 1 < n; ++i) { m[i] = 0.5 * (delta[i - 1] + delta[i]); }
        // Fritsch-Carlson-Begrenzung je Segment: erzwingt Monotonie-Erhalt.
        for (std::size_t i = 0; i + 1 < n; ++i) {
            if (delta[i] == 0.0) { // flaches Segment -> beide Endsteigungen 0 (kein Overshoot)
                m[i]     = 0.0;
                m[i + 1] = 0.0;
                continue;
            }
            double const alpha = m[i] / delta[i];
            double const beta  = m[i + 1] / delta[i];
            if (alpha < 0.0) m[i] = 0.0;    // Vorzeichenwechsel am linken Knoten -> lokaler Extrempunkt
            if (beta < 0.0) m[i + 1] = 0.0; // Vorzeichenwechsel am rechten Knoten
            double const s = alpha * alpha + beta * beta;
            if (s > 9.0) {
                double const tau = 3.0 / std::sqrt(s);
                m[i]             = tau * alpha * delta[i];
                m[i + 1]         = tau * beta * delta[i];
            }
        }
        return m;
    }
};

// -- Strategy 2: natuerlicher kubischer Spline (C2, S''=0 an den Raendern) --------------------------
/// Loest die Knoten-Steigungen m_i so, dass der stueckweise kubische Hermite-Interpolant global C2 ist
/// (zweite Ableitung stetig) mit natuerlichen Randbedingungen S''(x_0)=S''(x_{n-1})=0. Glatter als
/// Fritsch-Carlson, aber NICHT monotonie-erhaltend (Overshoot moeglich) -- daher NICHT der F8-Default.
struct NaturalCubicStrategy {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "natural_cubic"; }
    [[nodiscard]] static constexpr bool             preserves_monotonicity() noexcept { return false; }

    [[nodiscard]] static std::vector<double> compute_slopes(std::vector<double> const& x,
                                                            std::vector<double> const& y) {
        std::size_t const   n = x.size();
        std::vector<double> m(n, 0.0);
        if (n < 2) return m;
        if (n == 2) { // eine Strecke -> lineare Steigung an beiden Knoten
            double const d = (y[1] - y[0]) / (x[1] - x[0]);
            m[0]           = d;
            m[1]           = d;
            return m;
        }
        std::vector<double> h(n - 1, 0.0), delta(n - 1, 0.0);
        for (std::size_t i = 0; i + 1 < n; ++i) {
            h[i]     = x[i + 1] - x[i];
            delta[i] = (y[i + 1] - y[i]) / h[i];
        }
        // Tridiagonal-System fuer die Steigungen m_i (aus Stetigkeit von S'' an inneren Knoten).
        std::vector<double> a(n, 0.0), b(n, 0.0), c(n, 0.0), d(n, 0.0);
        // Zeile 0: 2 m_0 + m_1 = 3 delta_0  (natuerliche Randbedingung S''(x_0)=0)
        b[0] = 2.0;
        c[0] = 1.0;
        d[0] = 3.0 * delta[0];
        for (std::size_t i = 1; i + 1 < n; ++i) {
            a[i] = 1.0 / h[i - 1];
            b[i] = 2.0 * (1.0 / h[i - 1] + 1.0 / h[i]);
            c[i] = 1.0 / h[i];
            d[i] = 3.0 * (delta[i - 1] / h[i - 1] + delta[i] / h[i]);
        }
        // Zeile n-1: m_{n-2} + 2 m_{n-1} = 3 delta_{n-2}  (natuerliche Randbedingung S''(x_{n-1})=0)
        a[n - 1]                = 1.0;
        b[n - 1]                = 2.0;
        d[n - 1]                = 3.0 * delta[n - 2];
        std::vector<double> sol = detail::solve_tridiagonal(std::move(a), std::move(b), std::move(c), std::move(d));
        if (sol.size() != n) { // degeneriert -> Fallback auf Sekanten-Steigungen (nie NaN)
            m[0]     = delta[0];
            m[n - 1] = delta[n - 2];
            for (std::size_t i = 1; i + 1 < n; ++i) m[i] = 0.5 * (delta[i - 1] + delta[i]);
            return m;
        }
        return sol;
    }
};

/// Concept-Haertung der Strategy-Naht (CRTP-frei; static-Policy, keine vtable). Jede Strategy liefert
/// einen stabilen Namen, ein Monotonie-Flag und die Knoten-Steigungs-Berechnung.
template <class S>
concept InterpolationStrategy = requires(std::vector<double> const& xs, std::vector<double> const& ys) {
    { S::name() } -> std::convertible_to<std::string_view>;
    { S::preserves_monotonicity() } -> std::same_as<bool>;
    { S::compute_slopes(xs, ys) } -> std::same_as<std::vector<double>>;
};

/// Der Spline EINER Achsen-Variante ueber ihre (x,y)-Stuetzstellen. Verfahren = compile-time Strategy.
/// Auswertung ist stueckweise kubisch-Hermite (einheitlich fuer beide Strategien -- beide liefern nur
/// die Knoten-Steigungen). Ausserhalb von [x_min, x_max] wird flach auf den Randwert geklemmt (definierte
/// Extrapolation fuer den Break-Even-Finder, der ohnehin nur die Domaenen-Ueberlappung betrachtet).
template <InterpolationStrategy Strategy = MonotoneCubicHermiteStrategy>
class AxisSpline {
public:
    /// Baut den Spline aus rohen Stuetzstellen. HONEST-EMPTY: < 2 verwertbare Knoten -> std::nullopt.
    [[nodiscard]] static std::optional<AxisSpline> build(std::vector<CurveSample> samples) {
        std::vector<CurveSample> knots = detail::normalize_samples(std::move(samples));
        if (knots.size() < 2) return std::nullopt; // unterbestimmt -> kein Modell
        std::vector<double> xs, ys;
        xs.reserve(knots.size());
        ys.reserve(knots.size());
        for (CurveSample const& s : knots) {
            xs.push_back(s.x);
            ys.push_back(s.y);
        }
        std::vector<double> m = Strategy::compute_slopes(xs, ys);
        if (m.size() != xs.size()) return std::nullopt; // Verfahren degeneriert -> ehrlich kein Modell
        AxisSpline sp;
        sp.x_ = std::move(xs);
        sp.y_ = std::move(ys);
        sp.m_ = std::move(m);
        return sp;
    }

    [[nodiscard]] static constexpr std::string_view strategy_name() noexcept { return Strategy::name(); }
    [[nodiscard]] static constexpr bool preserves_monotonicity() noexcept { return Strategy::preserves_monotonicity(); }

    [[nodiscard]] std::size_t                size() const noexcept { return x_.size(); }
    [[nodiscard]] double                     x_min() const noexcept { return x_.front(); }
    [[nodiscard]] double                     x_max() const noexcept { return x_.back(); }
    [[nodiscard]] std::vector<double> const& knots_x() const noexcept { return x_; }

    /// Wertet f(x) aus. Ausserhalb der Domaene: flach geklemmt auf den jeweiligen Randwert.
    [[nodiscard]] double eval(double x) const {
        if (x <= x_.front()) return y_.front();
        if (x >= x_.back()) return y_.back();
        // Segment finden: groesstes i mit x_[i] <= x (x liegt echt im Inneren).
        std::size_t const idx = static_cast<std::size_t>(std::upper_bound(x_.begin(), x_.end(), x) - x_.begin() - 1);
        std::size_t const i   = std::min(idx, x_.size() - 2);
        double const      h   = x_[i + 1] - x_[i];
        double const      t   = (x - x_[i]) / h;
        double const      t2  = t * t;
        double const      t3  = t2 * t;
        // Hermite-Basis: h00,h10,h01,h11.
        double const h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
        double const h10 = t3 - 2.0 * t2 + t;
        double const h01 = -2.0 * t3 + 3.0 * t2;
        double const h11 = t3 - t2;
        return h00 * y_[i] + h10 * h * m_[i] + h01 * y_[i + 1] + h11 * h * m_[i + 1];
    }

private:
    AxisSpline() = default;
    std::vector<double> x_; // streng aufsteigende Knoten
    std::vector<double> y_;
    std::vector<double> m_; // Knoten-Steigungen (aus der Strategy)
};

/// Kanonische Kurz-Aliase.
using MonotoneAxisSpline = AxisSpline<MonotoneCubicHermiteStrategy>;
using NaturalAxisSpline  = AxisSpline<NaturalCubicStrategy>;

} // namespace comdare::cache_engine::heuristik
