#pragma once
// AXIS_ALGO_VERSION: 2
// v1 -> v2 (2026-08-06, Lead-Entscheid): die Verwerf-Liste na_tokens hat sich seit dem letzten
// Lock-Regen (e016eccc, 19.07.) zweimal SEMANTISCH erweitert -- 150b0ede (26.07.) um das
// D1-Zulassungs-Token "gesperrt", 0fdeccff (02.08.) um das D1-Bau-Token "nicht_gebaut". Beide
// aendern, WELCHE CSV-Zeilen zu Kurvenpunkten werden, also das Verhalten des Laders. Der Bump
// wurde damals nicht gesetzt, weil der Tripwire-Job contract:axis-version-lock durch einen
// doppelten YAML-Schluessel seit dem 19.07. faktisch abgeschaltet war (siehe .gitlab-ci.yml,
// Block contract:axis-version-lock) -- die Drift kam also nicht durch, weil sie erlaubt war,
// sondern weil niemand hinsah. Hier nachgezogen, damit das Lock die Wahrheit sagt.
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
// wird GEZAEHLT uebersprungen (skipped_rows), NIE zu einem Phantom-Punkt (0,0). RF-2 (§70.2) ergaenzt das
// D1-Token "gesperrt" (admission_status_token) -- ebenfalls keine Zahl, aber aus der ANDEREN Domaene:
// nicht gemessen statt Messung gescheitert. Streng-numerisches Parsen
// (ganze Zelle muss konsumiert werden) -- gleiche Doktrin wie builder/curve_fit/curve_fit.hpp.
//
// HONEST-EMPTY: fehlende Spalte / leere Datei -> leeres Ergebnis (der Aufrufer erkennt es an .empty()).

#include <cache_engine/measurement/csv_cell_reader.hpp>

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
    std::string axis_col     = "sweep_axis";    // Gruppen-Dim "achse"   (leer -> "-")
    std::string variant_col  = "binary_id";     // Gruppen-Dim "variante"(leer -> "-")
    std::string workload_col = "workload";      // Gruppen-Dim "workload"(leer -> "-")
    std::string x_col        = "working_set_n"; // Parameter-Achse
    std::string y_col        = "ns_per_op";     // Messwert
    // Ehrliche Nicht-Zahl-Tokens (K-10) -> Zeile wird uebersprungen. Leere Zelle zaehlt ebenfalls als n/a.
    // RF-2 (§70.2): "gesperrt" ist das D1-Zulassungs-Token (axis_error.hpp :: admission_status_token) und
    // gehoert AUS DEMSELBEN GRUND hierher wie "failed" -- es ist keine Zahl. Semantisch ist es aber etwas
    // anderes: "failed" heisst gemessen-und-gescheitert, "gesperrt" heisst gar-nicht-erst-gemessen. Fuer
    // die Kurve ist beides gleich zu behandeln (kein Phantom-Punkt); wer die beiden Faelle TRENNEN will,
    // liest die Marker-Zeile ueber ihr eigenes Token, nicht ueber diese Liste.
    // A15/FK-1 (Owner-Q4 per Volles-GO 02.08.): "nicht_gebaut" ist das D1-BAU-Token (axis_error.hpp ::
    // build_cell_status_token) und gehoert aus demselben Grund hierher wie "failed" und "gesperrt" -- es
    // ist keine Zahl. Fehlte es in dieser Liste, wuerde jede nicht gebaute Binary als Phantom-Punkt in die
    // Kurve wandern (der Grund, warum die Marker-Zeile ueberhaupt ein eigenes Vokabular bekommt).
    // Semantisch sind es DREI verschiedene Aussagen: failed = gemessen und gescheitert, gesperrt =
    // nicht zugelassen, nicht_gebaut = es gibt gar keine Binary. Fuer die KURVE ist alles drei gleich zu
    // behandeln (kein Punkt); wer sie TRENNEN will, liest die Marker-Zeile ueber ihr eigenes Token.
    std::vector<std::string> na_tokens = {"n/a", "failed", "gesperrt", "nicht_gebaut", "-", ""};
};

/// Default-Spezifikationen fuer die zwei realen CSV-Dialekte des ce.
[[nodiscard]] inline LoaderSpec wide_lazy_spec() { return LoaderSpec{}; } // Default = WIDE (Semikolon)
[[nodiscard]] inline LoaderSpec snapshot_spec() {
    LoaderSpec s;
    s.delimiter    = ',';
    s.axis_col     = ""; // Snapshot-CSV traegt keine eigene Achsen-Spalte
    s.variant_col  = "permutation_id";
    s.workload_col = "workload_used";
    s.x_col        = "op_count";
    s.y_col        = "total_cycles";
    return s;
}

/// Gruppen-Schluessel (achse, variante, workload). Ordenbar -> deterministische Ausgabe-Reihenfolge.
struct GroupKey {
    std::string        axis     = "-";
    std::string        variant  = "-";
    std::string        workload = "-";
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

// Zell-Split, streng-numerischer Zellen-Parser, Header-Index (linear), Zell-Fallback: Single-Source
// in cache_engine/measurement/csv_cell_reader.hpp (Extraktion 2026-08-08, vormals hier und in
// builder/curve_fit/curve_fit.hpp byte-identisch dupliziert -- "identische Doktrin wie curve_fit"
// stand schon im alten Kommentar, ohne dass die Duplikation aufgeloest war). `using` haelt die
// Aufrufstellen unten woertlich unveraendert; AXIS_ALGO_VERSION bleibt unveraendert, weil diese
// Extraktion die SEMANTIK (welche Zeilen zu Kurvenpunkten werden) nicht beruehrt.
using ::comdare::cache_engine::measurement::csv::cell_or_dash;
using ::comdare::cache_engine::measurement::csv::col_index;
using ::comdare::cache_engine::measurement::csv::is_na;
using ::comdare::cache_engine::measurement::csv::parse_double_cell;
using ::comdare::cache_engine::measurement::csv::split_csv_line;

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
        std::vector<std::string> const cells   = loader_detail::split_csv_line(line, spec.delimiter);
        auto const                     max_idx = static_cast<std::ptrdiff_t>(cells.size());

        GroupKey key;
        key.axis                  = loader_detail::cell_or_dash(cells, ax_idx);
        key.variant               = loader_detail::cell_or_dash(cells, va_idx);
        key.workload              = loader_detail::cell_or_dash(cells, wl_idx);
        MeasurementSeries& series = out[key];

        if (x_idx >= max_idx || y_idx >= max_idx) { // unvollstaendige Zeile
            ++series.skipped_rows;
            continue;
        }
        std::string const& xc = cells[static_cast<std::size_t>(x_idx)];
        std::string const& yc = cells[static_cast<std::size_t>(y_idx)];
        double             x = 0.0, y = 0.0;
        bool const         x_ok = !loader_detail::is_na(xc, spec.na_tokens) && loader_detail::parse_double_cell(xc, x);
        bool const         y_ok = !loader_detail::is_na(yc, spec.na_tokens) && loader_detail::parse_double_cell(yc, y);
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
