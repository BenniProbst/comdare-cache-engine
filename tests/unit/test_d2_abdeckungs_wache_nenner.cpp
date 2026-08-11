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

#include <array>
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
//
// DIE UMGEBUNG WIRD GESCHEUERT (2026-08-10) -- und das ist kein Feinschliff, sondern die
// Heilung eines Fehlers, der die ce-Pipeline 15515 rot gemacht hat:
//   popen() startet die Prueflings-Subshell ueber 'sh -c' und vererbt ihr die VOLLE
//   Prozessumgebung. In GitLab steht COMDARE_PMC_LANES pipeline-weit auf "amd intel"
//   (.gitlab-ci.yml, variables:-Block -- nicht job-lokal). Die zwei Faelle, die einen
//   UNGESETZTEN Deklarations-Schalter brauchen, sahen dort deshalb "14 von 14 Gate(n)
//   gegen eine gesetzte Variable GEPRUEFT" und fielen -- lokal gruen, in der CI rot,
//   ohne dass am Pruefling irgendetwas defekt gewesen waere.
//   Literal aus dem Job-Trace (Job 371127, prod2, 2026-08-10T03:28:14):
//     [deckt ]  pmc   -L pmc   0 Tests   (COMDARE_PMC_LANES='amd intel')
//     14 von 14 Gate(n) gegen eine gesetzte Variable GEPRUEFT.
//
// WARUM EINE LISTE UND NICHT NUR DIESE EINE VARIABLE (T-6 SCHWESTERPFLICHT): der Riss
// ist nicht 'COMDARE_PMC_LANES', sondern 'die Umgebung entscheidet mit'. Gescheuert wird
// deshalb JEDE Variable, die das Urteil der Wache von aussen verschieben kann --
// die Gate-Variablen des Manifests (declared:/optin:) und die Schalter der Wache selbst.
// Dass die Liste vollstaendig BLEIBT, prueft nicht der gute Wille, sondern der Fall
// ScheuerlisteDecktJedeGateVariableDesManifests: er liest die Namen aus dem MANIFEST
// (fremde Quelle, T-3) und faellt, sobald dort eine Variable dazukommt, die hier fehlt.
inline constexpr std::array<std::string_view, 4> kUmgebungsScheuerliste{
    "COMDARE_PMC_LANES",     // declared:-Gate 'pmc'      -- steht pipeline-weit in der CI
    "COMDARE_ISA_MATRIX",    // optin:-Gate 'arm64_smoke' -- heute nur in rules:, morgen vielleicht global
    "COMDARE_WACHE_STRIKT",  // WEICH/STRENG -- ein geerbtes '1' machte jeden weichen Fall rot
    "COMDARE_FREMD_INVENTUR" // schaltet Befund (e) scharf, gegen eine Datei, die kein Fall geschrieben hat
};

[[nodiscard]] Lauf wache_fahren(fs::path const& baum, std::string const& zusatz_env = "") {
    fs::path const ctest_dir = fs::path{COMDARE_D2_CTEST_BIN}.parent_path();
    fs::path const floor     = baum / "ci_test_inventory_floor.txt";

    // 'env -u' entfernt ZUERST, die Zuweisungen dahinter wirken DANACH: ein Fall, der eine
    // dieser Variablen bewusst SETZT (zusatz_env), bekommt sie also weiterhin -- er bekommt
    // sie nur nicht mehr geschenkt.
    std::string scheuern{"env"};
    for (std::string_view const variable : kUmgebungsScheuerliste) {
        scheuern += " -u ";
        scheuern += variable;
    }

    std::string const befehl = scheuern + " PATH=\"" + ctest_dir.string() + ":$PATH\" COMDARE_D2_FLOOR_PFAD=\"" +
                               floor.string() + "\" " + zusatz_env + "sh \"" + wachen_pfad() + "\" \"" + baum.string() +
                               "\" 2>&1";

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
    // Der Baum ist per Vorgabe ein Baum der REICHSTEN Klasse (avx512f). Das ist keine
    // Bequemlichkeit, sondern die Fortschreibung des Zustands, den alle Faelle vor
    // D2-G5 stillschweigend hatten: gebaut auf prod1, beide ISA-Proben positiv. Faelle,
    // die eine ANDERE Klasse brauchen, sagen das ausdruecklich (cmakecache_schreiben);
    // der Fall, der die fehlende Klasse prueft, loescht sie ausdruecklich.
    explicit PraeparierterBaum(std::string const& marke)
        : wurzel_{comdare::test::user_tmp_dir() / ("d2_nenner_" + marke)} {
        std::error_code ec;
        fs::create_directories(wurzel_, ec);
        cmakecache_schreiben("1", "1");
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

    // Die HOST-KLASSE dieses Baums -- in GENAU der Form, in der CMake sie wirklich
    // hinterlegt. AM OBJEKT GEMESSEN (2026-08-10, CMake 4.3.4, /tmp/d2probe_483e0110),
    // und im Quelltext des Moduls bestaetigt (Modules/Internal/CheckSourceRuns.cmake:95
    // try_run -> _EXITCODE + _COMPILED, :112-121 -> der Wert ist 1 ODER LEER, nie 0):
    //     Probe laeuft, Exit 0 ..... VAR:INTERNAL=1  VAR_COMPILED=TRUE   VAR_EXITCODE=0
    //     Probe laeuft, Exit 1 ..... VAR:INTERNAL=   VAR_COMPILED=TRUE   VAR_EXITCODE=1
    //     Probe kompiliert nicht ... VAR:INTERNAL=   VAR_COMPILED=FALSE  (KEINE EXITCODE-Zeile)
    // Alle drei Zeilen gehoeren deshalb dazu. Die _EXITCODE-Zeile ist nicht Beiwerk: sie
    // ist die einzige Angabe, die einen leeren Wert ("diese CPU kann es nicht") von einem
    // untergeschobenen leeren Wert unterscheidet -- siehe die Faelle ab
    // ErzwungeneKlasseUeberlebtDieCompiledZeile.
    void cmakecache_schreiben(char const* avx2, char const* avx512f) const {
        std::ofstream aus{wurzel_ / "CMakeCache.txt"};
        aus << "# praepariert von test_d2_abdeckungs_wache_nenner\n";
        isa_probe_schreiben(aus, "COMDARE_HOST_RUNS_AVX2", avx2);
        isa_probe_schreiben(aus, "COMDARE_HOST_RUNS_AVX512F", avx512f);
    }

    // Eine EHRLICHE Probe in drei Zeilen. Der Exit-Code folgt dem Wert und wird nicht frei
    // gewaehlt: genau diese Kopplung ist es, die CMake herstellt.
    static void isa_probe_schreiben(std::ofstream& aus, char const* name, std::string const& wert) {
        aus << name << ":INTERNAL=" << wert << "\n"
            << name << "_COMPILED:INTERNAL=TRUE\n"
            << name << "_EXITCODE:INTERNAL=" << (wert == "1" ? "0" : "1") << "\n";
    }

    // Roher Cache-Inhalt, fuer die Faelle, in denen gerade die FORM das Pruefstueck ist.
    void cmakecache_roh_schreiben(std::string const& inhalt) const {
        std::ofstream aus{wurzel_ / "CMakeCache.txt"};
        aus << inhalt;
    }

    void cmakecache_loeschen() const {
        std::error_code ec;
        fs::remove(wurzel_ / "CMakeCache.txt", ec);
    }

    // Die committete Untergrenze dieses Baums. Format wie die echte Datei:
    // '#'-Kommentare erlaubt, GENAU EINE Wertzeile JE HOST-KLASSE, alle drei da.
    //
    // Die einstellige Form setzt alle drei Klassen auf DIESELBE Zahl. Sie ist fuer die
    // Faelle da, denen die Klasse gleichgueltig ist -- sie pruefen etwas anderes, und
    // ein Klassenunterschied waere dort nur eine zweite, unbeteiligte Ursache.
    void floor_schreiben(int untergrenze) const { floor_schreiben(untergrenze, untergrenze, untergrenze); }

    void floor_schreiben(int avx512f, int avx2, int basis) const {
        std::ofstream aus{wurzel_ / "ci_test_inventory_floor.txt"};
        aus << "# praepariert von test_d2_abdeckungs_wache_nenner\n"
            << "avx512f " << avx512f << "\n"
            << "avx2 " << avx2 << "\n"
            << "basis " << basis << "\n";
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

    // HEILE, aber ausdruecklich NICHT TRIVIALE Partition -- der Gegeneingang zum Bruch
    // darueber. Beide Klassen sind besetzt und kein Name kommt zweimal vor: 'ctest -N'
    // zaehlt N, '-LE pmc' die ohne Label, '-L pmc' die mit; die Summe MUSS N sein.
    //
    // WARUM NICHT EINFACH inventur_schreiben(): ein Baum ohne einen einzigen 'pmc'-Test
    // hat '-L pmc' = 0. Die Gleichung ginge dort auch bei einer Wache auf, die die zweite
    // Zahl gar nicht erst erhebt (Mutant 'CE_MIT_PMC=0' -- gefahren, siehe Koeder-Protokoll
    // im Bericht). Ein Freispruch ueber einer trivialen Partition belegt nichts; die
    // Asymmetrie (mehr ohne als mit) faengt zusaetzlich den Vertauschungs-Mutanten.
    void inventur_mit_heiler_partition_schreiben(std::vector<std::string> const& ohne_pmc,
                                                 std::vector<std::string> const& mit_pmc) const {
        std::ofstream aus{wurzel_ / "CTestTestfile.cmake"};
        for (auto const& name : ohne_pmc) { aus << "add_test(" << name << " \"/bin/true\")\n"; }
        for (auto const& name : mit_pmc) {
            aus << "add_test(" << name << " \"/bin/true\")\n";
            aus << "set_tests_properties(" << name << " PROPERTIES LABELS \"pmc\")\n";
        }
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
    // Klasse richtig, WERT unbrauchbar: die Hex-Marke ist keine Zahl.
    baum.floor_roh_schreiben("# praepariert\navx512f " + marke + "\n");

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
    // Zwei Zahlen fuer DIESELBE Klasse. Vor D2-G5 hiess derselbe Fall "1\n2\n" -- die
    // Zweideutigkeit ist dieselbe geblieben, sie hat jetzt nur einen Gegenstand.
    baum.floor_roh_schreiben("avx512f 1\navx512f 2\n");

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

    // NACHGEZOGEN 2026-08-10: die zwei Zusicherungen darueber pruefen WOERTER, keine
    // ZAHLEN -- ein Pruefling, der 'Summe' und 'Differenz' mit falschen Werten druckt,
    // ueberlebt sie. Die Rechen-Zeile traegt beide Seiten des Widerspruchs als Zahlen,
    // und sie ist DIESELBE Zeile, die der Gegeneingang unten mit differenz=0 verlangt.
    // Am Objekt gemessen (ctest 4.3.4): alle=2, -LE pmc=2, -L pmc=1, Summe 3, Differenz 1.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "PARTITIONS-RECHNUNG: ohne=2 mit=1 summe=3 inventur=2 differenz=1"))
        << lauf.ausgabe;
}

// -- D2-G3.2, T-4 GEGENEINGANG: eine HEILE Partition MUSS SCHWEIGEN -------------------------
// Ohne diesen Fall war der Biss darueber auch von einer Wache erfuellt, die JEDEN Baum als
// Partitions-Widerspruch meldet. Die Untergrenze und der Registrierungs-Block haben ihren
// Gegeneingang seit dem 2026-08-09; die Partition hatte bis heute keinen.
//
// WARUM DIESER FALL NICHT AUS 'EXPECT_NE(code, 4) + EXPECT_FALSE(...)' BESTEHT -- und das
// ist der ganze Punkt: eine solche Fassung haelt auch dann, wenn die Wache an dieser Stelle
// gar nicht gerechnet hat. Sie haelt sogar, wenn das Werkzeug nie gestartet ist. Das ist die
// Klasse "gruenes Gate ohne Gegenstand", und eine Luecke mit einem solchen Orakel zu
// schliessen ist schlimmer, als sie offen zu lassen: danach gilt sie als gedeckt.
// Deshalb steht hier ZUERST der DURCHLAUF-BELEG (Muster: GelaufenerBlockIstKeinNennerBefund
// weiter oben, der 'BILANZ:' verlangt) und ERST DANN die Abwesenheit des Befunds.
TEST(D2AbdeckungsWacheNenner, HeilePartitionSchweigtUndDieRechnungIstBelegt) {
    std::string const marke = koeder_marke();

    // T-3: der Nenner kommt aus DIESEM Fall, nicht aus dem Pruefling. Die Zusammensetzung
    // ist asymmetrisch (2 ohne, 1 mit) -- eine 1:1-Aufteilung koennte ein Vertauschen der
    // beiden Zahlen nicht von der Wahrheit unterscheiden.
    std::vector<std::string> const ohne_pmc{grundstock(marke), "test_ohne_pmc_" + marke};
    std::vector<std::string> const mit_pmc{"test_mit_pmc_" + marke};
    int const                      kOhne = static_cast<int>(ohne_pmc.size());
    int const                      kMit  = static_cast<int>(mit_pmc.size());
    int const                      kAlle = kOhne + kMit;

    PraeparierterBaum baum{marke};
    baum.inventur_mit_heiler_partition_schreiben(ohne_pmc, mit_pmc);
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(kAlle);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("heile Partition", lauf, marke);
    std::cout << "  [D2] Partition dieses Baums: ohne 'pmc' " << kOhne << " / mit 'pmc' " << kMit << " / Summe "
              << kAlle << " / Inventur " << kAlle << " / Differenz 0"
              << " -- erwartete Nenner-Befunde: 0 von 1 moeglichen (PARTITIONS-WIDERSPRUCH)\n";

    // (1) DURCHLAUF-BELEG MIT ZAHLEN. Diese eine Zusicherung bindet dreierlei zusammen:
    //     dass die Wache an der Partitions-Stelle ueberhaupt gelaufen ist, dass sie BEIDE
    //     Teilmengen erhoben hat (ohne=2 UND mit=1 -- ein 'CE_MIT_PMC=0'-Mutant liest sich
    //     hier als 'mit=0 summe=2'), und dass sie die Differenz gebildet hat.
    std::string const beleg = "PARTITIONS-RECHNUNG: ohne=" + std::to_string(kOhne) + " mit=" + std::to_string(kMit) +
                              " summe=" + std::to_string(kAlle) + " inventur=" + std::to_string(kAlle) + " differenz=0";
    EXPECT_TRUE(enthaelt(lauf.ausgabe, beleg))
        << "Erwartet wurde woertlich '" << beleg
        << "'. Fehlt sie, ist 'kein Widerspruch' die Abwesenheit jeder Aussage.\n"
           "Und sie darf nicht am Gesamturteil haengen: ein praeparierter Baum ist nie gruen "
           "(die -R-Selektoren des Manifests treffen dort nichts), das Gesamturteil dieses "
           "Laufs war Exit "
        << lauf.code << ". Genau deshalb reicht der PARTITIONS-BELEG im gruenen Schwanz nicht.\n"
        << lauf.ausgabe;

    // (2) ZWEITER, UNABHAENGIGER DURCHLAUF-BELEG (Muster des Nachbarfalls): 'BILANZ:' steht
    //     hinter der Partitions-Rechnung. Wer sie liest, hat die Rechnung passiert.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "BILANZ:")) << lauf.ausgabe;

    // (3) ERST JETZT die Abwesenheit des Befunds -- ueber einem belegten Gegenstand.
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "PARTITIONS-WIDERSPRUCH")) << lauf.ausgabe;
    EXPECT_NE(lauf.code, 4) << "Eine heile Partition ist KEIN Nenner-Befund.\n" << lauf.ausgabe;
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

// ===========================================================================================
// D2-G5 (2026-08-10): DIE UNTERGRENZE IST HOST-KLASSEN-FAEHIG
// ===========================================================================================
// WAS VORHER FALSCH WAR -- und zwar nachrechenbar, nicht dem Gefuehl nach: es gab GENAU EINE
// committete Zahl, und der Kopf der Datei behauptete, sie gelte "auf JEDER Hardware-Klasse".
// Sechs ctest-Registrierungen dieses Repos haengen an der ISA des BAU-HOSTS; vier fallen beim
// Schritt avx512f -> avx2, zwei weitere beim Schritt avx2 -> basis. Die alte Zahl zog VIER ab
// und galt damit fuer 'avx2' -- auf einer Maschine ganz ohne AVX2 haette sie ROT gemeldet, an
// einem voellig gesunden Baum.
//
// GEGENORAKEL (V-7, die Zahl kommt aus einer anderen Quelle als der Rechnung): denselben Baum
// mit erzwungener Klasse NEU KONFIGURIEREN und die Namensmengen vergleichen. Gemessen am
// 2026-08-10, Stand 9f932e91, Bau-Baum build-p, Host prod1:
//     avx512f .......................................... 496 Tests
//     -DCOMDARE_HOST_RUNS_AVX512F=0            -> avx2 ... 492 Tests  (-4)
//     -DCOMDARE_HOST_RUNS_AVX2=0 -D...AVX512F=0 -> basis .. 490 Tests  (-2)
// Beide Gegenrichtungen waren LEER -- es kommt bei keinem Schritt etwas hinzu.
//
// WORAN DIE KLASSE HAENGT -- und warum nicht an /proc/cpuinfo: gefragt ist nicht, was die
// Maschine kann, auf der die Wache gerade laeuft, sondern was der BAU dieses Baums vorgefunden
// hat. Nur die CMakeCache.txt DESSELBEN Baums haelt das fest.

// -- D2-G5 ABNAHME: die avx512f-Zahl gegen einen avx2-Baum MUSS rot sein --------------------
// Das ist der Fall, den die alte Bauform nicht haben KONNTE: sie kannte nur eine Zahl, also
// gab es keine "falsche Klasse", gegen die man haette halten koennen.
TEST(D2AbdeckungsWacheNenner, Avx512fZahlGegenAvx2BaumIstNennerBefund) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_schreiben("1", ""); // AVX2 ja, AVX-512F nein -> Klasse avx2
    baum.inventur_schreiben({grundstock(marke), "test_zweiter_" + marke});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});

    // Frisch gewuerfelt (K13), garantiert oberhalb der 2 Tests des Baums.
    int const avx512f_zahl = 1000 + (static_cast<int>(std::strtoul(marke.substr(0, 4).c_str(), nullptr, 16)) % 5000);
    // Die avx2-ZEILE traegt die avx512f-ZAHL -- genau der Irrtum, den D2-G5 heilt.
    baum.floor_schreiben(avx512f_zahl, avx512f_zahl, 1);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("avx512f-Zahl gegen avx2-Baum", lauf, marke);

    EXPECT_EQ(lauf.code, 4) << "Eine unterschrittene Klassen-Untergrenze ist ein NENNER-Befund.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "UNTERSCHRITTEN")) << lauf.ausgabe;
    // V-1: die Zahl OHNE die Klasse waere eine Zahl ohne Gegenstand. Beides muss dastehen.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, std::to_string(avx512f_zahl))) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "Klasse avx2")) << "Die Klasse fehlt in der Meldung.\n" << lauf.ausgabe;
}

// -- T-4 GEGENEINGANG zum Fall darueber: dieselbe Lage, NUR die avx2-Zahl richtig -----------
// Ohne ihn waere die Zusicherung oben auch von einer Wache erfuellt, die auf einem avx2-Baum
// grundsaetzlich rot meldet. Die beiden Faelle unterscheiden sich in GENAU EINER Zahl.
TEST(D2AbdeckungsWacheNenner, Avx2ZahlGegenAvx2BaumIstKeinBefund) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_schreiben("1", "");
    baum.inventur_schreiben({grundstock(marke), "test_zweiter_" + marke});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});

    int const avx512f_zahl = 1000 + (static_cast<int>(std::strtoul(marke.substr(0, 4).c_str(), nullptr, 16)) % 5000);
    baum.floor_schreiben(avx512f_zahl, 2, 1); // avx2-Zeile jetzt die RICHTIGE Zahl

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("avx2-Zahl gegen avx2-Baum", lauf, marke);

    EXPECT_NE(lauf.code, 4) << "Die avx512f-Zahl darf einen avx2-Baum nicht mehr treffen.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "Klasse avx2")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "UNTERSCHRITTEN")) << lauf.ausgabe;
    // Die grosse Zahl steht in der Datei, wird hier aber NICHT angewendet -- sonst haette
    // die Wache nur zufaellig die richtige Zeile getroffen.
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "fuer Klasse avx2: " + std::to_string(avx512f_zahl))) << lauf.ausgabe;
}

// -- D2-G5: der 'basis'-Baum, an dem die ALTE Zahl zerbrochen waere -------------------------
// Historisch konkret: 492 gemessen, 488 committet, und ein Host ohne AVX2 haette 486 gehabt.
// Hier in klein nachgebaut -- der Baum liegt UNTER der avx2-Zahl und GENAU auf der basis-Zahl.
TEST(D2AbdeckungsWacheNenner, BasisBaumNutztDieBasisZeileNichtDieAvx2Zeile) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_schreiben("", ""); // weder AVX2 noch AVX-512F -> Klasse basis
    baum.inventur_schreiben({grundstock(marke), "test_zweiter_" + marke});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(6, 4, 2); // die Leiter im Kleinen: -2 und -2

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("basis-Baum gegen basis-Zeile", lauf, marke);

    EXPECT_NE(lauf.code, 4) << "Ein gesunder basis-Baum ist kein Nenner-Befund.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "Klasse basis")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "genau erreicht")) << lauf.ausgabe;
    // Die avx2-Zahl (4) liegt ueber den 2 Tests des Baums. Haette die Wache SIE genommen,
    // stuende hier UNTERSCHRITTEN -- genau das war der alte Defekt.
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "UNTERSCHRITTEN")) << lauf.ausgabe;
}

// -- V-1: die Klasse und ihre Herkunft stehen in der AUSGABE --------------------------------
// Eine Testzahl ohne Host-Angabe ist eine Zahl ohne Gegenstand. Geprueft wird nicht, DASS
// gerechnet wurde, sondern dass das Ergebnis nachpruefbar beschriftet ist.
TEST(D2AbdeckungsWacheNenner, HostKlasseUndIhreQuelleStehenInDerAusgabe) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_schreiben("1", "");
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(1);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("Klasse in der Ausgabe", lauf, marke);

    EXPECT_TRUE(enthaelt(lauf.ausgabe, "HOST-KLASSE")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "avx2")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "COMDARE_HOST_RUNS_AVX512F=nein")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "COMDARE_HOST_RUNS_AVX2=ja")) << lauf.ausgabe;
    // Die QUELLE gehoert dazu: ohne sie ist die Klasse eine Behauptung.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "CMakeCache.txt")) << lauf.ausgabe;
}

// -- FAIL-CLOSED: keine bestimmbare Klasse ist ABBRUCH, nicht 'dann eben basis' -------------
TEST(D2AbdeckungsWacheNenner, FehlenderCmakeCacheIstAbbruchStattAnnahme) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_loeschen(); // genau das ist der Pruefgegenstand
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(1);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("CMakeCache fehlt", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "Host-Klasse")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// -- FAIL-CLOSED, DIE SCHAERFERE HAELFTE: leerer Wert MIT _COMPILED=FALSE -------------------
// Hier trennt sich eine ehrliche von einer bequemen Umsetzung. 'VAR:INTERNAL=' heisst NICHT
// "diese CPU kann es nicht" -- es heisst das nur, wenn die Probe auch wirklich uebersetzt hat.
// Wer bloss den Wert liest, stuft einen kaputten Werkzeugkasten als 'basis' ein und vergleicht
// gegen die NIEDRIGSTE Zahl: ein stilles Durchwinken, genau falschherum.
TEST(D2AbdeckungsWacheNenner, NichtKompilierteProbeIstAbbruchStattBasisKlasse) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_roh_schreiben("COMDARE_HOST_RUNS_AVX2:INTERNAL=\n"
                                  "COMDARE_HOST_RUNS_AVX2_COMPILED:INTERNAL=FALSE\n"
                                  "COMDARE_HOST_RUNS_AVX512F:INTERNAL=\n"
                                  "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=FALSE\n");
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(1);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("ISA-Probe nicht kompiliert", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << "Eine nicht uebersetzte Probe ist 'unbekannt', nicht 'basis'.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "_COMPILED")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "Klasse basis")) << "Die Klasse wurde geraten.\n" << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// -- FAIL-CLOSED: eine von aussen gesetzte Klasse ist keine gemessene -----------------------
// AM OBJEKT GEMESSEN: 'cmake -DCOMDARE_HOST_RUNS_AVX2=0' hinterlaesst
// 'COMDARE_HOST_RUNS_AVX2:UNINITIALIZED=0' und KEINE _COMPILED-Zeile -- die Probe lief nie.
TEST(D2AbdeckungsWacheNenner, VonAussenGesetzteKlasseIstAbbruch) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_roh_schreiben("COMDARE_HOST_RUNS_AVX2:UNINITIALIZED=1\n"
                                  "COMDARE_HOST_RUNS_AVX512F:UNINITIALIZED=0\n");
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(1);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("Klasse von aussen gesetzt", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "_COMPILED")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// ===========================================================================================
// DER FUND DES PRUEFERS (2026-08-10, Nachbesserung): _COMPILED UEBERLEBT DAS ERZWINGEN.
// ===========================================================================================
// Der Fall darueber deckt nur einen FRISCHEN Baum ab. Faehrt jemand das Verfahren, das der
// Kopf von scripts/ci_test_inventory_floor.txt als Gegenorakel VORSCHREIBT -- erst ehrlich
// konfigurieren, DANN die Klasse per -D erzwingen --, sieht der Cache anders aus.
//
// AM OBJEKT GEMESSEN, CMake 4.3.4, Marke 483e0110d81153e9, /tmp/d2probe_483e0110:
//   Schritt 1  cmake -S src -B b
//              COMDARE_HOST_RUNS_AVX2:INTERNAL=1
//              COMDARE_HOST_RUNS_AVX2_COMPILED:INTERNAL=TRUE
//              COMDARE_HOST_RUNS_AVX2_EXITCODE:INTERNAL=0
//   Schritt 2  cmake -S src -B b -DCOMDARE_HOST_RUNS_AVX2=0 -DCOMDARE_HOST_RUNS_AVX512F=0
//              COMDARE_HOST_RUNS_AVX2:UNINITIALIZED=0        <- neu geschrieben, von aussen
//              COMDARE_HOST_RUNS_AVX2_COMPILED:INTERNAL=TRUE <- UEBERLEBT
//              COMDARE_HOST_RUNS_AVX2_EXITCODE:INTERNAL=0    <- UEBERLEBT
//
// CMake entfernt die _COMPILED-Zeile NIEMALS. Wer nur ihre ANWESENHEIT prueft, nimmt eine
// ERZWUNGENE Klasse als gemessene und vergleicht gegen die NIEDRIGSTE Untergrenze. Damit
// darf der Baum beliebig viele Tests verlieren, ohne dass irgendetwas klappert.
//
// DAS UNTERSCHEIDENDE MERKMAL IST DER TYP DER ZEILE, nicht die Anwesenheit von _COMPILED:
// check_cxx_source_runs schreibt 'CACHE INTERNAL' (Modules/Internal/CheckSourceRuns.cmake:113
// und :121). Jede Form von aussen schreibt einen anderen Typ -- gemessen: '-DVAR=0' gibt
// UNINITIALIZED, '-DVAR:BOOL=0' gibt BOOL.
//
// DIESER FALL BAUT DEN SCHADEN NACH, in klein: der Baum haelt die basis-Zeile, aber NICHT die
// avx512f-Zeile, die ihm nach seinem ehrlichen Cache zustuende. Vor der Heilung endete er mit
// 0 statt 2 -- der Nenner-Befund verschwand lautlos.
TEST(D2AbdeckungsWacheNenner, ErzwungeneKlasseUeberlebtDieCompiledZeile) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_roh_schreiben("COMDARE_HOST_RUNS_AVX2:UNINITIALIZED=0\n"
                                  "COMDARE_HOST_RUNS_AVX512F:UNINITIALIZED=0\n"
                                  "COMDARE_HOST_RUNS_AVX2_COMPILED:INTERNAL=TRUE\n"
                                  "COMDARE_HOST_RUNS_AVX2_EXITCODE:INTERNAL=0\n"
                                  "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=TRUE\n"
                                  "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=0\n");
    baum.inventur_schreiben({grundstock(marke), "test_zweiter_" + marke});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    // 2 Tests im Baum: die basis-Zeile haelt (2 >= 2), die avx512f-Zeile nicht (2 < 4).
    baum.floor_schreiben(4, 3, 2);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("erzwungene Klasse, _COMPILED ueberlebt", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << "Eine ERZWUNGENE Klasse ist keine gemessene -- das ist Abbruch.\n" << lauf.ausgabe;
    // Der Typ gehoert in die Meldung: sonst weiss der Mensch nicht, WORAN es lag.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "UNINITIALIZED")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "HOST-KLASSE (gemessen"))
        << "Eine erzwungene Klasse als 'gemessen' auszugeben ist "
           "genau der Defekt.\n"
        << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// -- DAS DRITTE BEIN: derselbe Baum, ehrlicher avx512f-Cache -> NENNER-BEFUND ---------------
// Hier wird der Schaden sichtbar, den der Fall darueber anrichtet. Die Bytes des Baums sind
// bis auf die zwei Wertzeilen des Caches IDENTISCH -- Inventur, Protokoll, Untergrenze. Der
// ehrlich gemessene Baum reisst seine Untergrenze (2 < 4) und faellt mit 4. Derselbe Baum mit
// erzwungener Klasse fiel vor der Heilung auf die basis-Zeile (2 >= 2) und meldete den Befund
// NICHT. Genau so verschwinden im echten Repo sechs Tests lautlos.
TEST(D2AbdeckungsWacheNenner, EhrlicherAvx512fCacheMitDenselbenBytesIstNennerBefund) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_schreiben("1", "1"); // ehrlich gemessen: die Probe lief und sagte 'ja'
    baum.inventur_schreiben({grundstock(marke), "test_zweiter_" + marke});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(4, 3, 2); // dieselben drei Zahlen wie in den Nachbarfaellen

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("ehrlicher avx512f-Cache, gleiche Bytes", lauf, marke);

    EXPECT_EQ(lauf.code, 4) << "2 Tests gegen die avx512f-Untergrenze 4 ist ein NENNER-Befund.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "UNTERSCHRITTEN")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "Klasse avx512f")) << lauf.ausgabe;
}

// -- T-4 GEGENEINGANG: derselbe Baum, dieselbe Untergrenze, NUR der Cache ehrlich -----------
// Die Faelle unterscheiden sich in GENAU den zwei Wertzeilen des Caches. Ohne diesen Fall
// waere die Zusicherung oben auch von einer Wache erfuellt, die jeden basis-Baum ablehnt --
// und die waere unbrauchbar, denn es gibt echte basis-Maschinen.
// EXIT 0 ist hier NICHT zu haben und waere ein falscher Anspruch: der praeparierte Baum hat
// zwei Tests, das echte CI-Manifest nennt Hunderte, also findet die Wache zu Recht tote
// Namen (Exit 1). Gemessen wird deshalb, was dieser Fall wirklich behauptet: KEIN Abbruch
// (2) und KEIN Nenner-Befund (4).
TEST(D2AbdeckungsWacheNenner, EhrlicherBasisCacheMitDenselbenBytesTraegtDieBasisZeile) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_schreiben("", ""); // ehrlich gemessen: die Probe lief und sagte 'nein'
    baum.inventur_schreiben({grundstock(marke), "test_zweiter_" + marke});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(4, 3, 2); // dieselben drei Zahlen wie in den Nachbarfaellen

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("ehrlicher basis-Cache, gleiche Bytes", lauf, marke);

    EXPECT_NE(lauf.code, 2) << "Ein ehrlich gemessener basis-Baum ist bestimmbar, nicht unbekannt.\n" << lauf.ausgabe;
    EXPECT_NE(lauf.code, 4) << "Auf seiner eigenen Zeile haelt dieser Baum.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "HOST-KLASSE (gemessen")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "basis")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "UNTERSCHRITTEN")) << lauf.ausgabe;
}

// -- Der TYP ist das Merkmal, nicht das Wort 'UNINITIALIZED' --------------------------------
// AM OBJEKT GEMESSEN: 'cmake -B b -DCOMDARE_HOST_RUNS_AVX2:BOOL=0' auf den konfigurierten
// Baum hinterlaesst 'COMDARE_HOST_RUNS_AVX2:BOOL=0' -- _COMPILED und _EXITCODE ueberleben
// genauso. Eine Wache, die bloss die Zeichenkette 'UNINITIALIZED' auf eine schwarze Liste
// setzt, besteht den Fall darueber und faellt hier. Deshalb steht er da.
TEST(D2AbdeckungsWacheNenner, ErzwungeneKlasseAlsBoolIstEbensoAbbruch) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_roh_schreiben("COMDARE_HOST_RUNS_AVX2:BOOL=0\n"
                                  "COMDARE_HOST_RUNS_AVX2_COMPILED:INTERNAL=TRUE\n"
                                  "COMDARE_HOST_RUNS_AVX2_EXITCODE:INTERNAL=0\n"
                                  "COMDARE_HOST_RUNS_AVX512F:BOOL=0\n"
                                  "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=TRUE\n"
                                  "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=0\n");
    baum.inventur_schreiben({grundstock(marke), "test_zweiter_" + marke});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(4, 3, 2);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("erzwungene Klasse als BOOL", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "BOOL")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "HOST-KLASSE (gemessen")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// -- V-8 AM GEGENSTAND: der Typ allein reicht NICHT -----------------------------------------
// "Welcher Zustand erzeugt diese Ausgabe, ohne dass die Sache existiert?" -- eine Wache, die
// nur den Typ prueft, wird von EINEM Zusatz auf derselben Kommandozeile besiegt. AM OBJEKT
// GEMESSEN: 'cmake -B b -DCOMDARE_HOST_RUNS_AVX2:INTERNAL=' hinterlaesst
//     COMDARE_HOST_RUNS_AVX2:INTERNAL=            <- Typ INTERNAL, Wert leer: sieht ehrlich aus
//     COMDARE_HOST_RUNS_AVX2_COMPILED:INTERNAL=TRUE
//     COMDARE_HOST_RUNS_AVX2_EXITCODE:INTERNAL=0  <- die Probe LIEF und war ERFOLGREICH
// Der leere Wert behauptet "diese CPU kann kein AVX2", der Exit-Code sagt das Gegenteil. Nur
// die Wertzeile ist gefaelscht, der Beleg daneben nicht -- CMake koppelt beide (:112-121).
TEST(D2AbdeckungsWacheNenner, LeererWertMitErfolgreichemExitCodeIstAbbruch) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_roh_schreiben("COMDARE_HOST_RUNS_AVX2:INTERNAL=\n"
                                  "COMDARE_HOST_RUNS_AVX2_COMPILED:INTERNAL=TRUE\n"
                                  "COMDARE_HOST_RUNS_AVX2_EXITCODE:INTERNAL=0\n"
                                  "COMDARE_HOST_RUNS_AVX512F:INTERNAL=\n"
                                  "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=TRUE\n"
                                  "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=0\n");
    baum.inventur_schreiben({grundstock(marke), "test_zweiter_" + marke});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(4, 3, 2);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("leerer Wert, Exit-Code 0", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << "Wert und Exit-Code widersprechen sich -- das ist kein Messergebnis.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "_EXITCODE")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "HOST-KLASSE (gemessen")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// -- Und die Gegenrichtung: '1' mit einem Exit-Code, der nie erfolgreich war -----------------
// Ohne diesen Fall waere die Zusicherung oben auch von einer Wache erfuellt, die schlicht
// jeden Exit-Code 0 ablehnt -- und die haette keinen einzigen avx512f-Baum mehr durchgelassen.
TEST(D2AbdeckungsWacheNenner, WertEinsMitGescheitertemExitCodeIstAbbruch) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_roh_schreiben("COMDARE_HOST_RUNS_AVX2:INTERNAL=1\n"
                                  "COMDARE_HOST_RUNS_AVX2_COMPILED:INTERNAL=TRUE\n"
                                  "COMDARE_HOST_RUNS_AVX2_EXITCODE:INTERNAL=1\n"
                                  "COMDARE_HOST_RUNS_AVX512F:INTERNAL=1\n"
                                  "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=TRUE\n"
                                  "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=1\n");
    baum.inventur_schreiben({grundstock(marke), "test_zweiter_" + marke});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(4, 3, 2);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("Wert 1, Exit-Code 1", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "_EXITCODE")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// -- Der Wert '0' ist bei Typ INTERNAL kein Messergebnis ------------------------------------
// Der Modul-Quelltext laesst nur zwei Werte zu -- 1 (CheckSourceRuns.cmake:113) und LEER
// (:121). Eine 0 hat also nie eine Probe geschrieben, egal wie ehrlich die Nachbarzeilen
// aussehen.
//
// DIESER FALL TRAEGT SEINEN EXIT-CODE ABSICHTLICH KONSISTENT (1 zu 'kann es nicht'). Die
// erste Fassung setzte ihn auf 0 -- und wurde damit schon von Merkmal (3) erschlagen, also
// von einer Zusicherung, die gar nicht seine ist. Der Wegwerf-Mutant, der bloss den Wert 0
// wieder durchwinkt, UEBERLEBTE ihn. Erst so beisst er auf seine eigene Regel.
// AM OBJEKT GEMESSEN und mit einem realen Befehl erreichbar: auf einem Baum, dessen ehrliche
// Probe mit Exit 1 lief (echte basis-Maschine), hinterlaesst
// 'cmake -B e -DCOMDARE_HOST_RUNS_AVX2:INTERNAL=0' genau
//     COMDARE_HOST_RUNS_AVX2:INTERNAL=0
//     COMDARE_HOST_RUNS_AVX2_COMPILED:INTERNAL=TRUE
//     COMDARE_HOST_RUNS_AVX2_EXITCODE:INTERNAL=1
TEST(D2AbdeckungsWacheNenner, WertNullBeiTypInternalIstAbbruch) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_roh_schreiben("COMDARE_HOST_RUNS_AVX2:INTERNAL=0\n"
                                  "COMDARE_HOST_RUNS_AVX2_COMPILED:INTERNAL=TRUE\n"
                                  "COMDARE_HOST_RUNS_AVX2_EXITCODE:INTERNAL=1\n"
                                  "COMDARE_HOST_RUNS_AVX512F:INTERNAL=0\n"
                                  "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=TRUE\n"
                                  "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=1\n");
    baum.inventur_schreiben({grundstock(marke), "test_zweiter_" + marke});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(4, 3, 2);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("Wert 0 bei Typ INTERNAL", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "HOST-KLASSE (gemessen")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// -- V-8, dieselbe Fehlerklasse: DIE Zeile ist nicht EINE Zeile -----------------------------
// "Welcher Zustand erzeugt diese Ausgabe, ohne dass die Sache existiert?" -- ein Cache mit
// ZWEI Wertzeilen. Die Wache liest die erste (sed '1p'); AM OBJEKT GEMESSEN nimmt CMake beim
// Laden aber die LETZTE: an einen ehrlichen Cache 'COMDARE_HOST_RUNS_AVX2:UNINITIALIZED=0'
// angehaengt, meldete der naechste Konfigurationslauf 'AVX2=0' und schrieb die Datei mit
// genau dieser einen Zeile neu. Die ehrliche Zeile oben haette die Wache also beruhigt,
// waehrend der Bau nach der unteren gefahren ist. CMake selbst legt jeden Eintrag nur einmal
// an -- zwei Zeilen heisst: von Hand bearbeitet, und das ist kein Messergebnis.
TEST(D2AbdeckungsWacheNenner, ZweiWertzeilenImCacheSindAbbruch) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_roh_schreiben("COMDARE_HOST_RUNS_AVX2:INTERNAL=1\n"
                                  "COMDARE_HOST_RUNS_AVX2_COMPILED:INTERNAL=TRUE\n"
                                  "COMDARE_HOST_RUNS_AVX2_EXITCODE:INTERNAL=0\n"
                                  "COMDARE_HOST_RUNS_AVX512F:INTERNAL=1\n"
                                  "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=TRUE\n"
                                  "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=0\n"
                                  "COMDARE_HOST_RUNS_AVX2:UNINITIALIZED=0\n");
    baum.inventur_schreiben({grundstock(marke), "test_zweiter_" + marke});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(4, 3, 2);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("zwei Wertzeilen im Cache", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << "Die erste Zeile zu lesen und die zweite zu uebersehen ist "
                               "derselbe Riss wie der Fund oben.\n"
                            << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "HOST-KLASSE (gemessen")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// -- FAIL-CLOSED: ein unverstandener Wert wird nicht zu 'nein' gerundet ---------------------
TEST(D2AbdeckungsWacheNenner, UnverstandenerKlassenWertIstAbbruch) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    // Der Wert ist die frisch gewuerfelte Marke: garantiert weder 1 noch 0 noch leer.
    baum.cmakecache_roh_schreiben("COMDARE_HOST_RUNS_AVX2:INTERNAL=" + marke +
                                  "\n"
                                  "COMDARE_HOST_RUNS_AVX2_COMPILED:INTERNAL=TRUE\n"
                                  "COMDARE_HOST_RUNS_AVX512F:INTERNAL=1\n"
                                  "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=TRUE\n");
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(1);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("Klassenwert unverstanden", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, marke)) << "Der unbrauchbare Wert gehoert in die Meldung.\n" << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// -- Die Leiter hat drei Sprossen und keine vierte ------------------------------------------
TEST(D2AbdeckungsWacheNenner, Avx512fOhneAvx2WidersprichtDerLeiter) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_schreiben("", "1"); // AVX-512F ohne AVX2 -- gibt es auf keiner Maschine
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(1);

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("AVX-512F ohne AVX2", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "widerspruechlich")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// -- Die ALTE Bauform wird nicht stillschweigend weitergelesen ------------------------------
// Sie ist im Kopf der echten Datei als Historie erhalten -- als WERT ist sie ungueltig,
// denn genau ihr Anspruch ("gilt ueberall") war der Defekt.
TEST(D2AbdeckungsWacheNenner, AlteNackteZahlIstAbbruchMitMigrationsHinweis) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_roh_schreiben("# alte Bauform\n1\n");

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("alte nackte Zahl", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "ohne Host-Klasse")) << lauf.ausgabe;
    // Die Meldung muss sagen, WAS statt dessen zu tun ist -- sonst ist sie nur beunruhigend.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "avx512f")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// -- Eine fehlende Klasse faellt SOFORT auf, nicht erst auf der Maschine, die sie braucht ---
TEST(D2AbdeckungsWacheNenner, FehlendeKlassenZeileIstAbbruchAuchWennDieEigeneDaSteht) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.cmakecache_schreiben("1", "1"); // avx512f -- die eigene Zeile ist vorhanden
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_roh_schreiben("avx512f 1\navx2 1\n"); // 'basis' fehlt

    Lauf const lauf = wache_fahren(baum.pfad());
    lauf_berichten("Klassenzeile fehlt", lauf, marke);

    EXPECT_EQ(lauf.code, 2) << "Eine unvollstaendige Leiter ist keine Leiter.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "basis")) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABDECKUNGS-WACHE: GRUEN")) << lauf.ausgabe;
}

// ===========================================================================================
// DAS UMGEBUNGS-LECK, DAS DIE ce-PIPELINE 15515 ROT GEMACHT HAT (2026-08-10)
// ===========================================================================================

// -- Der Rueckfall-Waechter: eine geerbte COMDARE_PMC_LANES darf nichts mehr aendern --------
// Dieser Fall REPRODUZIERT den CI-Riss lokal. Ohne die Scheuerliste in wache_fahren() faellt
// er hier genauso wie er in Job 371127 auf prod2 gefallen ist -- und zwar in der Zeile, die
// 'UNGEPRUEFT' erwartet. Er ist damit der Koeder, der vor der Heilung BEISST.
TEST(D2AbdeckungsWacheNenner, GeerbtesPmcLanesAendertDasUrteilNicht) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(1);

    // Genau der Zustand, den GitLab herstellt (.gitlab-ci.yml, variables:-Block).
    //
    // ZWEITER, EIGENER KOEDER -- und das ist keine Umstaendlichkeit: 'marke' steckt bereits
    // im Pfad des praeparierten Baums und in seinen Testnamen und taucht deshalb voellig zu
    // Recht in der Ausgabe auf. Ein Koeder, der mit der Identitaet des Baums zusammenfaellt,
    // kann ein Leck nicht von normalem Betrieb unterscheiden -- er ist immer 'gefunden'.
    // 'leck' wird frisch gewuerfelt und kommt sonst NIRGENDS vor: steht er in der Ausgabe,
    // kann er nur aus der geerbten Umgebung stammen.
    std::string const leck = koeder_marke();
    ASSERT_NE(leck, marke) << "Zwei Wuerfe aus /dev/urandom waren gleich -- der Koeder taugt nicht.";

    ::setenv("COMDARE_PMC_LANES", leck.c_str(), 1);
    Lauf const lauf = wache_fahren(baum.pfad());
    ::unsetenv("COMDARE_PMC_LANES");
    lauf_berichten("PMC_LANES geerbt", lauf, marke);
    std::cout << "  [D2] Umgebungs-Koeder (darf NICHT durchschlagen): " << leck << "\n";

    // Die Wache darf die geerbte Variable GAR NICHT gesehen haben.
    EXPECT_FALSE(enthaelt(lauf.ausgabe, leck)) << "Die geerbte Umgebung ist in den Pruefling durchgeschlagen.\n"
                                               << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "UNGEPRUEFT")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "ANNAHME")) << lauf.ausgabe;
    EXPECT_NE(lauf.code, 4) << lauf.ausgabe;
}

// -- T-4 GEGENEINGANG: die Variable BEWUSST gesetzt wirkt weiterhin -------------------------
// Ohne ihn waere die Zusicherung oben auch von einem 'env -i' erfuellt, das jede Absicht
// mitloescht -- dann koennte kein Fall mehr etwas ueber gesetzte Gates aussagen.
TEST(D2AbdeckungsWacheNenner, AusdruecklichGesetztesPmcLanesWirktTrotzScheuerung) {
    std::string const marke = koeder_marke();
    PraeparierterBaum baum{marke};
    baum.inventur_schreiben({grundstock(marke)});
    baum.protokoll_schreiben({"BLOCK|kb_" + marke + "|AKTIV|" + grundstock(marke) + "|praepariert", "ENDE|1"});
    baum.floor_schreiben(1);

    // Geerbt (muss verschwinden) UND ausdruecklich gesetzt (muss ankommen) -- verschiedene
    // Werte, damit die Ausgabe zeigt, welcher von beiden gewonnen hat.
    std::string const geerbt = "geerbt_" + marke;
    ::setenv("COMDARE_PMC_LANES", geerbt.c_str(), 1);
    Lauf const lauf = wache_fahren(baum.pfad(), "COMDARE_PMC_LANES=gesetzt_" + marke + " ");
    ::unsetenv("COMDARE_PMC_LANES");
    lauf_berichten("PMC_LANES ausdruecklich gesetzt", lauf, marke);

    EXPECT_TRUE(enthaelt(lauf.ausgabe, "gesetzt_" + marke)) << "Die ABSICHT ist mitgescheuert worden.\n"
                                                            << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "geerbt_" + marke)) << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "UNGEPRUEFT")) << lauf.ausgabe;
}

// -- T-6: die Scheuerliste bleibt vollstaendig, weil eine PRUEFUNG sie dazu zwingt ----------
// T-3 FREMDER NENNER: die Grundgesamtheit der Gate-Variablen kommt aus dem MANIFEST, nicht
// aus diesem Test. Kommt dort ein 'declared:'- oder 'optin:'-Gate mit einer neuen Variablen
// dazu, faellt dieser Fall -- und niemand muss daran gedacht haben.
TEST(D2AbdeckungsWacheNenner, ScheuerlisteDecktJedeGateVariableDesManifests) {
    fs::path const manifest = fs::path{COMDARE_D2_GUARD_SH}.parent_path() / "ci_test_coverage_manifest.sh";
    std::ifstream  quelle{manifest};
    ASSERT_TRUE(quelle.good()) << "Manifest nicht lesbar: " << manifest;

    std::vector<std::string> gefunden;
    std::string              zeile;
    while (std::getline(quelle, zeile)) {
        if (zeile.rfind("CE_COV_GATE_", 0) != 0) { continue; }
        std::size_t const gleich = zeile.find('=');
        if (gleich == std::string::npos) { continue; }
        std::string wert = zeile.substr(gleich + 1);
        // Anfuehrungszeichen und angehaengte Kommentare weg.
        if (std::size_t const auf = wert.find('"'); auf != std::string::npos) {
            std::size_t const zu = wert.find('"', auf + 1);
            wert = (zu == std::string::npos) ? wert.substr(auf + 1) : wert.substr(auf + 1, zu - auf - 1);
        }
        std::string variable;
        if (wert.rfind("declared:", 0) == 0) {
            variable = wert.substr(std::string_view{"declared:"}.size());
        } else if (wert.rfind("optin:", 0) == 0) {
            variable = wert.substr(std::string_view{"optin:"}.size());
            if (std::size_t const gl = variable.find('='); gl != std::string::npos) {
                variable = variable.substr(0, gl);
            }
        } else {
            continue; // 'always' haengt an keiner Variablen
        }
        gefunden.push_back(variable);
    }

    // T-2 AUSSAGE STATT ANWESENHEIT: eine leere Fundmenge waere kein Freispruch, sondern
    // ein kaputter Parser -- dann sagte dieser Fall gar nichts.
    ASSERT_FALSE(gefunden.empty()) << "Im Manifest wurde KEINE Gate-Variable gefunden. Der Parser dieses Falls "
                                      "ist defekt oder das Manifest hat seine Form geaendert: "
                                   << manifest;
    std::cout << "  [D2] Gate-Variablen im Manifest (fremde Quelle): " << gefunden.size() << "\n";

    for (std::string const& variable : gefunden) {
        bool gescheuert = false;
        for (std::string_view const bekannt : kUmgebungsScheuerliste) {
            if (bekannt == variable) { gescheuert = true; }
        }
        EXPECT_TRUE(gescheuert) << "Das Manifest deklariert das Gate '" << variable
                                << "', aber wache_fahren() scheuert es nicht aus der Umgebung. Genau so hat "
                                   "COMDARE_PMC_LANES am 2026-08-10 die ce-Pipeline 15515 rot gemacht: der "
                                   "Pruefling erbte einen Wert, den kein Fall gesetzt hatte.";
    }
}

#endif // !_WIN32
