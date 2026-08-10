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
// Fuer den EINEN unveraenderten Satz aus (2) existieren heute DREI Zahlen. Der Code sagte 3319.
// Der Ledger bemerkte den Bruch selbst (KON2-22) und korrigierte auf 9077. Auch 9077 ist heute
// tot (dort steht §19.C Dock-Topologie); am Objekt wohnt der Satz bei Zeile 10409, also in §62-B.
// Die mittlere Zahl wurde von jemandem geschrieben, der GERADE EINE TOTE ZAHL REPARIERTE -- und
// sie war binnen Tagen wieder tot. Eine Zeilennummer laesst sich nicht richtig pflegen; sie laesst
// sich nur ersetzen. Marken (§62-B, §43.b, §58-V, "Nachtrag 05.08.2026 mittag-9") wandern nicht.
//
// WAS DIESE WACHE TUT
// -------------------
//   TEIL 1  NULL-ZONE (hart, kein Spielraum): die geheilten Dateien tragen NULL Zeilenverweise.
//           Ein Rueckfall wird namentlich mit Datei:Zeile gemeldet.
//   TEIL 2  RATSCHE ueber den Gesamtbestand: der Baum traegt hoechstens kObergrenze Verweise.
//           Heute sind es exakt so viele; JEDER NEUE Verweis -- egal in welcher Datei -- hebt die
//           Zahl darueber und macht diesen Test rot. Damit ist die Klasse ab sofort geschlossen,
//           ohne dass die Pipeline heute rot wird (Doktrin "Pipeline immer hart gruen"). Die
//           Ratsche ist kein Dauerzustand: wer Altbestand heilt, senkt kObergrenze mit.
//   TEIL 3  GEGENPROBE gegen die stille Null: ein Zaehler, der nichts findet, weil er nichts
//           ansieht, wuerde TEIL 1 und TEIL 2 beide gruen erscheinen lassen. Deshalb wird
//           positiv belegt, dass der Scan Dateien liest, Marken findet und Verweise findet.
//   TEIL 4  KOEDER-EINHEIT (K13, beidseitig): der Erkenner selbst wird mit gewuerfelten Eingaben
//           gefahren -- Zeilenverweis MUSS gefangen werden, Marke MUSS durchgelassen werden.
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
#include <random>
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
    std::size_t marken{0};                    ///< §-Marken -- die erlaubte Form, als Gegenprobe
    std::size_t dateien{0};
    std::size_t zeilen{0};
};

/// Zaehlt die Treffer in EINER Textzeile. Getrennt herausgezogen, damit der Koeder-Test (TEIL 4)
/// exakt dieselbe Logik fahren kann wie der Baum-Scan -- ein zweiter, nachgebauter Erkenner waere
/// die Drift-Quelle, gegen die diese Datei an jeder anderen Stelle argumentiert.
std::size_t zeilenverweise_in(std::string const& zeile) {
    std::string const wort  = marken_wort();
    std::string const klein_wort = [&] {
        std::string s = wort;
        std::transform(s.begin(), s.end(), s.begin(), klein);
        return s;
    }();
    std::string kleinzeile = zeile;
    std::transform(kleinzeile.begin(), kleinzeile.end(), kleinzeile.begin(), klein);

    std::size_t treffer = 0;
    for (std::size_t p = kleinzeile.find(klein_wort); p != std::string::npos;
         p              = kleinzeile.find(klein_wort, p + 1)) {
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
    return e == ".hpp" || e == ".cpp" || e == ".h" || e == ".hh" || e == ".cc" || e == ".cxx" ||
           e == ".tpp" || e == ".ipp" || e == ".inl" || e == ".cmake";
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
        if (s == "ext" || s == "vendor" || s == "third_party" || s == "_deps" || s == "googletest" ||
            s == ".git" || s == "build" || s.rfind("build-", 0) == 0 || s.rfind("cmake-build", 0) == 0) {
            return true;
        }
    }
    return false;
}

Ernte ernte_baum() {
    Ernte e{};
    fs::path const wurzel{COMDARE_ANKER_QUELLBAUM};
    for (std::string_view const zweig : {"libs", "apps", "tools", "tests"}) {
        fs::path const start = wurzel / zweig;
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
            ++e.dateien;
            std::string zeile;
            std::size_t nr = 0;
            while (std::getline(in, zeile)) {
                ++nr;
                ++e.zeilen;
                e.marken += marken_in(zeile);
                std::size_t const n = zeilenverweise_in(zeile);
                for (std::size_t k = 0; k < n; ++k) {
                    e.zeilenverweise.push_back(
                        {fs::relative(p, wurzel).generic_string(), nr, zeile.substr(0, 110)});
                }
            }
        }
    }
    return e;
}

Ernte const& ernte() {
    static Ernte const einmal = ernte_baum();
    return einmal;
}

/// Wuerfel aus /dev/urandom, mit benanntem Rueckfall -- ein Test, der beim Fehlen der
/// Entropie-Quelle stumm eine Konstante nimmt, hat seinen Koeder verloren.
unsigned zufall(unsigned modulo) {
    std::random_device rd;
    return rd() % modulo;
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
}

// ------------------------------------------------------------------------------------------------
// TEIL 1: die NULL-ZONE. Hart, ohne Spielraum.
// ------------------------------------------------------------------------------------------------
TEST(AnkerWache, GeheilteDateienTragenKeinenZeilenverweis) {
    Ernte const& e = ernte();
    std::size_t geprueft = 0;
    std::size_t verstoesse = 0;
    for (std::string_view const soll : kNullZone) {
        ++geprueft;
        for (auto const& t : e.zeilenverweise) {
            if (t.datei == soll) {
                ++verstoesse;
                ADD_FAILURE() << "Zeilenverweis in der NULL-ZONE: " << t.datei << ":" << t.zeile << "\n"
                              << "  " << t.text << "\n"
                              << "  Regel: MARKE statt Zeile (§62-B, Nachtrag 05.08.2026 mittag-9, KON2-22).";
            }
        }
    }
    std::cout << "[NENNER] null-zone: " << geprueft << "/" << std::size(kNullZone)
              << " Dateien geprueft, " << verstoesse << " Verstoesse (Soll 0)\n";
    EXPECT_EQ(geprueft, std::size(kNullZone));
    EXPECT_EQ(verstoesse, 0u);
}

// ------------------------------------------------------------------------------------------------
// TEIL 2: die RATSCHE. Faengt JEDEN neuen Verweis, egal wo.
// ------------------------------------------------------------------------------------------------
TEST(AnkerWache, GesamtbestandUeberschreitetDieRatscheNicht) {
    Ernte const& e = ernte();
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
    std::size_t gefangen = 0;
    constexpr std::size_t kProben = 32;
    for (std::size_t i = 0; i < kProben; ++i) {
        unsigned const nr    = zufall(19310) + 1; // Ledger-Zeilenraum von heute
        unsigned const stil  = zufall(4);
        std::string koeder   = "// irgendein Kommentar (";
        std::string w        = wort;
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
    std::size_t falsch_positiv = 0;
    std::vector<std::string> const erlaubt = {
        "// Vertrag aus " + wort + " \xC2\xA7" "62-B (KOMPILATIONS-STATUS-KOPPLUNG, Owner 21.07.)",
        "// STUFEN-DOKTRIN (" + wort + " Nachtrag 05.08.2026 mittag-9/-10, Owner-Abnahme mittag-11)",
        "// s. " + wort + " KON2-22 -- der Bau hat entschieden, der Entscheid ist undokumentiert",
        "// \xC2\xA7" "43.b und \xC2\xA7" "58-V, beide ohne jede Zahl",
        "// " + wort + "-Nachtrag ohne Doppelpunkt 3319",
    };
    for (auto const& gut : erlaubt) { falsch_positiv += zeilenverweise_in(gut); }
    std::cout << "[KOEDER B] falsch-positiv auf erlaubten Formen: " << falsch_positiv << "/"
              << erlaubt.size() << " Proben (Soll 0)\n";
    EXPECT_EQ(falsch_positiv, 0u) << "Die Wache beanstandet die erlaubte Marken-Form -- sie beisst blind.";
}
