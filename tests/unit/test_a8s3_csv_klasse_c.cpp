// test_a8s3_csv_klasse_c -- A8-S3 (2026-08-04): WACHE der HOST-seitigen CSV-Klasse-C-Spalten.
//
// KLASSE C (Qualitaets-Parameter-Katalog Abschnitt 3, Triage E2/E3 -> A8-S3) = Spalten-Ereignisse am HOST:
// sie bewegen KEIN Wire-Byte, sondern haengen ADDITIV hinten an das WIDE-Schema (lazy_csv_header). Vier
// Bloecke sind in dieser Scheibe entstanden:
//   (C1) pmc_branch_misses  -- Befund B8/Katalog P11: die Spalte war "geschrieben, aber stumm" (Header
//        trug sie nicht). Zwischenstand 2026-08-06 (B5/M-2-KORREKTUR-2): die urspruengliche Zusage
//        "PmcCounters ERHOB branch_misses real" war falsch, die Spalte war honest-0, und dieser Test
//        pruefte nur das Plumbing. SEIT M-3a (2026-08-07) IST SIE REAL: LinuxPerfPmcSource oeffnet
//        PERF_TYPE_HARDWARE / PERF_COUNT_HW_BRANCH_MISSES als vierten Zaehler, das Feld traegt ein eigenes
//        branch_misses_source_available, und die Zelle rendert ueber pmc_zelle -- also mit demselben
//        Ehrlichkeits-Vertrag wie l2/l3/coherence/energy. Der Test prueft entsprechend nicht mehr nur
//        Durchreichung, sondern die Unterscheidung "echte 0" (W14) gegen "Quelle nicht da" (W13).
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
//   * QUELLEN-EHRLICHKEIT je PMC-Zaehler (W7-W14): fuer l3, l2, coherence, energy UND seit M-3a
//     branch_misses steht IMMER das Paar nebeneinander -- Quelle offen + real 0 => "0"; Quelle nie
//     geoeffnet => "n/a". Nur so ist gezeigt (statt behauptet), dass das Flag die Unterscheidung traegt
//     und nicht der Zahlenwert.
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
    // M-3a (2026-08-07): seit branch_misses REAL erhoben wird (LinuxPerfPmcSource, PERF_TYPE_HARDWARE /
    // PERF_COUNT_HW_BRANCH_MISSES), rendert die Zelle ueber pmc_zelle und verlangt den Quellen-Beleg.
    // Eine Probe-Zeile, die einen Zaehlerwert OHNE Beleg setzt, ist genau der Fall, der jetzt 'n/a' wird --
    // die Standard-Probe stellt deshalb den Beleg mit (der Gegenfall steht als W13 unten).
    row.pmc.branch_misses_source_available = true;
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

    // -- W7-W10 (B5/M-2-KORREKTUR-2, 2026-08-06): die PMC-Zeilenehrlichkeit fuer cache_misses_l2/l3 und
    //    coherence_invalidations -- derselbe #156-De-Risk-7er-Block wie pmc_branch_misses' Nachbarn, nicht
    //    Klasse C, aber dieselbe Test-Infrastruktur passt am besten (probe_row/cell/format_csv_row).
    //    Der Kern: ein Zaehler, der real 0 misst, bleibt "0"; ein Zaehler, dessen Quelle nie geoeffnet
    //    wurde, wird "n/a" -- beide Faelle nebeneinander, damit die Unterscheidung nicht nur behauptet,
    //    sondern gezeigt wird.
    {
        // W7: l3 WURDE geoeffnet (source_available=true) und misst REAL 0 -- muss "0" bleiben, nicht "n/a".
        ex::LazyMeasuredRow real_zero_row                  = probe_row();
        real_zero_row.pmc.cache_misses_l3                  = 0;
        real_zero_row.pmc.cache_misses_l3_source_available = true;
        std::vector<std::string> const rz                  = split_semicolon(ex::format_csv_row(real_zero_row));
        std::string const              l3_rz               = cell(header, rz, "pmc_cache_misses_l3");
        std::cout << "    W7 pmc_cache_misses_l3 (Quelle offen, real 0) = '" << l3_rz << "'\n";
        tr("W7 eine ECHTE 0 (Quelle offen) bleibt '0', wird NIE zu '" + na + "'", l3_rz == "0");

        // W8: l3 wurde NIE geoeffnet (source_available bleibt Default false) -- muss 'n/a' werden, NICHT '0',
        // obwohl das POD-Feld selbst ebenfalls 0 ist (die Unterscheidung ist das Flag, nicht der Zahlenwert).
        ex::LazyMeasuredRow unavailable_row  = probe_row(); // cache_misses_l3_source_available bleibt Default false
        std::vector<std::string> const ua    = split_semicolon(ex::format_csv_row(unavailable_row));
        std::string const              l3_ua = cell(header, ua, "pmc_cache_misses_l3");
        std::cout << "    W8 pmc_cache_misses_l3 (Quelle nie geoeffnet) = '" << l3_ua << "'\n";
        tr("W8 eine NIE geoeffnete Quelle wird '" + na + "', nie eine erfundene '0'", l3_ua == na);

        // W9: l2 und coherence_invalidations gehen denselben Weg wie l3 (keine heutige Quelle oeffnet sie je).
        std::string const l2_ua  = cell(header, ua, "pmc_cache_misses_l2");
        std::string const coh_ua = cell(header, ua, "pmc_coherence_invalidations");
        std::cout << "    W9 pmc_cache_misses_l2='" << l2_ua << "' pmc_coherence_invalidations='" << coh_ua << "'\n";
        tr("W9 l2 UND coherence_invalidations folgen demselben Weg wie l3 ('" + na + "')", l2_ua == na && coh_ua == na);

        // W10 (#83 C-1(c), Owner-GO KON103 17.08.2026 -- UMSCHRIEB des frueheren Pins): die PMC-OFF-Zeile
        // (available=false, NullPmcSource-Analogon) trug bis #83 die "0-Konvention" -- alle acht PMC-Zellen
        // lasen "0", ununterscheidbar von einer echten Nullmessung. Der Owner hat die Empfehlung woertlich
        // angenommen ("die ERGEBNIS-Seite wird hart: die Zeile traegt den Zaehler oder einen ehrlichen
        // Status-Token, nie eine stille 0"). Ein Teil-Lauf ohne PMC bleibt buchbar, aber AUSDRUECKLICH
        // gekennzeichnet: pmc_available=0 UND Token statt Zahl in jeder nicht erhobenen Zelle.
        ex::LazyMeasuredRow pmc_off_row       = probe_row();
        pmc_off_row.pmc.available             = false;
        std::vector<std::string> const off    = split_semicolon(ex::format_csv_row(pmc_off_row));
        std::string const              l3_off = cell(header, off, "pmc_cache_misses_l3");
        std::string const              av_off = cell(header, off, "pmc_available");
        std::cout << "    W10 pmc_cache_misses_l3 (pmc_available=0, PMC-off-Analogon) = '" << l3_off << "'\n";
        tr("W10 PMC-off-Zeile traegt Token statt stiller 0 (#83: Teil-Lauf ausdruecklich gekennzeichnet)",
           l3_off == na);
        tr("W10b die Kennzeichnung pmc_available=0 bleibt dabei erhalten (Teil-Lauf, kein Voll-Lauf)", av_off == "0");

        // W11/W12 (B5/M-2-KORREKTUR-3, Owner-KERN "stiller Rueckfall ist verboten"): energy_micro_joules
        // folgt demselben Muster wie l2/l3/coherence -- ein best-effort-RAPL-Fehlschlag (kein Root, keine
        // Zone) darf nicht dieselbe '0' zeigen wie ein echtes Null-Delta.
        ex::LazyMeasuredRow real_energy_row                      = probe_row();
        real_energy_row.pmc.energy_micro_joules                  = 0;
        real_energy_row.pmc.energy_micro_joules_source_available = true;
        std::vector<std::string> const en_rz                     = split_semicolon(ex::format_csv_row(real_energy_row));
        std::string const              en_real                   = cell(header, en_rz, "pmc_energy_micro_joules");
        std::cout << "    W11 pmc_energy_micro_joules (Quelle gelesen, real 0) = '" << en_real << "'\n";
        tr("W11 ein ECHTES Null-Delta (RAPL gelesen) bleibt '0', wird NIE zu '" + na + "'", en_real == "0");

        std::string const en_ua = cell(header, ua, "pmc_energy_micro_joules"); // ua = unavailable_row von oben
        std::cout << "    W12 pmc_energy_micro_joules (RAPL nie gelesen) = '" << en_ua << "'\n";
        tr("W12 ein NIE gelesenes RAPL-Delta wird '" + na + "', nie eine erfundene '0'", en_ua == na);

        // W13/W14 (M-3a, 2026-08-07): branch_misses ist seit diesem Paket kein honest-0-Feld mehr, sondern
        // ein real erhobener Zaehler mit eigenem Quellen-Flag. Er faellt damit unter DENSELBEN Vertrag wie
        // l2/l3/coherence/energy -- und wird hier mit BEIDEN Faellen nebeneinander gezeigt, damit die
        // Unterscheidung nicht nur behauptet ist. W3 oben deckt bereits den Normalfall "Quelle offen,
        // Wert != 0" ab; hier kommen die beiden Grenzfaelle dazu.
        //
        // W13: die Quelle wurde NIE geoeffnet (z.B. perf_event_open scheitert) -- 'n/a', NICHT '0'. Das ist
        // exakt der Zustand, den die Spalte vor M-3a strukturell nicht ausdruecken konnte: sie stand als
        // nackte 0 da, ununterscheidbar von einer echten Nullmessung.
        ex::LazyMeasuredRow bm_unavail_row                = probe_row();
        bm_unavail_row.pmc.branch_misses                  = 0;
        bm_unavail_row.pmc.branch_misses_source_available = false;
        std::vector<std::string> const bmu                = split_semicolon(ex::format_csv_row(bm_unavail_row));
        std::string const              bm_ua              = cell(header, bmu, "pmc_branch_misses");
        std::cout << "    W13 pmc_branch_misses (Quelle nie geoeffnet) = '" << bm_ua << "'\n";
        tr("W13 (M-3a) eine NIE geoeffnete branch-Quelle wird '" + na + "', nie eine erfundene '0'", bm_ua == na);

        // W14: die Quelle WAR offen und hat real 0 gemessen (ein Zweig ohne Fehlvorhersagen ist moeglich) --
        // muss '0' bleiben. Kein Token darf einen realen Nullbefund verdecken.
        ex::LazyMeasuredRow bm_real_zero_row                = probe_row();
        bm_real_zero_row.pmc.branch_misses                  = 0;
        bm_real_zero_row.pmc.branch_misses_source_available = true;
        std::vector<std::string> const bmz                  = split_semicolon(ex::format_csv_row(bm_real_zero_row));
        std::string const              bm_rz                = cell(header, bmz, "pmc_branch_misses");
        std::cout << "    W14 pmc_branch_misses (Quelle offen, real 0) = '" << bm_rz << "'\n";
        tr("W14 (M-3a) eine ECHTE 0 (branch-Quelle offen) bleibt '0', wird NIE zu '" + na + "'", bm_rz == "0");
    }

    if (g_fail == 0)
        std::cout << "==== A8-S3 CSV-Klasse-C-Wache: ALLE OK ====\n";
    else
        std::cout << "==== A8-S3 CSV-Klasse-C-Wache: " << g_fail << " FEHLER ====\n";
    return g_fail == 0 ? 0 : 1;
}
