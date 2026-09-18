// test_pa1_tote_ausnahme -- ERLOSCHEN hat ZWEI Richtungen.                 (2026-08-10)
// =============================================================================
// PRUEFLING: scripts/ci_test_registrierungs_wache.sh
//
// DER BEFUND, GEGEN DEN DIESE DATEI GEBAUT IST
//
// Die Wache definierte ERLOSCHEN in EINER Richtung: "existiert der Gegenstand WIEDER,
// traegt die Ausnahme nicht mehr". Das faengt den Fall, dass der Gegenstand zurueck-
// kehrt. Es faengt NICHT den Fall, dass er NIE zurueckkehren kann -- und genau der lag
// vor: alle vier prt-art-Zeilen der Allowlist banden an
//     datei:prt_art/include/prt_art/prt_art.hpp
// einen Pfad, den nichts mehr erzeugen kann. Am Objekt gemessen (2026-08-10):
//   prt_art.hpp .................. 0 Treffer unter /home/comdare
//   Gegenprobe cache_engine.hpp .. 45 Treffer -- das Werkzeug sucht, der Nullbefund gilt
//   Historie ..................... in ce vom 2026-05-12 (5c210581) bis 2026-05-14
//                                  getrackt, dann per 'git mv' nach
//                                  libs/deprecated/prt_art_legacy/ (b9abc720, R100),
//                                  dieser Baum am 2026-06-01 entfernt (6b3ed0d9).
//                                  Im Nachbarrepo comdare-prt-art NIE getrackt.
// FOLGE: die vier Zeilen konnten per Konstruktion nie erloeschen. Sie waren die
// Freistellung mit unbegrenzter Laufzeit, vor der der Kopf ihrer eigenen Datei warnt.
//
// DER ERSTLAUF, WOERTLICH (Rot zuerst, T-1). Gegen einen Bau-Baum, dem GENAU diese vier
// Dateien fehlten, meldete die Wache VOR der Heilung:
//     davon 4 begruendet, 0 mit ERLOSCHENER, 0 mit UNPRUEFBARER Begruendung, 0 ohne.
//     TEST-REGISTRIERUNGS-WACHE: OK (470 Quelldateien, 0 ohne Begruendung ausserhalb).
// Vier Ausnahmen ohne Ablauf, und die Wache sagte "OK".
//
// WAS DIE WACHE JETZT MISST -- und was das NICHT beweist:
// "Kann nie entstehen" ist nicht entscheidbar, solange man nur den eigenen Baum kennt.
// Gemessen wird deshalb etwas Engeres: KEINE QUELLE DIESES REPOS ERKLAERT DEN
// ZWEIG, IN DEM ER LIEGT -- weder der Git-Index (getrackte Datei ODER Submodul-Gitlink)
// noch eine .gitignore-Regel, und das bis hinauf zur Wurzel. Dann hat er in diesem
// Repository keinen Erzeuger.
// DAS BEWEIST NICHT, dass er nie existieren kann: jemand kann ihn morgen 'git add'en.
// Diese Datei prueft deshalb GENAU DIESE ENGERE AUSSAGE und keine groessere -- Fall (2),
// (3) und (4) belegen ausdruecklich, dass ein Pfad mit Erzeuger NICHT als tot gilt.
//
// WIE DIE VIER GRUEN-FAELLE MESSEN, und warum nicht anders (am Objekt gelernt): sie
// fragen NICHT, ob die Zeichenkette "TOTE AUSNAHME" in der Ausgabe fehlt. Der NENNER
// der Wache nennt die Klasse bei JEDEM Lauf ("... 0 TOTE AUSNAHME ..."), ein solches
// EXPECT_FALSE waere also immer verletzt -- beim ersten Lauf dieser Datei ist genau das
// passiert. Geprueft wird deshalb der ZAEHLER: "0 TOTE AUSNAHME" fuer gruen,
// "1 TOTE AUSNAHME" fuer den Biss. Dieselbe Klasse wie '/build' gegen '/builds': eine
// Teilzeichenkette ist kein Feld.
//
// WARUM ROT UND KEINE EIGENE WARNKLASSE (begruendete Entscheidung, PA-1): eine Warnung
// reproduziert exakt die Fehlerklasse -- an genau so einer Stelle sind vier Dateien 14
// bzw. 68 Tage unbemerkt geblieben. Die Hausregel kennt ZELLE=Warnung, JOB=hart rot.
// Der Preis ist ein moeglicher Fehlalarm; er ist billig zu beheben (erreichbaren
// Gegenstand nennen oder 'frist:' setzen) und faellt sofort auf, waehrend das Gegenteil
// -- eine tote Zeile -- per Konstruktion nie auffaellt.
//
// WARUM EIN GOOGLE TEST UND KEINE WEITERE SHELL-PROBE (Owner-KERN 09.08.2026):
//   "Ich sehe einen Haufen shells statt vernuenftiger google tests, was soll das? ...
//    SKRIPTE SAGEN GAR NICHTS."
// Debug UND Release, ueber die normale ctest-Registrierung.
//
// K13 BEIDSEITIG: jeder Fall wuerfelt frisch aus /dev/urandom. Fall (1) verlangt, dass
// ein unmoeglicher Pfad BEISST; Fall (2) und (3) verlangen, dass ein moeglicher Pfad
// NICHT beisst. Ein Orakel, das immer ROT liefert, faellt an (2)/(3); eines, das immer
// GRUEN liefert, faellt an (1).
//
// GRENZE, EHRLICH BENANNT (T-9): Fall (8) faehrt die ECHTE Allowlist, laesst dabei aber
// die 'isa:'-Zeile ausdruecklich AUS dem Pruefsatz -- ihre Auswertung braucht den
// CMakeCache des gemessenen Baums, und ein kuenstlich erzeugtes Fehlen wuerde sie auf
// einem AVX-512-Host als ERLOSCHEN melden, obwohl sie im echten Baum nie gefragt wird.
// Geprueft sind dort also die vier 'frist:'-Zeilen; NICHT geprueft ist die 'isa:'-Zeile.
// Beide Mengen stehen in der Ausgabe des Falls.
//
// NACHTRAG (2026-09-17, Owner-Order 206 "OV-2 Archiv-Variante gilt und frist Zeilen entfernen"):
// der Satz "geprueft sind dort die vier 'frist:'-Zeilen" ist UEBERHOLT -- die vier Zeilen sind aus
// der Allowlist entfernt. An ihre Stelle tritt die ARCHIV-Klasse der Wache: eine getrackte
// Test-Quelldatei unter tests/deprecated/<ordner>/ zaehlt nicht zum SOLL, wenn und nur wenn
// tests/deprecated/<ordner>/VERMERK.md im Index liegt. Fall (8) misst ab jetzt GENAU DAS am echten
// Repo (die vier fehlen im Bauweg und muessen trotzdem gruen sein, weil sie ARCHIV sind), Fall (10)
// die Regel selbst an einem Wegwerf-Repo -- beidseitig, samt Nachbar-Anker-Probe.
// UNVERAENDERT: die 'frist:'-ART der Wache bleibt in Gebrauch und wird von Fall (7) beidseitig
// gefahren; sie hat nach diesem Entscheid nur keinen Gegenstand mehr in der committeten Allowlist.
//
// NACHTRAG 2 (2026-09-18, Lens-Funde r1 des OV-2-Zuges: Lens A LA-04, Lens B LB-01/LB-02/LB-03/LB-05):
// Fall (10) traegt jetzt die Stufen (d)/(e) -- eine Allowlist-Zeile fuer eine ARCHIV-Datei ist eine
// Ausnahme OHNE ANLASS und ROT; die Faelle (11a-c) geben den drei im Wachen-Kopf behaupteten Grenzen
// der Archiv-Regel je einen Traeger; Fall (12) verlangt einen Anker mit Inhalt und Form (0 Byte oder
// Symlink ankern nicht); Fall (13) faengt die Schwesterklasse "Allowlist-Zeile ohne Gegenstand im
// Index". Alle Archiv-Nenner sind ab hier FELD-verankert gepinnt (", 4 archiviert)" mit Komma davor
// und Klammer danach statt "4 archiviert" -- Teilzeichenkette von "14 archiviert", Klasse s. oben).
//
// ASCII-only, Zeilen <= 120 Byte.
// =============================================================================

#if defined(_WIN32)

#include <gtest/gtest.h>

TEST(Pa1ToteAusnahme, NurPosix) {
    GTEST_SKIP() << "scripts/ci_test_registrierungs_wache.sh ist ein POSIX-sh-Skript "
                    "(kein Windows-Job faehrt es).";
}

#else

#include "support/wachen_werkbank.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
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
// Bauweg stehen, sonst faellt die Wache kein Urteil (Exit 2).
constexpr char const* kGegenprobe = "tests/unit/test_pressure_state.cpp";

// Die vier geparkten Posten. Sie stehen hier als Literale, weil Fall (8) und Fall (9)
// genau sie gegen die beiden committeten Allowlists fahren.
// W-B (2026-08-15, Vorlage-B2-GO): per 'git mv' nach tests/deprecated/prt_art_legacy_waisen/
// archiviert (VERMERK.md dort). Der SOLL ist am DATEINAMEN verankert, die vier bleiben also
// im SOLL und in der Allowlist -- nur der Pfad in Feld 1 und hier ist nachgezogen.
// UEBERHOLT (2026-09-17, Owner-Order 206): der letzte Satz gilt nicht mehr. Die vier stehen in
// KEINER Allowlist mehr und sind auch nicht mehr im SOLL der Registrierungs-Wache -- sie sind
// ARCHIV, weil tests/deprecated/prt_art_legacy_waisen/VERMERK.md im Index liegt und die Wache
// diesen Anker liest. Die Literale hier bleiben trotzdem noetig: Fall (8) nimmt sie aus dem
// Bauweg heraus (Anlass des Archiv-Abzugs) und Fall (9) sucht ihre Namen in der Schwesterliste.
constexpr char const* kGeparkt[] = {
    "tests/deprecated/prt_art_legacy_waisen/test_concepts_compile.cpp",
    "tests/deprecated/prt_art_legacy_waisen/test_value_handle.cpp",
    "tests/deprecated/prt_art_legacy_waisen/test_three_layer_audit.cpp",
    "tests/deprecated/prt_art_legacy_waisen/test_six_page_structures.cpp",
};

[[nodiscard]] std::string wachen_pfad() {
    if (char const* const ueberschrieben = std::getenv("COMDARE_PA1_WACHE_PFAD");
        ueberschrieben != nullptr && *ueberschrieben != '\0') {
        return std::string{ueberschrieben};
    }
    return std::string{COMDARE_PA1_WACHE};
}

[[nodiscard]] std::string repo_wurzel() { return std::string{COMDARE_PA1_REPO}; }

void berichten(char const* fall, Lauf const& lauf, std::string const& marke) {
    std::cout << "  [PA-1] Fall '" << fall << "' | Koeder " << marke << " | Prueflig " << wachen_pfad() << " | Exit "
              << lauf.code << "\n";
}

// ---------------------------------------------------------------------------
// Ein Fall: ein Wegwerf-Repo mit Gegenprobe-Datei, einer Waise und einem Bau-Baum, in
// dem die Waise fehlt. Was die Allowlist-Zeile dann in Feld 2 nennt, ist der Gegenstand
// der Untersuchung.
// ---------------------------------------------------------------------------
class Fall {
public:
    explicit Fall(std::string const& marke) : Fall{marke, "tests/unit/test_waise_" + marke + ".cpp"} {}

    // Zweiter Konstruktor (2026-09-17): der Waisen-PFAD ist waehlbar. Die ARCHIV-Klasse der Wache
    // haengt am ORT der Datei, nicht an ihrem Namen -- ein Fall dafuer braucht eine Waise unter
    // tests/deprecated/<ordner>/ statt unter tests/unit/. Alles andere bleibt identisch.
    Fall(std::string const& marke, std::string const& waisen_pfad)
        : marke_{marke}, repo_{marke}, baum_{repo_.pfad().string() + "_baum"}, waise_{waisen_pfad} {
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
        r = repo_.schreibe_und_verfolge(kGegenprobe, "// Gegenprobe-Datei des Falls\n");
        if (!r) { return r; }
        // Die Waise ist getrackt und fehlt gleich im Bauweg -- sie ist der Anlass,
        // aus dem die Wache ueberhaupt in die Allowlist schaut.
        r = repo_.schreibe_und_verfolge(waise_, "// Waise des Falls " + marke_ + "\n");
        if (!r) { return r; }
        // Nur die Gegenprobe steht im Bauweg, die Waise nicht.
        return bauweg_schreiben({kGegenprobe});
    }

    // Feld 2 der einen Allowlist-Zeile setzen. Feld 3 traegt die Koeder-Marke, damit
    // ein GRUEN-Fall belegen kann, dass die Wache GENAU DIESE Zeile gelesen hat.
    [[nodiscard]] testing::AssertionResult allowlist_setzen(std::string const& feld2) const {
        return repo_.schreibe("scripts/ci_test_registrierungs_allowlist.txt", "# Allowlist des Falls " + marke_ + "\n" +
                                                                                  waise_ + " | " + feld2 +
                                                                                  " | Koeder " + marke_ + "\n");
    }

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

    // 'heute' leer = Systemuhr. Sonst wird der Wache ihr Heute vorgegeben, damit beide
    // Seiten einer Frist gefahren werden koennen, ohne die Systemuhr zu stellen.
    [[nodiscard]] Lauf fahren(std::string const& heute = "") const {
        std::string vorspann = WegwerfRepo::umgebung();
        if (!heute.empty()) { vorspann += " COMDARE_WACHE_HEUTE=" + heute; }
        return fahre("cd " + zitiert(repo_.pfad()) + " && " + vorspann + " sh " + zitiert(wachen_pfad()) + " " +
                     zitiert(baum_));
    }

    [[nodiscard]] WegwerfRepo const& repo() const { return repo_; }
    [[nodiscard]] std::string const& waise() const { return waise_; }

private:
    std::string marke_;
    WegwerfRepo repo_;
    fs::path    baum_;
    std::string waise_;
};

} // namespace

// =============================================================================
// (1) DER UNMOEGLICHE GEGENSTAND BEISST. Der Pfad kommt frisch aus /dev/urandom und
//     liegt unter einem Verzeichnis, das dieses Wegwerf-Repo nicht kennt -- weder im
//     Index noch in einer .gitignore-Regel. Vor der Heilung war dieser Fall GRUEN.
// =============================================================================
TEST(Pa1ToteAusnahme, UnmoeglicherGegenstandWirdTOT) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    // Der gewuerfelte Pfad liegt in einem gewuerfelten Verzeichnis: beide Segmente
    // kommen in keiner Datei dieses Repos vor.
    std::string const tot = "nirgends_" + marke + "/tief/" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + tot));

    Lauf const lauf = fall.fahren();
    berichten("UnmoeglicherGegenstandWirdTOT", lauf, marke);

    EXPECT_EQ(lauf.code, 1) << "Eine Ausnahme, die per Konstruktion nie erloeschen kann, muss ROT sein.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "TOTE AUSNAHME"))
        << "Der Befund muss seinen Namen tragen -- sonst ist er von jedem anderen Rot nicht zu trennen.\n"
        << lauf.ausgabe;
    // Der Koeder muss WOERTLICH zurueckkommen: nur dann stammt die Meldung aus dem
    // Gegenstand, den dieser Fall angelegt hat, und nicht aus einer anderen Quelle.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, tot)) << "Der gewuerfelte Pfad fehlt in der Ausgabe.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "1 TOTE AUSNAHME")) << "Der Nenner muss die Klasse zaehlen (V-1).\n"
                                                           << lauf.ausgabe;
}

// =============================================================================
// (2) DER GEGENKOEDER (K13, andere Richtung). Derselbe Aufbau, derselbe Wuerfel -- aber
//     der Pfad liegt unter tests/unit/, einem Verzeichnis MIT getracktem Inhalt. Er
//     existiert heute nicht und kann trotzdem jederzeit entstehen. Er darf NICHT
//     beissen. Ohne diesen Fall waere Fall (1) auch von einer Wache erfuellt, die
//     jeden abwesenden Gegenstand fuer tot erklaert.
// =============================================================================
TEST(Pa1ToteAusnahme, MoeglicherGegenstandBleibtGRUEN) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    // tests/unit/ traegt in diesem Repo die Gegenprobe-Datei und die Waise -- das
    // Verzeichnis ist dem Index also bekannt.
    std::string const lebendig = "tests/unit/kommt_vielleicht_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + lebendig));

    Lauf const lauf = fall.fahren();
    berichten("MoeglicherGegenstandBleibtGRUEN", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << "Ein Pfad unter einem bekannten Verzeichnis kann entstehen -- kein Befund.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "0 TOTE AUSNAHME")) << "Fehlalarm: der Gegenstand hat einen Erzeuger.\n"
                                                           << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, marke))
        << "Die Wache muss GENAU diese Allowlist-Zeile gelesen haben -- sonst belegt das Gruen nichts.\n"
        << lauf.ausgabe;
}

// =============================================================================
// (3) DAS BAUPRODUKT IST ERREICHBAR. Ein Pfad unter einer .gitignore-Regel ist ein
//     angemeldeter Ablageort fuer Erzeugtes: er existiert nicht, ist nicht getrackt --
//     und kann bei jedem Bau entstehen. Die zweite Haelfte des Gegenkoeders, und die
//     Stelle, an der die Falle 'check-ignore <dir>' vs. '<dir>/' sitzt.
// =============================================================================
TEST(Pa1ToteAusnahme, BauproduktUnterGitignoreIstErreichbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(".gitignore", "erzeugt_" + marke + "/\n"));
    std::string const bauprodukt = "erzeugt_" + marke + "/artefakt.hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + bauprodukt));

    Lauf const lauf = fall.fahren();
    berichten("BauproduktUnterGitignoreIstErreichbar", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << "Ein ignoriertes Verzeichnis ist ein angemeldeter Ablageort -- kein Befund.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "0 TOTE AUSNAHME")) << "Fehlalarm auf einem Bauprodukt.\n" << lauf.ausgabe;
}

// =============================================================================
// (4) DER SUBMODUL-PFAD IST ERREICHBAR. Ein nicht ausgechecktes Submodul steht als
//     GITLINK im Index -- sein Inhalt nicht. Ein Gegenstand darunter existiert also
//     nicht, ist nicht getrackt, ist nicht ignoriert, und kann trotzdem jederzeit
//     durch ein 'submodule update' auftauchen. Genau der Fall, den die Regel NICHT
//     faelschlich toeten darf.
// =============================================================================
TEST(Pa1ToteAusnahme, SubmodulGitlinkIstErreichbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());

    std::string const modul = "ext/fremd_" + marke;
    // Einen Gitlink von Hand in den Index legen: 160000 ist der Modus fuer 'commit'.
    // Der SHA muss kein existierendes Objekt sein -- der Index haelt ihn trotzdem.
    Lauf const idx = fahre("cd " + zitiert(fall.repo().pfad()) + " && " + WegwerfRepo::umgebung() +
                           " git update-index --add --cacheinfo 160000,"
                           "0000000000000000000000000000000000000001," +
                           modul);
    ASSERT_EQ(idx.code, 0) << "Gitlink konnte nicht in den Index gelegt werden:\n" << idx.ausgabe;

    std::string const drin = modul + "/include/kopf.hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + drin));

    Lauf const lauf = fall.fahren();
    berichten("SubmodulGitlinkIstErreichbar", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << "Ein Pfad unter einem Gitlink kann jederzeit ausgecheckt werden -- kein Befund.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "0 TOTE AUSNAHME"))
        << "Fehlalarm: ein nicht initialisiertes Submodul ist kein toter Gegenstand.\n"
        << lauf.ausgabe;
}

// =============================================================================
// (5) AUSSERHALB DES REPOS WIRD NICHT GEURTEILT. Ein absoluter Pfad ist mit den
//     Quellen dieses Repos nicht beurteilbar. Die Wache darf ihn deshalb NIE tot
//     nennen -- und muss sagen, dass sie ihn nicht beurteilt hat (T-9, V-1).
// =============================================================================
TEST(Pa1ToteAusnahme, AusserhalbDesRepoWirdNichtBeurteilt) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.allowlist_setzen("datei:/opt/gibtsnicht_" + marke + "/kopf.hpp"));

    Lauf const lauf = fall.fahren();
    berichten("AusserhalbDesRepoWirdNichtBeurteilt", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << "Ein nicht beurteilbarer Pfad ist kein Befund.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "0 TOTE AUSNAHME"))
        << "Die Wache hat ueber etwas geurteilt, das sie nicht messen kann.\n"
        << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "AUSSERHALB"))
        << "Der Nenner muss die nicht beurteilte Menge ausweisen -- sonst sieht sie wie geprueft aus.\n"
        << lauf.ausgabe;
}

// =============================================================================
// (6) DIE ERSTE RICHTUNG BLEIBT ERHALTEN. Regressionsschutz: die neue Richtung darf die
//     alte nicht verdraengen. Existiert der Gegenstand, ist die Ausnahme ERLOSCHEN --
//     und ausdruecklich NICHT 'tot'. Die beiden Klassen verlangen verschiedene Abhilfen.
// =============================================================================
TEST(Pa1ToteAusnahme, RichtungEinsBleibtErhalten) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    std::string const wieder_da = "tests/unit/wieder_da_" + marke + ".hpp";
    ASSERT_TRUE(fall.repo().schreibe(wieder_da, "// der Gegenstand ist zurueck\n"));
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + wieder_da));

    Lauf const lauf = fall.fahren();
    berichten("RichtungEinsBleibtErhalten", lauf, marke);

    EXPECT_EQ(lauf.code, 1) << "Ein wieder vorhandener Gegenstand muss ROT sein.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "ERLOSCHEN")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "0 TOTE AUSNAHME"))
        << "Ein vorhandener Gegenstand ist erloschen, nicht tot -- die Abhilfe waere sonst falsch.\n"
        << lauf.ausgabe;
}

// =============================================================================
// (7) DIE FRIST, BEIDE SEITEN. 'frist:' ist die Form fuer die ehrlich unbeweisbare
//     Ausnahme. Sie muss tragen, solange das Datum laeuft, und ROT werden, sobald es
//     verstrichen ist -- sonst waere sie nur die naechste unbegrenzte Freistellung.
//     Beide Seiten in EINEM Fall, mit demselben Aufbau: nur das Heute unterscheidet sie.
// =============================================================================
TEST(Pa1ToteAusnahme, FristTraegtBisZumTagUndDannNichtMehr) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.allowlist_setzen("frist:2026-09-15"));

    Lauf const vorher = fall.fahren("2026-09-15");
    berichten("FristTraegtBisZumTagUndDannNichtMehr/am-Tag", vorher, marke);
    EXPECT_EQ(vorher.code, 0) << "Am Tag der Frist traegt die Ausnahme noch.\n" << vorher.ausgabe;
    EXPECT_TRUE(enthaelt(vorher.ausgabe, "2026-09-15")) << "Das Datum gehoert in die Ausgabe (V-1).\n"
                                                        << vorher.ausgabe;

    Lauf const nachher = fall.fahren("2026-09-16");
    berichten("FristTraegtBisZumTagUndDannNichtMehr/danach", nachher, marke);
    EXPECT_EQ(nachher.code, 1) << "Einen Tag nach der Frist muss die Ausnahme ROT sein.\n" << nachher.ausgabe;
    EXPECT_TRUE(enthaelt(nachher.ausgabe, "ABGELAUFEN")) << nachher.ausgabe;

    // Die Herkunft des Heute muss im Nenner stehen: ein verschiebbarer Zeitbegriff, den
    // niemand sieht, waere selbst wieder ein Freibrief.
    EXPECT_TRUE(enthaelt(nachher.ausgabe, "ueberschrieben"))
        << "Die Wache verschweigt, dass ihr Heute vorgegeben wurde.\n"
        << nachher.ausgabe;
}

// =============================================================================
// (8) AM ECHTEN OBJEKT (T-7): die COMMITTETE Allowlist darf keine tote Zeile tragen.
//     Verfahren: ein Bau-Baum ueber dem ECHTEN Repo, dem GENAU die vier geparkten
//     Dateien fehlen -- damit werden genau ihre Zeilen ausgewertet, von der echten
//     Wache, gegen den echten Baum. Das ist derselbe Aufbau, der den Befund gefunden
//     hat; er bleibt ab hier stehen.
//
//     BEIDE MENGEN: geprueft sind die vier 'frist:'-Zeilen. NICHT geprueft ist die
//     'isa:'-Zeile (test_ap5_simd_extension_coherence) -- ihr Fehlen laesst sich hier
//     nicht ehrlich herstellen, s. Kopf.
//
//     NACHTRAG 2026-09-17 (Owner-Order 206): die vier 'frist:'-Zeilen gibt es nicht mehr; der
//     AUFBAU dieses Falls bleibt Byte fuer Byte derselbe, aber der GEGENSTAND ist jetzt die
//     ARCHIV-Klasse. Die vier Dateien fehlen dem Baum weiterhin -- gruen sind sie nur, weil die
//     Wache sie wegen tests/deprecated/prt_art_legacy_waisen/VERMERK.md gar nicht erst in ihren
//     SOLL nimmt. Genau das prueft der Fall ab hier: Exit 0 UND eine ARCHIV-Zeile, die alle vier
//     namentlich auffuehrt. Faellt der Anker weg, faellt dieser Fall -- und zwar laut.
//     Die 'isa:'-Zeile bleibt aus demselben Grund wie oben ungeprueft; sie ist nach dem Entscheid
//     die einzige verbliebene wirksame Zeile der committeten Allowlist.
// =============================================================================
TEST(Pa1ToteAusnahme, EchteAllowlistTraegtKeineToteZeile) {
    std::string const marke = koeder();
    fs::path const    baum  = comdare::test::user_tmp_dir() / ("pa1_echt_" + marke);
    std::error_code   ec;
    fs::create_directories(baum, ec);

    // Der SOLL der Wache, aus derselben Quelle wie bei ihr: git ls-files, ohne ext/,
    // am DATEINAMEN verankert.
    Lauf const soll = fahre("cd " + zitiert(fs::path{repo_wurzel()}) + " && " + WegwerfRepo::umgebung() +
                            " git ls-files | /usr/bin/grep -v '^ext/' | /usr/bin/grep -v '/ext/'"
                            " | /usr/bin/grep -E '(^|/)test_[^/]*[.]cpp$' | sort");
    ASSERT_EQ(soll.code, 0) << "git ls-files im echten Repo fehlgeschlagen:\n" << soll.ausgabe;

    std::vector<std::string> alle;
    for (std::size_t start = 0; start < soll.ausgabe.size();) {
        std::size_t const ende = soll.ausgabe.find('\n', start);
        std::string const z    = soll.ausgabe.substr(start, ende == std::string::npos ? ende : ende - start);
        if (!z.empty()) { alle.push_back(z); }
        if (ende == std::string::npos) { break; }
        start = ende + 1;
    }
    // GEGENPROBE des Messgeraets: ohne belastbaren SOLL beweist der Fall nichts.
    ASSERT_GT(alle.size(), 100U) << "Der SOLL ist unglaubwuerdig klein (" << alle.size() << ") -- fail-closed.";

    std::size_t   weggelassen = 0;
    std::ofstream aus{baum / "compile_commands.json", std::ios::trunc};
    ASSERT_TRUE(aus.good()) << "compile_commands.json nicht schreibbar";
    aus << "[\n";
    bool erste = true;
    for (auto const& datei : alle) {
        bool geparkt = false;
        for (char const* const g : kGeparkt) {
            if (datei == g) { geparkt = true; }
        }
        if (geparkt) {
            ++weggelassen;
            continue;
        }
        if (!erste) { aus << ",\n"; }
        erste = false;
        aus << "  {\"directory\": \"" << baum.string() << "\",\n"
            << "   \"command\": \"c++ -c " << repo_wurzel() << "/" << datei << "\",\n"
            << "   \"file\": \"" << repo_wurzel() << "/" << datei << "\"}";
    }
    aus << "\n]\n";
    aus.close();

    ASSERT_EQ(weggelassen, sizeof kGeparkt / sizeof kGeparkt[0])
        << "Nicht alle vier geparkten Dateien standen im SOLL -- der Fall maesse etwas anderes.";

    Lauf const lauf = fahre("cd " + zitiert(fs::path{repo_wurzel()}) + " && " + WegwerfRepo::umgebung() + " sh " +
                            zitiert(wachen_pfad()) + " " + zitiert(baum));
    // ETIKETT (Lens B LB-05, 2026-09-18): 'alle' ist der ROH-Bestand aus git ls-files; der SOLL der Wache
    // ist die Zahl NACH dem Archiv-Abzug. Beide stehen hier, in derselben Sprache wie der Nenner der
    // Wache ("N getrackte ... davon M ARCHIV ... -- SOLL: N-M").
    std::cout << "  [PA-1] Fall 'EchteAllowlistTraegtKeineToteZeile' | getrackt " << alle.size() << " | SOLL "
              << (alle.size() - weggelassen) << " | ARCHIV " << weggelassen
              << " (die vier Archiv-Dateien fehlen dem Baum) | NICHT geprueft: die 'isa:'-Zeile"
              << " | Exit " << lauf.code << "\n";

    EXPECT_EQ(lauf.code, 0) << "Die committete Allowlist traegt eine Zeile, die nie erloeschen kann.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "0 TOTE AUSNAHME")) << "Der Nenner muss die Null ausweisen (V-1).\n"
                                                           << lauf.ausgabe;
    // Die vier fehlen dem Baum und sind trotzdem gruen -- das traegt NUR die ARCHIV-Klasse.
    // Ohne diese Erwartungen waere der Fall auch von einer Wache erfuellt, die sie stillschweigend
    // uebersieht: gemessen wird deshalb die AUSGEWIESENE Menge, nicht nur der Exit.
    // FELD-ANKER statt Teilzeichenkette (Lens B LB-03, 2026-09-18): "4 archiviert" ist Teilzeichenkette
    // von "14 archiviert" -- dieselbe Klasse wie '/build' gegen '/builds' (Kopf). Gepinnt wird deshalb
    // das ganze Feld mit Komma davor und Klammer danach, und die ARCHIV-Zeile mit Datei- UND Ordner-Zahl.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "ARCHIV (tests/deprecated/, VERMERK.md-Anker): 4 Datei(en) in 1 Ordner(n),"))
        << "Die vier archivierten Dateien muessen als Menge ausgewiesen sein (4 Dateien, 1 Ordner).\n"
        << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, ", 4 archiviert)")) << "Die Endzeile muss den Archiv-Nenner tragen.\n"
                                                           << lauf.ausgabe;
    for (char const* const g : kGeparkt) {
        EXPECT_TRUE(enthaelt(lauf.ausgabe, std::string{g})) << "'" << g << "' fehlt in der ARCHIV-Liste der Wache.\n"
                                                            << lauf.ausgabe;
    }
    fs::remove_all(baum, ec);
}

// =============================================================================
// (9) T-6 SCHWESTERPFLICHT -- und eine BERICHTIGUNG der urspruenglichen Annahme.
//
//     Die Annahme lautete: dieselben vier stehen "aus dem spiegelbildlichen Grund" auch
//     in scripts/ci_test_sichtbarkeit_allowlist.txt. Am Objekt gemessen (2026-08-10)
//     stimmt das NICHT MEHR, und zwar zu Recht. Die beiden Stufen haben verschiedene
//     SOLL-Quellen:
//       Stufe 1  SOLL = git ls-files          -> die vier .cpp liegen weiter im Baum
//       Stufe 2  SOLL = die Registrierungs-AUFRUFE im CMake-Quelltext -> seit 8a8f6877
//                                                 entfernt, die vier kommen dort nicht
//                                                 mehr vor
//     Eine Zeile in der Schwesterliste waere daher eine Ausnahme OHNE ANLASS -- und die
//     faengt nur das naechste echte Verschwinden desselben Namens lautlos weg. Genau
//     diese Fehlerform steht im Kopf jener Datei.
//
//     DIESER FALL VERLANGT ALSO DAS GEGENTEIL DER ANNAHME: keiner der vier Namen darf
//     dort als WIRKSAME Zeile stehen. 'Wirksam' heisst: keine Kommentarzeile. Die vier
//     Namen kommen im Kopf jener Datei sehr wohl vor -- in der Begruendung, warum sie
//     entfernt wurden. Ein Orakel, das die ganze Datei nach dem Namen absucht, wuerde
//     diesen Kommentar fuer eine Zeile halten und waere hohl; deshalb wird hier Zeile
//     fuer Zeile das ERSTE FELD verglichen und nicht die Datei nach Teilzeichenketten.
//
//     GEGENPROBE (sonst bewiese ein Nullbefund nichts): der Parser MUSS wirksame Zeilen
//     finden. Findet er keine, ist nicht die Liste sauber, sondern der Parser kaputt.
// =============================================================================
TEST(Pa1ToteAusnahme, T6SchwesterlisteFuehrtDieVierNichtMehr) {
    fs::path const schwester = fs::path{repo_wurzel()} / "scripts/ci_test_sichtbarkeit_allowlist.txt";
    std::ifstream  quelle{schwester};
    ASSERT_TRUE(quelle.good()) << "Schwesterliste nicht lesbar: " << schwester.string();

    std::vector<std::string> wirksam;
    std::string              zeile;
    while (std::getline(quelle, zeile)) {
        std::size_t const erst = zeile.find_first_not_of(" \t\r");
        if (erst == std::string::npos) { continue; }
        if (zeile[erst] == '#') { continue; }
        std::size_t const ende = zeile.find_first_of(" \t", erst);
        wirksam.push_back(zeile.substr(erst, ende == std::string::npos ? ende : ende - erst));
    }

    // GEGENPROBE des Messgeraets vor jedem Nullbefund.
    ASSERT_FALSE(wirksam.empty()) << "Der Parser fand KEINE wirksame Zeile in " << schwester.string()
                                  << " -- ein Nullbefund waere dann eine Aussage ueber den Parser, "
                                     "nicht ueber die Liste. Fail-closed.";

    for (char const* const g : kGeparkt) {
        std::string const datei = std::string{g};
        std::string const name  = datei.substr(datei.rfind('/') + 1, datei.size() - datei.rfind('/') - 1 - 4);
        bool              steht = false;
        for (auto const& w : wirksam) {
            if (w == name) { steht = true; }
        }
        EXPECT_FALSE(steht) << "'" << name << "' steht wieder als wirksame Zeile in " << schwester.string()
                            << ". Seine Registrierung ist aus tests/unit/CMakeLists.txt entfernt, der Name "
                               "kommt im SOLL jener Wache also gar nicht vor -- die Zeile deckt nichts und "
                               "faengt nur das naechste echte Verschwinden desselben Namens lautlos weg.";
    }

    std::cout << "  [PA-1/T-6] Schwesterliste " << schwester.filename().string() << ": " << wirksam.size()
              << " wirksame Zeile(n), davon 0 der 4 geparkten -- richtig, ihr SOLL kennt sie nicht mehr.\n"
              << "  [PA-1/T-6] NICHT geprueft und ausdruecklich OFFEN: jene Liste hat ueberhaupt kein\n"
                 "             Gegenstand-Feld (Format '<name> <begruendung>'). Ihre Wache kennt daher\n"
                 "             KEINE Erloschen-Probe -- weder Richtung 1 noch Richtung 2. Struktur-Nenner:\n"
              << "             " << wirksam.size() << " von " << wirksam.size()
              << " Zeilen ohne nachpruefbaren Gegenstand. Eigenes Paket.\n";
}

// =============================================================================
// (10) DIE ARCHIV-KLASSE, BEIDSEITIG UND MIT NACHBAR-PROBE (Owner 2026-09-17, Order 206).
//      Die Regel lautet: eine getrackte Test-Quelldatei unter tests/deprecated/<ordner>/
//      faellt aus dem SOLL, WENN UND NUR WENN tests/deprecated/<ordner>/VERMERK.md im
//      Index liegt. Dieser Fall faehrt alle drei Zustaende am SELBEN Gegenstand:
//        (a) kein Anker            -> ROT, die Waise steht namentlich als unbegruendet
//        (b) Anker im NACHBARordner -> weiter ROT (ein Anker traegt NUR seinen Ordner)
//        (c) Anker im eigenen Ordner -> GRUEN, die Waise steht in der ARCHIV-Zeile
//      K13 beidseitig: ein Orakel, das immer GRUEN liefert, faellt an (a) und (b); eines,
//      das immer ROT liefert, faellt an (c). (b) ist der Teil, der aus dem "wenn" ein
//      "wenn und nur wenn" macht -- ohne ihn waere die Regel "irgendwo unter
//      tests/deprecated/ liegt ein VERMERK.md" und damit genau der Freibrief, gegen den
//      diese Wache gebaut ist.
//      (a)-(c) AUSDRUECKLICH OHNE ALLOWLIST-ZEILE: die Archiv-Klasse darf nicht an einer Ausnahme
//      haengen, sonst pruefte der Fall die Allowlist und nicht den Ort.
//        (d) Anker PLUS Allowlist-Zeile -> ROT: die Zeile ist eine Ausnahme OHNE ANLASS (LB-01)
//        (e) Anker, Allowlist ohne die Zeile -> wieder GRUEN (Gegenrichtung zu (d))
// =============================================================================
TEST(Pa1ToteAusnahme, ArchivOrdnerZaehltNurMitVermerkAnker) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/archiv_" + marke;
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());

    Lauf const ohne = fall.fahren();
    berichten("ArchivOrdnerZaehltNurMitVermerkAnker/ohne-Anker", ohne, marke);
    EXPECT_EQ(ohne.code, 1) << "Ohne VERMERK.md bleibt die Datei im SOLL -- und fehlt im Bauweg.\n" << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, fall.waise()))
        << "Der gewuerfelte Pfad fehlt in der Ausgabe -- der Befund stammt dann nicht aus diesem Fall.\n"
        << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, ", 0 archiviert)"))
        << "Die Archiv-Klasse gehoert auch dann in den Nenner, wenn sie leer ist (V-1).\n"
        << ohne.ausgabe;

    ASSERT_TRUE(fall.repo().schreibe_und_verfolge("tests/deprecated/anderer_" + marke + "/VERMERK.md",
                                                  "# Nachbar-Anker " + marke + "\n"));
    Lauf const nachbar = fall.fahren();
    berichten("ArchivOrdnerZaehltNurMitVermerkAnker/Nachbar-Anker", nachbar, marke);
    EXPECT_EQ(nachbar.code, 1) << "Ein Anker im NACHBARordner darf nicht tragen.\n" << nachbar.ausgabe;
    EXPECT_TRUE(enthaelt(nachbar.ausgabe, ", 0 archiviert)")) << "Der fremde Anker hat etwas archiviert.\n"
                                                              << nachbar.ausgabe;

    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/VERMERK.md", "# Archiv-Anker " + marke + "\n"));
    Lauf const mit = fall.fahren();
    berichten("ArchivOrdnerZaehltNurMitVermerkAnker/mit-Anker", mit, marke);
    EXPECT_EQ(mit.code, 0) << "Mit eigenem VERMERK.md ist die Datei ARCHIV und nicht mehr im SOLL.\n" << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, "ARCHIV (tests/deprecated/, VERMERK.md-Anker): 1 Datei(en) in 1 Ordner(n),"))
        << "Die Archiv-Menge muss mit Datei- und Ordner-Zahl ausgewiesen sein, sonst schrumpft der Nenner lautlos.\n"
        << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, fall.waise())) << "Die archivierte Datei muss namentlich erscheinen.\n"
                                                     << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, ", 1 archiviert)")) << "Die Endzeile muss den Archiv-Nenner tragen.\n"
                                                          << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << mit.ausgabe;

    // (d) EINE ALLOWLIST-ZEILE FUER DIE ARCHIVIERTE DATEI (Lens B LB-01, 2026-09-18). Die Allowlist-
    //     Schleife der Wache laeuft nur ueber die Dateien, die dem SOLL fehlen; eine archivierte Datei
    //     steht nicht im SOLL, ihre Zeile wuerde also NIE ausgewertet -- eine abgelaufene Frist bliebe
    //     still gruen (Lens-B-Probe P10: 'frist:2000-01-01' -> Exit 0). Genau das ist die "Ausnahme
    //     OHNE ANLASS" aus Fall (9). Die Wache muss die Zeile als UNPRUEFBAR melden: ROT, namentlich,
    //     und mit "0 erloschen" -- sie darf die Frist gar nicht erst bewerten, der Ort traegt.
    //     ROT ZUERST: gegen die Wache vom Stand 54296857 war diese Stufe GRUEN (Exit 0) -- Beleg im
    //     Beweisort des OV-2-Zuges (FIX-r1.md).
    ASSERT_TRUE(fall.allowlist_setzen("frist:2000-01-01"));
    Lauf const zeile = fall.fahren();
    berichten("ArchivOrdnerZaehltNurMitVermerkAnker/Anker-plus-Allowlist-Zeile", zeile, marke);
    EXPECT_EQ(zeile.code, 1) << "Eine Allowlist-Zeile fuer eine ARCHIV-Datei ist eine Ausnahme ohne Anlass -- ROT.\n"
                             << zeile.ausgabe;
    EXPECT_TRUE(enthaelt(zeile.ausgabe, fall.waise() + " -- UNPRUEFBAR: ARCHIV-Datei mit Allowlist-Zeile"))
        << "Die Zeile muss namentlich und mit ihrer Klasse gemeldet werden.\n"
        << zeile.ausgabe;
    EXPECT_TRUE(enthaelt(zeile.ausgabe, "Ausnahme ohne Anlass")) << zeile.ausgabe;
    EXPECT_TRUE(enthaelt(zeile.ausgabe, "1 unpruefbar")) << "Die Endzeile muss die Zeile als UNPRUEFBAR zaehlen.\n"
                                                         << zeile.ausgabe;
    EXPECT_TRUE(enthaelt(zeile.ausgabe, "1 Allowlist-Zeile(n) fuer ARCHIV-Dateien"))
        << "Der Nenner muss die Klasse getrennt zaehlen (V-1).\n"
        << zeile.ausgabe;
    EXPECT_TRUE(enthaelt(zeile.ausgabe, "0 erloschen"))
        << "Die Frist darf nicht bewertet worden sein -- die Zeile hat keinen Gegenstand im SOLL.\n"
        << zeile.ausgabe;
    EXPECT_TRUE(enthaelt(zeile.ausgabe, ", 1 archiviert)")) << "Die Datei bleibt ARCHIV; ROT ist die Zeile.\n"
                                                            << zeile.ausgabe;

    // (e) GEGENRICHTUNG zu (d): dieselbe Allowlist-Datei OHNE die Zeile -> wieder GRUEN. Damit ist
    //     belegt, dass (d) an der ZEILE hing und nicht an der blossen Anwesenheit einer Allowlist.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt", "# ohne Zeile " + marke + "\n"));
    Lauf const ohne_zeile = fall.fahren();
    berichten("ArchivOrdnerZaehltNurMitVermerkAnker/Anker-ohne-Allowlist-Zeile", ohne_zeile, marke);
    EXPECT_EQ(ohne_zeile.code, 0) << "Ohne die Zeile muss der Anker allein wieder tragen.\n" << ohne_zeile.ausgabe;
    EXPECT_TRUE(enthaelt(ohne_zeile.ausgabe, ", 1 archiviert)")) << ohne_zeile.ausgabe;
    EXPECT_TRUE(enthaelt(ohne_zeile.ausgabe, "0 Allowlist-Zeile(n) fuer ARCHIV-Dateien")) << ohne_zeile.ausgabe;
}

// =============================================================================
// (11) DIE DREI GRENZEN DER ARCHIV-REGEL, je mit Test-Traeger (Lens B LB-02, 2026-09-18).
//      Der Kopf der Wache behauptet drei Grenzen; am Objekt stimmten sie (Lens-Proben P1/P2/P4),
//      aber "Skripte sagen gar nichts" -- ohne Google-Test-Deckung bleibt jede Grenze Behauptung:
//        (a) eine Datei DIREKT unter tests/deprecated/ (ohne Ordner) hat keinen Anker und bleibt im
//            SOLL -- auch wenn tests/deprecated/VERMERK.md im Index liegt;
//        (b) ein VERMERK.md TIEFER als das dritte Pfadsegment ankert nichts -- der Anker des dritten
//            Segments dagegen traegt auch tiefer liegende Dateien (Gegenrichtung);
//        (c) ein VERMERK.md, das nur im ARBEITSBAUM liegt (nicht im Index), ankert nichts --
//            dieselbe Datei per 'git add' traegt (Gegenrichtung).
//      Jede Stufe misst Exit-Code UND Literal: die OHNE-BEGRUENDUNG-Zeile mit dem Waisen-Pfad und
//      den Archiv-Nenner ", 0 archiviert)" (Feld-Anker, s. Fall (8)).
//      ROT ZUERST am Mutanten: jede Stufe wurde gegen eine absichtlich aufgeweichte Kopie der Wache
//      gefahren (Anker-Muster ohne Segment-Grenze; Anker aus dem Arbeitsbaum statt aus dem Index)
//      und war dort rot -- Beleg im Beweisort des OV-2-Zuges (FIX-r1.md).
// =============================================================================
TEST(Pa1ToteAusnahme, ArchivGrenzeDateiDirektUnterDeprecatedBleibtImSoll) {
    std::string const marke = koeder();
    Fall              fall{marke, "tests/deprecated/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());
    // Der Anker liegt eine Ebene ZU HOCH: tests/deprecated/VERMERK.md ist kein <ordner>-Anker.
    ASSERT_TRUE(
        fall.repo().schreibe_und_verfolge("tests/deprecated/VERMERK.md", "# kein Ordner-Anker " + marke + "\n"));

    Lauf const lauf = fall.fahren();
    berichten("ArchivGrenzeDateiDirektUnterDeprecatedBleibtImSoll", lauf, marke);
    EXPECT_EQ(lauf.code, 1) << "Eine Datei direkt unter tests/deprecated/ hat keinen Anker und bleibt im SOLL.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, fall.waise())) << "Der gewuerfelte Pfad fehlt in der Ausgabe.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, ", 0 archiviert)")) << "tests/deprecated/VERMERK.md hat etwas archiviert.\n"
                                                           << lauf.ausgabe;
}

TEST(Pa1ToteAusnahme, ArchivGrenzeAnkerTieferAlsDrittesSegmentAnkertNicht) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/tief_" + marke;
    Fall              fall{marke, ordner + "/unter/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());
    // Der Anker liegt NEBEN der Datei, aber im VIERTEN Segment -- <ordner> ist genau das dritte.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/unter/VERMERK.md", "# zu tiefer Anker " + marke + "\n"));

    Lauf const tief = fall.fahren();
    berichten("ArchivGrenzeAnkerTieferAlsDrittesSegmentAnkertNicht/Anker-im-vierten-Segment", tief, marke);
    EXPECT_EQ(tief.code, 1) << "Ein VERMERK.md tiefer als das dritte Pfadsegment darf nicht ankern.\n" << tief.ausgabe;
    EXPECT_TRUE(enthaelt(tief.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << tief.ausgabe;
    EXPECT_TRUE(enthaelt(tief.ausgabe, fall.waise())) << tief.ausgabe;
    EXPECT_TRUE(enthaelt(tief.ausgabe, ", 0 archiviert)")) << "Der zu tiefe Anker hat etwas archiviert.\n"
                                                           << tief.ausgabe;

    // GEGENRICHTUNG: der Anker im dritten Segment traegt auch die tiefer liegende Datei.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/VERMERK.md", "# Ordner-Anker " + marke + "\n"));
    Lauf const drittes = fall.fahren();
    berichten("ArchivGrenzeAnkerTieferAlsDrittesSegmentAnkertNicht/Anker-im-dritten-Segment", drittes, marke);
    EXPECT_EQ(drittes.code, 0) << "Der Anker im dritten Segment muss den ganzen Ordner tragen.\n" << drittes.ausgabe;
    EXPECT_TRUE(enthaelt(drittes.ausgabe, fall.waise())) << drittes.ausgabe;
    EXPECT_TRUE(enthaelt(drittes.ausgabe, ", 1 archiviert)")) << drittes.ausgabe;
}

TEST(Pa1ToteAusnahme, ArchivGrenzeAnkerNurImArbeitsbaumAnkertNicht) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/ungetrackt_" + marke;
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());
    // repo().schreibe statt schreibe_und_verfolge: die Datei liegt im Arbeitsbaum, NICHT im Index.
    ASSERT_TRUE(fall.repo().schreibe(ordner + "/VERMERK.md", "# ungetrackter Anker " + marke + "\n"));
    ASSERT_FALSE(fall.repo().ist_verfolgt(ordner + "/VERMERK.md"))
        << "Das Arrangement ist falsch: der Anker steht im Index, die Stufe maesse nichts.";

    Lauf const ungetrackt = fall.fahren();
    berichten("ArchivGrenzeAnkerNurImArbeitsbaumAnkertNicht/nur-Arbeitsbaum", ungetrackt, marke);
    EXPECT_EQ(ungetrackt.code, 1) << "Ein VERMERK.md nur im Arbeitsbaum darf nicht ankern.\n" << ungetrackt.ausgabe;
    EXPECT_TRUE(enthaelt(ungetrackt.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << ungetrackt.ausgabe;
    EXPECT_TRUE(enthaelt(ungetrackt.ausgabe, fall.waise())) << ungetrackt.ausgabe;
    EXPECT_TRUE(enthaelt(ungetrackt.ausgabe, ", 0 archiviert)")) << "Der ungetrackte Anker hat etwas archiviert.\n"
                                                                 << ungetrackt.ausgabe;

    // GEGENRICHTUNG: dieselbe Datei im Index traegt.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/VERMERK.md", "# jetzt getrackter Anker " + marke + "\n"));
    Lauf const getrackt = fall.fahren();
    berichten("ArchivGrenzeAnkerNurImArbeitsbaumAnkertNicht/im-Index", getrackt, marke);
    EXPECT_EQ(getrackt.code, 0) << "Derselbe Anker im Index muss tragen.\n" << getrackt.ausgabe;
    EXPECT_TRUE(enthaelt(getrackt.ausgabe, ", 1 archiviert)")) << getrackt.ausgabe;
}

// =============================================================================
// (12) DIE FORM DES ANKERS (Lens A LA-04, 2026-09-18). Der Anker traegt die Begruendung der
//      Ablage. Ein Index-Eintrag namens VERMERK.md, der ein Blob mit 0 Byte oder kein regulaeres
//      Blob ist (Symlink, Modus 120000), traegt nichts -- und darf deshalb NICHT ankern. Die
//      Doktrin der Wache macht aus einer leeren Begruendung ROT (Feld 2 leer = UNPRUEFBAR); vor
//      diesem Fall war der Anker die einzige Stelle, an der Leere gruen trug. Drei Stufen am
//      selben Gegenstand:
//        (a) VERMERK.md mit 0 Byte im Index   -> ROT: UNPRUEFBARER ANKER, Waise im SOLL, 0 archiviert
//        (b) VERMERK.md als Symlink ins Nichts -> ROT: UNPRUEFBARER ANKER (Modus 120000), 0 archiviert
//        (c) VERMERK.md mit Inhalt             -> GRUEN, 1 archiviert (Gegenrichtung)
//      ROT ZUERST: (a) und (b) waren gegen die Wache vom Stand 54296857 GRUEN (Exit 0, "1 archiviert")
//      -- Beleg im Beweisort des OV-2-Zuges (FIX-r1.md).
// =============================================================================
TEST(Pa1ToteAusnahme, ArchivAnkerOhneInhaltOderFormAnkertNicht) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/hohl_" + marke;
    std::string const anker  = ordner + "/VERMERK.md";
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());

    // (a) 0 Byte, im Index.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(anker, ""));
    Lauf const leer = fall.fahren();
    berichten("ArchivAnkerOhneInhaltOderFormAnkertNicht/0-Byte", leer, marke);
    EXPECT_EQ(leer.code, 1) << "Ein Anker mit 0 Byte traegt keine Begruendung und darf nicht ankern.\n" << leer.ausgabe;
    EXPECT_TRUE(enthaelt(leer.ausgabe, anker + " -- UNPRUEFBARER ANKER"))
        << "Der Anker muss namentlich und mit seiner Klasse gemeldet werden.\n"
        << leer.ausgabe;
    EXPECT_TRUE(enthaelt(leer.ausgabe, "0 Byte")) << leer.ausgabe;
    EXPECT_TRUE(enthaelt(leer.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << leer.ausgabe;
    EXPECT_TRUE(enthaelt(leer.ausgabe, fall.waise())) << leer.ausgabe;
    EXPECT_TRUE(enthaelt(leer.ausgabe, ", 0 archiviert)")) << "Der leere Anker hat etwas archiviert.\n" << leer.ausgabe;
    EXPECT_TRUE(enthaelt(leer.ausgabe, "1 unpruefbar")) << "Die Endzeile muss den Anker als UNPRUEFBAR zaehlen.\n"
                                                        << leer.ausgabe;
    EXPECT_TRUE(enthaelt(leer.ausgabe, "1 ARCHIV-Anker ohne Inhalt/Form"))
        << "Der Nenner muss die Klasse getrennt zaehlen (V-1).\n"
        << leer.ausgabe;

    // (b) derselbe Pfad als Symlink auf ein Ziel, das es nicht gibt. 'git add' legt ihn mit Modus
    //     120000 in den Index; das Blob traegt nur den Link-Text.
    fs::path const  anker_abs = fall.repo().pfad() / anker;
    std::error_code ec;
    fs::remove(anker_abs, ec);
    ASSERT_FALSE(fs::exists(anker_abs)) << "Der 0-Byte-Anker liess sich nicht entfernen.";
    fs::create_symlink("nirgends_" + marke, anker_abs, ec);
    ASSERT_FALSE(ec) << "Symlink nicht anlegbar: " << ec.message();
    Lauf const add =
        fahre("cd " + zitiert(fall.repo().pfad()) + " && " + WegwerfRepo::umgebung() + " git add -- " + zitiert(anker));
    ASSERT_EQ(add.code, 0) << "git add des Symlinks fehlgeschlagen:\n" << add.ausgabe;
    Lauf const modus = fahre("cd " + zitiert(fall.repo().pfad()) + " && " + WegwerfRepo::umgebung() +
                             " git ls-files -s -- " + zitiert(anker));
    ASSERT_TRUE(enthaelt(modus.ausgabe, "120000 ")) << "Das Arrangement ist falsch: kein Symlink im Index:\n"
                                                    << modus.ausgabe;
    Lauf const symlink = fall.fahren();
    berichten("ArchivAnkerOhneInhaltOderFormAnkertNicht/Symlink", symlink, marke);
    EXPECT_EQ(symlink.code, 1) << "Ein Symlink namens VERMERK.md ist kein Anker.\n" << symlink.ausgabe;
    EXPECT_TRUE(enthaelt(symlink.ausgabe, anker + " -- UNPRUEFBARER ANKER")) << symlink.ausgabe;
    EXPECT_TRUE(enthaelt(symlink.ausgabe, "120000")) << "Der Modus gehoert in die Meldung (V-1).\n" << symlink.ausgabe;
    EXPECT_TRUE(enthaelt(symlink.ausgabe, ", 0 archiviert)")) << symlink.ausgabe;
    EXPECT_TRUE(enthaelt(symlink.ausgabe, "1 ARCHIV-Anker ohne Inhalt/Form")) << symlink.ausgabe;

    // (c) GEGENRICHTUNG: derselbe Pfad mit Inhalt traegt.
    fs::remove(anker_abs, ec);
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(anker, "# Archiv-Anker mit Inhalt " + marke + "\n"));
    Lauf const voll = fall.fahren();
    berichten("ArchivAnkerOhneInhaltOderFormAnkertNicht/mit-Inhalt", voll, marke);
    EXPECT_EQ(voll.code, 0) << "Derselbe Pfad mit Inhalt muss tragen.\n" << voll.ausgabe;
    EXPECT_TRUE(enthaelt(voll.ausgabe, ", 1 archiviert)")) << voll.ausgabe;
    EXPECT_TRUE(enthaelt(voll.ausgabe, "0 ARCHIV-Anker ohne Inhalt/Form")) << voll.ausgabe;
}

// =============================================================================
// (13) DIE SCHWESTERKLASSE ZU (10d) (Lens A LA-08, 2026-09-18): eine Allowlist-Zeile, deren Feld 1
//      KEINE getrackte Test-Quelldatei nennt (geloescht, umbenannt, unter ext/, anders geschrieben),
//      wird genauso nie ausgewertet -- und erwacht mit dem naechsten Namensgleichen als Freibrief.
//      Aufbau: eine BEGRUENDETE Waise (Zeile wie in Fall (2), sie traegt) plus eine zweite Zeile fuer
//      einen Geist. ROT darf dann NUR die Geist-Zeile sein: "davon 1 begruendet" bleibt stehen.
//      Gegenrichtung: dieselbe Allowlist ohne die Geist-Zeile -> GRUEN.
// =============================================================================
TEST(Pa1ToteAusnahme, AllowlistZeileOhneGegenstandImIndexIstUnpruefbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    std::string const lebendig = "tests/unit/kommt_vielleicht_" + marke + ".hpp";
    std::string const geist    = "tests/unit/test_geist_" + marke + ".cpp";
    std::string const tragend  = fall.waise() + " | datei:" + lebendig + " | Koeder " + marke + "\n";
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + tragend + geist +
                                         " | frist:2999-12-31 | Geist " + marke + "\n"));

    Lauf const mit = fall.fahren();
    berichten("AllowlistZeileOhneGegenstandImIndexIstUnpruefbar/mit-Geist-Zeile", mit, marke);
    EXPECT_EQ(mit.code, 1) << "Eine Zeile ohne Gegenstand im Index ist ein schlafender Freibrief -- ROT.\n"
                           << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, geist + " -- UNPRUEFBAR: Allowlist-Zeile ohne Gegenstand")) << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, "davon 1 begruendet")) << "Die tragende Zeile darf nicht mit rot werden.\n"
                                                             << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, "1 unpruefbar")) << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, "1 ohne Gegenstand im Index")) << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, "0 erloschen")) << "Die Frist der Geist-Zeile darf nicht bewertet worden sein.\n"
                                                      << mit.ausgabe;

    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + tragend));
    Lauf const ohne = fall.fahren();
    berichten("AllowlistZeileOhneGegenstandImIndexIstUnpruefbar/ohne-Geist-Zeile", ohne, marke);
    EXPECT_EQ(ohne.code, 0) << "Ohne die Geist-Zeile muss die tragende Zeile allein gruen sein.\n" << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, "0 mit UNPRUEFBARER")) << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, "0 ohne Gegenstand im Index")) << ohne.ausgabe;
}

#endif // _WIN32
