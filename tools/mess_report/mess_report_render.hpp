#pragma once
// tools/mess_report/mess_report_render.hpp -- A9-S4: die Substanz von `render`/`plan`. main.cpp
// bleibt duenn (Host-Rolle: Argumente aufloesen, diese Funktionen aufrufen, Exit-Code liefern) --
// Haus-Muster wie apps/experiment_planner/main.cpp (Substanz in einer Fassade/Bibliothek).
//
// EIN GRUPPIERUNGS-ALGORITHMUS FUER BEIDE QUELLFORMEN (--csv und --realm-root): die Bestands-WIDE-CSV
// traegt `binary_id` als Pflichtspalte (real verifiziert: measurements.csv UND jede
// per_binary/*.result.csv-Datei des Erstbeleg-Archivs, A9-S5). Gruppierung nach binary_id
// rekonstruiert bei --realm-root automatisch "eine Datei = eine Gruppe" (jede per_binary-Datei
// traegt bereits nur EINE binary_id) und splittet bei --csv eine Aggregat-Datei in dieselben
// Gruppen auf -- EIN Algorithmus, kein Sonderfall je Quellform.
//
// SheetSchluessel-POLITIK (bewusste Caller-Entscheidung, s. ergebnis_mappe.hpp-Kommentar
// "SheetSchluessel ... ist nur der SCHLUESSEL" -- keine Registry-Pflicht): organ_unter = binary_id
// (die Bin-Zusammensetzung ist ueberwiegend Organ-Achsen-Herkunft, NOTIZ-binary-id-Grammatik der
// Erstbeleg-Archive); mess_unter/system_unter bleiben LEER ("diese Unter-Ebene ist hier nicht
// noetig", SheetSchluessel-Doc) -- dieses Werkzeug splittet (noch) nicht weiter danach. Eine
// feinere Aufteilung ist additiv nachruestbar, sobald ein Konsument sie braucht.
//
// DATEINAME/KV-KETTE (A9-Doc Abschnitt 5, woertlich): "nie sich aendernde Variablen werden
// WEGGELASSEN (Meta-Eintrag im INFO-Blatt)" -- NUR dynamische (ueber die Gruppen hinweg
// mehrwertige) Kandidaten-Spalten tragen `achse=sweep` im Dateinamen; konstante/leere Kandidaten
// landen AUSSCHLIESSLICH in KonstantenMeta, NIE im Dateinamen (kein Sonderfall fuer "konstant, aber
// meinungsvoll" -- die Doktrin nennt nur die eine Regel).
//
// FASSUNG 3 (checkpoint_measure, ce cc028e1d): NICHT GEBAUT. Ein Quellen-CSV ohne die 8
// Pflichtspalten bricht bei --fassung3 LAUT ab -- KEINE Platzhalter, KEIN stiller Fassung-1/2-
// Rueckfall (Owner/Lead-Entscheid A9-S4-Plan Punkt 2).

#include "dynamic_axis_filter.hpp"
#include "realm_scan.hpp"
#include "skip_manifest.hpp"

#include <builder/lager_ablage/ergebnis_mappe.hpp>
#include <cache_engine/measurement/csv_cell_reader.hpp>

#include <array>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <ostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::tools::mess_report {

namespace lab  = ::comdare::cache_engine::builder::lager_ablage;
namespace bl   = ::comdare::cache_engine::builder::bestandslog;
namespace csvu = ::comdare::cache_engine::measurement::csv;

/// checkpoint_measure SOLL-Design (ce cc028e1d) -- die 8-Spalten-Schema-Wahrheit, hier NUR fuer den
/// lauten Fehlend-Spalten-Abbruch von --fassung3 gebraucht (dieselbe Liste wie
/// tests/unit/test_a9s3_mess_ebene_blattsorte.cpp:249, wo die Fassung-3-Schreiber-Seite schon
/// getestet ist).
inline constexpr std::array<std::string_view, 8> kFassung3PflichtSpalten = {
    "prozess", "thread", "mess_ebene", "ziel", "aufrufer", "checkpoint", "zeitpunkt", "messwert"};

/// Kuratierte Kandidaten-Liste der Unter-Achsen-artigen Spalten (KEINE Achsen-Registry-
/// Klassifikation -- s. dynamic_axis_filter.hpp Kopf-Kommentar). Herkunft: SPALTEN-WAHRHEIT-
/// Kommentar in heuristik/measurement_curve_loader.hpp (workload/working_set_n/platform/
/// build_version/series/sweep_axis) + real verifizierte Erstbeleg-Kopfzeile (setting/quality_flag/
/// pruefling_type/fairness_mode, A9-S5).
inline constexpr std::array<std::string_view, 10> kUnterAchsenKandidaten = {
    "workload",   "working_set_n", "platform",     "build_version",  "series",
    "sweep_axis", "setting",       "quality_flag", "pruefling_type", "fairness_mode"};

struct MessReportFehler final : std::runtime_error {
    explicit MessReportFehler(std::string const& was) : std::runtime_error(was) {}
};

// =====================================================================================================
// 1. Wide-CSV-Lesen (semikolon-getrennt, header-getrieben -- Single-Source csv_cell_reader.hpp)
// =====================================================================================================

struct GelesenesCsv {
    std::filesystem::path                 quelle;
    std::vector<std::string>              header;
    std::vector<std::vector<std::string>> zeilen;
};

[[nodiscard]] inline GelesenesCsv lies_wide_csv(std::filesystem::path const& pfad) {
    std::ifstream in(pfad, std::ios::binary);
    if (!in.is_open()) throw MessReportFehler("Quelle nicht lesbar: " + pfad.string());
    GelesenesCsv out;
    out.quelle = pfad;
    std::string kopfzeile;
    if (!std::getline(in, kopfzeile)) return out; // ehrlich leer (Aufrufer meldet 0 Zeilen)
    out.header = csvu::split_csv_line(kopfzeile, ';');
    std::string zeile;
    while (std::getline(in, zeile)) {
        if (zeile.empty() || zeile == "\r") continue;
        out.zeilen.push_back(csvu::split_csv_line(zeile, ';'));
    }
    return out;
}

// =====================================================================================================
// 2. Zeit -- Wall-Clock fuer render, Epochen-Platzhalter fuer plan (Determinismus, LED-61)
// =====================================================================================================

struct DatumZeit {
    std::string datum; // YYYYMMDD
    std::string zeit;  // HHMMSS
};

[[nodiscard]] inline DatumZeit jetzt_utc() {
    std::time_t const t = std::time(nullptr);
    std::tm           tm{};
    gmtime_r(&t, &tm);
    char buf_d[9];
    std::snprintf(buf_d, sizeof buf_d, "%04d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    char buf_z[7];
    std::snprintf(buf_z, sizeof buf_z, "%02d%02d%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    return {std::string(buf_d), std::string(buf_z)};
}

/// plan druckt den Ziel-Baum literal und MUSS bei zwei Laeufen byte-gleich sein (LED-61); der
/// Dateiname traegt aber strukturell Wall-Clock-Zeit (A9-Doc Abschnitt 5). Der Platzhalter loest den
/// Widerspruch NACH derselben Doktrin wie die ZIP-Zeitstempel im xlsx-Backend ("auf Epoche
/// gepinnt") -- render (ohne --dry-run) nutzt jetzt_utc(), plan ausschliesslich diesen Platzhalter.
inline constexpr DatumZeit kPlanZeitstempelPlatzhalter{"19700101", "000000"};

// =====================================================================================================
// 3. Gruppierung nach binary_id (Pflichtspalte -- hartes Scheitern statt Rate-Gruppierung)
// =====================================================================================================

struct ZeilenGruppe {
    std::string                           binary_id;
    std::vector<std::vector<std::string>> zeilen;
};

struct GruppierteQuelle {
    std::vector<std::string>            header;
    std::map<std::string, ZeilenGruppe> gruppen; // sortiert nach binary_id -- Determinismus (LED-61)
    std::size_t                         gelesene_zeilen              = 0;
    std::size_t                         ohne_binary_id_uebersprungen = 0;
};

/// Liest ALLE `quellen` und gruppiert ihre Zeilen gemeinsam nach `binary_id`. Alle Quellen muessen
/// DENSELBEN Header tragen (dieselbe Kopfzeilen-Form, wie es lazy_csv_header() garantiert) -- eine
/// abweichende Kopfzeile ist ein ehrlicher Abbruch, kein stilles Best-Effort-Merge zweier Schemata.
[[nodiscard]] inline GruppierteQuelle gruppiere_quellen(std::vector<std::filesystem::path> const& quellen) {
    GruppierteQuelle out;
    if (quellen.empty()) throw MessReportFehler("keine Quelle angegeben (--csv oder --realm-root)");

    for (std::filesystem::path const& q : quellen) {
        GelesenesCsv const g = lies_wide_csv(q);
        if (g.header.empty()) continue; // leere Datei -> ehrlich uebersprungen
        if (out.header.empty()) {
            out.header = g.header;
        } else if (out.header != g.header) {
            throw MessReportFehler("Header-Abweichung: '" + q.string() +
                                   "' traegt eine andere Kopfzeile als die erste gelesene Quelle -- "
                                   "kein stilles Zusammenfuehren zweier Schemata");
        }

        csvu::HeaderIndex const idx(g.header);
        std::size_t             bid_col = 0;
        if (!idx.find("binary_id", bid_col))
            throw MessReportFehler("Pflichtspalte 'binary_id' fehlt in '" + q.string() + "'");

        for (auto const& zeile : g.zeilen) {
            ++out.gelesene_zeilen;
            std::string const bid = bid_col < zeile.size() ? zeile[bid_col] : std::string{};
            if (bid.empty()) {
                ++out.ohne_binary_id_uebersprungen;
                continue;
            }
            auto& grp     = out.gruppen[bid];
            grp.binary_id = bid;
            grp.zeilen.push_back(zeile);
        }
    }
    return out;
}

// =====================================================================================================
// 3B. SKIP bei validem Bestand (vierter Owner-Nachtrag Punkt 2): "exakt gleiche binary" -> nicht
// neu schreiben. Nur im xlsx-Pfad -- ein --format=csv-Testlauf ist bewusst blind gegen den
// Skip-Zustand (skip_manifest.hpp Kopf-Kommentar).
// =====================================================================================================

struct SkipFilterErgebnis {
    std::map<std::string, ZeilenGruppe> neu;               // wird in DIESEM Lauf geschrieben
    std::vector<std::string>            bereits_vorhanden; // binary_ids, die das Manifest schon fuehrt (SKIP)
};

[[nodiscard]] inline SkipFilterErgebnis filtere_bereits_vorhandene(std::map<std::string, ZeilenGruppe> const& gruppen,
                                                                   std::filesystem::path const& ziel_verzeichnis,
                                                                   lab::ErgebnisFormat          format) {
    SkipFilterErgebnis erg;
    if (format != lab::ErgebnisFormat::xlsx) {
        erg.neu = gruppen; // CSV-Testpfad: kein Skip-Zustand gelesen, alles wird geschrieben
        return erg;
    }
    std::set<std::string> const bestand = lies_manifest(ziel_verzeichnis);
    for (auto const& [bid, grp] : gruppen) {
        if (bestand.contains(bid))
            erg.bereits_vorhanden.push_back(bid);
        else
            erg.neu.emplace(bid, grp);
    }
    return erg;
}

// =====================================================================================================
// 4. Dynamik-Kette anwenden -> Dateinamens-kvkette + KonstantenMeta
// =====================================================================================================

/// EIN Kandidat, EINMAL bewertet -- eigene (owning) Strings statt string_view, damit das Ergebnis
/// den Aufruf-Scope unbeschadet ueberlebt (KvPaar fuer blatt_dateiname() wird erst am Aufrufort
/// gebaut, s. baue_dateiname()).
struct KandidatBefund {
    std::string achse;
    std::string wert;
    bool        dynamisch = false; // true -> Dateiname traegt "sweep"; false -> nur KonstantenMeta
};

[[nodiscard]] inline std::vector<KandidatBefund> bewerte_dynamik(std::vector<std::string> const&            header,
                                                                 std::map<std::string, ZeilenGruppe> const& gruppen) {
    std::vector<KandidatBefund> out;
    csvu::HeaderIndex const     idx(header);
    DynamikKette const          kette;

    for (std::string_view kandidat : kUnterAchsenKandidaten) {
        std::size_t col = 0;
        if (!idx.find(kandidat, col)) continue; // Spalte nicht in dieser Quelle -- nicht anwendbar

        std::vector<std::string> werte;
        for (auto const& [bid, grp] : gruppen)
            for (auto const& zeile : grp.zeilen) werte.push_back(col < zeile.size() ? zeile[col] : std::string{});

        DynamikVerdikt const v = kette.bewerte(std::string{kandidat}, werte);
        KandidatBefund       b;
        b.achse     = std::string{kandidat};
        b.dynamisch = (v.entscheidung == DynamikVerdikt::Entscheidung::Dynamisch);
        b.wert      = v.wert; // "sweep" bei dynamisch, sonst der eine Wert bzw. "-"
        out.push_back(std::move(b));
    }
    return out;
}

// =====================================================================================================
// 5. Dateiname bauen (nur die dynamischen Kandidaten, A9-Doc Abschnitt 5 woertlich)
// =====================================================================================================

struct DateinameErgebnis {
    std::string text;
    bool        gekuerzt = false;
    std::string voll_kette;
    bool        ok = false;
    std::string fehler_text;
};

[[nodiscard]] inline DateinameErgebnis baue_dateiname(DatumZeit const& dz, std::vector<KandidatBefund> const& befunde,
                                                      std::string_view endung) {
    std::vector<bl::KvPaar> paare;
    for (auto const& b : befunde)
        if (b.dynamisch) paare.push_back({std::string_view{b.achse}, std::string_view{b.wert}});

    bl::PfadErgebnis const e = bl::blatt_dateiname(dz.datum, dz.zeit, paare, endung);
    DateinameErgebnis      out;
    out.gekuerzt   = e.gekuerzt;
    out.voll_kette = e.voll_kette;
    out.ok         = e.ok();
    out.text       = e.text;
    if (!out.ok) out.fehler_text = std::string{bl::to_string(e.fehler)};
    return out;
}

// =====================================================================================================
// 6. Plan (Vorschau, KEIN Schreiben) + Render (tatsaechliches Schreiben)
// =====================================================================================================

struct SheetPlanEintrag {
    std::string organ_unter; // == binary_id (SheetSchluessel-Politik, s. Kopf-Kommentar)
    std::size_t zeilen_anzahl = 0;
};

struct RenderPlan {
    std::filesystem::path         ziel_verzeichnis;
    bool                          schreibt_nichts = false; // true -> kein Dateiname berechnet, nichts zu schreiben
    std::string                   dateiname_stamm;         // OHNE Endung (s. ErgebnisMappenFactory::oeffne())
    bool                          dateiname_gekuerzt = false;
    std::string                   dateiname_voll_kette;
    std::vector<SheetPlanEintrag> sheets;
    std::size_t                   gelesene_zeilen              = 0;
    std::size_t                   ohne_binary_id_uebersprungen = 0;
    std::size_t                   stale_uebersprungen          = 0; // nur bei --realm-root belegt
    std::vector<std::string>      bereits_vorhanden_binary_ids;     // SKIP: exakt gleiche Binary schon im Manifest
    lab::KonstantenMeta           konstanten_meta;
};

/// Baut den Plan (liest Quellen, gruppiert, filtert bereits vorhandene Binaries heraus, entscheidet
/// die Dynamik-Kette NUR ueber die NEUEN Gruppen, validiert den Dateinamen) -- schreibt NICHTS.
/// render() ruft dieselbe Funktion und schreibt danach zusaetzlich; plan() ruft sie mit
/// kPlanZeitstempelPlatzhalter und schreibt nie -- beide lesen (aber schreiben nie) dasselbe
/// Skip-Manifest, damit eine Vorschau nie etwas anderes verspricht als render() tatsaechlich tut.
[[nodiscard]] inline RenderPlan berechne_plan(std::vector<std::filesystem::path> const& quellen,
                                              std::filesystem::path const& ziel_verzeichnis, DatumZeit const& dz,
                                              lab::ErgebnisFormat format, std::size_t stale_uebersprungen = 0) {
    GruppierteQuelle const   q    = gruppiere_quellen(quellen);
    SkipFilterErgebnis const skip = filtere_bereits_vorhandene(q.gruppen, ziel_verzeichnis, format);

    RenderPlan plan;
    plan.ziel_verzeichnis             = ziel_verzeichnis;
    plan.gelesene_zeilen              = q.gelesene_zeilen;
    plan.ohne_binary_id_uebersprungen = q.ohne_binary_id_uebersprungen;
    plan.stale_uebersprungen          = stale_uebersprungen;
    plan.bereits_vorhanden_binary_ids = skip.bereits_vorhanden;
    for (auto const& [bid, grp] : skip.neu) plan.sheets.push_back({bid, grp.zeilen.size()});

    if (skip.neu.empty()) {
        plan.schreibt_nichts = true; // nichts Neues -- kein Dateiname zu berechnen, keine Mappe zu oeffnen
        return plan;
    }

    auto const              befunde = bewerte_dynamik(q.header, skip.neu);
    std::string_view const  endung  = (format == lab::ErgebnisFormat::xlsx) ? "xlsx" : "csv";
    DateinameErgebnis const dn      = baue_dateiname(dz, befunde, endung);
    if (!dn.ok) throw MessReportFehler("Dateiname verletzt die Wache (fehlerklasse=" + dn.fehler_text + ")");
    plan.dateiname_stamm      = dn.text.substr(0, dn.text.size() - endung.size() - 1); // ohne ".<endung>"
    plan.dateiname_gekuerzt   = dn.gekuerzt;
    plan.dateiname_voll_kette = dn.voll_kette;
    for (auto const& b : befunde)
        if (!b.dynamisch) plan.konstanten_meta.push_back({b.achse, b.wert});
    return plan;
}

/// Druckt den Plan literal (A9-Doc: "druckt den Ziel-Baum literal"). Deterministisch bei
/// kPlanZeitstempelPlatzhalter: zwei Laeufe ueber dieselben Quellen liefern byte-gleichen Text
/// (LED-61) -- die Gruppen-Iteration laeuft ueber std::map (binary_id-sortiert), keine
/// Hash-Reihenfolge.
inline void drucke_plan(RenderPlan const& plan, std::ostream& out) {
    out << "ziel_verzeichnis=" << plan.ziel_verzeichnis.string() << "\n";
    out << "bereits_vorhanden_skip=" << plan.bereits_vorhanden_binary_ids.size() << "\n";
    for (auto const& bid : plan.bereits_vorhanden_binary_ids) out << "  SKIP binary_id=" << bid << "\n";
    if (plan.schreibt_nichts) {
        out << "schreibt_nichts=ja (alle Binaries dieser Quelle sind bereits im Manifest -- exakt gleiche "
               "Binary, kein Neu-Schreiben)\n";
    } else {
        out << "dateiname=" << plan.dateiname_stamm << (plan.dateiname_gekuerzt ? " (gekuerzt)" : "") << "\n";
        if (plan.dateiname_gekuerzt) out << "  volle_kette=" << plan.dateiname_voll_kette << "\n";
    }
    out << "gelesene_zeilen=" << plan.gelesene_zeilen << "\n";
    out << "ohne_binary_id_uebersprungen=" << plan.ohne_binary_id_uebersprungen << "\n";
    out << "stale_uebersprungen=" << plan.stale_uebersprungen << "\n";
    out << "sheets=" << plan.sheets.size() << "\n";
    for (auto const& s : plan.sheets)
        out << "  S organ_unter=" << s.organ_unter << " zeilen=" << s.zeilen_anzahl << "\n";
    out << "konstanten_meta=" << plan.konstanten_meta.size() << "\n";
    for (auto const& k : plan.konstanten_meta) out << "  M " << k.achse << "=" << k.wert << "\n";
}

/// Schreibt die Mappe tatsaechlich -- dieselbe Gruppierung/Skip-Filterung wie berechne_plan (beide
/// laufen ueber gruppiere_quellen() -> filtere_bereits_vorhandene() -> bewerte_dynamik(), damit plan
/// und render NIE auseinanderlaufen koennen). Binaries, die das Manifest schon fuehrt, werden NICHT
/// neu geschrieben (SKIP, vierter Owner-Nachtrag Punkt 2); sind ALLE Binaries dieser Quelle bereits
/// erfasst, wird NICHTS angelegt (kein leeres Verzeichnis, keine leere Mappe -- lazy). Nach
/// erfolgreichem schliessen() werden genau die NEU geschriebenen binary_ids additiv ins Manifest
/// aufgenommen (nur im xlsx-Pfad -- s. skip_manifest.hpp Kopf-Kommentar).
inline void fuehre_render_aus(std::vector<std::filesystem::path> const& quellen,
                              std::filesystem::path const& ziel_verzeichnis, DatumZeit const& dz,
                              lab::ErgebnisFormat format) {
    GruppierteQuelle const   q    = gruppiere_quellen(quellen);
    SkipFilterErgebnis const skip = filtere_bereits_vorhandene(q.gruppen, ziel_verzeichnis, format);
    if (skip.neu.empty()) return; // alle Binaries schon im Manifest -- nichts zu tun, nichts anzulegen

    auto const              befunde = bewerte_dynamik(q.header, skip.neu);
    std::string_view const  endung  = (format == lab::ErgebnisFormat::xlsx) ? "xlsx" : "csv";
    DateinameErgebnis const dn      = baue_dateiname(dz, befunde, endung);
    if (!dn.ok) throw MessReportFehler("Dateiname verletzt die Wache (fehlerklasse=" + dn.fehler_text + ")");
    std::string const stamm = dn.text.substr(0, dn.text.size() - endung.size() - 1);

    // LAZY-Erstellung (Owner-Direktive, vierter Nachtrag Punkt 3: "in der Realitaet werden Ordner
    // und Daten erst lazy erstellt, wenn es darauf ankommt"): genau HIER, unmittelbar vor dem
    // ersten tatsaechlichen Schreiben, nicht frueher (das Skip-Gate oben kann bis hierher komplett
    // ohne jedes Anlegen durchlaufen -- s. skip.neu.empty()-Rueckkehr). Der xlsx-/CSV-Backend
    // erwartet ein bestehendes Zielverzeichnis (workbook_close scheitert sonst mit ENOENT); in der
    // Produktionskette uebernimmt das LagerBaumWriter::einlagern(), das dieses CLI (noch) nicht
    // anspricht (s. Kopf-Kommentar skip_manifest.hpp) -- also traegt der Aufrufer hier dieselbe
    // Pflicht selbst.
    std::filesystem::create_directories(ziel_verzeichnis);

    // NIE STILL UEBERSCHREIBEN (Messdaten-nie-loeschen-Doktrin, verschaerft durch den Owner:
    // "es wird weder etwas als stale XLSX behalten, noch valide Messdaten geloescht"): trifft der
    // berechnete Dateiname auf eine BEREITS VORHANDENE Datei, ist das eine Kollision (gleicher
    // Zeitstempel + gleiche dynamische kv-Kette aus zwei GETRENNTEN render-Aufrufen) -- kein
    // stilles workbook_close-Ueberschreiben, sondern ein lauter Abbruch. Nur der xlsx-Pfad ist
    // hier bewacht (die reale Ausgabe, Owner-Direktive); der CSV-Testpfad bleibt unveraendert.
    if (format == lab::ErgebnisFormat::xlsx) {
        std::filesystem::path const ziel_datei = ziel_verzeichnis / (stamm + ".xlsx");
        if (std::filesystem::exists(ziel_datei))
            throw MessReportFehler("Ziel-Datei '" + ziel_datei.string() +
                                   "' existiert bereits -- Messdaten werden NIE still ueberschrieben "
                                   "(Owner-Direktive). Kollision aus gleichem Zeitstempel + gleicher "
                                   "dynamischer kv-Kette zweier getrennter render-Aufrufe -- erneut "
                                   "ausfuehren (andere Wall-Clock-Sekunde) oder die Quellen in EINEM "
                                   "Aufruf zusammen rendern.");
    }

    auto mappe = lab::ErgebnisMappenFactory::oeffne(ziel_verzeichnis, stamm, format);

    lab::KonstantenMeta konstanten_meta;
    for (auto const& b : befunde)
        if (!b.dynamisch) konstanten_meta.push_back({b.achse, b.wert});
    mappe->info_blatt(lab::MaschinenSysinfo{}, lab::HauptAchsenBelegung{}, konstanten_meta);

    std::vector<std::string> neu_geschrieben;
    neu_geschrieben.reserve(skip.neu.size());
    for (auto const& [bid, grp] : skip.neu) {
        lab::IErgebnisBlatt& blatt = mappe->blatt(lab::SheetSchluessel{"", "", bid});
        blatt.kopf(std::span<std::string const>{q.header});
        for (auto const& zeile : grp.zeilen) blatt.zeile(std::span<std::string const>{zeile});
        neu_geschrieben.push_back(bid);
    }
    mappe->schliessen(); // wirft ErgebnisSchreibFehler bei jedem Fehl-Ausgang -- das Manifest wird
                         // erst DANACH ergaenzt, ein Fehl-Ausgang darf keine Binary als "erledigt" fuehren.

    if (format == lab::ErgebnisFormat::xlsx) ergaenze_manifest(ziel_verzeichnis, neu_geschrieben);
}

} // namespace comdare::cache_engine::tools::mess_report
