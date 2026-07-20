#pragma once
// E4'-Kurven-Fit-SKELETON (Phase-6-Vorbau, 2026-07-10): CSV -> Kurven -> Schaetzer-Stufe.
// Fundament: cacheline_policy_selector + Objectives-BEFUND (backups/20260709-pareto-objectives-*).
// HONEST-EMPTY-Doktrin: ohne (gueltige) Daten gibt es KEINEN Fit — nie Phantom-Werte.
// Der reale Fit ist datengetrieben und bleibt auf #156-Messdaten gated.
//
// SPALTEN-WAHRHEIT (Review wf_c99a2132, CONFIRMED): die BESTANDS-WIDE-CSV der E1-Strecke
// (lazy_csv_header) ist SEMIKOLON-separiert mit Spalten wie working_set_n / op_<art>_p99_ns /
// pmc_cache_misses_l1 — die Registry-Namen (LATENCY_P99, ...) sind das E4-REPORTING-Vokabular,
// NICHT die Bestands-Spalten. Der Leser ist deshalb dialekt-parametrisiert (delimiter) und der
// AUFRUFER liefert den realen Spaltennamen; das Mapping Registry-Name -> Bestands-Spalte gehoert
// in die E4'-Folge-Stufe.

#include <cache_engine/measurement/measurement_category.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <istream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::curve_fit {

namespace mm = ::comdare::cache_engine::measurement;

/// Ein Messpunkt einer Achsen-Kurve ueber die Working-Set-Groesse.
struct CurvePoint {
    std::uint64_t x_working_set_bytes = 0;
    double        y_value             = 0.0;
    std::uint64_t sample_count        = 0;
};

/// Kurve EINER Mess-Achse (Kategorie aus der Registry) ueber aufsteigende Working-Set-Groessen.
struct MeasurementCurve {
    mm::MeasurementCategory category = mm::MeasurementCategory::CLU;
    std::string             axis_name;
    std::vector<CurvePoint> points;           // aufsteigend nach x
    std::uint64_t           skipped_rows = 0; // nicht-numerische/unvollstaendige Zeilen (Diagnose)
};

enum class FitStatus : std::uint8_t {
    NoData             = 0, ///< keine Punkte — ehrlicher Leerstatus, KEIN Phantom-Fit
    InsufficientPoints = 1, ///< <2 Punkte oder keine x-Streuung — Modell unterbestimmt
    InvalidData        = 2, ///< x==0 (log2 undefiniert) oder nicht-endliche Werte — ehrlich abgelehnt
    Ok                 = 3,
};

/// Modell y = a*log2(x) + b (Cache-Hierarchie-Kurven ueber Working-Set, Objectives-BEFUND).
struct FitResult {
    FitStatus status       = FitStatus::NoData;
    double    a            = 0.0;
    double    b            = 0.0;
    double    residual_rms = 0.0;
};

/// Kleinste Quadrate ueber (log2 x, y). Honest-Doktrin (Review wf_c99a2132): x==0 oder
/// nicht-endliche y => InvalidData (NIE Ok mit NaN); keine x-Streuung => InsufficientPoints
/// (varianz-basiert statt exaktem det==0 — Gleitkomma-Rundung, 7x-identisch-Beweis des Reviews).
[[nodiscard]] inline FitResult fit_log_linear(MeasurementCurve const& curve) {
    FitResult r;
    if (curve.points.empty()) return r; // NoData
    for (CurvePoint const& p : curve.points) {
        if (p.x_working_set_bytes == 0 || !std::isfinite(p.y_value)) {
            r.status = FitStatus::InvalidData;
            return r;
        }
    }
    if (curve.points.size() < 2) {
        r.status = FitStatus::InsufficientPoints;
        return r;
    }
    double const n  = static_cast<double>(curve.points.size());
    double       sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    for (CurvePoint const& p : curve.points) {
        double const lx = std::log2(static_cast<double>(p.x_working_set_bytes));
        sx += lx;
        sy += p.y_value;
        sxx += lx * lx;
        sxy += lx * p.y_value;
    }
    // Normalisierte x-Varianz: unterhalb der Schwelle ist die Gerade unterbestimmt (alle x ~gleich).
    double const var_x = (sxx - (sx * sx) / n) / n;
    if (!(var_x > 1e-9)) {
        r.status = FitStatus::InsufficientPoints;
        return r;
    }
    double const det = n * sxx - sx * sx;
    r.a              = (n * sxy - sx * sy) / det;
    r.b              = (sy - r.a * sx) / n;
    double rss       = 0.0;
    for (CurvePoint const& p : curve.points) {
        double const lx = std::log2(static_cast<double>(p.x_working_set_bytes));
        double const e  = p.y_value - (r.a * lx + r.b);
        rss += e * e;
    }
    r.residual_rms = std::sqrt(rss / n);
    if (!std::isfinite(r.a) || !std::isfinite(r.b) || !std::isfinite(r.residual_rms)) {
        r        = FitResult{};
        r.status = FitStatus::InvalidData; // Schutznetz: NIE Ok mit NaN
        return r;
    }
    r.status = FitStatus::Ok;
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
// E4'-SPLINE-SKELETON (§32-F8 / §49 Hybrid-Break-Even-Vorbau, 2026-07-20)
// ─────────────────────────────────────────────────────────────────────────────
// Zweck: die Break-Even-Heuristik (§32-F8) modelliert je Achse EINE f(x)-Kurve; die SCHNITTPUNKTE zweier
// Achsen-/Algorithmen-Kurven sind die Switch-Thresholds (Break-Even-Punkte). Dieses Geruest liefert den
// natuerlichen kubischen Spline je MeasurementCurve (C2-stetig; Abszisse = log2(x), wie fit_log_linear) +
// einen MODELL-AGNOSTISCHEN Schnittpunkt-Finder (Raster + Bisektion). HONEST-EMPTY wie fit_log_linear:
// ohne >=2 Punkte mit x-Streuung entsteht KEIN Spline (status != Ok) -- nie Phantom-Kurven.
// SCAFFOLD-GRENZE: reale Stuetzstellen sind #156-DATA-gated; entwickelt+getestet auf SYNTHETISCHEN Kurven.
// In S7 speist derselbe Spline die eval()-Kontur des best_binary_selector-Break-Even (der SELF-CONTAINED
// Tool-Scaffold traegt eine eigene std-only Spiegelung, KEINE Engine-Kopplung -- Doktrin des Werkzeugs).
// §49: KEIN std::variant; die Haupt-Observer-Kommunikation bleibt statisch (IObservableTier/Pruef-Dock).

/// Ein Spline-Knoten in der Modell-Abszisse log2(x).
struct SplineKnot {
    double x_log2 = 0.0; ///< Modell-Abszisse = log2(working_set_bytes)
    double y      = 0.0; ///< Messwert an diesem Knoten
    double m2     = 0.0; ///< zweite Ableitung (natuerlicher Spline: Rand-Knoten m2 == 0)
};

/// Natuerlicher kubischer Spline EINER Mess-Achse ueber aufsteigende Working-Set-Groessen.
struct AxisSpline {
    FitStatus               status   = FitStatus::NoData;
    mm::MeasurementCategory category = mm::MeasurementCategory::CLU;
    std::string             axis_name;
    std::vector<SplineKnot> knots; ///< streng aufsteigend nach x_log2

    [[nodiscard]] bool valid() const noexcept { return status == FitStatus::Ok && knots.size() >= 2; }

    /// eval direkt auf der Modell-Abszisse (log2). Ausserhalb [x_first,x_last]: LINEARE Extrapolation mit
    /// der Rand-Segment-Steigung (keine wilde kubische Extrapolation). Ungueltiger Spline => NaN.
    [[nodiscard]] double eval_log2(double xl) const {
        if (!valid()) return std::numeric_limits<double>::quiet_NaN();
        std::size_t const n = knots.size();
        if (xl <= knots.front().x_log2) {
            double const h  = knots[1].x_log2 - knots[0].x_log2;
            double const s0 = (knots[1].y - knots[0].y) / h - h * (2.0 * knots[0].m2 + knots[1].m2) / 6.0;
            return knots[0].y + s0 * (xl - knots[0].x_log2);
        }
        if (xl >= knots.back().x_log2) {
            double const h = knots[n - 1].x_log2 - knots[n - 2].x_log2;
            double const sN =
                (knots[n - 1].y - knots[n - 2].y) / h + h * (knots[n - 2].m2 + 2.0 * knots[n - 1].m2) / 6.0;
            return knots[n - 1].y + sN * (xl - knots[n - 1].x_log2);
        }
        std::size_t hi = 1;
        while (hi < n && knots[hi].x_log2 < xl) ++hi;
        std::size_t const lo = hi - 1;
        double const      h  = knots[hi].x_log2 - knots[lo].x_log2;
        double const      a  = (knots[hi].x_log2 - xl) / h;
        double const      b  = (xl - knots[lo].x_log2) / h;
        return a * knots[lo].y + b * knots[hi].y +
               ((a * a * a - a) * knots[lo].m2 + (b * b * b - b) * knots[hi].m2) * (h * h) / 6.0;
    }

    /// eval an einer realen Working-Set-Groesse (x==0 => NaN, log2 undefiniert).
    [[nodiscard]] double eval(std::uint64_t x_working_set_bytes) const {
        if (x_working_set_bytes == 0) return std::numeric_limits<double>::quiet_NaN();
        return eval_log2(std::log2(static_cast<double>(x_working_set_bytes)));
    }
};

/// Natuerlicher kubischer Spline-Fit ueber (log2 x, y). Honest-Doktrin (wie fit_log_linear): x==0 oder
/// nicht-endliche y => InvalidData; <2 DISTINKTE x => InsufficientPoints. Wiederholungen derselben
/// Working-Set-Groesse sind Samples EINER Stuetzstelle (y gemittelt). n==2 => linearer Spline (m2==0).
[[nodiscard]] inline AxisSpline fit_natural_cubic_spline(MeasurementCurve const& curve) {
    AxisSpline s;
    s.category  = curve.category;
    s.axis_name = curve.axis_name;
    if (curve.points.empty()) return s; // NoData
    for (CurvePoint const& p : curve.points) {
        if (p.x_working_set_bytes == 0 || !std::isfinite(p.y_value)) {
            s.status = FitStatus::InvalidData;
            return s;
        }
    }
    // Nach x sortieren + auf DISTINKTE x (log2) mit gemitteltem y kollabieren (Spline verlangt streng
    // steigende Knoten).
    std::vector<CurvePoint> pts = curve.points;
    std::sort(pts.begin(), pts.end(),
              [](CurvePoint const& a, CurvePoint const& b) { return a.x_working_set_bytes < b.x_working_set_bytes; });
    std::vector<double> xs, ys;
    for (std::size_t i = 0; i < pts.size();) {
        std::uint64_t const x   = pts[i].x_working_set_bytes;
        double              sum = 0.0;
        std::size_t         cnt = 0;
        while (i < pts.size() && pts[i].x_working_set_bytes == x) {
            sum += pts[i].y_value;
            ++cnt;
            ++i;
        }
        xs.push_back(std::log2(static_cast<double>(x)));
        ys.push_back(sum / static_cast<double>(cnt));
    }
    std::size_t const n = xs.size();
    if (n < 2) {
        s.status = FitStatus::InsufficientPoints;
        return s;
    }
    // Tridiagonales System fuer die zweiten Ableitungen (natuerlicher Spline: m[0]=m[n-1]=0), Thomas-Algo.
    std::vector<double> m(n, 0.0), rhs(n, 0.0), diag(n, 0.0), sup(n, 0.0), sub(n, 0.0);
    for (std::size_t i = 1; i + 1 < n; ++i) {
        double const hl = xs[i] - xs[i - 1];
        double const hr = xs[i + 1] - xs[i];
        sub[i]          = hl / 6.0;
        diag[i]         = (hl + hr) / 3.0;
        sup[i]          = hr / 6.0;
        rhs[i]          = (ys[i + 1] - ys[i]) / hr - (ys[i] - ys[i - 1]) / hl;
    }
    for (std::size_t i = 2; i + 1 < n; ++i) { // Vorwaerts-Elimination der inneren Zeilen
        double const w = sub[i] / diag[i - 1];
        diag[i] -= w * sup[i - 1];
        rhs[i] -= w * rhs[i - 1];
    }
    for (std::size_t i = n - 2; i >= 1; --i) { // Rueckwaerts-Substitution (m[0]=m[n-1]=0 fix)
        m[i] = (rhs[i] - sup[i] * m[i + 1]) / diag[i];
        if (i == 1) break; // size_t-Unterlauf vermeiden
    }
    s.knots.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        if (!std::isfinite(m[i])) { // Schutznetz: NIE Ok mit NaN
            AxisSpline bad;
            bad.category  = curve.category;
            bad.axis_name = curve.axis_name;
            bad.status    = FitStatus::InvalidData;
            return bad;
        }
        s.knots.push_back(SplineKnot{xs[i], ys[i], m[i]});
    }
    s.status = FitStatus::Ok;
    return s;
}

/// Break-Even-Schnittpunkt zweier Achsen-Splines.
struct SplineIntersection {
    double x_working_set_bytes = 0.0; ///< Break-Even-Working-Set (reale Skala, aus log2 zurueckgerechnet)
    double x_log2              = 0.0; ///< Modell-Abszisse des Schnittpunkts
    double y                   = 0.0; ///< gemeinsamer Modellwert am Schnittpunkt
};

/// Schnittpunkte (Break-Even) zweier Achsen-Splines im ueberlappenden x-Bereich. Modell-agnostisch:
/// rastert d(xl) = a.eval_log2 - b.eval_log2 ueber `samples` Stuetzstellen, lokalisiert Vorzeichenwechsel
/// und verfeinert per Bisektion. `samples` >= 2; beide Splines muessen valid() sein; kein Ueberlapp => leer.
[[nodiscard]] inline std::vector<SplineIntersection> spline_intersections(AxisSpline const& a, AxisSpline const& b,
                                                                          std::size_t samples = 256) {
    std::vector<SplineIntersection> out;
    if (!a.valid() || !b.valid() || samples < 2) return out;
    double const lo = std::max(a.knots.front().x_log2, b.knots.front().x_log2);
    double const hi = std::min(a.knots.back().x_log2, b.knots.back().x_log2);
    if (!(hi > lo)) return out;
    auto const d       = [&](double xl) { return a.eval_log2(xl) - b.eval_log2(xl); };
    auto const push_ix = [&](double xl) { out.push_back(SplineIntersection{std::exp2(xl), xl, a.eval_log2(xl)}); };
    double     d_prev  = d(lo);
    if (d_prev == 0.0) push_ix(lo);
    double x_prev = lo;
    for (std::size_t i = 1; i <= samples; ++i) {
        double const x_cur = lo + (hi - lo) * static_cast<double>(i) / static_cast<double>(samples);
        double const d_cur = d(x_cur);
        if (d_cur == 0.0) {
            push_ix(x_cur);
        } else if (d_prev != 0.0 && (d_prev < 0.0) != (d_cur < 0.0)) {
            double xl = x_prev, xr = x_cur, dl = d_prev; // Bisektion auf dem Vorzeichenwechsel
            for (int it = 0; it < 60 && (xr - xl) > 1e-12; ++it) {
                double const xm = 0.5 * (xl + xr);
                double const dm = d(xm);
                if (dm == 0.0) {
                    xl = xr = xm;
                    break;
                }
                if ((dm < 0.0) == (dl < 0.0)) {
                    xl = xm;
                    dl = dm;
                } else {
                    xr = xm;
                }
            }
            push_ix(0.5 * (xl + xr));
        }
        x_prev = x_cur;
        d_prev = d_cur;
    }
    return out;
}

namespace detail {

/// Zell-Split mit minimalem RFC-4180-Quoting (Repo-Writer csv_quote quotet unconditional) und
/// CRLF-Strip — Review wf_c99a2132: Windows-CSVs und gequotete Zellen duerfen weder Spalten
/// verschieben noch das Header-Matching brechen.
[[nodiscard]] inline std::vector<std::string> split_csv_line(std::string_view line, char delimiter) {
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    std::vector<std::string> cells;
    std::string              cell;
    bool                     in_quotes = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        char const c = line[i];
        if (c == '"') {
            if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') { // escaped quote
                cell.push_back('"');
                ++i;
            } else {
                in_quotes = !in_quotes;
            }
        } else if (c == delimiter && !in_quotes) {
            cells.push_back(std::move(cell));
            cell.clear();
        } else {
            cell.push_back(c);
        }
    }
    cells.push_back(std::move(cell));
    return cells;
}

/// Streng-numerische Zellen-Parser: ganze Zelle muss konsumiert werden (Review: 'n/a' darf NIE
/// still zum Phantom-Punkt (0,0) werden).
[[nodiscard]] inline bool parse_u64_cell(std::string const& cell, std::uint64_t& out) {
    if (cell.empty()) return false;
    char*                    end = nullptr;
    unsigned long long const v   = std::strtoull(cell.c_str(), &end, 10);
    if (end == nullptr || *end != '\0') return false;
    out = static_cast<std::uint64_t>(v);
    return true;
}

[[nodiscard]] inline bool parse_double_cell(std::string const& cell, double& out) {
    if (cell.empty()) return false;
    char*        end = nullptr;
    double const v   = std::strtod(cell.c_str(), &end);
    if (end == nullptr || *end != '\0' || !std::isfinite(v)) return false;
    out = v;
    return true;
}

} // namespace detail

/// Liest EINE Spalte eines CSV-Streams als Kurve. `delimiter` waehlt den Dialekt — die
/// Bestands-WIDE-CSV der E1-Strecke ist SEMIKOLON-separiert (Kopf-Kommentar); Default = ';'.
/// Fehlende Spalte => leere Kurve (Fit meldet NoData); nicht-numerische Zeilen werden gezaehlt
/// uebersprungen (skipped_rows), NIE zu Phantom-Punkten.
[[nodiscard]] inline MeasurementCurve parse_wide_csv_column(std::istream& csv, std::string_view x_column,
                                                            std::string_view y_column, mm::MeasurementCategory category,
                                                            char delimiter = ';') {
    MeasurementCurve curve;
    curve.category  = category;
    curve.axis_name = std::string{y_column};

    std::string header;
    if (!std::getline(csv, header)) return curve;

    std::ptrdiff_t x_idx = -1, y_idx = -1;
    {
        auto const cols = detail::split_csv_line(header, delimiter);
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(cols.size()); ++i) {
            if (cols[static_cast<std::size_t>(i)] == x_column) x_idx = i;
            if (cols[static_cast<std::size_t>(i)] == y_column) y_idx = i;
        }
    }
    if (x_idx < 0 || y_idx < 0) return curve; // Spalte fehlt -> ehrlich leer

    std::string line;
    while (std::getline(csv, line)) {
        if (line.empty() || line == "\r") continue;
        auto const cells   = detail::split_csv_line(line, delimiter);
        auto const max_idx = static_cast<std::ptrdiff_t>(cells.size());
        CurvePoint p;
        double     y  = 0.0;
        bool const ok = x_idx < max_idx && y_idx < max_idx &&
                        detail::parse_u64_cell(cells[static_cast<std::size_t>(x_idx)], p.x_working_set_bytes) &&
                        detail::parse_double_cell(cells[static_cast<std::size_t>(y_idx)], y);
        if (!ok) {
            ++curve.skipped_rows;
            continue;
        }
        p.y_value      = y;
        p.sample_count = 1;
        curve.points.push_back(p);
    }
    return curve;
}

} // namespace comdare::cache_engine::builder::curve_fit
