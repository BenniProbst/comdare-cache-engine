#pragma once
// ergebnis_mappe_naht.hpp -- A9-S5: die EINE Naht, an der aus EINEM echten Mess-Lauf EINE echte
// Ergebnis-Mappe entsteht.
//
// WARUM ES DIESE DATEI GIBT (Bestandsaufnahme 2026-08-09, am Objekt gemessen): der xlsx-Writer
// (builder/lager_ablage/) ist seit A9-S3 vollstaendig gebaut -- und hatte NULL Produktions-Aufrufer.
// `grep -rn "ErgebnisMappenFactory"` traf ausschliesslich die Implementierung selbst und drei
// Test-Ziele; `grep -rn 'method value="xlsx"'` traf in KEINEM getrackten Profil (Gegenprobe: dieselbe
// Regex gegen "csv" traf in 8 Profilen -- die Null liegt am Bestand, nicht am Suchmuster). Der
// Owner-KERN 26.07. ("xlsx = kuenftig DEFAULT, CSV einstellbar + Fallback") war damit an der einzigen
// Stelle unwirksam, an der er zaehlt: im Lauf. Diese Datei schliesst genau diese Luecke -- ADDITIV.
//
// ADDITIV HIESS WOERTLICH (A9-S5-Aera, bis 2026-08-21): der bestehende rohe CSV-Pfad (std::ofstream
// csv{a.out_csv...}) wurde NICHT angefasst; das ERSETZEN war "ein spaeterer, Owner-freizugebender
// Schritt". DIESER SCHRITT IST GEFALLEN -- S13-01 (#18/D-1, KON32-01; Design
// 20260817-DESIGN-s13-buendel-di25.md 4.1): die offizielle CSV ist seither die PROJEKTION der Mappe
// (projektion_oeffnen/projektion_ok unten), der rohe Parallel-Strom in den beiden Seams ist ENTFERNT.
// golden bleibt byte-identisch -- nicht weil der alte Weg stehen blieb, sondern weil die Projektion
// dieselben Bytes aus denselben Feldern formt (Beweis: test_s13_01_csv_kind_projektion, Byte-Orakel).
//
// RICHTUNG (Owner 2026-08-09 16:31, woertlich): "die csv wird doch aus der xlsx gebildet, IMMER. es
// wird nur entweder xlsx oder csv oder BEIDE auf Platte gesichert, was auf dem RAM liegt ist etwas
// voellig anderes." Daraus folgen die drei Saetze, die Abschnitt 4 traegt:
//   (1) DIE MAPPE ENTSTEHT IMMER. Die xlsx wird bedingungslos im Speicher gebaut -- auch bei einem
//       Profil, das ausschliesslich csv persistiert. Sie ist der STAMM.
//   (2) DIE CSV IST IHR KIND. Eine Zeile erreicht die csv-Ausgabe NUR, wenn der Stamm sie zuvor
//       angenommen hat. Bricht die xlsx-Erzeugung, ist auch die csv ungueltig.
//   (3) PERSISTENZ IST EINE EIGENE FRAGE. <writeback_methods> entscheidet allein, WAS auf die Platte
//       geht -- nicht, WAS im Speicher entsteht.
//
// KORREKTUR-VERMERK (2026-08-10): bis hierher stand an dieser Stelle "Die Mappe wird aus DENSELBEN
// In-Memory-Zeilen gespeist, aus denen auch die rohe CSV entsteht" -- also GESCHWISTER. Genau diese
// Fassung hat der Owner am 09.08. 16:31 zurueckgewiesen; sie war in den Code geschrieben worden,
// BEVOR die Korrektur fiel (Commits d2e20e7c/f82707bc/d5e2da59, alle vor 16:31), und blieb seither
// unveraendert stehen. Sie ist hiermit ersetzt -- in der BEGRUENDUNG und im BAU (Abschnitt 4).
//
// ZAHLEN-WIDERRUF (2026-08-10) zu Commit 579ec9f1: dessen Commit-Text sagt "Bei einem reinen
// csv-Profil (9 von 12 getrackten Profilen, darunter das golden-Profil ...)". BEIDE Zahlen sind
// falsch. Nachgemessen am Bestand (Schleife ueber algorithm_profiles/thesis_profiles/*.profile.xml,
// je Datei <writeback_methods> / method value="csv" / method value="xlsx" ausgezaehlt):
//     11 Profile gesamt -- 8 mit <writeback_methods> UND csv, 3 ganz OHNE <writeback_methods>
//                          (base_pilot, m3v2_smoke, wdk_fairness_example), 0 mit xlsx.
// Ausserhalb von thesis_profiles/ traegt KEINE .profile.xml einen <writeback_methods>-Block --
// "getrackt" bezeichnet also genau diese 11 Dateien. Die Zeile 8-9 oben ("in 8 Profilen") ist damit
// die richtige Teilzahl und bleibt. Der Commit-Text selbst wird NICHT umgeschrieben: 579ec9f1 ist
// auf origin/development bereits von weiteren Commits ueberbaut, und die Projekt-Doktrin verbietet
// das Umschreiben geteilter Historie. Der Widerruf lebt deshalb HIER, im Artefakt -- nach demselben
// Muster wie der Vermerk darueber, und nicht nur in einem Bericht, den spaeter niemand liest.
// STAND-VERMERK (2026-08-21, S13-02): die Zaehlung darueber ist HISTORISCH (Messdatum 10.08.).
// Seit ce fbe48f99 (14.08.) tragen 11/11 getrackte Profile einen Block; seit S13-02 traegt
// m3v2_smoke zusaetzlich csv (Leser-Erhebung: die super-Wachen lesen seine measurements.csv).
// Die LEBENDE Zaehlung erhebt test_w0b_durchstich_kette (G1) am Bestand -- nie diese Zeilen.
//
// I/O-LAGE (Contention-Doktrin, Design-Dossier 20260803-a9_xlsx_writer_f3_soll_design.md V-A9-4):
// im Mess-Fenster nimmt der STAMM Zeilen nur ENTGEGEN (libxlsxwriter haelt sie im Speicher, kein
// Datei-Zugriff); der EINZIGE Datei-Schreibvorgang der Mappe liegt in schliessen(), das der Aufrufer
// NACH der Mess-Schleife ruft -- neben csv.flush(). Das KIND (CsvErgebnisMappe) streamt dagegen
// bereits waehrend des Laufs in seine .tmp-Dateien (ergebnis_mappe.hpp:430-435) -- der Satz oben gilt
// also fuer den Stamm, nicht fuer die Ableitung. Das ist ehrlich vermerkt, nicht wegdefiniert.
//
// DOKTRIN: C++23, header-only, ASCII-only. Kein std::variant, kein Laufzeit-switch ueber Achsen --
// die Format-Wahl ist der EINE bewusste Laufzeit-Draht der Abstract Factory (ergebnis_mappe.hpp sagt
// das ueber sich selbst) und keine Achsen-Auffaecherung.

#include <builder/bestandslog/lager_pfad_grammatik.hpp>
#include <builder/lager_ablage/ergebnis_mappe.hpp>

#include <cache_engine/measurement/writeback_method_registry.hpp>

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>  // S13-01: der Projektions-Strom der offiziellen CSV
#include <iostream> // S13-03: die per-Binary-Mappen-Ansage des Fassaden-Sinks
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace comdare::cache_engine::lager_naht {

namespace lab = ::comdare::cache_engine::builder::lager_ablage;
namespace bl  = ::comdare::cache_engine::builder::bestandslog;
namespace ms  = ::comdare::cache_engine::measurement;

// =================================================================================================
// 1. FORMAT-ENTSCHEIDUNG -- eines ODER BEIDE aus {csv,xlsx}
// =================================================================================================
//
// OWNER-ENTSCHEID 2026-08-09, woertlich: "xlsx ist der Standard und CSV ist waehlbar, und ich lege
// jetzt fest, dass AUCH BEIDE ZUSAMMEN waehlbar sein koennen. NICHT NUR ENTWEDER ODER."
// Damit faellt die exactly-one-Regel ueber der FORMAT-Teilmenge, die die erste Fassung dieser Datei
// noch fuehrte (Nachtrag 05.08.: "entweder CSV xor xlsx"). ES GILT SEITHER:
//     leer / fehlend / nur Kanal-Token  ->  xlsx           (Default, UNVERAENDERT)
//     nur csv                           ->  csv
//     nur xlsx                          ->  xlsx
//     csv UND xlsx                      ->  BEIDE          (gueltiger Eingang, KEIN Fehler)
//
// WAS VOM 05.08. UNVERAENDERT BLEIBT -- nur die Ausschliesslichkeit ist aufgehoben, nicht der Rest:
// es ist ein STRATEGY PATTERN, xlsx ist der Default, und es gibt KEINE Chain-of-Responsibility-
// Weiterreichung. Jenes Verbot richtete sich gegen "unnoetigen overhead", also gegen ZWEI
// UNABHAENGIGE SCHREIBWEGE, die dieselbe Sache zweimal erzeugen. Zwei Ausgaben aus EINER Mappe sind
// das nicht: MappenNaht baut die Mappe EINMAL -- ein Zeitstempel, ein Dateiname-Stamm, EIN
// Feld-Split je Zeile -- und reicht DIESELBEN Felder an die aktiven Strategien weiter (Abschnitt 4).
//
// DIE FALLE, die hier WEITERHIN NICHT gebaut wird -- und die durch den Entscheid nicht kleiner,
// sondern groesser geworden ist: die exactly-one-Regel von <run_methodology> (validate_profile.hpp
// :480/:1296: "if (run_methodology.size() > 1) ... GENAU EINE erlaubt") laesst sich NICHT auf
// <writeback_methods> uebertragen -- weder wortwoertlich noch auf die Format-Teilmenge verengt.
// Gemessen am Objekt: all_axes_golden.profile.xml:216-220 -- das Profil, das .gitlab-ci.yml:93
// woertlich "das golden-Profil der Abgabe-Messung" nennt -- deklariert DREI Eintraege:
//     <method value="csv"/> <method value="latex_table"/> <method value="comparison_metrics"/>
// Eine Regel `writeback_methods.size() > 1 => Fehler` braeche also exakt die Abgabe-Messung.
//
// Der Grund ist semantisch, nicht kosmetisch: {debug,measure,release} sind ALTERNATIVEN derselben
// Frage (welcher Modus?), {csv,latex_table,comparison_metrics,xlsx} sind es NICHT.
// writeback_method_registry.hpp sagt ueber sich selbst, die Achse sei "die EHRLICHE Formalisierung
// des heutigen <output>-Trios" -- also UNABHAENGIGE Ausgabe-Kanaele (Roh-CSV, LaTeX-Tabelle,
// SOTA-Vergleichsmetriken), von denen mehrere gleichzeitig sinnvoll sind. Seit dem 09.08. gilt das
// AUCH innerhalb der Format-Teilmenge {csv,xlsx}: die beiden sind keine konkurrierenden
// Alternativen mehr, sondern zwei AUSGABE-STRATEGIEN derselben Mappe.
//
// NENNER: die Zahl steht nie nackt da -- FormatWahl traegt sowohl die Grundgesamtheit (token_gesamt)
// als auch die Teilmenge (format_token), aus der die Wahl entstand, und diagnose() rendert beide
// fuer die Lauf-AUSGABE.

/// Welche der FORMAT-Token {csv,xlsx} eine id bezeichnet -- oder keine.
enum class TokenSorte : std::uint8_t {
    format,    ///< csv | xlsx -- benennt eine AUSGABE-STRATEGIE der Mappe (mehrere zugleich erlaubt)
    kanal,     ///< latex_table | comparison_metrics -- unabhaengiger Ausgabe-Kanal, kein Format
    unbekannt, ///< in kWritebackMethodRegistry NICHT enthalten
};

/// Einordnung EINER id gegen die Registry (Single-Source: kWritebackMethodRegistry, kein zweites
/// Token-Verzeichnis). Der Vergleich ist ein String-Vergleich, weil das geparste Feld
/// `std::vector<std::string>` ist (xml_config_parser.hpp:342/:493) und KEIN vector<WritebackMethod>.
[[nodiscard]] inline TokenSorte token_sorte(std::string_view id) noexcept {
    for (auto const& info : ms::kWritebackMethodRegistry) {
        if (info.id != id) continue;
        return (info.method == ms::WritebackMethod::Csv || info.method == ms::WritebackMethod::Xlsx)
                   ? TokenSorte::format
                   : TokenSorte::kanal;
    }
    return TokenSorte::unbekannt;
}

/// Lage der Format-Entscheidung. KEINER dieser Werte ist ein Fehler: `default_leer` ist der
/// Owner-KERN-Default, und `beide` ist seit dem Owner-Entscheid 09.08. ein gueltiger Eingang.
enum class FormatLage : std::uint8_t {
    default_leer, ///< kein FORMAT-Token deklariert -> xlsx (Owner-KERN 26.07.)
    gewaehlt,     ///< genau EINE Format-Sorte deklariert (ggf. mehrfach genannt) -> diese
    beide,        ///< csv UND xlsx deklariert -> BEIDE aus DERSELBEN Mappe (Owner 09.08.)
};

struct FormatWahl {
    // Welche Strategien die EINE Mappe herausschreiben. Beide zugleich ist zulaessig; beide false
    // kann nicht entstehen -- ohne FORMAT-Token faellt die Wahl auf den xlsx-Default.
    bool       xlsx = true;  ///< die Mappe entsteht als EINE .xlsx-Datei
    bool       csv  = false; ///< die Mappe entsteht ZUSAETZLICH/STATTDESSEN als flache Sheet-CSVs
    FormatLage lage = FormatLage::default_leer;
    // NENNER-Trias: nie eine nackte Zahl. token_gesamt ist die Grundgesamtheit (alle deklarierten
    // Eintraege), format_token die Teilmenge, aus der die Wahl entstand, unbekannt_token die
    // Eintraege, die in KEINER Registry-Zeile stehen.
    std::size_t token_gesamt    = 0;
    std::size_t format_token    = 0;
    std::size_t kanal_token     = 0;
    std::size_t unbekannt_token = 0;
    std::string unbekannt_liste; ///< die unbekannten ids selbst, komma-getrennt (nie nur gezaehlt)

    /// Wieviele Ausgaben diese Wahl erzeugt -- 1 oder 2, nie 0.
    [[nodiscard]] std::size_t ausgaben() const noexcept {
        return (xlsx ? std::size_t{1} : std::size_t{0}) + (csv ? std::size_t{1} : std::size_t{0});
    }

    /// Menschenlesbare Fassung MIT Grundgesamtheit -- gehoert in die Lauf-AUSGABE, nicht nur in den
    /// Kopf des Lesers (GOAL v8 Prueffrage 1).
    [[nodiscard]] std::string diagnose() const {
        std::string s = "format=";
        if (xlsx) s += "xlsx";
        if (xlsx && csv) s += "+";
        if (csv) s += "csv";
        s += " lage=";
        switch (lage) {
            case FormatLage::default_leer: s += "default_leer"; break;
            case FormatLage::gewaehlt: s += "gewaehlt"; break;
            case FormatLage::beide: s += "beide"; break;
        }
        s += " format_token=" + std::to_string(format_token) + "/" + std::to_string(token_gesamt);
        s += " (kanal=" + std::to_string(kanal_token) + " unbekannt=" + std::to_string(unbekannt_token);
        if (!unbekannt_liste.empty()) s += ":" + unbekannt_liste;
        s += ")";
        return s;
    }
};

/// Die Format-Entscheidung aus dem geparsten <writeback_methods>.
///
/// DEFENSIV MIT ABSICHT: der Auftrag beschrieb das Feld als "bereits geparst UND validiert" -- am
/// Objekt stimmt nur die erste Haelfte. validate_profile() haengt ausschliesslich im
/// `--validate`-CLI-Zweig (profile_run_facade.cpp:809, dort selbst als "rein-lesend ... KEINE Messung"
/// kommentiert); der echte Mess-Lauf geht ueber tlz::run_profile(a) (:776) und ruft validate_profile()
/// NIRGENDS. Ein unbekanntes Token erreicht diese Funktion also ungeprueft. Es wird deshalb weder
/// stillschweigend geschluckt (das waere der Stellvertreter: die Entscheidung faellt, niemand sieht
/// woran) noch zum Lauf-Abbruch erhoben (eine neue Fehlerklasse, die der rohe CSV-Pfad nicht hat) --
/// es wird GEZAEHLT und BENANNT und aendert die Format-Frage nicht, denn es ist kein Format-Token.
[[nodiscard]] inline FormatWahl waehle_ergebnis_format(std::vector<std::string> const& writeback_methods) {
    FormatWahl w;
    w.token_gesamt = writeback_methods.size();

    bool csv_gesehen  = false;
    bool xlsx_gesehen = false;
    for (auto const& id : writeback_methods) {
        switch (token_sorte(id)) {
            case TokenSorte::format:
                ++w.format_token;
                if (id == "xlsx") xlsx_gesehen = true;
                if (id == "csv") csv_gesehen = true;
                break;
            case TokenSorte::kanal: ++w.kanal_token; break;
            case TokenSorte::unbekannt:
                ++w.unbekannt_token;
                if (!w.unbekannt_liste.empty()) w.unbekannt_liste += ",";
                w.unbekannt_liste += id;
                break;
        }
    }

    // Beide Formate zugleich sind seit dem Owner-Entscheid 09.08. der VOLLE Eingang, nicht der
    // verbotene: die Mappe wird einmal gebaut und zweimal herausgeschrieben.
    if (csv_gesehen && xlsx_gesehen) {
        w.lage = FormatLage::beide;
        w.xlsx = true;
        w.csv  = true;
        return w;
    }
    // Ein doppeltes "csv;csv" nennt zweimal DIESELBE Strategie -- das sind zwei Nennungen, aber
    // EINE Ausgabe. Nur csv UND xlsx sind zwei Ausgaben.
    if (csv_gesehen) {
        w.lage = FormatLage::gewaehlt;
        w.xlsx = false;
        w.csv  = true;
        return w;
    }
    if (xlsx_gesehen) {
        w.lage = FormatLage::gewaehlt;
        w.xlsx = true;
        w.csv  = false;
        return w;
    }
    // LEER, FEHLEND, oder ausschliesslich Kanal-/Unbekannt-Token -> xlsx (Owner-KERN-Default).
    w.lage = FormatLage::default_leer;
    w.xlsx = true;
    w.csv  = false;
    return w;
}

// =================================================================================================
// 2. FELD-ADAPTER -- kein neues Schema, nur ein Split
// =================================================================================================
//
// lazy_csv_header() und format_csv_row() (cache_engine_builder_iterator.hpp:504/:680) bleiben die
// EINZIGE Schema-Quelle. Hier entsteht keine zweite Spaltenliste; hier wird der von ihnen erzeugte
// Semikolon-String nur in einen Vektor zerlegt, den kopf()/zeile() entgegennehmen.
//
// WARUM NICHT split_on() AUS profile_run_facade.cpp: jene Funktion (:99-108, TU-lokal in einem
// anonymen Namespace, von hier ohnehin nicht erreichbar) verwirft LEERE Felder --
// `if (!cur.empty()) out.push_back(cur);`. Fuer eine Mess-Zeile ist das kein Schoenheitsfehler,
// sondern genau der STELLVERTRETER-Fehler: format_csv_row() schreibt sehr wohl leere Zellen, und
// jede verschluckte Zelle schoebe ALLE folgenden Werte eine Spalte nach links. Das Messgeraet bliebe
// korrekt, die Zahl belastbar -- und stuende unter der falschen Ueberschrift. Nichts wuerde klappern.
// Der Split hier ERHAELT leere Felder; die Wache darunter (feld_abweichungen) meldet zusaetzlich
// jede Zeile, deren Feldzahl nicht zur Kopfzeile passt.

/// Zerlegt EINE bereits gerenderte CSV-Zeile in ihre Felder. Erhaelt leere Felder. Entfernt genau
/// einen abschliessenden Zeilenumbruch (format_csv_row endet auf '\n') und ein davor stehendes '\r'.
[[nodiscard]] inline std::vector<std::string> csv_zeile_in_felder(std::string_view zeile) {
    if (!zeile.empty() && zeile.back() == '\n') zeile.remove_suffix(1);
    if (!zeile.empty() && zeile.back() == '\r') zeile.remove_suffix(1);

    std::vector<std::string> felder;
    if (zeile.empty()) return felder;

    std::string aktuell;
    for (char const c : zeile) {
        if (c == ';') {
            felder.push_back(aktuell); // LEERES Feld wird bewusst uebernommen (s. Kopf)
            aktuell.clear();
        } else {
            aktuell += c;
        }
    }
    felder.push_back(aktuell); // das letzte Feld hat keinen Trenner hinter sich
    return felder;
}

/// Zerlegt einen mehrzeiligen CSV-Block (LazyRunResult::resumed_csv_rows ist genau das) in Zeilen.
/// Eine leere Schlusszeile nach dem letzten '\n' entsteht nicht.
[[nodiscard]] inline std::vector<std::string_view> csv_blob_in_zeilen(std::string_view blob) {
    std::vector<std::string_view> zeilen;
    std::size_t                   start = 0;
    while (start < blob.size()) {
        std::size_t const nl = blob.find('\n', start);
        if (nl == std::string_view::npos) {
            zeilen.push_back(blob.substr(start));
            break;
        }
        zeilen.push_back(blob.substr(start, nl - start));
        start = nl + 1;
    }
    return zeilen;
}

// =================================================================================================
// 3. DATEINAME-STAMM -- die Grammatik wird nicht neu erfunden
// =================================================================================================

struct StammErgebnis {
    std::string text;
    bool        ok = false;
    std::string fehler;
};

/// <datum>-<zeit> als Stamm, abgeleitet aus bestandslog::blatt_dateiname() -- der bereits gebauten,
/// consteval-selbstgetesteten Grammatik (lager_pfad_grammatik.hpp:417). Sie liefert einen fertigen
/// DATEINAMEN inkl. Endung; die Mappen-Fabrik will einen STAMM und haengt ihre Endung selbst an
/// (".xlsx" bzw. "__S001.csv"). Deshalb wird hier die von der Grammatik erzeugte Endung wieder
/// abgeschnitten, statt den Stamm daneben ein zweites Mal zusammenzusetzen: so bleibt die Grammatik
/// -- inklusive ihrer Ueberlauf-Kuerzung auf "<datum>-<zeit>_H=<hex16>" -- die einzige Quelle.
///
/// Die kv-Kette ist hier LEER: unter diesem Blatt variiert (noch) nichts. Der Kopfkommentar der
/// Grammatik nennt das ausdruecklich den Normalfall, nicht einen Sonderfall.
[[nodiscard]] inline StammErgebnis mappen_stamm(std::string_view datum, std::string_view zeit,
                                                std::string_view endung) {
    StammErgebnis               r;
    std::span<bl::KvPaar const> keine_paare{};
    auto const                  e = bl::blatt_dateiname(datum, zeit, keine_paare, endung);
    if (e.fehler != bl::LagerPfadFehler::ok) {
        r.fehler = std::string{"blatt_dateiname: "} + std::string{bl::to_string(e.fehler)};
        return r;
    }
    std::string const suffix = "." + std::string{endung};
    if (e.text.size() <= suffix.size() || e.text.compare(e.text.size() - suffix.size(), suffix.size(), suffix) != 0) {
        r.fehler = "blatt_dateiname lieferte '" + e.text + "' ohne die erwartete Endung '" + suffix + "'";
        return r;
    }
    r.text = e.text.substr(0, e.text.size() - suffix.size());
    r.ok   = true;
    return r;
}

/// Datum (8 Ziffern) und Uhrzeit (6 Ziffern) der lokalen Zeit -- genau die Formen, die
/// blatt_dateiname() per detail::nur_ziffern(datum,8)/(zeit,6) verlangt. Muster uebernommen von
/// experiment_run_entry.hpp:132 (strftime "%Y%m%d") -- die einzige vorhandene Zeitstempel-Quelle,
/// utc_iso_from_epoch(), liefert "YYYY-MM-DDTHH:MM:SSZ" und ist fuer diese Grammatik unbrauchbar.
inline void jetzt_datum_zeit(std::string& datum, std::string& zeit) {
    std::time_t const now = std::time(nullptr);
    std::tm           tm{};
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char d[16] = {};
    char z[16] = {};
    std::strftime(d, sizeof(d), "%Y%m%d", &tm);
    std::strftime(z, sizeof(z), "%H%M%S", &tm);
    datum = d;
    zeit  = z;
}

// =================================================================================================
// 4. DIE NAHT SELBST
// =================================================================================================
//
// EIN KONSTANTER SheetSchluessel: alle Zeilen gehen in EIN Sheet. Ein SheetSchluessel mit drei leeren
// Feldern heisst laut ergebnis_mappe.hpp woertlich "diese Unter-Ebene ist hier nicht noetig" -- das
// ist keine Notloesung, sondern die vorgesehene Form. Die Auffaecherung nach
// Unter-Achsen-Permutationen ist die CoR-Filterkette der Auswertung (A9-S4) und ausdruecklich NICHT
// Teil dieses Pakets.
//
// EINE MAPPE, EIN ODER ZWEI AUSGABEN (Owner-Entscheid 09.08.): sind csv UND xlsx deklariert, wird
// die Mappe trotzdem nur EINMAL gebaut -- EIN Zeitstempel, EIN Dateiname-Stamm, EIN Feld-Split je
// Zeile, EIN INFO-Blatt-Inhalt. Was sich verdoppelt, ist allein das HERAUSSCHREIBEN. Es gibt hier
// KEINEN zweiten Bau-Weg, keinen zweiten Lauf und keine zweite Zeilen-Quelle -- genau darauf zielte
// das 05.08.-Verbot, und genau das bleibt.
//
// STAMM UND KIND, NICHT ZWEI GESCHWISTER (Owner 09.08. 16:31 -- s. Kopf, Satz 1-3):
//   Der STAMM ist die Mappe: eine xlsx im Speicher, BEDINGUNGSLOS gebaut. Ueber ihr steht kein `if`.
//   Das KIND ist die csv-Ableitung: sie existiert nur, wenn csv auch persistiert werden soll, und
//   sie bekommt eine Zeile AUSSCHLIESSLICH aus schreibe(), NACHDEM der Stamm sie angenommen hat.
//   Lehnt der Stamm ab, faellt das Kind mit -- ein csv-Ergebnis ohne funktionierenden xlsx-Weg
//   waere ein Widerspruch, kein Sparmodus.
//   Die PERSISTENZ ist davon getrennt: wahl_.xlsx/wahl_.csv steuern allein, welche der beiden
//   geschlossen (= auf die Platte geschrieben) wird. Wird der Stamm NICHT geschlossen, raeumt sein
//   Dtor den Vendor-Speicher UND den .xlsx.tmp-Pfad weg (xlsx_ergebnis_writer.cpp:148-157) -- ein
//   csv-only-Lauf hinterlaesst also keinerlei xlsx-Spur auf der Platte, obwohl die Mappe lief.
//
// WIRFT NIE nach aussen: ein Mappen-Fehler darf einen laufenden Mess-Lauf nicht abbrechen, solange
// die Mappe additiv neben der offiziellen CSV steht. Ein Fehlausgang hinterlaesst seinen Grund MIT
// ZIELNAMEN in diagnose(); der Aufrufer gibt ihn aus. Beobachtbar, nicht verdeckt.
//
// FAIL-CLOSED beim Ziel: laesst sich das Zielverzeichnis nicht herstellen, entsteht GAR NICHTS --
// weder Stamm noch Kind. Frueher wurde der error_code von create_directories() verworfen; dann baute
// die xlsx-Mappe trotzdem auf (workbook_new() fasst die Platte nicht an), und der Fehlausgang fiel
// erst in schliessen() auf -- also NACH dem Mess-Fenster, wenn die Zeilen schon verbraucht sind.
class MappenNaht {
public:
    MappenNaht()                             = default;
    MappenNaht(MappenNaht const&)            = delete;
    MappenNaht& operator=(MappenNaht const&) = delete;

    /// Oeffnet die Mappe NEBEN der offiziellen CSV (dasselbe Verzeichnis -- die Lagerbaum-Stelle wird
    /// hier nicht neu erfunden, sie wird von der bereits getroffenen Wahl des Aufrufers geerbt).
    void oeffnen(std::filesystem::path const& out_csv, std::vector<std::string> const& writeback_methods) {
        wahl_     = waehle_ergebnis_format(writeback_methods);
        diagnose_ = wahl_.diagnose();

        // EIN Zeitstempel fuer Stamm UND Kind: zwei jetzt_datum_zeit()-Aufrufe koennten ueber einen
        // Sekundenwechsel stolpern -- dann truege EINE Messung zwei verschiedene Namen.
        std::string datum;
        std::string zeit;
        jetzt_datum_zeit(datum, zeit);

        // EIN Stamm-Name fuer beide: die Mappe hat EINE Identitaet, auch wenn sie zweimal
        // herausgeschrieben wird -- <datum>-<zeit>.xlsx und <datum>-<zeit>__S001.csv gehoeren
        // sichtbar zusammen. Die Endung ist hier FEST "xlsx", weil die Mappe IMMER die xlsx ist;
        // frueher stand hier `wahl_.xlsx ? "xlsx" : "csv"` -- eine Verzweigung, die es seit der
        // Entkopplung von Erzeugung und Persistenz nicht mehr gibt. Der Stamm-TEXT ist von der
        // Endung ohnehin unabhaengig: blatt_dateiname() setzt ihn als "<datum>-<zeit>" und haengt
        // die Endung nur an (lager_pfad_grammatik.hpp), mappen_stamm schneidet sie wieder ab.
        auto const stamm = mappen_stamm(datum, zeit, "xlsx");
        if (!stamm.ok) {
            diagnose_ += " -- " + stamm.fehler;
            return;
        }

        auto verzeichnis = out_csv.parent_path();
        if (verzeichnis.empty()) verzeichnis = ".";
        std::error_code ec;
        std::filesystem::create_directories(verzeichnis, ec); // vorhandenes Verzeichnis ist kein Fehler
        // FAIL-CLOSED: der error_code allein genuegt nicht (bei bereits vorhandenem Verzeichnis ist
        // er leer UND der Rueckgabewert false). Gefragt ist der GEGENSTAND: liegt danach wirklich
        // ein Verzeichnis dort? Wenn nein, entsteht hier nichts -- weder Stamm noch Kind.
        std::error_code ec_stat;
        if (!std::filesystem::is_directory(verzeichnis, ec_stat)) {
            diagnose_ += " -- Zielverzeichnis nicht herstellbar: " + verzeichnis.string() +
                         (ec ? " (" + ec.message() + ")" : std::string{});
            return;
        }

        // (1) DER STAMM -- BEDINGUNGSLOS. Hier steht mit Absicht kein `if (wahl_.xlsx)`: die
        //     Format-Wahl entscheidet ueber das SCHLIESSEN (schliessen()), nicht ueber das ENTSTEHEN.
        try {
            stamm_       = lab::ErgebnisMappenFactory::oeffne(verzeichnis, stamm.text, lab::ErgebnisFormat::xlsx);
            stamm_blatt_ = &stamm_->blatt(lab::SheetSchluessel{});
            stamm_ziel_  = verzeichnis / (stamm.text + ".xlsx");
            zeilen_je_blatt_.push_back(0); // EIN Blatt, ab jetzt gefuehrt (s. blatt_zahl())
        } catch (std::exception const& e) {
            stamm_.reset();
            stamm_blatt_ = nullptr;
            zeilen_je_blatt_.clear();
            diagnose_ += std::string{" -- die MAPPE (xlsx) entstand NICHT: "} + e.what();
            return; // ohne Stamm kein Kind -- Satz 2.
        }

        // (2) DAS KIND -- nur wenn die csv auch persistiert werden soll. Es wird hier NICHT gefuettert;
        //     seine Zeilen bekommt es ausschliesslich in schreibe(), nach der Annahme durch den Stamm.
        if (wahl_.csv) {
            try {
                kind_       = lab::ErgebnisMappenFactory::oeffne(verzeichnis, stamm.text, lab::ErgebnisFormat::csv);
                kind_blatt_ = &kind_->blatt(lab::SheetSchluessel{});
                kind_ziel_  = verzeichnis / (stamm.text + "__S001.csv");
            } catch (std::exception const& e) {
                kind_.reset();
                kind_blatt_ = nullptr;
                diagnose_ += std::string{" -- die csv-Ableitung entstand nicht: "} + e.what();
            }
        }
    }

    /// Steht DIE MAPPE -- und nimmt sie noch an? Das ist die Frage, an der alles haengt; NICHT, ob
    /// eine Datei gewaehlt wurde. Ein csv-only-Lauf ist hier genauso scharf wie ein xlsx-Lauf.
    [[nodiscard]] bool scharf() const noexcept { return stamm_blatt_ != nullptr; }
    /// Existiert das Mappen-OBJEKT noch (auch wenn es keine Zeile mehr annimmt)?
    [[nodiscard]] bool               mappe_lebt() const noexcept { return stamm_ != nullptr; }
    [[nodiscard]] FormatWahl const&  wahl() const noexcept { return wahl_; }
    [[nodiscard]] std::string const& diagnose() const noexcept { return diagnose_; }

    /// Die Ziele, die auf der PLATTE landen -- komma-getrennt, in der Reihenfolge xlsx, csv. Der
    /// Stamm steht hier NUR, wenn er auch persistiert wird: eine genannte Datei, die nie entsteht,
    /// waere derselbe Stellvertreter wie eine ungenannte, die entsteht.
    [[nodiscard]] std::string ziele() const {
        std::string s;
        if (wahl_.xlsx && stamm_ != nullptr) s += stamm_ziel_.string();
        if (kind_ != nullptr) {
            if (!s.empty()) s += ", ";
            s += kind_ziel_.string();
        }
        // S13-01: die offizielle Auswerte-CSV ist seit dem Umbau ebenfalls ein ZIEL dieser Mappe.
        if (projektion_gewollt_ && !projektion_fehler_) {
            if (!s.empty()) s += ", ";
            s += projektion_ziel_.string();
        }
        return s;
    }

    /// Wieviele Blaetter die Mappe im Speicher fuehrt (heute EIN konstanter SheetSchluessel -- die
    /// Auffaecherung ist die CoR-Filterkette der Auswertung, s. Kopf dieses Abschnitts).
    [[nodiscard]] std::size_t blatt_zahl() const noexcept { return zeilen_je_blatt_.size(); }

    /// Zeilen, die DIE MAPPE in diesem Blatt ANGENOMMEN hat -- der Beleg, dass die xlsx-Erzeugung
    /// gelaufen ist, unabhaengig davon, ob sie je auf die Platte geht. Index >= blatt_zahl() ergibt 0.
    [[nodiscard]] std::size_t zeilen_im_blatt(std::size_t index) const noexcept {
        return index < zeilen_je_blatt_.size() ? zeilen_je_blatt_[index] : 0;
    }

    /// Zeilen, die der Naht ANGEBOTEN wurden (Leerzeilen zaehlen nicht) -- die Grundgesamtheit, gegen
    /// die zeilen() zu lesen ist. Ohne sie stuende die angenommene Zahl nackt da.
    [[nodiscard]] std::size_t angeboten() const noexcept { return angeboten_; }

    /// Zeilen, die VERWORFEN wurden, weil die Mappe zum Zeitpunkt ihres Eintreffens nicht mehr annahm.
    /// Muss 0 sein; jeder Wert > 0 heisst: die csv dieses Laufs ist unvollstaendig.
    [[nodiscard]] std::size_t verworfen() const noexcept { return verworfen_; }

    /// Der BESTAND im Speicher, fuer die Lauf-AUSGABE -- nie eine nackte Zahl: angenommene Zeilen je
    /// Blatt IMMER gegen die angebotenen. Trennt sichtbar, was im RAM liegt, von dem, was persistiert
    /// wird -- genau die Unterscheidung, die der Owner-KERN verlangt.
    ///
    /// V-8, EHRLICH BEANTWORTET (2026-08-10) -- "Was waere der Zustand, in dem diese Ausgabe erscheint
    /// und die Sache trotzdem nicht existiert?" Antwort: genau der, den ein adversarischer Pruefer am
    /// 10.08. gewuerfelt hat (Koeder M3s). Macht man den Aufruf `stamm_blatt_->zeile(felder)` in
    /// schreibe() zu einem No-op, OHNE dass er wirft, dann laeuft der Erfolgspfad darunter unveraendert
    /// weiter: zeilen_je_blatt_ waechst, verworfen_ bleibt 0, und diese Zeile meldet "S1:7/7 angeboten
    /// verworfen=0" -- waehrend im Blatt der Mappe NULL Datenzeilen stehen. Die csv rettet dabei nicht,
    /// denn sie haengt am KIND-Zweig derselben Funktion und bewegt sich mit dem Zaehler gemeinsam.
    ///
    /// DARAUS FOLGT, WAS DIESE ZAHL IST UND WAS NICHT: sie ist die Buchhaltung DER NAHT ueber den
    /// Handschlag mit der Mappe ("die Mappe hat nicht abgelehnt"), NICHT eine Messung des
    /// Blatt-INHALTS. Beides faellt nur zusammen, solange kopf()/zeile() ihren Vertrag halten -- und
    /// genau das kann diese Klasse nicht selbst bezeugen, weil IErgebnisBlatt::zeile() `void`
    /// zurueckgibt (ergebnis_mappe.hpp:376) und es keinen Getter fuer die tatsaechlich angenommene
    /// Zeilenzahl gibt. Die Ausgabe sagt ihre Herkunft deshalb selbst an (quelle=naht-buchhaltung),
    /// damit ein Leser sie nicht fuer eine Inhalts-Aussage haelt.
    ///
    /// WER DEN INHALT PRUEFT (V-7, fremder Nenner): tests/unit/test_a9s5_ergebnis_mappe_naht.cpp,
    /// Suite A9S5MappenInhalt -- sie liest die erzeugte .xlsx als ZIP zurueck, holt `dimension ref`
    /// und die <row>-Zahl aus xl/worksheets/sheet1.xml (beides von libxlsxwriter selbst gefuehrt, also
    /// aus einer ANDEREN Quelle als dieser Zaehler) und haelt sie gegen zeilen_im_blatt(). Diese Zeile
    /// hier ist damit kein unverbrauchtes Angebot mehr: eine Wache liest sie und vergleicht sie.
    [[nodiscard]] std::string bestand() const {
        std::string s = "mappe=";
        s += (stamm_ != nullptr ? "xlsx-im-speicher" : "FEHLT");
        s += " blaetter=" + std::to_string(blatt_zahl());
        s += " zeilen=";
        if (zeilen_je_blatt_.empty()) {
            s += "-";
        } else {
            for (std::size_t i = 0; i < zeilen_je_blatt_.size(); ++i) {
                if (i != 0) s += "+";
                s += "S" + std::to_string(i + 1) + ":" + std::to_string(zeilen_je_blatt_[i]);
            }
        }
        s += "/" + std::to_string(angeboten_) + " angeboten";
        s += " verworfen=" + std::to_string(verworfen_);
        s += " persistenz=";
        if (!wahl_.xlsx && !wahl_.csv) {
            s += "keine";
        } else {
            if (wahl_.xlsx) s += "xlsx";
            if (wahl_.xlsx && wahl_.csv) s += "+";
            if (wahl_.csv) s += "csv";
        }
        // HERKUNFT DER ZAHL, in der Ausgabe selbst (s. V-8-Absatz oben): alles links davon ist die
        // Buchhaltung DIESER Klasse ueber den Handschlag mit der Mappe -- keine Messung am Blatt.
        s += " quelle=naht-buchhaltung";
        return s;
    }

    /// Zeilen DER MAPPE -- eine Zeile bleibt EINE Zeile, auch wenn sie in zwei Dateien faellt.
    [[nodiscard]] std::size_t zeilen() const noexcept { return zeilen_; }
    /// Zeilen, deren Feldzahl NICHT der Kopfzeile entsprach -- die Wache gegen stille Spalten-
    /// Verschiebung. Muss bei intaktem Schema 0 sein.
    [[nodiscard]] std::size_t feld_abweichungen() const noexcept { return feld_abweichungen_; }
    /// Grundgesamtheit, ueber der feld_abweichungen() zaehlt: die Zahl der Spalten der Kopfzeile.
    [[nodiscard]] std::size_t kopf_spalten() const noexcept { return kopf_spalten_; }

    /// Die Kopfzeile -- genau der String, den lazy_csv_header() liefert. EIN Split fuer ALLE
    /// Strategien: der Kopf der Mappe ist EIN Kopf.
    void kopf_aus_csv(std::string_view header_zeile) {
        if (!scharf()) return;
        auto const felder = csv_zeile_in_felder(header_zeile);
        kopf_spalten_     = felder.size();
        schreibe(felder, true);
    }

    /// EINE Zeile -- genau der String, den format_csv_row() liefert.
    void zeile_aus_csv(std::string_view csv_zeile) {
        auto const felder = csv_zeile_in_felder(csv_zeile);
        if (felder.empty()) return; // Leerzeile: nichts zu schreiben, kein Fehler
        // ANGEBOTEN wird gezaehlt, BEVOR die Mappe befragt wird -- sonst waere die Grundgesamtheit
        // aus dem Prueflung selbst abgeleitet und koennte nie von der angenommenen Zahl abweichen.
        ++angeboten_;
        if (stamm_ == nullptr) return; // nie geoeffnet: kein Bestand, keine Verwerfung
        if (kopf_spalten_ != 0 && felder.size() != kopf_spalten_) ++feld_abweichungen_;
        schreibe(felder, false);
    }

    /// Ein mehrzeiliger Block (LazyRunResult::resumed_csv_rows -- resumierte Alt-Zeilen).
    void blob_aus_csv(std::string_view blob) {
        // KEIN scharf()-Vorbehalt: die Zeilen sollen auch dann als ANGEBOTEN gezaehlt werden, wenn
        // die Mappe nicht (mehr) annimmt -- sonst schrumpfte der Nenner mit dem Defekt und die
        // Verwerfung bliebe unsichtbar. zeile_aus_csv() entscheidet je Zeile.
        for (auto const z : csv_blob_in_zeilen(blob)) zeile_aus_csv(z);
    }

    // =============================================================================================
    // S13-01 (#18/D-1, Owner 09.08. 16:31 Satz 2): DIE OFFIZIELLE CSV IST KIND DER MAPPE.
    // Die Datei a.out_csv (measurements.csv) entsteht seit diesem Schritt NUR NOCH als PROJEKTION:
    // eine Zeile erreicht sie AUSSCHLIESSLICH in schreibe(), NACHDEM der Stamm sie angenommen hat.
    // Ihre Byte-Form ist die des Kindes (Felder ';'-gejoint + '\n', CsvErgebnisBlatt-Form) -- fuer
    // Zeilen aus lazy_csv_header()/format_csv_row()/lazy_try_resume_binary() ist das byte-identisch
    // zum entfernten rohen Strom (jede Zeile traegt genau EIN '\n', kein '\r', keine Leerzeile --
    // das Byte-Orakel in test_s13_01_csv_kind_projektion pinnt genau diese Gleichheit).
    //
    // SCOPE-FESTLEGUNG (S13-02 Folgewirkung 2, Design 4.1): die Projektion speist sich AUS DEM
    // ERGEBNIS-Blatt (SheetSchluessel, Zeile pro Messergebnis) -- konkret: aus schreibe(), dem
    // einzigen Weg dorthin. Kuenftige Profil-Blaetter (MessEbenenSchluessel, S13-09-Drain) duerfen
    // sie NIE speisen; der Zeilen-Zaehl-Koeder (Projektion == 1 Kopf + zeilen()) beisst dann ROT.
    // =============================================================================================

    /// Oeffnet die offizielle Auswerte-CSV als Projektion. VOR kopf_aus_csv() rufen.
    /// Rueckgabe false = eine VERLANGTE csv kann nicht entstehen (Stamm fehlt / nicht oeffenbar)
    /// -- der Aufrufer soll den Lauf fail-fast beenden (D-2(2): "Bricht die xlsx-Erzeugung, ist
    /// auch die csv ungueltig"). Ist csv NICHT gewaehlt (S13-02-Filter), ist der Aufruf ein No-op
    /// mit true: die Datei darf dann NICHT entstehen -- genau das prueft der Fall-2/4-Koeder.
    [[nodiscard]] bool projektion_oeffnen(std::filesystem::path const& ziel) {
        if (!wahl_.csv) return true; // nicht deklariert -> nichts anlegen (Ziel-Filter S13-02)
        if (!scharf()) {
            diagnose_ += " -- offizielle CSV nicht eroeffnet: der Stamm steht nicht (D-2(2))";
            projektion_fehler_ = true;
            return false;
        }
        projektion_ziel_ = ziel;
        projektion_.open(ziel, std::ios::trunc | std::ios::binary);
        if (!projektion_) {
            diagnose_ += " -- offizielle CSV nicht oeffenbar: " + ziel.string();
            projektion_fehler_ = true;
            return false;
        }
        projektion_gewollt_ = true;
        return true;
    }

    /// Steht die offizielle CSV so da, wie sie verlangt war? true auch, wenn sie nie verlangt war
    /// (csv nicht gewaehlt / projektion_oeffnen nie gerufen -- z. B. der per-Binary-Sink, dessen
    /// Auswerte-CSV das __S001-Kind ist). Verlaesslich erst NACH schliessen().
    [[nodiscard]] bool projektion_ok() const noexcept {
        return projektion_gewollt_ ? projektion_geschlossen_ok_ : !projektion_fehler_;
    }
    [[nodiscard]] std::filesystem::path const& projektion_ziel() const noexcept { return projektion_ziel_; }

    /// Schreibt INFO-Blatt und PERSISTIERT, was persistiert werden soll -- NUR das. Der Aufrufer ruft
    /// ihn NACH der Mess-Schleife; DAS ist der einzige Punkt, an dem die Mappe die Platte anfasst.
    /// DIESELBEN Meta-Daten gehen in beide Ausgaben (ein INFO-Inhalt, zwei Darstellungen).
    ///
    /// DIE ENTKOPPLUNG WIRD HIER KONKRET: ist xlsx NICHT gewaehlt, wird der Stamm NICHT geschlossen.
    /// Er wird nur freigegeben -- sein Dtor gibt den Vendor-Speicher frei und entfernt den
    /// .xlsx.tmp-Pfad (xlsx_ergebnis_writer.cpp:148-157). Die Mappe LIEF, sie LANDET nur nicht.
    ///
    /// Rueckgabe false = etwas Verlangtes steht NICHT vollstaendig auf der Platte -- oder es gab gar
    /// keine Mappe (Grund jeweils in diagnose(), mit Zielnamen).
    bool schliessen(lab::MaschinenSysinfo const& sysinfo, lab::HauptAchsenBelegung const& haupt,
                    lab::KonstantenMeta const& konstanten) {
        if (stamm_ == nullptr) return false;
        bool alle_ok = true;

        // S13-01: die offizielle CSV (Projektion) ZUERST abschliessen. GUELTIG heisst: der Stamm
        // nimmt noch an, KEINE Zeile wurde verworfen, der Strom hat nie gefehlt und Flush+Close
        // gelingen. Alles andere heisst: die Datei WAERE eine Luege -- sie wird ENTFERNT, nicht
        // halb liegen gelassen (Design-Koeder S13-01: liegen gelassene CSV bei Schreibfehler = ROT).
        if (projektion_gewollt_) {
            bool gueltig = (stamm_blatt_ != nullptr) && verworfen_ == 0 && !projektion_fehler_;
            if (gueltig) {
                projektion_.flush();
                gueltig = static_cast<bool>(projektion_);
                projektion_.close();
                gueltig = gueltig && !projektion_.fail();
            } else {
                projektion_.close();
            }
            if (gueltig) {
                projektion_geschlossen_ok_ = true;
            } else {
                std::error_code pec;
                std::filesystem::remove(projektion_ziel_, pec);
                std::error_code ist_ec;
                if (pec || std::filesystem::exists(projektion_ziel_, ist_ec)) {
                    diagnose_ += " -- WARNUNG: ungueltige offizielle CSV NICHT entfernt: " + projektion_ziel_.string();
                } else if (!projektion_fehler_) {
                    diagnose_ += " -- offizielle CSV (" + projektion_ziel_.string() + ") ungueltig und entfernt";
                }
                projektion_fehler_ = true;
                alle_ok            = false;
            }
        }

        if (wahl_.csv) {
            if (kind_ == nullptr) {
                alle_ok = false; // csv war verlangt, die Ableitung steht nicht -- Grund in diagnose_
            } else {
                try {
                    kind_->info_blatt(sysinfo, haupt, konstanten);
                    kind_->schliessen();
                } catch (std::exception const& e) {
                    diagnose_ += " -- schliessen (" + kind_ziel_.string() + ") fehlgeschlagen: " + e.what();
                    alle_ok = false;
                }
            }
        }

        if (wahl_.xlsx) {
            try {
                stamm_->info_blatt(sysinfo, haupt, konstanten);
                stamm_->schliessen();
            } catch (std::exception const& e) {
                diagnose_ += " -- schliessen (" + stamm_ziel_.string() + ") fehlgeschlagen: " + e.what();
                alle_ok = false;
            }
        }

        kind_blatt_ = nullptr;
        kind_.reset();
        stamm_blatt_ = nullptr;
        stamm_.reset(); // hier faellt der Dtor -- bei nicht-persistiertem Stamm samt tmp-Aufraeumen
        return alle_ok;
    }

private:
    /// Reicht DIESELBEN Felder erst in die MAPPE und dann -- nur bei Annahme -- in ihr KIND.
    ///
    /// DIE REIHENFOLGE IST DIE AUSSAGE, nicht eine Bequemlichkeit: eine csv-Zeile, die entstuende,
    /// obwohl die Mappe sie abgelehnt hat, waere wieder ein GESCHWISTER. Sie ist es nicht. Legt der
    /// Stamm die Annahme nieder (Zeilenlimit, Schreibfehler), faellt das Kind im selben Zug mit --
    /// und jede weitere Zeile wird als VERWORFEN gezaehlt statt still zu verschwinden.
    void schreibe(std::vector<std::string> const& felder, bool ist_kopf) {
        if (stamm_blatt_ == nullptr) {
            if (!ist_kopf) ++verworfen_;
            return;
        }
        try {
            if (ist_kopf) {
                stamm_blatt_->kopf(felder);
            } else {
                stamm_blatt_->zeile(felder);
            }
        } catch (std::exception const& e) {
            diagnose_ += " -- die MAPPE nahm nicht mehr an (" + stamm_ziel_.string() + "): " + e.what();
            stamm_blatt_ = nullptr;
            // S13-01: DIE PROJEKTION FAELLT MIT DEM STAMM -- SOFORT, nicht erst in schliessen().
            // Eine Fassung, die bei ErgebnisSchreibFehler trotzdem eine measurements.csv liegen
            // laesst, ist der benannte Design-Koeder und wird ROT (test_s13_01_csv_kind_projektion,
            // Ablehnungs-Fall). Bereits geschriebene Zeilen sind mit dem Bruch UNGUELTIG (D-2(2)).
            if (projektion_gewollt_) {
                projektion_.close();
                std::error_code pec;
                std::filesystem::remove(projektion_ziel_, pec);
                std::error_code ist_ec;
                if (pec || std::filesystem::exists(projektion_ziel_, ist_ec)) {
                    diagnose_ += " -- WARNUNG: ungueltige offizielle CSV NICHT entfernt: " + projektion_ziel_.string();
                } else {
                    diagnose_ += " -- offizielle CSV entfernt (Projektion faellt mit dem Stamm)";
                }
                projektion_fehler_ = true;
            }
            // Das Kind faellt MIT -- ohne Stamm keine gueltige Ableitung.
            //
            // DIESE ZUWEISUNG IST REDUNDANT, UND ZWAR NACHWEISBAR (2026-08-10): kind_blatt_ wird
            // ausschliesslich weiter unten in dieser Funktion gelesen, und dorthin fuehrt kein Weg
            // mehr, sobald stamm_blatt_ null ist -- der Frueh-Ausgang ganz oben faengt jeden weiteren
            // Aufruf ab. Ein Koeder, der NUR diese Zeile loescht, ist deshalb ein AEQUIVALENTER
            // MUTANT: er kann von keinem Test rot gemacht werden, weil er nichts Beobachtbares
            // aendert. Sie bleibt trotzdem stehen (Tiefenverteidigung: wer den Frueh-Ausgang oben
            // spaeter umbaut, soll das Kind nicht versehentlich weiterlaufen lassen) -- aber sie wird
            // hier ausdruecklich NICHT als abgesichert ausgegeben.
            //
            // DIE SEMANTIK dagegen -- "legt der Stamm die Annahme nieder, faellt das Kind im selben
            // Zug mit" -- ist sehr wohl beobachtbar und ist gedeckt: A9S5Ablehnung im Testfile prueft
            // sie am GEGENSTAND, naemlich an der Zeilenzahl der erzeugten csv-Datei auf der Platte.
            kind_blatt_ = nullptr;
            if (!ist_kopf) ++verworfen_;
            return;
        }
        if (!ist_kopf) {
            ++zeilen_;
            if (!zeilen_je_blatt_.empty()) ++zeilen_je_blatt_.front();
        }

        // S13-01: die offizielle CSV bekommt DIESELBEN Felder, die der Stamm soeben ANGENOMMEN hat
        // -- ';'-gejoint + '\n' (die Byte-Form des Kindes). Ein Strom-Fehler wird gemerkt und macht
        // die Projektion in schliessen() ungueltig (Datei wird entfernt, nie halb liegen gelassen).
        if (projektion_gewollt_ && !projektion_fehler_ && projektion_.is_open()) {
            std::string zeile;
            for (std::size_t i = 0; i < felder.size(); ++i) {
                if (i != 0) zeile += ';';
                zeile += felder[i];
            }
            zeile += '\n';
            projektion_.write(zeile.data(), static_cast<std::streamsize>(zeile.size()));
            if (!projektion_) {
                diagnose_ += " -- offizielle CSV (" + projektion_ziel_.string() + ") nahm nicht mehr an";
                projektion_fehler_ = true;
            }
        }

        if (kind_blatt_ == nullptr) return;
        try {
            if (ist_kopf) {
                kind_blatt_->kopf(felder);
            } else {
                kind_blatt_->zeile(felder);
            }
        } catch (std::exception const& e) {
            // Nur die ABLEITUNG faellt aus; die Mappe steht und nimmt weiter an. Umgekehrt geht es
            // nicht -- das ist die Asymmetrie zwischen Stamm und Kind.
            diagnose_ += " -- die csv-Ableitung nahm nicht mehr an (" + kind_ziel_.string() + "): " + e.what();
            kind_blatt_ = nullptr;
        }
    }

    /// DER STAMM: die Mappe im Speicher. IMMER eine xlsx, IMMER vorhanden, sobald oeffnen() gelang.
    std::unique_ptr<lab::IErgebnisMappe> stamm_;
    lab::IErgebnisBlatt*                 stamm_blatt_ = nullptr; ///< nullptr = nimmt nicht (mehr) an
    std::filesystem::path                stamm_ziel_;
    /// DAS KIND: die csv-Ableitung. Nur vorhanden, wenn csv auch persistiert werden soll.
    std::unique_ptr<lab::IErgebnisMappe> kind_;
    lab::IErgebnisBlatt*                 kind_blatt_ = nullptr;
    std::filesystem::path                kind_ziel_;

    /// S13-01: DIE OFFIZIELLE CSV (a.out_csv) als Projektion des Stamms. Nur belegt, wenn der
    /// Aufrufer sie via projektion_oeffnen() verlangt hat UND wahl_.csv sie deckt.
    std::ofstream         projektion_;
    std::filesystem::path projektion_ziel_;
    bool                  projektion_gewollt_        = false; ///< oeffnen gelang, Strom existiert(e)
    bool                  projektion_fehler_         = false; ///< Open-/Schreib-/Stamm-Fehler unterwegs
    bool                  projektion_geschlossen_ok_ = false; ///< schliessen(): vollstaendig auf Platte

    FormatWahl               wahl_{};
    std::string              diagnose_ = "nicht geoeffnet";
    std::vector<std::size_t> zeilen_je_blatt_;       ///< der BESTAND im Speicher, je Blatt der Mappe
    std::size_t              angeboten_         = 0; ///< Grundgesamtheit: hereingereichte Datenzeilen
    std::size_t              verworfen_         = 0; ///< angeboten, aber von der Mappe nicht angenommen
    std::size_t              zeilen_            = 0;
    std::size_t              kopf_spalten_      = 0;
    std::size_t              feld_abweichungen_ = 0;
};

// =================================================================================================
// 5. S13-03(a) -- DER PER-BINARY-MAPPEN-SINK DER FASSADE
// =================================================================================================
//
// KON32-01 (Owner, woertlich): "nur per Binary xlsx" -- je Tier-Binary EINE Ergebnis-Mappe ueber
// DIESELBE ErgebnisMappenFactory wie das Aggregat, im bin_dir der Binary. Der Iterator kennt diese
// Fabrik NICHT (Schichtung: builder darf nicht auf profile_facade zeigen); er bekommt vom Seam eine
// LazyRunConfig::per_binary_mappe-Funktion nach dem measurement_sink-Muster (No-Op-Default =>
// byte-neutral) und ruft sie an der per-Binary-Synchron-Naht (NACH result.csv+stamp).
//
// DIE DREI VERTRAGS-SAETZE (S13-03, Design 4.1):
//   (a) die Mappe entsteht ueber MappenNaht -- Stamm xlsx (Filter: schliessen), Kind __S001.csv
//       (Filter: wahl_.csv). Das KIND ist die per-Binary-AUSWERTE-CSV; ihr Name (<stamm>__S001.csv)
//       ist per Grammatik von result.csv verschieden -- Koeder 3 (Ueberschreiben des
//       Resume-Vertrags) ist damit strukturell ausgeschlossen und im Test als Byte-Wache gedeckt.
//   (b) result.csv + result.csv.stamp schreibt WEITERHIN und BEDINGUNGSLOS der Iterator -- sie sind
//       RESUME-VERTRAG (Betriebszustand), NICHT Auswerte-Format, und stehen AUSSERHALB des Filters.
//       Dieser Sink fasst sie NIE an (er schreibt ausschliesslich Mappen-Ziele mit Grammatik-Stamm).
//   (c) eine Projektion (projektion_oeffnen) wird hier NICHT eroeffnet -- die offizielle
//       Auswerte-CSV je Binary IST das Kind aus (a).
//
// FEHLER-POLITIK: MESSEN WEITER (Muster measurement_sink/cache_push) -- eine per-Binary-Mappe, die
// nicht entsteht, bricht die Messung nicht ab, sie wird LAUT benannt (cerr, mit bin_dir und Grund).
// Das Aggregat + result.csv tragen die Daten weiter; der Zaehl-Test (N Mappen + 1 Aggregat) macht
// einen stillen Ausfall im Abnahme-Lauf sichtbar.

/// Liefert den per-Binary-Mappen-Sink fuer LazyRunConfig::per_binary_mappe. hostname ist
/// lauf-konstant (dieselbe Quelle wie das Aggregat: measurement::live_hostname() am Seam).
[[nodiscard]] inline auto mach_per_binary_mappe_sink(std::vector<std::string> writeback_methods, std::string hostname) {
    return [methods = std::move(writeback_methods),
            host    = std::move(hostname)](std::filesystem::path const& bin_dir, std::string const& binary_id,
                                        std::string_view header, std::string_view rows) {
        MappenNaht n;
        // Der Anker dient NUR der Verzeichnis-Wahl (oeffnen liest parent_path()); die Datei
        // "result.csv" selbst wird von dieser Naht nie geoeffnet oder geschrieben -- Satz (b).
        n.oeffnen(bin_dir / "result.csv", methods);
        if (!n.scharf()) {
            std::cerr << "[MAPPE-BINARY] FEHLER binary_id='" << binary_id << "' " << n.diagnose() << "\n";
            return;
        }
        n.kopf_aus_csv(header);
        n.blob_aus_csv(rows);
        // ziele() und bestand() VOR schliessen() lesen -- danach sind Stamm-/Kind-Objekt freigegeben
        // und beide Auskuenfte waeren leer bzw. "FEHLT" (dieselbe Lese-Reihenfolge wie
        // [MAPPE-BESTAND] an den Seams).
        std::string const     ziele_txt   = n.ziele();
        std::string const     bestand_txt = n.bestand();
        lab::MaschinenSysinfo sysinfo;
        sysinfo.hostname = host;
        bool const ok    = n.schliessen(sysinfo, {}, {});
        if (!ok || n.verworfen() != 0) {
            std::cerr << "[MAPPE-BINARY] FEHLER binary_id='" << binary_id << "' ok=" << (ok ? "1" : "0") << " "
                      << bestand_txt << " " << n.diagnose() << "\n";
            return;
        }
        std::cout << "  [MAPPE-BINARY] binary_id='" << binary_id << "' " << bestand_txt
                  << (ziele_txt.empty() ? std::string{} : "  ziele=" + ziele_txt) << "\n";
    };
}

} // namespace comdare::cache_engine::lager_naht
