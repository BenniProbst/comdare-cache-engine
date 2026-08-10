// tests/unit/test_rf2_admission_marker_inert.cpp -- RF-2 (Kanon-Abschnitt 70.2), 26.07.2026.
// REGISTRIERT seit MT-L4 (09.08.2026) -- neben seinen CSV-Zeilen-Nachbarn test_a8s3_csv_klasse_c
// und test_m1hb_observer_zellen_ehrlich, die denselben Renderer pruefen.
//
// STAND 26.07. (ueberholt, Wortlaut erhalten): "Absichtlich NICHT in tests/unit/CMakeLists.txt
// (Sammel-Registrierung am Join, wie die Lane-C-Guards, der A9b-Guard und der RF-3-Guard); der
// Voll-ctest-Zaehler bleibt unveraendert."
// STAND 09.08.: der Join ist erfolgt. Der RF-3-Guard nebenan wurde am 02.08. nach demselben Muster
// nachregistriert; RF-2 blieb dabei liegen -- 14 Tage. WAS DAS GEKOSTET HAT, in einem Satz: der
// ERSTE Lauf dieser Datei nach der Registrierung war ROT, an genau EINER Zusicherung (Teil 3), und
// zwar seit dem Tag, an dem der Renderer eine vierte Zelle je Op-Art bekam. Ein unregistrierter Test
// altert lautlos mit seinem Pruefling; siehe die Begruendung an der reparierten Stelle unten.
// Ohne gtest bleibt es (eigenes main, Exit 0/1) -- comdare_add_test linkt gtest/gtest_main nur als
// Archiv, der main-tragende Member wird nicht gezogen. Der Hand-Bau bleibt moeglich; die unten
// notierte Zeile ist der Stand vom 26.07. und braucht heute ZWEI Wurzeln mehr (am 09.08. gemessen:
// -I <build>/generated fuer cache_engine/abi/overlay_source_hash_generated.hpp und
// -I libs/cache_engine/src fuer sha512/ctsha512.hpp -- beide kamen nach dem 26.07. dazu).
// Literal gelaufen (26.07.), ohne Zeilenfortsetzung notiert (-Wcomment):
//
//   g++ -std=c++23 -O0 -Wall -I libs/cache_engine -I libs/cache_engine/include -I libs/common/serialization -I libs/common -I cmake/third_party/boost_mp11/include tests/unit/test_rf2_admission_marker_inert.cpp -o <build>/test_rf2_guard
//
// WAS HIER BEWIESEN WIRD. RF-2 gibt einer D1-ZULASSUNGS-gesperrten Permutation einen eigenen, sichtbaren
// CSV-Datensatz. Owner-Begruendung (§70.2): die Hardware-Erweiterung spezialisiert die CPU-Spezifikation,
// jede untergeordnete Gesamtrekombination ist eine eigene Permutation und Messevaluation -- eine gesperrte
// Perm ist deshalb kein Leerraum, sondern ein Datenpunkt mit dem Inhalt "hier wurde nicht gemessen".
//
// Der Kern ist die DOMAENEN-TRENNUNG (W-4, Bauplan TEIL II): "failed" ist D2 und heisst GEMESSEN UND
// GESCHEITERT; "gesperrt" ist D1 und heisst GAR NICHT GEMESSEN. Wer beides in dasselbe Token presst,
// nimmt jeder spaeteren Auswertung die Moeglichkeit, die Faelle zu unterscheiden. Darum eine eigene
// Kleintaxonomie AdmissionStatus statt eines fuenften SampleStatus-Werts.
//
// WIE BEI A9b IST DER KANAL HEUTE UNAUSGELOEST: den Entscheider (simd_release_on_machine) bringt erst A6,
// und die Inventar-Zeile B (PERM-INVENTAR/perms_skipped) gehoert ebenfalls A6 -- beide existieren im Ist
// NICHT (gemessen: je 0 grep-Treffer). Dieser Guard beweist deshalb primaer die WIRKUNGSLOSIGKEIT:
//   Teil 1  TRENNUNG      -- die Tokens sind paarweise disjunkt zu ALLEN D2-Tokens, und "gesperrt" ist
//                            zementiert (der Reader fuehrt es woertlich in seiner Verwerf-Liste).
//   Teil 2  DEFAULT INERT -- eine Zeile ohne gesetzten admission_status rendert Zeichen fuer Zeichen wie
//                            vor dem Paket. Das ist die Byte-Aussage des Pakets.
//   Teil 3  MARKER-FORM   -- wird Gesperrt gesetzt, traegt die Zeile das D1-Token, behaelt aber die
//                            EXAKT gleiche Spaltenzahl und ihre Identitaets-Spalten (binary_id, setting).
//                            Die Zeile entsteht aus DEMSELBEN Renderer wie jede andere (format_csv_row),
//                            nie aus einer handgezaehlten Feldliste.
//   Teil 4  READER-PFLICHT-- der Kurven-Leser verwirft das Token wie "failed": kein Phantom-Punkt, die
//                            uebrigen Punkte bleiben unveraendert, skipped_rows zaehlt es.
//   Teil 5  VORRANG       -- ist eine Zeile gesperrt UND (widerspruechlich) Failed markiert, gewinnt D1:
//                            was nie gemessen wurde, kann nicht gescheitert sein.

#include "builder/experiment_tree/cache_engine_builder_iterator.hpp"
#include "heuristik/measurement_curve_loader.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace cem = ::comdare::cache_engine::measurement;

static int g_fail = 0;

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

namespace {

std::size_t feld_zahl(std::string const& zeile) {
    if (zeile.empty()) return 0;
    return static_cast<std::size_t>(std::count(zeile.begin(), zeile.end(), ';')) + 1;
}

std::size_t token_zahl(std::string const& zeile, std::string_view token) {
    std::size_t n = 0;
    for (std::size_t pos = zeile.find(token); pos != std::string::npos; pos = zeile.find(token, pos + 1)) ++n;
    return n;
}

// Eine Zeile mit realistischen Identitaets-Feldern; die Messwerte bleiben Default (das reicht: verglichen
// wird immer dieselbe Zeile mit und ohne gesetzten admission_status).
ex::LazyMeasuredRow basis_zeile() {
    ex::LazyMeasuredRow row;
    row.binary_id     = "search_algo=art/memory_layout=soa";
    row.setting_label = "concurrency.thread_count=4";
    row.setting_id    = row.binary_id + "#" + row.setting_label;
    row.n_ops         = 1000;
    row.total_ns      = 500000;
    row.timed_ops     = 2000;
    return row;
}

} // namespace

int main() {
    std::cout << "\n== Teil 1: D1 und D2 sind getrennte Vokabulare ==\n";
    check_eq("das D1-Token", std::string{cem::admission_status_token(cem::AdmissionStatus::Gesperrt)},
             std::string{"gesperrt"});
    check_true("gesperrt != failed (der eigentliche W-4-Punkt)",
               cem::admission_status_token(cem::AdmissionStatus::Gesperrt) !=
                   cem::sample_status_token(cem::SampleStatus::Failed));
    check_true("gesperrt kollidiert mit KEINEM D2-Token",
               cem::admission_status_token(cem::AdmissionStatus::Gesperrt) !=
                       cem::sample_status_token(cem::SampleStatus::Ok) &&
                   cem::admission_status_token(cem::AdmissionStatus::Gesperrt) !=
                       cem::sample_status_token(cem::SampleStatus::NotApplicable) &&
                   cem::admission_status_token(cem::AdmissionStatus::Gesperrt) !=
                       cem::sample_status_token(cem::SampleStatus::SourceUnavailable));
    check_eq("Zulassungs-Status-Zahl", cem::kAdmissionStatusCount, std::size_t{2});
    check_eq("Zugelassen ist 0 (Default-Init = messen wie bisher)", static_cast<int>(cem::AdmissionStatus::Zugelassen),
             0);

    std::cout << "\n== Teil 2: der Default-Pfad ist byte-identisch (die Byte-Aussage) ==\n";
    ex::LazyMeasuredRow const ohne     = basis_zeile();
    std::string const         zeile_ok = ex::format_csv_row(ohne);
    ex::LazyMeasuredRow       explizit = basis_zeile();
    explizit.admission_status          = cem::AdmissionStatus::Zugelassen;
    check_true("Default == explizit Zugelassen, Zeichen fuer Zeichen", zeile_ok == ex::format_csv_row(explizit));
    check_eq("die Default-Zeile traegt KEIN gesperrt-Token", token_zahl(zeile_ok, "gesperrt"), std::size_t{0});

    std::cout << "\n== Teil 3: die Marker-Zeile ist formgleich, nur anders befuellt ==\n";
    ex::LazyMeasuredRow marker     = basis_zeile();
    marker.admission_status        = cem::AdmissionStatus::Gesperrt;
    std::string const zeile_marker = ex::format_csv_row(marker);
    check_eq("gleiche Spaltenzahl wie eine gemessene Zeile", feld_zahl(zeile_marker), feld_zahl(zeile_ok));
    check_true("die Marker-Zeile traegt das D1-Token", token_zahl(zeile_marker, "gesperrt") > 0);
    // T-3, FREMDER NENNER: die Zahl der D1-Zellen steht NICHT mehr als Literal hier, sondern kommt aus
    // dem D2-PENDANT -- derselbe Renderer, der ANDERE Zweig. Genau das ist die Zusicherung, die
    // cache_engine_builder_iterator.hpp:734-735 im Klartext behauptet ("Derselbe Zell-Satz, dieselbe
    // Spaltenzahl wie beim D2-Pendant"); hier wird sie gemessen statt geglaubt.
    //
    // WARUM DIE ALTE FASSUNG FIEL (gemessen 09.08.2026, Erstlauf nach der Registrierung): sie stand auf
    // der festen Zahl 18 mit der Begruendung "6 Op-Arten x 3". Der Renderer schreibt inzwischen eine
    // VIERTE Zelle je Op-Art -- op_<art>_p999_ns, Block (C2) bei :954-965, zusaetzlich zu den drei des
    // op_*-Blocks bei :741-765. Die Wahrheit ist 24; die Zusicherung war seit dem Tag dieser Erweiterung
    // falsch. Sie ist niemandem aufgefallen, weil die Datei unregistriert war und nie gebaut wurde --
    // das ist der eigentliche Befund von MT-L4 und der Grund, warum eine Zahl, die der Pruefling selbst
    // veraendern kann, nicht als Literal in den Test gehoert.
    ex::LazyMeasuredRow d2_pendant = basis_zeile();
    d2_pendant.sample_status       = cem::SampleStatus::Failed;
    std::string const zeile_d2     = ex::format_csv_row(d2_pendant);
    std::size_t const d1_zellen    = token_zahl(zeile_marker, "gesperrt");
    check_eq("D1 belegt GENAU so viele Zellen wie D2 im Pendant (Nenner aus dem anderen Zweig)", d1_zellen,
             token_zahl(zeile_d2, "failed"));
    // Zweiter, UNABHAENGIGER Anker -- gegen den Fall, dass beide Zweige gemeinsam auf null fallen und der
    // Vergleich oben dann trivial gilt (V-8: der Zustand, in dem die Aussage erscheint und die Sache
    // trotzdem nicht existiert). Die Op-Art-Zahl kommt aus dem Objekt; nur der Faktor 4 steht hier, und
    // er ist die Aussage: 3 Zellen im op_*-Block (n/p50/p99) + 1 Tail-Zelle (p999).
    check_eq("und das sind 4 Zellen je Op-Art (3 im op_*-Block + 1 Tail-p999)", d1_zellen,
             std::size_t{4} * marker.op_lat.size());
    check_true("beide Anker sind nicht trivial null (sonst waere der Vergleich oben wertlos)", d1_zellen > 0);
    check_true("die Identitaet bleibt lesbar (binary_id + setting stehen unveraendert vorne)",
               zeile_marker.rfind(marker.binary_id, 0) == 0 &&
                   zeile_marker.find(marker.setting_label) != std::string::npos);
    check_eq("kein failed in der Marker-Zeile (D1 statt D2)", token_zahl(zeile_marker, "failed"), std::size_t{0});

    std::cout << "\n== Teil 4: der Auswerte-Reader verwirft das Token wie failed ==\n";
    namespace heu = ::comdare::cache_engine::heuristik;
    heu::LoaderSpec const spec; // Default-Spalten + Default-na_tokens -- genau die Naht, die RF-2 erweitert

    // Minimal-CSV im Loader-Schema: zwei echte Punkte, dazwischen eine gesperrte und eine failed-Zeile.
    std::istringstream mit_marker{"sweep_axis;binary_id;workload;working_set_n;ns_per_op\n"
                                  "achse;var;wl;1000;10.0\n"
                                  "achse;var;wl;2000;gesperrt\n"
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
    check_eq("die gesperrte UND die failed-Zeile werden gezaehlt uebersprungen", uebersprungen, std::uint64_t{2});

    // Gegenprobe: OHNE die gesperrte Zeile ergibt sich dieselbe Punktmenge -> das Token fuegt der Kurve
    // nichts hinzu und nimmt ihr nichts weg, es wird nur sauber verworfen.
    std::istringstream ohne_marker_csv{"sweep_axis;binary_id;workload;working_set_n;ns_per_op\n"
                                       "achse;var;wl;1000;10.0\n"
                                       "achse;var;wl;3000;failed\n"
                                       "achse;var;wl;4000;40.0\n"};
    auto const         ohne_marker = heu::load_curves(ohne_marker_csv, spec);
    std::size_t        punkte_ohne = 0;
    for (auto const& [key, serie] : ohne_marker) punkte_ohne += serie.samples.size();
    check_eq("dieselbe Punktmenge wie ohne die Marker-Zeile", punkte, punkte_ohne);

    // NEGATIV-Richtung: nimmt man das Token aus der Verwerf-Liste, entsteht genau der Phantom-Punkt, den
    // die Reader-Pflicht verhindern soll -- damit ist belegt, dass der Listen-Eintrag traegt und nicht
    // bloss mitlaeuft.
    heu::LoaderSpec ohne_token = spec;
    ohne_token.na_tokens.erase(
        std::remove(ohne_token.na_tokens.begin(), ohne_token.na_tokens.end(), std::string{"gesperrt"}),
        ohne_token.na_tokens.end());
    std::istringstream nochmal{"sweep_axis;binary_id;workload;working_set_n;ns_per_op\n"
                               "achse;var;wl;1000;10.0\n"
                               "achse;var;wl;2000;gesperrt\n"
                               "achse;var;wl;4000;40.0\n"};
    auto const         ohne_liste        = heu::load_curves(nochmal, ohne_token);
    std::size_t        punkte_ohne_liste = 0;
    for (auto const& [key, serie] : ohne_liste) punkte_ohne_liste += serie.samples.size();
    check_eq("ohne den Listen-Eintrag bleibt es bei 2 Punkten (streng-numerisch faengt es zusaetzlich ab)",
             punkte_ohne_liste, std::size_t{2});

    std::cout << "\n== Teil 5: D1 hat Vorrang vor D2 ==\n";
    ex::LazyMeasuredRow beides     = basis_zeile();
    beides.admission_status        = cem::AdmissionStatus::Gesperrt;
    beides.sample_status           = cem::SampleStatus::Failed; // widerspruechlich gesetzt
    std::string const zeile_beides = ex::format_csv_row(beides);
    check_eq("gesperrt gewinnt: keine failed-Zelle", token_zahl(zeile_beides, "failed"), std::size_t{0});
    check_eq("die Zeile ist identisch zur reinen Marker-Zeile", zeile_beides, zeile_marker);

    std::cout << "\n==== RF-2 Zulassungs-Marker INERT: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
