// test_t6_biss_sichtbarkeits_wache -- P1 der T-6-Priorisierung.            (2026-08-10)
// =============================================================================
// PRUEFLING: scripts/ci_test_sichtbarkeit_wache.sh
//
// WARUM SIE P1 IST -- sie steht am Anfang der Abdeckungs-Kette:
//   Stufe 1  ci_test_registrierungs_wache.sh   Quelldatei -> Bauweg
//   Stufe 2  ci_test_sichtbarkeit_wache.sh     Registrierung -> ctest-Eintrag   <== hier
//   Stufe 3  ci_test_coverage_guard.sh         ctest-Inventur -> Job-Auswahl
// Stufe 3 rechnet ueber der Inventur, die Stufe 2 herstellt. Ist Stufe 2 blind, rechnet
// Stufe 3 korrekt ueber einer zu kleinen Menge und meldet GRUEN -- am Objekt belegt:
// 27 gtest-Faelle aus libs/cache_engine/builder/commands/tests standen 79 Tage lang in
// keiner Inventur, weil enable_testing() in der Wurzel NACH dem add_subdirectory kam.
// Eine Wache, deren Biss niemand nachfaehrt, kann genau so still konstant gruen sein.
//
// WARUM EIN GOOGLE TEST UND KEIN *.selbsttest.sh (Owner-KERN 09.08.2026):
//   "Ich sehe einen Haufen shells statt vernuenftiger google tests, was soll das? ...
//    SKRIPTE SAGEN GAR NICHTS."
//
// DIE DREI ZUSTAENDE, DIE HIER GETRENNT BLEIBEN:
//   Exit 0 = alles sichtbar oder begruendet abwesend -> GRUEN, und ein Fall verlangt es
//   Exit 1 = unsichtbare Registrierung ohne Grund    -> der BISS, mit dem Literal
//   Exit 2 = die Wache konnte nicht pruefen          -> fail-closed, KEIN Biss
//
// DER FALL, DEN DIESE DATEI VOR ALLEM BEWEIST -- die MEHRZEILIGE Schreibweise:
// Die erste Fassung der Wache suchte je EINE Zeile und war damit blind fuer
//     add_test(
//         NAME test_x
//         COMMAND ...)
// Drei real registrierte Gates (test_axis_registry_roundtrip und die zwei Schwestern)
// standen deshalb gar nicht erst in ihrem SOLL -- ihr Verschwinden waere nie gemeldet
// worden. Die Reparatur (ein awk-Zustandsautomat) war bis heute durch NICHTS gedeckt.
// Fall (4) unten faehrt genau diese Schreibweise mit einem gewuerfelten Namen.
//
// K13: jeder Fall wuerfelt seine Kennungen frisch aus /dev/urandom. Sie kommen in keiner
// Datei dieses Repos vor; stehen sie in der Ausgabe der Wache, hat sie wirklich gemessen.
// GEGENKOEDER: Fall (0) faehrt denselben Baum ohne Manipulation und verlangt GRUEN.
//
// MUTATIONS-NACHWEIS (T-1): COMDARE_T6_SICHT_WACHE_PFAD schiebt einen anderen Prueflig
// unter; welcher gefahren wurde, steht in der Ausgabe jedes Falls.
//
// GRENZE, EHRLICH BENANNT (T-9): geprueft wird das URTEIL der Wache ueber ein
// praepariertes Wegwerf-Repo. Ob sie im ECHTEN Baum die richtige Menge sieht, sagt sie
// selbst (ihr Nenner steht in ihrer Ausgabe); dass sie in der CI ueberhaupt gerufen
// wird, sagt test_t6_wachen_inventar. Die drei Aussagen ersetzen einander nicht.
//
// ASCII-only, Zeilen <= 120 Byte.
// =============================================================================

#include <gtest/gtest.h>

#if defined(_WIN32)

TEST(T6BissSichtbarkeitsWache, NurPosix) {
    GTEST_SKIP() << "scripts/ci_test_sichtbarkeit_wache.sh ist ein POSIX-sh-Skript (kein Windows-Job faehrt es).";
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
using comdare::test::wachen::WegwerfRepo;
using comdare::test::wachen::zitiert;

namespace {

[[nodiscard]] std::string wachen_pfad() {
    if (char const* const ueberschrieben = std::getenv("COMDARE_T6_SICHT_WACHE_PFAD");
        ueberschrieben != nullptr && *ueberschrieben != '\0') {
        return std::string{ueberschrieben};
    }
    return std::string{COMDARE_T6_SICHT_WACHE};
}

// Die Wache bestimmt ihre Repo-Wurzel selbst und wechselt dorthin -- gefahren wird
// deshalb AUS dem Wegwerf-Repo heraus, mit dem Bau-Baum als Argument.
[[nodiscard]] Lauf wache_fahren(WegwerfRepo const& repo, fs::path const& baum) {
    return fahre("cd " + zitiert(repo.pfad()) + " && " + WegwerfRepo::umgebung() + " " +
                 mit_ctest_im_pfad(COMDARE_T6_CTEST_BIN) + " sh " + zitiert(wachen_pfad()) + " " + zitiert(baum));
}

void berichten(char const* fall, Lauf const& lauf, std::string const& marke) {
    std::cout << "  [T6/P1] Fall '" << fall << "' | Koeder " << marke << " | Prueflig " << wachen_pfad() << " | Exit "
              << lauf.code << "\n";
}

// Ein Repo, in dem GENAU die Registrierungen stehen, die der Fall braucht. Der Bau-Baum
// liegt bewusst AUSSERHALB des Repos: er darf nicht als getrackte Datei mitgescannt
// werden, sonst faende die Wache ihre eigene Fixture-Inventur als Quelltext wieder.
class Fall {
public:
    explicit Fall(std::string const& marke) : marke_{marke}, repo_{marke}, baum_{repo_.pfad().string() + "_baum"} {
        std::error_code ec;
        fs::create_directories(baum_, ec);
    }
    Fall(Fall const&)            = delete;
    Fall& operator=(Fall const&) = delete;
    ~Fall() {
        std::error_code ec;
        fs::remove_all(baum_, ec);
    }

    [[nodiscard]] testing::AssertionResult aufbauen(std::string const& cmake_inhalt) {
        testing::AssertionResult r = repo_.init();
        if (!r) { return r; }
        r = repo_.ist_eigene_wurzel();
        if (!r) { return r; }
        return repo_.schreibe_und_verfolge("CMakeLists.txt", cmake_inhalt);
    }

    [[nodiscard]] WegwerfRepo&       repo() { return repo_; }
    [[nodiscard]] fs::path const&    baum() const { return baum_; }
    [[nodiscard]] std::string const& marke() const { return marke_; }

private:
    std::string marke_;
    WegwerfRepo repo_;
    fs::path    baum_;
};

// Zwei Registrierungen in der EINZEILIGEN Hausform.
[[nodiscard]] std::string cmake_einzeilig(std::string const& a, std::string const& b) {
    return "comdare_add_test(" + a + " SOURCES " + a + ".cpp)\n" + "comdare_add_test(" + b + " SOURCES " + b +
           ".cpp)\n";
}

// EINE Registrierung in der MEHRZEILIGEN Schreibweise -- der historische blinde Fleck.
[[nodiscard]] std::string cmake_mehrzeilig(std::string const& name) {
    return "add_test(\n    NAME " + name + "\n    COMMAND /bin/true)\n";
}

} // namespace

// =============================================================================
// (0) DER GESUNDE GEGENSTAND IST GRUEN -- und das ist zugleich der GEGENKOEDER.
//     Ohne diesen Fall waere jede Rot-Zusicherung unten auch durch eine Wache erfuellt,
//     die immer 1 zurueckgibt (Daueralarm; T-1 verbietet ihn ausdruecklich).
// =============================================================================
TEST(T6BissSichtbarkeitsWache, AllesSichtbarIstGRUEN) {
    std::string const marke = koeder();
    std::string const a     = "test_sicht_a_" + marke;
    std::string const b     = "test_sicht_b_" + marke;
    Fall              fall{marke};

    ASSERT_TRUE(fall.aufbauen(cmake_einzeilig(a, b)));
    ASSERT_TRUE(schreibe_inventur(fall.baum(), {a, b}));

    Lauf const lauf = wache_fahren(fall.repo(), fall.baum());
    berichten("alles sichtbar", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << "Der gesunde Baum MUSS gruen sein.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "TEST-SICHTBARKEITS-WACHE: OK")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "2 Registrierung(en) im Quelltext"))
        << "Der Nenner gehoert in die Ausgabe (V-1).\n"
        << lauf.ausgabe;
}

// =============================================================================
// (1) DER BISS: eine Registrierung steht im Quelltext und in keiner Inventur.
//     Verlangt wird Exit 1 UND der gewuerfelte Name woertlich -- 'irgendwie rot'
//     genuegt nicht, ein Mutant gilt erst mit seinem eigenen Riss-Literal als getoetet.
// =============================================================================
TEST(T6BissSichtbarkeitsWache, UnsichtbareRegistrierungBeisstWoertlich) {
    std::string const marke     = koeder();
    std::string const sichtbar  = "test_sicht_da_" + marke;
    std::string const verborgen = "test_sicht_weg_" + marke;
    Fall              fall{marke};

    ASSERT_TRUE(fall.aufbauen(cmake_einzeilig(sichtbar, verborgen)));
    ASSERT_TRUE(schreibe_inventur(fall.baum(), {sichtbar})); // 'verborgen' fehlt

    Lauf const lauf = wache_fahren(fall.repo(), fall.baum());
    berichten("unsichtbare Registrierung", lauf, marke);

    EXPECT_EQ(lauf.code, 1) << "Eine unsichtbare Registrierung MUSS Exit 1 geben -- nicht 2 (das hiesse: die Wache "
                               "konnte nicht pruefen) und erst recht nicht 0.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "UNSICHTBARE REGISTRIERUNGEN OHNE BEGRUENDUNG")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, verborgen))
        << "Der gewuerfelte Koeder MUSS woertlich erscheinen -- sonst hat die Wache ihn nie gesehen.\n"
        << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "TEST-SICHTBARKEITS-WACHE: OK")) << lauf.ausgabe;
}

// =============================================================================
// (2) T-4 GEGENEINGANG: derselbe Baum, derselbe Koeder -- nur steht die Abwesenheit
//     jetzt MIT BEGRUENDUNG in der Allowlist. Dann darf die Wache gerade NICHT rot
//     werden. Ohne diesen Fall waere (1) auch durch eine Wache erfuellt, die jede
//     Allowlist ignoriert.
// =============================================================================
TEST(T6BissSichtbarkeitsWache, BegruendeteAbwesenheitIstKeinBefund) {
    std::string const marke     = koeder();
    std::string const sichtbar  = "test_sicht_da_" + marke;
    std::string const verborgen = "test_sicht_weg_" + marke;
    Fall              fall{marke};

    ASSERT_TRUE(fall.aufbauen(cmake_einzeilig(sichtbar, verborgen)));
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_sichtbarkeit_allowlist.txt",
                                     "# Wegwerf-Allowlist des Falls\n" + verborgen +
                                         "\tbraucht den optionalen Fremdbaum, hier abwesend\n"));
    ASSERT_TRUE(schreibe_inventur(fall.baum(), {sichtbar}));

    Lauf const lauf = wache_fahren(fall.repo(), fall.baum());
    berichten("begruendet abwesend", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "UNSICHTBAR, ABER BEGRUENDET")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, verborgen)) << lauf.ausgabe;
    // Beleg, dass die Wache wirklich durchgelaufen ist und nicht frueh abgebrochen hat --
    // sonst waere 'kein Befund' nur die Abwesenheit jeder Aussage.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "TEST-SICHTBARKEITS-WACHE: OK")) << lauf.ausgabe;
}

// =============================================================================
// (3) DIE ALLOWLIST OHNE BEGRUENDUNG ist selbst ein Fehler -- sonst waere sie der
//     stille Rueckfall, gegen den die Wache gebaut ist. Der Tabulator-Fall ist hier
//     Absicht: 'name<TAB>' galt einmal als begruendet ("cut -d' ' -f2-" gibt eine
//     Zeile ohne Leerzeichen VOLLSTAENDIG zurueck).
// =============================================================================
TEST(T6BissSichtbarkeitsWache, AllowlistOhneBegruendungIstSelbstEinFehler) {
    std::string const marke     = koeder();
    std::string const sichtbar  = "test_sicht_da_" + marke;
    std::string const verborgen = "test_sicht_weg_" + marke;
    Fall              fall{marke};

    ASSERT_TRUE(fall.aufbauen(cmake_einzeilig(sichtbar, verborgen)));
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_sichtbarkeit_allowlist.txt", verborgen + "\t\n"));
    ASSERT_TRUE(schreibe_inventur(fall.baum(), {sichtbar}));

    Lauf const lauf = wache_fahren(fall.repo(), fall.baum());
    berichten("Allowlist ohne Grund", lauf, marke);

    EXPECT_EQ(lauf.code, 1) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "Allowlist-Zeile(n) ohne Begruendung")) << lauf.ausgabe;
}

// =============================================================================
// (4) DER HISTORISCHE BLINDE FLECK: die MEHRZEILIGE Registrierung. Die erste Fassung
//     der Wache sah sie nicht und haette das Verschwinden von drei realen Gates nie
//     gemeldet. Der Zustandsautomat, der das repariert hat, war bis heute ungedeckt.
// =============================================================================
TEST(T6BissSichtbarkeitsWache, MehrzeiligeRegistrierungStehtImSOLL) {
    std::string const marke     = koeder();
    std::string const sichtbar  = "test_sicht_da_" + marke;
    std::string const verborgen = "test_mehrzeilig_" + marke;
    Fall              fall{marke};

    // Der einzeilige Nachbar ist noetig: er belegt, dass der Scanner ueberhaupt sucht.
    // Ohne ihn koennte ein kaputter Scanner mit 0 Registrierungen abbrechen (Exit 2)
    // und der Fall haette den mehrzeiligen Fund nicht von einem Werkzeug-Ausfall
    // getrennt.
    ASSERT_TRUE(fall.aufbauen("comdare_add_test(" + sichtbar + " SOURCES " + sichtbar + ".cpp)\n" +
                              cmake_mehrzeilig(verborgen)));
    ASSERT_TRUE(schreibe_inventur(fall.baum(), {sichtbar}));

    Lauf const lauf = wache_fahren(fall.repo(), fall.baum());
    berichten("mehrzeilige Registrierung", lauf, marke);

    EXPECT_EQ(lauf.code, 1) << "Die mehrzeilige Schreibweise MUSS im SOLL landen -- sonst ist die Wache fuer genau "
                               "die Bloecke blind, um derentwillen sie umgebaut wurde.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, verborgen)) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "2 Registrierung(en) im Quelltext"))
        << "Beide Schreibweisen muessen im Nenner stehen, die einzeilige und die mehrzeilige.\n"
        << lauf.ausgabe;
}

// =============================================================================
// (5) FAIL-CLOSED I: ein Bau-Baum ohne einen einzigen ctest-Eintrag. Dann waere JEDE
//     Registrierung unsichtbar -- ein Befund, der nur aus einer kaputten Messung kommt.
//     Die Wache MUSS abbrechen (Exit 2), nicht 1 melden.
// =============================================================================
TEST(T6BissSichtbarkeitsWache, LeereInventurIstAbbruchStattBefund) {
    std::string const marke = koeder();
    std::string const a     = "test_sicht_a_" + marke;
    std::string const b     = "test_sicht_b_" + marke;
    Fall              fall{marke};

    ASSERT_TRUE(fall.aufbauen(cmake_einzeilig(a, b)));
    ASSERT_TRUE(schreibe_inventur(fall.baum(), {}));

    Lauf const lauf = wache_fahren(fall.repo(), fall.baum());
    berichten("leere Inventur", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << "Eine leere Inventur ist ein ABBRUCH, kein Befund.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "meldet 0 Tests")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "UNSICHTBARE REGISTRIERUNGEN OHNE BEGRUENDUNG")) << lauf.ausgabe;
}

// =============================================================================
// (6) FAIL-CLOSED II: ein Repo ohne eine einzige CMake-Datei. Der SOLL waere leer und
//     jede Aussage waere wahr.
// =============================================================================
TEST(T6BissSichtbarkeitsWache, OhneCMakeDateiIstAbbruch) {
    std::string const marke = koeder();
    Fall              fall{marke};

    ASSERT_TRUE(fall.repo().init());
    ASSERT_TRUE(fall.repo().ist_eigene_wurzel());
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge("LIESMICH.txt", "kein CMake in diesem Repo\n"));
    ASSERT_TRUE(schreibe_inventur(fall.baum(), {"test_irgendwas_" + marke}));

    Lauf const lauf = wache_fahren(fall.repo(), fall.baum());
    berichten("kein CMake", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "0 CMakeLists.txt gefunden")) << lauf.ausgabe;
}

#endif // _WIN32
