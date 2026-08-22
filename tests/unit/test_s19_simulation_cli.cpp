// test_s19_simulation_cli.cpp -- S-19 PLANUNGS-SIMULATION (#7, 2026-08-20): die KOMMANDO-Schicht am
// ECHTEN Planer und am ECHTEN golden-320-Profil (m3v2_study) -- kein Nachbau der Kette (T-2).
//
// == T-3 / T-5: WOHER DIE SOLL-WERTE KOMMEN =======================================================
// Die Zahlen-Orakel stammen aus dem PROFIL-XML SELBST (m3v2_study.profile.xml, von Hand gezaehlt)
// und aus der Kommando-HILFE (Exit-Codes) -- beides andere Quellen als der gepruefte Code-Pfad:
//   organ_produkt = 4 (search_algo) * 4 (node_type) * 5 (memory_layout) * 4 (prefetch) * 1^14 = 320
//   system_perms  = 1 (kein <system_axes> -> Default O3 x no_extension)
//   n_bau         = 320 * 1 = 320
//   dyn_produkt   = 1 (thread_count) * 3 (hw_prefetcher) * 6 (workloads) * 4 (working_set)  = 72
//   messungen     = 3 (drift-reps DEFAULT, kein <drift_gate>) * 3 (<repetitions count="3">)  = 9
//   lager         = 320 * 428000 = 136960000 (Grenz-Probe: Budget == Bedarf haelt, 1 darunter reisst)
// Die REKOMBINATION wird NICHT auf eine feste Zahl geprueft: sie haengt an der GEMESSENEN PMC-Torlage
// dieses Hosts. Geprueft wird die KONSISTENZ: pmc_tor=an => 4! = 24, pmc_tor=aus => 3! = 6 (die
// Fakultaets-Orakel stehen von Hand in test_s19_simulation_rechnung; hier zaehlt die Kopplung).
//
// == T-4 ==========================================================================================
// Kaputte Flags (rc 2), unbekanntes Profil (rc 5), verlangter Deckel ohne Kalibrierung (rc 1,
// unbestimmbar -- fail-closed), Kandidaten mit und ohne ganzzahliges Verhaeltnis.
//
// ARGUMENTE: argv[1] = Planer-Binary ($<TARGET_FILE:...>), argv[2] = m3v2_study-Profilpfad (aus der
// CMakeLists gereicht, nicht aus der Umgebung geraten). Rezept gespiegelt von test_check_size_cli_deckel.

#include <cstdio>
#include <cstring>
#include <string>

#if defined(_WIN32)
#define COMDARE_POPEN  _popen
#define COMDARE_PCLOSE _pclose
#else
#include <sys/wait.h>
#define COMDARE_POPEN  popen
#define COMDARE_PCLOSE pclose
#endif

namespace {

int gesamt = 0;
int rot    = 0;

void pruefe(bool bedingung, char const* was) {
    ++gesamt;
    if (!bedingung) {
        ++rot;
        std::printf("  ROT : %s\n", was);
    }
}

struct Lauf {
    int         rc = -1;
    std::string ausgabe;
};

Lauf fahre(std::string const& binary, std::string const& argzeile) {
    Lauf        l{};
    std::string cmd = "\"" + binary + "\" " + argzeile + " 2>&1";
    std::FILE*  p   = COMDARE_POPEN(cmd.c_str(), "r");
    if (p == nullptr) return l;
    char puffer[512];
    while (std::fgets(puffer, sizeof(puffer), p) != nullptr) l.ausgabe += puffer;
    int const status = COMDARE_PCLOSE(p);
#if defined(_WIN32)
    l.rc = status;
#else
    l.rc = (status >= 0 && WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
#endif
    return l;
}

void pruefe_lauf(Lauf const& l, int soll_rc, char const* nadel, char const* was) {
    ++gesamt;
    bool const rc_ok   = (l.rc == soll_rc);
    bool const text_ok = (nadel == nullptr) || (l.ausgabe.find(nadel) != std::string::npos);
    if (!rc_ok || !text_ok) {
        ++rot;
        std::printf("  ROT : %s -- rc ist=%d soll=%d%s%s\n", was, l.rc, soll_rc,
                    text_ok ? "" : " -- es fehlt der Text: ", text_ok ? "" : nadel);
        std::printf("        Ausgabe (gekuerzt): %.400s\n", l.ausgabe.c_str());
    }
}

} // namespace

int main(int argc, char* argv[]) {
    std::printf("test_s19_simulation_cli -- die Simulations-Kommando-Schicht (echte Binary, echtes Profil)\n");
    if (argc < 3) {
        std::printf("  ROT : Aufruf braucht argv[1]=Planer-Binary argv[2]=m3v2_study-Profil (aus CMake)\n");
        return 1;
    }
    std::string const binary = argv[1];
    std::string const profil = std::string{"\""} + argv[2] + "\"";

    // == L1: der golden-320-Lauf -- die Hand-Orakel aus dem XML ===================================
    auto const l1 = fahre(binary, "simulate " + profil);
    pruefe_lauf(l1, 0, "organ_produkt=320", "L1: organ_produkt 4*4*5*4 aus dem XML");
    pruefe(l1.ausgabe.find("n_bau=320") != std::string::npos, "L1: n_bau = 320 * 1 System-Perm");
    pruefe(l1.ausgabe.find("dyn_produkt=72") != std::string::npos, "L1: dyn_produkt 1*3*6*4");
    pruefe(l1.ausgabe.find("messungen_je_einstellung=9") != std::string::npos, "L1: T-15 x KF-10 = 3*3");
    pruefe(l1.ausgabe.find("summe_n_bau=320") != std::string::npos, "L1: Kampagne aus 1 XML = 320");
    pruefe(l1.ausgabe.find("zellen=1/1") != std::string::npos, "L1: 1 Zelle, Gegenprobe deckungsgleich");

    // == L2: Rekombinations-KONSISTENZ mit der gemessenen Torlage (keine Host-Annahme) ============
    bool const tor_an  = l1.ausgabe.find("pmc_tor=an") != std::string::npos;
    bool const tor_aus = l1.ausgabe.find("pmc_tor=aus") != std::string::npos;
    pruefe(tor_an != tor_aus, "L2: genau eine Torlage in der Schlusszeile");
    if (tor_an) {
        pruefe(l1.ausgabe.find("rekombination=24") != std::string::npos, "L2: Tor an -> 4! = 24");
    } else {
        pruefe(l1.ausgabe.find("rekombination=6") != std::string::npos, "L2: Tor aus -> 3! = 6");
    }

    // == L3: Kandidaten-Gegenprobe (320 trifft; 131072 hat KEIN ganzzahliges Verhaeltnis zu 320) ==
    auto const l3 = fahre(binary, "simulate " + profil + " --kandidaten=131072,320,64");
    pruefe_lauf(l3, 0, "kandidat 320: TRIFFT", "L3: Kandidat 320 trifft");
    pruefe(l3.ausgabe.find("kandidat 131072: weicht ab") != std::string::npos,
           "L3: 131072 weicht ab (131072 = 409*320 + 192, von Hand)");
    pruefe(l3.ausgabe.find("kandidat 64: Faktor 5 KLEINER") != std::string::npos, "L3: 64 = 320/5, von Hand");

    // == L4/L5: kaputte Flags sind rc 2 (Hilfe-Vertrag), nie stumm ================================
    pruefe_lauf(fahre(binary, "simulate " + profil + " --mess-teilmenge=0.5"), 2, "GANZE Zahl",
                "L4: 0.5 ist keine ganze Teilmenge");
    pruefe_lauf(fahre(binary, "simulate " + profil + " --fremde-lane=foo"), 2, "amd|intel|unbrauchbar",
                "L5: unbekannte Lane ist rc 2");
    pruefe_lauf(fahre(binary, "simulate " + profil + " --so-nicht"), 2, "unbekanntes Flag",
                "L5b: unbekanntes Flag ist rc 2");

    // == L6: verlangter Zeit-Deckel ohne Kalibrierung ist UNBESTIMMBAR und lehnt ab (fail-closed) =
    pruefe_lauf(fahre(binary, "simulate " + profil + " --t3-fenster-tage=1"), 1, "unbestimmbar",
                "L6: OV-4 ohne Kalibrierungen fail-closed");

    // == L7: unbekanntes Profil ist rc 5 (Hilfe-Vertrag) ==========================================
    pruefe_lauf(fahre(binary, "simulate /pfad/den/es/sicher/nicht/gibt.profile.xml"), 5, nullptr,
                "L7: unlesbares Profil ist rc 5");

    // == L8: Kampagne aus ZWEI XMLs (dasselbe Profil zweimal): Summe verdoppelt, Union bleibt =====
    auto const l8 = fahre(binary, "simulate " + profil + " " + profil);
    pruefe_lauf(l8, 0, "summe_n_bau=640", "L8: Summe 320+320");
    pruefe(l8.ausgabe.find("320 x 1 = 320") != std::string::npos, "L8: FULL-JOIN-Union bleibt 320 x 1");

    // == L9: Lager-Gate mit GN-9-Kalibrierwert -- Grenze haelt, ein Byte darunter reisst ==========
    auto const basis = "simulate " + profil + " --bytes-je-dll=428000 --lager-budget-bytes=";
    pruefe_lauf(fahre(binary, basis + "136960000"), 0, "lager: haelt", "L9: Budget == 320*428000 haelt");
    pruefe_lauf(fahre(binary, basis + "136959999"), 1, "GERISSEN", "L9b: ein Byte darunter reisst");

    std::printf("%s: %d Pruefungen, %d rot\n", (rot == 0) ? "GRUEN" : "ROT", gesamt, rot);
    return (rot == 0) ? 0 : 1;
}
