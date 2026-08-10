// test_t6_biss_registrierungs_wache -- P2 der T-6-Priorisierung.           (2026-08-10)
// =============================================================================
// PRUEFLING: scripts/ci_test_registrierungs_wache.sh
//
// WARUM SIE P2 IST -- sie ist die UNTERSTE Stufe der Abdeckungs-Kette:
//   Stufe 1  ci_test_registrierungs_wache.sh   Quelldatei -> Bauweg          <== hier
//   Stufe 2  ci_test_sichtbarkeit_wache.sh     Registrierung -> ctest-Eintrag
//   Stufe 3  ci_test_coverage_guard.sh         ctest-Inventur -> Job-Auswahl
// Eine Test-Quelldatei, die gar kein add_test aufruft, steht im SOLL von Stufe 2 nie
// drin und kann von ihr nicht vermisst werden. Genau so lagen vier Dateien 14 bzw. 68
// Tage im Repo, ohne je gebaut zu werden; eine davon war beim ersten Lauf sofort ROT.
// Faellt Stufe 1 aus, ist die ganze Kette darueber blind fuer eine ganze Fehlerklasse.
//
// WARUM EIN GOOGLE TEST UND KEIN *.selbsttest.sh (Owner-KERN 09.08.2026):
//   "Ich sehe einen Haufen shells statt vernuenftiger google tests, was soll das? ...
//    SKRIPTE SAGEN GAR NICHTS."
//
// DIE DREI ZUSTAENDE, DIE HIER GETRENNT BLEIBEN:
//   Exit 0 = im Bauweg oder begruendet abwesend  -> GRUEN, und ein Fall verlangt es
//   Exit 1 = ohne gueltige Begruendung ausserhalb -> der BISS, mit dem Literal
//   Exit 2 = die Wache konnte nicht pruefen       -> fail-closed, KEIN Biss
//
// EXIT 1 HAT DREI EINGAENGE, und der Test deckt jeden einzeln: gar keine Allowlist-Zeile
// (1), eine ERLOSCHENE (3), eine UNPRUEFBARE (7). Bis zum 10.08. fehlte der dritte --
// die Sammellandung hat genau diese Luecke gefunden, s. Kopf von Fall (7).
//
// DIE DREI EIGENSCHAFTEN, DIE BIS HEUTE REINE BEHAUPTUNG WAREN:
//  (a) DIE ERLOSCHENE AUSNAHME. Jede Allowlist-Zeile nennt einen GEGENSTAND, dessen
//      Abwesenheit die Ausnahme traegt. Existiert er wieder, ist die Ausnahme erloschen
//      und die Zeile wird ROT -- auch wenn niemand die Allowlist angefasst hat. Ohne
//      diese Gegenprobe waere die Allowlist ein Freibrief mit unbegrenzter Laufzeit.
//      Fall (3) unten legt den Gegenstand an und verlangt genau diesen Riss.
//  (b) DIE MESSGERAET-GEGENPROBE. Bevor ein Nullbefund etwas bedeutet, muss belegt
//      sein, dass das Werkzeug ueberhaupt sucht. Fall (5) nimmt der Wache den Beleg
//      und verlangt ABBRUCH -- nicht "nichts gefunden, also in Ordnung".
//  (c) DIE ART DER BEGRUENDUNG. Feld 2 nennt seit MT-L4 (461776ef) ein PRAEFIX
//      ('datei:' oder 'isa:'), das die Gegenstandsart benennt. Eine Zeile ohne
//      bekannte Art ist nicht nachpruefbar und damit ROT. Fall (7) faehrt denselben
//      Gegenstand ohne und mit Praefix -- der Unterschied ist das Praefix, sonst nichts.
//
// K13: jeder Fall wuerfelt frisch aus /dev/urandom; die Marken kommen in keiner Datei
// dieses Repos vor. GEGENKOEDER: Fall (0) faehrt denselben Baum ohne Manipulation und
// verlangt GRUEN.
//
// MUTATIONS-NACHWEIS (T-1): COMDARE_T6_REG_WACHE_PFAD schiebt einen anderen Prueflig
// unter; welcher gefahren wurde, steht in der Ausgabe jedes Falls.
//
// GRENZE, EHRLICH BENANNT (T-9) -- eine ungedeckte Stelle, die hier NICHT zugedeckt
// wird: der Abgleich sucht "/<repo-relativer-pfad>" als Teilzeichenkette im Bauweg.
// Enthielte der Bauweg einen ANDEREN Pfad mit demselben Ende -- etwa
// /woanders/tests/unit/test_x.cpp -- galte tests/unit/test_x.cpp als gebaut, obwohl
// eine andere Datei gemeint ist. Fall (6) belegt die Richtung, die HAELT (ein
// namensverwandter Nachbar deckt NICHT); die Gegenrichtung ist gemessen und im
// Abnahmebericht als offene Stelle benannt, nicht stillschweigend uebergangen.
//
// ASCII-only, Zeilen <= 120 Byte.
// =============================================================================

#include <gtest/gtest.h>

#if defined(_WIN32)

TEST(T6BissRegistrierungsWache, NurPosix) {
    GTEST_SKIP() << "scripts/ci_test_registrierungs_wache.sh ist ein POSIX-sh-Skript (kein Windows-Job faehrt es).";
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
using comdare::test::wachen::WegwerfRepo;
using comdare::test::wachen::zitiert;

namespace {

// Die Wache verlangt diese Datei als MESSGERAET-GEGENPROBE: sie muss existieren UND im
// Bauweg stehen, sonst darf die Wache kein Urteil faellen. Der Name steht hier als
// Literal -- die Wache traegt ihn ebenfalls als Literal, und genau dieses Paar wird
// hier gegeneinander gefahren.
constexpr char const* kGegenprobe = "tests/unit/test_pressure_state.cpp";

[[nodiscard]] std::string wachen_pfad() {
    if (char const* const ueberschrieben = std::getenv("COMDARE_T6_REG_WACHE_PFAD");
        ueberschrieben != nullptr && *ueberschrieben != '\0') {
        return std::string{ueberschrieben};
    }
    return std::string{COMDARE_T6_REG_WACHE};
}

[[nodiscard]] Lauf wache_fahren(WegwerfRepo const& repo, fs::path const& baum) {
    return fahre("cd " + zitiert(repo.pfad()) + " && " + WegwerfRepo::umgebung() + " sh " + zitiert(wachen_pfad()) +
                 " " + zitiert(baum));
}

void berichten(char const* fall, Lauf const& lauf, std::string const& marke) {
    std::cout << "  [T6/P2] Fall '" << fall << "' | Koeder " << marke << " | Prueflig " << wachen_pfad() << " | Exit "
              << lauf.code << "\n";
}

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

    [[nodiscard]] testing::AssertionResult init() {
        testing::AssertionResult r = repo_.init();
        if (!r) { return r; }
        r = repo_.ist_eigene_wurzel();
        if (!r) { return r; }
        // Die Gegenprobe-Datei MUSS im Repo liegen, sonst bricht die Wache mit Exit 2
        // ab, bevor sie irgendetwas misst -- der Fall maesse dann die falsche Sache.
        return repo_.schreibe_und_verfolge(kGegenprobe, "// Gegenprobe-Datei des Falls\n");
    }

    // Der Bauweg als compile_commands.json. Die Wache liest sie als reinen Text und
    // sucht "/<repo-relativer-pfad>" -- ein gueltiges JSON ist deshalb nicht noetig,
    // aber es wird trotzdem eines geschrieben: eine Fixture, die nur durch ein Leck im
    // Parser funktioniert, belegt nichts ueber den echten Betrieb.
    [[nodiscard]] testing::AssertionResult bauweg_schreiben(std::vector<std::string> const& relativ) const {
        std::ofstream aus{baum_ / "compile_commands.json", std::ios::trunc};
        if (!aus.good()) { return testing::AssertionFailure() << "compile_commands.json nicht schreibbar"; }
        aus << "[\n";
        for (std::size_t i = 0; i < relativ.size(); ++i) {
            aus << "  {\"directory\": \"" << baum_.string() << "\",\n"
                << "   \"command\": \"c++ -c " << (repo_.pfad() / relativ[i]).string() << "\",\n"
                << "   \"file\": \"" << (repo_.pfad() / relativ[i]).string() << "\"}"
                << (i + 1 == relativ.size() ? "\n" : ",\n");
        }
        aus << "]\n";
        aus.close();
        if (!fs::exists(baum_ / "compile_commands.json")) {
            return testing::AssertionFailure() << "compile_commands.json fehlt nach dem Schreiben";
        }
        return testing::AssertionSuccess();
    }

    [[nodiscard]] WegwerfRepo&       repo() { return repo_; }
    [[nodiscard]] WegwerfRepo const& repo() const { return repo_; }
    [[nodiscard]] fs::path const&    baum() const { return baum_; }

private:
    std::string marke_;
    WegwerfRepo repo_;
    fs::path    baum_;
};

} // namespace

// =============================================================================
// (0) DER GESUNDE GEGENSTAND IST GRUEN -- zugleich der GEGENKOEDER. Ohne ihn waere jede
//     Rot-Zusicherung unten auch durch eine Wache erfuellt, die immer 1 liefert (T-1).
// =============================================================================
TEST(T6BissRegistrierungsWache, AllesImBauwegIstGRUEN) {
    std::string const marke = koeder();
    std::string const datei = "tests/unit/test_reg_" + marke + ".cpp";
    Fall              fall{marke};

    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(datei, "// Koeder-Testquelle\n"));
    ASSERT_TRUE(fall.bauweg_schreiben({kGegenprobe, datei}));

    Lauf const lauf = wache_fahren(fall.repo(), fall.baum());
    berichten("alles im Bauweg", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << "Der gesunde Baum MUSS gruen sein.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "2 getrackte Test-Quelldatei(en)"))
        << "Der Nenner gehoert in die Ausgabe (V-1).\n"
        << lauf.ausgabe;
}

// =============================================================================
// (1) DER BISS: eine getrackte Test-Quelldatei liegt ausserhalb des Bauwegs und hat
//     keine Begruendung. Verlangt wird Exit 1 UND der gewuerfelte Pfad woertlich.
// =============================================================================
TEST(T6BissRegistrierungsWache, WaiseOhneBegruendungBeisstWoertlich) {
    std::string const marke = koeder();
    std::string const waise = "tests/unit/test_waise_" + marke + ".cpp";
    Fall              fall{marke};

    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(waise, "// nie gebaut\n"));
    ASSERT_TRUE(fall.bauweg_schreiben({kGegenprobe})); // die Waise fehlt im Bauweg

    Lauf const lauf = wache_fahren(fall.repo(), fall.baum());
    berichten("Waise ohne Begruendung", lauf, marke);

    EXPECT_EQ(lauf.code, 1) << "Eine unbegruendete Waise MUSS Exit 1 geben -- nicht 2 (das hiesse: die Wache konnte "
                               "nicht pruefen) und erst recht nicht 0.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, waise))
        << "Der gewuerfelte Koeder MUSS woertlich erscheinen -- sonst hat die Wache ihn nie gesehen.\n"
        << lauf.ausgabe;
}

// =============================================================================
// (2) T-4 GEGENEINGANG: dieselbe Waise, jetzt MIT Begruendung, deren Gegenstand
//     wirklich abwesend ist. Dann darf die Wache gerade NICHT rot werden.
// =============================================================================
TEST(T6BissRegistrierungsWache, BegruendeteWaiseIstKeinBefund) {
    std::string const marke = koeder();
    std::string const waise = "tests/unit/test_waise_" + marke + ".cpp";
    std::string const grund = "ext/fremdbaum_" + marke + "/CMakeLists.txt"; // existiert NICHT
    Fall              fall{marke};

    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(waise, "// nie gebaut\n"));
    // 'datei:' benennt die ART des Gegenstands (MT-L4, 461776ef). Ohne Praefix ist die
    // Zeile seit dieser Aenderung UNPRUEFBAR und damit ROT -- Fall (7) faehrt genau das.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Wegwerf-Allowlist des Falls\n" + waise + " | datei:" + grund +
                                         " | braucht den optionalen Fremdbaum\n"));
    ASSERT_TRUE(fall.bauweg_schreiben({kGegenprobe}));

    Lauf const lauf = wache_fahren(fall.repo(), fall.baum());
    berichten("begruendete Waise", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "ABWESEND, ABER BEGRUENDET")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, waise)) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, grund))
        << "Der tragende Gegenstand muss NAMENTLICH erscheinen -- eine Begruendung, die die Wache nicht "
           "ausweist, ist von Schweigen nicht zu unterscheiden.\n"
        << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK"))
        << "Beleg, dass die Wache durchgelaufen ist -- sonst waere 'kein Befund' nur Schweigen.\n"
        << lauf.ausgabe;
}

// =============================================================================
// (3) DIE ERLOSCHENE AUSNAHME -- die Eigenschaft, die die Allowlist ueberhaupt erst
//     ertraeglich macht, und die bis heute durch nichts gedeckt war. Der Gegenstand
//     der Begruendung existiert wieder: die Ausnahme traegt nicht mehr, OBWOHL niemand
//     die Allowlist angefasst hat.
// =============================================================================
TEST(T6BissRegistrierungsWache, ErloscheneAusnahmeWirdROT) {
    std::string const marke = koeder();
    std::string const waise = "tests/unit/test_waise_" + marke + ".cpp";
    std::string const grund = "ext/fremdbaum_" + marke + "/CMakeLists.txt";
    Fall              fall{marke};

    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(waise, "// nie gebaut\n"));
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     waise + " | datei:" + grund + " | braucht den optionalen Fremdbaum\n"));
    // DER EINZIGE UNTERSCHIED ZU FALL (2): der Gegenstand ist wieder da.
    ASSERT_TRUE(fall.repo().schreibe(grund, "# der Fremdbaum ist zurueck\n"));
    ASSERT_TRUE(fall.bauweg_schreiben({kGegenprobe}));

    Lauf const lauf = wache_fahren(fall.repo(), fall.baum());
    berichten("erloschene Ausnahme", lauf, marke);

    EXPECT_EQ(lauf.code, 1) << "Eine Ausnahme, deren Gegenstand wieder existiert, MUSS rot werden -- sonst ist die "
                               "Allowlist ein Freibrief mit unbegrenzter Laufzeit.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "AUSNAHME ERLOSCHEN")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, grund)) << "Der Gegenstand muss NAMENTLICH erscheinen.\n" << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << lauf.ausgabe;
}

// =============================================================================
// (4) FAIL-CLOSED I: kein Bauweg-Artefakt im Baum. Der IST-Nenner waere leer und JEDE
//     Datei erschiene als unregistriert -- ein Befund, der nur aus einer kaputten
//     Messung kommt. ABBRUCH (Exit 2), nicht 1.
// =============================================================================
TEST(T6BissRegistrierungsWache, OhneBauwegArtefaktIstAbbruch) {
    std::string const marke = koeder();
    Fall              fall{marke};

    ASSERT_TRUE(fall.init());
    // bewusst KEIN bauweg_schreiben()

    Lauf const lauf = wache_fahren(fall.repo(), fall.baum());
    berichten("kein Bauweg-Artefakt", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "weder compile_commands.json noch build.ninja")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "fail-closed")) << lauf.ausgabe;
}

// =============================================================================
// (5) FAIL-CLOSED II -- DIE MESSGERAET-GEGENPROBE. Die Gegenprobe-Datei existiert im
//     Repo, steht aber NICHT im Bauweg. Damit ist nicht belegt, dass das Suchmuster
//     ueberhaupt trifft; jeder Nichtfund waere wertlos. Die Wache MUSS abbrechen.
//     DAS IST DER UNTERSCHIED ZU EINER WACHE, DIE NUR ZAEHLT: sie trennt "nichts
//     gefunden" von "das Werkzeug sucht nicht".
// =============================================================================
TEST(T6BissRegistrierungsWache, GegenprobeNichtImBauwegIstAbbruch) {
    std::string const marke = koeder();
    std::string const datei = "tests/unit/test_reg_" + marke + ".cpp";
    Fall              fall{marke};

    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(datei, "// gebaut\n"));
    // Der Bauweg kennt die Koeder-Datei, aber NICHT die Gegenprobe.
    ASSERT_TRUE(fall.bauweg_schreiben({datei}));

    Lauf const lauf = wache_fahren(fall.repo(), fall.baum());
    berichten("Gegenprobe fehlt im Bauweg", lauf, marke);

    EXPECT_EQ(lauf.code, 2)
        << "Ohne Gegenprobe darf die Wache kein Urteil faellen -- weder gruen noch rot-als-Befund.\n"
        << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "steht NICHT in")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << lauf.ausgabe;
}

// =============================================================================
// (6) DER NAMENSVERWANDTE NACHBAR DECKT NICHT. Im Bauweg steht test_<marke>_real.cpp,
//     gesucht wird test_<marke>.cpp. Ein Praefix-Vergleich haette die Waise hier
//     stillschweigend als gebaut durchgewinkt -- die Klasse, gegen die die Verankerung
//     im Skript ausdruecklich gebaut ist.
// =============================================================================
TEST(T6BissRegistrierungsWache, NamensverwandterNachbarDecktNicht) {
    std::string const marke   = koeder();
    std::string const waise   = "tests/unit/test_reg_" + marke + ".cpp";
    std::string const nachbar = "tests/unit/test_reg_" + marke + "_real.cpp";
    Fall              fall{marke};

    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(waise, "// nie gebaut\n"));
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(nachbar, "// gebaut\n"));
    ASSERT_TRUE(fall.bauweg_schreiben({kGegenprobe, nachbar}));

    Lauf const lauf = wache_fahren(fall.repo(), fall.baum());
    berichten("namensverwandter Nachbar", lauf, marke);

    EXPECT_EQ(lauf.code, 1) << "Der Nachbar darf die Waise NICHT decken.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, waise)) << lauf.ausgabe;
    // GEGENPROBE derselben Zusicherung: der Nachbar selbst gilt als gebaut und darf
    // NICHT als Waise gemeldet werden -- sonst waere die Aussage oben durch eine Wache
    // erfuellt, die schlicht alles meldet.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "3 getrackte Test-Quelldatei(en)")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "1 davon NICHT im Bauweg")) << lauf.ausgabe;
}

// =============================================================================
// (7) DIE UNPRUEFBARE BEGRUENDUNG -- die DRITTE Rot-Ursache der Wache, und die einzige,
//     die dieser Biss-Test bis zur Sammellandung vom 10.08. nicht kannte.
//
// WOFUER ER DA IST. Feld 2 nennt seit MT-L4 (461776ef) die ART des Gegenstands. Eine
// Zeile ohne bekannte Art kann niemand nachpruefen; sie ist nicht "vermutlich in
// Ordnung", sondern ein Freibrief, und die Wache macht sie fail-closed ROT. Die Faelle
// (2) und (3) oben pruefen den Weg der GUELTIGEN Begruendung -- sie sagen nichts
// darueber, was mit einer ungueltigen geschieht. Genau diese Luecke hat die
// Sammellandung aufgedeckt: beide Faelle bauten ihr Fixture noch in der alten
// Zwei-Feld-Form, waren einzeln gruen und im Zusammenschluss rot.
//
// BEIDE RICHTUNGEN AN EINEM GEGENSTAND (K13, T-4). Derselbe Baum, derselbe gewuerfelte
// Gegenstand, EIN Unterschied: das Praefix. Ohne Richtung (b) belegte (a) nur, dass die
// Wache bei irgendeiner Allowlist rot wird -- nicht, dass das PRAEFIX das Urteil traegt.
//
// ABGRENZUNG, EHRLICH (T-9): die 'isa:'-Art wird hier NICHT nachgebaut. Sie haengt am
// CMakeCache des gemessenen Baums und ist in test_mt_l4_registrierungs_wache_isa.cpp an
// einem Synth-Cache in beide Richtungen gefahren (Faelle 1, 2, 5, 6 dort). Dieser Test
// deckt den Biss der Wache am Wegwerf-REPO; eine zweite Abschrift derselben Zusicherung
// truege hier nichts bei. Was hier fehlte und nun steht, ist die Art-Pruefung selbst.
// =============================================================================
TEST(T6BissRegistrierungsWache, UnpruefbareBegruendungWirdROT) {
    std::string const marke = koeder();
    std::string const waise = "tests/unit/test_waise_" + marke + ".cpp";
    std::string const grund = "ext/fremdbaum_" + marke + "/CMakeLists.txt"; // existiert NICHT
    Fall              fall{marke};

    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(waise, "// nie gebaut\n"));
    ASSERT_TRUE(fall.bauweg_schreiben({kGegenprobe}));

    // (a) OHNE ART-PRAEFIX -- woertlich die Form, in der (2) und (3) bis zum 10.08.
    //     standen. Der Gegenstand ist abwesend, die Begruendung klingt plausibel, und
    //     trotzdem MUSS die Wache beissen: niemand kann nachpruefen, was gemeint ist.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     waise + " | " + grund + " | braucht den optionalen Fremdbaum\n"));
    Lauf const ohne = wache_fahren(fall.repo(), fall.baum());
    berichten("Feld 2 ohne Art-Praefix", ohne, marke);

    EXPECT_EQ(ohne.code, 1) << "Eine Begruendung ohne nachpruefbare Art MUSS Exit 1 geben. Nicht 0 (das waere der "
                               "Freibrief) und nicht 2 (die Wache KONNTE pruefen -- sie hat ein Urteil).\n"
                            << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, "UNPRUEFBARE BEGRUENDUNG")) << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, grund))
        << "Der gewuerfelte Gegenstand MUSS woertlich erscheinen -- sonst hat die Wache ihn nie gelesen.\n"
        << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, "1 mit UNPRUEFBARER Begruendung"))
        << "Der Nenner gehoert in die Ausgabe (V-1), und er muss die Ursache TRENNEN: eine unpruefbare Zeile "
           "ist etwas anderes als eine fehlende.\n"
        << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, "1 unpruefbar"))
        << "Das Schluss-Urteil muss die Ursache mitfuehren -- diese Zeile stand woertlich im roten Job 371647.\n"
        << ohne.ausgabe;
    EXPECT_FALSE(enthaelt(ohne.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << ohne.ausgabe;

    // (b) DIESELBE ZEILE MIT 'datei:' -- gruen. Das ist der Gegenkoeder UND der Beleg,
    //     dass oben das Praefix gemessen wurde und nicht etwa der Gegenstand, der Baum
    //     oder die blosse Anwesenheit einer Allowlist.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     waise + " | datei:" + grund + " | braucht den optionalen Fremdbaum\n"));
    Lauf const mit = wache_fahren(fall.repo(), fall.baum());
    berichten("dasselbe Feld 2 MIT 'datei:'", mit, marke);

    EXPECT_EQ(mit.code, 0) << "Ein Praefix mehr, sonst nichts -- die Zeile MUSS jetzt tragen.\n" << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, "ABWESEND, ABER BEGRUENDET")) << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, "0 mit UNPRUEFBARER Begruendung")) << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << mit.ausgabe;
}

#endif // _WIN32
