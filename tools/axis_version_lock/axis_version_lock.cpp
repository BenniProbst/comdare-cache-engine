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
//     OrganDetail die Literal-Mechanik (organ_axes/ inkl. der per #72 eingezogenen queuing-Traeger
//     + topics/-Andock-Verzeichnisse). EINE Wache, EIN Lock
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
//   (G3) ALLE LITERALE EINER DATEI, NICHT DAS ERSTE. organ_axes/lookup/axis_03a_search_algo_k_ary.hpp
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
// PFLICHT-FIXUP 3 (13.08.2026, Owner-Dauerregel; der KERN-DEFEKT, am Objekt gemessen und rot-zuerst
// gefahren, nie vermutet):
//   (D1) DIE GRUNDGESAMTHEIT WAR DIE FALSCHE MENGE. Gemessen 13.08.2026: 387 *.hpp unter den
//        organ-Homes, 152 im Lock, 235 (60 Prozent) UNBEWACHT bei 'GRUEN bestand konsistent'.
//        Ursache war der Substring-Filter text.find("algo_version") in OrganDetail::discover --
//        er machte die Grundgesamtheit zur Teilmenge derer, die zufaellig ein Literal ZITIEREN.
//        Massgeblich ist aber, was der Overlay-Hash sieht: das Overlay-Glied [7] des Tier-
//        Fingerprints (builder/overlay_source_set.hpp) hasht ALLE Quell-Dateien seines Schnitts;
//        jede unbewachte Datei konnte den Fingerprint verschieben, ohne dass die Bump-Wache es
//        merkt ('Aenderung ohne Bump => kein Neubau => schneller UND falsch'; der Stempel ist
//        Cache- UND Lager-Schluessel). JETZT erhebt die Wache ihre Overlay-Grundgesamtheit AUS
//        DEM SCHNITT-HEADER SELBST (EIN Schnitt, keine Zweitliste): kOverlaySourceSet +
//        kQuellEndungen/kNichtQuellEndungen, Ist 13.08.2026: 712 Quell-Dateien (organ 640,
//        system 16, mess 1, tier_substanz 55) + heuristik 6 = 718 Records. Dateien ohne Literal
//        stehen digest-only ('-') im Lock; der Riegel faengt sie ueber Digest-Drift.
//        DIFFERENZ BEIDER MENGEN, BENANNT: 26 *.hpp der alten Homes stehen NICHT im Schnitt
//        (organ_axes/telemetry_axis 10: telemetry ist CEB-System-Achse geworden; organ_axes/simd 10: isa ->
//        target_isa, simd_extension ist Build-only-Achse in Glied [6]; organ_axes/cacheline 3 +
//        organ_axes/axis_centric_namespaces.hpp + topics/queuing-Topic-Huelle 2: keine Achsen-
//        Implementierung im Sinne des Schnitts). 0 der 26 waren gelockt, 0 enthalten
//        'algo_version' (gemessen) -- die Wache folgt dem Schnitt, nicht umgekehrt.
//   (D2) TRAEGER-AUSTRITT IST ROT. Rot-zuerst literal: Literal entfernt und --write gefahren =>
//        Exit 0, der Record verschwand ganz (keine 'algo_version'-Erwaehnung mehr => aus der
//        alten Discovery gefallen) bzw. wurde still 'organ -' (Erwaehnung blieb). Die VERSION
//        einer Datei verliess das Register auf einem vom Werkzeug ANGEWIESENEN Weg. JETZT:
//        Rueckstufung traeger -> digest-only ist ROT (Exit 1, benannt mit Datei und
//        Register-Version) in --check UND --write; --write verweigert byte-identisch. Ein
//        bewusster Austritt geht nur ueber Loeschen/Umbenennen der Datei (verwaist -> Regen).
//   (D3) AUFWERTUNG IST EXIT 3, NICHT EXIT 1. Rot-zuerst literal: digest-only-Datei bekam ein
//        gueltiges Literal => 'ROT Digest geaendert OHNE gueltigen Version-Bump (- -> 1.0.0.c)'.
//        Das ist eine legitime Aenderung: JETZT 'OK Aufwertung digest-only -> traeger' =>
//        REGEN ERFORDERLICH (Exit 3), der Regen-Commit segnet den Inhalt.
//   (D4) DECKUNGS-BELEG STATT SUGGESTION: die GRUEN-Zeile nennt die gedeckte Menge GEGEN die
//        Overlay-Menge ('deckt N von N Overlay-Quellen'); eine Deckungsluecke ist ROT (doppeltes
//        Netz neben dem unlocked-Pfad, nie Prosa). Die Kategorie-Spalte wird beim --check
//        GEPRUEFT (Kategorie-Abweichung = ROT; am Objekt belegt, ctest-Koeder T5) und traegt
//        jetzt die Overlay-Kategorien organ/system/mess/tier_substanz + heuristik.
//   (D5) MINDEST-NENNER MITGEZOGEN (Fixup-2-D4-Form, s. Konstanten unten): die alte Schwelle
//        organ>=120 haette unter dem neuen Nenner 712 eine Schrumpfung auf ein Sechstel als
//        gruen durchgelassen.
//   (D6) LOCK-FORMAT v3 -- PFAD-SPLIT WEGEN DER 120-SPALTEN-DIFF-HYGIENE. Am Objekt gemessen:
//        die neue Grundgesamtheit traegt Record-Koepfe bis 146 Byte (laengster Pfad 138 Byte:
//        topics/search_engine/axis_01_index_organization/concepts/axis_01_index_organization_
//        cache_engine_permutation_concept.hpp) -- das v2-Zweizeilen-Format konnte sein eigenes
//        Versprechen 'jede Zeile <= 120' strukturell nicht mehr halten, und eine Ausnahme-
//        Klasse in der Diff-Hygiene-Wache ist per Fixup-2-Entscheid ausgeschlossen. v3 haelt
//        die Records dreizeilig (s. LOCK-FORMAT unten); laengste Zeile heute 106 Byte.
//
// KATEGORIEN UND IHRE REGELN:
//   heuristik  Home libs/cache_engine/heuristik/, jede *.hpp ist Traeger. Version = Integer aus
//              der DEDIZIERTEN Marker-Zeile '// AXIS_ALGO_VERSION: <N>' (zeilen-verankert: vor
//              dem '//' nur Weissraum, nach der Zahl nur Weissraum). 0 Marker, >1 Marker oder
//              unparsbarer Marker (keine Ziffer, Ueberlauf, Restzeichen) => ROT. bump_ok =
//              Integer echt groesser (streng geparst, kein strtoull-Clamp).
//   organ, system, mess, tier_substanz (D1): die Quell-Dateien des Overlay-Schnitts, Kategorie je
//              Schnitt-Eintrag (builder/overlay_source_set.hpp). DISCOVERED ist jede regulaere
//              Datei mit Endung aus kQuellEndungen unter einem verzeichnis-Eintrag (rekursiv)
//              bzw. mit Praefix-Treffer eines datei_praefix-Eintrags (flach). Endungen aus
//              kNichtQuellEndungen (Doku/Registry-Spiegel) sind BEWUSST draussen -- der
//              Fingerprint sieht sie nicht; eine Endung in KEINER der beiden Listen ist ROT
//              (dieselbe Entscheidung, die der Codegen fail-loud erzwingt). Symlinks unter einem
//              Schnitt-Pfad sind Exit 2: der Codegen ueberspringt sie (keine Zweitzaehlung),
//              ihr Inhalt ist dem Fingerprint also UNSICHTBAR -- die Wache zertifiziert keinen
//              Baum, dessen Teile am Hash vorbeilaufen (Bestand: 0 Symlinks, gemessen).
//              TRAEGER ist jede discovered-Datei mit mindestens einer echten algo_version-
//              String-Literal-Zuweisung IM CODE (kommentar-/string-bewusster Scan mit
//              Wortgrenzen; Forwarder wie '= Strategy::algo_version' und Prosa fallen heraus
//              und stehen als version='-' digest-only im Lock).
//              VERSION = geordnete LISTE ALLER Literale (G3). KANONISCHE FORM im Register:
//              N gleiche Literale => der Einzelwert (k_ary: 2x '1.0.0.c' => '1.0.0.c');
//              ungleiche => komma-gefuegt in Text-Reihenfolge ('1.1.0.c,1.0.0.c'). bump_ok
//              ueber die Liste: gegen ein EINZELWERT-Register darf kein Ist-Literal kleiner
//              sein und mindestens eines muss echt groesser sein (aritaets-frei -- das Register
//              sagt 'alle standen auf v'); gegen ein LISTEN-Register gilt gleiche Aritaet +
//              elementweise nie kleiner, mindestens einmal echt groesser. Aritaetswechsel gegen
//              ein Listen-Register hat KEINEN Bump-Pfad (bewusster Regen-Commit). Grammatik
//              ueber den BESTANDS-Parser measurement/algo_semver.hpp (EIN Parser, keine
//              Zweitgrammatik); unparsbares Literal ist Sentinel => ROT.
//              KATEGORIE-REGELN (D2/D3/D4): Kategorie-Abweichung Lock vs. Discovery => ROT;
//              traeger -> digest-only => ROT ohne --write-Weg; digest-only -> traeger =>
//              Exit 3 (Regen).
//              S-18/#16 DETAIL-SPLIT (KON27-01, 15.08.2026): SystemDetail/MessDetail tragen
//              Home + Phasigkeits-Pruefsyntax (system+organ ZWEIPHASIG hardware-only via
//              ce_owned_version_is_wellformed; mess DREIPHASIG = benannte Leerstelle bis zum
//              G-1-Stufe-C-Bau, bis dahin Parser-Pruefung). Homes: system_axes/ + mess_axes/
//              (organ: organ_axes/+topics/, compile-hart gepinnt im Schnitt-Header).
//
// LOCK-FORMAT v3 (v1 UND v2 werden mit klarer Meldung abgewiesen, kein stilles Weiterlesen):
//   # format: v3
//   DREIZEILEN-RECORD je Datei (D6):
//     '<category> <version> <dirname/>'   Kopfzeile; dirname repo-relativ MIT Schluss-'/'
//     '    <basename>'                    Datei-Name (vier Leerzeichen Einrueckung)
//     '    <sha256-hex>'                  Digest (vier Leerzeichen Einrueckung)
//   Der volle Pfad ist EXAKT dirname+basename -- derselbe repo-relative Pfad, den v2 einzeilig
//   trug und den alle Meldungen weiterhin nennen. WARUM DREI ZEILEN: die 120-Spalten-Diff-
//   Hygiene (ci_diff_ascii_width_guard) deckt JEDE Zeile dieser Datei; Pfade bis 138 Byte
//   passen in keine Einzelzeile mit Praefix (D6). Records global nach vollem Pfad sortiert;
//   version bei den Overlay-Kategorien = kanonische Literal-Form (s.o.) bzw. '-'. Eine
//   Kopfzeile ohne BEIDE Folgezeilen ist ein LAUTER Formatfehler (kein Ueberlesen). Der Parser
//   liest STRENG (Pflicht-Fixup 13.08.2026, seit v3 dreizeilig): kein viertes Token auf der
//   Kopfzeile (Pfade mit Leerzeichen sind nicht zugelassen -- deklarierte Grenze), dirname
//   endet auf '/', basename = genau EIN Token ohne '/', Digest = genau 64 lowercase-Hex-
//   Zeichen ohne Zusatz, kein doppelter Record je Pfad. Jede Verletzung nennt die literale
//   Zeile bzw. den Pfad und macht ROT.
//   LOCK-KOPF: Fixup 2 hatte den Kopftext bewusst auf der v2-Erstform eingefroren, weil die
//   158er-Baseline byte-identisch regenerierbar bleiben musste, und den Nachzug auf den
//   naechsten LEGITIMEN Lock-Regen vertagt. DAS war dieser Regen (Fixup 3, D1/D6): der Kopf
//   beschreibt seither die geltende Semantik (Listen-Regel, Overlay-Grundgesamtheit,
//   Kategorie-Regeln, v3-Split); CI erzwingt weiterhin --write + git diff --exit-code.
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

// D1: DIE EINE QUELLE der Overlay-Grundgesamtheit -- kOverlaySourceSet + kQuellEndungen +
// kNichtQuellEndungen. Derselbe Schnitt-Header, aus dem der Fingerprint-Codegen
// (tools/overlay_source_hash_gen) seine Dateimenge liest: Wache und Hash koennen nicht mehr
// auseinanderlaufen, ohne dass es hier bricht. Zieht via axis_path_serialization.hpp Boost::mp11
// (Header-only) -- weiterhin KEINE Registry-/Varianten-TU (die bleibt dem ctest vorbehalten).
#include <builder/overlay_source_set.hpp>

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
namespace ovl  = ::comdare::cache_engine::builder::overlay;

// MINDEST-NENNER (G5a, Hausvertrag V-1 'Nenner 0 = Exit != 0, nie GRUEN'): unterschreitet die
// Grundgesamtheit diese Anker, ist der Lauf ein Umgebungsfehler (Exit 2) -- eine Wache ueber
// einer leeren Menge behauptet sonst Konsistenz, die sie nie gemessen hat. Die Werte spiegeln
// die ctest-Anker. #16-UMGLIEDERUNG VOLLZOGEN (15.08.2026, S-18/KON27-01): die system-/mess-
// Familien wohnen seither in den Kategorie-Homes libs/cache_engine/system_axes/ (16 Dateien)
// und libs/cache_engine/mess_axes/ (1 Datei) statt flach in include/cache_engine/measurement/;
// der Schnitt (builder/overlay_source_set.hpp) traegt die neuen Pfade und pinnt die Homes
// compile-hart (kategorie_haelt_ihr_home). Die ANKER-WERTE bleiben BEWUSST unveraendert --
// der Umzug bewegt exakt die 17 verzeichneten Dateien, die Ist-Zahlen (heuristik 6, organ 640,
// system 16, mess 1, tier_substanz 55; gemessen 13.08., am 15.08. unveraendert) halten die
// Schwellen weiter; ein Struktur-Verlust (Home leer/umgezogen ohne Schnitt-Nachzug) schlaegt
// weiterhin IMMER hier bzw. am Schnitt-Pfad-Fehler auf.
// Die exakten Ist-Zahlen pinnt der ctest (Anker organ-traeger==123 inkl. Synthetik usw.).
inline constexpr int kMinHeuristikDiscovered = 6;
inline constexpr int kMinOrganDiscovered     = 600; // Ist 640 (13.08.2026)
inline constexpr int kMinSystemDiscovered    = 12;  // Ist 16 (13.08.2026)
inline constexpr int kMinMessDiscovered      = 1;   // Ist 1 (13.08.2026): measurement_tooling-Familie
inline constexpr int kMinTierDiscovered      = 50;  // Ist 55 (13.08.2026): anatomy/

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

/// Eintrittspruefung der HEURISTIK-discover()-Schleife (G5d): Verzeichnis-Symlinks werden vom
/// recursive_directory_iterator NICHT deszendiert und kaputte Symlinks fallen durch
/// is_regular_file -- beides verschwand vorher STILL aus der Grundgesamtheit (literal belegt:
/// Traeger hinter Dir-Symlink => GRUEN ohne ihn). Datei-Symlinks auf existierende Ziele bleiben
/// im heuristik-Home wie bisher Traeger (Digest ueber die Ziel-Bytes). Die Overlay-Discovery
/// (D1) hat ihre EIGENE, strengere Symlink-Regel: dort ist JEDER Symlink Exit 2, weil der
/// Fingerprint-Codegen Symlinks ueberspringt und ihr Inhalt dem Hash unsichtbar waere.
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
// KATEGORIE-DETAIL: die Overlay-Kategorien (organ/system/mess/tier_substanz) -- Literal-Mechanik,
// Grundgesamtheit = der Overlay-Schnitt (D1). Der Klassen-Name bleibt OrganDetail: organ ist die
// tragende Kategorie (640 von 712 Dateien, alle heutigen Traeger), und die Literal-Mechanik ist
// fuer alle vier Kategorien DIESELBE -- nur die Kategorie-Spalte im Record unterscheidet sie.
// ================================================================================================

/// Kategorie-Text eines Schnitt-Eintrags -- exakt das Vokabular des Codegen-Manifests
/// (tools/overlay_source_hash_gen, kategorie_text): EIN Vokabular fuer Manifest und Lock.
[[nodiscard]] std::string_view kategorie_text(ovl::Kategorie k) {
    switch (k) {
        case ovl::Kategorie::organ: return "organ";
        case ovl::Kategorie::system: return "system";
        case ovl::Kategorie::mess: return "mess";
        case ovl::Kategorie::tier_substanz: return "tier_substanz";
    }
    return "?";
}

[[nodiscard]] bool ist_quell_endung(std::string const& endung) {
    for (auto const& e : ovl::kQuellEndungen)
        if (endung == e) return true;
    return false;
}

[[nodiscard]] bool ist_nicht_quell_endung(std::string const& endung) {
    for (auto const& e : ovl::kNichtQuellEndungen)
        if (endung == e) return true;
    return false;
}

struct OrganDetail {
    static constexpr std::string_view kName = "organ";
    static_assert(!kName.empty()); // B16: Vertrags-Pin (schliesst clang -Wunused-const-variable)

    /// Ergebnis des Literal-Scans einer Datei: alle Literale in Text-Reihenfolge + die
    /// Praeprozessor-Diagnose (G6b).
    struct Scan {
        std::vector<std::string> literale;
        bool                     unter_praeprozessor = false;
    };

    /// Ein Fund der Overlay-Discovery: relativer Pfad (zu --root) + Kategorie-Text des
    /// Schnitt-Eintrags, der ihn aufgenommen hat.
    struct Fund {
        std::string rel;
        std::string kategorie;
    };

    /// D1: discovered = die Quell-Dateien des Overlay-Schnitts, Eintrag fuer Eintrag aus
    /// kOverlaySourceSet erhoben (verzeichnis-Eintraege rekursiv, datei_praefix-Eintraege flach;
    /// Endungs-Entscheid ueber kQuellEndungen/kNichtQuellEndungen). Der fruehere Substring-
    /// Filter text.find("algo_version") ist ERSATZLOS GEFALLEN -- er war die Ursache der
    /// 235-Dateien-Deckungsluecke (60 Prozent der Homes unbewacht bei gruener Wache).
    /// FAIL-CLOSED, jede Klasse benannt:
    ///   * Schnitt-Pfad fehlt / Discovery-Abbruch / Praefix ohne Treffer => Exit-2-Klasse
    ///     (dieselben Klassen bricht der Codegen fail-loud -- eine Umbenennung darf den Schnitt
    ///     nicht still verkleinern).
    ///   * Symlink unter einem Schnitt-Pfad => Exit-2-Klasse: der Codegen ueberspringt Symlinks
    ///     (keine Zweitzaehlung), ihr Inhalt laeuft also am Fingerprint VORBEI -- die Wache
    ///     zertifiziert keinen Baum mit hash-unsichtbaren Teilen (Bestand: 0, gemessen).
    ///   * Sonderdatei (FIFO/Socket/Device) => Exit-2-Klasse (der Codegen ueberspraenge sie still).
    ///   * Endung in KEINER der beiden Listen => Politik-ROT: eine neue Endung muss die
    ///     Entscheidung 'Identitaet oder nicht' erzwingen (Spiegel der Codegen-Regel).
    ///   * Schnitt-Ueberlappung (dieselbe Datei aus zwei Eintraegen) => Exit-2-Klasse.
    [[nodiscard]] static std::vector<Fund> discover(fs::path const& root, DiscoveryLage& lage) {
        std::map<std::string, std::string> funde; // rel -> kategorie; sortiert => deterministisch
        fs::path const                     ce = root / "libs/cache_engine";
        for (auto const& e : ovl::kOverlaySourceSet) {
            fs::path const basis = ce / fs::path{std::string{e.pfad}};
            if (!fs::is_directory(basis)) {
                std::fprintf(stderr,
                             "axis_version_lock: FEHLER Schnitt-Pfad fehlt unter --root (Achse '%s'): "
                             "libs/cache_engine/%s\n",
                             std::string{e.achse}.c_str(), std::string{e.pfad}.c_str());
                lage.env_ok = false;
                continue;
            }
            // true = als Quell-Datei aufgenommen (Zaehler fuer die Praefix-Treffer-Wache).
            auto const klassifiziere = [&](fs::directory_entry const& de) -> bool {
                std::string const rel = de.path().lexically_relative(root).generic_string();
                if (de.is_symlink()) {
                    std::fprintf(stderr,
                                 "axis_version_lock: FEHLER Symlink unter einem Schnitt-Pfad (dem "
                                 "Overlay-Hash unsichtbar, nicht zugelassen): %s\n",
                                 rel.c_str());
                    lage.env_ok = false;
                    return false;
                }
                if (de.is_directory()) return false;
                if (!de.is_regular_file()) {
                    std::fprintf(stderr,
                                 "axis_version_lock: FEHLER keine regulaere Datei unter einem Schnitt-Pfad "
                                 "(FIFO/Socket/Device, nicht zugelassen): %s\n",
                                 rel.c_str());
                    lage.env_ok = false;
                    return false;
                }
                std::string const endung = de.path().extension().string();
                if (ist_quell_endung(endung)) {
                    auto const [it, neu] = funde.emplace(rel, std::string(kategorie_text(e.kategorie)));
                    if (!neu) {
                        std::fprintf(stderr,
                                     "axis_version_lock: FEHLER Schnitt-Ueberlappung -- Datei aus zwei "
                                     "Eintraegen des Overlay-Schnitts: %s\n",
                                     rel.c_str());
                        lage.env_ok = false;
                    }
                    return true;
                }
                if (ist_nicht_quell_endung(endung)) return false; // Doku/Spiegel: bewusst ausserhalb
                std::fprintf(stderr,
                             "axis_version_lock: ROT Endung '%s' steht in KEINER Endungs-Liste des "
                             "Overlay-Schnitts (Entscheidung erzwungen, s. overlay_source_set.hpp): %s\n",
                             endung.c_str(), rel.c_str());
                lage.politik_rot = true;
                return false;
            };
            try {
                if (e.form == ovl::Form::verzeichnis) {
                    for (auto const& de : fs::recursive_directory_iterator(basis)) (void)klassifiziere(de);
                } else {
                    std::string const praefix{e.praefix};
                    int               treffer = 0;
                    for (auto const& de : fs::directory_iterator(basis)) {
                        std::string const name = de.path().filename().string();
                        if (name.rfind(praefix, 0) != 0) continue;
                        if (klassifiziere(de)) ++treffer;
                    }
                    if (treffer == 0) {
                        std::fprintf(stderr,
                                     "axis_version_lock: FEHLER Datei-Praefix '%s' (Achse '%s') trifft keine "
                                     "Quelldatei unter libs/cache_engine/%s -- Schnitt nachziehen, nicht "
                                     "ignorieren\n",
                                     praefix.c_str(), std::string{e.achse}.c_str(), std::string{e.pfad}.c_str());
                        lage.env_ok = false;
                    }
                }
            } catch (fs::filesystem_error const& err) {
                std::fprintf(stderr, "axis_version_lock: FEHLER Overlay-Discovery abgebrochen (%s)\n", err.what());
                lage.env_ok = false;
            }
        }
        std::vector<Fund> aus;
        aus.reserve(funde.size());
        for (auto const& [rel, kat] : funde) aus.push_back(Fund{rel, kat});
        return aus;
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
// DETAIL-KLASSEN-SPLIT S-18/#16 (KON27-01 'je Achsen-Kategorie EIN Home + EIN Waechter' als
// Detail-Klassen IN DER EINEN Wache; KON17-03 'EINE Wache, EIN Lock' -- kein zweites Werkzeug,
// kein zweites Lockfile). SystemDetail/MessDetail tragen Home und PRUEF-SYNTAX ihrer Kategorie;
// die Literal-MECHANIK (Scan/Render/Bump) bleibt die EINE in OrganDetail -- der Split traegt
// POLITIK, keine Zweitmechanik, und die Grammatik bleibt der EINE Bestands-Parser
// measurement/algo_semver.hpp (NIE Zweitgrammatik).
//   ZWEIPHASIG (system UND organ, KON27-01): jedes algo_version-Literal muss die volle
//     Hardware-Wache ce_owned_version_is_wellformed tragen (Katalog + c-Pflichtform +
//     Voraussetzungs-Ketten). Sie lehnt insbesondere Mess-Vokabular ('m'-Baum) auf System-/
//     Organ-Zeilen ab -- exakt die G-1-Abgrenzung ('das generische Praedikat bleibt
//     hardware-only').
//   DREIPHASIG (mess): BENANNTE LEERSTELLE, kein stiller Default (Praezedenz KON58-01): die
//     G-1-Form-Wachen (mess_form_ist_dreiphasig / mess_version_is_wellformed / Katalog
//     kMessGrammarCatalog im Mess-Home) sind per G-1-Design Par. 6 Stufe B/C ein EIGENER,
//     owner-gestufter Bau NACH diesem Fenster (#17/G-2 folgt). Bis dahin prueft mess ueber den
//     EINEN Parser auf Parsebarkeit (Sentinel = ROT, bestehender Weg); der Hardware-Katalog
//     wird auf mess BEWUSST NICHT erzwungen -- die Mess-Grammatik ist ein v2-PROFIL MIT
//     m-Vokabular, das der Hardware-Katalog nicht kennt.
//   tier_substanz: KEIN Achsen-Eigentum (anatomy/ = gemeinsamer Traeger) -- keine
//     Phasigkeits-Syntax; heuristik behaelt den Integer-Marker-Weg.
// Die HOME-ZUGEHOERIGKEIT der Discovery pinnt der Schnitt-Header compile-hart
// (builder/overlay_source_set.hpp, kategorie_haelt_ihr_home); die bidirektionale
// Lock-Deckung (unlocked/verwaist) besteht seit v2. Hier steht die LAUFZEIT-Politik je Literal.
struct SystemDetail {
    static constexpr std::string_view kName = "system";
    static constexpr std::string_view kHome = "libs/cache_engine/system_axes";
    static_assert(!kHome.empty()); // B16: Vertrags-Pin (schliesst clang -Wunused-const-variable)
    /// ZWEIPHASIG: volle Hardware-Wache je Literal (EIN Katalog, EIN Parser).
    [[nodiscard]] static bool literal_zulaessig(std::string const& literal) {
        return meas::ce_owned_version_is_wellformed(literal);
    }
};

struct MessDetail {
    static constexpr std::string_view kName = "mess";
    static constexpr std::string_view kHome = "libs/cache_engine/mess_axes";
    static_assert(!kHome.empty()); // B16: Vertrags-Pin (schliesst clang -Wunused-const-variable)
    /// DREIPHASIG: heute NUR Parsebarkeit ueber den EINEN Parser (Leerstelle s. Block-Kopf);
    /// der G-1-Bau ersetzt diesen Rumpf durch mess_version_is_wellformed (Stufe C).
    [[nodiscard]] static bool literal_zulaessig(std::string const& literal) {
        return !meas::parse_algo_semver(literal).is_sentinel();
    }
};

/// Phasigkeits-Politik je Kategorie-Literal (S-18). true = gedruckt und rot. Laeuft NACH der
/// Sentinel-Wache (befund_immer_rot-Reihenfolge): ein unparsbares Literal ist dort schon rot,
/// hier faellt die KATALOG-/PROFIL-Frage der wohlgeformten Literale.
[[nodiscard]] bool phasigkeit_rot(std::string const& rel, std::string const& kategorie, std::string const& version) {
    if (version == "-") return false;
    bool        rot  = false;
    char const* form = nullptr;
    for (std::string const& teil : OrganDetail::split_versionen(version)) {
        if (kategorie == std::string(SystemDetail::kName) || kategorie == "organ") {
            if (!SystemDetail::literal_zulaessig(teil)) {
                form = "ZWEIPHASIG hardware-only, ce_owned_version_is_wellformed";
                rot  = true;
            }
        } else if (kategorie == std::string(MessDetail::kName)) {
            if (!MessDetail::literal_zulaessig(teil)) {
                form = "DREIPHASIG (heute: Parsebarkeit, G-1-Formwache folgt)";
                rot  = true;
            }
        }
        if (rot) {
            std::fprintf(stderr,
                         "axis_version_lock: ROT Phasigkeits-Syntax verletzt (Kategorie '%s', %s): "
                         "Literal '%s' %s\n",
                         kategorie.c_str(), form, teil.c_str(), rel.c_str());
            return true;
        }
    }
    return false;
}

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

/// Zaehler EINER Overlay-Kategorie (D1): discovered mit traeger/multi-Aufriss.
struct KatZaehler {
    int discovered = 0;
    int traeger    = 0;
    int multi      = 0; // G3: Dateien mit >= 2 Literalen (heute: 1, k_ary in organ)
};

struct BestandZaehler {
    int        heuristik_n = 0;
    KatZaehler organ;
    KatZaehler system;
    KatZaehler mess;
    KatZaehler tier;

    [[nodiscard]] KatZaehler& fuer(std::string const& kategorie) {
        if (kategorie == "system") return system;
        if (kategorie == "mess") return mess;
        if (kategorie == "tier_substanz") return tier;
        return organ;
    }
    [[nodiscard]] int overlay_gesamt() const {
        return organ.discovered + system.discovered + mess.discovered + tier.discovered;
    }
    [[nodiscard]] int overlay_traeger() const { return organ.traeger + system.traeger + mess.traeger + tier.traeger; }
};

/// true = b traegt eine der vier Overlay-Kategorien (Literal-Mechanik); false = heuristik.
[[nodiscard]] bool ist_overlay_kategorie(std::string const& category) {
    return category != std::string(HeuristikDetail::kName);
}

/// IMMER-ROT-Pruefung eines Befunds (G1(2)/G4/G6b), digest-unabhaengig -- gilt fuer --write UND
/// --check. true = gedruckt und rot.
[[nodiscard]] bool befund_immer_rot(std::string const& rel, Befund const& b) {
    if (b.category == HeuristikDetail::kName && HeuristikDetail::version_rot(b.version)) {
        std::fprintf(stderr, "axis_version_lock: ROT %s: %s\n", HeuristikDetail::rot_grund(b.version), rel.c_str());
        return true;
    }
    if (ist_overlay_kategorie(b.category)) {
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
        if (phasigkeit_rot(rel, b.category, b.version)) return true; // S-18 Detail-Split (s.o.)
    }
    return false;
}

/// Erhebt die Grundgesamtheit aller Kategorien unter root. env_ok=false = Exit-2-Klasse.
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

    for (OrganDetail::Fund const& fund : OrganDetail::discover(root, lage)) {
        std::vector<std::uint8_t> bytes;
        if (!read_file_bytes(root / fund.rel, bytes)) {
            std::fprintf(stderr, "axis_version_lock: FEHLER Datei nicht lesbar: %s\n", fund.rel.c_str());
            lage.env_ok = false;
            continue;
        }
        OrganDetail::Scan const scan = OrganDetail::scan_literale(bytes);
        KatZaehler&             kz   = z.fuer(fund.kategorie);
        ++kz.discovered;
        if (!scan.literale.empty()) ++kz.traeger;
        if (scan.literale.size() >= 2) ++kz.multi;
        Befund b{fund.kategorie, OrganDetail::render_versionen(scan.literale), digest_hex_of(bytes)};
        b.praep_rot   = scan.unter_praeprozessor;
        out[fund.rel] = b;
    }
    return lage;
}

/// Die Riegel-Schaerfung (ii): discovered-Zaehler MIT NENNER, je Kategorie eine BESTAND-Zeile.
/// multi (G3) = Dateien mit mehreren Literalen -- gefahren, nicht behauptet. Seit D1 je
/// Overlay-Kategorie eine Zeile + die Gesamtzeile (der Nenner der Deckungs-Aussage).
void drucke_bestand(BestandZaehler const& z) {
    std::fprintf(stdout,
                 "axis_version_lock: BESTAND heuristik discovered=%d traeger=%d "
                 "(nenner: alle *.hpp unter libs/cache_engine/heuristik)\n",
                 z.heuristik_n, z.heuristik_n);
    struct Zeile {
        char const*       name;
        KatZaehler const& kz;
    };
    for (Zeile const& zl :
         {Zeile{"organ", z.organ}, Zeile{"system", z.system}, Zeile{"mess", z.mess}, Zeile{"tier_substanz", z.tier}}) {
        std::fprintf(stdout,
                     "axis_version_lock: BESTAND %s discovered=%d traeger=%d forwarder_prosa=%d multi=%d "
                     "(nenner: Quell-Dateien der %s-Eintraege des Overlay-Schnitts, "
                     "builder/overlay_source_set.hpp)\n",
                     zl.name, zl.kz.discovered, zl.kz.traeger, zl.kz.discovered - zl.kz.traeger, zl.kz.multi, zl.name);
    }
    std::fprintf(stdout,
                 "axis_version_lock: BESTAND overlay gesamt=%d traeger=%d (nenner: ALLE Quell-Dateien des "
                 "Overlay-Schnitts -- die Menge, ueber die das Overlay-Glied [7] hasht)\n",
                 z.overlay_gesamt(), z.overlay_traeger());
}

/// Mindest-Nenner-Wache (G5a, seit D5 je Kategorie). true = verletzt (Exit-2-Klasse).
[[nodiscard]] bool nenner_verletzt(BestandZaehler const& z) {
    if (z.heuristik_n >= kMinHeuristikDiscovered && z.organ.discovered >= kMinOrganDiscovered &&
        z.system.discovered >= kMinSystemDiscovered && z.mess.discovered >= kMinMessDiscovered &&
        z.tier.discovered >= kMinTierDiscovered)
        return false;
    std::fprintf(stderr,
                 "axis_version_lock: ROT Mindest-Nenner unterschritten: heuristik=%d (min %d), organ=%d "
                 "(min %d), system=%d (min %d), mess=%d (min %d), tier_substanz=%d (min %d) -- leere/"
                 "geschrumpfte Homes sind kein Gruen (V-1); nach einer bewussten Umgliederung die Anker "
                 "im Tool nachziehen\n",
                 z.heuristik_n, kMinHeuristikDiscovered, z.organ.discovered, kMinOrganDiscovered, z.system.discovered,
                 kMinSystemDiscovered, z.mess.discovered, kMinMessDiscovered, z.tier.discovered, kMinTierDiscovered);
    return true;
}

// Definition unten (Reihenfolge unveraendert); do_write braucht den Parser seit D2 fuer die
// Rueckstufungs-Pruefung gegen das BESTEHENDE Register.
[[nodiscard]] bool parse_lock_v3(std::string const& path, BefundMap& out);

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

    // D2: TRAEGER-AUSTRITT HAT KEINEN --write-WEG. Rot-zuerst literal belegt: Literal entfernt,
    // --write gefahren => Exit 0, und der Record verschwand ganz bzw. wurde still 'organ -' --
    // die VERSION verliess das Register auf dem vom Werkzeug selbst angewiesenen Weg. Jetzt:
    // steht dieselbe Datei im BESTEHENDEN Register als Traeger (version != '-') und im neuen
    // Bestand als digest-only, verweigert --write byte-identisch. Der legitime Austritt ist
    // Loeschen/Umbenennen der Datei (verwaist -> bewusster Regen); die Aufwertung '-' ->
    // Literal bleibt frei (D3). Ein nicht (als v2) lesbares Alt-Register kann keine
    // Rueckstufung bezeugen -- dann Neuaufbau mit HINWEIS statt stiller Annahme.
    if (fs::exists(lockfile)) {
        BefundMap alt;
        if (parse_lock_v3(lockfile, alt)) {
            for (auto const& [rel, b] : bestand) {
                auto const it = alt.find(rel);
                if (it == alt.end()) continue;
                if (ist_overlay_kategorie(it->second.category) && it->second.version != "-" && b.version == "-") {
                    std::fprintf(stderr,
                                 "axis_version_lock: ROT Traeger-Austritt: algo_version-Literal entfernt, Datei "
                                 "wuerde digest-only (Register-Version '%s'): %s\n",
                                 it->second.version.c_str(), rel.c_str());
                    rc = 1;
                }
            }
        } else {
            std::fprintf(stderr, "axis_version_lock: HINWEIS bestehendes Lock nicht als v3 lesbar (Meldungen "
                                 "oben betreffen das ALTE Register) -- Neuaufbau ohne Rueckstufungs-Pruefung\n");
        }
    }
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
    // LOCK-KOPF: seit Fixup 3 (D1) beschreibt er die GELTENDE Semantik -- der Fixup-2-Einfrier-
    // Grund (byte-identische 158er-Baseline) endete mit diesem legitimen Regen (s. Datei-Kopf).
    out << "# axis_version.lock -- content-digest tripwire ueber die algo_version-Traeger und die\n";
    out << "# Overlay-Quellmenge des Tier-Fingerprints (S-14a Riegel, Task #33/KON17-03; vormals W3-C)\n";
    out << "# format: v3\n";
    out << "# record (DREI Zeilen je Datei):\n";
    out << "#   '<category> <version> <dirname/>'   Kopfzeile; dirname repo-relativ MIT Schluss-'/'\n";
    out << "#   '    <basename>'                    Datei-Name (vier Leerzeichen Einrueckung)\n";
    out << "#   '    <sha256-hex>'                  Digest (vier Leerzeichen Einrueckung)\n";
    out << "# Der volle Pfad ist EXAKT dirname+basename.\n";
    out << "#   heuristik: alle *.hpp unter libs/cache_engine/heuristik; version = Integer der EINEN\n";
    out << "#              dedizierten '// AXIS_ALGO_VERSION: <N>'-Marker-Zeile.\n";
    out << "#   organ/system/mess/tier_substanz: die Quell-Dateien des Overlay-Schnitts\n";
    out << "#              (builder/overlay_source_set.hpp -- die Menge, ueber die das Overlay-Glied [7]\n";
    out << "#              des Tier-Fingerprints hasht; Endungen kQuellEndungen, kNichtQuellEndungen\n";
    out << "#              bewusst draussen). version = geordnete LISTE aller algo_version-String-\n";
    out << "#              Literale im Code ('X.Y.Z[.flags]'): N gleiche Literale => der Einzelwert,\n";
    out << "#              ungleiche => komma-gefuegt in Text-Reihenfolge; '-' = kein Literal\n";
    out << "#              (digest-only).\n";
    out << "# KATEGORIE-REGELN: die Kategorie-Spalte wird beim --check gegen die Discovery gehalten\n";
    out << "# (Abweichung = ROT). Rueckstufung traeger -> digest-only ist ROT und hat keinen\n";
    out << "# --write-Weg; die Aufwertung digest-only -> traeger ist legitim (--check Exit 3, dann\n";
    out << "# Regen). Jede Aenderung an einer verzeichneten Datei verlangt den bewussten\n";
    out << "# Lock-Regen-Commit (--write); ein Versions-Bump macht ihn legitim, ersetzt ihn nicht.\n";
    out << "# Formatwahl v3: der Pfad-SPLIT (dirname auf der Kopfzeile, basename als Folgezeile)\n";
    out << "# haelt jede Zeile <=120 Spalten -- die neue Grundgesamtheit traegt Pfade bis 138 Byte,\n";
    out << "# die in keine Einzelzeile passen (Diff-Hygiene-Wache voll gedeckt, keine Ausnahme-\n";
    out << "# Klasse). v1-/v2-Locks werden beim --check mit klarer Meldung abgewiesen (kein stilles\n";
    out << "# Weiterlesen); Kopfzeile ohne BEIDE Folgezeilen = lauter Formatfehler.\n";
    for (auto const& [rel, b] : bestand) {
        // Pfad-Split (D6): dirname MIT Schluss-'/', basename = Rest. Jeder Pfad der
        // Grundgesamtheit liegt unter libs/cache_engine/ -- rfind('/') trifft immer; der
        // unmoegliche Fall ohne '/' ergaebe eine Kopfzeile ohne Verzeichnis-Schluss und
        // fiele im strengen Parser LAUT um, nie still.
        std::size_t const schnitt  = rel.rfind('/');
        std::string const dirname  = (schnitt == std::string::npos) ? "" : rel.substr(0, schnitt + 1);
        std::string const basename = (schnitt == std::string::npos) ? rel : rel.substr(schnitt + 1);
        out << b.category << ' ' << b.version << ' ' << dirname << '\n';
        out << "    " << basename << '\n';
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

/// Parst die Lock-Datei v3 (D6: DREIZEILEN-Record). Verlangt die Formatzeile '# format: v3'
/// VOR dem ersten Eintrag; v1/v2 werden ueber die Format-Zeile mit klarer Meldung abgewiesen.
[[nodiscard]] bool parse_lock_v3(std::string const& path, BefundMap& out) {
    std::ifstream f(path);
    if (!f) {
        std::fprintf(stderr, "axis_version_lock: ROT Lock-Datei nicht lesbar: %s\n", path.c_str());
        return false;
    }
    enum class Erwarte { kopf, basename, digest };
    bool        format_ok = false;
    Erwarte     erwarte   = Erwarte::kopf;
    Befund      kopf;
    std::string kopf_dir;
    std::string kopf_base;
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("# format:", 0) == 0) {
            if (line == "# format: v3") {
                format_ok = true;
                continue;
            }
            std::fprintf(stderr,
                         "axis_version_lock: ROT Lock-Format unbekannt/veraltet ('%s', erwartet '# format: v3') "
                         "-- mit --write regenerieren: %s\n",
                         line.c_str(), path.c_str());
            return false;
        }
        if (line.empty() || line[0] == '#') continue;
        if (!format_ok) {
            std::fprintf(stderr, "axis_version_lock: ROT Lock ohne '# format: v3'-Kopf vor dem ersten Eintrag: %s\n",
                         path.c_str());
            return false;
        }
        if (line[0] == ' ' || line[0] == '\t') {
            // Folgezeile (basename oder Digest), nach ERWARTUNGS-ZUSTAND gelesen -- nie nach
            // Inhalt geraten. STRENG (Pflicht-Fixup 13.08.2026, Koeder G2/G3, seit v3 dreizeilig):
            // genau EIN Token; Digest exakt 64 lowercase-Hex; basename ohne '/'. Vorher schluckte
            // 'ds >> token' Rest-Tokens still, und size()==64 liess Muellstrings durch -- der
            // Fehler erschien dann als irrefuehrende Drift-Meldung UEBER DIE DATEI, nie ueber
            // das Register.
            std::istringstream ds(line);
            std::string        token;
            std::string        zusatz;
            ds >> token;
            bool const hat_zusatz = static_cast<bool>(ds >> zusatz);
            if (erwarte == Erwarte::kopf) {
                std::fprintf(stderr, "axis_version_lock: ROT Folgezeile ohne Record-Kopf im Lock: '%s'\n",
                             line.c_str());
                return false;
            }
            if (erwarte == Erwarte::basename) {
                if (token.empty() || hat_zusatz || token.find('/') != std::string::npos) {
                    std::fprintf(stderr,
                                 "axis_version_lock: ROT unparsbare Basename-Zeile im Lock (Record-Kopf %s; "
                                 "erwartet genau EIN Token ohne '/'): '%s'\n",
                                 kopf_dir.c_str(), line.c_str());
                    return false;
                }
                kopf_base = token;
                erwarte   = Erwarte::digest;
                continue;
            }
            // Erwarte::digest
            if (!ist_sha256_hex(token) || hat_zusatz) {
                std::fprintf(stderr,
                             "axis_version_lock: ROT unparsbare Digest-Zeile im Lock (Record-Kopf %s; erwartet "
                             "genau 64 Hex-Zeichen [0-9a-f], ohne Zusatz): '%s'\n",
                             (kopf_dir + kopf_base).c_str(), line.c_str());
                return false;
            }
            kopf.digest_hex        = token;
            std::string const voll = kopf_dir + kopf_base;
            // STRENG (Koeder G4): ein doppelter Record fuer denselben Pfad wuerde sonst still per
            // 'last wins' aufgeloest -- im Identitaets-Register ist das ein Formatfehler.
            if (out.find(voll) != out.end()) {
                std::fprintf(stderr, "axis_version_lock: ROT doppelter Lock-Record fuer Pfad: %s\n", voll.c_str());
                return false;
            }
            out[voll] = kopf;
            erwarte   = Erwarte::kopf;
            continue;
        }
        if (erwarte != Erwarte::kopf) {
            std::fprintf(stderr, "axis_version_lock: ROT Lock-Record ohne %s-Folgezeile: %s\n",
                         erwarte == Erwarte::basename ? "Basename" : "Digest", (kopf_dir + kopf_base).c_str());
            return false;
        }
        std::istringstream ls(line);
        if (!(ls >> kopf.category >> kopf.version >> kopf_dir)) {
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
        if (kopf_dir.empty() || kopf_dir.back() != '/') {
            std::fprintf(stderr,
                         "axis_version_lock: ROT Lock-Kopfzeile ohne Verzeichnis-Schluss '/' (v3-Pfad-Split): "
                         "'%s'\n",
                         line.c_str());
            return false;
        }
        kopf_base.clear();
        erwarte = Erwarte::basename;
    }
    if (erwarte != Erwarte::kopf) {
        std::fprintf(stderr, "axis_version_lock: ROT Lock-Record ohne %s-Folgezeile am Dateiende: %s\n",
                     erwarte == Erwarte::basename ? "Basename" : "Digest", (kopf_dir + kopf_base).c_str());
        return false;
    }
    if (!format_ok) std::fprintf(stderr, "axis_version_lock: ROT Lock ohne '# format: v3'-Kopf: %s\n", path.c_str());
    return format_ok;
}

int do_check(fs::path const& root, std::string const& lockfile) {
    BefundMap lock;
    if (!parse_lock_v3(lockfile, lock)) return 1;

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
        if (ist_overlay_kategorie(ist.category) && soll.version != "-" && ist.version == "-") {
            // D2: TRAEGER-AUSTRITT, benannt mit Datei und Register-Version. Vorher lief dieser
            // Fall in die generische OHNE-Bump-Meldung; der --write-Weg stand offen und die
            // Version verliess das Register still (rot-zuerst literal belegt).
            std::fprintf(stderr,
                         "axis_version_lock: ROT Traeger-Austritt: algo_version-Literal entfernt, Datei "
                         "wuerde digest-only (Register-Version '%s'): %s\n",
                         soll.version.c_str(), rel.c_str());
            std::fprintf(stderr, "axis_version_lock:     HINWEIS kein --write-Weg; Literal wiederherstellen "
                                 "oder Datei bewusst entfernen/umbenennen (verwaist -> Regen)\n");
            red = 1;
            continue;
        }
        if (ist_overlay_kategorie(ist.category) && soll.version == "-" && ist.version != "-") {
            // D3: AUFWERTUNG digest-only -> traeger ist eine legitime Aenderung (vorher falsch
            // als 'OHNE gueltigen Version-Bump' Exit 1, rot-zuerst literal belegt). Wie der
            // Bump-Zweig: erst der Regen-Commit segnet den konkreten Inhalt (Exit 3).
            std::fprintf(stdout, "axis_version_lock: OK Aufwertung digest-only -> traeger ('-' -> %s) %s\n",
                         ist.version.c_str(), rel.c_str());
            std::fprintf(stdout, "axis_version_lock: HINWEIS Lock erneuern (--write) fuer %s\n", rel.c_str());
            regen_noetig = true;
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
        std::fprintf(stderr, "axis_version_lock:     HINWEIS Datei geloescht/umgezogen/aus dem Schnitt "
                             "gefallen? Bewussten Lock-Regen-Commit (--write) fahren\n");
        red = 1;
    }
    // D4: DECKUNGS-BELEG. gedeckt = Overlay-Quellen MIT Register-Record -- unabhaengig vom
    // Kontrollfluss oben gezaehlt, damit die Zahl eine MESSUNG ist und kein Nebenprodukt.
    // Eine Luecke ist ROT: das ist das doppelte Netz neben dem unlocked-Pfad (Mutationsprobe
    // K13 im Fixup-Commit), nie Prosa.
    int gedeckt = 0;
    for (auto const& [rel, ist] : bestand)
        if (ist_overlay_kategorie(ist.category) && lock.find(rel) != lock.end()) ++gedeckt;
    if (gedeckt != z.overlay_gesamt()) {
        std::fprintf(stderr,
                     "axis_version_lock: ROT DECKUNGSLUECKE -- nur %d von %d Overlay-Quellen im Register "
                     "(Schnitt: builder/overlay_source_set.hpp)\n",
                     gedeckt, z.overlay_gesamt());
        red = 1;
    }
    drucke_bestand(z);
    if (red != 0) return 1;
    if (regen_noetig) {
        std::fprintf(stderr, "axis_version_lock: REGEN ERFORDERLICH -- Version-Bump akzeptiert, aber das "
                             "Register ist veraltet; Lock-Regen-Commit (--write) fahren (Exit 3)\n");
        return 3;
    }
    std::fprintf(stdout,
                 "axis_version_lock: GRUEN bestand konsistent -- %zu Dateien (heuristik=%d, organ=%d, "
                 "system=%d, mess=%d, tier_substanz=%d) -- deckt %d von %d Overlay-Quellen (Schnitt: "
                 "builder/overlay_source_set.hpp)\n",
                 bestand.size(), z.heuristik_n, z.organ.discovered, z.system.discovered, z.mess.discovered,
                 z.tier.discovered, gedeckt, z.overlay_gesamt());
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
