// test_mt_l4_registrierungs_wache_isa -- MT-L4/W0a (2026-08-10): DER SELBSTTEST DER
// ZWEITEN GEGENSTANDSART. Die Test-Registrierungs-Wache haelt jede getrackte
// Test-Quelldatei gegen den Bauweg und verlangt fuer jede Abwesenheit eine BEGRUENDUNG,
// deren Gegenstand sie NACHPRUEFT. Bis zum 10.08.2026 kannte Feld 2 der Allowlist genau
// eine Art von Gegenstand: einen PFAD, geprueft mit '[ -e ]'.
//
// DER STRUKTURMANGEL, am Objekt (ce-Pipeline 15517, Job test:coverage-guard, ROT):
//   tests/unit/CMakeLists.txt:4174 legt add_executable(test_ap5_simd_extension_coherence)
//   INNERHALB von 'if(COMDARE_HOST_RUNS_AVX2 AND COMDARE_HOST_RUNS_AVX512F)' (:4165) an.
//   Auf einem Host ohne AVX-512 entsteht das Ziel nie und die Quelldatei wird nie
//   uebersetzt. Es gibt keine Datei, deren Abwesenheit man dafuer nennen koennte -- die
//   Ausnahme haengt an einer HOST-EIGENSCHAFT. Die Wache hat richtig gemeldet; das FORMAT
//   konnte die Begruendung nicht ausdruecken.
//
// SEITHER ZWEI ARTEN, und das Praefix benennt sie:
//   datei:<pfad>              Ausnahme traegt, solange der Pfad NICHT existiert.
//   isa:<merkmal>[+<merkmal>] Ausnahme traegt, solange MINDESTENS EIN Merkmal auf dem
//                             Bau-Host fehlt (Negation des CMake-UND-Gatters).
// Alles andere -- unbekannte Art, fehlendes Praefix, leeres Feld 2, unbekanntes Merkmal --
// ist UNPRUEFBAR und damit ROT.
//
// WARUM DIESER TEST EXISTIERT: der ganze Wert einer Allowlist liegt darin, dass eine
// Ausnahme VON SELBST rot wird, wenn ihr Grund wegfaellt. Fuer 'isa:' heisst das: laeuft
// die Wache auf einem Bau-Baum, DER die Faehigkeit hatte, und die Datei fehlt trotzdem im
// Bauweg, dann ist die Ausnahme ERLOSCHEN. Eine Erloschen-Pruefung, die nie nachgefahren
// wird, ist eine Behauptung -- und eine Allowlist mit einer hohlen Erloschen-Pruefung ist
// schlimmer als gar keine: sie sieht geprueft aus.
//
// WARUM EIN GOOGLE TEST UND KEINE WEITERE SHELL-PROBE (Owner-Entscheid 2026-08-09):
//   "Es waere sauberer im cmake-Debug Modus standard google Tests zu fahren und diese in
//    Release zu wiederholen aufgrund von compile regressionen. Skripte sagen gar nichts."
//   Die Wache bleibt ein sh-Skript (ihr Umbau nach C++ ist ein eigenes Paket); ihre
//   PRUEFLOGIK faehrt ab hier als ctest-Ziel mit, in Debug wie in Release, ohne eigenen
//   CI-Job und ohne dass die Shell-Menge waechst.
//
// K13 -- DER KOEDER MUSS BEISSEN, UND DER GEGENKOEDER MUSS GRUEN BLEIBEN:
//   * Jeder Fall wuerfelt seinen Dateinamen FRISCH aus /dev/urandom. Taucht er in der
//     Ausgabe der Wache auf, kann er nur aus dem Baum stammen, den dieser Fall gerade
//     gebaut hat -- nicht aus einer Doku und nicht aus dem echten Repo.
//   * Zu jeder Zusicherung ein Gegeneingang (T-4): dieselbe Allowlist-Zeile gegen einen
//     Cache, der das Merkmal HAT (ROT), und gegen einen, der es NICHT hat (GRUEN). Eine
//     Wache, die immer rot ist, ist so wertlos wie eine, die nie rot wird.
//   * Geprueft wird nicht "irgendwie rot", sondern der EXIT-CODE der jeweiligen Klasse:
//     1 = Befund, 2 = die Wache konnte nicht pruefen (fail-closed). Die zwei sind
//     verschiedene Aussagen und werden nicht zusammengeworfen.
//
// DER PRUEFLING IST DIE ECHTE WACHE, in einen synthetischen git-Baum kopiert. Der Baum
// ist noetig, weil die Wache ihren SOLL aus 'git ls-files' zieht: gegen das echte Repo
// gefahren waere jeder Fall eine Aussage ueber 459 fremde Dateien statt ueber den Koeder.
//
// GRENZE, EHRLICH BENANNT -- was dieser Test NICHT deckt:
//   * Er prueft das URTEIL der Wache ueber praeparierte Baeume und die WOHLGEFORMTHEIT
//     der echten Allowlist. Er faehrt die Wache NICHT gegen den echten Bau-Baum -- das
//     ist die Aufgabe des CI-Jobs test:coverage-guard, und ein zweiter Aufruf hier
//     wuerde nur dessen Urteil verdoppeln (und bei einem Ein-Pass-Baum aus einem fremden
//     Grund rot werden).
//   * Die ISA-Antwort kommt aus dem CMakeCache des GEMESSENEN Baums -- also aus dem, was
//     der Bau vorgefunden hat, nicht aus /proc/cpuinfo der laufenden Maschine. Dieselbe
//     Quelle und dasselbe Vokabular (avx2/avx512f) benutzt scripts/ci_test_coverage_guard.sh
//     fuer die Host-Klasse (D2-G5). Dass beide Skripte diese Lesung getrennt fuehren, ist
//     eine benannte Doppelung und ein eigenes Paket, kein Versehen.
//
// ASCII-only (Leitplanke).

#include "comdare_test_tmp.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)

TEST(MtL4RegistrierungsWacheIsa, NurPosix) {
    GTEST_SKIP() << "scripts/ci_test_registrierungs_wache.sh ist ein POSIX-sh-Skript "
                    "(kein Windows-Job faehrt es).";
}

#else

#include <sys/wait.h>

namespace fs = std::filesystem;

namespace {

// ---------------------------------------------------------------------------
// FRISCH GEWUERFELT (K13): /dev/urandom, nicht std::random_device und erst recht keine
// Konstante aus einer Doku. Der Name darf in keiner Datei dieses Repos vorkommen.
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

[[nodiscard]] Lauf schale(std::string const& befehl) {
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

[[nodiscard]] std::string datei_lesen(fs::path const& pfad) {
    std::ifstream     ein{pfad};
    std::stringstream puffer;
    puffer << ein.rdbuf();
    return puffer.str();
}

// ---------------------------------------------------------------------------
// Ein synthetischer git-Baum mit der ECHTEN Wache darin.
//
// WARUM SYNTHETISCH: die Wache zieht ihren SOLL aus 'git ls-files'. Im echten Repo
// waere jeder Fall eine Aussage ueber 459 fremde Dateien; hier sind es genau zwei --
// die Messgeraet-Gegenprobe, die die Wache selbst verlangt (test_pressure_state.cpp),
// und der gewuerfelte Koeder. Nur der Koeder fehlt im Bauweg.
// ---------------------------------------------------------------------------
class SynthBaum {
public:
    explicit SynthBaum(std::string const& marke)
        : wurzel_{comdare::test::user_tmp_dir() / ("mtl4_isa_" + marke)},
          koeder_{"tests/unit/test_koeder_" + marke + ".cpp"} {
        std::error_code ec;
        fs::remove_all(wurzel_, ec);
        fs::create_directories(wurzel_ / "scripts", ec);
        fs::create_directories(wurzel_ / "tests" / "unit", ec);
        fs::create_directories(wurzel_ / "baum", ec);

        fs::copy_file(fs::path{COMDARE_MTL4_WACHE_SH}, wurzel_ / "scripts" / "ci_test_registrierungs_wache.sh",
                      fs::copy_options::overwrite_existing, ec);

        for (auto const& rel : {std::string{"tests/unit/test_pressure_state.cpp"}, koeder_}) {
            std::ofstream aus{wurzel_ / rel};
            aus << "int main() { return 0; }\n";
        }

        // IST-Quelle: nur die Gegenprobe steht im Bauweg, der Koeder NICHT. Genau das
        // ist der Zustand, den ein uebersprungenes add_executable() erzeugt.
        {
            std::ofstream aus{wurzel_ / "baum" / "build.ninja"};
            aus << "build /x/tests/unit/test_pressure_state.cpp.o: CXX "
                << "/x/tests/unit/test_pressure_state.cpp\n";
        }

        (void)schale("git -C \"" + wurzel_.string() + "\" init -q 2>&1");
        (void)schale("git -C \"" + wurzel_.string() + "\" add -- tests/unit/test_pressure_state.cpp \"" + koeder_ +
                     "\" 2>&1");
    }

    SynthBaum(SynthBaum const&)            = delete;
    SynthBaum& operator=(SynthBaum const&) = delete;
    SynthBaum(SynthBaum&&)                 = delete;
    SynthBaum& operator=(SynthBaum&&)      = delete;

    ~SynthBaum() {
        std::error_code ec;
        fs::remove_all(wurzel_, ec);
    }

    /// Ein EHRLICHER Cache, wie ihn check_cxx_source_runs schreibt: Typ INTERNAL,
    /// _COMPILED=TRUE, Wert 1 bei Exit 0 und LEER sonst
    /// (Modules/Internal/CheckSourceRuns.cmake:112-121).
    void cache_ehrlich(bool avx2, bool avx512f) const {
        std::ofstream aus{wurzel_ / "baum" / "CMakeCache.txt"};
        auto const    zeile = [&aus](char const* name, bool da) {
            aus << name << ":INTERNAL=" << (da ? "1" : "") << "\n";
            aus << name << "_COMPILED:INTERNAL=TRUE\n";
            aus << name << "_EXITCODE:INTERNAL=" << (da ? "0" : "1") << "\n";
        };
        zeile("COMDARE_HOST_RUNS_AVX2", avx2);
        zeile("COMDARE_HOST_RUNS_AVX512F", avx512f);
    }

    void cache_roh(std::string const& inhalt) const {
        std::ofstream aus{wurzel_ / "baum" / "CMakeCache.txt"};
        aus << inhalt;
    }

    void cache_entfernen() const {
        std::error_code ec;
        fs::remove(wurzel_ / "baum" / "CMakeCache.txt", ec);
    }

    /// Genau EINE Allowlist-Zeile fuer den Koeder; 'feld2' ist der Prueflig.
    void allowlist(std::string const& feld2) const {
        std::ofstream aus{wurzel_ / "scripts" / "ci_test_registrierungs_allowlist.txt"};
        aus << "# gewuerfelt von test_mt_l4_registrierungs_wache_isa\n";
        aus << koeder_ << " | " << feld2 << " | Begruendungstext\n";
    }

    void allowlist_leer() const {
        std::ofstream aus{wurzel_ / "scripts" / "ci_test_registrierungs_allowlist.txt"};
        aus << "# absichtlich ohne Zeile fuer den Koeder\n";
    }

    [[nodiscard]] Lauf fahren() const {
        return schale("cd \"" + wurzel_.string() + "\" && sh scripts/ci_test_registrierungs_wache.sh baum 2>&1");
    }

    [[nodiscard]] std::string const& koeder() const { return koeder_; }

private:
    fs::path    wurzel_;
    std::string koeder_;
};

void berichten(char const* fall, Lauf const& lauf, SynthBaum const& baum) {
    std::cout << "  [MT-L4] Fall '" << fall << "' | Koeder " << baum.koeder() << " | Prueflig " << COMDARE_MTL4_WACHE_SH
              << " | Exit " << lauf.code << "\n";
}

// ---------------------------------------------------------------------------
// Die echte Allowlist zeilenweise, Kommentare und Leerzeilen heraus.
// ---------------------------------------------------------------------------
[[nodiscard]] std::vector<std::string> allowlist_zeilen() {
    std::vector<std::string> zeilen;
    std::ifstream            ein{fs::path{COMDARE_MTL4_ALLOWLIST}};
    std::string              z;
    while (std::getline(ein, z)) {
        if (z.empty() || z.front() == '#') { continue; }
        if (z.find_first_not_of(" \t") == std::string::npos) { continue; }
        zeilen.push_back(z);
    }
    return zeilen;
}

[[nodiscard]] std::vector<std::string> zerlegen(std::string const& zeile, char trenner) {
    std::vector<std::string> teile;
    std::string              aktuell;
    for (char const c : zeile) {
        if (c == trenner) {
            teile.push_back(aktuell);
            aktuell.clear();
        } else {
            aktuell.push_back(c);
        }
    }
    teile.push_back(aktuell);
    for (auto& t : teile) {
        auto const von = t.find_first_not_of(" \t");
        auto const bis = t.find_last_not_of(" \t");
        t              = (von == std::string::npos) ? std::string{} : t.substr(von, bis - von + 1);
    }
    return teile;
}

} // namespace

// ===========================================================================================
// (1) DIE ZWEITE ART, BEIDE RICHTUNGEN. Dieselbe Allowlist-Zeile, zwei Bau-Baeume.
//     Das ist der Kern: die Ausnahme muss VON SELBST erloeschen, wenn ihr Grund wegfaellt.
// ===========================================================================================
TEST(MtL4RegistrierungsWacheIsa, IsaAusnahmeTraegtOhneDasMerkmalUndErlischtMitIhm) {
    std::string const marke = koeder_marke();
    SynthBaum         baum{marke};
    baum.allowlist("isa:avx2+avx512f");

    // GEGENEINGANG (T-4): Bau-Host OHNE AVX-512F -- das UND-Gatter war falsch, die Datei
    // ist legitim nicht uebersetzt worden. Die Wache MUSS gruen sein.
    baum.cache_ehrlich(/*avx2=*/true, /*avx512f=*/false);
    Lauf const ohne = baum.fahren();
    berichten("isa: Merkmal FEHLT -> GRUEN", ohne, baum);
    EXPECT_EQ(ohne.code, 0) << "Die Ausnahme muesste tragen. Ausgabe:\n" << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, "ABWESEND, ABER BEGRUENDET")) << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, "avx2=ja avx512f=nein")) << "Die Wache muss die ISA-Antwort BENENNEN, "
                                                                   "nicht nur verwenden. Ausgabe:\n"
                                                                << ohne.ausgabe;

    // DER BISS: derselbe Baum, aber der Bau-Host HATTE beide Merkmale. Dann waere das
    // Gatter wahr gewesen und die Datei haette uebersetzt werden muessen -- die
    // Begruendung ist ERLOSCHEN, ohne dass jemand die Allowlist angefasst hat.
    baum.cache_ehrlich(/*avx2=*/true, /*avx512f=*/true);
    Lauf const mit = baum.fahren();
    berichten("isa: alle Merkmale DA -> ROT (erloschen)", mit, baum);
    EXPECT_EQ(mit.code, 1) << "Erwartet ist der Befund-Code 1. Ausgabe:\n" << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, "AUSNAHME ERLOSCHEN")) << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, baum.koeder())) << "Der Befund muss den Koeder NAMENTLICH nennen. Ausgabe:\n"
                                                      << mit.ausgabe;
    // V-1: EIN Befund ist EIN Befund. Eine mehrzeilige Meldung machte in der ersten
    // Fassung aus einer Datei zwei -- am Objekt gemessen 2026-08-10.
    EXPECT_TRUE(enthaelt(mit.ausgabe, "1 mit ERLOSCHENER")) << "Der Zaehler muss EINS sein. Ausgabe:\n" << mit.ausgabe;
}

// ===========================================================================================
// (2) DIE UND-SEMANTIK. 'isa:a+b' bildet 'if(a AND b)' ab: legitim abwesend ist die Datei
//     genau dann, wenn MINDESTENS EINES fehlt. Ein 'oder' waere hier der stille Fehler --
//     es liesse die Ausnahme auf einem Voll-AVX-512-Host weiterleben.
// ===========================================================================================
TEST(MtL4RegistrierungsWacheIsa, UndVerknuepfungEinFehlendesMerkmalGenuegt) {
    std::string const marke = koeder_marke();
    SynthBaum         baum{marke};
    baum.allowlist("isa:avx2+avx512f");

    baum.cache_ehrlich(/*avx2=*/false, /*avx512f=*/false);
    Lauf const keins = baum.fahren();
    berichten("isa: KEIN Merkmal da -> GRUEN", keins, baum);
    EXPECT_EQ(keins.code, 0) << keins.ausgabe;
    EXPECT_TRUE(enthaelt(keins.ausgabe, "avx2=nein avx512f=nein")) << keins.ausgabe;

    baum.cache_ehrlich(/*avx2=*/true, /*avx512f=*/false);
    Lauf const eins = baum.fahren();
    berichten("isa: genau EINS fehlt -> GRUEN", eins, baum);
    EXPECT_EQ(eins.code, 0) << eins.ausgabe;
}

// ===========================================================================================
// (3) FAIL-CLOSED AN DER ART. Kann die Wache eine Art nicht pruefen, ist das ROT -- nicht
//     stilles Durchwinken. Vier Eingaenge, alle mit demselben Ergebnis; dazu der
//     GEGENKOEDER, damit "immer rot" ausgeschlossen ist.
// ===========================================================================================
TEST(MtL4RegistrierungsWacheIsa, UnpruefbaresFeldZweiIstRotUndNichtGruen) {
    std::string const marke = koeder_marke();
    SynthBaum         baum{marke};
    baum.cache_ehrlich(/*avx2=*/true, /*avx512f=*/false);

    struct Fall {
        char const* feld2;
        char const* was;
    };
    // Der nackte Pfad ist der wichtigste Eingang: genau so sahen die vier Alt-Zeilen aus.
    // Ohne diesen Fall koennte jemand das Praefix wieder weglassen und die Wache bliebe still.
    std::vector<Fall> const faelle{{"quatsch:foo", "unbekannte Art"},
                                   {"", "leeres Feld 2"},
                                   {"prt_art/include/prt_art/prt_art.hpp", "Pfad OHNE 'datei:'"},
                                   {"isa:sse9", "unbekanntes ISA-Merkmal"},
                                   {"isa:", "'isa:' ohne Merkmal"},
                                   {"datei:", "'datei:' ohne Pfad"}};

    for (auto const& fall : faelle) {
        baum.allowlist(fall.feld2);
        Lauf const lauf = baum.fahren();
        berichten(fall.was, lauf, baum);
        EXPECT_EQ(lauf.code, 1) << "UNPRUEFBAR muss ROT sein (" << fall.was << "). Ausgabe:\n" << lauf.ausgabe;
        EXPECT_TRUE(enthaelt(lauf.ausgabe, "UNPRUEFBARE BEGRUENDUNG")) << "(" << fall.was << ") Ausgabe:\n"
                                                                       << lauf.ausgabe;
        EXPECT_TRUE(enthaelt(lauf.ausgabe, "1 mit UNPRUEFBARER")) << "(" << fall.was << ") Ausgabe:\n" << lauf.ausgabe;
    }

    // GEGENKOEDER (K13): dieselbe Maschinerie, eine WOHLGEFORMTE Zeile -- gruen. Ohne
    // diesen Fall belegten die sechs oben nur, dass die Wache immer rot ist.
    // NACHGEZOGEN 2026-08-10 (PA-1): der Gegenstand muss ABWESEND, aber ERREICHBAR sein.
    // Seit der zweiten ERLOSCHEN-Richtung genuegt "existiert nicht" nicht mehr -- ein Pfad,
    // dessen ganzer Zweig dem Repo unbekannt ist, ist eine TOTE AUSNAHME und damit ROT.
    // 'gibt/es/nicht/...' war genau das; der Fall waere aus dem falschen Grund rot geworden.
    // 'tests/unit/' kennt das Wegwerf-Repo (dort liegt die Gegenprobe-Datei), der Gegenstand
    // ist dort also abwesend UND koennte jederzeit entstehen -- die neutrale Lage, die dieser
    // Fall braucht.
    baum.allowlist("datei:tests/unit/gibt_es_nicht_" + marke + ".hpp");
    Lauf const gegen = baum.fahren();
    berichten("GEGENKOEDER wohlgeformt -> GRUEN", gegen, baum);
    EXPECT_EQ(gegen.code, 0) << "Eine wohlgeformte Zeile muss tragen. Ausgabe:\n" << gegen.ausgabe;
}

// ===========================================================================================
// (4) DIE ALTE ART BLEIBT SCHARF. 'datei:' ist keine Umbenennung, sondern dieselbe
//     Gegenprobe -- in beide Richtungen nachgefahren, damit die Umstellung des Formats
//     nicht die eine Art repariert und die andere hohl macht.
// ===========================================================================================
TEST(MtL4RegistrierungsWacheIsa, DateiArtTraegtAbwesendUndErlischtAnwesend) {
    std::string const marke = koeder_marke();
    SynthBaum         baum{marke};
    baum.cache_ehrlich(/*avx2=*/true, /*avx512f=*/true);

    // NACHGEZOGEN 2026-08-10 (PA-1): der Gegenstand muss ABWESEND, aber ERREICHBAR sein.
    // Seit der zweiten ERLOSCHEN-Richtung genuegt "existiert nicht" nicht mehr -- ein Pfad,
    // dessen ganzer Zweig dem Repo unbekannt ist, ist eine TOTE AUSNAHME und damit ROT.
    // 'gibt/es/nicht/...' war genau das; der Fall waere aus dem falschen Grund rot geworden.
    // 'tests/unit/' kennt das Wegwerf-Repo (dort liegt die Gegenprobe-Datei), der Gegenstand
    // ist dort also abwesend UND koennte jederzeit entstehen -- die neutrale Lage, die dieser
    // Fall braucht.
    baum.allowlist("datei:tests/unit/gibt_es_nicht_" + marke + ".hpp");
    Lauf const abwesend = baum.fahren();
    berichten("datei: abwesend -> GRUEN", abwesend, baum);
    EXPECT_EQ(abwesend.code, 0) << abwesend.ausgabe;

    // Die Wache liegt in JEDEM dieser Baeume -- ein Gegenstand, der garantiert existiert.
    baum.allowlist("datei:scripts/ci_test_registrierungs_wache.sh");
    Lauf const anwesend = baum.fahren();
    berichten("datei: anwesend -> ROT (erloschen)", anwesend, baum);
    EXPECT_EQ(anwesend.code, 1) << anwesend.ausgabe;
    EXPECT_TRUE(enthaelt(anwesend.ausgabe, "AUSNAHME ERLOSCHEN")) << anwesend.ausgabe;
}

// ===========================================================================================
// (5) DIE ISA-ANTWORT WIRD NICHT GEGLAUBT. Ein per '-D' erzwungener Cache sieht einem
//     gemessenen zum Verwechseln aehnlich: CMake entfernt _COMPILED und _EXITCODE beim
//     Erzwingen NIE, sie ueberleben aus dem ehrlichen Lauf davor (D2-G5, am Objekt
//     gemessen 2026-08-10). Wer nur den Wert liest, nimmt eine BEHAUPTETE Host-Klasse fuer
//     eine gemessene -- und eine gefaelschte 'nein'-Antwort haelt jede Ausnahme am Leben.
//     Alle fuenf Eingaenge muessen mit 2 enden: 'konnte nicht pruefen', ausdruecklich
//     nicht 'Merkmal fehlt'.
// ===========================================================================================
TEST(MtL4RegistrierungsWacheIsa, GefaelschterOderFehlenderCacheIstExitZweiUndNichtGruen) {
    std::string const marke = koeder_marke();
    SynthBaum         baum{marke};
    baum.allowlist("isa:avx2+avx512f");

    static constexpr char kEhrlichAvx2[] = "COMDARE_HOST_RUNS_AVX2:INTERNAL=1\n"
                                           "COMDARE_HOST_RUNS_AVX2_COMPILED:INTERNAL=TRUE\n"
                                           "COMDARE_HOST_RUNS_AVX2_EXITCODE:INTERNAL=0\n";

    // Leerer String, nicht nullptr: 'was' geht nach berichten(char const*) und wird dort in
    // einen ostream geschoben -- ein Nullzeiger waere dabei undefiniert, ein leerer nicht.
    struct Fall {
        std::string zusatz;
        char const* was = "";
    };
    std::vector<Fall> const faelle{// '-D' auf einen konfigurierten Baum: der Typ verraet es, _COMPILED nicht.
                                   {"COMDARE_HOST_RUNS_AVX512F:UNINITIALIZED=0\n"
                                    "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=TRUE\n"
                                    "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=0\n",
                                    "Typ UNINITIALIZED statt INTERNAL"},
                                   // '-DVAR:INTERNAL=0' trifft den Typ -- aber eine Probe schreibt nie eine 0.
                                   {"COMDARE_HOST_RUNS_AVX512F:INTERNAL=0\n"
                                    "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=TRUE\n"
                                    "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=0\n",
                                    "Wert 0 (den schreibt keine Probe)"},
                                   // Leerer Wert bei Exit 0: der Beleg daneben widerlegt die Behauptung.
                                   {"COMDARE_HOST_RUNS_AVX512F:INTERNAL=\n"
                                    "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=TRUE\n"
                                    "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=0\n",
                                    "Wert leer, aber _EXITCODE 0"},
                                   // Frischer Baum + '-D': die Probe lief nie, _COMPILED fehlt ganz.
                                   {"COMDARE_HOST_RUNS_AVX512F:INTERNAL=\n"
                                    "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=1\n",
                                    "_COMPILED fehlt"},
                                   // Zwei Wertzeilen: CMake nimmt beim Laden die letzte, die Wache liest die erste.
                                   {"COMDARE_HOST_RUNS_AVX512F:INTERNAL=1\n"
                                    "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=TRUE\n"
                                    "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=0\n"
                                    "COMDARE_HOST_RUNS_AVX512F:INTERNAL=\n",
                                    "zwei Wertzeilen (von Hand bearbeitet)"},
                                   // Gar keine Probe im Cache (Cross-Build): UNBEKANNT, nicht 'basis'.
                                   {"", "ISA-Probe fehlt ganz"}};

    for (auto const& fall : faelle) {
        baum.cache_roh(std::string{kEhrlichAvx2} + fall.zusatz);
        Lauf const lauf = baum.fahren();
        berichten(fall.was, lauf, baum);
        EXPECT_EQ(lauf.code, 2) << "Fail-closed verlangt Exit 2 (" << fall.was << "). Ausgabe:\n" << lauf.ausgabe;
        EXPECT_TRUE(enthaelt(lauf.ausgabe, "UNBEANTWORTBAR")) << "(" << fall.was << ") Ausgabe:\n" << lauf.ausgabe;
    }

    // Und ohne CMakeCache.txt ueberhaupt.
    baum.cache_entfernen();
    Lauf const ohne = baum.fahren();
    berichten("CMakeCache.txt fehlt", ohne, baum);
    EXPECT_EQ(ohne.code, 2) << ohne.ausgabe;
}

// ===========================================================================================
// (6) DIE ISA-FRAGE WIRD NUR GESTELLT, WENN SIE ANSTEHT. Sonst waere die neue Art ein
//     Daueralarm: jeder Baum ohne CMakeCache.txt bekaeme Exit 2, obwohl keine einzige
//     Zeile 'isa:' sagt. Der Gegeneingang zu Fall (5).
// ===========================================================================================
TEST(MtL4RegistrierungsWacheIsa, OhneIsaZeileWirdDerCacheNichtVerlangt) {
    std::string const marke = koeder_marke();
    SynthBaum         baum{marke};
    baum.cache_entfernen();
    // NACHGEZOGEN 2026-08-10 (PA-1): der Gegenstand muss ABWESEND, aber ERREICHBAR sein.
    // Seit der zweiten ERLOSCHEN-Richtung genuegt "existiert nicht" nicht mehr -- ein Pfad,
    // dessen ganzer Zweig dem Repo unbekannt ist, ist eine TOTE AUSNAHME und damit ROT.
    // 'gibt/es/nicht/...' war genau das; der Fall waere aus dem falschen Grund rot geworden.
    // 'tests/unit/' kennt das Wegwerf-Repo (dort liegt die Gegenprobe-Datei), der Gegenstand
    // ist dort also abwesend UND koennte jederzeit entstehen -- die neutrale Lage, die dieser
    // Fall braucht.
    baum.allowlist("datei:tests/unit/gibt_es_nicht_" + marke + ".hpp");

    Lauf const lauf = baum.fahren();
    berichten("kein isa:, kein Cache -> GRUEN", lauf, baum);
    EXPECT_EQ(lauf.code, 0) << "Ohne 'isa:'-Zeile darf der Cache egal sein. Ausgabe:\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "ISA-Gegenprobe: nicht gefragt"))
        << "Die Wache muss AUSWEISEN, dass sie nicht gefragt hat. Ausgabe:\n"
        << lauf.ausgabe;
}

// ===========================================================================================
// (7) DASS DIE WACHE UEBERHAUPT BEISST. Ohne diesen Fall belegten alle anderen nur, dass
//     sie ein praepariertes Feld 2 verschieden bewertet -- nicht, dass eine fehlende Zeile
//     ueberhaupt auffaellt. Das ist die Messgeraet-Gegenprobe des ganzen Harnischs (V4).
// ===========================================================================================
TEST(MtL4RegistrierungsWacheIsa, OhneAllowlistZeileIstDerKoederEinBefund) {
    std::string const marke = koeder_marke();
    SynthBaum         baum{marke};
    baum.cache_ehrlich(/*avx2=*/true, /*avx512f=*/true);
    baum.allowlist_leer();

    Lauf const lauf = baum.fahren();
    berichten("keine Zeile -> ROT (ohne Begruendung)", lauf, baum);
    EXPECT_EQ(lauf.code, 1) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, baum.koeder())) << lauf.ausgabe;
    // NENNER (V-1): die Wache nennt beide Zahlen, nicht nur den Befund.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "1 von 2 ohne Begruendung")) << lauf.ausgabe;
}

// ===========================================================================================
// (8) DIE ECHTE ALLOWLIST IST WOHLGEFORMT. Der einzige Fall an einem echten Gegenstand.
//     Er greift dort, wo die Faelle oben nicht hinreichen: eine Zeile, die im Repo landet
//     und deren Art die Wache nicht kennt, macht den CI-Job rot -- dieser Test sagt es
//     schon im Bau, in Debug wie in Release.
//     NENNER in die Ausgabe (V-1): die Zahl der geprueften Zeilen steht im Protokoll.
// ===========================================================================================
TEST(MtL4RegistrierungsWacheIsa, EchteAllowlistNutztNurBekannteArten) {
    auto const zeilen = allowlist_zeilen();
    ASSERT_FALSE(zeilen.empty()) << "Die echte Allowlist " << COMDARE_MTL4_ALLOWLIST
                                 << " hat keine einzige Wertzeile -- dann prueft dieser Fall nichts. "
                                    "Entweder ist der Pfad falsch oder die Liste ist leer.";

    std::size_t datei_n = 0;
    std::size_t isa_n   = 0;
    std::size_t frist_n = 0;
    for (auto const& zeile : zeilen) {
        auto const felder = zerlegen(zeile, '|');
        ASSERT_GE(felder.size(), 3U) << "Zeile mit weniger als drei Feldern: '" << zeile << "'";
        std::string const& feld2 = felder[1];

        if (feld2.rfind("datei:", 0) == 0) {
            EXPECT_GT(feld2.size(), std::string_view{"datei:"}.size()) << "'datei:' ohne Pfad: '" << zeile << "'";
            ++datei_n;
        } else if (feld2.rfind("isa:", 0) == 0) {
            std::string const merkmale = feld2.substr(std::string_view{"isa:"}.size());
            EXPECT_FALSE(merkmale.empty()) << "'isa:' ohne Merkmal: '" << zeile << "'";
            for (auto const& m : zerlegen(merkmale, '+')) {
                EXPECT_TRUE(m == "avx2" || m == "avx512f")
                    << "Unbekanntes ISA-Merkmal '" << m << "' in '" << zeile
                    << "'. Bekannt sind genau zwei -- mehr gattert dieses Repo nicht "
                       "(tests/unit/CMakeLists.txt:4165/5330/5343/5372/5375).";
            }
            ++isa_n;
        } else if (feld2.rfind("frist:", 0) == 0) {
            // DRITTE ART seit PA-1 (2026-08-10): die ehrlich unbeweisbare Ausnahme. Sie
            // gibt es, weil es Faelle ohne JEDEN erreichbaren Gegenstand gibt -- die vier
            // prt-art-Posten. Statt einer Pfad-Fiktion tragen sie ein Ablaufdatum.
            // Geprueft wird hier die FORM: nur JJJJ-MM-TT sortiert lexikografisch wie
            // chronologisch, und genau darauf beruht der Vergleich in der Wache.
            std::string const tag = feld2.substr(std::string_view{"frist:"}.size());
            EXPECT_EQ(tag.size(), 10U) << "'frist:' erwartet JJJJ-MM-TT: '" << zeile << "'";
            if (tag.size() == 10U) {
                bool form = tag[4] == '-' && tag[7] == '-';
                for (std::size_t i = 0; form && i < tag.size(); ++i) {
                    if (i == 4U || i == 7U) { continue; }
                    form = tag[i] >= '0' && tag[i] <= '9';
                }
                EXPECT_TRUE(form) << "'frist:" << tag << "' ist kein Datum JJJJ-MM-TT in '" << zeile << "'";
            }
            ++frist_n;
        } else {
            ADD_FAILURE() << "Feld 2 ohne bekannte Art: '" << feld2 << "' in Zeile '" << zeile
                          << "'. Erwartet 'datei:<pfad>', 'isa:<merkmal>[+<merkmal>]' oder "
                             "'frist:<JJJJ-MM-TT>'.";
        }
    }
    std::cout << "  [MT-L4] Allowlist " << COMDARE_MTL4_ALLOWLIST << ": " << zeilen.size() << " Wertzeile(n), davon "
              << datei_n << " mit 'datei:', " << isa_n << " mit 'isa:' und " << frist_n << " mit 'frist:'.\n";
}

// ===========================================================================================
// (9) T-6 SCHWESTERPFLICHT, MIT FREMDEM NENNER (T-3/V-7). Der SOLL kommt hier NICHT aus der
//     Allowlist, sondern aus tests/unit/CMakeLists.txt: jedes add_executable(), das in
//     einem 'if(... COMDARE_HOST_RUNS ...)'-Block steht, kann auf einer aermeren Host-
//     Klasse aus dem Bauweg fallen und BRAUCHT deshalb eine 'isa:'-Zeile. Heute ist das
//     genau eines (am Objekt gemessen 2026-08-10 ueber erzwungene Neukonfiguration:
//     455 -> 454 uebersetzte Test-Quelldateien, Differenz namentlich
//     test_ap5_simd_extension_coherence.cpp, Gegenrichtung leer). Kommt ein zweites dazu,
//     faellt dieser Fall -- und nicht erst der CI-Job auf dem anderen Runner.
// ===========================================================================================
TEST(MtL4RegistrierungsWacheIsa, JedesIsaGegatterteAddExecutableHatEineIsaZeile) {
    std::string const quelle = datei_lesen(fs::path{COMDARE_MTL4_TESTS_CMAKE});
    ASSERT_FALSE(quelle.empty()) << "tests/unit/CMakeLists.txt (" << COMDARE_MTL4_TESTS_CMAKE
                                 << ") ist leer oder nicht lesbar -- ohne sie hat dieser Fall keinen Nenner.";

    // Ein flacher if/endif-Zaehler: wir merken uns, ab welcher Tiefe ein umschliessendes
    // if() die Host-ISA nennt. Das genuegt hier und ist absichtlich stumpf -- ein
    // vollstaendiger CMake-Parser waere ein eigenes Werkzeug.
    std::vector<std::string> gegattert;
    int                      tiefe     = 0;
    int                      isa_tiefe = -1;
    std::istringstream       ein{quelle};
    std::string              zeile;
    while (std::getline(ein, zeile)) {
        std::string const nackt = zeile.substr(std::min(zeile.find_first_not_of(" \t"), zeile.size()));
        if (nackt.rfind("#", 0) == 0) { continue; }

        if (nackt.rfind("if(", 0) == 0) {
            ++tiefe;
            if (isa_tiefe < 0 && nackt.find("COMDARE_HOST_RUNS") != std::string::npos) { isa_tiefe = tiefe; }
        } else if (nackt.rfind("endif(", 0) == 0) {
            if (isa_tiefe == tiefe) { isa_tiefe = -1; }
            --tiefe;
        } else if (isa_tiefe > 0 && nackt.rfind("add_executable(", 0) == 0) {
            auto const start = nackt.find('(') + 1;
            auto const ende  = nackt.find_first_of(" \t)", start);
            gegattert.push_back("tests/unit/" + nackt.substr(start, ende - start) + ".cpp");
        }
    }

    // MESSGERAET-GEGENPROBE (V4): findet der Scanner NICHTS, ist entweder das Gatter
    // verschwunden (ein Befund) oder der Scanner kaputt (auch einer). Still gruen ist
    // beides nicht.
    ASSERT_FALSE(gegattert.empty()) << "Kein einziges add_executable() hinter 'COMDARE_HOST_RUNS' gefunden. "
                                       "Entweder ist das Host-ISA-Gatter aus tests/unit/CMakeLists.txt "
                                       "verschwunden, oder dieser Scanner trifft nicht mehr -- in beiden "
                                       "Faellen ist dieser Fall wertlos und meldet deshalb ROT.";

    auto const  zeilen    = allowlist_zeilen();
    std::size_t gedeckt_n = 0;
    for (auto const& quelldatei : gegattert) {
        bool gedeckt = false;
        for (auto const& z : zeilen) {
            auto const felder = zerlegen(z, '|');
            if (felder.size() >= 3 && felder[0] == quelldatei && felder[1].rfind("isa:", 0) == 0) { gedeckt = true; }
        }
        if (gedeckt) { ++gedeckt_n; }
        EXPECT_TRUE(gedeckt) << "tests/unit/CMakeLists.txt legt '" << quelldatei
                             << "' hinter einem COMDARE_HOST_RUNS-Gatter an; auf einem Host ohne dieses "
                                "Merkmal wird die Datei nie uebersetzt. Dann braucht sie eine 'isa:'-Zeile "
                                "in "
                             << COMDARE_MTL4_ALLOWLIST
                             << " -- sonst ist der CI-Job test:coverage-guard "
                                "auf dem anderen Runner rot.";
    }
    std::cout << "  [MT-L4] ISA-gegatterte add_executable() in tests/unit/CMakeLists.txt: " << gegattert.size()
              << ", davon mit 'isa:'-Zeile gedeckt: " << gedeckt_n << " (SOLL aus dem CMake-Quelltext, "
              << "IST aus der Allowlist -- zwei verschiedene Dateien).\n";
}

#endif // !_WIN32
