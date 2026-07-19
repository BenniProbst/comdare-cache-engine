#pragma once
// AXIS_ALGO_VERSION: 1
// heuristik/measurement_curve_loader.hpp -- laedt (x,y)-Reihen je (achse,variante,workload)-Gruppe aus
// einer realen Messungs-/Result-CSV. PAKET W3-C (Ledger Sec.32-F8). Header-only, kein Python.
//
// SPALTEN-WAHRHEIT (aus dem ce gelesen -- KEINE erfundenen Namen):
//   * WIDE-Pfad (builder/experiment_tree/cache_engine_builder_iterator.hpp :: lazy_csv_header):
//       SEMIKOLON-separiert; u.a. "binary_id;setting;repetition;n_ops;total_ns;ns_per_op;..." und ganz
//       hinten "...;workload;...;series;sweep_axis;working_set_n;platform;build_version;...". Reale
//       Fixture-Beispiele: tests/unit/fixtures/best_binary_cells.csv (binary_id;ns_per_op;...;workload;
//       working_set_n;...) und tests/unit/thesis_tiere/tier150_measurements.csv.
//       -> Gruppe = (sweep_axis, binary_id, workload); x = working_set_n; y = ns_per_op.
//   * SNAPSHOT-Pfad (builder/measurement_snapshot.hpp :: serialize_measurements_csv):
//       KOMMA-separiert; "permutation_id,fingerprint,succeeded,workload_used,op_count,total_cycles,...".
//       -> Gruppe = (-, permutation_id, workload_used); x = op_count; y = total_cycles.
//
// ROBUST gegen n/a-Zellen (K-10-Token, measurement/axis_error.hpp :: sample_status_token): die ehrlichen
// Zell-Tokens "n/a" (NotApplicable/SourceUnavailable) und "failed" (Failed) sind KEINE Zahlen -> die Zeile
// wird GEZAEHLT uebersprungen (skipped_rows), NIE zu einem Phantom-Punkt (0,0). Streng-numerisches Parsen
// (ganze Zelle muss konsumiert werden) -- gleiche Doktrin wie builder/curve_fit/curve_fit.hpp.
//
// HONEST-EMPTY: fehlende Spalte / leere Datei -> leeres Ergebnis (der Aufrufer erkennt es an .empty()).

#include "axis_spline.hpp" // CurveSample (Single-Source des Sample-Typs)

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <istream>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace comdare::cache_engine::heuristik {

/// Loader-Spezifikation: Dialekt (Trennzeichen) + reale Spaltennamen der drei Gruppen-Dimensionen und
/// der x/y-Achse + die n/a-Tokens. Fehlt eine Gruppen-Spalte im Header, faellt ihre Dimension auf "-"
/// (ehrlich, kein Absturz). x/y MUESSEN existieren, sonst leeres Ergebnis.
struct LoaderSpec {
    char        delimiter    = ';';
    std::string axis_col     = "sweep_axis";   // Gruppen-Dim "achse"   (leer -> "-")
    std::string variant_col  = "binary_id";    // Gruppen-Dim "variante"(leer -> "-")
    std::string workload_col = "workload";     // Gruppen-Dim "workload"(leer -> "-")
    std::string x_col        = "working_set_n"; // Parameter-Achse
    std::string y_col        = "ns_per_op";     // Messwert
    // Ehrliche Nicht-Zahl-Tokens (K-10) -> Zeile wird uebersprungen. Leere Zelle zaehlt ebenfalls als n/a.
    std::vector<std::string> na_tokens = {"n/a", "failed", "-", ""};
};

/// Default-Spezifikationen fuer die zwei realen CSV-Dialekte des ce.
[[nodiscard]] inline LoaderSpec wide_lazy_spec() { return LoaderSpec{}; } // Default = WIDE (Semikolon)
[[nodiscard]] inline LoaderSpec snapshot_spec() {
    LoaderSpec s;
    s.delimiter    = ',';
    s.axis_col     = "";              // Snapshot-CSV traegt keine eigene Achsen-Spalte
    s.variant_col  = "permutation_id";
    s.workload_col = "workload_used";
    s.x_col        = "op_count";
    s.y_col        = "total_cycles";
    return s;
}

/// Gruppen-Schluessel (achse, variante, workload). Ordenbar -> deterministische Ausgabe-Reihenfolge.
struct GroupKey {
    std::string axis     = "-";
    std::string variant  = "-";
    std::string workload = "-";
    [[nodiscard]] bool operator<(GroupKey const& o) const {
        return std::tie(axis, variant, workload) < std::tie(o.axis, o.variant, o.workload);
    }
    [[nodiscard]] bool operator==(GroupKey const& o) const {
        return axis == o.axis && variant == o.variant && workload == o.workload;
    }
};

/// Eine geladene (x,y)-Reihe einer Gruppe + Diagnose (uebersprungene Zeilen).
struct MeasurementSeries {
    std::vector<CurveSample> samples;
    std::uint64_t            skipped_rows = 0; // n/a / non-numerisch / unvollstaendig
};

namespace loader_detail {

/// CSV-Zell-Split mit minimalem RFC-4180-Quoting + CRLF-Strip (identische Doktrin wie curve_fit).
[[nodiscard]] inline std::vector<std::string> split_csv_line(std::string_view line, char delimiter) {
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    std::vector<std::string> cells;
    std::string              cell;
    bool                     in_quotes = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        char const c = line[i];
        if (c == '"') {
            if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
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

/// Streng-numerisches Zellen-Parsen: die GANZE Zelle muss konsumiert werden (kein stiller Phantom-Punkt).
[[nodiscard]] inline bool parse_double_cell(std::string const& cell, double& out) {
    if (cell.empty()) return false;
    char*        end = nullptr;
    double const v   = std::strtod(cell.c_str(), &end);
    if (end == nullptr || *end != '\0' || !std::isfinite(v)) return false; // NaN/Inf -> kein Punkt
    out = v;
    return true;
}

[[nodiscard]] inline bool is_na(std::string const& cell, std::vector<std::string> const& na_tokens) {
    for (std::string const& t : na_tokens)
        if (cell == t) return true;
    return false;
}

/// Header-Index einer Spalte; -1 wenn Name leer oder nicht vorhanden.
[[nodiscard]] inline std::ptrdiff_t col_index(std::vector<std::string> const& header, std::string const& name) {
    if (name.empty()) return -1;
    for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(header.size()); ++i)
        if (header[static_cast<std::size_t>(i)] == name) return i;
    return -1;
}

/// Zell-Wert an idx oder Default-Fallback "-" (idx<0 = Spalte fehlt bzw. leer konfiguriert).
[[nodiscard]] inline std::string cell_or_dash(std::vector<std::string> const& cells, std::ptrdiff_t idx) {
    if (idx < 0 || idx >= static_cast<std::ptrdiff_t>(cells.size())) return "-";
    std::string const& v = cells[static_cast<std::size_t>(idx)];
    return v.empty() ? std::string{"-"} : v;
}

} // namespace loader_detail

/// Laedt alle (x,y)-Reihen je (achse,variante,workload)-Gruppe aus einem CSV-Stream. Fehlende x/y-Spalte
/// -> leeres Ergebnis. n/a-/failed-/nicht-numerische x- oder y-Zellen -> Zeile uebersprungen (skipped_rows
/// je Gruppe). Innerhalb einer Gruppe bleibt die Datei-Reihenfolge erhalten (AxisSpline::build sortiert
/// spaeter deterministisch nach x und aggregiert Duplikate).
[[nodiscard]] inline std::map<GroupKey, MeasurementSeries> load_curves(std::istream& csv, LoaderSpec const& spec) {
    std::map<GroupKey, MeasurementSeries> out;

    std::string header_line;
    if (!std::getline(csv, header_line)) return out; // leere Datei -> ehrlich leer
    std::vector<std::string> const header = loader_detail::split_csv_line(header_line, spec.delimiter);

    std::ptrdiff_t const ax_idx = loader_detail::col_index(header, spec.axis_col);
    std::ptrdiff_t const va_idx = loader_detail::col_index(header, spec.variant_col);
    std::ptrdiff_t const wl_idx = loader_detail::col_index(header, spec.workload_col);
    std::ptrdiff_t const x_idx  = loader_detail::col_index(header, spec.x_col);
    std::ptrdiff_t const y_idx  = loader_detail::col_index(header, spec.y_col);
    if (x_idx < 0 || y_idx < 0) return out; // Pflicht-Spalten fehlen -> ehrlich leer

    std::string line;
    while (std::getline(csv, line)) {
        if (line.empty() || line == "\r") continue;
        std::vector<std::string> const cells = loader_detail::split_csv_line(line, spec.delimiter);
        auto const max_idx = static_cast<std::ptrdiff_t>(cells.size());

        GroupKey key;
        key.axis     = loader_detail::cell_or_dash(cells, ax_idx);
        key.variant  = loader_detail::cell_or_dash(cells, va_idx);
        key.workload = loader_detail::cell_or_dash(cells, wl_idx);
        MeasurementSeries& series = out[key];

        if (x_idx >= max_idx || y_idx >= max_idx) { // unvollstaendige Zeile
            ++series.skipped_rows;
            continue;
        }
        std::string const& xc = cells[static_cast<std::size_t>(x_idx)];
        std::string const& yc = cells[static_cast<std::size_t>(y_idx)];
        double             x = 0.0, y = 0.0;
        bool const         x_ok = !loader_detail::is_na(xc, spec.na_tokens) &&
                          loader_detail::parse_double_cell(xc, x);
        bool const y_ok = !loader_detail::is_na(yc, spec.na_tokens) &&
                          loader_detail::parse_double_cell(yc, y);
        if (!x_ok || !y_ok) { // n/a / failed / nicht-numerisch -> NIE Phantom-Punkt
            ++series.skipped_rows;
            continue;
        }
        series.samples.push_back(CurveSample{x, y});
    }
    return out;
}

/// Konvenienz: baue direkt Splines je Gruppe (HONEST-EMPTY -> Gruppen mit < 2 Punkten fallen heraus).
template <InterpolationStrategy Strategy = MonotoneCubicHermiteStrategy>
[[nodiscard]] std::map<GroupKey, AxisSpline<Strategy>> build_axis_splines(std::istream& csv, LoaderSpec const& spec) {
    std::map<GroupKey, AxisSpline<Strategy>> out;
    for (auto& [key, series] : load_curves(csv, spec)) {
        auto sp = AxisSpline<Strategy>::build(series.samples);
        if (sp.has_value()) out.emplace(key, std::move(*sp));
    }
    return out;
}

} // namespace comdare::cache_engine::heuristik
