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

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <istream>
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
