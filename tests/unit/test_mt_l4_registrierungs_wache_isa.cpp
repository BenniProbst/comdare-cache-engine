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

// GANZE ZEILE (Fix-r12, Lens B r10 LB10-01; Vorbild zeile_exakt() in test_pa1_tote_ausnahme.cpp): enthaelt()
// traefe den Text auch mitten in einer laengeren oder praefigierten Zeile. Eine Nenner-Zeile wird deshalb als
// GANZE Zeile verlangt -- wahlweise mit der zweistelligen Einrueckung des Nenner-Blocks der Wache.
[[nodiscard]] bool zeile_exakt(std::string const& ausgabe, std::string const& text) {
    for (std::size_t start = 0; start <= ausgabe.size();) {
        std::size_t const ende = ausgabe.find('\n', start);
        std::string const zl   = ausgabe.substr(start, ende == std::string::npos ? std::string::npos : ende - start);
        if (zl == text || zl == "  " + text) { return true; }
        if (ende == std::string::npos) { break; }
        start = ende + 1;
    }
    return false;
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
//
// DER WURZEL-VERTRAG DER WACHE (Fix-474, 2026-09-23, EXPLORE-474-DESIGN-K282 Option A):
// seit Fix-r10 (Kopf der Wache, Folgen (17c)/(17d)) sucht die Wache jeden Quellpfad im
// Bauweg an der ABSOLUTEN Repo-Wurzel verankert ('git rev-parse --show-toplevel'), mit
// einer linken Grenze davor und einer rechten Grenze dahinter. Eine Attrappe unter der
// Phantasie-Wurzel '/x/' traf dieses Muster nicht mehr: die Messgeraet-Gegenprobe fiel
// durch (ABBRUCH, Exit 2) und sieben der neun Faelle starben VOR jeder Allowlist-
// Auswertung. Deshalb: erst 'git init', dann die von git GEMELDETE Wurzel holen und
// gegen den eigenen Pfad halten (sonst misst der Fall still den falschen Baum), dann
// die Attrappe in der CI-Form von ninja schreiben -- Objektpfad build-relativ links,
// Quellpfad absolut unter der gemeldeten Wurzel rechts, einmal vor ' || deps' und einmal
// am Zeilenende (beide rechten Grenzen). Der Koeder steht weiterhin NICHT darin (K13).
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

        // Das Repo entsteht VOR der Attrappe: die Attrappe braucht die Wurzel, die git meldet.
        // Jeder git-Aufruf faehrt isoliert von der globalen und der System-Konfiguration des
        // Bau-Hosts -- so wie die Wache selbst in fahren() (sonst laese ihr 'git rev-parse'
        // eine andere Konfiguration als dieser Aufbau).
        (void)schale(umgebung() + " git -C \"" + wurzel_.string() + "\" init -q 2>&1");
        (void)schale(umgebung() + " git -C \"" + wurzel_.string() + "\" add -- tests/unit/test_pressure_state.cpp \"" +
                     koeder_ + "\" 2>&1");

        // DIE VON GIT GEMELDETE WURZEL, nicht wurzel_.string(): 'git rev-parse --show-toplevel'
        // ist die Quelle der Wache; nur ein byte-gleicher Pfad trifft ihr Muster. Weicht die
        // Meldung ab oder scheitert sie, misst der Fall den falschen Baum -- das ist ein
        // Fehler dieses Aufbaus, kein Urteil der Wache, und wird als solcher gemeldet.
        Lauf const meldung = schale(umgebung() + " git -C \"" + wurzel_.string() + "\" rev-parse --show-toplevel 2>&1");
        std::string gemeldet = meldung.ausgabe;
        while (!gemeldet.empty() && (gemeldet.back() == '\n' || gemeldet.back() == '\r')) { gemeldet.pop_back(); }
        if (meldung.code != 0 || gemeldet.empty()) {
            ADD_FAILURE() << "git rev-parse --show-toplevel in '" << wurzel_.string() << "' fehlgeschlagen (Exit "
                          << meldung.code << "):\n"
                          << meldung.ausgabe;
        } else {
            std::error_code ec_erwartet;
            std::error_code ec_ist;
            fs::path const  erwartet = fs::canonical(wurzel_, ec_erwartet);
            fs::path const  ist      = fs::canonical(fs::path{gemeldet}, ec_ist);
            if (ec_erwartet || ec_ist || erwartet != ist) {
                ADD_FAILURE() << "git meldet die Wurzel '" << gemeldet << "', erwartet war '" << wurzel_.string()
                              << "' -- der Fall wuerde den falschen Baum messen.";
            }
        }
        gemeldete_wurzel_ = gemeldet;

        // IST-Quelle: nur die Gegenprobe steht im Bauweg, der Koeder NICHT. Genau das
        // ist der Zustand, den ein uebersprungenes add_executable() erzeugt. CI-Form von
        // ninja (Vorbild test_pa1_tote_ausnahme.cpp, bauweg_schreiben): links der
        // build-relative Objektpfad, rechts der absolute Quellpfad unter der gemeldeten
        // Wurzel -- in der ersten Zeile vor ' || deps', in der zweiten am Zeilenende: beide
        // FORMEN der rechten Grenze stehen in der Attrappe, das Leerzeichen davor ist die
        // linke. BERICHTIGT (Fix-r12, Lens B r10 LB10-I02): 'gedeckt' sind die Grenzen hier
        // NICHT einzeln -- jede der beiden Zeilen allein erfuellt die Gegenprobe, ein Mutant
        // ohne '$' oder ohne '[[:space:]]' in IST_GRENZE ueberlebt diesen Test 9/9; die
        // Deckung JE Grenze traegt test_pa1_tote_ausnahme (35b)/(35d).
        {
            std::string const quelle = gemeldete_wurzel_ + "/tests/unit/test_pressure_state.cpp";
            std::ofstream     aus{wurzel_ / "baum" / "build.ninja"};
            aus << "build CMakeFiles/x.dir/a.o: CXX_COMPILER__x_Release " << quelle
                << " || cmake_object_order_depends_target_x\n";
            aus << "build CMakeFiles/x.dir/tests/unit/test_pressure_state.cpp.o: CXX_COMPILER__x_Release " << quelle
                << "\n";
        }
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

    /// GAR KEINE Allowlist-Datei (Fix-r12, Lens A r11 LA11-01 = Lens B r10 LB10-02): die Wache toleriert das
    /// (allow_zeile liefert keine Zeile, der Nachscan wird uebersprungen) und muss es in der Bilanz SAGEN.
    void allowlist_entfernen() const {
        std::error_code ec;
        fs::remove(wurzel_ / "scripts" / "ci_test_registrierungs_allowlist.txt", ec);
    }

    /// DER ALLOWLIST-PFAD dieses Baums (Fix-r13): fuer die Vorbedingungen der Faelle (7b)/(7c) und die zwei
    /// Nicht-Datei-Formen darunter.
    [[nodiscard]] fs::path allowlist_pfad() const {
        return wurzel_ / "scripts" / "ci_test_registrierungs_allowlist.txt";
    }

    /// Liegt am Allowlist-Pfad IRGENDETWAS? exists() folgt einem Symlink; ein Symlink ohne Ziel ist nur ueber
    /// is_symlink() sichtbar -- dieselbe Zweiteilung wie '[ -e ] || [ -L ]' in der Wache.
    [[nodiscard]] bool allowlist_pfad_belegt() const {
        std::error_code ec_e;
        std::error_code ec_l;
        return fs::exists(allowlist_pfad(), ec_e) || fs::is_symlink(allowlist_pfad(), ec_l);
    }

    /// EIN VERZEICHNIS am Allowlist-Pfad (Fix-r13, Lens A r12 LA12-01 = Lens B r11 LB11-01): der Pfad ist belegt,
    /// aber keine regulaere Datei -- die Wache darf ihn weder lesen noch 'FEHLT' nennen.
    void allowlist_als_verzeichnis() const {
        std::error_code ec;
        fs::remove(allowlist_pfad(), ec);
        fs::create_directory(allowlist_pfad(), ec);
    }

    /// EIN SYMLINK OHNE ZIEL am Allowlist-Pfad (dieselbe Klasse; fuer '-e' unsichtbar, fuer '-L' belegt). Das Ziel
    /// ist ein Name, den es im Baum nie gibt -- er erscheint in keiner Ausgabe der Wache.
    void allowlist_als_symlink_ohne_ziel() const {
        std::error_code ec;
        fs::remove(allowlist_pfad(), ec);
        fs::create_symlink("ziel_ohne_datei.txt", allowlist_pfad(), ec);
    }

    [[nodiscard]] Lauf fahren() const {
        return schale("cd \"" + wurzel_.string() + "\" && " + umgebung() +
                      " sh scripts/ci_test_registrierungs_wache.sh baum 2>&1");
    }

    [[nodiscard]] std::string const& koeder() const { return koeder_; }

    // Dieselbe Isolation wie WegwerfRepo::umgebung() der Werkbank (support/wachen_werkbank.hpp):
    // ohne sie hinge jeder Fall an der globalen git-Konfiguration des Bau-Hosts. Der Umzug
    // dieses Baums AUF die Werkbank ist ein eigener Zug (Board #253), nicht dieser Fix.
    [[nodiscard]] static std::string umgebung() {
        return "GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null GIT_TERMINAL_PROMPT=0";
    }

private:
    fs::path    wurzel_;
    std::string koeder_;
    std::string gemeldete_wurzel_;
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
    // NENNER DER ALLOWLIST (Lens C r8 LC8W-10, Fix-474 (c)): die Wache nennt, wie viele Datenzeilen sie
    // gelesen hat -- sonst ist '0 Allowlist-Zeile(n) fuer ARCHIV-Dateien' nicht von 'gar nicht gelesen'
    // zu trennen. Dieser Baum: eine Kommentar- und eine Datenzeile -> genau 1.
    EXPECT_TRUE(enthaelt(ohne.ausgabe, "Allowlist gelesen: 1 Datenzeile(n) in "
                                       "scripts/ci_test_registrierungs_allowlist.txt "
                                       "(Kommentar- und Leerzeilen abgezogen)."))
        << "Die Wache muss den NENNER der Allowlist nennen. Ausgabe:\n"
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
    // NULLSEITE DER NENNER-ZEILE (Fix-r12, Lens B r10 LB10-01): eine VORHANDENE Allowlist ohne Datenzeile (nur
    // die Kommentarzeile von allowlist_leer()) zaehlt 0 -- als GANZE Zeile gepinnt. Ein Mutant mit fester Zahl
    // (WM2: ALLOW_ZEILEN_N=1) oder stummer Zeile bei 0 (WM2b) ueberlebte Fall (1) allein 9/9 + test_pa1 39/39
    // (LENS-B-r10.md Abschn. 2.6); dieser Pin und die test_pa1-Pins (Fall (8), Mehrzeilen) machen beide rot.
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, "Allowlist gelesen: 0 Datenzeile(n) in "
                                          "scripts/ci_test_registrierungs_allowlist.txt "
                                          "(Kommentar- und Leerzeilen abgezogen)."))
        << "Die Nullseite der Nenner-Zeile muss als ganze Zeile stehen. Ausgabe:\n"
        << lauf.ausgabe;
    // GEGENRICHTUNG zu Fall (7b): die Datei IST da -- die FEHLT-Form darf hier nicht stehen.
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "Allowlist NICHT gelesen"))
        << "Eine vorhandene Datei ohne Datenzeile ist GELESEN (0), nicht FEHLT. Ausgabe:\n"
        << lauf.ausgabe;
}

// ===========================================================================================
// (7b) OHNE ALLOWLIST-DATEI SAGT DIE BILANZ 'NICHT GELESEN', NICHT '0 GELESEN' (Fix-r12, Lens A r11 LA11-01
//      = Lens B r10 LB10-02; Lens-Kennung 'Fall (8b)'). Die Wache toleriert eine fehlende Allowlist (Vorbestand:
//      allow_zeile liefert keine Zeile, der Nachscan wird uebersprungen); bis 1fa2f50b lautete die Nenner-Zeile
//      dann 'Allowlist gelesen: 0 Datenzeile(n) in ...' -- byte-gleich zur VORHANDENEN Datei ohne Datenzeile
//      (Fall (7)), obwohl nichts gelesen wurde: genau die Falsch-Null, gegen die die Zeile gebaut ist (Lens C
//      r8 LC8W-10 'N Datenzeilen / FEHLT'). Jetzt traegt die Bilanz die eigene Form 'Allowlist NICHT gelesen:
//      <pfad> FEHLT -- Nachscan uebersprungen (0 Datenzeile(n)).'; Urteil (ROT: der Koeder ist ohne
//      Begruendung) und Exit-Vertrag sind unveraendert. Beide Richtungen: Fall (7) verlangt die Null-Form und
//      verbietet die FEHLT-Form, dieser Fall umgekehrt.
// ===========================================================================================
TEST(MtL4RegistrierungsWacheIsa, OhneAllowlistDateiHeisstDieBilanzNichtGelesenStattNull) {
    std::string const marke = koeder_marke();
    SynthBaum         baum{marke};
    baum.cache_ehrlich(/*avx2=*/true, /*avx512f=*/true);
    baum.allowlist_entfernen();

    Lauf const lauf = baum.fahren();
    berichten("keine Allowlist-DATEI -> ROT + 'NICHT gelesen'", lauf, baum);
    EXPECT_EQ(lauf.code, 1) << "Ohne Allowlist ist der Koeder ohne Begruendung -- Befund-Code 1. Ausgabe:\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "1 von 2 ohne Begruendung")) << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, "Allowlist NICHT gelesen: scripts/ci_test_registrierungs_allowlist.txt "
                                          "FEHLT -- Nachscan uebersprungen (0 Datenzeile(n))."))
        << "Die Bilanz muss die fehlende Datei als NICHT gelesen ausweisen. Ausgabe:\n"
        << lauf.ausgabe;
    EXPECT_FALSE(enthaelt(lauf.ausgabe, "Allowlist gelesen: 0 Datenzeile(n)"))
        << "Die Null-Form gehoert der VORHANDENEN Datei ohne Datenzeile (Fall (7)), nicht der fehlenden. Ausgabe:\n"
        << lauf.ausgabe;
}

// ===========================================================================================
// (7c) EIN BELEGTER NICHT-DATEI-PFAD IST KEIN 'FEHLT' (Fix-r13, Lens A r12 LA12-01 = Lens B r11 LB11-01). Die
//      FEHLT-Form aus (7b) stand bis 2c5f8b00 auch fuer ein VERZEICHNIS, eine FIFO und einen Symlink ohne Ziel am
//      Allowlist-Pfad -- der Pfad ist dann belegt, nur keine regulaere Datei (Kunstbaeume PDIR / PFIFO / PBROKEN
//      bzw. P-DIR / P-DANGLING, je 3 Shells; Klon-Mutanten WM-E / MB2 '-f' nach '-e' ueberlebten 10/10 + 39/39).
//      Jetzt klassifiziert die Wache den Pfad (ALLOW_PFAD_ART) und sagt 'ist VORHANDEN, aber keine regulaere
//      Datei (<Art>)'. Toleranz und Exit-Vertrag bleiben: ROT nur, weil der Koeder ohne Begruendung ist; Exit 2
//      fuer diese Klasse waere eine Vertragsaenderung (Lead-Entscheid O-1). Zwei Stufen: (a) Verzeichnis,
//      (b) Symlink ohne Ziel ('-e' folgt dem Link und sieht nichts, '-L' sieht den Link). Eine FIFO deckt NUR die
//      Shell-Probe (FIX-r13 Abschn. 7): ein Leser an einer FIFO ohne Schreiber blockiert -- unter einem
//      '-f'-Mutanten hinge dieser Prozess statt rot zu werden.
// ===========================================================================================
TEST(MtL4RegistrierungsWacheIsa, BelegterNichtDateiPfadHeisstVorhandenStattFehlt) {
    std::string const marke = koeder_marke();
    SynthBaum         baum{marke};
    baum.cache_ehrlich(/*avx2=*/true, /*avx512f=*/true);

    auto const pruefen = [&baum](char const* was, char const* art, Lauf const& lauf) {
        berichten(was, lauf, baum);
        EXPECT_EQ(lauf.code, 1) << "(" << was
                                << ") Ohne lesbare Allowlist ist der Koeder ohne Begruendung -- Befund-Code 1, "
                                   "kein Abbruch. Ausgabe:\n"
                                << lauf.ausgabe;
        EXPECT_TRUE(enthaelt(lauf.ausgabe, "1 von 2 ohne Begruendung")) << "(" << was << ") " << lauf.ausgabe;
        EXPECT_TRUE(zeile_exakt(lauf.ausgabe,
                                std::string{"Allowlist NICHT gelesen: scripts/ci_test_registrierungs_allowlist.txt "
                                            "ist VORHANDEN, aber keine regulaere Datei ("} +
                                    art + ") -- Nachscan uebersprungen (0 Datenzeile(n))."))
            << "(" << was << ") Die Bilanz muss den belegten Pfad mit seiner Art nennen. Ausgabe:\n"
            << lauf.ausgabe;
        EXPECT_FALSE(enthaelt(lauf.ausgabe, " FEHLT -- Nachscan uebersprungen"))
            << "(" << was << ") 'FEHLT' gehoert dem leeren Pfad (Fall (7b)), nicht dem belegten. Ausgabe:\n"
            << lauf.ausgabe;
        EXPECT_FALSE(enthaelt(lauf.ausgabe, "Allowlist gelesen: 0 Datenzeile(n)"))
            << "(" << was << ") Nichts wurde gelesen -- die Null-Form waere die Falsch-Null. Ausgabe:\n"
            << lauf.ausgabe;
    };

    // (a) VERZEICHNIS: '-e' wahr, '-f' falsch.
    baum.allowlist_als_verzeichnis();
    ASSERT_TRUE(fs::is_directory(baum.allowlist_pfad())) << "Aufbau: kein Verzeichnis am Allowlist-Pfad.";
    pruefen("Allowlist-Pfad ist ein VERZEICHNIS", "Verzeichnis", baum.fahren());

    // (b) SYMLINK OHNE ZIEL: '-e' falsch (folgt dem Link), '-L' wahr -- fuer eine Probe nur mit '-e' saehe der
    //     Pfad leer aus, und die Bilanz sagte wieder FEHLT.
    baum.allowlist_als_symlink_ohne_ziel();
    ASSERT_TRUE(fs::is_symlink(baum.allowlist_pfad())) << "Aufbau: kein Symlink am Allowlist-Pfad.";
    ASSERT_FALSE(fs::exists(baum.allowlist_pfad())) << "Aufbau: der Symlink hat ein Ziel -- er soll keines haben.";
    pruefen("Allowlist-Pfad ist ein SYMLINK OHNE ZIEL", "Symlink ohne Ziel", baum.fahren());
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
