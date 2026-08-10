// test_d2g1_ctest_registrierung.cpp -- D2-G1 (2026-08-10): DIE REGISTRIERUNG IST TEIL DES TESTS.
//
// SELBSTCHECK: dieser Test behauptet NICHTS ueber den INHALT irgendeines Testfalls. Er behauptet genau
// eine Sache: JEDE gtest-Binary unterhalb libs/ ERSCHEINT IN ctest -- unter ihrem eigenen Namen, ohne
// Platzhalter, ohne Defer-Datei. Wer hier eine Welch- oder Achsen-Rechnung nachprueft, ist in der
// falschen Datei (die haelt libs/cache_engine/builder/commands/tests/test_commands.cpp).
//
// DER BEFUND, DER IHN AUSGELOEST HAT (D2-G1, am Objekt gemessen 10.08.2026, Host prod1):
//   libs/cache_engine/builder/commands/tests/CMakeLists.txt war die EINZIGE Stelle im Repo, die
//   gtest_discover_tests noch benutzte -- dasselbe Verfahren, das cmake/gtest_setup.cmake:56-82
//   ausdruecklich als unbrauchbar verworfen hat. Folge, literal gemessen im frisch konfigurierten,
//   NICHT gebauten Baum:
//
//       Test   #1: test_commands_NOT_BUILT
//       Test   #2: test_engine_adapters_NOT_BUILT
//
//   Zwei Eintraege, die beim Lauf mit "Could not find executable" FEHLSCHLAGEN -- ein Daueralarm, und
//   die 30 echten gtest-Faelle standen dahinter NICHT in ctest. Erst nach einem vollen Bau ersetzte
//   die Discovery sie durch 30 Einzeleintraege. Die Sichtbarkeit hing damit am Bauweg, nicht am
//   Bauplan: EIN Job, der nur comdare_tests baut statt 'all', stellt den Daueralarm wieder her.
//
// WARUM EIN GOOGLE TEST UND KEIN SKRIPT (Owner-KERN, Rohtranskript Z. 25562, promptSource=typed):
//   "Ich sehe einen Haufen shells statt vernuenftiger google tests, was soll das? [...] SKRIPTE SAGEN
//    GAR NICHTS." -- diese Wache faehrt in Debug UND Release durch dieselbe ctest-Zeile wie jeder
//   andere Unit-Test.
//
// NENNER (V-1): jeder der drei Faelle druckt seine Grundgesamtheit MIT der Aussage aus. Eine nackte
// Zahl ohne Nenner waere hier besonders wertlos -- die Zahl der CTestTestfile.cmake haengt an der
// Host-Klasse und an den vier bedingten Registrierungs-Bloecken (2-PASS-Falle).
//
// FAIL-CLOSED: findet der Sucher seinen Gegenstand nicht (0 CMakeLists unter libs/, 0 CTestTestfile
// im Baubaum), ist das ROT. Ein Sucher, der ins Leere greift und gruen meldet, ist die Klasse von
// Wache, gegen die dieser Test gebaut ist.

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// Quellbaum-Wurzel und Baubaum-Wurzel kommen aus CMake (tests/unit/CMakeLists.txt), nicht aus argv:
// der Test soll aus JEDEM Arbeitsverzeichnis dieselbe Aussage treffen.
constexpr char const* kQuellbaum = COMDARE_D2G1_QUELLBAUM;
constexpr char const* kBaubaum   = COMDARE_D2G1_BAUBAUM;

std::string datei_inhalt(fs::path const& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) { return {}; }
    std::ostringstream puffer;
    puffer << in.rdbuf();
    return puffer.str();
}

// Ein Bezeichner im CMake-Sinn: Buchstaben, Ziffern, Unterstrich.
bool ist_namens_zeichen(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

// Zieht alle Vorkommen von <schluessel>( <NAME> aus einem CMake-Text. Bewusst KEIN std::regex:
// die generierten CTestTestfile.cmake schreiben add_test([=[NAME]=] ...), die handgeschriebenen
// add_test(NAME <n> COMMAND ...) und die Defer-Datei add_test(<n> <n>) -- drei Formen, die eine
// gemeinsame Regex nur unleserlich machen wuerden.
std::vector<std::string> namen_nach_schluessel(std::string const& text, std::string const& schluessel) {
    std::vector<std::string> treffer;
    std::size_t              pos = 0;
    while ((pos = text.find(schluessel, pos)) != std::string::npos) {
        std::size_t i = pos + schluessel.size();
        // Nur ein echter Aufruf zaehlt: zwischen Schluesselwort und '(' steht hoechstens Weissraum.
        while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) { ++i; }
        if (i >= text.size() || text[i] != '(') {
            pos += schluessel.size();
            continue;
        }
        // Auskommentierte Zeilen ueberspringen: vom Fundort rueckwaerts bis zum Zeilenanfang.
        std::size_t zeilen_start = text.rfind('\n', pos);
        zeilen_start             = (zeilen_start == std::string::npos) ? 0 : zeilen_start + 1;
        bool auskommentiert      = false;
        for (std::size_t k = zeilen_start; k < pos; ++k) {
            if (text[k] == '#') {
                auskommentiert = true;
                break;
            }
            if (text[k] != ' ' && text[k] != '\t') { break; }
        }
        if (auskommentiert) {
            pos += schluessel.size();
            continue;
        }
        ++i;
        while (i < text.size() && (text[i] == ' ' || text[i] == '\t' || text[i] == '\n')) { ++i; }
        // Generator-Form add_test([=[NAME]=] ...) auspacken.
        if (text.compare(i, 3, "[=[") == 0) { i += 3; }
        // Handgeschriebene Form add_test(NAME <n> COMMAND ...): das Schluesselwort NAME ueberspringen.
        if (text.compare(i, 5, "NAME ") == 0) {
            i += 5;
            while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) { ++i; }
        }
        std::string name;
        while (i < text.size() && ist_namens_zeichen(text[i])) {
            name.push_back(text[i]);
            ++i;
        }
        if (!name.empty()) { treffer.push_back(name); }
        pos += schluessel.size();
    }
    return treffer;
}

// NENNER-QUELLE (T-3, FREMD): der QUELLBAUM. Welche gtest-Binaries unterhalb libs/ existieren, sagt
// uns die CMakeLists, die sie deklariert -- nicht ctest, gegen das wir gleich vergleichen.
// Erfasst wird ein add_executable nur dann, wenn dieselbe Datei gtest_main nennt: ein kuenftiges
// Nicht-Test-Executable unter libs/ soll diese Wache nicht faelschlich rot faerben.
std::set<std::string> deklarierte_gtest_binaries(fs::path const& libs_wurzel, std::size_t& cmakelists_gesehen) {
    std::set<std::string> namen;
    cmakelists_gesehen = 0;
    std::error_code ec;
    if (!fs::is_directory(libs_wurzel, ec)) { return namen; }
    for (fs::recursive_directory_iterator it(libs_wurzel, ec), ende; it != ende; it.increment(ec)) {
        if (ec || !it->is_regular_file(ec) || it->path().filename() != "CMakeLists.txt") { continue; }
        ++cmakelists_gesehen;
        std::string const text = datei_inhalt(it->path());
        if (text.find("gtest_main") == std::string::npos) { continue; }
        for (auto const& n : namen_nach_schluessel(text, "add_executable")) { namen.insert(n); }
    }
    return namen;
}

// ZAEHLER: was ctest tatsaechlich kennt. Gelesen wird das OBJEKT auf der Platte -- die generierten
// CTestTestfile.cmake und die von gtest_discover_tests erzeugten *_tests.cmake -- nicht eine
// Log-Zeile und nicht die Ankuendigung in einer CMakeLists (V-8).
std::set<std::string> registrierte_ctest_namen(fs::path const& bau_wurzel, std::size_t& testfiles_gesehen) {
    std::set<std::string> namen;
    testfiles_gesehen = 0;
    std::error_code ec;
    if (!fs::is_directory(bau_wurzel, ec)) { return namen; }
    for (fs::recursive_directory_iterator it(bau_wurzel, ec), ende; it != ende; it.increment(ec)) {
        if (ec || !it->is_regular_file(ec)) { continue; }
        std::string const name = it->path().filename().string();
        bool const        ist_testfile =
            (name == "CTestTestfile.cmake") || (name.size() > 12 && name.rfind("_tests.cmake") == name.size() - 12);
        if (!ist_testfile) { continue; }
        ++testfiles_gesehen;
        for (auto const& n : namen_nach_schluessel(datei_inhalt(it->path()), "add_test")) { namen.insert(n); }
    }
    return namen;
}

// Die Defer-Dateien, die gtest_discover_tests anlegt: <ziel>[N]_include.cmake, mit dem
// NOT_BUILT-Zweig darin. Rein informativ -- siehe die Warnung unten, warum ihre blosse EXISTENZ
// NICHT das Orakel sein darf.
std::vector<fs::path> defer_dateien_auf_platte(fs::path const& bau_wurzel) {
    std::vector<fs::path> treffer;
    std::error_code       ec;
    if (!fs::is_directory(bau_wurzel, ec)) { return treffer; }
    for (fs::recursive_directory_iterator it(bau_wurzel, ec), ende; it != ende; it.increment(ec)) {
        if (ec || !it->is_regular_file(ec)) { continue; }
        std::string const name = it->path().filename().string();
        if (name.size() > 14 && name.rfind("_include.cmake") == name.size() - 14) { treffer.push_back(it->path()); }
    }
    return treffer;
}

// DAS ORAKEL: welche CTestTestfile.cmake BINDEN eine Defer-Datei EIN?
//
// WARUM NICHT EINFACH DIE EXISTENZ PRUEFEN -- am Objekt gemessen, 10.08.2026: nach der Umstellung
// auf add_test lagen die zwei Defer-Dateien des vorigen Configure-Laufs WEITER im Baubaum. CMake
// raeumt sie nicht weg. Die neu erzeugte CTestTestfile.cmake verweist aber auf keine von ihnen mehr
// (gemessen: 0 von 128 CTestTestfile.cmake nennen noch "_include.cmake"). Eine Wache auf blosse
// Existenz waere in jedem wiederverwendeten Bau-Verzeichnis rot -- ein Daueralarm, und damit genau
// die Sorte Messgeraet, gegen die T-1 steht. Erreichbarkeit ist die Eigenschaft, die zaehlt: nur ein
// EINGEBUNDENER Defer-Zweig kann test_<ziel>_NOT_BUILT nach ctest tragen.
std::vector<fs::path> testfiles_mit_defer_einbindung(fs::path const& bau_wurzel, std::size_t& testfiles_gesehen) {
    std::vector<fs::path> treffer;
    testfiles_gesehen = 0;
    std::error_code ec;
    if (!fs::is_directory(bau_wurzel, ec)) { return treffer; }
    for (fs::recursive_directory_iterator it(bau_wurzel, ec), ende; it != ende; it.increment(ec)) {
        if (ec || !it->is_regular_file(ec) || it->path().filename() != "CTestTestfile.cmake") { continue; }
        ++testfiles_gesehen;
        if (datei_inhalt(it->path()).find("_include.cmake") != std::string::npos) { treffer.push_back(it->path()); }
    }
    return treffer;
}

} // namespace

// (1) DIE EIGENTLICHE ZUSICHERUNG. Jede unter libs/ deklarierte gtest-Binary muss in ctest unter
//     GENAU IHREM NAMEN stehen. Vor dem Bau dieser Wache stand dort ExecuteEngineCommand.BasicExecution
//     und 29 Geschwister -- aber eben NICHT test_commands.
TEST(D2G1CtestRegistrierung, JedeGtestBinaryUnterLibsIstUnterIhremNamenRegistriert) {
    std::size_t                 cmakelists_gesehen = 0;
    std::set<std::string> const deklariert =
        deklarierte_gtest_binaries(fs::path(kQuellbaum) / "libs", cmakelists_gesehen);
    std::size_t                 testfiles_gesehen = 0;
    std::set<std::string> const registriert       = registrierte_ctest_namen(fs::path(kBaubaum), testfiles_gesehen);

    // FAIL-CLOSED: ein Sucher ohne Gegenstand meldet nicht gruen.
    ASSERT_GT(cmakelists_gesehen, 0u) << "NENNER LEER: keine CMakeLists.txt unter " << kQuellbaum << "/libs gefunden";
    ASSERT_GT(testfiles_gesehen, 0u) << "NENNER LEER: keine CTestTestfile.cmake unter " << kBaubaum << " gefunden";

    std::vector<std::string> fehlend;
    for (auto const& n : deklariert) {
        if (registriert.find(n) == registriert.end()) { fehlend.push_back(n); }
    }

    std::ostringstream befund;
    befund << "NENNER: " << deklariert.size() << " gtest-Binaries unter libs/ deklariert (aus " << cmakelists_gesehen
           << " CMakeLists.txt); " << registriert.size() << " ctest-Namen registriert (aus " << testfiles_gesehen
           << " Testfiles). NICHT registriert: " << fehlend.size();
    for (auto const& n : fehlend) { befund << "\n  [FEHLT] " << n; }
    EXPECT_TRUE(fehlend.empty()) << befund.str();
    std::cout << befund.str() << "\n";
}

// (2) DAS VERWORFENE VERFAHREN DARF NICHT ZURUECKKOMMEN. Ein EINGEBUNDENER Defer-Zweig traegt
//     test_<ziel>_NOT_BUILT nach ctest, sobald ein Bauweg die Binary auslaesst -- ein Eintrag, der
//     beim Lauf garantiert fehlschlaegt. Geprueft wird die EINBINDUNG, nicht die blosse Existenz der
//     Datei (Begruendung an testfiles_mit_defer_einbindung; Kurzfassung: CMake raeumt alte
//     Defer-Dateien nicht weg, und eine Wache darauf waere in jedem wiederbenutzten Baubaum rot).
TEST(D2G1CtestRegistrierung, KeinEingebundenerDeferZweigAusGtestDiscoverTests) {
    std::size_t                 testfiles_gesehen = 0;
    std::vector<fs::path> const eingebunden = testfiles_mit_defer_einbindung(fs::path(kBaubaum), testfiles_gesehen);
    ASSERT_GT(testfiles_gesehen, 0u) << "NENNER LEER: keine CTestTestfile.cmake unter " << kBaubaum << " gefunden";

    std::size_t                 namen_gesehen = 0;
    std::set<std::string> const registriert   = registrierte_ctest_namen(fs::path(kBaubaum), namen_gesehen);
    std::vector<std::string>    platzhalter;
    for (auto const& n : registriert) {
        if (n.size() > 10 && n.rfind("_NOT_BUILT") == n.size() - 10) { platzhalter.push_back(n); }
    }

    // Rein informativ: liegen gebliebene Defer-Dateien aus frueheren Configure-Laeufen. Sie faerben
    // diesen Test NICHT rot -- ohne Einbindung erreicht sie ctest nie.
    std::vector<fs::path> const inert = defer_dateien_auf_platte(fs::path(kBaubaum));

    std::ostringstream befund;
    befund << "NENNER: " << testfiles_gesehen << " CTestTestfile.cmake im Baubaum, " << registriert.size()
           << " registrierte Namen. Mit eingebundenem Defer-Zweig: " << eingebunden.size()
           << ", NOT_BUILT-Platzhalter: " << platzhalter.size()
           << ", inerte Defer-Dateien auf Platte: " << inert.size();
    for (auto const& p : eingebunden) { befund << "\n  [EINGEBUNDEN] " << p.string(); }
    for (auto const& n : platzhalter) { befund << "\n  [PLATZHALTER] " << n; }
    EXPECT_TRUE(eingebunden.empty()) << befund.str();
    EXPECT_TRUE(platzhalter.empty()) << befund.str();
    std::cout << befund.str() << "\n";
}

// (3) DIE URSACHE VON 2026-08-08 BLEIBT GEHEILT. enable_testing() gilt nur fuer das aktuelle
//     Verzeichnis und alle DANACH betretenen. Rutscht der Aufruf wieder hinter
//     add_subdirectory(libs/cache_engine), verschwindet unterhalb libs/ jede CTestTestfile.cmake --
//     lautlos, ohne CMake-Warnung. Genau diese Datei wird hier am Objekt geprueft.
TEST(D2G1CtestRegistrierung, LibsUnterbaumTraegtCtestTestfile) {
    fs::path const  bau        = fs::path(kBaubaum);
    std::size_t     gesamt     = 0;
    std::size_t     unter_libs = 0;
    std::error_code ec;
    ASSERT_TRUE(fs::is_directory(bau, ec)) << "Baubaum nicht auffindbar: " << kBaubaum;
    for (fs::recursive_directory_iterator it(bau, ec), ende; it != ende; it.increment(ec)) {
        if (ec || !it->is_regular_file(ec) || it->path().filename() != "CTestTestfile.cmake") { continue; }
        ++gesamt;
        if (it->path().string().find("/libs/") != std::string::npos) { ++unter_libs; }
    }
    ASSERT_GT(gesamt, 0u) << "NENNER LEER: keine CTestTestfile.cmake unter " << kBaubaum;

    fs::path const zelle = bau / "libs/cache_engine/builder/commands/tests/CTestTestfile.cmake";
    std::cout << "NENNER: " << gesamt << " CTestTestfile.cmake im Baubaum, davon " << unter_libs
              << " unterhalb libs/\n";
    ASSERT_TRUE(fs::exists(zelle, ec)) << "FEHLT: " << zelle.string()
                                       << "\nenable_testing() steht wieder NACH add_subdirectory(libs/cache_engine).";

    std::string const text = datei_inhalt(zelle);
    EXPECT_NE(text.find("test_commands"), std::string::npos) << "CTestTestfile ohne test_commands: " << text;
    EXPECT_NE(text.find("test_engine_adapters"), std::string::npos)
        << "CTestTestfile ohne test_engine_adapters: " << text;
}

// (4) GEGENEINGANG (T-4). Die drei Zusicherungen oben sind nur etwas wert, wenn der Sucher eine
//     FEHLENDE Registrierung ueberhaupt bemerkt. Hier bekommt er einen kuenstlichen Quellbaum, in dem
//     genau eine gtest-Binary deklariert und KEINE registriert ist -- er MUSS sie melden. Ohne diesen
//     Fall koennte deklarierte_gtest_binaries() konstant die leere Menge liefern und (1) waere fuer
//     immer gruen: eine Wache, die nie rot wird, ist so wertlos wie eine, die immer rot ist.
TEST(D2G1CtestRegistrierung, GegeneingangDerSucherMeldetEineFehlendeRegistrierung) {
    // Fester Name statt Zufall (Owner-KERN: "Wir arbeiten NIE mit Zufall -- ausser fuer Koeder"):
    // vorher wegraeumen macht den Lauf wiederholbar, auch nach einem Abbruch im vorigen Durchgang.
    fs::path const  wurzel = fs::temp_directory_path() / "comdare_d2g1_gegeneingang";
    fs::path const  zelle  = wurzel / "libs" / "irgendwas" / "tests";
    std::error_code ec;
    fs::remove_all(wurzel, ec);
    fs::create_directories(zelle, ec);
    ASSERT_FALSE(ec) << "Gegeneingang liess sich nicht anlegen: " << wurzel.string();

    {
        std::ofstream out(zelle / "CMakeLists.txt");
        out << "add_executable(test_gegeneingang_nie_registriert quelle.cpp)\n";
        out << "target_link_libraries(test_gegeneingang_nie_registriert PRIVATE GTest::gtest_main)\n";
        // Auskommentierter Aufruf: der Sucher darf ihn NICHT als Deklaration zaehlen.
        out << "# add_executable(test_gegeneingang_auskommentiert quelle.cpp)\n";
    }

    std::size_t                 cmakelists_gesehen = 0;
    std::set<std::string> const deklariert         = deklarierte_gtest_binaries(wurzel / "libs", cmakelists_gesehen);
    fs::remove_all(wurzel, ec);

    EXPECT_EQ(cmakelists_gesehen, 1u) << "der Sucher hat den kuenstlichen Quellbaum nicht gelesen";
    EXPECT_EQ(deklariert.size(), 1u) << "erwartet genau EINE Deklaration (die auskommentierte zaehlt nicht)";
    EXPECT_TRUE(deklariert.find("test_gegeneingang_nie_registriert") != deklariert.end())
        << "der Sucher hat die deklarierte Binary nicht gefunden -- (1) waere damit blind";
    EXPECT_TRUE(deklariert.find("test_gegeneingang_auskommentiert") == deklariert.end())
        << "der Sucher zaehlt auskommentierte add_executable mit -- er wuerde Phantome melden";
}
