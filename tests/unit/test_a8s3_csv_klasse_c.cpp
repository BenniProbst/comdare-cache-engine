// test_a8s3_csv_klasse_c -- A8-S3 (2026-08-04): WACHE der HOST-seitigen CSV-Klasse-C-Spalten.
//
// KLASSE C (Qualitaets-Parameter-Katalog Abschnitt 3, Triage E2/E3 -> A8-S3) = Spalten-Ereignisse am HOST:
// sie bewegen KEIN Wire-Byte, sondern haengen ADDITIV hinten an das WIDE-Schema (lazy_csv_header). Vier
// Bloecke sind in dieser Scheibe entstanden:
//   (C1) pmc_branch_misses  -- Befund B8/Katalog P11: PmcCounters ERHOB branch_misses real
//        (pmc_source.hpp), der 7er-pmc-Block emittierte es als EINZIGES Feld nicht. "Geschrieben, aber
//        stumm" -- dieselbe Klasse wie die T7-Schema-Luecke.
//   (C2) op_<art>_p999_ns   -- Tail-Perzentile aus DENSELBEN IST-Vektoren wie p50/p99. Kern-Groesse fuer
//        den T6-Alloc-Tail und das T16-eager/lazy-Pareto, fuer die p99 zu grob ist.
//   (C3) alloc_bytes_in_use_peak / alloc_external_frag_milli / alloc_internal_frag_milli -- die drei
//        T6-Groessen, fuer die es am Ist KEINE erhobene Quelle gibt (Entscheide E2/E9, Befund B7). Sie
//        werden ehrlich als "n/a" gerendert (G3-Regel: nie eine stille 0, nie ein Momentanwert unter
//        Peak-Etikett) -- ueber die EINE D2-Taxonomie sample_status_token(SourceUnavailable), nicht ueber
//        ein neu erfundenes Vokabular.
//   (C4) CSV-Legende der stat_*-Achsen-Semantik (Kommentar-Block ueber lazy_csv_header) -- nicht testbar,
//        aber hier benannt, damit die Wache den ganzen Klasse-C-Satz dokumentiert.
//
// WAS DIESE TU FESTHAELT:
//   * STRUKTUR: Header und Zeile haben dieselbe Zellenzahl. Ein halb nachgezogener END-Append (Spalte im
//     Header, Zelle vergessen) verschoebe ALLE Spalten der Auswertung -- der teuerste denkbare stille
//     Fehler. Geprueft wird die Gleichheit, nie eine Literal-Spaltenzahl.
//   * NAMEN: jede neue Spalte wird NACH NAMEN gesucht (nie positional).
//   * WERTE: pmc_branch_misses traegt den realen Zaehler; p999 >= p99 (Nearest-Rank-Monotonie).
//   * EHRLICHKEIT: die drei T6-Spalten sind "n/a", NIE "0"; und in einer failed-Zeile tragen die
//     p999-Spalten "failed" statt einer stillen Null (dieselbe Ersatz-Kaskade wie der op_*-Bestandsblock).
//
// Build: Standalone int main() (kein gtest), reiner Host-Pfad -- kein Tier, keine Achsen-Instanz.

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>
#include <cache_engine/measurement/axis_error.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace cem = ::comdare::cache_engine::measurement;

int g_fail = 0;

void tr(std::string const& was, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << "\n";
    if (!ok) ++g_fail;
}

[[nodiscard]] std::vector<std::string> split_semicolon(std::string_view line) {
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.remove_suffix(1);
    std::vector<std::string> out;
    std::size_t              begin = 0;
    for (std::size_t i = 0; i <= line.size(); ++i) {
        if (i == line.size() || line[i] == ';') {
            out.emplace_back(line.substr(begin, i - begin));
            begin = i + 1;
        }
    }
    return out;
}

[[nodiscard]] std::size_t column_index(std::vector<std::string> const& header, std::string_view name) {
    for (std::size_t i = 0; i < header.size(); ++i)
        if (header[i] == name) return i;
    return header.size();
}

/// Liest die Zelle einer NACH NAMEN gesuchten Spalte; leerer string = Spalte fehlt.
[[nodiscard]] std::string cell(std::vector<std::string> const& header, std::vector<std::string> const& cells,
                               std::string_view name) {
    std::size_t const i = column_index(header, name);
    if (i >= header.size() || i >= cells.size()) return {};
    return cells[i];
}

/// Eine vollstaendig gefuellte Mess-Zeile mit reproduzierbaren Host-Werten (kein Tier noetig).
[[nodiscard]] ex::LazyMeasuredRow probe_row() {
    ex::LazyMeasuredRow row;
    row.binary_id         = "a8s3_klasse_c_probe";
    row.setting_id        = row.binary_id;
    row.unified_real      = true;
    row.total_ns          = 1'000'000;
    row.n_ops             = 1000;
    row.timed_ops         = 2000;
    row.pmc.available     = true;
    row.pmc.branch_misses = 4711; // der reale Zaehler, der bisher nicht emittiert wurde
    for (std::size_t k = 0; k < row.op_lat.size(); ++k) {
        row.op_lat[k].n       = 1000 + k;
        row.op_lat[k].p50_ns  = 100 + static_cast<std::int64_t>(k);
        row.op_lat[k].p99_ns  = 900 + static_cast<std::int64_t>(k);
        row.op_lat[k].p999_ns = 9000 + static_cast<std::int64_t>(k);
    }
    return row;
}

} // namespace

int main() {
    std::cout << "==== A8-S3: CSV-Klasse-C (Host-Spalten, kein Wire-Ereignis) ====\n";

    std::vector<std::string> const header = split_semicolon(ex::lazy_csv_header());
    ex::LazyMeasuredRow const      row    = probe_row();
    std::vector<std::string> const cells  = split_semicolon(ex::format_csv_row(row));

    std::cout << "    Spalten im Header = " << header.size() << "  Zellen in der Zeile = " << cells.size() << "\n";
    tr("W1 Struktur: Header und Zeile haben dieselbe Zellenzahl (kein halb nachgezogener END-Append)",
       header.size() == cells.size());

    // -- W2/W3 (C1): die real erhobene, bisher stumme PMC-Spalte.
    std::string const bm = cell(header, cells, "pmc_branch_misses");
    std::cout << "    pmc_branch_misses = '" << bm << "'\n";
    tr("W2 (C1) Spalte 'pmc_branch_misses' existiert", column_index(header, "pmc_branch_misses") < header.size());
    tr("W3 (C1) sie traegt den REALEN PmcCounters-Wert (4711), nicht 0", bm == "4711");

    // -- W4 (C2): p999 je Op-Art, Reihenfolge/Namen aus derselben Single-Source wie p50/p99.
    bool alle_da = true, monoton = true;
    for (std::size_t k = 0; k < ex::kOpKindNames.size(); ++k) {
        std::string const name99  = std::string{"op_"} + ex::kOpKindNames[k] + "_p99_ns";
        std::string const name999 = std::string{"op_"} + ex::kOpKindNames[k] + "_p999_ns";
        std::string const c99     = cell(header, cells, name99);
        std::string const c999    = cell(header, cells, name999);
        if (c999.empty()) alle_da = false;
        if (c99.empty() || c999.empty() || std::stoll(c999) < std::stoll(c99)) monoton = false;
    }
    tr("W4a (C2) alle op_<art>_p999_ns-Spalten existieren (Reihenfolge kOpKindNames)", alle_da);
    tr("W4b (C2) p999 >= p99 je Op-Art (Nearest-Rank-Monotonie)", monoton);

    // -- W5 (C3): die drei T6-Ehrlichkeits-Spalten. "n/a" aus der EINEN D2-Taxonomie, NIE "0".
    std::string const na{cem::sample_status_token(cem::SampleStatus::SourceUnavailable)};
    bool              t6_ehrlich = true;
    for (char const* n : {"alloc_bytes_in_use_peak", "alloc_external_frag_milli", "alloc_internal_frag_milli"}) {
        std::string const c = cell(header, cells, n);
        std::cout << "    " << n << " = '" << c << "'\n";
        if (c != na) t6_ehrlich = false;
    }
    tr("W5 (C3) die drei nicht erhobenen T6-Groessen stehen als '" + na + "' (G3-Regel), nie als '0'", t6_ehrlich);

    // -- W6: eine FAILED-Zeile darf in den neuen Tail-Spalten keine stille Null zeigen.
    ex::LazyMeasuredRow failed_row    = probe_row();
    failed_row.sample_status          = cem::SampleStatus::Failed;
    std::vector<std::string> const fc = split_semicolon(ex::format_csv_row(failed_row));
    std::string const              failed_tok{cem::sample_status_token(cem::SampleStatus::Failed)};
    bool                           failed_ok = (fc.size() == header.size());
    for (std::size_t k = 0; k < ex::kOpKindNames.size(); ++k) {
        std::string const name999 = std::string{"op_"} + ex::kOpKindNames[k] + "_p999_ns";
        if (cell(header, fc, name999) != failed_tok) failed_ok = false;
    }
    tr("W6 failed-Zeile: op_<art>_p999_ns == '" + failed_tok + "' (dieselbe Ersatz-Kaskade wie p50/p99)", failed_ok);

    if (g_fail == 0)
        std::cout << "==== A8-S3 CSV-Klasse-C-Wache: ALLE OK ====\n";
    else
        std::cout << "==== A8-S3 CSV-Klasse-C-Wache: " << g_fail << " FEHLER ====\n";
    return g_fail == 0 ? 0 : 1;
}
