#pragma once
// lager_pfad_grammatik.hpp -- LB-0 (F9-Paketschnitt, KONSOLIDIERT 01.08.): DIE EINE Grammatik der
// Lager-Pfade. Single-Source fuer BEIDE Realm-Baeume (Binaries + Messdaten) UND fuer den
// A9-xlsx-Writer (Ownership-Entscheid L5: diese Datei gehoert der A1-Lager-Welle, A9 KONSUMIERT sie
// unveraendert -- kein Fork, keine Zweitquelle).
//
// WAS HIER WOHNT (und nur hier):
//   * die kv-Grammatik  unterachse=wert  mit dem Joiner '+' (A9-Doc Abschnitt 5, EBNF),
//   * die Zeichenklasse der Werte [a-z0-9._-] und der Achsen-Namen [a-z0-9_],
//   * die WACHEN: 200-Byte-Komponentenlimit, Zeichenklassen-Pruefung, explizite Sanitisierung,
//     deterministischer SHA512-hex16-Kurzname bei Ueberlauf MIT Erhalt der Voll-Kette,
//   * die Praefix-Semantik MIT Trenner (TP1-F1 auf Ordner-Ebene: "...=1" darf nie "...=16" treffen),
//   * das Blatt-Dateinamen-Schema  <datum>-<zeit>_<kvkette>.<endung>  (A9-Doc Abschnitt 5).
//
// DIE EINE ABBILDUNG (LB-0): Baum-Pfad == minio-Objekt-Praefix == Verortung des key_sha512-Blatts.
// Es gibt KEINE zweite Uebersetzung zwischen Dateisystem und Objekt-Store: der mit pfad_join()
// gebaute String IST der Objekt-Schluessel (objekt_key() haengt nur den Blatt-Namen an, mit demselben
// Trenner). Wer eine eigene Uebersetzung baut, erzeugt genau die Drift, die dieser Header schliesst.
//
// LAYER-KANTE (Doktrin wie im Kopf von artifact_cache_transport.hpp): dieser Header kennt NUR stdlib
// + ctsha512. Er inkludiert NICHTS aus experiment_tree -- die Serialisierung der ORGAN-Segmente bleibt
// in builder/experiment_tree/axis_path_serialization.hpp (serialize_axis_segment, :45-50) und die
// bindende System-Achsen-Ordnung in abi/system_axis_order.hpp. KOMPONIERT werden beide erst im
// Baum-Writer (lager_baum_writer.hpp), der die Kante bewusst und an EINER Stelle zieht.
//
// L3-ENTSCHEID -- AUFGEHOBEN DURCH K1 (Owner-Entscheid 09.08.2026). NICHT GELOESCHT, sondern datiert:
//
// BIS K1 galt (L3, Manager): "Das Lager reserviert KEIN Hybrid-Segment." Die Grammatik trug dazu einen
// Token "hybrid", ein Praedikat traegt_hybrid_token() und die Fehlerklasse hybrid_segment_verboten,
// die jeden Achsen-Namen und jeden Wert mit diesem Praefix compile- und laufzeit-hart abwies. L3 war
// ausdruecklich eine WARTEMARKE: die Lager-Identitaet der Hybrid-.so war "ein EIGENER Owner-/K1-
// Entscheid im Hybrid-B-Fenster".
//
// K1 HAT ENTSCHIEDEN (verbatim): die Hybrid-Gattung ist "einfach eine weitere Gattung+Genus, die
// parallel zu allen anderen Gattung+Genus in den beiden wurzel-Ordner-Ebenen des Lagerbaumes mit
// einsortiert wird". Damit ist die Wartemarke eingeloest und die Wache GEGENSTANDSLOS -- ein Token,
// der eine legitime Gattung abweist, waere ab jetzt selbst die Regression. Sie faellt deshalb
// ersatzlos aus DIESER Datei.
//
// ERSATZLOS heisst NICHT ungesichert: was L3 hier verbot, ordnet K1 eine Schicht hoeher. Die zwei
// Wurzelebenen (Gattung -> Genus) und ihre Wachen wohnen im Baum-Writer (lager_baum_writer.hpp),
// weil nur DER die Layer-Kante zur Anatomie ziehen darf -- siehe LAYER-KANTE oben. Eine Gattung ohne
// Lager-Token bricht dort den Bau; ein Genus, das nicht zu seiner Gattung gehoert, wird dort
// klassifiziert abgewiesen. Diese Grammatik bleibt, was sie war: Zeichen, Laengen, Form.
//
// CT/RT-GRENZE: die Regeln und Wachen sind CT (constexpr-Praedikate + static_assert-Batterie am Ende,
// inklusive der drei A9-Namens-Beispiele als consteval-Anker). Die KONKRETEN Pfade sind RT-Strings
// aus RT-Unter-Achsen-Werten (A-15: Unter-Achsen erscheinen NIE im Stempel, wohl aber im DATEINAMEN).
// Die Wahl des Realms ist eine CT-Policy (lager_baum_writer.hpp), NIE ein Laufzeit-Switch.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare, stdlib + ctsha512. Kein std::variant, kein
// Runtime-Switch, statischer Dispatch.

#include <sha512/ctsha512.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

// ---------------------------------------------------------------------------
// str_von -- die EINE Art, hier einen std::string aus einer Sicht zu bauen (08.08.2026).
// ---------------------------------------------------------------------------
/// WARUM NICHT std::string{sv}: die A9-Abnahme-Anker am Dateiende (a9_namens_beispiele_treffen /
/// a9_pfad_beispiele_treffen) sind consteval und laufen in static_assert -- die Pfad-Bildung MUSS
/// also zur Uebersetzungszeit auswertbar bleiben. Genau der string_view-Konstruktor von std::string
/// ist das nicht: am Objekt gemessen mit clang 22.1 gegen libstdc++ 13, 15.3 UND 16 bricht
///     std::string s{sv};
/// in einer constant expression mit "undefined function '_M_construct<const char *>' cannot be used
/// in a constant expression", waehrend g++ 15.3 es auswertet. Die Fehlerklasse ist eng und gemessen:
/// der const char*-Konstruktor, operator+=, append(string_view), das Wachsen ueber die SSO-Grenze und
/// der Vergleich gegen ein string_view sind ALLE auswertbar -- nur die Konstruktion aus einer Sicht
/// nicht. append() nimmt denselben Text ueber einen auswertbaren Weg auf.
///
/// ABGRENZUNG: das ist eine Anpassung an eine gemessene Bibliotheksgrenze, KEIN Umbau auf eine
/// heap-freie Bildung. Anders als bei der Mess-Gates-Grammatik (abi/mess_gates_glied.hpp, dort ist
/// die Bildung selbst auf einen Puffer fester Kapazitaet umgestellt) ist der PRODUKTIVE Weg hier die
/// Laufzeit -- variable Ebenen-Zahl, bis kMaxKomponenteBytes je Komponente, sha512-Hex. constexpr
/// traegt hier nur die beiden Abnahme-Anker. Ein Puffer fester Kapazitaet waere dort kein besserer,
/// sondern ein engerer Bau.
[[nodiscard]] constexpr std::string str_von(std::string_view sicht) {
    std::string s;
    s.append(sicht);
    return s;
}

// ---------------------------------------------------------------------------
// Die Zeichen der Grammatik. Vier Konstanten, damit kein Konsument ein Literal nachbildet.
// ---------------------------------------------------------------------------

/// Trenner INNERHALB eines kv-Paares: "unterachse=wert" (deckungsgleich mit
/// experiment::serialize_axis_segment, axis_path_serialization.hpp:45-50 -- dieselbe Konvention, damit
/// Organ-Segmente und Lager-Segmente nicht in zwei Formen auseinanderlaufen).
inline constexpr char kKvTrenner = '=';

/// Joiner ZWISCHEN kv-Paaren derselben Ebene: "a=1+b=2" (Muster der build_version-Suffixe +cxx=/+opt=).
inline constexpr char kKvJoiner = '+';

/// Trenner der Baum-EBENEN. Identisch im Dateisystem und im Objekt-Store (LB-0, s. Kopf).
inline constexpr char kEbenenTrenner = '/';

/// Trenner ZWISCHEN Achse und Wert INNERHALB eines Gruppen-Segments. Ein Gruppen-Ordner traegt bereits
/// ein '=' fuer die Gruppe selbst ("01_read_path=..."), deshalb kann die Achsen-Bindung darin nicht
/// noch einmal '=' sein: "01_read_path=search_algo-prt_art+cache_traversal-bfs" (A9-Doc 6.1).
inline constexpr char kGruppenPaarTrenner = '-';

/// Komponenten-Limit EINER Pfad-Komponente bzw. des Blatt-Dateinamens in BYTE. 200 statt der 255 von
/// ext4/NAS: die Reserve traegt Suffixe, die spaetere Schichten anhaengen (".stale", "__S001.csv",
/// ".lock"), ohne dass eine legale Komponente dadurch unschreibbar wird (A9-Doc Abschnitt 5).
inline constexpr std::size_t kMaxKomponenteBytes = 200;

/// Laenge des deterministischen Kurznamens in Hex-Zeichen (SHA512 ueber die VOLLE Kette, vorne
/// abgeschnitten). 16 Hex == 64 Bit: bei den hier realistischen Komponenten-Zahlen kollisionsfrei genug,
/// und kurz genug, dass der Kurzname zusammen mit Datum/Zeit weit unter dem Limit bleibt.
inline constexpr std::size_t kKurznameHexLen = 16;

/// Praefix des Kurznamens. "H=" bleibt in der kv-Grammatik (Achse "H", Wert = Hex) -- ein gekuerzter
/// Name ist damit fuer jeden Leser der Grammatik weiterhin ein regulaeres Segment und nicht ein
/// Fremdkoerper. Die VOLL-Kette geht nie verloren: sie reist in PfadErgebnis::voll_kette mit und
/// gehoert vom Aufrufer ins INFO-Blatt bzw. neben das Blatt (nie stilles Kuerzen).
inline constexpr std::string_view kKurznamePraefix = "H=";

// ---------------------------------------------------------------------------
// Fehlerklassen (Fehlerklassen-Pflicht: jede Achse, jede Unter-Achse, jeder Algorithmus). KEIN
// bool-Salat und keine stille Reparatur -- jeder Ausgang hat einen Namen, den ein Testat zitieren kann.
// ---------------------------------------------------------------------------
enum class LagerPfadFehler : int {
    ok = 0,
    achse_leer,           // Achsen-Name ohne Zeichen
    achse_zeichenklasse,  // Achsen-Name ausserhalb [a-z0-9_]
    wert_leer,            // Wert ohne Zeichen (ein "achse=" waere ein unlesbares Segment)
    wert_zeichenklasse,   // Wert ausserhalb [a-z0-9._-]
    gruppen_wert_trenner, // '-' im Wert eines GRUPPEN-Segments (dort ist '-' der Paar-Trenner)
    // K1 (09.08.2026): hier stand hybrid_segment_verboten (L3). Der Entscheid ist aufgehoben, die
    // Klasse ist ersatzlos entfallen -- s. Kopf. Bewusst NICHT als Platzhalter stehengelassen: eine
    // Fehlerklasse, die niemand mehr vergeben kann, ist ein Stolperstein fuer jeden spaeteren Leser.
    kette_leer,           // kv-Kette ohne ein einziges Paar
    komponente_ueberlang, // > kMaxKomponenteBytes, ohne dass ein Kurzname zulaessig waere
    datum_form,           // datum != 8 Ziffern (YYYYMMDD)
    zeit_form,            // zeit  != 6 Ziffern (HHMMSS)
    endung_leer,          // Datei-Endung ohne Zeichen
};

[[nodiscard]] constexpr std::string_view to_string(LagerPfadFehler f) noexcept {
    switch (f) {
        case LagerPfadFehler::ok: return "ok";
        case LagerPfadFehler::achse_leer: return "achse_leer";
        case LagerPfadFehler::achse_zeichenklasse: return "achse_zeichenklasse";
        case LagerPfadFehler::wert_leer: return "wert_leer";
        case LagerPfadFehler::wert_zeichenklasse: return "wert_zeichenklasse";
        case LagerPfadFehler::gruppen_wert_trenner: return "gruppen_wert_trenner";
        case LagerPfadFehler::kette_leer: return "kette_leer";
        case LagerPfadFehler::komponente_ueberlang: return "komponente_ueberlang";
        case LagerPfadFehler::datum_form: return "datum_form";
        case LagerPfadFehler::zeit_form: return "zeit_form";
        case LagerPfadFehler::endung_leer: return "endung_leer";
    }
    return "ok";
}

// ---------------------------------------------------------------------------
// Zeichenklassen (CT-Praedikate -- die Regeln selbst sind compile-time, nur ihre Anwendung auf
// RT-Achsenwerte ist Laufzeit).
// ---------------------------------------------------------------------------

/// Werte-Zeichenklasse [a-z0-9._-] (A9-Doc Abschnitt 5: shell-sicher, keine Grossbuchstaben, damit ein
/// case-insensitives Dateisystem zwei Werte nie verschmilzt).
[[nodiscard]] constexpr bool ist_wert_zeichen(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
}

/// Achsen-Namens-Zeichenklasse [a-z0-9_]. ENGER als die Werte-Klasse und mit Absicht: '.' und '-'
/// sind in Werten legal (Versionen wie 6.17.0-35), im Achsen-NAMEN waeren sie mit dem
/// Gruppen-Paar-Trenner bzw. mit Datei-Endungen verwechselbar. Ziffern sind erlaubt -- die
/// Organ-Gruppen heissen 01_read_path .. 05_write_path_io.
[[nodiscard]] constexpr bool ist_achsen_zeichen(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

[[nodiscard]] constexpr bool ist_wert_rein(std::string_view s) noexcept {
    if (s.empty()) return false;
    for (char c : s)
        if (!ist_wert_zeichen(c)) return false;
    return true;
}

[[nodiscard]] constexpr bool ist_achse_rein(std::string_view s) noexcept {
    if (s.empty()) return false;
    for (char c : s)
        if (!ist_achsen_zeichen(c)) return false;
    return true;
}

// K1 (09.08.2026): hier wohnte die L3-WACHE (kVerbotenerHybridToken + traegt_hybrid_token). Sie ist
// mit dem L3-Entscheid entfallen -- die Begruendung steht im Kopf. Wer sie in der Historie sucht:
// letzter Stand vor dem Umbau ist Commit 4ab97516.

// ---------------------------------------------------------------------------
// Ergebnis-Typ. Ein Segment-Bau liefert IMMER: den Text, die Fehlerklasse, und -- falls gekuerzt --
// die volle Kette. Kein Ausgang ist stumm.
// ---------------------------------------------------------------------------
struct PfadErgebnis {
    std::string     text;                           // das gebaute Segment bzw. der Dateiname
    LagerPfadFehler fehler   = LagerPfadFehler::ok; // die Fehlerklasse (ok == gebaut)
    bool            gekuerzt = false;               // true == text traegt den SHA512-Kurznamen
    std::string     voll_kette;                     // die UNGEKUERZTE Kette (nur wenn gekuerzt)

    [[nodiscard]] constexpr bool ok() const noexcept { return fehler == LagerPfadFehler::ok; }
};

/// Ein kv-Paar der Grammatik. Views -- der Aufrufer haelt die Strings (RT-Achsenwerte).
struct KvPaar {
    std::string_view achse;
    std::string_view wert;
};

// ---------------------------------------------------------------------------
// Pruefung EINES kv-Paares. Reihenfolge der Wachen ist bewusst: erst Anwesenheit, dann Zeichenklasse.
// So nennt die Fehlerklasse immer die ERSTE Ursache und nicht eine Folge davon.
// ---------------------------------------------------------------------------
[[nodiscard]] constexpr LagerPfadFehler pruefe_kv(KvPaar const& p) noexcept {
    if (p.achse.empty()) return LagerPfadFehler::achse_leer;
    if (!ist_achse_rein(p.achse)) return LagerPfadFehler::achse_zeichenklasse;
    if (p.wert.empty()) return LagerPfadFehler::wert_leer;
    if (!ist_wert_rein(p.wert)) return LagerPfadFehler::wert_zeichenklasse;
    return LagerPfadFehler::ok;
}

// ---------------------------------------------------------------------------
// Sanitisierung -- EXPLIZIT und laut, nie implizit.
//
// Die Bau-Funktionen unten sanitisieren NICHT von selbst: ein Wert ausserhalb der Zeichenklasse ist
// ein klassifizierter FEHLER (wert_zeichenklasse), kein Anlass zum stillen Umschreiben. Wer eine
// fremde Zeichenkette (Compiler-Version, Kernel-Release, Tool-Name) in die Grammatik heben will, ruft
// diese Funktion SICHTBAR und weiss danach, dass der Wert nicht mehr der Original-String ist.
//
// Abbildung: 'A'-'Z' -> Kleinbuchstabe (dieselbe Identitaet, nur in der Klasse); jedes andere Zeichen
// ausserhalb der Klasse -> '_' (ein Zeichen je Zeichen, damit die Laenge stabil bleibt und zwei
// verschiedene Rohwerte nicht durch Kollabieren gleich werden).
// ---------------------------------------------------------------------------
[[nodiscard]] constexpr std::string sanitisiere_wert(std::string_view roh) {
    std::string out;
    out.reserve(roh.size());
    for (char c : roh) {
        if (c >= 'A' && c <= 'Z') {
            out += static_cast<char>(c - 'A' + 'a');
        } else if (ist_wert_zeichen(c)) {
            out += c;
        } else {
            out += '_';
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Der deterministische Kurzname: die ersten kKurznameHexLen Hex-Zeichen des SHA512 ueber die VOLLE
// Kette. Deterministisch heisst: dieselbe Kette ergibt auf jeder Maschine, in jedem Lauf denselben
// Kurznamen -- deshalb ein Hash und keine laufende Nummer.
// ---------------------------------------------------------------------------
[[nodiscard]] constexpr std::string kurzname_hex(std::string_view kette) {
    auto const digest = ::comdare::cache_engine::sha512::sha512(
        std::span<std::uint8_t const>{reinterpret_cast<std::uint8_t const*>(kette.data()), kette.size()});
    constexpr char kHex[] = "0123456789abcdef";
    std::string    out;
    out.reserve(kKurznameHexLen);
    for (std::size_t i = 0; i < (kKurznameHexLen + 1) / 2; ++i) {
        out += kHex[(digest[i] >> 4) & 0xf];
        if (out.size() < kKurznameHexLen) out += kHex[digest[i] & 0xf];
    }
    return out;
}

namespace detail {

/// Die rohe kv-Kette OHNE Laengenwache: "a=1+b=2". Getrennt, weil sowohl das Komponenten-Limit als
/// auch der Dateinamen-Bau dieselbe Kette brauchen, aber unterschiedlich budgetieren.
[[nodiscard]] constexpr std::string baue_kv_kette(std::span<KvPaar const> paare) {
    std::string out;
    for (std::size_t i = 0; i < paare.size(); ++i) {
        if (i != 0) out += kKvJoiner;
        out.append(paare[i].achse);
        out += kKvTrenner;
        out.append(paare[i].wert);
    }
    return out;
}

/// Alle Paare pruefen; erste Fehlerklasse gewinnt.
[[nodiscard]] constexpr LagerPfadFehler pruefe_alle(std::span<KvPaar const> paare) noexcept {
    for (auto const& p : paare)
        if (auto const f = pruefe_kv(p); f != LagerPfadFehler::ok) return f;
    return LagerPfadFehler::ok;
}

/// 8 bzw. 6 Ziffern -- die Datum-/Zeit-Form des A9-Dateinamen-Schemas (UTC).
[[nodiscard]] constexpr bool nur_ziffern(std::string_view s, std::size_t n) noexcept {
    if (s.size() != n) return false;
    for (char c : s)
        if (c < '0' || c > '9') return false;
    return true;
}

} // namespace detail

// ---------------------------------------------------------------------------
// kv_kette -- EINE Ebene des Baumes als kv-Kette: "mess=vereint+load_framework=on".
//
// UEBERLAUF: ueberschreitet die Kette kMaxKomponenteBytes, tritt der deterministische Kurzname
// "H=<hex16>" an ihre Stelle; die Voll-Kette reist im Ergebnis mit und MUSS vom Aufrufer festgehalten
// werden (INFO-Blatt bzw. Knoten-Log). Es wird NIE still abgeschnitten -- ein abgeschnittener
// Ordnername waere ein anderer Ordner, der wie der richtige aussieht.
// ---------------------------------------------------------------------------
[[nodiscard]] constexpr PfadErgebnis kv_kette(std::span<KvPaar const> paare) {
    PfadErgebnis e;
    if (paare.empty()) {
        e.fehler = LagerPfadFehler::kette_leer;
        return e;
    }
    if (auto const f = detail::pruefe_alle(paare); f != LagerPfadFehler::ok) {
        e.fehler = f;
        return e;
    }
    std::string kette = detail::baue_kv_kette(paare);
    if (kette.size() > kMaxKomponenteBytes) {
        e.gekuerzt   = true;
        e.voll_kette = kette;
        e.text       = str_von(kKurznamePraefix) + kurzname_hex(kette);
        return e;
    }
    e.text = std::move(kette);
    return e;
}

// ---------------------------------------------------------------------------
// gruppen_segment -- ein ORGAN-GRUPPEN-Ordner: "01_read_path=search_algo-prt_art+cache_traversal-bfs"
// (A9-Doc 6.1). Die Gruppe traegt das '=', die Achsen-Bindung innen den '-'.
//
// DESHALB die Zusatz-Wache gruppen_wert_trenner: '-' ist in der Werte-Zeichenklasse legal (Versionen),
// im Gruppen-Segment aber der PAAR-Trenner. Ein Wert mit '-' machte die Zerlegung mehrdeutig. Statt die
// Mehrdeutigkeit stillschweigend zuzulassen (und spaeter falsch zu parsen) wird sie hier klassifiziert
// abgewiesen -- die Organ-Achsen-Werte des Hauses tragen Unterstriche (b_tree, prt_art, path_
// compression_none), die Regel kostet also nichts und schliesst eine ganze Fehlerklasse.
// ---------------------------------------------------------------------------
[[nodiscard]] constexpr PfadErgebnis gruppen_segment(std::string_view gruppe, std::span<KvPaar const> paare) {
    PfadErgebnis e;
    if (gruppe.empty()) {
        e.fehler = LagerPfadFehler::achse_leer;
        return e;
    }
    if (!ist_achse_rein(gruppe)) {
        e.fehler = LagerPfadFehler::achse_zeichenklasse;
        return e;
    }
    if (paare.empty()) {
        e.fehler = LagerPfadFehler::kette_leer;
        return e;
    }
    if (auto const f = detail::pruefe_alle(paare); f != LagerPfadFehler::ok) {
        e.fehler = f;
        return e;
    }
    for (auto const& p : paare)
        if (p.wert.find(kGruppenPaarTrenner) != std::string_view::npos) {
            e.fehler = LagerPfadFehler::gruppen_wert_trenner;
            return e;
        }
    std::string innen;
    for (std::size_t i = 0; i < paare.size(); ++i) {
        if (i != 0) innen += kKvJoiner;
        innen.append(paare[i].achse);
        innen += kGruppenPaarTrenner;
        innen.append(paare[i].wert);
    }
    std::string kette = str_von(gruppe);
    kette += kKvTrenner;
    kette += innen;
    if (kette.size() > kMaxKomponenteBytes) {
        e.gekuerzt   = true;
        e.voll_kette = kette;
        e.text       = str_von(gruppe) + kKvTrenner + str_von(kKurznamePraefix) + kurzname_hex(kette);
        // Auch der GEKUERZTE Gruppen-Ordner behaelt seinen Gruppen-Namen: der Baum bleibt fuer einen
        // Menschen navigierbar, nur die Werte-Rekombination weicht dem Hash. Wird auch das zu lang
        // (absurd lange Gruppen-Namen), meldet die Wache unten ehrlich statt zu schneiden.
        if (e.text.size() > kMaxKomponenteBytes) e.fehler = LagerPfadFehler::komponente_ueberlang;
        return e;
    }
    e.text = std::move(kette);
    return e;
}

// ---------------------------------------------------------------------------
// blatt_dateiname -- <datum>-<zeit>_<kvkette>.<endung> (A9-Doc Abschnitt 5).
//
// Name = NUR Datum + Uhrzeit + die DYNAMISCHEN Unter-Achsen-Variablen (KERN 26.07. Section 6):
// Haupt-Achsen liegen im ORDNER-Pfad und als Metadaten IN der Datei, konstante Variablen werden
// weggelassen. A-15 bleibt gewahrt: Unter-Achsen erscheinen NIE im Stempel -- diese Datei ist ihr
// Zuhause. Ueberlauf -> "<datum>-<zeit>_H=<hex16>.<endung>" mit Voll-Kette im Ergebnis.
//
// Eine LEERE kv-Kette ist hier zulaessig und bedeutet "keine dynamische Variable" -> der Name ist
// "<datum>-<zeit>.<endung>". Das ist der Normalfall eines Blattes, unter dem nichts mehr variiert;
// ihn als kette_leer abzulehnen waere eine erfundene Pflicht.
// ---------------------------------------------------------------------------
[[nodiscard]] constexpr PfadErgebnis blatt_dateiname(std::string_view datum, std::string_view zeit,
                                                     std::span<KvPaar const> paare, std::string_view endung) {
    PfadErgebnis e;
    if (!detail::nur_ziffern(datum, 8)) {
        e.fehler = LagerPfadFehler::datum_form;
        return e;
    }
    if (!detail::nur_ziffern(zeit, 6)) {
        e.fehler = LagerPfadFehler::zeit_form;
        return e;
    }
    if (endung.empty()) {
        e.fehler = LagerPfadFehler::endung_leer;
        return e;
    }
    if (!ist_wert_rein(endung)) {
        e.fehler = LagerPfadFehler::wert_zeichenklasse;
        return e;
    }
    if (auto const f = detail::pruefe_alle(paare); f != LagerPfadFehler::ok) {
        e.fehler = f;
        return e;
    }
    std::string const kopf  = str_von(datum) + '-' + str_von(zeit);
    std::string const kette = detail::baue_kv_kette(paare);
    std::string       name  = kopf;
    if (!kette.empty()) {
        name += '_';
        name += kette;
    }
    name += '.';
    name.append(endung);
    if (name.size() > kMaxKomponenteBytes) {
        e.gekuerzt   = true;
        e.voll_kette = kette;
        e.text       = kopf + '_' + str_von(kKurznamePraefix) + kurzname_hex(kette) + '.' + str_von(endung);
        if (e.text.size() > kMaxKomponenteBytes) e.fehler = LagerPfadFehler::komponente_ueberlang;
        return e;
    }
    e.text = std::move(name);
    return e;
}

// ---------------------------------------------------------------------------
// Pfad-Komposition. pfad_join IST zugleich der minio-Objekt-Praefix (LB-0, s. Kopf) -- es gibt keine
// zweite Uebersetzung.
// ---------------------------------------------------------------------------
[[nodiscard]] constexpr std::string pfad_join(std::span<std::string const> ebenen) {
    std::string out;
    for (auto const& e : ebenen) {
        if (e.empty()) continue; // eine leere Ebene erzeugt nie "//" (der Aufrufer laesst Ebenen weg)
        if (!out.empty()) out += kEbenenTrenner;
        out += e;
    }
    return out;
}

/// Objekt-Schluessel == Praefix + Trenner + Blatt-Name. Dieselbe Funktion fuer Dateisystem-Pfad und
/// Objekt-Store-Key -- der Unterschied ist die Wurzel, nicht die Grammatik.
[[nodiscard]] constexpr std::string objekt_key(std::string_view praefix, std::string_view name) {
    if (praefix.empty()) return str_von(name);
    std::string out = str_von(praefix);
    out += kEbenenTrenner;
    out.append(name);
    return out;
}

// ---------------------------------------------------------------------------
// pfad_praefix_passt -- die PRAEFIX-Semantik MIT Trenner (TP1-F1 auf Ordner-Ebene).
//
// Ein nacktes starts_with ist an dieser Grammatik FALSCH: Werte sind numerisch ("...=1", "...=16"),
// also ist jeder kurze Ordnername Praefix eines laengeren. Derselbe Defekt, den
// discard_fresh_with_pfad_prefix (builder_registration.hpp:698) fuer Eintragspfade schon traegt --
// hier fuer die Baum-Ebenen. Gleichheit zaehlt als Treffer (ein Knoten ist sein eigenes Praefix).
// ---------------------------------------------------------------------------
[[nodiscard]] constexpr bool pfad_praefix_passt(std::string_view pfad, std::string_view praefix) noexcept {
    if (praefix.empty()) return false; // ein leeres Praefix traefe alles -- das ist nie gemeint
    if (pfad == praefix) return true;
    if (pfad.size() <= praefix.size()) return false;
    if (pfad.substr(0, praefix.size()) != praefix) return false;
    return pfad[praefix.size()] == kEbenenTrenner;
}

// ---------------------------------------------------------------------------
// CT-BATTERIE -- die Regeln dieses Headers beweisen sich beim Uebersetzen. Wer eine Regel aendert,
// bricht hier, nicht erst im Feld.
// ---------------------------------------------------------------------------

// Zeichenklassen.
static_assert(ist_wert_zeichen('a') && ist_wert_zeichen('z') && ist_wert_zeichen('0') && ist_wert_zeichen('9'));
static_assert(ist_wert_zeichen('.') && ist_wert_zeichen('_') && ist_wert_zeichen('-'));
static_assert(!ist_wert_zeichen('A') && !ist_wert_zeichen('=') && !ist_wert_zeichen('+') && !ist_wert_zeichen('/'));
static_assert(!ist_wert_zeichen(' ') && !ist_wert_zeichen(':'));
static_assert(ist_achsen_zeichen('0') && ist_achsen_zeichen('_') && ist_achsen_zeichen('r'));
static_assert(!ist_achsen_zeichen('-') && !ist_achsen_zeichen('.'),
              "Achsen-Namen duerfen weder den Gruppen-Paar-Trenner noch den Endungs-Punkt tragen.");
static_assert(ist_achse_rein("01_read_path") && ist_achse_rein("target_isa") && ist_achse_rein("load_framework"));
static_assert(ist_wert_rein("amd64_v3") && ist_wert_rein("6.17.0-35") && ist_wert_rein("sweep"));
static_assert(!ist_wert_rein("") && !ist_achse_rein(""));

// K1 (09.08.2026) -- DIE UMKEHRUNG DES L3-SATZES, an derselben Stelle und genauso hart.
// Vorher stand hier: pruefe_kv({"tier","hybrid"}) == hybrid_segment_verboten. K1 hebt das auf, also
// muss der Gegensatz belegt sein -- sonst waere die Aufhebung nur ein geloeschter Test. Ein
// Hybrid-Wert ist ab jetzt ein Wert wie jeder andere: er faellt NUR noch durch die Zeichenklasse.
static_assert(pruefe_kv(KvPaar{"tier", "hybrid"}) == LagerPfadFehler::ok,
              "K1 (Owner 09.08.2026): die Hybrid-Gattung wird 'parallel zu allen anderen Gattung+Genus' in den "
              "beiden Wurzelebenen einsortiert. Ein Sonderfall gegen 'hybrid' in der Zeichen-Grammatik waere "
              "damit die Regression -- die Ordnung entsteht im Baum-Writer, nicht in einem Namens-Verbot.");
static_assert(pruefe_kv(KvPaar{"hybrid_tier", "on"}) == LagerPfadFehler::ok);
static_assert(pruefe_kv(KvPaar{"tier", "hybridization"}) == LagerPfadFehler::ok);
// GEGENEINGANG (T-4): die Aufhebung ist ENG -- sie hebelt die Zeichenklasse NICHT aus.
static_assert(pruefe_kv(KvPaar{"tier", "Hybrid"}) == LagerPfadFehler::wert_zeichenklasse,
              "K1 hebt das Hybrid-VERBOT auf, nicht die Zeichenklasse: Grossbuchstaben bleiben abgewiesen.");

// Fehlerklassen-Reihenfolge (erste Ursache gewinnt).
static_assert(pruefe_kv(KvPaar{"", "x"}) == LagerPfadFehler::achse_leer);
static_assert(pruefe_kv(KvPaar{"Achse", "x"}) == LagerPfadFehler::achse_zeichenklasse);
static_assert(pruefe_kv(KvPaar{"achse", ""}) == LagerPfadFehler::wert_leer);
static_assert(pruefe_kv(KvPaar{"achse", "WERT"}) == LagerPfadFehler::wert_zeichenklasse);
static_assert(pruefe_kv(KvPaar{"achse", "wert"}) == LagerPfadFehler::ok);

// Praefix-Semantik MIT Trenner (TP1-F1-Klasse auf Ordner-Ebene).
static_assert(pfad_praefix_passt("a=1/b=2", "a=1"));
static_assert(pfad_praefix_passt("a=1", "a=1"));
static_assert(!pfad_praefix_passt("a=16/b=2", "a=1"), "TP1-F1: '=1' darf niemals '=16' treffen.");
static_assert(!pfad_praefix_passt("a=1", ""));

namespace detail {

// Die drei A9-NAMENS-BEISPIELE (A9-Doc Abschnitt 5) als consteval-Anker. Sie sind die Abnahme-Naht
// zwischen dieser Single-Source und dem A9-Writer: aendert jemand die Grammatik, bricht das
// Uebersetzen HIER -- lange bevor A9 einen abweichenden Namen erzeugt (L5, "kein Fork").
[[nodiscard]] consteval bool a9_namens_beispiele_treffen() {
    {
        std::array<KvPaar, 3> const p{KvPaar{"measurement_category", "wallclock"}, KvPaar{"workload", "ycsb_a"},
                                      KvPaar{"working_set_n", "sweep"}};
        auto const                  e = blatt_dateiname("20260812", "093011", p, "xlsx");
        if (!e.ok() || e.gekuerzt) return false;
        if (e.text != "20260812-093011_measurement_category=wallclock+workload=ycsb_a+working_set_n=sweep.xlsx")
            return false;
    }
    {
        std::array<KvPaar, 2> const p{KvPaar{"opt_level", "sweep"}, KvPaar{"atomic128", "on"}};
        auto const                  e = blatt_dateiname("20260812", "101500", p, "xlsx");
        if (!e.ok() || e.text != "20260812-101500_opt_level=sweep+atomic128=on.xlsx") return false;
    }
    {
        std::array<KvPaar, 2> const p{KvPaar{"node_fanout", "sweep"}, KvPaar{"prefetch_distance", "4"}};
        auto const                  e = blatt_dateiname("20260812", "114205", p, "xlsx");
        if (!e.ok() || e.text != "20260812-114205_node_fanout=sweep+prefetch_distance=4.xlsx") return false;
    }
    return true;
}

// Die zwei Pfad-Beispiele aus A9-Doc 6.1 (Ebene 1 und Ebene 2 der Messdaten-Kaskade) und ein
// Gruppen-Ordner -- dieselbe Anker-Logik auf der ORDNER-Ebene.
[[nodiscard]] consteval bool a9_pfad_beispiele_treffen() {
    {
        std::array<KvPaar, 2> const p{KvPaar{"mess", "vereint"}, KvPaar{"load_framework", "on"}};
        auto const                  e = kv_kette(p);
        if (!e.ok() || e.text != "mess=vereint+load_framework=on") return false;
    }
    {
        std::array<KvPaar, 3> const p{KvPaar{"target_isa", "amd64_v3"}, KvPaar{"operating_system", "linux"},
                                      KvPaar{"external_utils", "avx2"}};
        auto const                  e = kv_kette(p);
        if (!e.ok() || e.text != "target_isa=amd64_v3+operating_system=linux+external_utils=avx2") return false;
    }
    {
        std::array<KvPaar, 2> const p{KvPaar{"search_algo", "prt_art"}, KvPaar{"cache_traversal", "bfs"}};
        auto const                  e = gruppen_segment("01_read_path", p);
        if (!e.ok() || e.text != "01_read_path=search_algo-prt_art+cache_traversal-bfs") return false;
    }
    return true;
}

} // namespace detail

static_assert(detail::a9_namens_beispiele_treffen(),
              "Die drei Namens-Beispiele des A9-Design-Dossiers (Abschnitt 5) sind der Abnahme-Anker dieser "
              "Grammatik. Wer sie bricht, forkt die Single-Source -- genau das verhindert L5.");
static_assert(detail::a9_pfad_beispiele_treffen(),
              "Die Pfad-Beispiele des A9-Design-Dossiers (Abschnitt 6.1) sind der Abnahme-Anker der "
              "Ordner-Ebenen. Ein Bruch hier heisst: Baum-Writer und xlsx-Writer legen verschiedene Ordner an.");

} // namespace comdare::cache_engine::builder::bestandslog
