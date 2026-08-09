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
[[nodiscard]] Lauf wache_fahren(fs::path const& baum) {
    fs::path const    ctest_dir = fs::path{COMDARE_D2_CTEST_BIN}.parent_path();
    std::string const befehl    = "PATH=\"" + ctest_dir.string() + ":$PATH\" sh \"" + wachen_pfad() + "\" \"" +
                               baum.string() + "\" 2>&1";

    Lauf  ergebnis;
    FILE* rohr = ::popen(befehl.c_str(), "r");
    if (rohr == nullptr) {
        ADD_FAILURE() << "popen fehlgeschlagen: " << befehl;
        return ergebnis;
    }
    char puffer[4096];
    while (std::fgets(puffer, sizeof puffer, rohr) != nullptr) {
        ergebnis.ausgabe += puffer;
    }
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
        for (auto const& name : testnamen) {
            aus << "add_test(" << name << " \"/bin/true\")\n";
        }
    }

    void protokoll_schreiben(std::vector<std::string> const& zeilen) const {
        std::ofstream aus{wurzel_ / "comdare_registrierungs_protokoll.txt"};
        aus << "# praepariert von test_d2_abdeckungs_wache_nenner\n";
        for (auto const& z : zeilen) {
            aus << z << "\n";
        }
    }

    [[nodiscard]] fs::path const& pfad() const { return wurzel_; }

private:
    fs::path wurzel_;
};

void lauf_berichten(char const* fall, Lauf const& lauf, std::string const& marke) {
    std::cout << "  [D2] Fall '" << fall << "' | Koeder " << marke << " | Prueflig " << wachen_pfad()
              << " | Exit " << lauf.code << "\n";
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
    std::string const marke      = koeder_marke();
    std::string const block      = "koederblock_" + marke;
    std::string const fehlt      = "test_koeder_fehlt_" + marke;
    PraeparierterBaum baum{marke};

    // Der Koeder-Test steht NICHT in der Inventur -- genau das ist der Zustand, den ein
    // uebersprungener Registrierungs-Block erzeugt.
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|" + block + "|UEBERSPRUNGEN|" + fehlt + "|Werkzeug fehlt (praepariert)",
                              "ENDE|1"});

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
    baum.protokoll_schreiben({"BLOCK|" + block + "|AKTIV|" + gibtes + "|Werkzeug vorhanden (praepariert)",
                              "ENDE|1"});

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
    baum.protokoll_schreiben({"BLOCK|" + block + "|AKTIV|" + behauptet + "|behauptet gelaufen (praepariert)",
                              "ENDE|1"});

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
    baum.protokoll_schreiben({"ENDE|0"});

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("null Bloecke", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "ohne einen einzigen Block")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

#endif // !_WIN32
