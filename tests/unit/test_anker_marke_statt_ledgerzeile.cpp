// tests/unit/test_anker_marke_statt_ledgerzeile.cpp
// ================================================================================================
// DIE ANKER-WACHE: Code-Kommentare verweisen auf MARKEN, nie auf Ledger-ZEILENNUMMERN.
// ================================================================================================
//
// DIE INVARIANTE, DIE HIER GEPRUEFT WIRD
// -------------------------------------
// Eine Zeilennummer in einem Code-Kommentar ist eine tickende Fehlmeldung. Der Ledger
// (super docs/DIPLOMARBEIT-ZIELE-OFFENE-PUNKTE-LEDGER.md) waechst -- allein am 10.08.2026 von
// 16.785 auf 19.310 Zeilen. Jeder Verweis auf eine Zahl zeigt danach auf etwas anderes, und zwar
// LAUTLOS: der Kommentar liest sich weiter wie ein Beleg, ist aber keiner mehr.
//
// DER BEFUND, DER DIESE WACHE ERZWINGT (10.08.2026, am Objekt erhoben, ce 95cb3039)
// --------------------------------------------------------------------------------
// Drei gemeldete tote Anker, alle bestaetigt, und die Kette dahinter ist der eigentliche Beweis:
//
//   (1) profile_facade/mess_achsen_naht.hpp -- Verweis auf die Ledger-Zeilen 4082-4095 fuer die
//       STUFEN-DOKTRIN. Dort steht heute "DER LANDUNGSSTAND DES TAGES" (super-/ce-SHAs). Tot.
//   (2) DIESELBE Datei, Verweis auf Ledger-Zeile 3319 fuer die Kompilations-Status-Kopplung.
//       Dort steht heute der work_mode/--debug-Entscheid. Tot. -- UND: derselbe tote Anker stand
//       nicht einmal, sondern FUENFMAL im Baum (mess_achsen_naht, mess_konsistenz_gate,
//       cache_engine_builder_iterator x3). Die Meldung nannte eine Stelle; das Objekt trug fuenf.
//   (3) abi/anatomy_version_stamp.hpp -- ein STALE gewordener Inhalt statt einer toten Zahl:
//       der Kommentar fuehrte eine Architektur-Frage als "offen", die der Bau laengst zugunsten
//       des Registry-Weges entschieden hatte (axis_variant_version_table.hpp, produktiv bis in
//       die ABI). Anker koennen also auf zwei Arten sterben: die ZAHL wandert, oder die AUSSAGE
//       wird ueberholt.
//
// DIE ENTSCHEIDENDE BEOBACHTUNG -- WARUM EINE REGEL UND NICHT NUR DREI KORREKTUREN:
// Fuer den EINEN unveraenderten Satz aus (2) existieren inzwischen VIER Zahlen. Der Code sagte 3319.
// Der Ledger bemerkte den Bruch selbst -- in KON2-15, NICHT in KON2-22 (das ist der Anatomy-Stale-
// Befund; die erste Fassung dieses Kopfes setzte die falsche Marke, im Marken-Paket) -- und
// korrigierte auf 9077. Auch 9077 war binnen Tagen tot (dort steht die §19.C/§19.D-Dock-Topologie).
// Die Fassung vom 10.08.2026 schrieb daraufhin "am Objekt wohnt der Satz bei Zeile 10409".
// AM 11.08.2026 IST AUCH DIESE ZAHL TOT: der Ledger wuchs ueber Nacht von 19.310 auf 19.589 Zeilen,
// 10409 traegt jetzt den §52-B13-Rest, der Satz wohnt bei 10688. Die Kette 3319 -> 9077 -> 10409 ->
// 10688 hat sich also WAEHREND der Arbeit an ihr um ein Glied verlaengert, und drei der vier Zahlen
// schrieb jemand, der gerade eine tote Zahl reparierte. Eine Zeilennummer laesst sich nicht pflegen;
// sie laesst sich nur ersetzen. Marken (§62-B, §43.b, §58-V, "Nachtrag 05.08.2026 mittag-9", KON2-15)
// wandern nicht. Auch eine Zahl, die HEUTE stimmt, ist keine Ausnahme -- sie ist der naechste Fall.
//
// WAS DIESE WACHE TUT
// -------------------
//   TEIL 1  NULL-ZONE (hart, kein Spielraum): die geheilten Dateien tragen NULL Zeilenverweise.
//           Ein Rueckfall wird namentlich mit Datei:Zeile gemeldet. Der Nenner sind die
//           TATSAECHLICH GELESENEN Dateien -- eine benannte, aber nicht gescannte Datei ist ein
//           harter Fehlschlag, kein stilles Gruen (s. NACHTRAG 11.08.2026 unten).
//   TEIL 2  RATSCHE ueber den Gesamtbestand: der Baum traegt hoechstens kObergrenze Verweise.
//           Heute sind es exakt so viele; JEDER NEUE Verweis -- egal in welcher Datei -- hebt die
//           Zahl darueber und macht diesen Test rot. Damit ist die Klasse ab sofort geschlossen,
//           ohne dass die Pipeline heute rot wird (Doktrin "Pipeline immer hart gruen"). Die
//           Ratsche ist kein Dauerzustand: wer Altbestand heilt, senkt kObergrenze mit.
//   TEIL 3  GEGENPROBE gegen die stille Null: ein Zaehler, der nichts findet, weil er nichts
//           ansieht, wuerde TEIL 1 und TEIL 2 beide gruen erscheinen lassen. Deshalb wird
//           positiv belegt, dass der Scan Dateien liest, Marken findet und Verweise findet --
//           und seit dem 11.08.2026 auch, dass der Erkenner IM SCAN verdrahtet ist.
//   TEIL 4  KOEDER-EINHEIT (K13, beidseitig): der Erkenner selbst wird mit gewuerfelten Eingaben
//           gefahren -- Zeilenverweis MUSS gefangen werden, Marke MUSS durchgelassen werden.
//   TEIL 5  KOEDER AUF DEN NENNER (K13, beidseitig): die Buchfuehrung "welche Datei wurde gelesen"
//           wird selbst gebissen -- erfundener Pfad MUSS durchfallen, gelesener Pfad MUSS bestehen,
//           und eine (simuliert) umbenannte Null-Zonen-Datei MUSS gefangen werden.
//
// NACHTRAG 11.08.2026 -- ZWEI LANDUNGSBLOCKER AUS DER ZWEITEN LENS, BEIDE AN DER URSACHE GEHEILT
// ----------------------------------------------------------------------------------------------
// (B2) DER NULL-ZONEN-NENNER WAR HOHL. TEIL 1 zaehlte je Eintrag von kNullZone BEDINGUNGSLOS hoch
//      und verglich das Ergebnis mit std::size(kNullZone) -- eine Tautologie, die nicht fallen
//      konnte. Es gab keinen Existenz- und keinen Gelesen-Nachweis. UEBERLEBENDE MUTATION: eine
//      Null-Zonen-Datei umbenennen; der Test blieb gruen und druckte weiter "3/3 geprueft".
//      Das war DIESELBE stille Null, die derselbe Autor in ist_ausgeschlossen() vorbildlich
//      seziert und dokumentiert hatte -- an zweiter Stelle, ungefixt. Ein Befund, den man einmal
//      versteht, ist damit nicht ueberall behoben.
//      GEHEILT: der Scan fuehrt jetzt e.gelesene (die real geoeffneten Dateien); TEIL 1 zaehlt nur
//      beobachtete Dateien und meldet eine nicht gescannte namentlich; TEIL 5 beisst den Nenner.
// (B1) lag ausserhalb dieser Datei (falsche Architektur-Aussage in abi/anatomy_version_stamp.hpp)
//      und ist dort geheilt. Fuer diese Wache bleibt die LEHRE: der Commit vom 10.08.2026 erzeugte
//      beim Heilen eines toten Ankers einen NEUEN toten Querverweis (er entfernte das Wort
//      METADATEN-BLOCKER, auf das zwei andere Dateien namentlich zeigen) -- und diese Wache konnte
//      ihn nicht sehen, weil sie nur das Ledger-Wort mit Doppelpunkt und Ziffer kennt. S. GRENZEN.
//
// GRENZEN, EHRLICH BENANNT (was diese Wache NICHT kann)
// -----------------------------------------------------
//   * Sie prueft die FORM des Ankers, nicht seine WAHRHEIT. Ob "§62-B" inhaltlich das Richtige
//     benennt, kann sie nicht wissen -- Fall (3) oben (stale Inhalt bei intakter Form) faellt
//     strukturell nicht in ihren Griff. Sie schliesst die Klasse "tickende Zahl", nicht die
//     Klasse "ueberholte Aussage".
//   * Sie sieht nur diesen Baum (ce). Verweise in super/ und im Thesis-Repo bleiben ungedeckt --
//     benannt, nicht gedeckt.
//   * Der Restbestand von kObergrenze Verweisen ist NICHT geprueft. Jeder einzelne davon kann
//     tot sein; die Stichprobe dieses Tages fand 2 tote unter 3 untersuchten. Die Ratsche haelt
//     nur den ZUWACHS auf.
//   * SIE KENNT NUR EIN WORT. Alles unten am Objekt gemessen (11.08.2026, Stand dieses Commits,
//     GNU grep, Muster "Wort, optional Leerraum, Doppelpunkt, optional Leerraum, Ziffer"):
//       - Kurzform "LED" statt "LEDGER": 6 Fundstellen IM Scan-Bereich, fuer die Wache unsichtbar
//         (profile_run_facade.cpp, registrierungs_sidecar.hpp, bestandslog_lock.hpp,
//         run_methodology_registry.hpp, test_g3_bestandslog_lock.cpp, test_experiment_plan_director).
//       - Andere wachsende Plandokumente, Form "DOSSIER:NNNN": 11 Fundstellen im Scan-Bereich.
//         Die Doktrin gilt der ZAHL; diese Wache gilt einem WORT. Das ist die Luecke, nicht ein Rest.
//       - ce-eigenes docs/: 92 Fundstellen der Vollform, ausserhalb der Scan-Wurzeln.
//     Ein Folgeposten hat den Erkenner auf die Klasse "wachsendes Dokument, Doppelpunkt, Zahl" zu
//     heben und die Ratsche danach NEU zu erheben. Bis dahin: benannt, nicht gedeckt.
//   * SCAN-WURZELN: gescannt werden libs/apps/tools/tests. Ausserhalb liegen 110 Dateien mit
//     Quell-Endung (adapters, benchmarks, cmake, deploy, modules, prerequisites, scripts, plus die
//     Wurzel-CMakeLists). GEGENPROBE, damit die Null nicht nackt bleibt: dasselbe Muster findet dort
//     heute 0 Treffer, waehrend es in den gescannten Wurzeln 94-mal trifft -- die Null ist echt, aber
//     sie ist eine Momentaufnahme. Zukunftsrisiko, kein Gegenwartsdefekt.
//   * SIE SIEHT KEINE MARKEN-QUERVERWEISE ZWISCHEN DATEIEN. Genau daran ist die erste Fassung
//     dieses Pakets gescheitert: sie entfernte das Wort METADATEN-BLOCKER aus einem Datei-Kopf, auf
//     das zwei andere Dateien namentlich zeigen. Form intakt, Ziel weg, Wache blind.
//
// SELBSTBEZUG, bewusst geloest: diese Datei wird von ihrem eigenen Scan miterfasst (sie liegt in
// tests/). Sie enthaelt deshalb nirgends die verbotene Form; der Suchbegriff wird zur Laufzeit
// zusammengesetzt und die toten Zahlen oben stehen ohne die Doppelpunkt-Form. Eine Ausnahme-Liste
// waere die schlechtere Loesung gewesen -- sie waere das naechste, was still verrottet.
//
// Praezedenz fuer das Quellbaum-Lesen: test_t15_drift_gate_messschleife.cpp (COMDARE_T15_*_QUELLE).
// Doktrin: Owner 09.08.2026 -- "Es waere sauberer im cmake-Debug Modus standard google Tests zu
// fahren und diese in Release zu wiederholen ... Skripte sagen gar nichts."

#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#ifndef COMDARE_ANKER_QUELLBAUM
#error "COMDARE_ANKER_QUELLBAUM fehlt -- ohne Quellbaum-Pfad waere jeder Nenner dieses Tests eine stille Null."
#endif

namespace {

namespace fs = std::filesystem;

/// Die Obergrenze der Ratsche: der am 10.08.2026 nach der Heilung gemessene Bestand.
/// ERHEBUNG (reproduzierbar; GNU grep unter /usr/bin, NICHT die ugrep-Emulation -- die zaehlt bei
/// -o -c TREFFER statt ZEILEN). Muster: das Wort, optionaler Leerraum, Doppelpunkt, optionaler
/// Leerraum, Ziffer; Endungen hpp/cpp/h/cc/cmake plus CMakeLists.txt; Ergebnis durch "wc -l".
/// Vorher 100, nach der Heilung der drei Stellen (6 Fundorte) 94.
/// WER ALTBESTAND HEILT, SENKT DIESE ZAHL MIT. Sie darf nie steigen.
constexpr std::size_t kObergrenze = 94;

/// Die geheilten Dateien: hier ist NULL die Vorgabe, nicht "hoechstens".
/// cache_engine_builder_iterator.hpp steht bewusst NICHT hier -- dort wurden die drei Verweise auf
/// die tote Zeile 3319 geheilt, aber sieben weitere Verweise (auf andere Zahlen) sind UNGEPRUEFT
/// stehen geblieben. Eine Datei in die Null-Zone zu nehmen, deren Rest man nicht untersucht hat,
/// waere genau die Sorte Gruen, die nur ihren eigenen Gegenstand deckt.
constexpr std::string_view kNullZone[] = {
    "libs/cache_engine/profile_facade/mess_achsen_naht.hpp",
    "libs/cache_engine/include/cache_engine/abi/anatomy_version_stamp.hpp",
    "libs/cache_engine/builder/pruef_dock/mess_konsistenz_gate.hpp",
};

/// Der Suchbegriff, zur Laufzeit zusammengesetzt -- so steht die verbotene Form nirgends im
/// Quelltext dieser Datei und der Selbstbezug loest sich ohne Ausnahme-Liste.
std::string marken_wort() { return std::string{"LEDG"} + "ER"; }

char klein(char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }

/// Ist das der Anfang eines ZEILENVERWEISES? Erwartet wird: das Wort, dann optional Leerraum,
/// dann ':', dann optional Leerraum, dann eine ZIFFER. "§62-B" oder "KON2-22" treffen das nicht.
bool ist_zeilenverweis_ab(std::string const& zeile, std::size_t pos, std::size_t wortlaenge) {
    std::size_t i = pos + wortlaenge;
    while (i < zeile.size() && (zeile[i] == ' ' || zeile[i] == '\t')) { ++i; }
    if (i >= zeile.size() || zeile[i] != ':') { return false; }
    ++i;
    while (i < zeile.size() && (zeile[i] == ' ' || zeile[i] == '\t')) { ++i; }
    return i < zeile.size() && std::isdigit(static_cast<unsigned char>(zeile[i])) != 0;
}

struct ZeilenTreffer {
    std::string datei;
    std::size_t zeile{0};
    std::string text;
};

struct Ernte {
    std::vector<ZeilenTreffer> zeilenverweise; ///< die verbotene Form
    /// Die TATSAECHLICH gelesenen Dateien, relativ zur Wurzel, generic_string.
    /// SIE IST DER NENNER DER NULL-ZONE -- nicht die Wunschliste kNullZone. Vor dem 11.08.2026 zaehlte
    /// TEIL 1 seine eigene Soll-Liste ab ("3/3 geprueft") und konnte deshalb strukturell nie fallen:
    /// eine umbenannte Null-Zonen-Datei blieb gruen. Das war dieselbe stille Null wie die in
    /// ist_ausgeschlossen() dokumentierte -- an zweiter Stelle, im selben Test.
    std::vector<std::string> gelesene;
    std::size_t              marken{0}; ///< §-Marken -- die erlaubte Form, als Gegenprobe
    std::size_t              dateien{0};
    std::size_t              zeilen{0};
};

/// Wurde diese Datei vom Scan WIRKLICH gelesen? Der Unterschied zwischen "0 Treffer" und "nie
/// angesehen" ist die ganze Frage; genau ihn beantwortet diese Funktion.
bool wurde_gelesen(Ernte const& e, std::string_view const relativ) {
    return std::find(e.gelesene.begin(), e.gelesene.end(), relativ) != e.gelesene.end();
}

/// Zaehlt die Treffer in EINER Textzeile. Getrennt herausgezogen, damit der Koeder-Test (TEIL 4)
/// exakt dieselbe Logik fahren kann wie der Baum-Scan -- ein zweiter, nachgebauter Erkenner waere
/// die Drift-Quelle, gegen die diese Datei an jeder anderen Stelle argumentiert.
std::size_t zeilenverweise_in(std::string const& zeile) {
    std::string const wort       = marken_wort();
    std::string const klein_wort = [&] {
        std::string s = wort;
        std::transform(s.begin(), s.end(), s.begin(), klein);
        return s;
    }();
    std::string kleinzeile = zeile;
    std::transform(kleinzeile.begin(), kleinzeile.end(), kleinzeile.begin(), klein);

    std::size_t treffer = 0;
    for (std::size_t p = kleinzeile.find(klein_wort); p != std::string::npos; p = kleinzeile.find(klein_wort, p + 1)) {
        if (ist_zeilenverweis_ab(kleinzeile, p, klein_wort.size())) { ++treffer; }
    }
    return treffer;
}

/// Die Paragraf-Marke (die ERLAUBTE Form) -- nur fuer die Gegenprobe gezaehlt, nie beanstandet.
std::size_t marken_in(std::string const& zeile) {
    std::size_t treffer = 0;
    // '\xC2\xA7' ist das Paragraf-Zeichen in UTF-8. Byteweise gesucht, damit keine Locale
    // mitredet (dieselbe Vorsicht wie in scripts/ci_diff_ascii_width_guard.sh).
    for (std::size_t p = zeile.find("\xC2\xA7"); p != std::string::npos; p = zeile.find("\xC2\xA7", p + 1)) {
        ++treffer;
    }
    return treffer;
}

bool ist_quellendung(fs::path const& p) {
    if (p.filename() == "CMakeLists.txt") { return true; }
    std::string e = p.extension().string();
    std::transform(e.begin(), e.end(), e.begin(), klein);
    return e == ".hpp" || e == ".cpp" || e == ".h" || e == ".hh" || e == ".cc" || e == ".cxx" || e == ".tpp" ||
           e == ".ipp" || e == ".inl" || e == ".cmake";
}

/// Ausschluss ueber PFAD-KOMPONENTEN des RELATIVEN Pfades, nie ueber Teilstrings und nie ueber den
/// absoluten Pfad.
///
/// ZWEI FALLEN, beide hier am Objekt bezahlt:
///  (1) TEILSTRING (Fallen-Register, bekannt): ein Filter auf "build" frisst "/builder/" gleich mit
///      -- und builder/ ist genau der Baum, in dem die meisten Verweise stehen. Gruen und blind.
///  (2) ABSOLUTER PFAD (10.08.2026, in DIESEM Test aufgelaufen, NEU): die erste Fassung lief ueber
///      die Komponenten des ABSOLUTEN Pfades. Dieser Worktree liegt unter
///      ".../probst-diplomarbeit-cache-engine/.git/modules/.../worktrees/wf_...", traegt also ".git"
///      als Komponente des WURZEL-PRAEFIX. Ergebnis: JEDE Datei galt als ausgeschlossen, der Scan
///      las 0 Dateien -- und die Null-Zone (TEIL 1) wie die Ratsche (TEIL 2) meldeten GRUEN, weil
///      "keine Treffer" und "nichts angesehen" fuer sie ununterscheidbar sind. Nur die Gegenprobe
///      (TEIL 3) wurde rot. Das ist der Beleg, warum TEIL 3 existiert; er ist nicht Zierde.
bool ist_ausgeschlossen(fs::path const& relativ) {
    for (auto const& teil : relativ) {
        std::string const s = teil.string();
        if (s == "ext" || s == "vendor" || s == "third_party" || s == "_deps" || s == "googletest" || s == ".git" ||
            s == "build" || s.rfind("build-", 0) == 0 || s.rfind("cmake-build", 0) == 0) {
            return true;
        }
    }
    return false;
}

Ernte ernte_baum() {
    Ernte          e{};
    fs::path const wurzel{COMDARE_ANKER_QUELLBAUM};
    for (std::string_view const zweig : {"libs", "apps", "tools", "tests"}) {
        fs::path const  start = wurzel / zweig;
        std::error_code ec{};
        if (!fs::is_directory(start, ec)) { continue; }
        for (fs::recursive_directory_iterator it{start, fs::directory_options::skip_permission_denied, ec}, ende;
             it != ende; it.increment(ec)) {
            if (ec) { break; }
            if (!it->is_regular_file(ec)) { continue; }
            fs::path const& p = it->path();
            // NUR der Pfad UNTERHALB der Wurzel wird gefiltert -- das Wurzel-Praefix gehoert der
            // Umgebung und darf nie mitentscheiden (s. Falle (2) in ist_ausgeschlossen).
            fs::path const rel = fs::relative(p, wurzel);
            if (ist_ausgeschlossen(rel) || !ist_quellendung(p)) { continue; }
            std::ifstream in{p};
            if (!in) { continue; }
            // ERST HIER gilt die Datei als gelesen -- nach dem Oeffnen, nicht nach dem Finden. Ein
            // Eintrag, der schon am ifstream scheitert, darf keinen Nenner fuellen.
            std::string const rel_s = rel.generic_string();
            ++e.dateien;
            e.gelesene.push_back(rel_s);
            std::string zeile;
            std::size_t nr = 0;
            while (std::getline(in, zeile)) {
                ++nr;
                ++e.zeilen;
                e.marken += marken_in(zeile);
                std::size_t const n = zeilenverweise_in(zeile);
                for (std::size_t k = 0; k < n; ++k) { e.zeilenverweise.push_back({rel_s, nr, zeile.substr(0, 110)}); }
            }
        }
    }
    return e;
}

Ernte const& ernte() {
    static Ernte const einmal = ernte_baum();
    return einmal;
}

/// Wuerfel LITERAL aus /dev/urandom -- nicht ueber std::random_device.
/// RICHTIGSTELLUNG 11.08.2026: hier versprach der Kommentar einen "benannten Rueckfall", den der Code
/// nicht hatte. Er nahm std::random_device, und dessen Verhalten ohne Entropie-Quelle ist
/// implementierungsdefiniert -- eine Implementierung DARF eine konstante Folge liefern. Ein Koeder aus
/// einer Konstante ist kein Koeder. Jetzt steht die Quelle im Code, und der Rueckfall ist wirklich
/// benannt: es gibt keinen. Fehlt die Quelle, ist das ein sprechender Fehlschlag, keine stille 0.
bool wuerfel_bytes(unsigned char* ziel, std::size_t n) {
    std::ifstream q{"/dev/urandom", std::ios::binary};
    if (!q) { return false; }
    q.read(reinterpret_cast<char*>(ziel), static_cast<std::streamsize>(n));
    return static_cast<std::size_t>(q.gcount()) == n;
}

unsigned zufall(unsigned modulo) {
    if (modulo == 0) {
        ADD_FAILURE() << "zufall(0) -- ein Wuerfel ohne Seiten. Aufrufstelle pruefen.";
        return 0;
    }
    unsigned char b[4]{};
    if (!wuerfel_bytes(b, sizeof b)) {
        ADD_FAILURE() << "/dev/urandom nicht lesbar -- der Koeder waere eine Konstante. "
                         "KEIN stiller Rueckfall auf eine feste Zahl.";
        return 0;
    }
    unsigned v = 0;
    for (unsigned char const x : b) { v = (v << 8) | static_cast<unsigned>(x); }
    return v % modulo;
}

} // namespace

// ------------------------------------------------------------------------------------------------
// TEIL 3 zuerst: die GEGENPROBE. Ohne sie waeren TEIL 1 und TEIL 2 vakuum-gruen.
// ------------------------------------------------------------------------------------------------
TEST(AnkerWache, DerScanSiehtUeberhauptEtwas) {
    Ernte const& e = ernte();
    std::cout << "[NENNER] quellbaum=" << COMDARE_ANKER_QUELLBAUM << "\n"
              << "[NENNER] dateien gelesen=" << e.dateien << " zeilen gelesen=" << e.zeilen << "\n"
              << "[NENNER] zeilenverweise=" << e.zeilenverweise.size() << " von hoechstens " << kObergrenze
              << "  paragraf-marken=" << e.marken << "\n";
    // Wuerde der Scan ins Leere greifen (falscher Pfad, falscher Filter), waeren alle drei Zahlen 0
    // und die beiden Wachen unten meldeten Gruen, ohne etwas geprueft zu haben.
    EXPECT_GT(e.dateien, 500u) << "Der Quellbaum-Scan liest zu wenige Dateien -- stille Null.";
    EXPECT_GT(e.marken, 100u) << "Keine Paragraf-Marken gefunden -- der Erkenner greift nicht.";
    // Die erlaubte Form muss die verbotene deutlich ueberwiegen; sonst ist die Hausregel Fiktion.
    EXPECT_GT(e.marken, e.zeilenverweise.size())
        << "Mehr Zeilenverweise als Marken -- die Anker-Regel ist im Bestand nicht die Mehrheit.";
    // DIE VERDRAHTUNG Scan -> Erkenner (11.08.2026 nachgezogen). Ohne diese Zeile ueberlebt eine
    // Mutation der Aufrufstelle in ernte_baum() ("n = 0" statt zeilenverweise_in(zeile)): TEIL 1 und
    // TEIL 2 blieben gruen (0 Treffer sieht aus wie Sauberkeit), und TEIL 4 merkt nichts, weil er den
    // Erkenner DIREKT ruft und den Baum-Scan gar nicht beruehrt. Der Restbestand ist die Zusicherung.
    EXPECT_GT(e.zeilenverweise.size(), 0u)
        << "Der Baum-Scan findet 0 Zeilenverweise. Bei kObergrenze=" << kObergrenze
        << " Restbestand heisst das: der Erkenner ist im Scan nicht verdrahtet, nicht dass der "
           "Baum sauber ist. Faellt diese Zusicherung nach echtem Vollzug der Heilung, gehoert sie "
           "GEMEINSAM mit kObergrenze=0 entfernt -- nie allein.";
    std::cout << "[NENNER] verdrahtung: zeilenverweise=" << e.zeilenverweise.size()
              << " (>0 verlangt) gelesene-liste=" << e.gelesene.size() << " == dateien=" << e.dateien << "\n";
    EXPECT_EQ(e.gelesene.size(), e.dateien) << "Gelesen-Liste und Datei-Zaehler driften -- der Nenner "
                                               "der Null-Zone haengt an dieser Gleichheit.";
}

// ------------------------------------------------------------------------------------------------
// TEIL 1: die NULL-ZONE. Hart, ohne Spielraum.
// ------------------------------------------------------------------------------------------------
TEST(AnkerWache, GeheilteDateienTragenKeinenZeilenverweis) {
    Ernte const& e          = ernte();
    std::size_t  beobachtet = 0; ///< vom Scan WIRKLICH gelesen -- der einzige zulaessige Nenner
    std::size_t  fehlend    = 0; ///< in kNullZone benannt, aber nie angesehen
    std::size_t  verstoesse = 0;
    for (std::string_view const soll : kNullZone) {
        if (!wurde_gelesen(e, soll)) {
            ++fehlend;
            ADD_FAILURE() << "NULL-ZONEN-DATEI NICHT GESCANNT: " << soll << "\n"
                          << "  Der Scan hat sie nie geoeffnet -- 'keine Treffer' waere hier BEDEUTUNGSLOS.\n"
                          << "  Ursache ist entweder eine Umbenennung/Verschiebung (dann kNullZone nachziehen)\n"
                          << "  oder ein Filter, der sie frisst. Beides macht den Nenner hohl.";
            continue;
        }
        ++beobachtet;
        for (auto const& t : e.zeilenverweise) {
            if (t.datei == soll) {
                ++verstoesse;
                ADD_FAILURE() << "Zeilenverweis in der NULL-ZONE: " << t.datei << ":" << t.zeile << "\n"
                              << "  " << t.text << "\n"
                              << "  Regel: MARKE statt Zeile (§62-B, Nachtrag 05.08.2026 mittag-9, KON2-15).";
            }
        }
    }
    std::cout << "[NENNER] null-zone: " << beobachtet << "/" << std::size(kNullZone)
              << " Dateien GELESEN (nicht bloss gewuenscht), " << fehlend << " nicht gescannt, " << verstoesse
              << " Verstoesse (Soll 0)\n";
    // Der Vergleich ist erst jetzt eine Aussage: beobachtet wird NUR hochgezaehlt, wenn die Datei in
    // e.gelesene steht. Vor dem 11.08.2026 stand hier ein Zaehler, der im selben Schleifendurchlauf
    // bedingungslos wuchs -- die Gleichung war eine Tautologie und konnte nicht fallen.
    EXPECT_EQ(beobachtet, std::size(kNullZone)) << "Der Null-Zonen-Nenner ist hohl: nicht jede benannte "
                                                   "Datei wurde gelesen.";
    EXPECT_EQ(fehlend, 0u);
    EXPECT_EQ(verstoesse, 0u);
}

// ------------------------------------------------------------------------------------------------
// TEIL 2: die RATSCHE. Faengt JEDEN neuen Verweis, egal wo.
// ------------------------------------------------------------------------------------------------
TEST(AnkerWache, GesamtbestandUeberschreitetDieRatscheNicht) {
    Ernte const&      e   = ernte();
    std::size_t const ist = e.zeilenverweise.size();
    std::cout << "[NENNER] ratsche: " << ist << " von hoechstens " << kObergrenze
              << "  (Stand 10.08.2026 nach Heilung: 94; vorher 100)\n";
    if (ist > kObergrenze) {
        std::cout << "[NEU HINZUGEKOMMEN - Kandidaten, alle Fundorte gelistet]\n";
        for (auto const& t : e.zeilenverweise) {
            std::cout << "  " << t.datei << ":" << t.zeile << "  " << t.text << "\n";
        }
    }
    EXPECT_LE(ist, kObergrenze)
        << "Ein NEUER Ledger-Zeilenverweis ist in den Baum gekommen. Zeilennummern wandern -- der "
           "Ledger wuchs am 10.08.2026 allein um 2.525 Zeilen. Verweise auf die MARKE (§62-B, §43.b, "
           "§58-V) oder auf einen datierten Nachtrag, nie auf eine Zahl.";
    // Die Ratsche ist eine Obergrenze, keine Zusicherung: sinkt der Bestand, gehoert die Konstante
    // nachgezogen. Diese Meldung ist bewusst nur Hinweis, nicht rot -- Heilen darf nie bestraft werden.
    if (ist < kObergrenze) {
        std::cout << "[HINWEIS] Bestand ist auf " << ist << " gesunken -- kObergrenze bitte nachziehen.\n";
    }
}

// ------------------------------------------------------------------------------------------------
// TEIL 4: der KOEDER (K13), beidseitig, gewuerfelt. Derselbe Erkenner wie im Baum-Scan.
// ------------------------------------------------------------------------------------------------
TEST(AnkerWache, KoederBeisstBeidseitig) {
    std::string const wort = marken_wort();

    // Seite A -- der Koeder MUSS beissen. Zahl und Schreibweise gewuerfelt, damit kein Sonderfall
    // die Wache traegt: mal ohne Leerraum, mal mit, mal in Kleinschreibung.
    std::size_t           gefangen = 0;
    constexpr std::size_t kProben  = 32;
    for (std::size_t i = 0; i < kProben; ++i) {
        unsigned const nr     = zufall(19310) + 1; // Ledger-Zeilenraum von heute
        unsigned const stil   = zufall(4);
        std::string    koeder = "// irgendein Kommentar (";
        std::string    w      = wort;
        if (stil == 3) { std::transform(w.begin(), w.end(), w.begin(), klein); }
        koeder += w;
        koeder += (stil == 1) ? " :" : ((stil == 2) ? ": " : ":");
        koeder += std::to_string(nr);
        koeder += ") und weiterer Text";
        gefangen += (zeilenverweise_in(koeder) == 1) ? 1 : 0;
    }
    std::cout << "[KOEDER A] zeilenverweis erkannt: " << gefangen << "/" << kProben << "\n";
    EXPECT_EQ(gefangen, kProben) << "Ein gewuerfelter Zeilenverweis ist durchgerutscht.";

    // Seite B -- die ERLAUBTE Form darf NICHT beissen. Ohne diese Seite waere ein Erkenner, der
    // einfach immer 'true' sagt, gruen: er faenge jeden Koeder und jede Marke gleich mit.
    std::size_t                    falsch_positiv = 0;
    std::vector<std::string> const erlaubt        = {
        "// Vertrag aus " + wort +
            " \xC2\xA7"
            "62-B (KOMPILATIONS-STATUS-KOPPLUNG, Owner 21.07.)",
        "// STUFEN-DOKTRIN (" + wort + " Nachtrag 05.08.2026 mittag-9/-10, Owner-Abnahme mittag-11)",
        "// s. " + wort + " KON2-22 -- der Bau hat entschieden, der Entscheid ist undokumentiert",
        "// \xC2\xA7"
        "43.b und \xC2\xA7"
        "58-V, beide ohne jede Zahl",
        "// " + wort + "-Nachtrag ohne Doppelpunkt 3319",
    };
    for (auto const& gut : erlaubt) { falsch_positiv += zeilenverweise_in(gut); }
    std::cout << "[KOEDER B] falsch-positiv auf erlaubten Formen: " << falsch_positiv << "/" << erlaubt.size()
              << " Proben (Soll 0)\n";
    EXPECT_EQ(falsch_positiv, 0u) << "Die Wache beanstandet die erlaubte Marken-Form -- sie beisst blind.";
}

// ------------------------------------------------------------------------------------------------
// TEIL 5: DER KOEDER AUF DEN NENNER SELBST (K13, beidseitig, gewuerfelt aus /dev/urandom).
//
// TEIL 4 prueft den ERKENNER. Dieser Teil prueft den NENNER -- und das ist eine andere Frage.
// Die ueberlebende Mutation, gegen die er antritt (gefunden in der zweiten Lens am 10.08.2026):
// eine Null-Zonen-Datei UMBENENNEN. Bis zum 11.08.2026 blieb TEIL 1 dabei gruen und druckte weiter
// "3/3 Dateien geprueft" -- er zaehlte seine eigene Wunschliste ab. Der Test bestand aus einer
// Behauptung ueber sich selbst.
//
// Warum das hier eine EIGENE Einheit ist und nicht bloss eine Zeile in TEIL 1: der Nenner-Fehler ist
// nur sichtbar, wenn man ihn ABSICHTLICH herbeifuehrt. Ein gruener TEIL 1 beweist gar nichts ueber
// seine eigene Fallhoehe -- genau das war ja der Defekt.
// ------------------------------------------------------------------------------------------------
TEST(AnkerWache, NullZonenNennerFaengtDieNichtGescannteDatei) {
    Ernte const&          e       = ernte();
    constexpr std::size_t kProben = 32;

    // Seite A -- was NICHT gescannt wurde, MUSS als nicht gescannt erkannt werden.
    // Gewuerfelt, damit kein einzelner Sonderfall die Aussage traegt.
    std::size_t erkannt_fehlend = 0;
    for (std::size_t i = 0; i < kProben; ++i) {
        std::string const erfunden = "libs/cache_engine/gibt_es_nicht_" + std::to_string(zufall(1000000000u)) + ".hpp";
        erkannt_fehlend += wurde_gelesen(e, erfunden) ? 0u : 1u;
    }
    std::cout << "[KOEDER C] erfundene Pfade als 'nicht gescannt' erkannt: " << erkannt_fehlend << "/" << kProben
              << "\n";
    EXPECT_EQ(erkannt_fehlend, kProben) << "wurde_gelesen() haelt einen erfundenen Pfad fuer gelesen.";

    // Seite B -- was WIRKLICH gescannt wurde, darf NICHT als fehlend gelten. Ohne diese Seite waere
    // ein wurde_gelesen(), das immer 'false' sagt, in Seite A gruen -- und wuerde in TEIL 1 jede
    // Null-Zonen-Datei falsch anklagen. Die Stichprobe wird aus der Gelesen-Liste GEWUERFELT.
    ASSERT_FALSE(e.gelesene.empty()) << "Gelesen-Liste leer -- Seite B haette keinen Gegenstand.";
    std::size_t erkannt_vorhanden = 0;
    for (std::size_t i = 0; i < kProben; ++i) {
        std::string const& treffer = e.gelesene[zufall(static_cast<unsigned>(e.gelesene.size()))];
        erkannt_vorhanden += wurde_gelesen(e, treffer) ? 1u : 0u;
    }
    std::cout << "[KOEDER D] gewuerfelte gelesene Pfade als 'gelesen' erkannt: " << erkannt_vorhanden << "/" << kProben
              << " (aus " << e.gelesene.size() << " gelesenen Dateien)\n";
    EXPECT_EQ(erkannt_vorhanden, kProben) << "wurde_gelesen() verneint eine tatsaechlich gelesene Datei.";

    // Seite C -- DIE SCHARFE PROBE: die Umbenennung, exakt wie sie die zweite Lens vorfuehrte.
    // Jeder Null-Zonen-Name bekommt ein gewuerfeltes Suffix; der so entstandene Name MUSS durchfallen.
    // Das ist der Biss, der vor dem 11.08.2026 nicht stattfand.
    std::size_t umbenannt_gefangen = 0;
    for (std::string_view const soll : kNullZone) {
        std::string const umbenannt = std::string{soll} + ".umbenannt_" + std::to_string(zufall(1000000u));
        umbenannt_gefangen += wurde_gelesen(e, umbenannt) ? 0u : 1u;
    }
    std::cout << "[KOEDER E] umbenannte Null-Zonen-Dateien gefangen: " << umbenannt_gefangen << "/"
              << std::size(kNullZone) << "\n";
    EXPECT_EQ(umbenannt_gefangen, std::size(kNullZone))
        << "Eine umbenannte Null-Zonen-Datei gilt weiter als geprueft -- der Nenner ist wieder hohl.";
}
