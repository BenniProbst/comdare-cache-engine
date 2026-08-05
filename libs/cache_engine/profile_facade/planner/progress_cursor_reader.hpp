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
// Das done kommt GENAU EINMAL, am Fensterende. Es ist damit das kuenftige FERTIG-SIGNAL der 38.b-Sequenz
// ("naechste CEB erst nach Abschluss der vorigen") -- CursorStand::done_gesehen ist der Andockpunkt, an dem
// die Planer-Takt-Hoheit (F6-Zielbild) spaeter einhaengt, OHNE dass dieser Leser sich aendern muss.
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

/// Der gelesene Stand EINES Fensters.
struct CursorStand {
    bool          datei_vorhanden = false; ///< false = es gibt keine progress.cursor (ehrlich "keine Daten")
    bool          done_gesehen    = false; ///< true = das Fenster hat sein Fertig-Signal geschrieben
    std::uint64_t letzte_perm     = 0;     ///< der zuletzt gemeldete fenster-relative Perm-Cursor
    std::uint64_t done_perm       = 0;     ///< der Cursor der done-Zeile (nur gueltig bei done_gesehen)
    std::uint64_t zeilen_gesamt   = 0;     ///< alle Zeilen der Datei
    std::uint64_t zeilen_perm     = 0;     ///< davon perm-Fortschritts-Zeilen
    std::uint64_t zeilen_done     = 0;     ///< davon done-Zeilen (Vertrag: hoechstens 1 je Fenster)
    std::uint64_t zeilen_fremd    = 0;     ///< davon keiner der beiden Formen zuordenbar (Drift-Anzeiger)
};

namespace progress_cursor_detail {

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
            stand.done_gesehen = true;
            stand.done_perm    = v;
            stand.letzte_perm  = v;
            return;
        }
        ++stand.zeilen_fremd; // done-Praefix, aber nicht die vollstaendige Form -> Drift, ehrlich zaehlen
        return;
    }
    if (d::beginnt_mit(zeile, kProgressPermPraefix)) {
        std::uint64_t v = 0;
        std::size_t   e = 0;
        if (d::lies_u64(zeile, std::string_view{kProgressPermPraefix}.size(), v, e) &&
            d::beginnt_mit(zeile.substr(e), kProgressAxesSchluessel)) {
            ++stand.zeilen_perm;
            stand.letzte_perm = v; // additive Datei -> die letzte Zeile traegt den aktuellen Cursor
            return;
        }
        ++stand.zeilen_fremd;
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
