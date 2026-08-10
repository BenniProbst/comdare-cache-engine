#pragma once
// tools/mess_report/realm_scan.hpp -- A9-S4: der --realm-root-Verzeichnis-Scan.
//
// Findet Mess-Ergebnis-CSVs unter einer Wurzel. Es gibt ZWEI reale Ablageformen, und der Scan trifft
// BEIDE ueber GENAU EINE Entscheidungsstelle (namens_form(), s.u.):
//
//   PRODUKTIONSFORM  <binary_stamm>/result.csv   -- was ein frischer Lauf hinterlaesst. Der Erzeuger
//                    schreibt sie als `bin_dir / "result.csv"`, ein Verzeichnis je Binary-Stamm
//                    (builder/experiment_tree/cache_engine_builder_iterator.hpp:2950). Der Name ist
//                    FEST, der Stamm steckt im VERZEICHNIS.
//   ARCHIVFORM       per_binary/<binary_id>.result.csv -- was das Erstbeleg-Archiv traegt (real:
//                    super/docs/architektur/measurement/erstbeleg-d03-20260726/.../per_binary/,
//                    8 Dateien). Hier steckt der Stamm im DATEINAMEN.
//
// D3-6 (2026-08-10) -- WARUM DAS HIER STEHT: bis zu diesem Datum kannte der Scan NUR den Suffix
// ".result.csv". "result.csv" ist 10 Zeichen, ".result.csv" ist 11 -- der Suffix-Vergleich konnte die
// Produktionsform RECHNERISCH NIE treffen, die beiden Muster waren DISJUNKT. Ein --realm-root auf
// einen frischen Produktionslauf fand null Dateien und meldete "enthaelt keine *.result.csv", ohne
// jeden Nenner: von "nichts da" war "falsch gesucht" nicht zu unterscheiden. Dieselbe Fehlerklasse
// wie beim anhang:forward-Selektor in super (ci/anhang_forward_core.sh, AF_RESULT_NAMEN) -- dort wie
// hier ist die Heilung dieselbe: EIN Muster, das beide Formen fuehrt, an EINER Stelle, und ein
// Nenner in der AUSGABE (scan_bilanz(), s.u.), damit eine Null nie wieder unbelegt bleibt.
//
// Eine etwaige Aggregat-Datei ("measurements.csv" im Erstbeleg-Archiv, dieselben Zeilen wie die Summe
// aller per_binary-Dateien) wird vom Verzeichnis-Scan bewusst NICHT mitgenommen -- sonst zaehlte jede
// Zeile doppelt, einmal aus der Aggregat-Datei und einmal aus ihrer per-Binary-Quelle. Wer die
// Aggregat-Sicht statt der aufgesplitteten Quellen rendern will, benennt sie explizit ueber
// --csv=<pfad>. Genau deshalb ist die Produktionsform ein EXAKTER Namensvergleich und kein zweiter
// Glob: "result.csv" trifft die per-Binary-Quelle und nichts sonst.
//
// ".stale"-BESTAENDE WERDEN WEDER GELESEN NOCH ANGEFASST (A9-Doc Abschnitt 8: "Messdaten nie
// loeschen: ... invalidiert nie; .stale-Bestaende werden weder gelesen noch angefasst"). Der Scan
// prueft einen ENDUNGS-Vergleich (".stale" als Suffix, dokumentiert in
// builder/bestandslog/lager_pfad_grammatik.hpp:75 "Suffixe, die spaetere Schichten anhaengen") --
// bewusst ANDERS als profile_facade/planner/planner_status_types.hpp::kResultStaleName (dort ein
// EXAKTER Dateiname "result.csv.stale" fuer einen fixen Stamm; hier tragen die Dateien
// unterschiedliche Staemme wie "<binary_id>.result.csv", also passt fuer SIE nur der Suffix-Vergleich;
// die Produktionsform traegt den festen Namen und wird exakt verglichen -- beide Wege stehen in
// namens_form(), keiner davon ein zweites Mal irgendwo sonst).
//
// BOUNDED SCAN (dieselbe Doktrin wie planner_status_reader.hpp: Tiefen-Kappe + Eintrags-Kappe): ein
// vom Anwender benannter Baum, den das Kommando nicht kontrolliert, darf es nicht unbegrenzt
// festhalten (versehentlich verschachtelt / zyklischer Symlink). Greift die Kappe dennoch, sagt der
// Bericht es LITERAL (scan_gekappt=true) statt still eine zu kleine Bilanz zu melden.

// Der Produktions-Dateiname kommt NICHT aus einer eigenen Abschrift, sondern aus der Stelle, die ihn
// bereits fuehrt (planner_status_types.hpp: "Die drei Datei-Namen des per-Binary-Mess-Resume-Standes").
// Damit gibt es EINEN Namen und nicht zwei, die auseinanderlaufen koennen. Der Header ist header-only
// und stdlib-only ("ANSPRUCHSLOS", eigener Kopf-Kommentar) -- die Self-Contained-Zusage dieser
// tools/-Header bleibt unberuehrt.
#include "../../libs/cache_engine/profile_facade/planner/planner_status_types.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace comdare::cache_engine::tools::mess_report {

inline constexpr std::string_view kStaleSuffix      = ".stale";
inline constexpr std::string_view kResultCsvSuffix  = ".result.csv";
/// Die Produktionsform -- EXAKTER Name, uebernommen von der Stelle, die ihn ohnehin fuehrt.
inline constexpr std::string_view kResultCsvExakt = ::comdare::cache_engine::planner::kResultCsvName;
/// Der Selektor als EIN Text, fuer die Ausgabe: wer eine Null liest, sieht daneben, wonach gesucht wurde.
/// (Gegenstueck zu AF_RESULT_NAMEN in super/ci/anhang_forward_core.sh -- dieselbe Zusage, andere Sprache.)
inline constexpr std::string_view kSelektorText    = "result.csv, *.result.csv";
inline constexpr std::size_t      kMaxScanTiefe     = 8;
inline constexpr std::size_t      kMaxScanEintraege = 200000;

[[nodiscard]] inline bool endet_mit(std::string_view s, std::string_view suffix) noexcept {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/// Wozu ein Dateiname gehoert. TOTAL ueber alle Namen -- jeder Eintrag faellt in genau eine Klasse.
enum class NamensForm {
    Unbeteiligt, ///< keine Mess-CSV (Stempel, Notiz, Aggregat "measurements.csv", ...)
    Stale,       ///< ent-wertetes Rohdatum: wird GEZAEHLT, nie gelesen, nie angefasst
    Produktion,  ///< <binary_stamm>/result.csv  -- was ein frischer Lauf schreibt
    Archiv       ///< per_binary/<binary_id>.result.csv -- was das Erstbeleg-Archiv traegt
};

/// DIE EINE ENTSCHEIDUNGSSTELLE des Selektors. Es gibt im ganzen Werkzeug keine zweite Stelle, die
/// ueber Zugehoerigkeit entscheidet -- wer die Formen aendert, aendert GENAU DIESE Funktion.
///
/// WAS DEN AUSSCHLUSS WIRKLICH TRAEGT (nachgemessen 2026-08-10, nicht behauptet): die drei Klassen
/// sind ueber den realen Namensraum DISJUNKT -- fuer "result.csv.stale", "<stamm>.result.csv.stale",
/// "result.csv.stamp", "result.csv.bak" und "measurements.csv" trifft je hoechstens EIN Zweig zu. Die
/// Reihenfolge ist deshalb heute KEIN Vertrag, sondern nur ein Guertel: sie wuerde erst tragen, wenn
/// ein Zweig je aufgeweicht wird.
///
/// TRAGEND ist dagegen der EXAKTE Vergleich der Produktionsform. "result.csv.stale" und
/// "result.csv.stamp" beginnen beide mit "result.csv" -- ein bequemes starts_with() statt des
/// Gleichheits-Vergleichs wuerde ein ent-wertetes Rohdatum und einen Resume-Stempel als frische
/// Messung einlesen. Genau dieser Griff ist in tests/unit/test_d3_6_realm_selektor_produktionsablage.cpp
/// (Fall 4) als Gegeneingang gedeckt.
[[nodiscard]] inline NamensForm namens_form(std::string_view name) noexcept {
    if (endet_mit(name, kStaleSuffix)) return NamensForm::Stale;
    if (name == kResultCsvExakt) return NamensForm::Produktion;
    if (endet_mit(name, kResultCsvSuffix)) return NamensForm::Archiv;
    return NamensForm::Unbeteiligt;
}

struct RealmScanErgebnis {
    std::vector<std::filesystem::path> gefundene_csvs; // sortiert (Determinismus, LED-61 diff==0)
    std::size_t                        besuchte_eintraege  = 0;
    std::size_t                        uebersprungen_stale = 0;
    /// D3-6: die Fundmenge AUFGESCHLUESSELT nach Ablageform. Eine Gesamtzahl allein verbirgt genau den
    /// Zustand, der ein halbes Jahr gehalten hat -- "gefunden=7" saehe auf einem gemischten Baum gesund
    /// aus, auch wenn davon 0 aus der Produktionsform stammen und der Selektor sie gar nicht sieht.
    std::size_t gefunden_produktionsform = 0;
    std::size_t gefunden_archivform      = 0;
    bool        wurzel_vorhanden         = false;
    bool        scan_gekappt             = false;
};

/// DER NENNER IN DER AUSGABE (V-1): eine Zahl dieses Scans darf das Werkzeug nie ohne ihre
/// Grundgesamtheit verlassen. "0 gefunden" ist erst dann eine Aussage, wenn danebensteht, wie viele
/// Eintraege ueberhaupt angesehen wurden und wonach gesucht wurde.
[[nodiscard]] inline std::string scan_bilanz(RealmScanErgebnis const& e) {
    return "besuchte_eintraege=" + std::to_string(e.besuchte_eintraege) +
           " gefunden=" + std::to_string(e.gefundene_csvs.size()) +
           " (produktionsform=" + std::to_string(e.gefunden_produktionsform) +
           " archivform=" + std::to_string(e.gefunden_archivform) + ")" +
           " uebersprungen_stale=" + std::to_string(e.uebersprungen_stale) +
           " scan_gekappt=" + (e.scan_gekappt ? "ja" : "nein") + " (Selektor: " + std::string{kSelektorText} + ")";
}

/// Scanned `root` rekursiv (bounded) nach Mess-Ergebnis-CSVs. `.stale`-Dateien werden GEZAEHLT, aber
/// NICHT in `gefundene_csvs` aufgenommen -- der Aufrufer darf sie damit strukturell nicht erreichen.
[[nodiscard]] inline RealmScanErgebnis scanne_realm_root(std::filesystem::path const& root) {
    RealmScanErgebnis erg;
    if (root.empty()) return erg;
    std::error_code ec;
    erg.wurzel_vorhanden = std::filesystem::exists(root, ec) && !ec;
    if (!erg.wurzel_vorhanden) return erg; // ehrlich leer, kein Wurf -- Aufrufer meldet fehlende Wurzel

    std::filesystem::recursive_directory_iterator it(root, std::filesystem::directory_options::skip_permission_denied,
                                                     ec);
    if (ec) return erg;
    std::filesystem::recursive_directory_iterator const ende;

    while (it != ende) {
        if (erg.besuchte_eintraege >= kMaxScanEintraege) {
            erg.scan_gekappt = true;
            break;
        }
        ++erg.besuchte_eintraege;

        if (static_cast<std::size_t>(it.depth()) + 1 >= kMaxScanTiefe) {
            // Tiefen-Kappe erreicht: dieser Eintrag wird noch besucht, aber NICHT mehr abgestiegen.
            std::error_code disable_ec;
            it.disable_recursion_pending();
            (void)disable_ec;
            erg.scan_gekappt = true;
        }

        std::error_code reg_ec;
        bool const      ist_datei = it->is_regular_file(reg_ec);
        if (!reg_ec && ist_datei) {
            std::string const name = it->path().filename().string();
            switch (namens_form(name)) {
                case NamensForm::Stale: ++erg.uebersprungen_stale; break;
                case NamensForm::Produktion:
                    ++erg.gefunden_produktionsform;
                    erg.gefundene_csvs.push_back(it->path());
                    break;
                case NamensForm::Archiv:
                    ++erg.gefunden_archivform;
                    erg.gefundene_csvs.push_back(it->path());
                    break;
                case NamensForm::Unbeteiligt: break;
            }
        }

        std::error_code inc_ec;
        it.increment(inc_ec);
        if (inc_ec) break;
    }

    std::sort(erg.gefundene_csvs.begin(), erg.gefundene_csvs.end());
    return erg;
}

} // namespace comdare::cache_engine::tools::mess_report
