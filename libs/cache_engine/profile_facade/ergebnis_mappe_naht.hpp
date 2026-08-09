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
// RICHTUNG (Owner 2026-08-09): xlsx -> csv, NIE umgekehrt. Es gibt hier keinen Parser, der eine
// fertige CSV-DATEI einliest, um daraus eine Mappe zu bauen. Die Mappe wird aus DENSELBEN
// In-Memory-Zeilen gespeist, aus denen auch die rohe CSV entsteht (format_csv_row()), und CSV ist
// nicht Transportformat, sondern eine AUSGABE-STRATEGIE derselben Mappe (CsvErgebnisMappe).
//
// I/O-LAGE (Contention-Doktrin, Design-Dossier 20260803-a9_xlsx_writer_f3_soll_design.md V-A9-4):
// die Zeilen werden im Mess-Fenster nur ENTGEGENGENOMMEN (bei xlsx rein in den libxlsxwriter-Speicher,
// kein Datei-Zugriff); der EINZIGE Datei-Schreibvorgang der xlsx-Strategie liegt in schliessen(), das
// der Aufrufer NACH der Mess-Schleife ruft -- neben csv.flush(). Siehe Nachtrag in ebendiesem Dossier.
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
// 1. FORMAT-ENTSCHEIDUNG -- exactly-one, aber ueber der RICHTIGEN Grundgesamtheit
// =================================================================================================
//
// DIE FALLE, die hier bewusst NICHT gebaut wird: die exactly-one-Regel von <run_methodology>
// (validate_profile.hpp: "if (tp.run_methodology.size() > 1) ... GENAU EINE erlaubt") laesst sich
// NICHT wortwoertlich auf <writeback_methods> uebertragen. Gemessen am Objekt:
// all_axes_golden.profile.xml:216-220 -- das Profil, das .gitlab-ci.yml:93 woertlich "das
// golden-Profil der Abgabe-Messung" nennt -- deklariert DREI Eintraege:
//     <method value="csv"/> <method value="latex_table"/> <method value="comparison_metrics"/>
// Eine Regel `writeback_methods.size() > 1 => Fehler` braeche also exakt die Abgabe-Messung.
//
// Der Grund ist semantisch, nicht kosmetisch: {debug,measure,release} sind ALTERNATIVEN derselben
// Frage (welcher Modus?), {csv,latex_table,comparison_metrics,xlsx} sind es NICHT.
// writeback_method_registry.hpp sagt ueber sich selbst, die Achse sei "die EHRLICHE Formalisierung
// des heutigen <output>-Trios" -- also DREI UNABHAENGIGE Ausgabe-Kanaele (Roh-CSV, LaTeX-Tabelle,
// SOTA-Vergleichsmetriken), von denen mehrere gleichzeitig sinnvoll sind.
//
// Die exactly-one-Regel gilt deshalb ueber der Teilmenge, die tatsaechlich eine ALTERNATIVE bildet:
// den FORMAT-Token {csv, xlsx}. latex_table/comparison_metrics sind fuer DIESE Frage kein Konflikt,
// sondern anderer Kanal. NENNER: die Zahl steht nie nackt da -- FormatWahl traegt sowohl die
// Grundgesamtheit (token_gesamt) als auch die Teilmenge (format_token), ueber der entschieden wurde,
// und diagnose() rendert beide fuer die Lauf-AUSGABE.

/// Welche der FORMAT-Token {csv,xlsx} eine id bezeichnet -- oder keine.
enum class TokenSorte : std::uint8_t {
    format,    ///< csv | xlsx -- eine ALTERNATIVE, ueber die exactly-one entscheidet
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

/// Lage der Format-Entscheidung. `default_leer` ist KEIN Fehler (Owner-KERN-Default), `mehrdeutig`
/// schon -- dann wurde NICHT geraten.
enum class FormatLage : std::uint8_t {
    default_leer, ///< kein FORMAT-Token deklariert -> xlsx (Owner-KERN 26.07.)
    gewaehlt,     ///< genau ein FORMAT-Token deklariert -> dieses
    mehrdeutig,   ///< csv UND xlsx deklariert -> exactly-one verletzt, es wird NICHT geraten
};

struct FormatWahl {
    lab::ErgebnisFormat format = lab::ErgebnisFormat::xlsx;
    FormatLage          lage   = FormatLage::default_leer;
    // NENNER-Trias: nie eine nackte Zahl. token_gesamt ist die Grundgesamtheit (alle deklarierten
    // Eintraege), format_token die Teilmenge, ueber der exactly-one entschied, unbekannt_token die
    // Eintraege, die in KEINER Registry-Zeile stehen.
    std::size_t token_gesamt    = 0;
    std::size_t format_token    = 0;
    std::size_t kanal_token     = 0;
    std::size_t unbekannt_token = 0;
    std::string unbekannt_liste; ///< die unbekannten ids selbst, komma-getrennt (nie nur gezaehlt)

    [[nodiscard]] bool eindeutig() const noexcept { return lage != FormatLage::mehrdeutig; }

    /// Menschenlesbare Fassung MIT Grundgesamtheit -- gehoert in die Lauf-AUSGABE, nicht nur in den
    /// Kopf des Lesers (GOAL v8 Prueffrage 1).
    [[nodiscard]] std::string diagnose() const {
        std::string s = "format=";
        s += (format == lab::ErgebnisFormat::xlsx) ? "xlsx" : "csv";
        s += " lage=";
        switch (lage) {
            case FormatLage::default_leer: s += "default_leer"; break;
            case FormatLage::gewaehlt: s += "gewaehlt"; break;
            case FormatLage::mehrdeutig: s += "mehrdeutig"; break;
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

    // Ein doppeltes "csv;csv" ist KEINE Mehrdeutigkeit -- es benennt zweimal dieselbe Alternative.
    // Mehrdeutig ist nur der echte Konflikt: beide Formate gleichzeitig verlangt.
    if (csv_gesehen && xlsx_gesehen) {
        w.lage   = FormatLage::mehrdeutig;
        w.format = lab::ErgebnisFormat::xlsx; // Rueckfallwert; der Aufrufer DARF ihn nicht benutzen
        return w;
    }
    if (csv_gesehen) {
        w.lage   = FormatLage::gewaehlt;
        w.format = lab::ErgebnisFormat::csv;
        return w;
    }
    if (xlsx_gesehen) {
        w.lage   = FormatLage::gewaehlt;
        w.format = lab::ErgebnisFormat::xlsx;
        return w;
    }
    // LEER, FEHLEND, oder ausschliesslich Kanal-/Unbekannt-Token -> xlsx (Owner-KERN-Default).
    w.lage   = FormatLage::default_leer;
    w.format = lab::ErgebnisFormat::xlsx;
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
    StammErgebnis          r;
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
// WIRFT NIE nach aussen: ein Mappen-Fehler darf einen laufenden Mess-Lauf nicht abbrechen, solange
// die Mappe additiv neben der offiziellen CSV steht. Jeder Fehlausgang setzt scharf()==false und
// hinterlaesst seinen Grund in diagnose(); der Aufrufer gibt ihn aus. Beobachtbar, nicht verdeckt.
class MappenNaht {
public:
    MappenNaht()                             = default;
    MappenNaht(MappenNaht const&)            = delete;
    MappenNaht& operator=(MappenNaht const&) = delete;

    /// Oeffnet die Mappe NEBEN der offiziellen CSV (dasselbe Verzeichnis -- die Lagerbaum-Stelle wird
    /// hier nicht neu erfunden, sie wird von der bereits getroffenen Wahl des Aufrufers geerbt).
    void oeffnen(std::filesystem::path const& out_csv, std::vector<std::string> const& writeback_methods) {
        wahl_ = waehle_ergebnis_format(writeback_methods);
        if (!wahl_.eindeutig()) {
            diagnose_ = wahl_.diagnose() +
                        " -- csv UND xlsx gleichzeitig deklariert: exactly-one verletzt, es wird NICHT geraten";
            return;
        }

        std::string datum;
        std::string zeit;
        jetzt_datum_zeit(datum, zeit);
        std::string_view const endung = (wahl_.format == lab::ErgebnisFormat::xlsx) ? "xlsx" : "csv";

        auto const stamm = mappen_stamm(datum, zeit, endung);
        if (!stamm.ok) {
            diagnose_ = wahl_.diagnose() + " -- " + stamm.fehler;
            return;
        }

        auto verzeichnis = out_csv.parent_path();
        if (verzeichnis.empty()) verzeichnis = ".";

        try {
            std::error_code ec;
            std::filesystem::create_directories(verzeichnis, ec); // vorhandenes Verzeichnis ist kein Fehler
            mappe_ = lab::ErgebnisMappenFactory::oeffne(verzeichnis, stamm.text, wahl_.format);
            blatt_ = &mappe_->blatt(lab::SheetSchluessel{});
            ziel_  = verzeichnis / (wahl_.format == lab::ErgebnisFormat::xlsx
                                        ? stamm.text + ".xlsx"
                                        : stamm.text + "__S001.csv");
            diagnose_ = wahl_.diagnose();
        } catch (std::exception const& e) {
            mappe_.reset();
            blatt_    = nullptr;
            diagnose_ = wahl_.diagnose() + " -- oeffnen fehlgeschlagen: " + e.what();
        }
    }

    [[nodiscard]] bool                         scharf() const noexcept { return blatt_ != nullptr; }
    [[nodiscard]] FormatWahl const&            wahl() const noexcept { return wahl_; }
    [[nodiscard]] std::string const&           diagnose() const noexcept { return diagnose_; }
    [[nodiscard]] std::filesystem::path const& ziel() const noexcept { return ziel_; }
    [[nodiscard]] std::size_t                  zeilen() const noexcept { return zeilen_; }
    /// Zeilen, deren Feldzahl NICHT der Kopfzeile entsprach -- die Wache gegen stille Spalten-
    /// Verschiebung. Muss bei intaktem Schema 0 sein.
    [[nodiscard]] std::size_t feld_abweichungen() const noexcept { return feld_abweichungen_; }
    /// Grundgesamtheit, ueber der feld_abweichungen() zaehlt: die Zahl der Spalten der Kopfzeile.
    [[nodiscard]] std::size_t kopf_spalten() const noexcept { return kopf_spalten_; }

    /// Die Kopfzeile -- genau der String, den lazy_csv_header() liefert.
    void kopf_aus_csv(std::string_view header_zeile) {
        if (!scharf()) return;
        auto const felder = csv_zeile_in_felder(header_zeile);
        kopf_spalten_     = felder.size();
        schreibe(felder, true);
    }

    /// EINE Zeile -- genau der String, den format_csv_row() liefert.
    void zeile_aus_csv(std::string_view csv_zeile) {
        if (!scharf()) return;
        auto const felder = csv_zeile_in_felder(csv_zeile);
        if (felder.empty()) return; // Leerzeile: nichts zu schreiben, kein Fehler
        if (kopf_spalten_ != 0 && felder.size() != kopf_spalten_) ++feld_abweichungen_;
        schreibe(felder, false);
    }

    /// Ein mehrzeiliger Block (LazyRunResult::resumed_csv_rows -- resumierte Alt-Zeilen).
    void blob_aus_csv(std::string_view blob) {
        if (!scharf()) return;
        for (auto const z : csv_blob_in_zeilen(blob)) zeile_aus_csv(z);
    }

    /// Schreibt INFO-Blatt und schliesst die Mappe. DAS ist der einzige Punkt, an dem die
    /// xlsx-Strategie die Platte anfasst -- der Aufrufer ruft ihn NACH der Mess-Schleife.
    /// Rueckgabe false = die Mappe steht NICHT vollstaendig auf der Platte (Grund in diagnose()).
    bool schliessen(lab::MaschinenSysinfo const& sysinfo, lab::HauptAchsenBelegung const& haupt,
                    lab::KonstantenMeta const& konstanten) {
        if (mappe_ == nullptr) return false;
        try {
            mappe_->info_blatt(sysinfo, haupt, konstanten);
            mappe_->schliessen();
            mappe_.reset();
            blatt_ = nullptr;
            return true;
        } catch (std::exception const& e) {
            mappe_.reset();
            blatt_ = nullptr;
            diagnose_ += std::string{" -- schliessen fehlgeschlagen: "} + e.what();
            return false;
        }
    }

private:
    void schreibe(std::vector<std::string> const& felder, bool ist_kopf) {
        try {
            if (ist_kopf) {
                blatt_->kopf(felder);
            } else {
                blatt_->zeile(felder);
                ++zeilen_;
            }
        } catch (std::exception const& e) {
            // Zeilenlimit / Schreibfehler: die Mappe wird stillgelegt, aber LAUT -- ab hier ist
            // scharf() false, und der Grund steht in diagnose(). Der CSV-Pfad laeuft unberuehrt weiter.
            mappe_.reset();
            blatt_ = nullptr;
            diagnose_ += std::string{" -- schreiben abgebrochen: "} + e.what();
        }
    }

    std::unique_ptr<lab::IErgebnisMappe> mappe_;
    lab::IErgebnisBlatt*                 blatt_ = nullptr;
    FormatWahl                           wahl_{};
    std::string                          diagnose_ = "nicht geoeffnet";
    std::filesystem::path                ziel_;
    std::size_t                          zeilen_            = 0;
    std::size_t                          kopf_spalten_      = 0;
    std::size_t                          feld_abweichungen_ = 0;
};

} // namespace comdare::cache_engine::lager_naht
