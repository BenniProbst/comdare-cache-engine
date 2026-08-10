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
// ADDITIV HEISST WOERTLICH: der bestehende rohe CSV-Pfad (std::ofstream csv{a.out_csv...}) wird NICHT
// angefasst, nicht umgeleitet, nicht ersetzt. golden bleibt byte-identisch. Das ERSETZEN des rohen
// CSV-Pfads ist ein spaeterer, Owner-freizugebender Schritt und ausdruecklich NICHT dieses Paket.
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
    [[nodiscard]] bool               scharf() const noexcept { return stamm_blatt_ != nullptr; }
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

        kind_blatt_  = nullptr;
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
            kind_blatt_  = nullptr; // das Kind faellt MIT -- ohne Stamm keine gueltige Ableitung
            if (!ist_kopf) ++verworfen_;
            return;
        }
        if (!ist_kopf) {
            ++zeilen_;
            if (!zeilen_je_blatt_.empty()) ++zeilen_je_blatt_.front();
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

    FormatWahl               wahl_{};
    std::string              diagnose_ = "nicht geoeffnet";
    std::vector<std::size_t> zeilen_je_blatt_;   ///< der BESTAND im Speicher, je Blatt der Mappe
    std::size_t              angeboten_         = 0; ///< Grundgesamtheit: hereingereichte Datenzeilen
    std::size_t              verworfen_         = 0; ///< angeboten, aber von der Mappe nicht angenommen
    std::size_t              zeilen_            = 0;
    std::size_t              kopf_spalten_      = 0;
    std::size_t              feld_abweichungen_ = 0;
};

} // namespace comdare::cache_engine::lager_naht
