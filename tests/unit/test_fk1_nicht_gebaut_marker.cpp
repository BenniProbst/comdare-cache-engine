// tests/unit/test_fk1_nicht_gebaut_marker.cpp -- A15/FK-1 (Fehlerklassen-Framework #29), 02.08.2026.
//
// WAS HIER BEWIESEN WIRD. FK-1 schliesst die beiden letzten Naht-Luecken des 20260717-Designs:
//   L3  ein BAU-Fehler verschwand nur ins Log ("kein Mess-Eintrag (geloggt)"),
//   L4  ein LADE-/DOCK-Fehler hatte weder Log noch Zeile, nur den Aggregat-Zaehler load_failed.
// Owner-Freigabe des Zell-Tokens: Volles GO vom 02.08. auf die Bauplan-Empfehlung Q4
// ("nicht_gebaut", auswerte-sichtbar, analog "gesperrt").
//
// Der Kern ist wieder die DOMAENEN-TRENNUNG (W-4). Es sind jetzt DREI Nicht-Zahl-Aussagen, die eine
// Auswertung auseinanderhalten koennen muss:
//   failed        (D2) = GEMESSEN und dabei GESCHEITERT,
//   gesperrt      (D1) = baubar, aber NICHT ZUGELASSEN -- gar nicht erst gemessen,
//   nicht_gebaut  (D1) = es gibt gar keine Binary, der Bau selbst ist gescheitert.
// Wer zwei davon in dieselbe Vokabel presst, nimmt jeder spaeteren Auswertung die Unterscheidung.
//
//   Teil 1  TRENNUNG        -- die drei Tokens sind paarweise disjunkt, "nicht_gebaut" ist zementiert,
//                              und das D2-LOG-Etikett unterscheidet, was der Zell-Token bewusst nicht
//                              unterscheidet (n/a wegen "sinnlos" vs. n/a wegen "Quelle fehlt").
//   Teil 2  DEFAULT INERT   -- eine Zeile ohne gesetzten build_status rendert Zeichen fuer Zeichen wie
//                              vor dem Paket. Das ist die Byte-Aussage des Pakets.
//   Teil 3  MARKER-FORM     -- die Marker-Zeile entsteht aus DEMSELBEN Renderer (format_csv_row) wie
//                              jede andere Zeile und behaelt die EXAKT gleiche Spaltenzahl.
//   Teil 4  ABLEITUNGSWEG   -- und zwar FIXTURE-UNABHAENGIG geprueft: die Erwartung je Spalte wird aus
//                              lazy_csv_header() abgeleitet (Spalten-NAMEN), nicht aus einer im Test
//                              hartkodierten Feldzahl. Lehre "gruene Tests zementieren alte Ordnung":
//                              waechst das Schema um eine Mess-Spalte, muss sie hier automatisch das
//                              Token tragen -- ein handgezaehlter Test haette geschwiegen.
//   Teil 5  KEINE NULL      -- in der Marker-Zeile steht in keiner Mess-Spalte eine 0 ("Messung nie als
//                              Nullen"), und die Identitaets-/Lauf-Tags bleiben lesbar.
//   Teil 6  QUELLE-FEHLT    -- die Lade-/Dock-Zeile ist formgleich, traegt aber "n/a" (die Binary
//                              EXISTIERT, es fehlt die Mess-Quelle) und NIE "nicht_gebaut".
//   Teil 7  VORRANG         -- nicht_gebaut schlaegt gesperrt schlaegt failed: was nie gebaut wurde,
//                              kann weder zugelassen noch gemessen worden sein.
//   Teil 8  READER-PFLICHT  -- der Kurven-Leser verwirft das Token wie "failed"/"gesperrt": kein
//                              Phantom-Punkt; die NEGATIV-Richtung belegt, dass der Listen-Eintrag traegt.

#include "builder/experiment_tree/cache_engine_builder_iterator.hpp"
#include "heuristik/measurement_curve_loader.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace cem = ::comdare::cache_engine::measurement;

namespace {

int g_fail = 0;

template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

void check_true(char const* what, bool cond) {
    std::cout << (cond ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!cond) ++g_fail;
}

std::vector<std::string> felder(std::string zeile) {
    if (!zeile.empty() && zeile.back() == '\n') zeile.pop_back();
    std::vector<std::string> out;
    std::size_t              b = 0;
    for (std::size_t i = 0; i <= zeile.size(); ++i) {
        if (i == zeile.size() || zeile[i] == ';') {
            out.push_back(zeile.substr(b, i - b));
            b = i + 1;
        }
    }
    return out;
}

std::size_t token_zahl(std::string const& zeile, std::string_view token) {
    std::size_t n = 0;
    for (std::size_t pos = zeile.find(token); pos != std::string::npos; pos = zeile.find(token, pos + 1)) ++n;
    return n;
}

// Die Spalten, die NICHT aus der Messung stammen: Identitaet + die Lauf-/Selektions-Tags aus der
// Konfiguration. Sie bleiben in einer Marker-Zeile befuellt, damit die Zeile dem Lauf zuordenbar ist.
// Alles andere MUSS das Ersatz-Token tragen. Die Liste steht bewusst ueber SPALTEN-NAMEN (nicht ueber
// Positionen) -- eine neue Spalte am Ende faellt damit automatisch in die "muss Token tragen"-Haelfte.
std::set<std::string> const& tag_spalten() {
    static std::set<std::string> const s{"binary_id",      "series",        "sweep_axis",
                                         "working_set_n",  "platform",      "build_version",
                                         "pruefling_type", "fairness_mode", "h2_code_quality_score"};
    return s;
}

// Eine Zeile mit realistischen Identitaets-/Tag-Feldern -- genau die Felder, die make_marker_row im
// Iterator aus der Lauf-Konfiguration uebernimmt.
ex::LazyMeasuredRow marker_basis() {
    ex::LazyMeasuredRow row;
    row.binary_id      = "search_algo=art/memory_layout=soa";
    row.series         = "A";
    row.pruefling_type = "full";
    row.sweep_axis     = "migration_policy";
    row.working_set_n  = 131072;
    row.platform       = "linux-x86_64";
    row.build_version  = "m3v2";
    row.fairness_mode  = "native";
    row.h2_score       = "0.87";
    return row;
}

// Teil 4/5 in EINER fixture-unabhaengigen Pruefung: je Header-Spalte entweder ein Tag-Wert oder das
// erwartete Ersatz-Token -- und in KEINER Spalte eine nackte 0.
void pruefe_marker_zeile(char const* was, std::string const& zeile, std::string_view erwartetes_token) {
    auto const kopf   = felder(ex::lazy_csv_header());
    auto const zellen = felder(zeile);
    check_eq((std::string{was} + ": Spaltenzahl == Header-Spaltenzahl").c_str(), zellen.size(), kopf.size());
    if (zellen.size() != kopf.size()) return;
    std::size_t token_zellen = 0, tag_zellen = 0, null_zellen = 0;
    std::string erste_abweichung;
    for (std::size_t i = 0; i < kopf.size(); ++i) {
        bool const ist_tag = (tag_spalten().count(kopf[i]) != 0);
        if (ist_tag) {
            ++tag_zellen;
            continue;
        }
        if (zellen[i] == erwartetes_token)
            ++token_zellen;
        else if (erste_abweichung.empty())
            erste_abweichung = kopf[i] + "='" + zellen[i] + "'";
        if (zellen[i] == "0" || zellen[i] == "0.000") ++null_zellen;
    }
    check_eq((std::string{was} + ": Tag-Spalten (Identitaet/Lauf-Konfiguration)").c_str(), tag_zellen,
             tag_spalten().size());
    check_eq((std::string{was} + ": alle uebrigen Spalten tragen das Token").c_str(), token_zellen + tag_zellen,
             kopf.size());
    if (!erste_abweichung.empty()) std::cout << "         erste abweichende Spalte: " << erste_abweichung << "\n";
    check_eq((std::string{was} + ": KEINE nackte Null in der Zeile").c_str(), null_zellen, std::size_t{0});
}

} // namespace

int main() {
    std::cout << "\n== Teil 1: drei getrennte Nicht-Zahl-Vokabeln + ein unterscheidendes Log-Etikett ==\n";
    check_eq("das D1-Bau-Token", std::string{cem::build_cell_status_token(cem::BuildCellStatus::NichtGebaut)},
             std::string{"nicht_gebaut"});
    check_true("nicht_gebaut != failed (gemessen-und-gescheitert)",
               cem::build_cell_status_token(cem::BuildCellStatus::NichtGebaut) !=
                   cem::sample_status_token(cem::SampleStatus::Failed));
    check_true("nicht_gebaut != gesperrt (zulassungs-verweigert)",
               cem::build_cell_status_token(cem::BuildCellStatus::NichtGebaut) !=
                   cem::admission_status_token(cem::AdmissionStatus::Gesperrt));
    check_true("nicht_gebaut != n/a (Quelle fehlt)",
               cem::build_cell_status_token(cem::BuildCellStatus::NichtGebaut) !=
                   cem::sample_status_token(cem::SampleStatus::SourceUnavailable));
    check_eq("Bau-Status-Zahl", cem::kBuildCellStatusCount, std::size_t{2});
    check_eq("Gebaut ist 0 (Default-Init = rendern wie bisher)", static_cast<int>(cem::BuildCellStatus::Gebaut), 0);
    // Das Log muss unterscheiden koennen, was die ZELLE bewusst zusammenfasst.
    check_eq("Zell-Sicht bleibt zusammengefasst (beide 'n/a')",
             std::string{cem::sample_status_token(cem::SampleStatus::NotApplicable)},
             std::string{cem::sample_status_token(cem::SampleStatus::SourceUnavailable)});
    check_true("Log-Sicht differenziert (nicht_anwendbar vs quelle_nicht_verfuegbar)",
               cem::sample_status_label(cem::SampleStatus::NotApplicable) !=
                   cem::sample_status_label(cem::SampleStatus::SourceUnavailable));

    std::cout << "\n== Teil 2: der Default-Pfad ist byte-identisch (die Byte-Aussage) ==\n";
    ex::LazyMeasuredRow const ohne     = marker_basis();
    std::string const         zeile_ok = ex::format_csv_row(ohne);
    ex::LazyMeasuredRow       explizit = marker_basis();
    explizit.build_status              = cem::BuildCellStatus::Gebaut;
    check_true("Default == explizit Gebaut, Zeichen fuer Zeichen", zeile_ok == ex::format_csv_row(explizit));
    check_eq("die Default-Zeile traegt KEIN nicht_gebaut-Token", token_zahl(zeile_ok, "nicht_gebaut"), std::size_t{0});

    std::cout << "\n== Teil 3+4+5: die Marker-Zeile, gegen den HEADER abgeleitet ==\n";
    ex::LazyMeasuredRow marker     = marker_basis();
    marker.build_status            = cem::BuildCellStatus::NichtGebaut;
    std::string const zeile_marker = ex::format_csv_row(marker);
    pruefe_marker_zeile("nicht_gebaut-Zeile", zeile_marker, "nicht_gebaut");
    check_eq("gleiche Spaltenzahl wie eine gemessene Zeile", felder(zeile_marker).size(), felder(zeile_ok).size());
    check_true("die Identitaet steht unveraendert vorne", zeile_marker.rfind(marker.binary_id, 0) == 0);
    check_eq("kein failed in der Marker-Zeile", token_zahl(zeile_marker, "failed"), std::size_t{0});
    check_eq("kein gesperrt in der Marker-Zeile", token_zahl(zeile_marker, "gesperrt"), std::size_t{0});

    std::cout << "\n== Teil 6: die Lade-/Dock-Zeile sagt 'Quelle fehlt', nicht 'nie gebaut' ==\n";
    ex::LazyMeasuredRow quelle     = marker_basis();
    quelle.sample_status           = cem::SampleStatus::SourceUnavailable;
    std::string const zeile_quelle = ex::format_csv_row(quelle);
    pruefe_marker_zeile("SourceUnavailable-Zeile", zeile_quelle, "n/a");
    check_eq("die Lade-Zeile traegt NIE das Bau-Token", token_zahl(zeile_quelle, "nicht_gebaut"), std::size_t{0});
    check_true("und sie ist NICHT die Bau-Marker-Zeile", zeile_quelle != zeile_marker);

    std::cout << "\n== Teil 7: Vorrang nicht_gebaut > gesperrt > failed ==\n";
    ex::LazyMeasuredRow alles     = marker_basis();
    alles.build_status            = cem::BuildCellStatus::NichtGebaut;
    alles.admission_status        = cem::AdmissionStatus::Gesperrt; // widerspruechlich gesetzt
    alles.sample_status           = cem::SampleStatus::Failed;      // widerspruechlich gesetzt
    std::string const zeile_alles = ex::format_csv_row(alles);
    check_eq("nicht_gebaut gewinnt: kein gesperrt", token_zahl(zeile_alles, "gesperrt"), std::size_t{0});
    check_eq("nicht_gebaut gewinnt: kein failed", token_zahl(zeile_alles, "failed"), std::size_t{0});
    check_eq("die Zeile ist identisch zur reinen Marker-Zeile", zeile_alles, zeile_marker);

    std::cout << "\n== Teil 8: der Auswerte-Reader verwirft das Token wie failed/gesperrt ==\n";
    namespace heu = ::comdare::cache_engine::heuristik;
    heu::LoaderSpec const spec; // Default-Spalten + Default-na_tokens -- genau die Naht, die FK-1 erweitert

    std::istringstream mit_marker{"sweep_axis;binary_id;workload;working_set_n;ns_per_op\n"
                                  "achse;var;wl;1000;10.0\n"
                                  "achse;var;wl;2000;nicht_gebaut\n"
                                  "achse;var;wl;3000;failed\n"
                                  "achse;var;wl;4000;40.0\n"};
    auto const         geladen       = heu::load_curves(mit_marker, spec);
    std::size_t        punkte        = 0;
    std::uint64_t      uebersprungen = 0;
    for (auto const& [key, serie] : geladen) {
        punkte += serie.samples.size();
        uebersprungen += serie.skipped_rows;
    }
    check_eq("nur die zwei echten Punkte kommen an (kein Phantom-Punkt)", punkte, std::size_t{2});
    check_eq("die nicht_gebaut- UND die failed-Zeile werden gezaehlt uebersprungen", uebersprungen, std::uint64_t{2});

    // NEGATIV-Richtung: nimmt man das Token aus der Verwerf-Liste, muss die Zeile ueber den
    // Streng-Numerisch-Pfad fallen -- der Listen-Eintrag traegt also, er laeuft nicht bloss mit.
    heu::LoaderSpec ohne_token = spec;
    ohne_token.na_tokens.erase(
        std::remove(ohne_token.na_tokens.begin(), ohne_token.na_tokens.end(), std::string{"nicht_gebaut"}),
        ohne_token.na_tokens.end());
    check_eq("Gegenprobe: das Token stand wirklich in der Default-Liste", spec.na_tokens.size(),
             ohne_token.na_tokens.size() + 1);
    std::istringstream nochmal{"sweep_axis;binary_id;workload;working_set_n;ns_per_op\n"
                               "achse;var;wl;1000;10.0\n"
                               "achse;var;wl;2000;nicht_gebaut\n"
                               "achse;var;wl;4000;40.0\n"};
    auto const         ohne_liste        = heu::load_curves(nochmal, ohne_token);
    std::size_t        punkte_ohne_liste = 0;
    for (auto const& [key, serie] : ohne_liste) punkte_ohne_liste += serie.samples.size();
    check_eq("auch ohne den Listen-Eintrag bleiben es 2 Punkte (streng-numerisch faengt zusaetzlich ab)",
             punkte_ohne_liste, std::size_t{2});

    std::cout << "\n==== A15/FK-1 nicht_gebaut-Marker: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
