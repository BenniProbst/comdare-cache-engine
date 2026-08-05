#pragma once
// profile_facade/planner/planner_status_reader.hpp -- W5-KERN (2026-08-05, Owner-R5): der ON-DEMAND-
// RUECK-LESER des Planers. Er berichtet dem Anwender den Stand von CEB-Bauten, Tier-Binaries und Messwerten.
//
// WAS ER IST: ein LESER. Er baut nichts, misst nichts, reserviert nichts, steuert nichts. Er beantwortet die
// E-04-Ur-Frage ("wie viele Rekombinationen sind noch offen") aus dem, was auf der Platte und im Lager
// TATSAECHLICH steht -- und sagt ehrlich "keine Daten", wo nichts steht. Kein watch, kein Daemon
// (Owner-R5: on-demand); jeder Aufruf ist ein Schnappschuss.
//
// VIER QUELLEN (Owner-R5: das Bestandslog-XML ist die AGGREGAT-Quelle, der Rest ist Datei-Detail):
//   (1) progress.cursor    -- der fenster-relative Perm-Cursor je Perm (progress_cursor_reader.hpp)
//   (2) result.csv / .stamp / .stale -- der per-Binary-Mess-Resume-Stand
//   (3) Bestandslog-XML    -- doc_revision, Bestands-Eintraege, Reservierungen (parse_bestandslog)
//   (4) .fingerprint-Sidecars -- die gebauten Binaries (read_fingerprint_sidecar, DIE EINE Lese-Wahrheit)
//
// KEIN FORMAT-DUPLIKAT: das Wissen ueber die Mess-Datei-Form (CSV-Kopf, Stempel-Schwanz-Schluessel) wird
// NICHT nachgebaut, sondern als MessFormatFakten von der Fassade hereingereicht, die es aus der Iterator-
// Substanz zieht (lazy_csv_header + kLazyResumeRowsKey). Der Sidecar wird ueber read_fingerprint_sidecar
// gelesen -- dieselbe Funktion, die auch Skip-Gate und Lager-Binder benutzen (A2-Eichung GATE 5). Das
// Bestandslog wird ueber parse_bestandslog gelesen. Dieser Header erfindet KEINE dieser Formen neu.
//
// WAS ER BEWUSST NICHT PRUEFT: den Config-PRAEFIX des Resume-Stempels. Der Status-Leser kennt die Lauf-
// Konfiguration nicht (er ist nicht der Lauf) und erfindet sie auch nicht. Er berichtet, was er belegen
// kann: CSV da, Stempel da, Kopf-Identitaet gegen die EINE Schema-Wahrheit, Zeilenzahl == Stempel-Zahl.
// Alles andere heisst ehrlich `teilweise` mit Grund -- nie `gemessen`.
//
// W5 IST DIE VORSTUFE DER PLANER-TAKT-HOHEIT (F6-Zielbild). Die Schichtung ist genau dafuer geschnitten:
//   Leser-je-Quelle  ->  Aggregator (erhebe_status)  ->  Renderer (render_status)
// Der kuenftige Takt ersetzt NUR den Renderer durch eine Steuer-Schleife und konsumiert DIESELBEN Leser:
//   * CursorStand::done_gesehen ist das 38.b-Fertig-Signal ("naechste CEB erst nach Abschluss der vorigen"),
//   * der Aggregat-Schluessel (ceb, zelle, fenster) ist die Schluessel-Welt, auf der ein Takt-Scheduler
//     Zellen zuteilt -- es gibt bewusst KEIN zweites Keying,
//   * bestand_sicht_aus_xml ist eine EIGENE Funktion, weil die Reservierungs-Sicht der Ressourcen-
//     Freigabe-Input des Takts ist; der Takt haengt sich an sie, nicht an den Renderer.
// resource_group bleibt der Ist-Traeger der Sequenzierung (Owner-R6) -- status STEUERT NICHTS.
//
// AUSGABE-GESETZ (Marker-v2, wie die Testat-Marker der Kette): der Aggregat-Schluessel ist ein TUPEL
// (ceb, zelle, fenster), NIE die Zeilen-Reihenfolge; die Layer bleiben getrennt (ceb= ist ein EIGENES Feld,
// wird NIE in zelle= verschmolzen); ein Pflichtfeld entfaellt nie, unbelegt = Sentinel "unbelegt".
//
// BILANZ-GESETZ (die beiden Wege, auf denen eine Fortschritts-Zahl luegen kann -- beide hier verriegelt):
//   (1) FENSTER-TREUE: das gepinnte Fenster ist [start, start+count) ueber die Perm-Indizes. Was ausserhalb
//       liegt, gehoert einem ANDEREN Fenster und geht NIE in diese Bilanz ein -- es bekommt seine eigene
//       [status-fremdfenster]-Zeile (perm_im_fenster / ZellStand::im_fenster).
//   (2) SUMMEN-TREUE: die Gesamt-Bilanz ist die SUMME der Zell-Offenstaende, nie ein einzelnes Fenster-SOLL
//       minus aller Messungen -- sonst verschwindet die Zellen-Multiplizitaet (gesamt_offen_feld).
//
// DOKTRIN: header-only C++23, ASCII, LEICHT (nur stdlib + zwei bereits leichte ce-Header). Kein Umbrella,
// kein Katalog, kein Netz -- damit bleibt der Leser TU-testbar ohne Binary und die Planer-App umbrella-frei.

#include "planner_status_types.hpp" // die FLACHEN PODs der Fassaden-Naht (std-only)
#include "progress_cursor_reader.hpp"

#include <builder/bestandslog/bestandslog_document.hpp>       // parse_bestandslog + BestandslogDocument (Quelle 3)
#include <builder/build_orchestrator/fingerprint_sidecar.hpp> // read_fingerprint_sidecar (Quelle 4, EINE Wahrheit)

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace comdare::cache_engine::planner {

// ---------------------------------------------------------------------------------------------------------------
// IST -- was auf der Platte steht
// ---------------------------------------------------------------------------------------------------------------
struct BinaerStand {
    std::size_t gebaut            = 0; ///< gueltige .fingerprint-Sidecars (128 hex) -- die gebauten Binaries
    std::size_t sidecar_ungueltig = 0; ///< Sidecar da, Inhalt nicht gueltig -> sichtbar statt verschwunden
    std::size_t csv_gesehen       = 0; ///< Verzeichnisse mit result.csv
    std::size_t gemessen          = 0; ///< davon vollstaendig: Stempel + Kopf-Identitaet + Zeilen == rows
    std::size_t teilweise         = 0; ///< davon unvollstaendig (Grund unten je Zaehler)
    std::size_t ohne_stempel      = 0;
    std::size_t kopf_drift        = 0;
    std::size_t zeilen_abweichung = 0;
    std::size_t stale             = 0;     ///< result.csv.stale -- gesicherter Alt-Stand ohne Resume-Anspruch
    std::size_t scan_eintraege    = 0;     ///< besuchte Verzeichnis-Eintraege (die Breiten-Kappe misst hieran)
    bool        scan_gekappt      = false; ///< true = kMaxScanEintraege erreicht -> die Bilanz ist UNVOLLSTAENDIG
};

struct ZellStand {
    PlanZelleSoll         soll{};
    std::filesystem::path perm_dir;
    bool                  perm_dir_vorhanden = false;
    bool                  im_fenster         = true; ///< liegt perm_index in [fenster_start, +count)? (H1)
    BinaerStand           bin{};
    CursorStand           cursor{};
};

struct BestandSicht {
    bool          aktiv   = false; ///< das Bestandslog-Gate ist an (COMDARE_BESTANDSLOG + Ebene B)
    bool          gelesen = false; ///< Dokument geholt UND geparst
    bool          fehler  = false; ///< der Transport WARF (Netz/Store) -- eigene Klasse, nicht "keine Daten"
    std::string   grund;           ///< wenn !gelesen: warum -- ehrlich, nie geraten
    std::uint64_t doc_revision = 0;
    std::string   genus;
    std::size_t   eintraege    = 0;
    std::size_t   res_gesamt   = 0;
    std::size_t   res_offen    = 0;
    std::size_t   res_done     = 0;
    std::size_t   res_released = 0;
    std::size_t   res_ohne_eta = 0; ///< Reservierungen ohne ETA-Feld = "noch nicht geschaetzt"
};

struct StatusBericht {
    std::string            planer_stempel;
    std::string            profil;
    std::filesystem::path  root;
    bool                   root_vorhanden  = false;
    std::string            fenster         = kMarkerUnbelegt; ///< "START:COUNT" oder Sentinel
    bool                   fenster_bekannt = false;
    std::size_t            fenster_start   = 0; ///< der START aus START:COUNT -- FILTERT, nicht nur Anzeige (H1)
    std::size_t            fenster_count   = 0;
    PlanSollSicht          soll{};
    std::vector<ZellStand> zellen;
    BestandSicht           bestand{};
};

namespace status_detail {

[[nodiscard]] inline std::string ohne_zeilenende(std::string s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    return s;
}

/// Die Zeilenzahl aus dem Stempel-Schwanz "<prefix><rows_key><N>" ziehen. Der PRAEFIX wird bewusst NICHT
/// geprueft (der Leser kennt die Lauf-Konfiguration nicht); geprueft wird die FORM des Schwanzes.
[[nodiscard]] inline bool stempel_zeilenzahl(std::string const& stempel, std::string const& rows_key,
                                             std::uint64_t& out) {
    if (rows_key.empty()) return false;
    std::size_t const pos = stempel.rfind(rows_key);
    if (pos == std::string::npos) return false;
    std::size_t const ziffern_beginn = pos + rows_key.size();
    std::string const schwanz        = ohne_zeilenende(stempel.substr(ziffern_beginn));
    if (schwanz.empty()) return false;
    std::uint64_t v      = 0;
    auto const [ptr, ec] = std::from_chars(schwanz.data(), schwanz.data() + schwanz.size(), v, 10);
    if (ec != std::errc{} || ptr != schwanz.data() + schwanz.size()) return false;
    out = v;
    return true;
}

/// Kopf-Zeile + Zahl der nicht-leeren Datenzeilen einer CSV lesen.
[[nodiscard]] inline bool lies_csv_kopf_und_zeilen(std::filesystem::path const& csv, std::string& kopf,
                                                   std::uint64_t& datenzeilen) {
    std::ifstream in{csv, std::ios::binary};
    if (!in) return false;
    std::string zeile;
    if (!std::getline(in, zeile)) return false;
    kopf        = ohne_zeilenende(std::move(zeile));
    datenzeilen = 0;
    while (std::getline(in, zeile)) {
        if (!ohne_zeilenende(zeile).empty()) ++datenzeilen;
    }
    return true;
}

} // namespace status_detail

// ---------------------------------------------------------------------------------------------------------------
// (2)+(4) Der Platten-Ist EINES Perm-Verzeichnisses: gebaute Binaries + Mess-Resume-Stand.
// ---------------------------------------------------------------------------------------------------------------
//
// Bewusst REKURSIV statt entlang eines fest verdrahteten Unterpfades: die Zwischen-Segmente des
// Emissions-Baumes (e4_xml/dll/<stem>) sind Wissen der SCHREIBER-Seite. Baute der Leser sie nach, gaebe es
// zwei Wahrheiten -- und bei einer Layout-Aenderung meldete der Bericht still "0 gemessen" statt "Layout
// unbekannt". Ein bounded rekursiver Lauf findet, was da ist, und kann nur unter- statt falsch berichten.
//
// BOUNDED HEISST ZWEI KAPPEN, NICHT EINE: kMaxScanTiefe deckelt die TIEFE, kMaxScanEintraege die BREITE.
// Mit der Tiefen-Kappe allein bliebe ein flacher Baum mit Millionen Geschwistern unbegrenzt -- die Zusage
// "bounded" waere dann nur behauptet. Greift die Breiten-Kappe, wird der Stand als UNVOLLSTAENDIG markiert
// (scan_gekappt) und der Bericht sagt es literal, statt eine zu kleine Bilanz als volle auszugeben.
[[nodiscard]] inline BinaerStand lies_binaer_stand(std::filesystem::path const& perm_dir,
                                                   MessFormatFakten const&      fakten) {
    namespace fs = std::filesystem;
    namespace d  = status_detail;
    BinaerStand     st{};
    std::error_code ec;
    if (!fs::exists(perm_dir, ec) || ec) return st;

    fs::recursive_directory_iterator it{perm_dir, fs::directory_options::skip_permission_denied, ec};
    if (ec) return st;
    fs::recursive_directory_iterator const ende{};
    for (; it != ende; it.increment(ec)) {
        if (ec) break;
        if (st.scan_eintraege >= kMaxScanEintraege) {
            st.scan_gekappt = true;
            break;
        }
        ++st.scan_eintraege;
        if (static_cast<std::size_t>(it.depth()) >= kMaxScanTiefe) {
            it.disable_recursion_pending();
            continue;
        }
        std::error_code eec;
        if (it->is_symlink(eec) || eec) {
            it.disable_recursion_pending();
            continue;
        }
        if (!it->is_regular_file(eec) || eec) continue;

        fs::path const    p    = it->path();
        std::string const name = p.filename().string();

        // (4) Sidecar: DIE EINE Lese-Wahrheit entscheidet gueltig/ungueltig -- der Leser trimmt und
        //     validiert nicht selbst (sonst zweite Wahrheit gegen das Skip-Gate).
        if (name.size() > std::string_view{kFingerprintSidecarSuffix}.size() &&
            name.compare(name.size() - std::string_view{kFingerprintSidecarSuffix}.size(),
                         std::string_view{kFingerprintSidecarSuffix}.size(), kFingerprintSidecarSuffix) == 0) {
            fs::path const binary =
                fs::path{p.string().substr(0, p.string().size() - std::string_view{kFingerprintSidecarSuffix}.size())};
            if (::comdare::cache_engine::builder::experiment::read_fingerprint_sidecar(binary))
                ++st.gebaut;
            else
                ++st.sidecar_ungueltig;
            continue;
        }

        if (name == kResultStaleName) {
            ++st.stale;
            continue;
        }
        if (name != kResultCsvName) continue;

        // (2) Mess-Resume-Stand dieses Binary-Verzeichnisses.
        ++st.csv_gesehen;
        std::string   kopf;
        std::uint64_t datenzeilen = 0;
        if (!d::lies_csv_kopf_und_zeilen(p, kopf, datenzeilen)) {
            ++st.teilweise;
            continue;
        }
        std::ifstream sf{p.parent_path() / kResultStampName, std::ios::binary};
        std::string   stempel;
        if (!sf || !std::getline(sf, stempel)) {
            ++st.teilweise;
            ++st.ohne_stempel;
            continue;
        }
        std::uint64_t erwartete_zeilen = 0;
        if (!d::stempel_zeilenzahl(stempel, fakten.rows_key, erwartete_zeilen)) {
            ++st.teilweise;
            ++st.ohne_stempel;
            continue;
        }
        if (kopf != d::ohne_zeilenende(fakten.csv_header)) {
            ++st.teilweise;
            ++st.kopf_drift;
            continue;
        }
        if (datenzeilen != erwartete_zeilen || erwartete_zeilen == 0) {
            ++st.teilweise;
            ++st.zeilen_abweichung;
            continue;
        }
        ++st.gemessen;
    }
    return st;
}

// ---------------------------------------------------------------------------------------------------------------
// (3) Bestandslog-Sicht -- EIGENE Funktion, weil sie der Ressourcen-Freigabe-Input des kuenftigen Takts ist.
// ---------------------------------------------------------------------------------------------------------------
[[nodiscard]] inline BestandSicht bestand_sicht_aus_xml(std::string_view xml) {
    namespace bl = ::comdare::cache_engine::builder::bestandslog;
    BestandSicht s{};
    s.aktiv        = true;
    auto const doc = bl::parse_bestandslog(xml);
    if (!doc) {
        s.grund = "Dokument nicht als Bestandslog lesbar (Wurzel/Syntax)";
        return s;
    }
    if (!bl::document_syntax_supported(*doc)) {
        s.grund = "syntax_version " + std::to_string(doc->syntax_version) + " liegt ueber der lesbaren Grammatik";
        return s;
    }
    s.gelesen      = true;
    s.doc_revision = doc->doc_revision;
    s.genus        = std::string{bl::to_string(doc->genus)};
    s.eintraege    = doc->bestand.size();
    s.res_gesamt   = doc->reservierungen.size();
    for (auto const& r : doc->reservierungen) {
        switch (r.status) {
            case bl::BatchStatus::offen: ++s.res_offen; break;
            case bl::BatchStatus::done: ++s.res_done; break;
            case bl::BatchStatus::released: ++s.res_released; break;
        }
        if (r.eta_s.empty()) ++s.res_ohne_eta; // ehrlich: "noch nicht geschaetzt", kein erfundener Wert
    }
    return s;
}

// ---------------------------------------------------------------------------------------------------------------
// AGGREGATOR -- SOLL x IST je (ceb, zelle, fenster)
// ---------------------------------------------------------------------------------------------------------------
/// Das Perm-Verzeichnis EINER Zelle im Emissions-Kanon: <root>/<ceb_slug>/perm<idx>.
[[nodiscard]] inline std::filesystem::path perm_verzeichnis(std::filesystem::path const& root, PlanZelleSoll const& z) {
    return root / z.ceb_slug / ("perm" + std::to_string(z.perm_index));
}

/// Gehoert diese Perm in das gepinnte Fenster [start, start+count)?
///
/// H1: der START aus COMDARE_GOLDEN_N_RANGE ist ein FILTER, kein Anzeige-Schmuck. Ein Mess-Baum haelt die
/// Perm-Verzeichnisse ALLER bisher gelaufenen Chunks nebeneinander; erhoebe der Leser sie alle in EINE
/// Bilanz, zaehlten Fremd-Fenster-Ergebnisse als Fortschritt des aktuellen Fensters -- und `offen` fiele
/// faelschlich gegen 0, genau wenn noch am meisten zu tun ist. Ohne gepinntes Fenster gehoert alles dazu.
/// Gerechnet wird per Subtraktion statt per Addition: start+count kann bei grossem start ueberlaufen.
[[nodiscard]] inline bool perm_im_fenster(bool fenster_bekannt, std::size_t fenster_start, std::size_t fenster_count,
                                          std::size_t perm_index) noexcept {
    if (!fenster_bekannt) return true;
    return perm_index >= fenster_start && (perm_index - fenster_start) < fenster_count;
}

/// Den Ist je geplanter Zelle erheben. Fehlt ein Perm-Verzeichnis, ist das KEIN Fehler -- es heisst
/// "hier ist noch nichts gelaufen" und wird als solches berichtet.
inline void erhebe_zellen(StatusBericht& bericht, MessFormatFakten const& fakten) {
    namespace fs = std::filesystem;
    std::error_code ec;
    bericht.root_vorhanden = fs::exists(bericht.root, ec) && !ec;
    bericht.zellen.clear();
    bericht.zellen.reserve(bericht.soll.zellen.size());
    for (auto const& z : bericht.soll.zellen) {
        ZellStand st{};
        st.soll     = z;
        st.perm_dir = perm_verzeichnis(bericht.root, z);
        st.im_fenster =
            perm_im_fenster(bericht.fenster_bekannt, bericht.fenster_start, bericht.fenster_count, z.perm_index);
        std::error_code eec;
        st.perm_dir_vorhanden = fs::exists(st.perm_dir, eec) && !eec;
        if (st.perm_dir_vorhanden) {
            st.bin    = lies_binaer_stand(st.perm_dir, fakten);
            st.cursor = read_progress_cursor(st.perm_dir);
        }
        bericht.zellen.push_back(std::move(st));
    }
    // Deterministische Ordnung nach dem TUPEL-Schluessel (ceb, zelle, perm) -- nie nach Erhebungs-Reihenfolge.
    std::sort(bericht.zellen.begin(), bericht.zellen.end(), [](ZellStand const& a, ZellStand const& b) {
        if (a.soll.ceb != b.soll.ceb) return a.soll.ceb < b.soll.ceb;
        if (a.soll.zelle != b.soll.zelle) return a.soll.zelle < b.soll.zelle;
        return a.soll.perm_index < b.soll.perm_index;
    });
}

/// offen EINER Zelle = ihr Fenster-SOLL minus ihre eigenen Messungen. Ohne gepinntes Fenster gibt es kein
/// ehrliches SOLL auf Binary-Ebene -> Sentinel statt Zahl (die Plan-Schritt-Zahl ist eine ANDERE Groesse und
/// wird nie dafuer eingesetzt).
[[nodiscard]] inline std::size_t zell_offen(StatusBericht const& b, std::size_t gemessen) noexcept {
    return b.fenster_count > gemessen ? b.fenster_count - gemessen : std::size_t{0};
}

[[nodiscard]] inline std::string offen_feld(StatusBericht const& b, std::size_t gemessen) {
    if (!b.fenster_bekannt) return kMarkerUnbelegt;
    return std::to_string(zell_offen(b, gemessen));
}

/// H2: die GESAMT-Bilanz ist die SUMME der Zell-Offenstaende, NIE "ein Fenster-count minus alle Messungen".
/// Das Fenster-SOLL gilt JE ZELLE (jede Zelle misst ihre eigenen count Binaries); zieht man alle Messungen
/// von EINEM count ab, verschwindet genau die Zellen-Multiplizitaet, nach der die E-04-Ur-Frage fragt:
/// bei 2 Zellen x 16 Binaries und je 1 Messung meldete der Bericht "offen=14" statt "offen=30" -- er
/// unterschlaegt eine ganze Zelle und wird umso falscher, je mehr Zellen laufen. Fremd-Fenster-Zellen
/// gehen NICHT ein (H1); sie erscheinen als eigene [status-fremdfenster]-Zeile.
[[nodiscard]] inline std::string gesamt_offen_feld(StatusBericht const& b) {
    if (!b.fenster_bekannt) return kMarkerUnbelegt;
    std::size_t summe = 0;
    for (auto const& z : b.zellen) {
        if (!z.im_fenster) continue;
        summe += zell_offen(b, z.bin.gemessen);
    }
    return std::to_string(summe);
}

// ---------------------------------------------------------------------------------------------------------------
// RENDERER -- die einzige Stelle, die formatiert. Der kuenftige Takt ersetzt GENAU DIESE Funktion.
// ---------------------------------------------------------------------------------------------------------------
inline void render_status(StatusBericht const& b, std::ostream& os) {
    auto const nz = [](std::string const& s) -> std::string { return s.empty() ? std::string{kMarkerUnbelegt} : s; };

    os << "[status] planer=" << nz(b.planer_stempel) << " profil=" << nz(b.profil) << " root=" << b.root.string()
       << " root_vorhanden=" << (b.root_vorhanden ? "ja" : "nein") << " fenster=" << nz(b.fenster)
       << " plan=" << (b.soll.erhoben ? "erhoben" : "nicht_erhoben") << " source_kind=" << nz(b.soll.source_kind)
       << " profil_id=" << nz(b.soll.profile_id) << " perms=" << b.soll.perm_count
       << " mess_combos=" << b.soll.measurement_combo_count << "\n";

    if (!b.soll.erhoben) {
        os << "[status] quelle=plan keine Daten (" << nz(b.soll.grund) << ")\n";
    } else if (b.soll.zellen.empty()) {
        os << "[status] quelle=plan keine Daten (der Walk lieferte keine Zelle)\n";
    }
    if (!b.root_vorhanden) {
        os << "[status] quelle=messbaum keine Daten (root " << b.root.string() << " existiert nicht)\n";
    }

    // ZWEI GETRENNTE SUMMEN, bewusst nicht eine: g_* ist die Bilanz DIESES Fensters (nur Zellen in
    // [start, start+count)), f_* die der Fremd-Fenster. Verschmoelzen sie, ist die Fenster-Aussage
    // unbrauchbar; verschwiegen die Fremd-Fenster, verschwaende der Bericht vorhandenes Wissen.
    std::size_t g_schritte = 0, g_gebaut = 0, g_gemessen = 0, g_stale = 0, g_teilweise = 0, g_ungueltig = 0, g_csv = 0,
                g_ohne_dir = 0, g_gekappt = 0;
    std::size_t f_zellen = 0, f_gebaut = 0, f_gemessen = 0, f_teilweise = 0, f_stale = 0, f_csv = 0;
    // Die "keine Daten"-Aussagen gelten fuer die QUELLE, nicht fuer das Fenster -- sie zaehlen ueber ALLE
    // Zellen. Sonst behauptete der Bericht "kein result.csv", waehrend welche in Fremd-Fenstern liegen.
    std::size_t alle_csv = 0, alle_ohne_cursor = 0;
    for (auto const& z : b.zellen) {
        alle_csv += z.bin.csv_gesehen;
        if (!z.cursor.datei_vorhanden) ++alle_ohne_cursor;
        if (z.im_fenster) {
            g_schritte += z.soll.plan_schritte;
            g_gebaut += z.bin.gebaut;
            g_gemessen += z.bin.gemessen;
            g_stale += z.bin.stale;
            g_teilweise += z.bin.teilweise;
            g_ungueltig += z.bin.sidecar_ungueltig;
            g_csv += z.bin.csv_gesehen;
            if (!z.perm_dir_vorhanden) ++g_ohne_dir;
            if (z.bin.scan_gekappt) ++g_gekappt;
        } else {
            ++f_zellen;
            f_gebaut += z.bin.gebaut;
            f_gemessen += z.bin.gemessen;
            f_teilweise += z.bin.teilweise;
            f_stale += z.bin.stale;
            f_csv += z.bin.csv_gesehen;
        }

        os << "[status-zelle] ceb=" << nz(z.soll.ceb) << " zelle=" << nz(z.soll.zelle) << " fenster=" << nz(b.fenster)
           << " perm=" << z.soll.perm_index << " im_fenster=" << (z.im_fenster ? "ja" : "nein")
           << " plan_schritte=" << z.soll.plan_schritte
           << " perm_dir=" << (z.perm_dir_vorhanden ? "vorhanden" : "fehlt") << " gebaut=" << z.bin.gebaut
           << " gemessen=" << z.bin.gemessen << " teilweise=" << z.bin.teilweise << " stale=" << z.bin.stale
           << " csv_gesehen=" << z.bin.csv_gesehen << " sidecar_ungueltig=" << z.bin.sidecar_ungueltig
           << " ohne_stempel=" << z.bin.ohne_stempel << " kopf_drift=" << z.bin.kopf_drift
           << " zeilen_abweichung=" << z.bin.zeilen_abweichung << " scan_gekappt="
           << (z.bin.scan_gekappt ? "ja" : "nein")
           // Eine Fremd-Fenster-Zelle hat in DIESEM Fenster kein SOLL -- der Sentinel sagt das, statt eine
           // Zahl zu erfinden, die sich auf ein anderes Fenster bezoege.
           << " offen=" << (z.im_fenster ? offen_feld(b, z.bin.gemessen) : std::string{kMarkerUnbelegt}) << "\n";

        os << "[status-cursor] ceb=" << nz(z.soll.ceb) << " zelle=" << nz(z.soll.zelle) << " fenster=" << nz(b.fenster)
           << " perm=" << z.soll.perm_index << " cursor_datei=" << (z.cursor.datei_vorhanden ? "vorhanden" : "fehlt")
           << " letzte_perm="
           << (z.cursor.datei_vorhanden ? std::to_string(z.cursor.letzte_perm) : std::string{kMarkerUnbelegt})
           << " done="
           << (!z.cursor.datei_vorhanden ? std::string{kMarkerUnbelegt}
                                         : std::string{z.cursor.done_gesehen ? "ja" : "nein"})
           << " zeilen=" << z.cursor.zeilen_gesamt << " zeilen_perm=" << z.cursor.zeilen_perm
           << " zeilen_done=" << z.cursor.zeilen_done << " zeilen_fremd=" << z.cursor.zeilen_fremd
           << " abgebrochene_zeile=" << z.cursor.zeilen_abgebrochen << "\n";
    }
    if (!b.zellen.empty() && alle_ohne_cursor == b.zellen.size())
        os << "[status] quelle=progress_cursor keine Daten (keine der " << b.zellen.size() << " Zellen hat eine "
           << kProgressCursorDateiname << ")\n";
    if (!b.zellen.empty() && alle_csv == 0)
        os << "[status] quelle=result_csv keine Daten (kein " << kResultCsvName << " unter den Perm-Verzeichnissen)\n";

    // H1: die Fremd-Fenster bekommen eine EIGENE Zeile. Sie sind weder Fortschritt dieses Fensters noch
    // Nichts -- sie sind der Ist eines ANDEREN Fensters, und genau so wird er ausgewiesen.
    if (f_zellen > 0)
        os << "[status-fremdfenster] zellen=" << f_zellen << " fenster=" << nz(b.fenster) << " gebaut=" << f_gebaut
           << " gemessen=" << f_gemessen << " teilweise=" << f_teilweise << " stale=" << f_stale
           << " csv_gesehen=" << f_csv << "\n";

    if (!b.bestand.aktiv) {
        os << "[status] quelle=bestandslog keine Daten (COMDARE_BESTANDSLOG nicht aktiv / Ebene B fehlt)\n";
    } else if (b.bestand.fehler) {
        // M4: eine Transport-AUSNAHME ist BERICHTS-Inhalt wie jede andere fehlende Quelle. Sie als Wurf
        // durchzulassen braeche die Zusage des Kommandos ("fehlende Quellen sind Berichts-Inhalt, rc 0")
        // ausgerechnet im haeufigsten Ausfall: Objekt-Store nicht erreichbar.
        os << "[status-bestand] quelle=fehler (" << nz(b.bestand.grund) << ")\n";
    } else if (!b.bestand.gelesen) {
        os << "[status] quelle=bestandslog keine Daten (" << nz(b.bestand.grund) << ")\n";
    } else {
        os << "[status-bestand] genus=" << nz(b.bestand.genus) << " doc_revision=" << b.bestand.doc_revision
           << " eintraege=" << b.bestand.eintraege << " res_gesamt=" << b.bestand.res_gesamt
           << " res_offen=" << b.bestand.res_offen << " res_done=" << b.bestand.res_done
           << " res_released=" << b.bestand.res_released << " res_ohne_eta=" << b.bestand.res_ohne_eta << "\n";
    }

    os << "[status-gesamt] zellen=" << b.zellen.size() << " fenster_zellen=" << (b.zellen.size() - f_zellen)
       << " fremdfenster=" << f_zellen << " fenster=" << nz(b.fenster) << " plan_schritte=" << g_schritte
       << " perm_dirs_fehlen=" << g_ohne_dir << " gebaut=" << g_gebaut << " gemessen=" << g_gemessen
       << " teilweise=" << g_teilweise << " stale=" << g_stale << " csv_gesehen=" << g_csv
       << " sidecar_ungueltig=" << g_ungueltig << " scan_gekappt_zellen=" << g_gekappt
       << " offen=" << gesamt_offen_feld(b) << "\n";
}

} // namespace comdare::cache_engine::planner
