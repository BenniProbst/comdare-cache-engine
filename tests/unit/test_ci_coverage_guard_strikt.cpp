// test_ci_coverage_guard_strikt -- B13 (Z07): die STRIKT-Mechanik der Abdeckungs-Wache als
// Prozessprobe -- Exit-2-Klasse EXKLUSIV dem Streng-Befund.
//
// H2-BEFUND (BAULISTE): Lande-Laeufe fuhren WEICH -- ein declared:-Gate ohne gesetzte
// Deklarationsvariable wurde als ANNAHME gezaehlt statt als Fehler. Der Mini-Diff in
// .gitlab-ci.yml setzt COMDARE_WACHE_STRIKT=1 am Wache-Aufruf; DIESE Probe pinnt die Mechanik,
// an der der Diff haengt, am echten Skript (kein Nachbau der Shell-Logik).
//
// ERHEBUNGS-MATRIX (am Objekt 2026-08-21, Teil der B13-Abnahme): 14 Manifest-Jobs; GENAU EIN
// declared:-Gate (pmc -> COMDARE_PMC_LANES), global in .gitlab-ci.yml:66 deklariert ("amd
// intel"); arm64_smoke ist optin:-Klasse; 12x always. Fehlende Deklarations-Variablen je Job: 0.
//
// BAUM-ZUSTANDS-UNABHAENGIG (bewusst differenziell): auf einem J-0b-Baum (PRUEFLINGE leer) endet
// die Wache mit NENNER-ROT (rc 4, Block pruefling_slots_v1 UEBERSPRUNGEN -- am Objekt gemessen).
// Ein rc==0-Pin waere hier ein Pin auf den Baum-Zustand, nicht auf die STRIKT-Mechanik. Die
// Probe prueft deshalb die KLASSEN-Exklusivitaet: rc 2 gehoert GENAU dem Streng-Befund.
//
// T-11c-MUTATIONSANKER: den Streng-Zweig im Guard lahmlegen (CE_STRIKT nie als '1' erkennen)
// laesst Fall (1) mit rc!=2 enden -- die Probe bricht literal.

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <string>

#if !defined(_WIN32)
#include <sys/wait.h>
#endif

namespace {

std::string g_skript;    // argv[1]: scripts/ci_test_coverage_guard.sh
std::string g_build_dir; // argv[2]: konfigurierter Bau-Baum (ctest -N faehig)

struct Lauf {
    int         rc = -1;
    std::string ausgabe;
};

[[nodiscard]] Lauf fahre(std::string const& env_praefix) {
    Lauf        l{};
    std::string cmd = "env -u COMDARE_WACHE_STRIKT -u COMDARE_PMC_LANES " + env_praefix + " sh \"" + g_skript +
                      "\" \"" + g_build_dir + "\" 2>&1";
    std::FILE*  p   = ::popen(cmd.c_str(), "r");
    if (p == nullptr) return l;
    char puffer[1024];
    while (std::fgets(puffer, sizeof(puffer), p) != nullptr) l.ausgabe += puffer;
    int const status = ::pclose(p);
    l.rc             = (status >= 0 && WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
    return l;
}

TEST(CiCoverageGuardStrikt, StriktOhneVariableIstExitZweiMitBenanntemGate) {
    Lauf const l = fahre("COMDARE_WACHE_STRIKT=1");
    EXPECT_EQ(l.rc, 2) << "STRIKT + ungesetzte Deklarationsvariable MUSS Exit 2 sein\n"
                       << l.ausgabe.substr(l.ausgabe.size() > 800 ? l.ausgabe.size() - 800 : 0);
    EXPECT_NE(l.ausgabe.find("Streng-Modus"), std::string::npos) << "der Befund muss sich selbst benennen";
    EXPECT_NE(l.ausgabe.find("COMDARE_PMC_LANES"), std::string::npos) << "und die fehlende Variable beim Namen nennen";
}

TEST(CiCoverageGuardStrikt, StriktMitVariableIstNichtDieStrengKlasse) {
    Lauf const l = fahre("COMDARE_WACHE_STRIKT=1 COMDARE_PMC_LANES=amd");
    EXPECT_NE(l.rc, 2) << "mit gesetzter Variable darf die Streng-Klasse nicht feuern\n" << l.ausgabe;
    EXPECT_EQ(l.ausgabe.find("Streng-Modus: "), std::string::npos)
        << "keine Streng-Abbruchmeldung bei vollstaendiger Deklaration";
    EXPECT_NE(l.ausgabe.find("gegen eine gesetzte Variable GEPRUEFT"), std::string::npos)
        << "die Bilanz muss die geprueften Gates ausweisen";
}

TEST(CiCoverageGuardStrikt, WeichOhneVariableIstAnnahmeStattAbbruch) {
    Lauf const l = fahre("");
    EXPECT_NE(l.rc, 2) << "WEICH darf nie mit der Streng-Klasse enden\n" << l.ausgabe;
    EXPECT_NE(l.ausgabe.find("ANNAHME deklariert"), std::string::npos)
        << "der weiche Modus muss die Annahme AUSWEISEN (gezaehlt, nicht verschwiegen)";
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 3) {
        std::fprintf(stderr, "Aufruf: %s <guard-skript> <build-dir>\n", argv[0]);
        return 2;
    }
    g_skript    = argv[1];
    g_build_dir = argv[2];
    return RUN_ALL_TESTS();
}
