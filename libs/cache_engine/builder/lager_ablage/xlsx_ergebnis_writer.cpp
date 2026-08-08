// builder/lager_ablage/xlsx_ergebnis_writer.cpp -- A9-S3: das xlsx-Backend, EIGENE TU.
//
// GRUND FUER DIE TU-GRENZE (Design-Dossier 4.1): <xlsxwriter.h> darf NICHT in die oeffentliche
// Schnittstelle (ergebnis_mappe.hpp) lecken -- Nicht-Nutzer der Lager-Ablage-Strecke (golden-/CI-Pfad)
// duerfen keinen Link-Zwang auf den Vendor bekommen. XlsxErgebnisBlatt/XlsxErgebnisMappe leben deshalb
// in einem anonymen Namespace dieser Datei; nach aussen sichtbar ist ausschliesslich
// detail::erzeuge_xlsx_ergebnis_mappe() (in ergebnis_mappe.hpp nur DEKLARIERT).
//
// DETERMINISMUS: der Vendor-Bau (ext/io/libxlsxwriter/CMakeLists.txt) setzt USE_DTOA_LIBRARY (Milo-Yip-
// dtoa, MIT, locale-unabhaengig) -- worksheet_write_number() rendert double-Werte deshalb byte-
// deterministisch. Diese TU traegt dazu bei, INDEM Zahlen als echte Zahlenzellen geschrieben werden
// (nicht als Text): ein Feld wird numerisch geschrieben, wenn std::from_chars die GESAMTE Zeichenkette
// verbraucht (kein Suffix uebrig) -- sonst als Text. Die honest-Zell-Token (failed/gesperrt/
// nicht_gebaut/n/a/-) parsen NIE als Zahl und landen deshalb automatisch im Text-Zweig: KEINE
// Sonderbehandlung noetig, Token-Treue folgt aus der Parse-Eigenschaft selbst.
//
// ATOMARITAET (IErgebnisMappe::schliessen()): die Mappe wird unter einem ".xlsx.tmp"-Pfad angelegt
// (workbook_new() im Konstruktor -- die Bibliothek schreibt ohnehin erst bei workbook_close()
// tatsaechlich Daten), und erst nach erfolgreichem workbook_close() auf den finalen Namen umbenannt.
// Ein Leser sieht NIE eine unfertige Datei unter dem finalen Namen.

#include "ergebnis_mappe.hpp"

#include <xlsxwriter.h>

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::lager_ablage {

namespace {

/// Ist `feld` als Ganzes eine endliche Dezimalzahl? std::from_chars ist locale-unabhaengig (anders als
/// strtod) -- dieselbe Determinismus-Ueberlegung, die den Vendor-Bau zu USE_DTOA_LIBRARY gefuehrt hat.
///
/// BEWUSSTE, BENANNTE LUECKE (Owner-Review 08.08.): std::from_chars fuer double erkennt per C++-Norm
/// auch die woertlichen Tokens "nan"/"inf"/"infinity" (gross-/kleinschreibungs-unabhaengig) und rendert
/// sie dann als ZAHLENZELLE statt als Text. Die REALEN Zell-Token dieser Strecke ("-", "n/a", "failed",
/// "gesperrt", "nicht_gebaut") sind davon NICHT betroffen -- keines von ihnen parst als Zahl. Die Luecke
/// bleibt trotzdem offen: ein KUENFTIGES Feld, das woertlich "nan" oder "inf" traegt (z.B. ein IEEE-754-
/// Diagnosewert aus einer neuen Mess-Quelle), wuerde hier still zur Zahl. Kein Sonderfall eingebaut,
/// weil es heute keinen belegten Aufrufer gibt, der das braucht -- wird einer sichtbar, gehoert die
/// Ausnahme HIERHIN, nicht an den Aufrufer.
[[nodiscard]] bool ist_reine_zahl(std::string const& feld, double& wert) noexcept {
    if (feld.empty()) return false;
    auto const [ptr, ec] = std::from_chars(feld.data(), feld.data() + feld.size(), wert);
    return ec == std::errc{} && ptr == feld.data() + feld.size();
}

/// EIN xlsx-Sheet. Zeilen sind 0-indiziert (kopf() belegt Zeile 0); die Zeilenlimit-Wache greift VOR
/// jedem Schreiben -- kein stilles Truncate.
class XlsxErgebnisBlatt final : public IErgebnisBlatt {
public:
    /// url_format ist NULL fuer Sheets, die nie zeile_mit_verweis() nutzen (z.B. INFO) -- ein
    /// tatsaechlicher Aufruf ohne Format waere ein Programmierfehler, kein Laufzeit-Fall (assert unten).
    XlsxErgebnisBlatt(lxw_worksheet* blatt, lxw_format* url_format) : blatt_(blatt), url_format_(url_format) {}

    void kopf(std::span<std::string const> spalten) override { schreibe_zeile(spalten); }
    void zeile(std::span<std::string const> felder) override { schreibe_zeile(felder); }

    void zeile_mit_verweis(std::span<std::string const> felder_vor_verweis, std::string const& verweis_text,
                           std::string const& ziel_sheet, std::string const& ziel_zelle,
                           std::span<std::string const> felder_nach_verweis) override {
        pruefe_zeilenlimit();
        lxw_col_t col = 0;
        for (auto const& f : felder_vor_verweis) schreibe_zelle(col++, f);
        schreibe_verweis_zelle(col++, verweis_text, ziel_sheet, ziel_zelle);
        for (auto const& f : felder_nach_verweis) schreibe_zelle(col++, f);
        ++naechste_zeile_;
    }

private:
    void pruefe_zeilenlimit() {
        if (!xlsx_zeile_erlaubt(naechste_zeile_))
            throw ErgebnisSchreibFehler(measurement::LagerAblageFehlerKlasse::Zeilenlimit,
                                        "Zeile " + std::to_string(naechste_zeile_) + " erreicht das xlsx-Limit " +
                                            std::to_string(kXlsxZeilenlimit));
    }

    /// Numerisch, wenn die GESAMTE Zeichenkette als Zahl geparst wird -- sonst Text (s. Kopf-Kommentar).
    void schreibe_zelle(lxw_col_t spalte, std::string const& feld) {
        double    zahl = 0.0;
        lxw_error err;
        if (ist_reine_zahl(feld, zahl))
            err = worksheet_write_number(blatt_, naechste_zeile_, spalte, zahl, nullptr);
        else
            err = worksheet_write_string(blatt_, naechste_zeile_, spalte, feld.c_str(), nullptr);
        if (err != LXW_NO_ERROR)
            throw ErgebnisSchreibFehler(measurement::LagerAblageFehlerKlasse::ZipFehler,
                                        "worksheet_write bei Zeile " + std::to_string(naechste_zeile_) + " Spalte " +
                                            std::to_string(spalte) +
                                            ": lxw_error=" + std::to_string(static_cast<int>(err)));
    }

    /// Interner Hyperlink: worksheet_write_url() traegt die Verknuepfung, ein NACHFOLGENDER
    /// worksheet_write_string() auf DERSELBEN Zelle ueberschreibt nur den sichtbaren Text -- die
    /// Verknuepfung bleibt erhalten (dokumentiertes Muster, worksheet.h:2820-2831/:2836-2877).
    void schreibe_verweis_zelle(lxw_col_t spalte, std::string const& verweis_text, std::string const& ziel_sheet,
                                std::string const& ziel_zelle) {
        std::string const ziel    = "internal:'" + ziel_sheet + "'!" + ziel_zelle;
        lxw_error const   err_url = worksheet_write_url(blatt_, naechste_zeile_, spalte, ziel.c_str(), nullptr);
        if (err_url != LXW_NO_ERROR)
            throw ErgebnisSchreibFehler(measurement::LagerAblageFehlerKlasse::ZipFehler,
                                        "worksheet_write_url '" + ziel +
                                            "': lxw_error=" + std::to_string(static_cast<int>(err_url)));
        lxw_error const err_text =
            worksheet_write_string(blatt_, naechste_zeile_, spalte, verweis_text.c_str(), url_format_);
        if (err_text != LXW_NO_ERROR)
            throw ErgebnisSchreibFehler(measurement::LagerAblageFehlerKlasse::ZipFehler,
                                        "worksheet_write_string (Verweis-Text) '" + verweis_text +
                                            "': lxw_error=" + std::to_string(static_cast<int>(err_text)));
    }

    void schreibe_zeile(std::span<std::string const> felder) {
        pruefe_zeilenlimit();
        for (std::size_t col = 0; col < felder.size(); ++col) schreibe_zelle(static_cast<lxw_col_t>(col), felder[col]);
        ++naechste_zeile_;
    }

    lxw_worksheet* blatt_;
    lxw_format*    url_format_;
    std::uint32_t  naechste_zeile_ = 0;
};

/// EINE xlsx-Datei. Haelt die Sheets in Aufrufreihenfolge (SheetSchluessel -> Index, lineare Suche --
/// die Sheet-Zahl je Mappe ist klein, eine Hash-Map waere hier ein ungenutzter Mehrwert).
class XlsxErgebnisMappe final : public IErgebnisMappe {
public:
    XlsxErgebnisMappe(std::filesystem::path verzeichnis, std::string dateiname_stamm)
        : verzeichnis_(std::move(verzeichnis)), stamm_(std::move(dateiname_stamm)),
          tmp_pfad_(verzeichnis_ / (stamm_ + ".xlsx.tmp")) {
        mappe_ = workbook_new(tmp_pfad_.string().c_str());
        if (mappe_ == nullptr)
            throw ErgebnisSchreibFehler(measurement::LagerAblageFehlerKlasse::ZipFehler,
                                        tmp_pfad_.string() + ": workbook_new fehlgeschlagen");
        // Fassung 3: EINMAL geholt, an jedes Blatt weitergereicht (workbook_get_default_url_format
        // braucht das Mappen-Handle, nicht das Sheet-Handle -- worksheet.h:2820-2831).
        url_format_ = workbook_get_default_url_format(mappe_);
    }

    ~XlsxErgebnisMappe() override {
        // schliessen() nie aufgerufen: Vendor-Speicher trotzdem freigeben, aber KEINE Datei unter dem
        // finalen Namen sichtbar machen (Atomaritaets-Vertrag gilt auch beim vorzeitigen Abbruch). Ein
        // Dtor wirft nicht -- der Rueckgabewert von workbook_close() wird hier bewusst NICHT gemeldet;
        // der honest-Fehlerpfad ist ausschliesslich schliessen().
        if (mappe_ != nullptr) {
            workbook_close(mappe_);
            std::error_code ec;
            std::filesystem::remove(tmp_pfad_, ec);
        }
    }

    XlsxErgebnisMappe(XlsxErgebnisMappe const&)            = delete;
    XlsxErgebnisMappe& operator=(XlsxErgebnisMappe const&) = delete;

    void info_blatt(MaschinenSysinfo const& sysinfo, HauptAchsenBelegung const& haupt,
                    KonstantenMeta const& konstanten) override {
        sysinfo_    = sysinfo;
        haupt_      = haupt;
        konstanten_ = konstanten;
    }

    IErgebnisBlatt& blatt(SheetSchluessel const& schluessel) override {
        for (std::size_t i = 0; i < schluessel_.size(); ++i)
            if (schluessel_[i] == schluessel) return *blaetter_[i];

        std::string const label = sheet_label_fuer_index(++naechster_sheet_index_);
        lxw_worksheet*    ws    = workbook_add_worksheet(mappe_, label.c_str());
        if (ws == nullptr)
            throw ErgebnisSchreibFehler(measurement::LagerAblageFehlerKlasse::ZipFehler,
                                        label + ": workbook_add_worksheet fehlgeschlagen");
        schluessel_.push_back(schluessel);
        labels_.push_back(label);
        blaetter_.push_back(std::make_unique<XlsxErgebnisBlatt>(ws, url_format_));
        return *blaetter_.back();
    }

    IErgebnisBlatt& mess_ebene_blatt(MessEbenenSchluessel const& schluessel) override {
        for (std::size_t i = 0; i < mess_ebenen_schluessel_.size(); ++i)
            if (mess_ebenen_schluessel_[i] == schluessel) return *mess_ebenen_blaetter_[i];

        ++naechster_sheet_index_; // gemeinsamer Zaehler mit blatt() -- s. ergebnis_mappe.hpp Abschnitt 2B
        std::string const label = mess_ebene_sheetname(schluessel.ebene, schluessel.bezeichner, naechster_sheet_index_);
        lxw_worksheet*    ws    = workbook_add_worksheet(mappe_, label.c_str());
        if (ws == nullptr)
            throw ErgebnisSchreibFehler(measurement::LagerAblageFehlerKlasse::ZipFehler,
                                        label + ": workbook_add_worksheet fehlgeschlagen");
        mess_ebenen_schluessel_.push_back(schluessel);
        mess_ebenen_labels_.push_back(label);
        mess_ebenen_blaetter_.push_back(std::make_unique<XlsxErgebnisBlatt>(ws, url_format_));
        return *mess_ebenen_blaetter_.back();
    }

    void schliessen() override {
        // INFO-Sheet ZULETZT anlegen: erst hier sind ALLE bis dahin angelegten Sheets fuer die Legende
        // bekannt (info_blatt() selbst darf, wie dokumentiert, vor/nach/zwischen blatt()-Aufrufen kommen).
        lxw_worksheet* info_ws = workbook_add_worksheet(mappe_, std::string{kInfoBlattName}.c_str());
        if (info_ws == nullptr)
            throw ErgebnisSchreibFehler(measurement::LagerAblageFehlerKlasse::ZipFehler,
                                        "INFO: workbook_add_worksheet fehlgeschlagen");
        XlsxErgebnisBlatt info(info_ws, url_format_);
        info.kopf(std::vector<std::string>{"kategorie", "schluessel", "wert"});
        auto const feld = [](std::string_view s) { return std::string{n_a_wenn_leer(s)}; };
        info.zeile(std::vector<std::string>{"sysinfo", "hostname", feld(sysinfo_.hostname)});
        info.zeile(std::vector<std::string>{"sysinfo", "machine_id", feld(sysinfo_.machine_id)});
        info.zeile(std::vector<std::string>{"sysinfo", "cpu_fabrication", feld(sysinfo_.cpu_fabrication)});
        info.zeile(std::vector<std::string>{"sysinfo", "ram_pair", feld(sysinfo_.ram_pair)});
        info.zeile(std::vector<std::string>{"sysinfo", "os_version", feld(sysinfo_.os_version)});
        info.zeile(std::vector<std::string>{"sysinfo", "kernel", feld(sysinfo_.kernel)});
        info.zeile(std::vector<std::string>{"sysinfo", "build", feld(sysinfo_.build)});
        info.zeile(std::vector<std::string>{"sysinfo", "identity_verdict", feld(sysinfo_.identity_verdict)});
        for (auto const& e : haupt_) info.zeile(std::vector<std::string>{"hauptachse", e.achse, e.wert});
        for (auto const& e : konstanten_) info.zeile(std::vector<std::string>{"konstante", e.achse, e.wert});
        for (std::size_t i = 0; i < labels_.size(); ++i)
            info.zeile(std::vector<std::string>{"sheet_legende", labels_[i],
                                                schluessel_[i].mess_unter + "|" + schluessel_[i].system_unter + "|" +
                                                    schluessel_[i].organ_unter});
        // Fassung 3: compare/Macro/Micro-Legende -- der Sheet-Name TRAEGT hier bereits Ebene+Bezeichner
        // (mess_ebene_sheetname), die Legende bestaetigt es trotzdem explizit (dieselbe Ehrlichkeits-
        // Redundanz wie bei der Fassung-1/2-Legende: nie auf den Namen allein verlassen).
        for (std::size_t i = 0; i < mess_ebenen_labels_.size(); ++i)
            info.zeile(std::vector<std::string>{"mess_ebene_legende", mess_ebenen_labels_[i],
                                                std::string{mess_ebene_label(mess_ebenen_schluessel_[i].ebene)} + "|" +
                                                    mess_ebenen_schluessel_[i].bezeichner});

        lxw_error const err = workbook_close(mappe_);
        // workbook_close() gibt den Vendor-Speicher IMMER frei (Erfolg wie Fehlschlag, libxlsxwriter-
        // Kontrakt) -- der Zeiger ist danach in jedem Fall ungueltig und darf im Dtor nicht erneut
        // geschlossen werden.
        mappe_ = nullptr;
        if (err != LXW_NO_ERROR) {
            std::error_code ec;
            std::filesystem::remove(tmp_pfad_, ec);
            throw ErgebnisSchreibFehler(measurement::LagerAblageFehlerKlasse::ZipFehler,
                                        tmp_pfad_.string() +
                                            ": workbook_close lxw_error=" + std::to_string(static_cast<int>(err)));
        }

        auto const      final_pfad = verzeichnis_ / (stamm_ + ".xlsx");
        std::error_code ec;
        std::filesystem::rename(tmp_pfad_, final_pfad, ec);
        if (ec)
            throw ErgebnisSchreibFehler(measurement::LagerAblageFehlerKlasse::ZipFehler,
                                        tmp_pfad_.string() + " -> " + final_pfad.string() + ": " + ec.message());
    }

private:
    std::filesystem::path                           verzeichnis_;
    std::string                                     stamm_;
    std::filesystem::path                           tmp_pfad_;
    lxw_workbook*                                   mappe_      = nullptr;
    lxw_format*                                     url_format_ = nullptr;
    MaschinenSysinfo                                sysinfo_{};
    HauptAchsenBelegung                             haupt_{};
    KonstantenMeta                                  konstanten_{};
    std::size_t                                     naechster_sheet_index_ = 0; // geteilt: blatt()+mess_ebene_blatt()
    std::vector<SheetSchluessel>                    schluessel_;
    std::vector<std::string>                        labels_;
    std::vector<std::unique_ptr<XlsxErgebnisBlatt>> blaetter_;
    // Fassung 3 (parallele Buchhaltung, EIGENE Sheet-Zaehlung -- s. ergebnis_mappe.hpp Abschnitt 2B).
    std::vector<MessEbenenSchluessel>               mess_ebenen_schluessel_;
    std::vector<std::string>                        mess_ebenen_labels_;
    std::vector<std::unique_ptr<XlsxErgebnisBlatt>> mess_ebenen_blaetter_;
};

} // namespace

namespace detail {

std::unique_ptr<IErgebnisMappe> erzeuge_xlsx_ergebnis_mappe(std::filesystem::path const& verzeichnis,
                                                            std::string const&           dateiname_stamm) {
    return std::make_unique<XlsxErgebnisMappe>(verzeichnis, dateiname_stamm);
}

} // namespace detail

} // namespace comdare::cache_engine::builder::lager_ablage
