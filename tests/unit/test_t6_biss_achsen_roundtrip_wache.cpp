// test_t6_biss_achsen_roundtrip_wache -- P3 der T-6-Priorisierung.          (2026-08-10)
// =============================================================================
// PRUEFLING: scripts/ci_achsen_roundtrip_wache.sh
//
// WARUM DIESE WACHE UEBERHAUPT ZAEHLT -- sie ist die einzige mit fremdem Nenner:
// Die drei anderen Wachen der Abdeckungs-Kette vergleichen SOLL gegen IST, wobei der
// SOLL aus derselben tests/unit/CMakeLists.txt kommt, die geprueft wird. Loescht
// jemand einen der drei Registry-Roundtrip-Bloecke KOMPLETT, sinken SOLL und IST
// gemeinsam -- alle drei bleiben gruen, und das Gate ist trotzdem weg (V-7). Diese
// Wache traegt ihren Nenner als LITERAL in sich und ist damit die einzige, die eine
// vollstaendige Loeschung ueberhaupt bemerken kann.
// Genau deshalb ist ihr eigener Biss ungedeckt besonders teuer: sie ist die letzte
// Instanz einer ganzen Kette, und sie hatte bis heute keinen Selbsttest.
//
// WARUM EIN GOOGLE TEST UND KEINE WEITERE SHELL-PROBE (Owner-KERN 09.08.2026):
//   "Ich sehe einen Haufen shells statt vernuenftiger google tests, was soll das? Es
//    waere sauberer im cmake-Debug Modus standard google Tests zu fahren und diese in
//    Release zu wiederholen aufgrund von compile regressionen. SKRIPTE SAGEN GAR
//    NICHTS."
// Ein neuer *.selbsttest.sh vergroesserte genau die Menge, die weg soll.
//
// DIE DREI ZUSTAENDE, DIE HIER GETRENNT BLEIBEN (das ist der Kern der Korrektur):
//   Exit 0 = die drei Gates stehen           -> GRUEN, und der Fall verlangt es auch
//   Exit 1 = Anzahl/Namensmenge weicht ab    -> der BISS, mit dem erwarteten Literal
//   Exit 2 = die Wache konnte nicht pruefen  -> fail-closed, ausdruecklich KEIN Biss
// Ein Fall, der nur 'nicht 0' prueft, wuerde einen Werkzeug-Ausfall als Fang verbuchen.
// Genau das hat die abgeloeste Shell-Form am 09.08. getan (rc=127 galt als Biss).
//
// K13 -- DER KOEDER WIRD GEWUERFELT UND WOERTLICH ZURUECKGEFORDERT:
// Der vierte, unerwartete Gate-Name traegt eine Marke aus /dev/urandom. Sie kommt in
// KEINER Datei dieses Repos vor; steht sie in der Ausgabe der Wache, kann sie nur aus
// dem Baum stammen, den dieser Fall gerade angelegt hat. Die GEGENPROBE steht daneben:
// derselbe Baum ohne den vierten Namen bleibt GRUEN (T-4).
//
// MUTATIONS-NACHWEIS (T-1): COMDARE_T6_ACHSEN_WACHE_PFAD schiebt einen anderen
// Prueflig unter. Damit laesst sich dieselbe Test-Binary gegen eine praeparierte Wache
// fahren -- z.B. eine, der der Vergleich fehlt. Sie MUSS dann rot werden. Welcher
// Prueflig gefahren wurde, steht in der Ausgabe JEDES Falls.
//
// GRENZE, EHRLICH BENANNT (T-9): geprueft wird das URTEIL der Wache ueber eine
// praeparierte Inventur. Dass die drei Gates im ECHTEN Baum registriert sind, sagt
// dieser Test nicht -- das sagt die Wache selbst, und dass sie in der CI laeuft, sagt
// test_t6_wachen_inventar.
//
// ASCII-only, Zeilen <= 120 Byte.
// =============================================================================

#include <gtest/gtest.h>

#if defined(_WIN32)

TEST(T6BissAchsenRoundtripWache, NurPosix) {
    GTEST_SKIP() << "scripts/ci_achsen_roundtrip_wache.sh ist ein POSIX-sh-Skript (kein Windows-Job faehrt es).";
}

#else

#include "support/wachen_werkbank.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using comdare::test::wachen::enthaelt;
using comdare::test::wachen::fahre;
using comdare::test::wachen::koeder;
using comdare::test::wachen::Lauf;
using comdare::test::wachen::mit_ctest_im_pfad;
using comdare::test::wachen::schreibe_inventur;
using comdare::test::wachen::zitiert;

namespace {

// Die DREI Gates, die die Wache als Literal fuehrt. Sie stehen hier ein zweites Mal --
// bewusst: waere die Liste importiert, pruefte der Test die Wache gegen sich selbst.
constexpr char const* kGates[] = {"test_axis_registry_roundtrip", "test_system_axis_registry_roundtrip",
                                  "test_measurement_axis_registry_roundtrip"};

[[nodiscard]] std::string wachen_pfad() {
    if (char const* const ueberschrieben = std::getenv("COMDARE_T6_ACHSEN_WACHE_PFAD");
        ueberschrieben != nullptr && *ueberschrieben != '\0') {
        return std::string{ueberschrieben};
    }
    return std::string{COMDARE_T6_ACHSEN_WACHE};
}

[[nodiscard]] Lauf wache_fahren(fs::path const& baum) {
    return fahre(mit_ctest_im_pfad(COMDARE_T6_CTEST_BIN) + " sh " + zitiert(wachen_pfad()) + " " + zitiert(baum));
}

void berichten(char const* fall, Lauf const& lauf, std::string const& marke) {
    std::cout << "  [T6/P3] Fall '" << fall << "' | Koeder " << marke << " | Prueflig " << wachen_pfad() << " | Exit "
              << lauf.code << "\n";
}

class Baum {
public:
    explicit Baum(std::string const& marke) : pfad_{comdare::test::user_tmp_dir() / ("t6_achsen_" + marke)} {
        std::error_code ec;
        fs::remove_all(pfad_, ec);
        fs::create_directories(pfad_, ec);
    }
    Baum(Baum const&)            = delete;
    Baum& operator=(Baum const&) = delete;
    ~Baum() {
        std::error_code ec;
        fs::remove_all(pfad_, ec);
    }
    [[nodiscard]] fs::path const& pfad() const { return pfad_; }

private:
    fs::path pfad_;
};

// Der gesunde Baum: die drei Gates plus ein Fuellsel, das das Suchmuster NICHT trifft.
// Ohne das Fuellsel waere 'Gesamt > 0' allein durch die Gates erfuellt und der Fall
// koennte nicht zeigen, dass die Wache aus einer groesseren Menge auswaehlt.
[[nodiscard]] std::vector<std::string> gesunde_inventur(std::string const& marke) {
    std::vector<std::string> namen{"test_fuellsel_" + marke};
    for (char const* g : kGates) { namen.emplace_back(g); }
    return namen;
}

} // namespace

// =============================================================================
// (0) DER GESUNDE GEGENSTAND IST GRUEN. Ohne diesen Fall waere jede Rot-Zusicherung
//     unten auch durch eine Wache erfuellt, die IMMER 1 zurueckgibt (Daueralarm, T-1).
//     Er ist zugleich der GEGENKOEDER: derselbe Baum, nur ohne die Manipulation.
// =============================================================================
TEST(T6BissAchsenRoundtripWache, DreiGatesSindGRUEN) {
    std::string const marke = koeder();
    Baum              baum{marke};
    ASSERT_TRUE(schreibe_inventur(baum.pfad(), gesunde_inventur(marke)));

    Lauf const lauf = wache_fahren(baum.pfad());
    berichten("gesund", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << "Der gesunde Baum MUSS gruen sein -- sonst ist die Wache ein Daueralarm.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "ACHSEN-ROUNDTRIP-WACHE: OK")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "3 von 3")) << "Der Nenner gehoert in die Ausgabe.\n" << lauf.ausgabe;
    // Beleg, dass die Wache aus einer GROESSEREN Menge ausgewaehlt hat: das Fuellsel
    // zaehlt in die Grundgesamtheit, nicht in die Treffer.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "aus 4 ctest-Eintraegen")) << lauf.ausgabe;
}

// =============================================================================
// (1) EIN GATE VERSCHWINDET -- der Fall, gegen den die Wache gebaut ist.
//     Verlangt wird nicht 'irgendwie rot', sondern Exit 1 UND der Name des fehlenden
//     Gates woertlich. 'Irgendetwas ist gebrochen' genuegt nicht.
// =============================================================================
TEST(T6BissAchsenRoundtripWache, FehlendesGateIstExit1UndWirdNamentlichGenannt) {
    std::string const marke = koeder();
    Baum              baum{marke};

    std::vector<std::string> namen{"test_fuellsel_" + marke};
    namen.emplace_back(kGates[0]);
    namen.emplace_back(kGates[1]);
    // kGates[2] fehlt -- genau das ist die stille Loeschung.
    ASSERT_TRUE(schreibe_inventur(baum.pfad(), namen));

    Lauf const lauf = wache_fahren(baum.pfad());
    berichten("Gate fehlt", lauf, marke);

    EXPECT_EQ(lauf.code, 1) << "Ein fehlendes Gate MUSS Exit 1 geben -- nicht 0 und nicht 2 (2 hiesse: die Wache "
                               "konnte nicht pruefen).\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "FEHLT (im SOLL, nicht im Baum)")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, kGates[2])) << "Das fehlende Gate muss NAMENTLICH erscheinen.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "2 von 3")) << lauf.ausgabe;
}

// =============================================================================
// (2) K13 -- DER GEWUERFELTE KOEDER. Ein vierter Name, der das Muster trifft, muss
//     als UNERWARTET auftauchen, und zwar WOERTLICH. Die Marke steht in keiner Datei
//     dieses Repos: erscheint sie in der Ausgabe, hat die Wache wirklich gemessen.
// =============================================================================
TEST(T6BissAchsenRoundtripWache, ZusaetzlichesGateBeisstWoertlich) {
    std::string const marke        = koeder();
    std::string const eindringling = "test_koeder_" + marke + "_axis_registry_roundtrip";
    Baum              baum{marke};

    std::vector<std::string> namen = gesunde_inventur(marke);
    namen.push_back(eindringling);
    ASSERT_TRUE(schreibe_inventur(baum.pfad(), namen));

    Lauf const lauf = wache_fahren(baum.pfad());
    berichten("vierter Gate-Name", lauf, marke);

    EXPECT_EQ(lauf.code, 1) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "UNERWARTET (im Baum, nicht im SOLL)")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, eindringling))
        << "Der gewuerfelte Koeder MUSS woertlich in der Ausgabe stehen -- sonst hat die Wache ihn nie gesehen.\n"
        << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "4 von 3")) << lauf.ausgabe;
}

// =============================================================================
// (3) FAIL-CLOSED I: ein Baum ohne einen einzigen ctest-Eintrag. Ein Nichtfund waere
//     hier nicht von einer Loeschung zu unterscheiden -- die Wache MUSS abbrechen und
//     darf ausdruecklich NICHT rot-als-Befund melden (Exit 2, nicht 1, nicht 0).
// =============================================================================
TEST(T6BissAchsenRoundtripWache, LeererBaumIstAbbruchStattBefund) {
    std::string const marke = koeder();
    Baum              baum{marke};
    ASSERT_TRUE(schreibe_inventur(baum.pfad(), {}));

    Lauf const lauf = wache_fahren(baum.pfad());
    berichten("leerer Baum", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << "Ein leerer Nenner ist ein ABBRUCH, kein Befund -- und erst recht kein Gruen.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "meldet 0 Tests")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ACHSEN-ROUNDTRIP-WACHE: OK")) << lauf.ausgabe;
}

// =============================================================================
// (4) FAIL-CLOSED II: kein CTestTestfile.cmake. Der Baum ist nicht konfiguriert; jede
//     Zaehlung waere null und wuerde eine Loeschung vortaeuschen.
// =============================================================================
TEST(T6BissAchsenRoundtripWache, UnkonfigurierterBaumIstAbbruch) {
    std::string const marke = koeder();
    Baum              baum{marke}; // angelegt, aber ohne CTestTestfile.cmake

    Lauf const lauf = wache_fahren(baum.pfad());
    berichten("kein CTestTestfile", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "Fail-closed")) << lauf.ausgabe;
}

// =============================================================================
// (5) FAIL-CLOSED III: gar kein Verzeichnis. Der Aufrufer hat sich vertan -- auch das
//     ist ABBRUCH und nicht 'nichts gefunden, also in Ordnung'.
// =============================================================================
TEST(T6BissAchsenRoundtripWache, FehlendesVerzeichnisIstAbbruch) {
    std::string const marke    = koeder();
    fs::path const    nirgends = comdare::test::user_tmp_dir() / ("t6_achsen_gibtesnicht_" + marke);

    Lauf const lauf =
        fahre(mit_ctest_im_pfad(COMDARE_T6_CTEST_BIN) + " sh " + zitiert(wachen_pfad()) + " " + zitiert(nirgends));
    berichten("Verzeichnis fehlt", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "ist kein Verzeichnis")) << lauf.ausgabe;
}

#endif // _WIN32
