#pragma once
// profile_facade/planner/progress_cursor_reader.hpp -- W5 (2026-08-05, Owner-R5): der ERSTE und EINZIGE
// Leser der progress.cursor-Datei.
//
// WARUM ES DEN LESER BISHER NICHT GAB: die Welle-5-Naht (E-W5-2, Section 38-Fortschritts-Rueck-Kanal) hat den
// Schreiber gebaut -- der Treiber emittiert je bereitgestellte/gemessene Binary GENAU EIN ProgressDelta und am
// Fensterende GENAU EIN done -- und additiv nach <output_dir>/progress.cursor geschrieben. Einen KONSUMENTEN
// hatte diese Datei nie (Vollgrep-Befund). Damit war der Rueck-Kanal zwar vorhanden, aber stumm: niemand
// konnte sagen, wie weit ein laufendes Fenster ist. W5 schliesst genau diese Luecke auf der LESE-Seite.
//
// DEKLARIERTE CROSS-REPO-FORMAT-KOPPLUNG (das Risiko dieser Datei, hier benannt statt verschwiegen):
// Der SCHREIBER liegt im super-Repo (Code/02_messung_driver/main.cpp, der ProgressSinkFn-Lambda), der LESER
// hier in der ce-Welt. Beide Seiten teilen heute KEINE gemeinsame Quelle -- die Verbindung ist die
// Zeilen-Form. Die beiden Formen sind darum unten als benannte Konstanten festgenagelt UND in der TU
// (test_w5_status_reader) literal gepinnt, sodass eine Drift auf einer Seite rot wird statt still zu leeren.
// Die ECHTE Single-Source ist erst mit der #35-.so-Schnittstelle erreichbar (Nach-Abgabe-Posten).
//
// Die beiden Formen, VERBATIM aus dem Schreiber:
//   "[progress] perm=<N> axes_changed=<K> <achsen-idx>-><varianten-idx> ..."
//   "[progress] done perm=<N> window-complete"
// <K> ist eine ANKUENDIGUNG: es folgen GENAU K Paare, jedes eingeleitet von einem Leerzeichen. Der Leser
// prueft die perm-Form darum bis zum Zeilenende gegen diese Grammatik -- eine Zeile, die weniger (oder mehr)
// Paare traegt als sie ankuendigt, ist ein Schreib-ABBRUCH und kein Fortschritt, so plausibel ihr Anfang auch
// aussieht. Nur die VOLLSTAENDIGE Form darf den Cursor bewegen.
// Das done kommt GENAU EINMAL JE FENSTER, an dessen Ende. Es ist damit das kuenftige FERTIG-SIGNAL der
// 38.b-Sequenz ("naechste CEB erst nach Abschluss der vorigen") -- CursorStand::done_gesehen ist der
// Andockpunkt, an dem die Planer-Takt-Hoheit (F6-Zielbild) spaeter einhaengt, OHNE dass dieser Leser sich
// aendern muss.
//
// DIE DATEI IST ADDITIV UND UEBERLEBT DAS FENSTER: laeuft an derselben Perm ein FOLGE-Fenster, schreibt es
// seine perm-Zeilen HINTER das done des vorigen. `done_gesehen` ist darum eine Aussage ueber die ZULETZT
// gelesene Form, nicht ueber die Datei als Ganzes -- jede Nicht-done-Zeile nimmt es zurueck. Waere es sticky,
// meldete der Bericht "fertig", waehrend das Folge-Fenster noch misst.
//
// DER CURSOR HAT EINEN "NOCH GAR NICHT"-ZUSTAND, UND DER IST NICHT DIE 0: eine Datei, die NUR abgerissene
// oder fremde Zeilen traegt, hat NIE einen Cursor gemeldet. `letzte_perm` stuende dann auf seinem
// Default 0 -- und 0 ist ein VOLLKOMMEN GUELTIGER Cursor-Wert (die erste Perm eines Fensters). Der Bericht
// koennte beides nicht unterscheiden und meldete "steht bei Perm 0", wo "hat sich nie gemeldet" richtig
// waere. `letzte_perm_belegt` traegt genau diese Unterscheidung; der Renderer setzt daraufhin den Sentinel.
//
// ANSPRUCHSLOS: header-only C++23, nur stdlib, ASCII. Kein Wurf (Datei-/Formfehler sind BERICHTS-Inhalte,
// keine Ausnahmen) -- eine fehlende progress.cursor ist ein normaler Zustand, kein Fehler.

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

namespace comdare::cache_engine::planner {

/// Dateiname des Fortschritts-Cursors, relativ zum output_dir einer Perm (Schreiber-Konstante).
inline constexpr char kProgressCursorDateiname[] = "progress.cursor";

/// Die beiden Zeilen-Formen des Schreibers (super Code/02_messung_driver/main.cpp). Die done-Form wird ZUERST
/// geprueft, weil sie ein echtes Praefix der perm-Form waere, wenn man nur auf "[progress] " testete.
inline constexpr char kProgressDonePraefix[]    = "[progress] done perm=";
inline constexpr char kProgressDoneSuffix[]     = " window-complete";
inline constexpr char kProgressPermPraefix[]    = "[progress] perm=";
inline constexpr char kProgressAxesSchluessel[] = " axes_changed=";
/// Die ACHSEN-LISTE der perm-Form, ebenfalls VERBATIM aus dem Schreiber:
///   for (auto const& c : d.changed) line << " " << c.axis_index << "->" << c.variant_index;
/// Also GENAU K Paare, jedes eingeleitet von EINEM Leerzeichen, die beiden Indizes durch "->" getrennt.
/// Die angekuendigte Zahl K und die tatsaechliche Zahl der Paare sind damit dieselbe Aussage -- der Leser
/// darf sie nicht getrennt glauben (siehe pruefe_achsen_paare).
inline constexpr char kProgressPaarTrenner[] = " ";
inline constexpr char kProgressPaarPfeil[]   = "->";

/// Der gelesene Stand EINES Fensters.
struct CursorStand {
    bool          datei_vorhanden    = false; ///< false = es gibt keine progress.cursor (ehrlich "keine Daten")
    bool          done_gesehen       = false; ///< true = die ZULETZT gelesene Form war das Fertig-Signal
    bool          letzte_perm_belegt = false; ///< true = mindestens EINE vollstaendige Zeile setzte den Cursor
    std::uint64_t letzte_perm        = 0;     ///< der zuletzt gemeldete fenster-relative Perm-Cursor
    std::uint64_t done_perm          = 0;     ///< der Cursor der done-Zeile (nur gueltig bei done_gesehen)
    std::uint64_t zeilen_gesamt      = 0;     ///< alle Zeilen der Datei
    std::uint64_t zeilen_perm        = 0;     ///< davon vollstaendige perm-Fortschritts-Zeilen
    std::uint64_t zeilen_done        = 0;     ///< davon done-Zeilen (Vertrag: eine je Fenster, mehrere je Datei)
    std::uint64_t zeilen_fremd       = 0;     ///< davon keiner der beiden Formen zuordenbar (Drift-Anzeiger)
    std::uint64_t zeilen_abgebrochen = 0;     ///< davon MITTEN in der perm-Form abgerissen (fehlendes/unvoll-
                                              ///< staendiges <K>, oder <K> ohne die angekuendigten K Achsen-Paare)
};

namespace progress_cursor_detail {

/// Das Fertig-Signal des VORIGEN Fensters zuruecknehmen.
///
/// Die progress.cursor ist ADDITIV und wird von FOLGE-Fenstern derselben Perm weitergeschrieben. `done`
/// gilt darum nur bis zur naechsten Nicht-done-Zeile: steht hinter dem done wieder eine perm-Zeile, laeuft
/// bereits das naechste Fenster. Ein sticky done_gesehen gaebe dem kuenftigen 38.b-Takt ("naechste CEB erst
/// nach Abschluss der vorigen") ein Fertig-Signal, das nicht mehr gilt -- er zoege die Folge-CEB los, waehrend
/// die laufende noch misst. done_perm faellt mit zurueck, weil es laut Vertrag NUR bei done_gesehen gilt.
inline void fenster_laeuft_wieder(CursorStand& stand) noexcept {
    stand.done_gesehen = false;
    stand.done_perm    = 0;
}

/// Eine vorzeichenlose Dezimalzahl ab `pos` lesen. Liefert false, wenn dort keine Ziffer steht.
[[nodiscard]] inline bool lies_u64(std::string_view s, std::size_t pos, std::uint64_t& out, std::size_t& ende) {
    std::size_t e = pos;
    while (e < s.size() && s[e] >= '0' && s[e] <= '9') ++e;
    if (e == pos) return false;
    std::uint64_t v      = 0;
    auto const [ptr, ec] = std::from_chars(s.data() + pos, s.data() + e, v, 10);
    if (ec != std::errc{} || ptr != s.data() + e) return false;
    out  = v;
    ende = e;
    return true;
}

[[nodiscard]] inline bool beginnt_mit(std::string_view s, std::string_view p) noexcept {
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

/// Die ACHSEN-LISTE hinter <K> VOLLSTAENDIG gegen die Schreiber-Grammatik pruefen.
///
/// WARUM VOLLSTAENDIG UND NICHT NUR DAS ERSTE ZEICHEN: <K> ist keine Verzierung, sondern eine ANKUENDIGUNG --
/// der Schreiber sagt "es folgen K Paare" und schreibt sie im selben Rutsch. Wer nur prueft, ob hinter <K>
/// ein Trenn-Leerzeichen ODER das Zeilenende steht, glaubt die Ankuendigung, ohne die Einloesung zu lesen.
/// Genau dort liegen die Abbruch-Fragmente, die der Schreiber bei SIGKILL/Platte-voll hinterlaesst -- er
/// baut die Zeile links nach rechts, also reisst sie IMMER hinter einer schon geschriebenen Ankuendigung ab:
///   "axes_changed=1"          -- K angekuendigt, KEIN Paar geschrieben
///   "axes_changed=2 3->1"     -- 2 angekuendigt, 1 geschrieben
///   "axes_changed=1 3->"      -- mitten IM Paar abgerissen
/// Alle drei haben eine gueltige Zahl an der richtigen Stelle und ein gueltiges Ende-Zeichen dahinter; die
/// Praefix-Pruefung liess sie als vollen Fortschritt durch und zog letzte_perm auf einen nie erreichten
/// Cursor -- dieselbe Falle wie beim leeren und beim halb geschriebenen <K>, nur ein Feld weiter rechts.
/// Ebenso faellt der umgekehrte Fall ("axes_changed=1 3->1 5->2", MEHR Paare als angekuendigt): auch das ist
/// keine Form, die dieser Schreiber je erzeugt, also keine Zeile, deren Cursor man glauben darf.
///
/// `rest` beginnt DIREKT hinter dem <K>-Wert. Erwartet werden genau `k` Paare " <zahl>-><zahl>", danach
/// Zeilenende (die einzige Form, die der Schreiber erzeugt) oder reiner Rest-Whitespace (Zugestaendnis an
/// Zeilenenden-Kosmetik, das keine Aussage veraendert). JEDE andere Abweichung ist false.
[[nodiscard]] inline bool pruefe_achsen_paare(std::string_view rest, std::uint64_t k) {
    for (std::uint64_t i = 0; i < k; ++i) {
        if (!beginnt_mit(rest, kProgressPaarTrenner)) return false; // Paar i fehlt (Ankuendigung nicht eingeloest)
        rest.remove_prefix(std::string_view{kProgressPaarTrenner}.size());
        std::uint64_t achse = 0;
        std::size_t   ende  = 0;
        if (!lies_u64(rest, 0, achse, ende)) return false; // Achsen-Index fehlt oder ist nicht numerisch
        rest.remove_prefix(ende);
        if (!beginnt_mit(rest, kProgressPaarPfeil)) return false; // der Pfeil des Paares fehlt
        rest.remove_prefix(std::string_view{kProgressPaarPfeil}.size());
        std::uint64_t variante = 0;
        std::size_t   v_ende   = 0;
        if (!lies_u64(rest, 0, variante, v_ende)) return false; // Varianten-Index fehlt (Abriss IM Paar)
        rest.remove_prefix(v_ende);
    }
    // Hinter dem letzten angekuendigten Paar darf nichts Bedeutungstragendes mehr stehen -- ein weiteres Paar
    // waere MEHR als angekuendigt, alles andere fremder Anhang. Beides ist keine Schreiber-Form.
    return rest.find_first_not_of(" \t") == std::string_view::npos;
}

} // namespace progress_cursor_detail

/// EINE Zeile klassifizieren und in den Stand einrechnen. Oeffentlich, weil die TU damit die Schreiber-Formen
/// literal pinnen kann, ohne eine Datei anlegen zu muessen.
inline void verarbeite_cursor_zeile(std::string_view zeile, CursorStand& stand) {
    namespace d = progress_cursor_detail;
    while (!zeile.empty() && (zeile.back() == '\r' || zeile.back() == '\n')) zeile.remove_suffix(1);
    if (zeile.empty()) return;
    ++stand.zeilen_gesamt;

    if (d::beginnt_mit(zeile, kProgressDonePraefix)) {
        std::uint64_t v = 0;
        std::size_t   e = 0;
        if (d::lies_u64(zeile, std::string_view{kProgressDonePraefix}.size(), v, e) &&
            zeile.substr(e) == std::string_view{kProgressDoneSuffix}) {
            ++stand.zeilen_done;
            stand.done_gesehen       = true;
            stand.done_perm          = v;
            stand.letzte_perm        = v;
            stand.letzte_perm_belegt = true;
            return;
        }
        ++stand.zeilen_fremd; // done-Praefix, aber nicht die vollstaendige Form -> Drift, ehrlich zaehlen
        d::fenster_laeuft_wieder(stand);
        return;
    }

    // Ab hier ist die Zeile KEINE gueltige done-Zeile -> das Fertig-Signal des vorigen Fensters gilt nicht mehr.
    d::fenster_laeuft_wieder(stand);

    if (d::beginnt_mit(zeile, kProgressPermPraefix)) {
        std::uint64_t v = 0;
        std::size_t   e = 0;
        if (!d::lies_u64(zeile, std::string_view{kProgressPermPraefix}.size(), v, e)) {
            ++stand.zeilen_fremd; // perm=<N> traegt keine Zahl -> keine Fortschritts-Aussage
            return;
        }
        std::string_view const rest = zeile.substr(e);
        if (!d::beginnt_mit(rest, kProgressAxesSchluessel)) {
            ++stand.zeilen_fremd; // der Schluessel fehlt ganz -> fremde Form, nicht bloss abgerissen
            return;
        }
        // Der axes_changed-WERT muss da sein UND numerisch. Der Schreiber baut die Zeile in EINEM
        // Rutsch, aber ein Abbruch (SIGKILL/Platte voll) hinterlaesst das Fragment
        // "[progress] perm=7 axes_changed=" -- ein halb geschriebener Fortschritt ist KEIN Fortschritt.
        // Er wandert darum in einen EIGENEN Zaehler statt still als volle perm-Zeile durchzugehen
        // (das haette letzte_perm auf einen nie erreichten Cursor gezogen).
        std::uint64_t k     = 0;
        std::size_t   k_end = 0;
        if (!d::lies_u64(rest, std::string_view{kProgressAxesSchluessel}.size(), k, k_end)) {
            ++stand.zeilen_abgebrochen;
            return;
        }
        // ... UND die Zeile muss die mit <K> ANGEKUENDIGTEN K Achsen-Paare auch WIRKLICH tragen, in der
        // Schreiber-Form und ohne Anhang. <K> ist eine Ankuendigung; sie ungeprueft zu glauben, hiesse den
        // Abbruch-Fragmenten "axes_changed=1" (kein Paar), "axes_changed=2 3->1" (zu wenige) und
        // "axes_changed=1x" (Muell am Wert) denselben Cursor abzunehmen wie einer vollstaendigen Zeile --
        // sie zoegen letzte_perm auf eine Perm, die nie erreicht wurde. Die Grammatik-Pruefung deckt beide
        // Enden mit ab (K==0 -> Zeilenende, K>0 -> Trenn-Leerzeichen), es braucht daneben keine zweite
        // Rest-Regel. JEDE Abweichung ist derselbe Befund: eine ABGEBROCHENE Zeile, kein Fortschritt.
        if (!d::pruefe_achsen_paare(rest.substr(k_end), k)) {
            ++stand.zeilen_abgebrochen;
            return;
        }
        ++stand.zeilen_perm;
        stand.letzte_perm        = v; // additive Datei -> die letzte VOLLSTAENDIGE Zeile traegt den aktuellen Cursor
        stand.letzte_perm_belegt = true;
        return;
    }
    ++stand.zeilen_fremd;
}

/// Den Fortschritts-Cursor eines Perm-Verzeichnisses lesen. Fehlt die Datei, ist das KEIN Fehler --
/// datei_vorhanden bleibt false und der Bericht sagt ehrlich "keine Daten".
[[nodiscard]] inline CursorStand read_progress_cursor(std::filesystem::path const& perm_dir) {
    CursorStand     stand{};
    std::error_code ec;
    auto const      pfad = perm_dir / kProgressCursorDateiname;
    if (!std::filesystem::exists(pfad, ec) || ec) return stand;
    std::ifstream in{pfad, std::ios::binary};
    if (!in) return stand;
    stand.datei_vorhanden = true;
    std::string zeile;
    while (std::getline(in, zeile)) verarbeite_cursor_zeile(zeile, stand);
    return stand;
}

} // namespace comdare::cache_engine::planner
