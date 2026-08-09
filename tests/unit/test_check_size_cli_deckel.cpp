// test_check_size_cli_deckel.cpp -- check-size (2026-08-09): die CLI-/FASSADEN-SCHICHT des Byte-Deckels.
//
// == WARUM DIESE SCHICHT EINEN EIGENEN TEST BRAUCHT ================================================
// Der Lens-Befund vom 09.08.2026 fand VIER Defekte (B-1..B-4), und alle vier lagen in der Schicht,
// die test_check_size_mengenrechnung nicht deckt: der Argument-Schleife und run_check_size_guarded.
// Der Rechner war korrekt -- der Weg VOM KOMMANDO ZUM RECHNER verlor den Deckel:
//   B-1  --max-bytes=0.5  wurde als double geparst, static_cast<uint64_t>(0.5) == 0 == "kein Deckel",
//        rc 0. Gemessen VOR dem Fix: "speicher=kein_deckel ... urteil=ok", rc=0.
//   B-2  "max-bytes=1000" (Striche vergessen, Positional-Platz belegt) wurde ERSATZLOS verworfen,
//        rc 0. Gemessen VOR dem Fix: "speicher=kein_deckel ... urteil=ok", rc=0.
//   B-3  ein kaputtes Flag fiel als rc 1 -- derselbe Code wie "Deckel GERISSEN", in der CI
//        ununterscheidbar (die Hilfe verspricht 2).
// Dieser Test faehrt die ECHTE Binary als Prozess -- kein Nachbau der Parse-Logik (T-2/GEGENSTAND:
// gemessen wird das Kommando, nicht ein Modell davon).
//
// == T-3 / T-5 =====================================================================================
// Die Soll-Exit-Codes stammen aus der HILFE des Kommandos ('help check-size': "0 haelt; 1 Deckel
// gerissen/...; 2 kaputtes Flag ...; 5 unbekannte Profil-Wurzel") -- einer anderen Quelle als der
// gepruefte Code-Pfad selbst. Kein Sollwert ist aus dem Prueflung zurueckgelesen.
//
// == T-4 ===========================================================================================
// Zu jeder Ablehnung ein Gegeneingang, bei dem dieselbe Stelle DURCHLAESST: K5 (wohlgeformter
// Deckel erreicht die Profil-Stufe), K8/K9 (wohlgeformter Deckel urteilt GERISSEN bzw. haelt).
//
// ARGUMENTE: argv[1] = Pfad der comdare-experiment-planner-Binary ($<TARGET_FILE:...>, Muster
// test_adhoc_buildvariant_dll); argv[2] = Pfad eines GUELTIGEN Profils (aus der CMakeLists gereicht,
// nicht aus der Umgebung geraten). POSIX-popen; unter _WIN32 die _popen-Geschwister.

#include <cstdio>
#include <cstring>
#include <string>

#if defined(_WIN32)
#define COMDARE_POPEN _popen
#define COMDARE_PCLOSE _pclose
#else
#include <sys/wait.h>
#define COMDARE_POPEN popen
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
    int         rc = -1; ///< -1 == der Prozess lief nicht/starb ohne exit -- immer ROT
    std::string ausgabe; ///< stdout UND stderr (2>&1), damit Diagnosen pruefbar sind
};

/// Faehrt die Binary mit den gegebenen Argumenten; die Argumente sind hier fest verdrahtete
/// ASCII-Woerter ohne Shell-Metazeichen, nur der Binary-/Profil-Pfad wird gequotet.
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
    bool const rc_ok    = (l.rc == soll_rc);
    bool const text_ok  = (nadel == nullptr) || (l.ausgabe.find(nadel) != std::string::npos);
    if (!rc_ok || !text_ok) {
        ++rot;
        std::printf("  ROT : %s -- rc ist=%d soll=%d%s%s\n", was, l.rc, soll_rc,
                    text_ok ? "" : " -- es fehlt der Text: ", text_ok ? "" : nadel);
        std::printf("        Ausgabe (gekuerzt): %.300s\n", l.ausgabe.c_str());
    }
}

} // namespace

int main(int argc, char* argv[]) {
    std::printf("test_check_size_cli_deckel -- der Byte-Deckel an der Kommando-Schicht (echte Binary)\n");
    if (argc < 3) {
        std::printf("  ROT : Aufruf braucht argv[1]=Planer-Binary argv[2]=gueltiges Profil (aus CMake)\n");
        return 1;
    }
    std::string const binary = argv[1];
    std::string const profil = argv[2];
    // Ein Profil-Pfad, den es SICHER nicht gibt: fuer die Faelle, die die Profil-Stufe erreichen sollen.
    std::string const kein_profil = "/nonexistent/comdare_check_size_cli_probe";

    // == K1..K4 / B-1: ein Deckel, der keine positive GANZE Zahl ist, ist rc 2 -- NIE "kein Deckel" ==
    {
        auto const l = fahre(binary, "check-size --max-bytes=0.5");
        pruefe_lauf(l, 2, "GANZE Zahl", "K1: --max-bytes=0.5 => rc 2 mit Ganzzahl-Diagnose");
        pruefe(l.ausgabe.find("kein_deckel") == std::string::npos,
               "K1: 0.5 wird NICHT stillschweigend zu 'kein Deckel'");
    }
    pruefe_lauf(fahre(binary, "check-size --max-bytes=1e-9"), 2, "GANZE Zahl",
                "K2: --max-bytes=1e-9 => rc 2 (kein Trunkieren nach 0)");
    pruefe_lauf(fahre(binary, "check-size --max-bytes=18446744073709551616"), 2, "GANZE Zahl",
                "K3: --max-bytes=2^64 => rc 2 (ERANGE, kein UB-Cast)");
    pruefe_lauf(fahre(binary, "check-size --max-bytes=0"), 2, "GANZE Zahl",
                "K4: --max-bytes=0 => rc 2 (ein Null-Deckel ist ein Tippfehler, kein 'nicht pruefen')");

    // == K5 / T-4-GEGENEINGANG: ein WOHLGEFORMTER Deckel passiert die Parse-Stufe ====================
    // (rc 5 = unbekannte Profil-Wurzel: der Lauf kam ueber das Flag-Parsen HINAUS bis zur Profil-Stufe.)
    pruefe_lauf(fahre(binary, "check-size " + kein_profil + " --max-bytes=1000"), 5, "unbekannte/unlesbare",
                "K5: --max-bytes=1000 passiert den Parser und erreicht die Profil-Stufe (rc 5)");

    // == K6 / B-2: ein NICHT verbrauchtes Positional wird abgelehnt, nie verworfen ===================
    {
        auto const l = fahre(binary, "check-size " + kein_profil + " max-bytes=1000");
        pruefe_lauf(l, 2, "ueberzaehliges Argument",
                    "K6: 'max-bytes=1000' ohne Striche bei belegtem Profil-Platz => rc 2, nicht verworfen");
        pruefe(l.ausgabe.find("kein_deckel") == std::string::npos,
               "K6: der vertippte Deckel laeuft NICHT als 'kein Deckel' weiter");
    }

    // == K7 / B-3: kaputtes Flag ist rc 2 -- unterscheidbar vom Deckel-Urteil rc 1 ===================
    pruefe_lauf(fahre(binary, "check-size --bogus"), 2, "unbekanntes Flag",
                "K7: --bogus => rc 2 (die Hilfe verspricht '2 kaputtes Flag')");

    // == K8/K9 / T-4-GEGENEINGAENGE am ECHTEN Profil: der Deckel urteilt in beide Richtungen =========
    // Sollwerte sind RICHTUNGEN, keine eingefrorenen Arena-Zahlen: 1000 B reisst jede reale Arena
    // (>= 1 Zeile a 32 B mal n_ops), 10^18 B haelt jede (die Ueberlauf-Wachen kappen frueher).
    pruefe_lauf(fahre(binary, "check-size " + profil + " --max-bytes=1000"), 1, "speicher=GERISSEN",
                "K8: Kontrolltest am bekannten Treffer -- 1000 B => GERISSEN, rc 1");
    pruefe_lauf(fahre(binary, "check-size " + profil + " --max-bytes=1000000000000000000"), 0, "speicher=haelt",
                "K9: 10^18 B => haelt, rc 0 (gueltige grosse Ganzzahl faellt nicht faelschlich als 2)");

    // == K10: die REIHENFOLGE ist festgenagelt -- kaputtes Flag schlaegt kaputtes Profil =============
    pruefe_lauf(fahre(binary, "check-size " + kein_profil + " --max-bytes=0.5"), 2, "GANZE Zahl",
                "K10: kaputtes Flag + kaputtes Profil => rc 2 (Flags werden VOR dem Profil geprueft)");

    // == K11: die VORZEICHEN-KLASSE -- nur die Nur-Ziffern-Wache faengt sie ==========================
    // strtoull selbst wickelt "-5" KOMMENTARLOS nach 2^64-5 (kein ERANGE, Ganz-String verbraucht):
    // ohne die Nur-Ziffern-Wache wuerde ein negativer Deckel zu einem riesigen "haelt"-Deckel. Die
    // Ganz-String-Wache kann diese Klasse NICHT fangen -- dieser Fall ist der einzige, der die
    // Nur-Ziffern-Wache isoliert beobachtbar macht (Mutations-Befund 09.08.2026: eine Mutation der
    // Ganz-String-Wache allein ist von der Nur-Ziffern-Wache verdeckt und hier ausdruecklich bekannt).
    pruefe_lauf(fahre(binary, "check-size --max-bytes=-5"), 2, "GANZE Zahl",
                "K11: --max-bytes=-5 => rc 2 (strtoull haette nach 2^64-5 gewickelt)");

    std::printf("%s: %d/%d Zusicherungen bestanden (Grundgesamtheit: alle Zusicherungen dieser Datei)\n",
                rot == 0 ? "GRUEN" : "ROT", gesamt - rot, gesamt);
    return rot == 0 ? 0 : 1;
}
