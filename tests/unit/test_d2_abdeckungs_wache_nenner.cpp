// test_d2_abdeckungs_wache_nenner -- D2 (2026-08-09): DER SELBSTTEST DER ABDECKUNGS-WACHE.
//
// WAS HIER GEPRUEFT WIRD -- und warum es diesen Test ueberhaupt gibt:
//
//   scripts/ci_test_coverage_guard.sh haelt die Vereinigung aller CI-Job-Auswahlen gegen
//   die Live-Inventur 'ctest -N'. Diese Inventur ist ihr NENNER. Sie ist aber nicht
//   konstant: mehrere Test-Registrierungen dieses Projekts stehen in bedingten CMake-
//   Bloecken, die nur laufen, wenn ein 2-Pass-Werkzeug im Bau-Baum schon gebaut ist.
//   Fehlt das Werkzeug, wird der Block uebersprungen -- geraeuschlos. Die Wache rechnete
//   dann korrekt ueber einer zu kleinen Menge und meldete GRUEN.
//
//   Das ist die Fehlerklasse: ein FALSCHES Messgeraet faellt irgendwann auf, ein RICHTIGES
//   Messgeraet am FALSCHEN GEGENSTAND nie. Nichts klappert.
//
// AM OBJEKT BELEGT (2026-08-09, build-covguard dieses Worktrees, Verfahren: die gebaute
// Werkzeug-Binary beiseitelegen, neu konfigurieren, nicht neu bauen):
//   Werkzeuge da .......... 434 Tests -- Wache vor D2: GRUEN | Wache nach D2: GRUEN
//   Emitter weg ........... 432 Tests -- Wache vor D2: GRUEN | Wache nach D2: ROT, Exit 4
//   beide weg ............. 430 Tests -- Wache vor D2: GRUEN | Wache nach D2: ROT, Exit 4
// Die alte Wache war auf allen drei Baeumen gruen. Vier Tests konnten fehlen, ohne dass
// irgendetwas rot wurde.
//
// WARUM EIN GOOGLE TEST UND KEINE WEITERE SHELL-PROBE (Owner-Entscheid 2026-08-09):
//   "Es waere sauberer im cmake-Debug Modus standard google Tests zu fahren und diese in
//    Release zu wiederholen aufgrund von compile regressionen. Skripte sagen gar nichts."
//   Die Wache selbst bleibt vorerst ein sh-Skript (ihr Umbau nach C++ ist ein eigenes
//   Paket) -- ihre PRUEFLOGIK liegt ab hier aber hier, als ctest-Ziel, in Debug wie in
//   Release, ohne eigene Sonder-Ausfuehrung im CI.
//
// K13 -- DER KOEDER MUSS ERST BEISSEN UND DEN RICHTIGEN RISS ZEIGEN:
//   * Jeder Fall wuerfelt seine Kennungen FRISCH aus /dev/urandom. Kein Name ist aus einer
//     Doku abgeschrieben; taucht er in der Ausgabe der Wache auf, kann er nur aus DEM
//     Protokoll stammen, das dieser Fall gerade geschrieben hat.
//   * Geprueft wird nicht "irgendwie rot", sondern EXIT 4 -- der Code, den die Wache
//     ausschliesslich fuer Nenner-Befunde vergibt. Ein Phantom-Gate oder eine
//     Abdeckungsluecke endet mit 1, eine kaputte Umgebung mit 2. Ein Koeder, der nur
//     'nicht 0' prueft, beisst auch bei jedem anderen Defekt und belegte nichts.
//   * Zu jeder Zusicherung ein Gegeneingang (T-4): derselbe Baum mit GELAUFENEM Block
//     darf gerade NICHT mit 4 enden -- sonst waere die Zusicherung eine Konstante.
//
// GEGENPROBE GEGEN DEN EIGENEN MUTANTEN: die Umgebungsvariable COMDARE_D2_WACHE_PFAD
// verschiebt den Prueflings-Pfad. Damit laesst sich dieselbe Test-Binary gegen eine
// praeparierte Wache fahren (z.B. eine, der der Nenner-Befund fehlt); sie MUSS dann rot
// werden. Der Default ist der eincompilierte absolute Pfad der echten Wache, und welcher
// Prueflig gefahren wurde, steht in der Ausgabe jedes Falls.
//
// GRENZE, EHRLICH BENANNT: dieser Test prueft das URTEIL der Wache ueber ein praepariertes
// Protokoll. Dass das Protokoll im echten Bau-Baum vollstaendig ist -- also dass jeder
// bedingte CMake-Block wirklich einen comdare_registrierung_vermerken()-Aufruf in BEIDEN
// Zweigen hat -- kann er nicht sehen; das haelt die Endmarke in
// cmake/registrierungs_protokoll.cmake und der Abbruch bei null Bloecken.
//
// ASCII-only (Leitplanke).

#include "comdare_test_tmp.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

// Die Wache ist ein POSIX-sh-Skript und faehrt in keinem Windows-Job. Der ctest-Eintrag
// entsteht trotzdem UNBEDINGT -- ein weiterer bedingter Registrierungs-Block waere in
// genau diesem Paket die falsche Antwort. Auf Windows meldet der Fall sich als SKIP.
#if defined(_WIN32)

#include <gtest/gtest.h>

TEST(D2AbdeckungsWacheNenner, NurPosix) {
    GTEST_SKIP() << "scripts/ci_test_coverage_guard.sh ist ein POSIX-sh-Skript "
                    "(kein Windows-Job faehrt es).";
}

#else

#include <sys/wait.h>

namespace fs = std::filesystem;

namespace {

// ---------------------------------------------------------------------------
// Der Prueflig: die ECHTE Wache, es sei denn, jemand schiebt bewusst eine andere unter.
// ---------------------------------------------------------------------------
[[nodiscard]] std::string wachen_pfad() {
    if (char const* const ueberschrieben = std::getenv("COMDARE_D2_WACHE_PFAD");
        ueberschrieben != nullptr && *ueberschrieben != '\0') {
        return std::string{ueberschrieben};
    }
    return std::string{COMDARE_D2_GUARD_SH};
}

// ---------------------------------------------------------------------------
// FRISCH GEWUERFELT (K13). /dev/urandom, nicht std::random_device und erst recht keine
// Konstante aus einer Doku: der Wert darf in KEINER Datei dieses Repos vorkommen, sonst
// koennte die Ausgabe der Wache ihn aus einer anderen Quelle haben als aus unserem
// Protokoll.
// ---------------------------------------------------------------------------
[[nodiscard]] std::string koeder_marke() {
    std::ifstream quelle{"/dev/urandom", std::ios::binary};
    EXPECT_TRUE(quelle.good()) << "/dev/urandom nicht lesbar -- ohne frischen Koeder kein Beweis";
    unsigned char rohbytes[6]{};
    quelle.read(reinterpret_cast<char*>(rohbytes), sizeof rohbytes);
    static constexpr char kZiffern[] = "0123456789abcdef";
    std::string           marke;
    for (unsigned char const b : rohbytes) {
        marke.push_back(kZiffern[(b >> 4U) & 0x0FU]);
        marke.push_back(kZiffern[b & 0x0FU]);
    }
    return marke;
}

struct Lauf {
    int         code{-1};
    std::string ausgabe;
};

[[nodiscard]] bool enthaelt(std::string const& heuhaufen, std::string_view nadel) {
    return heuhaufen.find(nadel) != std::string::npos;
}

// ---------------------------------------------------------------------------
// Die Wache auf einem Bau-Baum fahren. ctest wird ueber den von CMake gemeldeten Pfad in
// den PATH gehoben -- die Wache bricht sonst mit "ctest ist nicht im PATH" (Exit 2) ab und
// wir haetten einen ununterscheidbaren zweiten Grund fuer ein Nicht-0.
// ---------------------------------------------------------------------------
// Die UNTERGRENZE wird IMMER in den praeparierten Baum umgelenkt (D2, 2026-08-09).
// Die committete Zahl in scripts/ci_test_inventory_floor.txt gilt fuer den echten
// Baum (dort 492 Tests, Untergrenze 488). Ein praeparierter Baum hat zwei Tests --
// er risse jede echte Untergrenze, und JEDER Fall dieser Datei waere rot aus dem
// falschen Grund. COMDARE_D2_FLOOR_PFAD zeigt deshalb auf eine Datei IM Baum, die
// der jeweilige Fall selbst schreibt. Schreibt er sie nicht, prueft er damit gerade
// den fail-closed-Zweig: eine Wache ohne Untergrenze meldet nicht gruen.
// 'zusatz_env' wird VOR den Aufruf gesetzt (z.B. "COMDARE_WACHE_STRIKT=1 "). Es steht
// bewusst als Zeichenkette da und nicht als putenv(): der Prozess dieses Tests darf die
// Variable nicht behalten, sonst faerbte ein Fall den naechsten ein.
[[nodiscard]] Lauf wache_fahren(fs::path const& baum, std::string const& zusatz_env = "") {
    fs::path const    ctest_dir = fs::path{COMDARE_D2_CTEST_BIN}.parent_path();
    fs::path const    floor     = baum / "ci_test_inventory_floor.txt";
    std::string const befehl = "PATH=\"" + ctest_dir.string() + ":$PATH\" COMDARE_D2_FLOOR_PFAD=\"" + floor.string()
                             + "\" " + zusatz_env + "sh \"" + wachen_pfad() + "\" \"" + baum.string() + "\" 2>&1";

    Lauf  ergebnis;
    FILE* rohr = ::popen(befehl.c_str(), "r");
    if (rohr == nullptr) {
        ADD_FAILURE() << "popen fehlgeschlagen: " << befehl;
        return ergebnis;
    }
    char puffer[4096];
    while (std::fgets(puffer, sizeof puffer, rohr) != nullptr) { ergebnis.ausgabe += puffer; }
    int const status = ::pclose(rohr);
    ergebnis.code    = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return ergebnis;
}

// ---------------------------------------------------------------------------
// Ein praeparierter Bau-Baum: eine handgeschriebene CTestTestfile.cmake (das ist alles,
// was 'ctest -N' liest -- kein Compilat noetig) plus ein Registrierungs-Protokoll.
// ---------------------------------------------------------------------------
class PraeparierterBaum {
public:
    explicit PraeparierterBaum(std::string const& marke)
        : wurzel_{comdare::test::user_tmp_dir() / ("d2_nenner_" + marke)} {
        std::error_code ec;
        fs::create_directories(wurzel_, ec);
    }

    PraeparierterBaum(PraeparierterBaum const&)            = delete;
    PraeparierterBaum& operator=(PraeparierterBaum const&) = delete;
    PraeparierterBaum(PraeparierterBaum&&)                 = delete;
    PraeparierterBaum& operator=(PraeparierterBaum&&)      = delete;

    ~PraeparierterBaum() {
        std::error_code ec;
        fs::remove_all(wurzel_, ec);
    }

    void inventur_schreiben(std::vector<std::string> const& testnamen) const {
        std::ofstream aus{wurzel_ / "CTestTestfile.cmake"};
        for (auto const& name : testnamen) { aus << "add_test(" << name << " \"/bin/true\")\n"; }
    }

    void protokoll_schreiben(std::vector<std::string> const& zeilen) const {
        std::ofstream aus{wurzel_ / "comdare_registrierungs_protokoll.txt"};
        aus << "# praepariert von test_d2_abdeckungs_wache_nenner\n";
        for (auto const& z : zeilen) { aus << z << "\n"; }
    }

    // Die committete Untergrenze dieses Baums. Format wie die echte Datei:
    // '#'-Kommentare erlaubt, GENAU EINE Wertzeile.
    void floor_schreiben(int untergrenze) const {
        std::ofstream aus{wurzel_ / "ci_test_inventory_floor.txt"};
        aus << "# praepariert von test_d2_abdeckungs_wache_nenner\n" << untergrenze << "\n";
    }

    // Roher Inhalt, fuer die Formatfaelle (keine Ganzzahl / zwei Wertzeilen).
    void floor_roh_schreiben(std::string const& inhalt) const {
        std::ofstream aus{wurzel_ / "ci_test_inventory_floor.txt"};
        aus << inhalt;
    }

    // PARTITIONSBRUCH: ein DOPPELNAME, von dem nur EINE Registrierung 'pmc' traegt.
    // 'ctest -N' zaehlt den Namen einmal (sort -u), '-L pmc' und '-LE pmc' zaehlen ihn
    // BEIDE -- die Summe wird groesser als die Inventur. Am Objekt gemessen, ctest
    // 4.3.4: alle=2, -LE pmc=2, -L pmc=1, Summe 3 != 2.
    void inventur_mit_partitionsbruch_schreiben(std::string const& zwilling, std::string const& normal) const {
        std::ofstream aus{wurzel_ / "CTestTestfile.cmake"};
        aus << "add_test(" << zwilling << " \"/bin/true\")\n";
        aus << "set_tests_properties(" << zwilling << " PROPERTIES LABELS \"pmc\")\n";
        aus << "add_test(" << zwilling << " \"/bin/true\")\n";
        aus << "add_test(" << normal << " \"/bin/true\")\n";
    }

    // Eine CTestTestfile.cmake, an der ctest SELBST scheitert (fehlende Klammer):
    // Exit 8 mit "Parse error." auf stderr. Das ist etwas anderes als "0 Treffer".
    void inventur_kaputt_schreiben(std::string const& name) const {
        std::ofstream aus{wurzel_ / "CTestTestfile.cmake"};
        aus << "add_test(" << name << " \"/bin/true\"\n";
    }

    [[nodiscard]] fs::path const& pfad() const { return wurzel_; }

private:
    fs::path wurzel_;
};

void lauf_berichten(char const* fall, Lauf const& lauf, std::string const& marke) {
    std::cout << "  [D2] Fall '" << fall << "' | Koeder " << marke << " | Prueflig " << wachen_pfad() << " | Exit "
              << lauf.code << "\n";
}

// Ein Bau-Baum braucht mindestens einen registrierten Test, sonst bricht die Wache mit
// Exit 2 ab ("Inventur meldet 0 Tests") und wir messen die falsche Sache.
[[nodiscard]] std::string grundstock(std::string const& marke) { return "test_grundstock_" + marke; }

} // namespace

// ===========================================================================================
// (1) DER EIGENTLICHE D2-DEFEKT: ein uebersprungener Block macht die Wache ROT und wird
//     NAMENTLICH genannt. Vor D2 war genau dieser Fall gruen.
// ===========================================================================================
TEST(D2AbdeckungsWacheNenner, UebersprungenerBlockIstRotUndWirdNamentlichGenannt) {
    std::string const marke = koeder_marke();
    std::string const block = "koederblock_" + marke;
    std::string const fehlt = "test_koeder_fehlt_" + marke;
    PraeparierterBaum baum{marke};

    // Der Koeder-Test steht NICHT in der Inventur -- genau das ist der Zustand, den ein
    // uebersprungener Registrierungs-Block erzeugt.
    baum.inventur_schreiben({grundstock(marke)});
    baum.floor_schreiben(1);
    baum.protokoll_schreiben(
        {"BLOCK|" + block + "|UEBERSPRUNGEN|" + fehlt + "|Werkzeug fehlt (praepariert)", "ENDE|1"});

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("uebersprungener Block", lauf, marke);

    EXPECT_EQ(lauf.code, 4) << "Ein uebersprungener Registrierungs-Block MUSS Exit 4 geben "
                               "(Nenner-Befund), nicht 0 und auch nicht irgendein anderes Rot.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "NENNER GESCHRUMPFT")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, block)) << "Der Block muss NAMENTLICH erscheinen.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, fehlt)) << "Der nicht registrierte Test muss NAMENTLICH erscheinen.\n"
                                               << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "ROT (NENNER)")) << lauf.ausgabe;
}

// ===========================================================================================
// (2) T-4 GEGENEINGANG: derselbe Baum, derselbe Koeder -- nur laeuft der Block. Dann darf
//     die Wache gerade NICHT mit dem Nenner-Code enden. Ohne diesen Fall waere (1) auch
//     durch eine Wache erfuellt, die immer 4 zurueckgibt.
// ===========================================================================================
TEST(D2AbdeckungsWacheNenner, GelaufenerBlockIstKeinNennerBefund) {
    std::string const marke  = koeder_marke();
    std::string const block  = "koederblock_" + marke;
    std::string const gibtes = "test_koeder_da_" + marke;
    PraeparierterBaum baum{marke};

    baum.inventur_schreiben({grundstock(marke), gibtes});
    baum.floor_schreiben(1);
    baum.protokoll_schreiben({"BLOCK|" + block + "|AKTIV|" + gibtes + "|Werkzeug vorhanden (praepariert)", "ENDE|1"});

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("gelaufener Block", lauf, marke);

    EXPECT_NE(lauf.code, 4) << "Ein gelaufener Block ist KEIN Nenner-Befund.\n" << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "NENNER GESCHRUMPFT")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "NENNER-WIDERSPRUCH")) << lauf.ausgabe;
    // Beleg, dass die Wache wirklich durchgelaufen ist und nicht frueh abgebrochen hat --
    // sonst waere "kein Nenner-Befund" nur die Abwesenheit jeder Aussage.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "[gelaufen")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, block)) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "BILANZ:")) << lauf.ausgabe;
}

// ===========================================================================================
// (3) DIE GEGENRICHTUNG: ein Block meldet sich als GELAUFEN, sein Test steht aber nicht in
//     der Inventur. Ohne diese Probe waere "AKTIV" eine unpruefbare Behauptung.
// ===========================================================================================
TEST(D2AbdeckungsWacheNenner, AktivOhneRegistriertenTestIstEinWiderspruch) {
    std::string const marke     = koeder_marke();
    std::string const block     = "koederblock_" + marke;
    std::string const behauptet = "test_koeder_behauptet_" + marke;
    PraeparierterBaum baum{marke};

    baum.inventur_schreiben({grundstock(marke)});
    baum.floor_schreiben(1);
    baum.protokoll_schreiben(
        {"BLOCK|" + block + "|AKTIV|" + behauptet + "|behauptet gelaufen (praepariert)", "ENDE|1"});

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("AKTIV ohne Test", lauf, marke);

    EXPECT_EQ(lauf.code, 4) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "NENNER-WIDERSPRUCH")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, behauptet)) << lauf.ausgabe;
}

// ===========================================================================================
// (4) FAIL-CLOSED: ohne Protokoll kann die Wache ihren Nenner nicht belegen. Sie meldet dann
//     nicht gruen, sondern bricht ab (Exit 2, Bedienung/Umgebung -- nicht 4, denn hier ist
//     nichts UEBER den Nenner bekannt, es fehlt die Auskunft selbst).
// ===========================================================================================
TEST(D2AbdeckungsWacheNenner, FehlendesProtokollIstAbbruchStattGruen) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.inventur_schreiben({grundstock(marke)});
    baum.floor_schreiben(1);
    // kein protokoll_schreiben()

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("Protokoll fehlt", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "Nenner ist unbelegt")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// ===========================================================================================
// (5) EIN ABGERISSENES PROTOKOLL SIEHT SONST AUS WIE "EBEN WENIGER BLOECKE". Die Endmarke
//     macht den Unterschied sichtbar: fehlt sie oder passt ihre Zahl nicht, ist Schluss.
// ===========================================================================================
TEST(D2AbdeckungsWacheNenner, ProtokollOhneEndmarkeIstAbbruch) {
    std::string const marke = koeder_marke();
    std::string const block = "koederblock_" + marke;
    PraeparierterBaum baum{marke};
    baum.inventur_schreiben({grundstock(marke)});
    baum.floor_schreiben(1);
    baum.protokoll_schreiben({"BLOCK|" + block + "|AKTIV|" + grundstock(marke) + "|abgerissen (praepariert)"});

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("Endmarke fehlt", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "Endmarke")) << lauf.ausgabe;
}

TEST(D2AbdeckungsWacheNenner, EndmarkeMitFalscherZahlIstAbbruch) {
    std::string const marke = koeder_marke();
    std::string const block = "koederblock_" + marke;
    PraeparierterBaum baum{marke};
    baum.inventur_schreiben({grundstock(marke)});
    baum.floor_schreiben(1);
    // Zwei Bloecke geschrieben, Endmarke behauptet drei -- so sieht ein Lauf aus, der nach
    // dem Abschluss noch etwas angehaengt bekommen hat oder unterwegs Zeilen verloren hat.
    baum.protokoll_schreiben({"BLOCK|" + block + "_a|AKTIV|" + grundstock(marke) + "|praepariert",
                              "BLOCK|" + block + "_b|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|3"});

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("Endmarke luegt", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "widersprechen sich")) << lauf.ausgabe;
}

// ===========================================================================================
// (6) NULL BLOECKE IST KEIN GUELTIGER ZUSTAND, sondern der alte Blindfleck in neuer Form:
//     verschwinden die Vermerke, saehe die Wache wieder nur eine kleinere Inventur.
// ===========================================================================================
TEST(D2AbdeckungsWacheNenner, ProtokollOhneJedenBlockIstAbbruch) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.inventur_schreiben({grundstock(marke)});
    baum.floor_schreiben(1);
    baum.protokoll_schreiben({"ENDE|0"});

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("null Bloecke", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "ohne einen einzigen Block")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// ===========================================================================================
// AB HIER: D2 (Untergrenze) und D2-G3 (drei Wege zu "gruen ohne Vergleich"), 2026-08-09.
//
// WARUM DIESE FAELLE HIER STEHEN UND NICHT IN EINER ZWEITEN SHELL-PROBE: derselbe
// Owner-Entscheid, der oben zitiert ist. Die alte Datei
// scripts/ci_test_coverage_guard.selbsttest.sh bleibt als Altlast liegen, bekommt aber
// keine neue Pruefung mehr -- die neuen Faelle sind ctest-Ziele, in Debug wie in Release.
//
// GEMEINSAMER NENNER ALLER FAELLE (T-3): der praeparierte Baum hat eine BEKANNTE
// Testzahl, die der Fall selbst schreibt -- nicht eine, die er vorfindet. Die
// Untergrenze wird gegen genau diese Zahl gesetzt, nie gegen die echte 488.
// ===========================================================================================

// -- D2: die Untergrenze beisst -------------------------------------------------------------
// Der Baum hat 2 Tests, die Untergrenze fordert eine GEWUERFELTE, deutlich groessere
// Zahl. Vor D2 gab es diese Pruefung nicht: die Wache verlangte nur '> 0'.
TEST(D2AbdeckungsWacheNenner, UntergrenzeUnterschrittenIstNennerBefundMitBeidenZahlen) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.inventur_schreiben({grundstock(marke), "test_zweiter_" + marke});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});

    // Frisch gewuerfelt (K13), garantiert oberhalb der 2 Tests des Baums.
    int const untergrenze = 1000 + (static_cast<int>(std::strtoul(marke.substr(0, 4).c_str(), nullptr, 16)) % 5000);
    baum.floor_schreiben(untergrenze);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("Untergrenze unterschritten", lauf, marke);

    EXPECT_EQ(lauf.code, 4) << "Eine unterschrittene Untergrenze ist ein NENNER-Befund.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "UNTERSCHRITTEN")) << lauf.ausgabe;
    // BEIDE Zahlen muessen dastehen -- eine nackte Meldung "zu wenige Tests" waere
    // nicht nachpruefbar (V-1).
    EXPECT_TRUE(enthaelt(lauf.ausgabe, std::to_string(untergrenze))) << "Die geforderte Zahl fehlt.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "2")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "ROT (NENNER)")) << lauf.ausgabe;
}

// -- D2, T-4 GEGENEINGANG: genau erreicht ist KEIN Befund -----------------------------------
// Ohne diesen Fall waere die Zusicherung oben auch von einer Wache erfuellt, die JEDE
// Untergrenze als unterschritten meldet.
TEST(D2AbdeckungsWacheNenner, UntergrenzeGenauErreichtIstKeinBefund) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.inventur_schreiben({grundstock(marke), "test_zweiter_" + marke});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(2); // exakt die Zahl der geschriebenen Tests

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("Untergrenze genau erreicht", lauf, marke);

    EXPECT_NE(lauf.code, 4) << "Gleichstand ist kein Nenner-Befund.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "genau erreicht")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "UNTERSCHRITTEN")) << lauf.ausgabe;
}

// -- D2 FAIL-CLOSED: keine Untergrenze ist kein "dann eben ohne" ----------------------------
TEST(D2AbdeckungsWacheNenner, FehlendeUntergrenzeIstAbbruchStattGruen) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    // kein floor_schreiben() -- genau das ist der Pruefgegenstand.

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("Untergrenze fehlt", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "Untergrenze fehlt")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// -- D2 FAIL-CLOSED: unlesbare Untergrenze wird nicht als "keine" behandelt ------------------
TEST(D2AbdeckungsWacheNenner, UntergrenzeOhneGanzzahlIstAbbruch) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_roh_schreiben("# praepariert\n" + marke + "\n"); // Hex-Marke, keine Zahl

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("Untergrenze keine Ganzzahl", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "keine reine Ganzzahl")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, marke)) << "Der unbrauchbare Wert gehoert NAMENTLICH in die Meldung.\n"
                                               << lauf.ausgabe;
}

TEST(D2AbdeckungsWacheNenner, UntergrenzeMitZweiWertzeilenIstAbbruch) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_roh_schreiben("1\n2\n");

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("Untergrenze zweideutig", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "GENAU EINE")) << lauf.ausgabe;
}

// -- D2-G3.2: der Partitions-Beleg rechnet, statt '==' zu drucken ---------------------------
// Konstruktion des Widerspruchs: ein DOPPELNAME, von dem eine Registrierung 'pmc' traegt.
// 'ctest -N' zaehlt den Namen einmal, '-L pmc' und '-LE pmc' zaehlen ihn beide.
TEST(D2AbdeckungsWacheNenner, PartitionsWiderspruchIstNennerBefundMitBeidenZahlen) {
    std::string const marke    = koeder_marke();
    std::string const zwilling = "test_zwilling_" + marke;
    std::string const normal   = "test_normal_" + marke;
    PraeparierterBaum baum{marke};
    baum.inventur_mit_partitionsbruch_schreiben(zwilling, normal);
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + normal + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(1);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("Partitions-Widerspruch", lauf, marke);

    EXPECT_EQ(lauf.code, 4) << "Ein Partitions-Widerspruch ist ein NENNER-Befund, kein Abdeckungs-Befund.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "PARTITIONS-WIDERSPRUCH")) << lauf.ausgabe;
    // Die Zeile darf nicht wieder nur eine Zeichenkette sein: Summe UND Inventur
    // muessen als getrennte Zahlen erscheinen, dazu die Differenz.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "Summe")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "Differenz")) << lauf.ausgabe;
}

// -- D2-G3.3: ein abgestuerztes ctest ist ein WERKZEUG-Fehler, kein Phantom-Gate ------------
// Vor der Heilung verschluckte '2>/dev/null' den Grund, und null Namen sahen aus wie
// "der Selektor trifft nichts".
TEST(D2AbdeckungsWacheNenner, KaputteInventurIstWerkzeugFehlerKeinPhantomGate) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.inventur_kaputt_schreiben(grundstock(marke));
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    // BEWUSST OHNE Untergrenze: der Werkzeug-Fehler muss VOR ihr greifen. Faende die
    // Wache hier zuerst die fehlende Untergrenze, waere die Diagnose wieder die falsche.

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("ctest abgestuerzt", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << "Ein abgestuerztes Werkzeug ist Umgebung (2), nicht Abdeckung (1).\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "DAS WERKZEUG SELBST HAT VERSAGT")) << lauf.ausgabe;
    // Der aufgefangene Fehlerkanal MUSS sichtbar sein -- das ist der ganze Punkt.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "Parse error")) << "ctest-stderr fehlt in der Ausgabe.\n" << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "PHANTOM-GATE")) << "Falsche Anschuldigung gegen den Selektor.\n"
                                                        << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "Untergrenze fehlt")) << "Der Werkzeug-Fehler muss zuerst greifen.\n"
                                                             << lauf.ausgabe;
}

// -- D2-G3.4: ein ungeprueftes Gate wird gezaehlt und genannt -------------------------------
// KEINE Rot-Haerte: der Lauf ausserhalb der CI hat diese Variablen zu Recht nicht
// gesetzt. Geprueft wird, dass die Annahme SICHTBAR ist -- und dass sie den Exit-Code
// gerade NICHT veraendert (sonst waere jeder lokale Lauf rot).
TEST(D2AbdeckungsWacheNenner, UngepruefteGatesWerdenGezaehltUndAendernDenRcNicht) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(1);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("ungepruefte Gates", lauf, marke);

    // Der Zaehler steht in der Ausgabe, MIT Nenner (V-1: nie eine nackte Zahl).
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "UNGEPRUEFT")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "ANNAHME")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "Gate(n)")) << lauf.ausgabe;
    // Und er macht gerade KEINEN Nenner-Befund daraus.
    EXPECT_NE(lauf.code, 4) << "Ein ungesetztes Deklarations-Gate ist kein Nenner-Defekt.\n" << lauf.ausgabe;
    // Der MODUS steht in der Ausgabe -- ohne ihn waere einem gruenen Ergebnis nicht
    // anzusehen, nach welchem Massstab es zustande kam.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "Modus")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "WEICH")) << lauf.ausgabe;
}

// -- STRENG-MODUS (Lead-Entscheid 2026-08-09) -----------------------------------------------
// Der Schalter beantwortet "wird hier Vollstaendigkeit erwartet?" -- ausdruecklich NICHT
// "laufe ich in GitLab?". $CI_JOB_ID waere ein Stellvertreter: auf dem baremetal-Zweig des
// Par. 61 Dual-Wegs waere er leer, die Wache liefe weich, und die Abnahme saehe aus wie ein
// Freispruch. Diese drei Faelle halten den Schalter an seiner Bedeutung fest.
TEST(D2AbdeckungsWacheNenner, StrengModusMachtUngepruefteGatesZumFehler) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(1);

    Lauf const lauf = wache_fahren(baum.pfad(), "COMDARE_WACHE_STRIKT=1 ");
    lauf_berichten("streng, Variable ungesetzt", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << "Fehlende Auskunft ist ein VERDRAHTUNGS-Fehler (2), kein Befund (1/4).\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "STRENG")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "OHNE AUSKUNFT")) << lauf.ausgabe;
    // Die betroffene Variable MUSS namentlich dastehen -- sonst waere die Meldung
    // nicht behebbar, nur beunruhigend (V-1).
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "COMDARE_PMC_LANES")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// T-4 GEGENEINGANG: mit gesetzter Variable ist der strenge Lauf gerade NICHT rot.
// Ohne diesen Fall waere die Zusicherung oben auch von einer Wache erfuellt, die im
// strengen Modus immer abbricht -- also von Daueralarm.
TEST(D2AbdeckungsWacheNenner, StrengModusMitGesetzterVariableBrichtNichtAb) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(1);

    Lauf const lauf = wache_fahren(baum.pfad(), "COMDARE_WACHE_STRIKT=1 COMDARE_PMC_LANES=amd ");
    lauf_berichten("streng, Variable gesetzt", lauf, marke);

    EXPECT_NE(lauf.code, 2) << "Mit gesetzter Variable darf der strenge Modus nicht abbrechen.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "STRENG")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "OHNE AUSKUNFT")) << lauf.ausgabe;
}

// Ein unlesbarer Schalter wird nicht als "weich" gelesen -- dieselbe Doktrin wie bei der
// Untergrenze. Wer 'COMDARE_WACHE_STRIKT=ja' schreibt, meinte streng.
TEST(D2AbdeckungsWacheNenner, UnlesbarerStrengSchalterIstAbbruchStattWeich) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(1);

    // Der Wert ist die frisch gewuerfelte Marke: garantiert weder '1' noch leer.
    Lauf const lauf = wache_fahren(baum.pfad(), "COMDARE_WACHE_STRIKT=" + marke + " ");
    lauf_berichten("Schalter unlesbar", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "COMDARE_WACHE_STRIKT")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, marke)) << "Der unbrauchbare Wert gehoert in die Meldung.\n" << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

#endif // !_WIN32
