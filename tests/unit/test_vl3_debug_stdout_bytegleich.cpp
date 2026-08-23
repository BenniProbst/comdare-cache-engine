// test_vl3_debug_stdout_bytegleich -- B04 (K07/VL-3-Rest): der stdout-BYTEVERGLEICH der Planer-Shell
// mit und ohne --debug, am ECHTEN Kommando (Prozess-Ebene).
//
// WARUM DIESER TEST EXISTIERT: main.cpp sagt es an der Vermerk-Stelle selbst -- "die stdout-Byte-
// Gleichheit mit/ohne --debug ist NICHT gemessen (Audit 17.08., VL-3-Restposten: Bytevergleich =
// offener Pruef-Posten, Welle-2-Fix)". Die PLAN-Ebene ist per s8kopf-#22(ii) gedeckt, die
// PROZESS-Ebene (die Bytes, die ein CI-Konsument wirklich sieht) war es nicht. Owner-Grammatik des
// Flags (Ledger 09.08.): es "beeinflusst zwar die AUSPRAEGUNG der States, aber nicht ihr Verhalten"
// -- und der [debug]-Vermerk geht bewusst auf stderr, "damit stdout emissions-rein bleibt".
//
// WAS GEPRUEFT WIRD (jede Zusage mit Nenner):
//   (0) DETERMINISMUS-ANKER: zwei identische Laeufe OHNE Flag sind stdout-byte-identisch --
//       ohne diesen Anker waere jeder spaetere Vergleich nicht beweisfaehig (Selbstdiagnose).
//   (1) 4 Subkommandos x 2 Laeufe: plan dump | plan ci | plan cmake | validate, je einmal ohne
//       und einmal mit --debug (Freigabe gesetzt): stdout BYTE-identisch, Exit-Codes gleich.
//
// OWNER-AUSNAHME pmc_befund (Praezisierung nach CI 16095, Job 383091): der Plan traegt GENAU EINE
// deklarierte host-LEBENDIGE Zeilenklasse -- `pmc_befund=` im Plan-Text (I-PMC-2, Owner 10.08.2026:
// "erkennt jeder Planer auf jeder Maschine fuer sich"; experiment_plan_director.hpp nennt sie "die
// EINZIGE host-abhaengige Zeile des --dump-plan") und ihre "# PMC-BEFUND ..."-Kommentar-Schwester der
// YAML-/CMake-Emissionen (pmc_befund_zeile, V-1). Ihr WERT ist eine PMU-Momentaufnahme JE PROZESSLAUF
// und unter Runner-Multiplexing lauf-zu-lauf variabel (383091: events=2/4 vs 3/4 zwischen ohne- und
// mit-Lauf; die Klasse ist im Probe-Kern pmc_event_biss.hpp dokumentiert und aelter als der pmcpaket-
// Umzug, CI 16073/382856). Die VL-3-Zusage gilt dem FLAG, nicht der PMU-Last: verglichen wird deshalb
// der STABILE Anteil -- die PMC-Zeilenklasse wird beidseitig strukturerhaltend maskiert, ihre ANZAHL
// (und ueber den Text ihre POSITION) bleibt Pflicht-Nenner, ALLE uebrigen Bytes bleiben streng. Die
// PRODUKT-Emission ist unberuehrt (fail-loud #83: nichts wird versteckt, der Plan traegt weiter den
// Live-Befund). Maskierungs-Drift ist laut: verschwindet das Praefix aus der Emission, reisst die
// Mindest-Anzahl (dump/ci >= 1) den Test.
//   (2) Exit-8-Zweig: --debug OHNE Freigabe -> rc 8 und stdout LEER (die Sperre emittiert kein
//       halbes Byte auf den Datenkanal).
//   (3) Exit-6-Zweig: Bestandslog-Gate unvollstaendig (COMDARE_BESTANDSLOG=true + minio-Ebene
//       konfiguriert + DOC_KEY fehlt) -> plan ci bricht mit rc 6, mit und ohne Flag identisch
//       (stdout-Paar byte-gleich leer).
//
// HERMETIK: Profil reist als argv[2] aus dem Quellbaum (kein COMDARE_THESIS_PROFILE-Einfluss);
// alle Gate-Variablen setzt/entfernt der Test selbst (popen erbt die Umgebung). stdout wird PUR
// gelesen (2>/dev/null) -- stderr DARF sich unterscheiden (der [debug]-Vermerk ist dort zuhause).
//
// T-11c-MUTATIONSANKER: ein zusaetzliches stdout-Byte im --debug-Zweig von main.cpp bricht (1)
// literal (die Maskierung nimmt AUSSCHLIESSLICH die deklarierte PMC-Zeilenklasse aus dem Vergleich).

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <string>

#include "support/pmc_stdout_maskierung.hpp" // PmcMaskiert + pmc_zeilen_maskieren (EINE Quelle, A2.2)

#if defined(_WIN32)
#define COMDARE_POPEN  _popen
#define COMDARE_PCLOSE _pclose
#else
#include <sys/wait.h>
#define COMDARE_POPEN  popen
#define COMDARE_PCLOSE pclose
#endif

namespace {

std::string g_binary; // argv[1]: $<TARGET_FILE:comdare_experiment_planner>
std::string g_profil; // argv[2]: Fixture-Profil aus dem Quellbaum

struct Lauf {
    int         rc = -1; ///< -1 == Prozess lief nicht/starb ohne exit -- immer ROT
    std::string out;     ///< stdout PUR (stderr -> /dev/null; der Datenkanal ist der Gegenstand)
};

/// Faehrt die Binary; Argumente sind fest verdrahtete ASCII-Woerter, nur Pfade sind gequotet.
/// stderr wird verworfen: verglichen wird der DATEN-Kanal (clig.dev-Trennung der Shell).
[[nodiscard]] Lauf fahre_stdout(std::string const& argzeile) {
    Lauf        l{};
    std::string cmd = "\"" + g_binary + "\" " + argzeile + " 2>/dev/null";
    std::FILE*  p   = COMDARE_POPEN(cmd.c_str(), "r");
    if (p == nullptr) return l;
    char puffer[512];
    while (std::fgets(puffer, sizeof(puffer), p) != nullptr) l.out += puffer;
    int const status = COMDARE_PCLOSE(p);
#if defined(_WIN32)
    l.rc = status;
#else
    l.rc = (status >= 0 && WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
#endif
    return l;
}

void env_setzen(char const* name, char const* wert) {
#if defined(_WIN32)
    std::string const zuweisung = std::string{name} + "=" + (wert == nullptr ? "" : wert);
    ::_putenv(zuweisung.c_str());
#else
    if (wert == nullptr)
        ::unsetenv(name);
    else
        ::setenv(name, wert, 1);
#endif
}

/// Hermetische Grundstellung VOR jedem Fall: Freigabe weg, Bestandslog weg, Profil-Env weg.
void grundstellung() {
    env_setzen("COMDARE_DEBUG_FREIGABE", nullptr);
    env_setzen("COMDARE_BESTANDSLOG", nullptr);
    env_setzen("COMDARE_BESTANDSLOG_DOC_KEY", nullptr);
    env_setzen("COMDARE_BESTANDSLOG_OWNER_UUID", nullptr);
    env_setzen("COMDARE_BESTANDSLOG_MASCHINE", nullptr);
    env_setzen("COMDARE_MINIO_ENDPOINT", nullptr);
    env_setzen("COMDARE_MINIO_BUCKET", nullptr);
    env_setzen("COMDARE_THESIS_PROFILE", nullptr);
}

/// Das Byte-Paar eines Subkommandos: einmal ohne, einmal mit --debug (Freigabe aktiv).
/// `mindest_pmc_zeilen` ist die design-zugesicherte Mindest-Anzahl der host-lebendigen PMC-Zeilen
/// (plan dump: I-PMC-2; plan ci: V-1) -- sie macht Maskierungs-Drift laut (s. support-Kopf); fuer
/// Subkommandos ohne solche Zusicherung bleibt sie 0 (V7: nichts assertieren, was das Design nicht gibt).
void pruefe_bytegleich(std::string const& sub, std::size_t mindest_pmc_zeilen = 0) {
    grundstellung();
    Lauf const ohne = fahre_stdout(sub + " \"" + g_profil + "\"");
    env_setzen("COMDARE_DEBUG_FREIGABE", "true");
    Lauf const mit = fahre_stdout(sub + " --debug \"" + g_profil + "\"");
    grundstellung();
    EXPECT_EQ(ohne.rc, mit.rc) << sub << ": Exit-Codes muessen mit/ohne --debug identisch sein";
    // Owner-Ausnahme pmc_befund (s. Kopf): der Vergleich pinnt auf den STABILEN Anteil; die
    // PMC-Zeilenklasse wird beidseitig strukturerhaltend maskiert, Anzahl+Position bleiben Nenner.
    comdare::test::PmcMaskiert const o = comdare::test::pmc_zeilen_maskieren(ohne.out);
    comdare::test::PmcMaskiert const m = comdare::test::pmc_zeilen_maskieren(mit.out);
    EXPECT_EQ(o.pmc_zeilen, m.pmc_zeilen)
        << sub << ": die PMC-Zeilenklasse muss beidseitig gleich oft auftreten (Struktur-Nenner)";
    EXPECT_GE(o.pmc_zeilen, mindest_pmc_zeilen)
        << sub << ": design-zugesicherte PMC-Zeile fehlt -- Emission oder Maskierungs-Praefix gedriftet";
    EXPECT_EQ(o.text, m.text) << sub
                              << ": stdout muss ausserhalb der host-lebendigen PMC-Zeilenklasse BYTE-identisch sein "
                                 "(Nenner: "
                              << ohne.out.size() << " vs " << mit.out.size() << " Bytes roh, " << o.pmc_zeilen
                              << " maskierte PMC-Zeile(n))";
    EXPECT_FALSE(ohne.out.empty()) << sub << ": der Vergleich zweier leerer Ausgaben bewiese nichts";
}

TEST(Vl3DebugStdoutByteGleich, DeterminismusAnkerZweiGleicheLaeufe) {
    // (0): ohne diesen Anker ist jeder mit/ohne-Vergleich nicht beweisfaehig. Der Anker traegt
    // DIESELBE Owner-Ausnahme wie (1): auch zwei flaglose Laeufe sind zwei PMU-Momentaufnahmen
    // (CI 16095/383091 -- events=2/4 vs 3/4 ist Prozess-zu-Prozess-Varianz, kein Flag-Einfluss).
    grundstellung();
    Lauf const a = fahre_stdout("plan dump \"" + g_profil + "\"");
    Lauf const b = fahre_stdout("plan dump \"" + g_profil + "\"");
    ASSERT_EQ(a.rc, b.rc);
    comdare::test::PmcMaskiert const na = comdare::test::pmc_zeilen_maskieren(a.out);
    comdare::test::PmcMaskiert const nb = comdare::test::pmc_zeilen_maskieren(b.out);
    ASSERT_EQ(na.pmc_zeilen, nb.pmc_zeilen) << "PMC-Zeilenklasse muss in beiden Laeufen gleich oft auftreten";
    ASSERT_GE(na.pmc_zeilen, 1u) << "plan dump traegt die pmc_befund-Zeile (I-PMC-2) -- Maskierung ohne Gegenstand";
    ASSERT_EQ(na.text, nb.text)
        << "plan dump ist ausserhalb der PMC-Zeilenklasse nicht run-to-run-deterministisch -- Bytevergleich unmoeglich";
    ASSERT_FALSE(a.out.empty());
}

TEST(Vl3DebugStdoutByteGleich, PlanDumpBytegleich) { pruefe_bytegleich("plan dump", 1u); }
TEST(Vl3DebugStdoutByteGleich, PlanCiBytegleich) { pruefe_bytegleich("plan ci", 1u); }
TEST(Vl3DebugStdoutByteGleich, PlanCmakeBytegleich) { pruefe_bytegleich("plan cmake"); }
TEST(Vl3DebugStdoutByteGleich, ValidateBytegleich) { pruefe_bytegleich("validate"); }

TEST(Vl3DebugStdoutByteGleich, ExitAchtSperreLaesstStdoutLeer) {
    // (2): Sperre ohne Freigabe -- rc 8, und der Datenkanal traegt KEIN Byte.
    grundstellung();
    Lauf const l = fahre_stdout("plan dump --debug \"" + g_profil + "\"");
    EXPECT_EQ(l.rc, 8) << "ohne COMDARE_DEBUG_FREIGABE=true muss die Zulassung sperren";
    EXPECT_TRUE(l.out.empty()) << "die Sperre darf kein halbes Emissions-Byte auf stdout lassen";
}

TEST(Vl3DebugStdoutByteGleich, ExitSechsGateAbbruchIdentischMitUndOhneFlag) {
    // (3): Bestandslog-Gate unvollstaendig -> rc 6; das Flag aendert daran nichts (Byte-Paar leer==leer).
    grundstellung();
    env_setzen("COMDARE_BESTANDSLOG", "true");
    env_setzen("COMDARE_MINIO_ENDPOINT", "testdummy-alias");
    env_setzen("COMDARE_MINIO_BUCKET", "testdummy-bucket");
    // DOC_KEY/OWNER_UUID/MASCHINE bleiben ungesetzt -> fehlerklasse=konfiguration_unvollstaendig.
    Lauf const ohne = fahre_stdout("plan ci \"" + g_profil + "\"");
    env_setzen("COMDARE_DEBUG_FREIGABE", "true");
    Lauf const mit = fahre_stdout("plan ci --debug \"" + g_profil + "\"");
    grundstellung();
    EXPECT_EQ(ohne.rc, 6) << "unvollstaendige Bestandslog-Konfig muss mit Exit 6 abbrechen";
    EXPECT_EQ(mit.rc, 6) << "--debug darf den Gate-Abbruch nicht veraendern";
    EXPECT_EQ(ohne.out, mit.out) << "Gate-Abbruch: stdout-Paar muss byte-identisch sein";
    EXPECT_TRUE(ohne.out.empty()) << "der Gate-Abbruch emittiert nicht auf den Datenkanal";
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 3) {
        std::fprintf(stderr, "Aufruf: %s <planner-binary> <fixture-profil>\n", argv[0]);
        return 2; // Werkzeug-Fehler, nicht Abdeckungs-Rot
    }
    g_binary = argv[1];
    g_profil = argv[2];
    return RUN_ALL_TESTS();
}
