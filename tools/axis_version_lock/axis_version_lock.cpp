// S-14a BUMP-WACHE TEIL 1 -- DER RIEGEL (Task #33; Plan 13.3 #90; Owner KON17-03 'Wache Modular
// erweitern und in Detail-Klassen splitten'). Nachfolger des PAKET-W3-C-Tools (GN-8/O-4, Ledger
// Sec.32-F4): Content-Digest-Tripwire ueber die algo_version-Traeger des Baums.
//
// WAS SICH GEGENUEBER v1 AENDERT (und warum):
//   * DISCOVERY STATT ARGV-LISTE. v1 pruefte genau die Header, die ihm der Aufrufer nannte -- die
//     hartkodierte 6-Pfad-Liste in der CI-YAML war Mitursache des Drei-Wochen-Ausfalls, und ein
//     verwaister Lock-Eintrag blieb prinzipiell unsichtbar (do_check iterierte nur ueber argv, nie
//     ueber Lock-Eintraege; am 13.08.2026 literal belegt: Phantom-Eintrag -> 'GRUEN', Exit 0).
//     v2 erhebt seine Grundgesamtheit SELBST unter --root und prueft BIDIREKTIONAL.
//   * KATEGORIE-DETAIL-KLASSEN (KON17-03): HeuristikDetail traegt den Marker-Mechanismus,
//     OrganDetail die Literal-Mechanik (axes/ + topics/queuing/). EINE Wache, EIN Lock
//     (KON27-01/KON17-03) -- die Kategorien sind Spalten desselben Locks, kein zweites Lockfile.
//   * ZWEI RIEGEL-SCHAERFUNGEN: (i) Lock-Eintrag ohne Datei => ROT (verwaist); (ii) je Kategorie
//     ein discovered-Zaehler MIT NENNER in der Ausgabe (BESTAND-Zeilen), damit die Zahl gefahren
//     wird statt behauptet.
//
// PFLICHT-FIXUP 2 (13.08.2026, Owner-Dauerregel 'Luecke = Behebung ist IMMER Pflicht'; 22 Funde
// aus zwei unabhaengigen Lenses, jeder Kern-Fund vorab am gebauten Tool literal gefahren):
//   (G1) RUHELAGE WIRD GEPRUEFT. Vorher uebersprang do_check bei Digest-Gleichheit JEDE weitere
//        Pruefung: ein verfaelschtes Register (Version 1.0.2.c -> 1.9.0.c von Hand, Digest-Zeile
//        erhalten) blieb GRUEN/Exit 0 -- und der naechste echte Vergleich lief gegen die falsche
//        Basis. Jetzt werden Version UND Kategorie auch in der Ruhelage gegen das Register
//        gehalten; ein Sentinel-/unentscheidbares Literal ist IMMER rot, digest-unabhaengig
//        (vorher: handverdrahtetes 'kaputt'-Register mit passendem Digest => GRUEN, literal
//        belegt). --write schreibt bei rc != 0 NICHTS mehr (vorher: rc=1 UND Lock geschrieben).
//   (G2) DER BUMP-ZWEIG IST KEIN DAUERLOCH MEHR. Vorher: Bump ohne Lock-Regen => Exit 0, und
//        JEDER weitere Inhalts-Drift unter der einmal gebumpten Version blieb gruen (Edit2 mit
//        neuem Inhalt, Version unveraendert => wieder Exit 0, literal belegt). Design-Entscheid,
//        BEIDE Wege: (a) der Bump-Zweig endet mit Exit 3 'REGEN ERFORDERLICH' -- --check ist nie
//        gruen, solange das Register nicht regeneriert ist; erst der Regen-Commit segnet den
//        KONKRETEN Inhalt. (b) Der CI-Job erzwingt zusaetzlich Byte-Identitaet des Registers
//        (--write + git diff --exit-code) -- Tiefenstaffelung, faengt auch Klassen, die --check
//        strukturell nicht sieht.
//   (G3) ALLE LITERALE EINER DATEI, NICHT DAS ERSTE. axes/lookup/axis_03a_search_algo_k_ary.hpp
//        traegt ZWEI Variantenfamilien mit je eigenem Literal (:141 KArySearchAlgoCore, :573
//        KAryPerKCore). Vorher zaehlte nur :141 -- Bump am falschen Literal => 'OK MIT
//        Version-Bump'/GRUEN, korrekter Bump an :573 => falsches ROT (beides literal belegt).
//        Jetzt erfasst der Scan die geordnete LITERAL-LISTE; Bump-Regel s. bump_ok.
//   (G4) HEURISTIK-WEG AUF ORGAN-NIVEAU. Marker gilt nur als DEDIZIERTE Kommentar-Zeile
//        (zeilen-verankert); Prosa-Zitate mid-line stellen die Version nicht mehr (vorher:
//        Zitat 'AXIS_ALGO_VERSION: 99' vor dem echten Marker => Version 99, literal belegt).
//        Ueberlauf ist ROT statt modulo-2^64 (vorher: Marker 2^64 => Version 0). Fehlender/
//        mehrdeutiger/unparsbarer Marker ist ROT statt '0 + Warnung'.
//   (G5) NENNER UND FAIL-CLOSED. Mindest-Nenner IM Werkzeug (leere Homes ergaben vorher
//        'GRUEN -- 0 Dateien', V-1 woertlich gebrochen, literal belegt). Unlesbares
//        UNTERverzeichnis => Exit 2 statt std::terminate/SIGABRT (literal belegt: Abort 134).
//        Nicht-hpp-Datei mit Literal/Marker unter den Homes => ROT statt unsichtbar (.h-Traeger
//        war vollstaendig unsichtbar, literal belegt). Verzeichnis-/kaputte Symlinks => Exit 2
//        statt stilles Ueberspringen (Traeger hinter Dir-Symlink unsichtbar, literal belegt).
//   (G6) SCANNER-ROBUSTHEIT. C++14-Digit-Separator (1'000.0) schaltete den char_lit-Modus und
//        verschluckte bis zum naechsten Apostroph -- ein Literal dahinter wurde still als '-'
//        digest-only gelockt (literal belegt). Literal unter #if/#ifdef/#ifndef ist ROT 'nicht
//        entscheidbar' (vorher gewann ein '#if 0'-Literal, literal belegt; Bestand hat 0 solche
//        Faelle, gemessen 13.08.2026). read_file_bytes prueft Streamzustand UND Vollstaendigkeit
//        gegen die Dateigroesse (vorher konnte ein I/O-Fehler nach dem Oeffnen partielle Bytes
//        als vollstaendig behandeln -- Digest ueber eine halbe Datei; am Code verifiziert,
//        Live-Repro braeuchte Fehler-Injektion).
//   (G7) TEST-LUECKEN geschlossen im ctest test_s14_axis_version_lock_tripwire (heuristik-ROT-
//        Pfad beidseitig, Lock-Byteidentitaet bei fehlgeschlagenem --write, v1-Abweisung).
//
// KATEGORIEN UND IHRE REGELN:
//   heuristik  Home libs/cache_engine/heuristik/, jede *.hpp ist Traeger. Version = Integer aus
//              der DEDIZIERTEN Marker-Zeile '// AXIS_ALGO_VERSION: <N>' (zeilen-verankert: vor
//              dem '//' nur Weissraum, nach der Zahl nur Weissraum). 0 Marker, >1 Marker oder
//              unparsbarer Marker (keine Ziffer, Ueberlauf, Restzeichen) => ROT. bump_ok =
//              Integer echt groesser (streng geparst, kein strtoull-Clamp).
//   organ      Homes libs/cache_engine/axes/ + libs/cache_engine/topics/queuing/, rekursiv *.hpp.
//              DISCOVERED ist jede Datei, die den Substring 'algo_version' enthaelt -- exakt die
//              golden-Erhebung (grep -l) der Baseline vom 13.08.2026, damit deren 158er-Schnitt
//              digest-stabil weiterlebt. TRAEGER ist darunter jede Datei mit mindestens einer
//              echten algo_version-String-Literal-Zuweisung IM CODE (kommentar-/string-bewusster
//              Scan mit Wortgrenzen; Forwarder wie '= Strategy::algo_version' und Prosa fallen
//              heraus und stehen als version='-' digest-only im Lock).
//              VERSION = geordnete LISTE ALLER Literale (G3). KANONISCHE FORM im Register:
//              N gleiche Literale => der Einzelwert (haelt die golden Baseline byte-stabil,
//              k_ary: 2x '1.0.0.c' => '1.0.0.c'); ungleiche => komma-gefuegt in Text-Reihenfolge
//              ('1.1.0.c,1.0.0.c'). bump_ok ueber die Liste: gegen ein EINZELWERT-Register darf
//              kein Ist-Literal kleiner sein und mindestens eines muss echt groesser sein
//              (aritaets-frei -- das Register sagt 'alle standen auf v'); gegen ein LISTEN-
//              Register gilt gleiche Aritaet + elementweise nie kleiner, mindestens einmal echt
//              groesser. Aritaetswechsel gegen ein Listen-Register hat KEINEN Bump-Pfad
//              (bewusster Regen-Commit). Grammatik ueber den BESTANDS-Parser
//              measurement/algo_semver.hpp (EIN Parser, keine Zweitgrammatik); unparsbares
//              Literal ist Sentinel => ROT.
//
// LOCK-FORMAT v2 (v1 wird mit klarer Meldung abgewiesen, kein stilles Weiterlesen):
//   # format: v2
//   ZWEIZEILEN-RECORD je Datei:  '<category> <version> <relative-path>' + Folgezeile
//   '    <sha256-hex>' (vier Leerzeichen Einrueckung). WARUM ZWEI ZEILEN: 64 Hex-Zeichen + Pfad
//   passen strukturell nicht in die 120-Spalten-Diff-Hygiene (ci_diff_ascii_width_guard); das
//   Zweizeilen-Format haelt JEDE Zeile der Datei wachen-gedeckt, statt eine Ausnahme-Klasse in
//   der Wache zu eroeffnen. Records global nach Pfad sortiert; version bei organ = kanonische
//   Literal-Form (s.o.) bzw. '-'. Eine Kopfzeile ohne Digest-Folgezeile ist ein LAUTER
//   Formatfehler (kein Ueberlesen). Der Parser liest STRENG (Pflicht-Fixup 13.08.2026): kein
//   viertes Token auf der Kopfzeile (Pfade mit Leerzeichen sind nicht zugelassen -- deklarierte
//   Grenze), Digest = genau 64 lowercase-Hex-Zeichen ohne Zusatz, kein doppelter Record je Pfad.
//   Jede Verletzung nennt die literale Zeile und macht ROT.
//   LOCK-KOPF BLEIBT BYTE-STABIL: die Kommentarzeilen im Lock beschreiben die v2-ERSTFORM
//   ('erstes Literal'); seit G3 gilt die LISTEN-Regel oben. Der Kopf wird BEWUSST nicht
//   angefasst, weil die golden Baseline byte-identisch regenerierbar bleiben MUSS (CI erzwingt
//   --write + git diff --exit-code auf das Lock). Die geltende Semantik steht HIER.
//
// AUFRUF (CI ruft ohne Datei-Liste; die Grundgesamtheit erhebt das Tool):
//   axis_version_lock --write <lockfile> [--root <repo-root>]
//   axis_version_lock --check <lockfile> [--root <repo-root>]
// Exit: 0 = gruen (Register aktuell und konsistent), 1 = ROT (Tripwire/Politik), 2 = Usage/
// Umgebungsfehler (Home fehlt, Datei/Verzeichnis unlesbar, Symlink-Struktur, Mindest-Nenner
// unterschritten -- fail-closed), 3 = NUR --check: Version-Bump akzeptiert, aber Lock-Regen
// fehlt ('REGEN ERFORDERLICH', G2 -- nie stilles Gruen auf veraltetem Register).
//
// BETRIEBSFOLGE (deklariert, seit G2 verschaerft): JEDE Aenderung an einem Traeger verlangt den
// Lock-Regen-Commit (--write); ein Versions-Bump macht den Regen LEGITIM (Exit 3 statt ROT),
// ersetzt ihn aber nicht. Forwarder-/Prosa-Dateien haben keinen Bump-Pfad und verlangen IMMER
// den bewussten Regen-Commit. Das ist gewollt (trifft S-6/S-7).
//
// SHA-256: WIEDERVERWENDUNG der vorhandenen consteval/constexpr-Implementation
// (libs/cache_engine/src/sha256/ctsha.hpp, RFC 6234). KEINE zweite SHA-Implementation.
// Der CT-Zaehler kAllRegisteredOrganVariantCount wird ABSICHTLICH NICHT hier eingebunden (die
// Varianten-Tabelle ist eine schwere Registry-TU; dieses Tool bleibt eine schlanke Datei-Wache).
// Der ctest test_s14_axis_version_lock_tripwire loggt ihn NEBEN den BESTAND-Zaehlern -- nur
// loggen, nie gleichsetzen (Datei != Variante).
//
// INERT-by-default (Anti-Phantom): EXCLUDE_FROM_ALL; gebaut vom CI-Job contract:axis-version-lock
// und als Fixture-Dependency des ctest.

#include <sha256/ctsha.hpp> // comdare::cache_engine::sha256::{sha256, to_hex, Digest}

#include <cache_engine/measurement/algo_semver.hpp> // BESTANDS-Parser parse_algo_semver (X.Y.Z[.flags])

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace fs   = std::filesystem;
namespace sha  = ::comdare::cache_engine::sha256;
namespace meas = ::comdare::cache_engine::measurement;

// MINDEST-NENNER (G5a, Hausvertrag V-1 'Nenner 0 = Exit != 0, nie GRUEN'): unterschreitet die
// Grundgesamtheit diese Anker, ist der Lauf ein Umgebungsfehler (Exit 2) -- eine Wache ueber
// einer leeren Menge behauptet sonst Konsistenz, die sie nie gemessen hat. Die Werte spiegeln
// die ctest-Anker (heuristik==6, organ>=120) und sind BEWUSST so gewaehlt, dass die
// angekuendigte #16-Umgliederung der Homes sie anfassen MUSS (wer Homes verschiebt/leert,
// entscheidet die neuen Anker hier, nicht per stillem Gruen).
inline constexpr int kMinHeuristikDiscovered = 6;
inline constexpr int kMinOrganDiscovered     = 120;

/// Rohe Bytes einer Datei. false = nicht lesbar ODER nicht vollstaendig lesbar.
/// FAIL-CLOSED (G6c): 'ss << f.rdbuf()' setzt bei einem I/O-Fehler NACH dem Oeffnen keine
/// Fehlerflags am ifstream (die Extraktion laeuft am Streambuf vorbei am Stream-Zustand) --
/// partielle Bytes saehen wie eine vollstaendig gelesene Datei aus und der Digest liefe ueber
/// eine halbe Datei. Deshalb: badbit pruefen UND die gelesene Laenge gegen fs::file_size halten.
[[nodiscard]] bool read_file_bytes(fs::path const& path, std::vector<std::uint8_t>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    if (ss.bad() || f.bad()) return false;
    std::error_code      ec;
    std::uintmax_t const soll_groesse = fs::file_size(path, ec);
    std::string const    s            = ss.str();
    if (ec || s.size() != soll_groesse) return false;
    out.assign(s.begin(), s.end());
    return true;
}

[[nodiscard]] std::string digest_hex_of(std::vector<std::uint8_t> const& bytes) {
    sha::Digest const d = sha::sha256(std::span<const std::uint8_t>{bytes.data(), bytes.size()});
    auto const        h = sha::to_hex(d);
    return std::string(h.begin(), h.end());
}

[[nodiscard]] bool ist_ident_zeichen(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

[[nodiscard]] bool ist_hex_ziffer(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

/// Exakt 64 lowercase-Hex-Zeichen -- das Alphabet, das sha256::to_hex emittiert (Pflicht-Fixup
/// 13.08.2026, Koeder G3: size()==64 allein liess einen 64-Zeichen-Muellstring als Digest durch).
[[nodiscard]] bool ist_sha256_hex(std::string const& s) {
    if (s.size() != 64) return false;
    for (char const c : s)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

/// Strenger uint64-Parser: nur Ziffern, mindestens eine, Ueberlauf => false (G4b: strtoull
/// clampt auf ULLONG_MAX und liesse '18446744073709551616' als Zahl durch; find()-Zeit wickelte
/// er sogar modulo 2^64 auf 0).
[[nodiscard]] bool parse_u64_streng(std::string_view s, std::uint64_t& out) {
    if (s.empty()) return false;
    std::uint64_t v = 0;
    for (char const c : s) {
        if (c < '0' || c > '9') return false;
        auto const d = static_cast<std::uint64_t>(c - '0');
        if (v > (std::numeric_limits<std::uint64_t>::max() - d) / 10u) return false;
        v = v * 10u + d;
    }
    out = v;
    return true;
}

/// Discovery-Lage einer Kategorie: env_ok=false ist die Exit-2-Klasse (Home fehlt, unlesbar,
/// Symlink-Struktur), politik_rot=true die Exit-1-Klasse (Traeger-Material ausserhalb der
/// deklarierten Grenze, G5c). Getrennt, weil Umgebungsfehler den Lauf beenden, Politik-Rot aber
/// die volle Diagnose weiterlaufen laesst.
struct DiscoveryLage {
    bool env_ok      = true;
    bool politik_rot = false;
};

/// Gemeinsame Eintrittspruefung beider discover()-Schleifen (G5d): Verzeichnis-Symlinks werden
/// vom recursive_directory_iterator NICHT deszendiert und kaputte Symlinks fallen durch
/// is_regular_file -- beides verschwand vorher STILL aus der Grundgesamtheit (literal belegt:
/// Traeger hinter Dir-Symlink => GRUEN ohne ihn). Datei-Symlinks auf existierende Ziele bleiben
/// wie bisher Traeger (Digest ueber die Ziel-Bytes).
[[nodiscard]] bool symlink_rot(fs::directory_entry const& de, fs::path const& root, DiscoveryLage& lage) {
    if (!de.is_symlink()) return false;
    // lexically_relative statt fs::relative: fs::relative kanonisiert und wuerde hier das ZIEL
    // des Symlinks benennen -- gemeldet gehoert der Ort des Links unter dem Home.
    std::string const rel = de.path().lexically_relative(root).generic_string();
    std::error_code   ec;
    if (fs::is_directory(de.path(), ec)) {
        std::fprintf(stderr,
                     "axis_version_lock: FEHLER Verzeichnis-Symlink unter einem Home wird nicht deszendiert "
                     "(nicht zugelassen): %s\n",
                     rel.c_str());
        lage.env_ok = false;
        return true;
    }
    if (ec || !fs::exists(de.path(), ec) || ec) {
        std::fprintf(stderr, "axis_version_lock: FEHLER unaufloesbarer Symlink unter einem Home: %s\n", rel.c_str());
        lage.env_ok = false;
        return true;
    }
    return false; // Datei-Symlink mit aufloesbarem Ziel: wie eine regulaere Datei behandeln
}

// ================================================================================================
// KATEGORIE-DETAIL: heuristik -- Marker-Mechanismus, seit G4 auf Organ-Strenge.
// ================================================================================================
struct HeuristikDetail {
    static constexpr std::string_view kName = "heuristik";

    /// Jede regulaere *.hpp unter dem Home ist Traeger (rekursiv; heute 6 flache Dateien).
    /// FAIL-CLOSED (G5b/G5c/G5d): Iterationsfehler => env-Fehler statt std::terminate; Nicht-hpp-
    /// Datei mit dedizierter Marker-Zeile => Politik-ROT (ein .h-Traeger waere sonst unsichtbar).
    [[nodiscard]] static std::vector<std::string> discover(fs::path const& root, DiscoveryLage& lage) {
        std::vector<std::string> rels;
        fs::path const           home = root / "libs/cache_engine/heuristik";
        if (!fs::is_directory(home)) {
            std::fprintf(stderr, "axis_version_lock: FEHLER heuristik-Home fehlt unter --root: %s\n",
                         "libs/cache_engine/heuristik");
            lage.env_ok = false;
            return rels;
        }
        try {
            for (auto const& de : fs::recursive_directory_iterator(home)) {
                if (symlink_rot(de, root, lage)) continue;
                if (!de.is_regular_file()) continue;
                std::string const rel = fs::relative(de.path(), root).generic_string();
                if (de.path().extension() == ".hpp") {
                    rels.push_back(rel);
                    continue;
                }
                std::vector<std::uint8_t> bytes;
                if (!read_file_bytes(de.path(), bytes)) {
                    std::fprintf(stderr, "axis_version_lock: FEHLER Datei nicht lesbar (heuristik-Discovery): %s\n",
                                 rel.c_str());
                    lage.env_ok = false;
                    continue;
                }
                if (marker_zeilen(bytes) > 0) {
                    std::fprintf(stderr,
                                 "axis_version_lock: ROT AXIS_ALGO_VERSION-Marker in Datei mit nicht "
                                 "zugelassener Endung (nur *.hpp ist Traeger): %s\n",
                                 rel.c_str());
                    lage.politik_rot = true;
                }
            }
        } catch (fs::filesystem_error const& e) {
            std::fprintf(stderr, "axis_version_lock: FEHLER heuristik-Discovery abgebrochen (%s)\n", e.what());
            lage.env_ok = false;
        }
        std::sort(rels.begin(), rels.end());
        return rels;
    }

    /// Zaehlt DEDIZIERTE Marker-Zeilen: vor dem '//' nur Weissraum, direkt nach '//' (plus
    /// Weissraum) der Marker. Prosa, die den Marker mid-line zitiert, zaehlt nicht (G4a).
    [[nodiscard]] static int marker_zeilen(std::vector<std::uint8_t> const& bytes) {
        int n = 0;
        durchlaufe_marker(bytes, [&](std::string_view, bool) { ++n; });
        return n;
    }

    /// Version = Integer der EINEN dedizierten Marker-Zeile. Rueckgabe beginnt bei Verletzung mit
    /// '!' (nie im Register eines gruenen Laufs: --write verweigert bei rc != 0, G1):
    ///   '!fehlt'      kein Marker (vorher: '0' + Warnung -- --write akzeptierte das, G4c)
    ///   '!mehrdeutig' mehr als eine Marker-Zeile
    ///   '!unparsbar'  Marker-Zeile ohne Zahl, mit Restzeichen oder Ueberlauf (G4b)
    [[nodiscard]] static std::string version_of(std::vector<std::uint8_t> const& bytes, std::string const&) {
        int         kandidaten = 0;
        bool        kaputt     = false;
        std::string wert;
        durchlaufe_marker(bytes, [&](std::string_view zahl, bool sauber) {
            ++kandidaten;
            std::uint64_t v = 0;
            if (!sauber || !parse_u64_streng(zahl, v)) {
                kaputt = true;
                return;
            }
            wert = std::to_string(v);
        });
        if (kandidaten == 0) return "!fehlt";
        if (kandidaten > 1) return "!mehrdeutig";
        if (kaputt) return "!unparsbar";
        return wert;
    }

    [[nodiscard]] static bool version_rot(std::string const& version) { return !version.empty() && version[0] == '!'; }

    [[nodiscard]] static char const* rot_grund(std::string const& version) {
        if (version == "!fehlt") return "kein dedizierter '// AXIS_ALGO_VERSION: <N>'-Marker";
        if (version == "!mehrdeutig") return "mehr als eine AXIS_ALGO_VERSION-Marker-Zeile (mehrdeutig)";
        return "AXIS_ALGO_VERSION-Marker unparsbar (keine Zahl, Restzeichen oder Ueberlauf)";
    }

    /// bump_ok = Integer echt groesser, beidseitig STRENG geparst (G4b: kein strtoull-Clamp).
    [[nodiscard]] static bool bump_ok(std::string const& alt, std::string const& neu) {
        std::uint64_t a = 0;
        std::uint64_t n = 0;
        if (!parse_u64_streng(alt, a) || !parse_u64_streng(neu, n)) return false;
        return n > a;
    }

private:
    /// Zerlegt die Bytes in Zeilen und ruft fn fuer jede dedizierte Marker-Zeile mit dem
    /// Zahl-Teil und einem Sauberkeits-Flag (nach der Zahl nur Weissraum bis Zeilenende).
    template <typename Fn>
    static void durchlaufe_marker(std::vector<std::uint8_t> const& bytes, Fn&& fn) {
        static constexpr std::string_view kMarker = "AXIS_ALGO_VERSION:";
        std::string_view const            text(reinterpret_cast<char const*>(bytes.data()), bytes.size());
        std::size_t                       zeile_von = 0;
        while (zeile_von <= text.size()) {
            std::size_t zeile_bis = text.find('\n', zeile_von);
            if (zeile_bis == std::string_view::npos) zeile_bis = text.size();
            std::string_view zeile = text.substr(zeile_von, zeile_bis - zeile_von);
            if (!zeile.empty() && zeile.back() == '\r') zeile.remove_suffix(1);
            std::size_t i       = 0;
            auto const  skip_ws = [&] {
                while (i < zeile.size() && (zeile[i] == ' ' || zeile[i] == '\t')) ++i;
            };
            skip_ws();
            if (zeile.compare(i, 2, "//") == 0) {
                i += 2;
                skip_ws();
                if (zeile.compare(i, kMarker.size(), kMarker) == 0) {
                    i += kMarker.size();
                    skip_ws();
                    std::size_t const zahl_von = i;
                    while (i < zeile.size() && zeile[i] >= '0' && zeile[i] <= '9') ++i;
                    std::string_view const zahl = zeile.substr(zahl_von, i - zahl_von);
                    skip_ws();
                    fn(zahl, i == zeile.size());
                }
            }
            if (zeile_bis == text.size()) break;
            zeile_von = zeile_bis + 1;
        }
    }
};

// ================================================================================================
// KATEGORIE-DETAIL: organ -- Literal-Mechanik (axes/ + topics/queuing/), seit G3 als LISTE.
// ================================================================================================
struct OrganDetail {
    static constexpr std::string_view kName = "organ";

    /// Ergebnis des Literal-Scans einer Datei: alle Literale in Text-Reihenfolge + die
    /// Praeprozessor-Diagnose (G6b).
    struct Scan {
        std::vector<std::string> literale;
        bool                     unter_praeprozessor = false;
    };

    /// discovered = *.hpp unter den zwei Homes, die den Substring 'algo_version' enthalten
    /// (golden-kompatible Erhebung, s. Kopf). Sortiert, '/'-Separatoren. FAIL-CLOSED wie bei
    /// HeuristikDetail::discover; zusaetzlich Politik-ROT fuer Nicht-hpp-Dateien, deren Bytes
    /// ein echtes Literal tragen (G5c: ein .h-Traeger war vorher vollstaendig unsichtbar).
    [[nodiscard]] static std::vector<std::string> discover(fs::path const& root, DiscoveryLage& lage) {
        std::vector<std::string> rels;
        for (char const* home_rel : {"libs/cache_engine/axes", "libs/cache_engine/topics/queuing"}) {
            fs::path const home = root / home_rel;
            if (!fs::is_directory(home)) {
                lage.env_ok = false;
                std::fprintf(stderr, "axis_version_lock: FEHLER organ-Home fehlt unter --root: %s\n", home_rel);
                continue;
            }
            try {
                for (auto const& de : fs::recursive_directory_iterator(home)) {
                    if (symlink_rot(de, root, lage)) continue;
                    if (!de.is_regular_file()) continue;
                    std::string const         rel = fs::relative(de.path(), root).generic_string();
                    std::vector<std::uint8_t> bytes;
                    if (!read_file_bytes(de.path(), bytes)) {
                        // FAIL-CLOSED (Pflicht-Fixup 13.08.2026, Koeder F): ein unlesbarer
                        // NEUZUGANG verschwand sonst still aus der Grundgesamtheit -- --check
                        // blieb GRUEN, --write regenerierte OHNE die Datei (literal belegt).
                        std::fprintf(stderr, "axis_version_lock: FEHLER Datei nicht lesbar (organ-Discovery): %s\n",
                                     rel.c_str());
                        lage.env_ok = false;
                        continue;
                    }
                    std::string_view const text(reinterpret_cast<char const*>(bytes.data()), bytes.size());
                    if (de.path().extension() == ".hpp") {
                        if (text.find("algo_version") == std::string_view::npos) continue;
                        rels.push_back(rel);
                        continue;
                    }
                    if (text.find("algo_version") == std::string_view::npos) continue;
                    if (!scan_literale(bytes).literale.empty()) {
                        std::fprintf(stderr,
                                     "axis_version_lock: ROT algo_version-Literal in Datei mit nicht "
                                     "zugelassener Endung (nur *.hpp ist Traeger): %s\n",
                                     rel.c_str());
                        lage.politik_rot = true;
                    }
                }
            } catch (fs::filesystem_error const& e) {
                std::fprintf(stderr, "axis_version_lock: FEHLER organ-Discovery abgebrochen (%s)\n", e.what());
                lage.env_ok = false;
            }
        }
        std::sort(rels.begin(), rels.end());
        return rels;
    }

    /// Kommentar-/string-bewusster Scan ueber ALLE algo_version-String-Literal-Zuweisungen im
    /// Code (G3: vorher gewann das ERSTE Literal; k_ary traegt zwei Variantenfamilien).
    /// Seit G6a kennt der Scan den C++14-Digit-Separator (Apostroph ZWISCHEN Hex-Ziffern bleibt
    /// Code); seit G6b zaehlt er #if/#ifdef/#ifndef-Tiefe mit und markiert Literale darunter als
    /// nicht entscheidbar. Der Scan kennt weiterhin KEINE Raw-Strings und keine Praefix-Char-
    /// Literale mit Hex-Buchstaben-Inhalt (u8'a') -- in den Traeger-Baeumen gibt es beide nicht;
    /// taucht einer auf, faellt er ueber die Ruhelage-Versionspruefung (G1) oder als
    /// Digest-Aenderung auf, nie als stilles Gruen.
    [[nodiscard]] static Scan scan_literale(std::vector<std::uint8_t> const& bytes) {
        Scan                              ergebnis;
        std::string_view const            text(reinterpret_cast<char const*>(bytes.data()), bytes.size());
        static constexpr std::string_view kTok = "algo_version";
        enum class Z { code, zeilen_kommentar, block_kommentar, string_lit, char_lit };
        Z           z                 = Z::code;
        int         praep_tiefe       = 0;
        bool        nur_ws_auf_zeile  = true;
        std::size_t i                 = 0;
        auto const  verbrauch_zeichen = [&](char c) {
            if (c == '\n')
                nur_ws_auf_zeile = true;
            else if (c != ' ' && c != '\t' && c != '\r')
                nur_ws_auf_zeile = false;
        };
        while (i < text.size()) {
            char const c = text[i];
            switch (z) {
                case Z::zeilen_kommentar:
                    if (c == '\n') z = Z::code;
                    verbrauch_zeichen(c);
                    ++i;
                    continue;
                case Z::block_kommentar:
                    if (c == '*' && i + 1 < text.size() && text[i + 1] == '/') {
                        z = Z::code;
                        verbrauch_zeichen('*');
                        i += 2;
                    } else {
                        verbrauch_zeichen(c);
                        ++i;
                    }
                    continue;
                case Z::string_lit:
                    if (c == '\\') {
                        i += 2;
                    } else {
                        if (c == '"') z = Z::code;
                        verbrauch_zeichen(c);
                        ++i;
                    }
                    continue;
                case Z::char_lit:
                    if (c == '\\') {
                        i += 2;
                    } else {
                        if (c == '\'') z = Z::code;
                        verbrauch_zeichen(c);
                        ++i;
                    }
                    continue;
                case Z::code: break;
            }
            if (c == '/' && i + 1 < text.size() && text[i + 1] == '/') {
                z = Z::zeilen_kommentar;
                verbrauch_zeichen(c);
                i += 2;
                continue;
            }
            if (c == '/' && i + 1 < text.size() && text[i + 1] == '*') {
                z = Z::block_kommentar;
                verbrauch_zeichen(c);
                i += 2;
                continue;
            }
            if (c == '"') {
                z = Z::string_lit;
                verbrauch_zeichen(c);
                ++i;
                continue;
            }
            if (c == '\'') {
                // G6a: C++14-Digit-Separator (1'000'000). Ein Apostroph ZWISCHEN Hex-Ziffern ist
                // Teil eines Zahlen-Tokens, kein Char-Literal-Beginn -- vorher schaltete er den
                // char_lit-Modus und verschluckte Text bis zum naechsten Apostroph (ein Literal
                // dahinter wurde still als '-' digest-only gelockt; literal belegt).
                if (i > 0 && ist_hex_ziffer(text[i - 1]) && i + 1 < text.size() && ist_hex_ziffer(text[i + 1])) {
                    verbrauch_zeichen(c);
                    ++i;
                    continue;
                }
                z = Z::char_lit;
                verbrauch_zeichen(c);
                ++i;
                continue;
            }
            if (c == '#' && nur_ws_auf_zeile) {
                // G6b: Praeprozessor-Tiefe. Nur die Direktiven-ERKENNUNG ist neu; die Zeile wird
                // ansonsten wie bisher gescannt (minimaler Eingriff, golden-byte-neutral).
                std::size_t j = i + 1;
                while (j < text.size() && (text[j] == ' ' || text[j] == '\t')) ++j;
                std::size_t const wort_von = j;
                while (j < text.size() && text[j] >= 'a' && text[j] <= 'z') ++j;
                std::string_view const wort = text.substr(wort_von, j - wort_von);
                if (wort == "if" || wort == "ifdef" || wort == "ifndef") ++praep_tiefe;
                if (wort == "endif" && praep_tiefe > 0) --praep_tiefe;
                verbrauch_zeichen(c);
                ++i;
                continue;
            }
            if (c == 'a' && text.compare(i, kTok.size(), kTok) == 0) {
                bool const        grenze_vor  = (i == 0) || !ist_ident_zeichen(text[i - 1]);
                std::size_t const nach        = i + kTok.size();
                bool const        grenze_nach = (nach >= text.size()) || !ist_ident_zeichen(text[nach]);
                if (grenze_vor && grenze_nach) {
                    std::size_t j = nach;
                    while (j < text.size() && (text[j] == ' ' || text[j] == '\t' || text[j] == '\n' || text[j] == '\r'))
                        ++j;
                    if (j < text.size() && text[j] == '=') {
                        ++j;
                        while (j < text.size() &&
                               (text[j] == ' ' || text[j] == '\t' || text[j] == '\n' || text[j] == '\r'))
                            ++j;
                        if (j < text.size() && text[j] == '"') {
                            std::size_t const anfang = j + 1;
                            std::size_t       ende   = anfang;
                            while (ende < text.size() && text[ende] != '"' && text[ende] != '\n') ++ende;
                            if (ende < text.size() && text[ende] == '"') {
                                ergebnis.literale.emplace_back(text.substr(anfang, ende - anfang));
                                if (praep_tiefe > 0) ergebnis.unter_praeprozessor = true;
                                nur_ws_auf_zeile = false;
                                i                = ende + 1;
                                continue;
                            }
                        }
                    }
                }
                nur_ws_auf_zeile = false;
                i += kTok.size();
                continue;
            }
            verbrauch_zeichen(c);
            ++i;
        }
        return ergebnis;
    }

    /// KANONISCHE Register-Form der Literal-Liste (G3): leer => '-'; N gleiche => der Einzelwert
    /// (haelt die golden Baseline byte-stabil); ungleiche => komma-gefuegt in Text-Reihenfolge.
    /// Das Komma kommt in der X.Y.Z[.flags]-Grammatik nicht vor und bleibt EIN Lock-Token.
    [[nodiscard]] static std::string render_versionen(std::vector<std::string> const& literale) {
        if (literale.empty()) return "-";
        bool alle_gleich = true;
        for (std::string const& l : literale)
            if (l != literale.front()) alle_gleich = false;
        if (alle_gleich) return literale.front();
        std::string s;
        for (std::string const& l : literale) {
            if (!s.empty()) s += ',';
            s += l;
        }
        return s;
    }

    [[nodiscard]] static std::vector<std::string> split_versionen(std::string const& version) {
        std::vector<std::string> teile;
        if (version == "-") return teile;
        std::size_t von = 0;
        while (von <= version.size()) {
            std::size_t bis = version.find(',', von);
            if (bis == std::string::npos) bis = version.size();
            teile.push_back(version.substr(von, bis - von));
            von = bis + 1;
        }
        return teile;
    }

    /// bump_ok ueber die LISTE (G3, Regel s. Kopf): '-' hat keinen Bump-Pfad; Sentinel nie.
    [[nodiscard]] static bool bump_ok(std::string const& alt, std::string const& neu) {
        std::vector<std::string> const soll = split_versionen(alt);
        std::vector<std::string> const ist  = split_versionen(neu);
        if (soll.empty() || ist.empty()) return false;
        auto const cmp = [](std::string const& a, std::string const& b) -> int {
            meas::AlgoSemVer const va = meas::parse_algo_semver(a);
            meas::AlgoSemVer const vb = meas::parse_algo_semver(b);
            if (va.is_sentinel() || vb.is_sentinel()) return -2; // nie ein gueltiger Bump
            if (va.x != vb.x) return va.x < vb.x ? -1 : 1;
            if (va.y != vb.y) return va.y < vb.y ? -1 : 1;
            if (va.z != vb.z) return va.z < vb.z ? -1 : 1;
            return 0;
        };
        bool einer_groesser = false;
        if (soll.size() == 1) {
            // Einzelwert-Register = 'alle Literale standen auf v' -- aritaets-frei pruefbar.
            for (std::string const& e : ist) {
                int const r = cmp(soll.front(), e);
                if (r == -2 || r == 1) return false; // Sentinel oder Element kleiner als v
                if (r == -1) einer_groesser = true;
            }
            return einer_groesser;
        }
        if (soll.size() != ist.size()) return false; // Aritaetswechsel: kein Bump-Pfad (Regen)
        for (std::size_t k = 0; k < soll.size(); ++k) {
            int const r = cmp(soll[k], ist[k]);
            if (r == -2 || r == 1) return false;
            if (r == -1) einer_groesser = true;
        }
        return einer_groesser;
    }

    /// Sichtbare Sentinel-Diagnose (Koeder E), listen-bewusst: JEDES unparsbare Element rotiert.
    [[nodiscard]] static bool literal_unparsbar(std::string const& version) {
        for (std::string const& teil : split_versionen(version))
            if (meas::parse_algo_semver(teil).is_sentinel()) return true;
        return false;
    }
};

// ================================================================================================
// Lock-Datei v2: Lesen/Schreiben + der bidirektionale Check.
// ================================================================================================
struct Befund {
    std::string category;
    std::string version;
    std::string digest_hex;
    bool        praep_rot = false; // G6b: Literal unter #if/#ifdef/#ifndef -- nicht entscheidbar
};

using BefundMap = std::map<std::string, Befund>; // key = relativer Pfad (sortiert => deterministisch)

struct BestandZaehler {
    int heuristik_n     = 0;
    int organ_n         = 0;
    int organ_traeger_n = 0;
    int organ_multi_n   = 0; // G3: Dateien mit >= 2 Literalen (heute: 1, k_ary)
};

/// IMMER-ROT-Pruefung eines Befunds (G1(2)/G4/G6b), digest-unabhaengig -- gilt fuer --write UND
/// --check. true = gedruckt und rot.
[[nodiscard]] bool befund_immer_rot(std::string const& rel, Befund const& b) {
    if (b.category == HeuristikDetail::kName && HeuristikDetail::version_rot(b.version)) {
        std::fprintf(stderr, "axis_version_lock: ROT %s: %s\n", HeuristikDetail::rot_grund(b.version), rel.c_str());
        return true;
    }
    if (b.category == OrganDetail::kName) {
        if (b.praep_rot) {
            std::fprintf(stderr,
                         "axis_version_lock: ROT algo_version-Literal unter #if/#ifdef/#ifndef -- nicht "
                         "entscheidbar, Literal aus dem bedingten Block ziehen: %s\n",
                         rel.c_str());
            return true;
        }
        if (OrganDetail::literal_unparsbar(b.version)) {
            std::fprintf(stderr, "axis_version_lock: ROT algo_version-Literal unparsbar (Sentinel): '%s' %s\n",
                         b.version.c_str(), rel.c_str());
            return true;
        }
    }
    return false;
}

/// Erhebt die Grundgesamtheit beider Kategorien unter root. env_ok=false = Exit-2-Klasse.
[[nodiscard]] DiscoveryLage erhebe_bestand(fs::path const& root, BefundMap& out, BestandZaehler& z) {
    DiscoveryLage lage;

    std::vector<std::string> const h_rels = HeuristikDetail::discover(root, lage);
    for (std::string const& rel : h_rels) {
        std::vector<std::uint8_t> bytes;
        if (!read_file_bytes(root / rel, bytes)) {
            std::fprintf(stderr, "axis_version_lock: FEHLER Datei nicht lesbar: %s\n", rel.c_str());
            lage.env_ok = false;
            continue;
        }
        out[rel] =
            Befund{std::string(HeuristikDetail::kName), HeuristikDetail::version_of(bytes, rel), digest_hex_of(bytes)};
    }
    z.heuristik_n = static_cast<int>(h_rels.size());

    std::vector<std::string> const o_rels = OrganDetail::discover(root, lage);
    for (std::string const& rel : o_rels) {
        std::vector<std::uint8_t> bytes;
        if (!read_file_bytes(root / rel, bytes)) {
            std::fprintf(stderr, "axis_version_lock: FEHLER Datei nicht lesbar: %s\n", rel.c_str());
            lage.env_ok = false;
            continue;
        }
        OrganDetail::Scan const scan = OrganDetail::scan_literale(bytes);
        if (!scan.literale.empty()) ++z.organ_traeger_n;
        if (scan.literale.size() >= 2) ++z.organ_multi_n;
        Befund b{std::string(OrganDetail::kName), OrganDetail::render_versionen(scan.literale), digest_hex_of(bytes)};
        b.praep_rot = scan.unter_praeprozessor;
        out[rel]    = b;
    }
    z.organ_n = static_cast<int>(o_rels.size());
    return lage;
}

/// Die Riegel-Schaerfung (ii): discovered-Zaehler MIT NENNER, je Kategorie eine BESTAND-Zeile.
/// multi (G3) = Dateien mit mehreren Literalen -- gefahren, nicht behauptet.
void drucke_bestand(BestandZaehler const& z) {
    std::fprintf(stdout,
                 "axis_version_lock: BESTAND heuristik discovered=%d traeger=%d "
                 "(nenner: alle *.hpp unter libs/cache_engine/heuristik)\n",
                 z.heuristik_n, z.heuristik_n);
    std::fprintf(stdout,
                 "axis_version_lock: BESTAND organ discovered=%d traeger=%d forwarder_prosa=%d multi=%d "
                 "(nenner: *.hpp mit 'algo_version' unter libs/cache_engine/axes + "
                 "libs/cache_engine/topics/queuing)\n",
                 z.organ_n, z.organ_traeger_n, z.organ_n - z.organ_traeger_n, z.organ_multi_n);
}

/// Mindest-Nenner-Wache (G5a). true = verletzt (Meldung gedruckt, Exit-2-Klasse).
[[nodiscard]] bool nenner_verletzt(BestandZaehler const& z) {
    if (z.heuristik_n >= kMinHeuristikDiscovered && z.organ_n >= kMinOrganDiscovered) return false;
    std::fprintf(stderr,
                 "axis_version_lock: ROT Mindest-Nenner unterschritten: heuristik=%d (min %d), organ=%d "
                 "(min %d) -- leere/geschrumpfte Homes sind kein Gruen (V-1); nach einer bewussten "
                 "Umgliederung die Anker im Tool nachziehen\n",
                 z.heuristik_n, kMinHeuristikDiscovered, z.organ_n, kMinOrganDiscovered);
    return true;
}

int do_write(fs::path const& root, std::string const& lockfile) {
    BefundMap           bestand;
    BestandZaehler      z;
    DiscoveryLage const lage = erhebe_bestand(root, bestand, z);
    if (!lage.env_ok) return 2;
    if (nenner_verletzt(z)) {
        drucke_bestand(z);
        return 2;
    }

    // G1(3): ERST vollstaendig pruefen, DANN schreiben. Vorher stand das Truncate vor der
    // Pruefung: --write ueber ein Sentinel-Literal lieferte rc=1, schrieb das Lock aber
    // trotzdem -- einmal committet, war --check dauerhaft gruen (literal belegt).
    int rc = lage.politik_rot ? 1 : 0;
    for (auto const& [rel, b] : bestand)
        if (befund_immer_rot(rel, b)) rc = 1;
    if (rc != 0) {
        drucke_bestand(z);
        std::fprintf(stderr, "axis_version_lock: ROT --write verweigert -- Lock-Datei unveraendert; "
                             "Befunde oben reparieren\n");
        return rc;
    }

    std::ofstream out(lockfile, std::ios::trunc);
    if (!out) {
        std::fprintf(stderr, "axis_version_lock: FEHLER kann Lock-Datei nicht schreiben: %s\n", lockfile.c_str());
        return 2;
    }
    out << "# axis_version.lock -- content-digest tripwire ueber die algo_version-Traeger\n";
    out << "# (S-14a Riegel, Task #33/KON17-03; vormals PAKET W3-C GN-8/O-4)\n";
    out << "# format: v2\n";
    out << "# record (ZWEI Zeilen je Datei): '<category> <version> <relative-path>' + Folgezeile\n";
    out << "#   '    <sha256-hex>' (vier Leerzeichen Einrueckung).\n";
    out << "#   heuristik: version = Integer aus dem '// AXIS_ALGO_VERSION: <N>'-Marker (0 = kein Marker)\n";
    out << "#   organ:     version = erstes algo_version-String-Literal 'X.Y.Z[.flags]' im Code;\n";
    out << "#              '-' = kein Literal (Forwarder/Prosa; digest-only, Aenderung verlangt\n";
    out << "#              bewussten Lock-Regen-Commit via --write)\n";
    out << "# Formatwahl v2: Kategorie-Spalte + Versions-STRING statt v1-Integer-Spalte; v1-Locks\n";
    out << "# werden beim --check mit klarer Meldung abgewiesen (kein stilles Weiterlesen).\n";
    out << "# Zweizeilen-Record, damit jede Zeile <=120 Spalten bleibt (Diff-Hygiene-Wache voll\n";
    out << "# gedeckt, keine Ausnahme-Klasse); Kopfzeile ohne Digest-Folgezeile = lauter Formatfehler.\n";
    for (auto const& [rel, b] : bestand) {
        out << b.category << ' ' << b.version << ' ' << rel << '\n';
        out << "    " << b.digest_hex << '\n';
        std::fprintf(stdout, "axis_version_lock: LOCK %s %s %s %s\n", b.category.c_str(), b.version.c_str(),
                     b.digest_hex.c_str(), rel.c_str());
    }
    out.flush();
    if (!out) {
        std::fprintf(stderr, "axis_version_lock: FEHLER Schreiben der Lock-Datei fehlgeschlagen: %s\n",
                     lockfile.c_str());
        return 2;
    }
    drucke_bestand(z);
    return 0;
}

/// Parst die Lock-Datei v2. Verlangt die Formatzeile '# format: v2' VOR dem ersten Eintrag.
[[nodiscard]] bool parse_lock_v2(std::string const& path, BefundMap& out) {
    std::ifstream f(path);
    if (!f) {
        std::fprintf(stderr, "axis_version_lock: ROT Lock-Datei nicht lesbar: %s\n", path.c_str());
        return false;
    }
    bool        format_ok = false;
    bool        have_kopf = false;
    Befund      kopf;
    std::string kopf_rel;
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("# format:", 0) == 0) {
            if (line == "# format: v2") {
                format_ok = true;
                continue;
            }
            std::fprintf(stderr,
                         "axis_version_lock: ROT Lock-Format unbekannt/veraltet ('%s', erwartet '# format: v2') "
                         "-- mit --write regenerieren: %s\n",
                         line.c_str(), path.c_str());
            return false;
        }
        if (line.empty() || line[0] == '#') continue;
        if (!format_ok) {
            std::fprintf(stderr, "axis_version_lock: ROT Lock ohne '# format: v2'-Kopf vor dem ersten Eintrag: %s\n",
                         path.c_str());
            return false;
        }
        if (line[0] == ' ' || line[0] == '\t') {
            // Digest-Folgezeile des Zweizeilen-Records (vier Leerzeichen + 64 Hex). STRENG gelesen
            // (Pflicht-Fixup 13.08.2026, Koeder G2/G3): genau EIN Token, exakt 64 lowercase-Hex.
            // Vorher schluckte 'ds >> digest' Rest-Tokens still, und size()==64 liess Muellstrings
            // durch -- der Fehler erschien dann als irrefuehrende Drift-Meldung UEBER DIE DATEI
            // ('erwartet zzzz...'), nie ueber das Register.
            std::istringstream ds(line);
            std::string        digest;
            std::string        zusatz;
            ds >> digest;
            bool const hat_zusatz = static_cast<bool>(ds >> zusatz);
            if (!have_kopf || !ist_sha256_hex(digest) || hat_zusatz) {
                std::fprintf(stderr,
                             "axis_version_lock: ROT unparsbare Digest-Zeile im Lock (Record-Kopf %s; erwartet "
                             "genau 64 Hex-Zeichen [0-9a-f], ohne Zusatz): '%s'\n",
                             have_kopf ? kopf_rel.c_str() : "FEHLT", line.c_str());
                return false;
            }
            kopf.digest_hex = digest;
            out[kopf_rel]   = kopf;
            have_kopf       = false;
            continue;
        }
        if (have_kopf) {
            std::fprintf(stderr, "axis_version_lock: ROT Lock-Record ohne Digest-Folgezeile: %s\n", kopf_rel.c_str());
            return false;
        }
        std::istringstream ls(line);
        if (!(ls >> kopf.category >> kopf.version >> kopf_rel)) {
            std::fprintf(stderr, "axis_version_lock: ROT unparsbare Lock-Zeile: '%s'\n", line.c_str());
            return false;
        }
        // STRENG (Pflicht-Fixup 13.08.2026, Koeder G1): vorher wurde alles nach dem dritten Feld
        // still verschluckt -- ein Pfad MIT Leerzeichen wurde abgeschnitten und das Urteil der
        // Wache traf eine andere Datei. DEKLARIERTE GRENZE: Pfade mit Leerzeichen sind im Lock
        // nicht zugelassen (im Bestand existieren keine); jedes vierte Token ist ein Formatfehler.
        std::string zusatz;
        if (ls >> zusatz) {
            std::fprintf(stderr,
                         "axis_version_lock: ROT Lock-Kopfzeile mit Zusatz-Token '%s' (Pfade mit Leerzeichen "
                         "sind nicht zugelassen): '%s'\n",
                         zusatz.c_str(), line.c_str());
            return false;
        }
        // STRENG (Koeder G4): ein doppelter Record fuer denselben Pfad wuerde sonst still per
        // 'last wins' aufgeloest -- im Identitaets-Register ist das ein Formatfehler.
        if (out.find(kopf_rel) != out.end()) {
            std::fprintf(stderr, "axis_version_lock: ROT doppelter Lock-Record fuer Pfad: %s\n", kopf_rel.c_str());
            return false;
        }
        have_kopf = true;
    }
    if (have_kopf) {
        std::fprintf(stderr, "axis_version_lock: ROT Lock-Record ohne Digest-Folgezeile am Dateiende: %s\n",
                     kopf_rel.c_str());
        return false;
    }
    if (!format_ok) std::fprintf(stderr, "axis_version_lock: ROT Lock ohne '# format: v2'-Kopf: %s\n", path.c_str());
    return format_ok;
}

int do_check(fs::path const& root, std::string const& lockfile) {
    BefundMap lock;
    if (!parse_lock_v2(lockfile, lock)) return 1;

    BefundMap           bestand;
    BestandZaehler      z;
    DiscoveryLage const lage = erhebe_bestand(root, bestand, z);
    if (!lage.env_ok) return 2;
    if (nenner_verletzt(z)) {
        drucke_bestand(z);
        return 2;
    }

    int  red          = lage.politik_rot ? 1 : 0;
    bool regen_noetig = false;
    for (auto const& [rel, ist] : bestand) {
        // G1(2)/G4/G6b: unentscheidbare/unparsbare IST-Zustaende sind IMMER rot -- unabhaengig
        // vom Digest (vorher deckte Digest-Gleichheit ein committetes Sentinel-Register dauerhaft).
        if (befund_immer_rot(rel, ist)) {
            red = 1;
            continue;
        }
        auto const it = lock.find(rel);
        if (it == lock.end()) {
            std::fprintf(stderr, "axis_version_lock: ROT Datei nicht im Lock verzeichnet (unlocked): %s\n",
                         rel.c_str());
            red = 1;
            continue;
        }
        Befund const& soll = it->second;
        if (ist.category != soll.category) {
            std::fprintf(stderr, "axis_version_lock: ROT Kategorie im Register weicht ab (Lock '%s', Datei '%s'): %s\n",
                         soll.category.c_str(), ist.category.c_str(), rel.c_str());
            red = 1;
            continue;
        }
        if (ist.digest_hex == soll.digest_hex) {
            // G1(1): RUHELAGE PRUEFEN. ist.version ist ohnehin berechnet -- der Abgleich ist
            // gratis. Vorher stand hier ein blankes 'continue': ein inkonsistentes Register
            // (Handedit/Fehl-Merge der Kopfzeile bei erhaltener Digest-Zeile) blieb still gruen,
            // und der naechste echte Vergleich lief gegen die falsche Basis (literal belegt:
            // 1.0.2.c -> 1.9.0.c im Lock => GRUEN).
            if (ist.version != soll.version) {
                std::fprintf(stderr,
                             "axis_version_lock: ROT Version im Register weicht bei unveraendertem Inhalt ab "
                             "(Lock '%s', Datei '%s'): %s\n",
                             soll.version.c_str(), ist.version.c_str(), rel.c_str());
                std::fprintf(stderr, "axis_version_lock:     HINWEIS Register verfaelscht/fehl-gemergt? "
                                     "Bewussten Lock-Regen-Commit (--write) fahren\n");
                red = 1;
            }
            continue;
        }
        bool bump = false;
        if (ist.category == HeuristikDetail::kName)
            bump = HeuristikDetail::bump_ok(soll.version, ist.version);
        else
            bump = OrganDetail::bump_ok(soll.version, ist.version);
        if (bump) {
            // G2: der akzeptierte Bump ist KEIN Gruen mehr -- das Register ist veraltet, und nur
            // der Regen-Commit segnet den KONKRETEN Inhalt. Vorher blieb nach 'Bump ohne Regen'
            // jeder weitere Inhalts-Drift unter derselben Version dauerhaft gruen (literal
            // belegt). Eigener Exit-Code 3 (s. Kopf), damit CI/Tooling die Lage benennen kann.
            std::fprintf(stdout, "axis_version_lock: OK Digest geaendert MIT Version-Bump %s -> %s %s\n",
                         soll.version.c_str(), ist.version.c_str(), rel.c_str());
            std::fprintf(stdout, "axis_version_lock: HINWEIS Lock erneuern (--write) fuer %s\n", rel.c_str());
            regen_noetig = true;
            continue;
        }
        std::fprintf(stderr, "axis_version_lock: ROT Digest geaendert OHNE gueltigen Version-Bump (%s -> %s) %s\n",
                     soll.version.c_str(), ist.version.c_str(), rel.c_str());
        std::fprintf(stderr, "axis_version_lock:     erwartet %s\n", soll.digest_hex.c_str());
        std::fprintf(stderr, "axis_version_lock:     ist      %s\n", ist.digest_hex.c_str());
        std::fprintf(stderr, "axis_version_lock:     HINWEIS Version bumpen UND bewussten Lock-Regen-Commit "
                             "(--write) fahren\n");
        red = 1;
    }
    // Riegel-Schaerfung (i), schliesst Koeder D: jeder Lock-Eintrag braucht seine Datei im Bestand.
    for (auto const& [rel, soll] : lock) {
        if (bestand.find(rel) != bestand.end()) continue;
        std::fprintf(stderr, "axis_version_lock: ROT Lock-Eintrag ohne Datei (verwaist): %s\n", rel.c_str());
        std::fprintf(stderr, "axis_version_lock:     HINWEIS Datei geloescht/umgezogen/ohne 'algo_version'? "
                             "Bewussten Lock-Regen-Commit (--write) fahren\n");
        red = 1;
    }
    drucke_bestand(z);
    if (red != 0) return 1;
    if (regen_noetig) {
        std::fprintf(stderr, "axis_version_lock: REGEN ERFORDERLICH -- Version-Bump akzeptiert, aber das "
                             "Register ist veraltet; Lock-Regen-Commit (--write) fahren (Exit 3)\n");
        return 3;
    }
    std::fprintf(stdout, "axis_version_lock: GRUEN bestand konsistent -- %zu Dateien (heuristik=%d, organ=%d)\n",
                 bestand.size(), z.heuristik_n, z.organ_n);
    return 0;
}

void usage() {
    std::fprintf(stderr, "usage: axis_version_lock (--write|--check) <lockfile> [--root <repo-root>]\n");
    std::fprintf(stderr, "       (v2: die Grundgesamtheit erhebt das Tool selbst unter --root; die\n");
    std::fprintf(stderr, "        v1-Form mit expliziter Header-Liste ist entfallen)\n");
    std::fprintf(stderr, "       Exit: 0 gruen, 1 ROT, 2 Umgebung/Usage, 3 (--check) Regen erforderlich\n");
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3 && argc != 5) {
        usage();
        return 2;
    }
    std::string const mode     = argv[1];
    std::string const lockfile = argv[2];
    fs::path          root     = ".";
    if (argc == 5) {
        if (std::string_view(argv[3]) != "--root") {
            usage();
            return 2;
        }
        root = argv[4];
    }
    if (mode == "--write") return do_write(root, lockfile);
    if (mode == "--check") return do_check(root, lockfile);
    usage();
    return 2;
}
