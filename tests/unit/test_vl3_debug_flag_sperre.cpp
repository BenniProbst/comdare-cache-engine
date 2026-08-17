// test_vl3_debug_flag_sperre.cpp -- VL-3 (#15-Vorlauf): das --debug-FLAG der Planer-Shell und seine
// ANWENDER-SPERRE, plus die AdmissionStatus-Umhaengung, die diese Sperre traegt.
//
// == WARUM DIESE SACHE UEBERHAUPT EXISTIERT (Ledger 09.08.2026 spaet, geltende work_mode-Fassung) =====
// Owner verbatim: "Damit ist Debug auch eher als Flag entkoppelt, als dass es als State gleichrangig
// einsortiert wird ... Es ist also ein CLI-Flag auf der Planer-Shell" -- und, im selben Abschluss:
// "Debug ist fuer normale Benutzer gesperrt."
//
// Die Sperre ist eine INTEGRITAETSREGEL, keine Bequemlichkeit (Ledger, Abschnitt "WARUM debug GESPERRT
// GEHOERT"): debug ist der einzige work_mode, der MISST und dabei PARALLEL laeuft. Parallel gemessene
// Latenzen sind nicht run-to-run-stabil -- genau die Stabilitaet, die `measure` zusichert.
//   "Ein Anwender, der debug waehlen koennte, bekaeme Zahlen, die wie Messwerte AUSSEHEN und keine
//    sind. Das ist die unheilbare Klasse: kontaminierte Daten."
//
// == WARUM DER TRAEGER VOR DEM ENUM-BRUCH KOMMT (Bau-Reihenfolge gruppe2 (c) Schritt 1) ===============
// Im #15-Bruch verlaesst `Debug` das RunMethodology-Enum (B-5d). Der EINZIGE debug-Token in XML
// repo-weit ist m3_smoke_coverage.profile.xml:172 <method value="debug"/>, gegatet von
// test_smoke_coverage_profile (tests/unit/CMakeLists.txt:3784-3791). Faellt der Enum-Wert, BEVOR das
// Flag existiert, verliert dieser Token seinen Traeger ERSATZLOS. Dieser Test baut den Traeger vor;
// das Enum, die Registry und m3_smoke:172 bleiben in diesem Change UNBERUEHRT (B-5d-Materie).
//
// == T-1 / DIE ROTE STUFE (gemessen, nicht behauptet) ================================================
// Vor dem Bau existierte `--debug` im ganzen Baum NICHT: `grep -rn -- '"--debug"'` ueber *.cpp/*.hpp/
// *.cmake/*.txt/*.sh/*.yml (ohne build/) = 0 Treffer, Gegenprobe `--check-size` = 8 Treffer (das
// Werkzeug sucht). Der Dispatcher fiel damit in unbekanntes_subkommando() -> rc 1 statt der hier
// verlangten rc 8; die Sperr-Diagnose fehlte vollstaendig. Beide roten Messungen stehen im
// Bau-Bericht.
//
// == T-3 / T-5: WOHER DIE SOLLWERTE STAMMEN =========================================================
// Die Soll-Exit-Codes stammen aus der HILFE des Kommandos ('comdare-experiment-planner help' ->
// Abschnitt "Exit-Codes") -- einer anderen Quelle als der geprueften Code-Pfad. rc 8 ist NEU und
// bewusst eigen: 0/1/2/5/6 sind belegt, und 7 ist im Haus als "Lane-Fehlrouting" des CEB vergeben
// (main.cpp-Hilfetext, Rollen-Trennung) -- eine Wiederverwendung waere eine Vokabel-Kollision.
// Eine Zulassungs-Sperre muss in der CI von einem Usage-Fehler UNTERSCHEIDBAR sein, sonst ist der
// Koeder-Beweis nicht fuehrbar.
//
// == T-4: ZU JEDER ABLEHNUNG EIN GEGENEINGANG =======================================================
// K2/K6/K7 lassen dieselbe Stelle DURCH (Gate gesetzt -> das Subkommando laeuft); K4 zeigt, dass das
// Gate ALLEIN nichts einschaltet. Eine Wache, die immer rot ist, ist so wertlos wie eine, die nie
// beisst (V-2).
//
// ARGUMENTE: argv[1] = Pfad der comdare-experiment-planner-Binary ($<TARGET_FILE:...>); argv[2] =
// Pfad eines GUELTIGEN Profils (aus der CMakeLists gereicht, nicht aus der Umgebung geraten -- der
// Test ist hermetisch gegen COMDARE_THESIS_PROFILE). Muster: test_check_size_cli_deckel.cpp.

#include <cache_engine/measurement/axis_error.hpp>

#include <cstdio>
#include <cstdlib>
#include <string>

#if defined(_WIN32)
#define COMDARE_POPEN  _popen
#define COMDARE_PCLOSE _pclose
#else
#include <sys/wait.h>
#define COMDARE_POPEN  popen
#define COMDARE_PCLOSE pclose
#endif

namespace cem = ::comdare::cache_engine::measurement;

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
    std::string ausgabe; ///< stdout UND stderr (2>&1), damit die Sperr-Diagnose pruefbar ist
};

/// Faehrt die Binary mit den gegebenen Argumenten; die Argumente sind fest verdrahtete ASCII-Woerter
/// ohne Shell-Metazeichen, nur der Binary-/Profil-Pfad wird gequotet.
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

/// Das Freigabe-Gate im Kind-Prozess setzen bzw. entfernen. popen erbt die Umgebung des Testlaufs,
/// also ist setenv/unsetenv HIER die Steuerung des Kindes -- kein VAR=wert-Praefix in der Kommando-
/// zeile, das sich mit dem Quoting des Binary-Pfades beissen wuerde.
void gate_setzen(char const* wert) {
#if defined(_WIN32)
    std::string const zuweisung = std::string{"COMDARE_DEBUG_FREIGABE="} + (wert == nullptr ? "" : wert);
    ::_putenv(zuweisung.c_str());
#else
    if (wert == nullptr)
        ::unsetenv("COMDARE_DEBUG_FREIGABE");
    else
        ::setenv("COMDARE_DEBUG_FREIGABE", wert, 1);
#endif
}

} // namespace

// == TEIL 1: die AdmissionStatus-UMHAENGUNG (Ledger-Bauliste Punkt 4b) ===============================
// "Die Zulassung haengt am FLAG, nicht am State. AdmissionStatus {Zugelassen, Gesperrt} bleibt die
// Form, aber sie bewertet, OB --debug gesetzt werden darf ... Der fail-closed-Default (Unbekannt ->
// gesperrt) traegt unveraendert."
//
// Diese Zusicherungen sind compile-time: faellt die Umhaengung weg oder dreht jemand ihre Richtung um,
// bricht der Bau LAUT (Haus-Doktrin "erst laute Compile-Fehler"), statt still eine Freigabe zu
// verschenken. Sie stehen ausserhalb von main(), damit sie auch dann greifen, wenn der Prozess-Teil
// mangels Binary gar nicht erst laeuft.
static_assert(cem::debug_flag_admission(false) == cem::AdmissionStatus::Gesperrt,
              "fail-closed: ohne Freigabe-Kontext ist --debug GESPERRT (Ledger 4b)");
static_assert(cem::debug_flag_admission(true) == cem::AdmissionStatus::Zugelassen,
              "mit Freigabe-Kontext ist --debug zugelassen -- sonst waere die Wache konstant rot (V-2)");
// Die Zell-Token-Naht bleibt dieselbe wie bei der Perm-Zulassung: die Umhaengung erfindet KEIN zweites
// Vokabular (W-4-Doktrin -- eine Form, zwei Gegenstaende, aber nur EIN Etikettensatz).
static_assert(cem::admission_status_token(cem::debug_flag_admission(false)) == std::string_view{"gesperrt"},
              "die Umhaengung rendert durch admission_status_token, nicht durch ein eigenes Etikett");

int main(int argc, char* argv[]) {
    std::printf("test_vl3_debug_flag_sperre -- das --debug-Flag der Planer-Shell + seine Anwender-Sperre\n");
    if (argc < 3) {
        std::printf("  ROT : Aufruf braucht argv[1]=Planer-Binary argv[2]=gueltiges Profil (aus CMake)\n");
        return 1;
    }
    std::string const binary = argv[1];
    std::string const profil = argv[2];

    // Die Sperr-Diagnose, WOERTLICH. Sie ist der Beweis, dass die Abweisung aus DIESER Regel kam und
    // nicht aus irgendeinem anderen Fehler -- ohne das Literal waere der Koeder nicht fuehrbar.
    char const* const kFehlerklasse = "fehlerklasse=debug_zulassung_gesperrt";
    // Die Nadel fuer "das Subkommando ist WIRKLICH gelaufen": der Stempel-Praefix von `version`, am
    // Objekt gemessen ("planner@1.0.0.c isa=x86_64 os=linux"), NICHT geraten. Eine Nadel, die im Bestand
    // gar nicht vorkommt, macht die Positiv-Zusicherung unerfuellbar und die Negativ-Zusicherung
    // trivial wahr -- beides waren Wachen, die nie beissen (V-2). Genau das ist hier beim ersten
    // gruenen Lauf aufgefallen und korrigiert worden.
    char const* const kVersionsNadel = "planner@";

    // == TEIL 2: DER KOEDER -- --debug aus dem ANWENDER-Kontext MUSS abgewiesen werden ===============
    gate_setzen(nullptr); // Anwender: kein Freigabe-Kontext
    {
        auto const l = fahre(binary, "--debug version");
        pruefe_lauf(l, 8, kFehlerklasse, "K1 KOEDER: --debug ohne Freigabe => rc 8 mit Sperr-Diagnose");
        // Der Kern der Integritaetsregel: das Subkommando darf NICHT gelaufen sein. `version` druckt
        // sonst seinen Stempel -- steht der da, hat die Sperre gemeldet und trotzdem durchgelassen.
        pruefe(l.ausgabe.find(kVersionsNadel) == std::string::npos,
               "K1b: der gesperrte Lauf fuehrt das Subkommando NICHT aus (keine version-Ausgabe)");
    }

    // == K2 / T-4-GEGENEINGANG: mit Freigabe laeuft dasselbe Kommando durch =========================
    gate_setzen("true");
    {
        auto const l = fahre(binary, "--debug version");
        pruefe_lauf(l, 0, kVersionsNadel, "K2: --debug MIT Freigabe => rc 0, das Subkommando laeuft");
        pruefe(l.ausgabe.find(kFehlerklasse) == std::string::npos,
               "K2b: der freigegebene Lauf traegt KEINE Sperr-Diagnose");
    }

    // == K3: der freigegebene Lauf ist SICHTBAR, nicht still ========================================
    // Ledger: die Zahlen eines Debug-Laufs sind ausdruecklich Ausschuss ("egal wie genau"). Ein Lauf,
    // der unbemerkt im Debug-Modus faehrt, ist genau der Weg, auf dem so eine Zahl spaeter fuer einen
    // Messwert gehalten wird. Der Vermerk gehoert darum auf stderr, nicht ins Schweigen.
    pruefe_lauf(fahre(binary, "--debug version"), 0, "[debug] AKTIV",
                "K3: der zugelassene Debug-Lauf vermerkt sich sichtbar auf stderr");

    // == K4 / GEGENPROBE: das Gate ALLEIN schaltet nichts ein =======================================
    // Sonst waere jeder CI-Lauf still ein Debug-Lauf, sobald die Variable irgendwo gesetzt ist.
    {
        auto const l = fahre(binary, "version");
        pruefe_lauf(l, 0, kVersionsNadel, "K4: Gate=true ohne --debug => normaler Lauf, rc 0");
        pruefe(l.ausgabe.find("[debug] AKTIV") == std::string::npos,
               "K4b: ohne das Flag entsteht KEIN Debug-Vermerk (das Gate ist Erlaubnis, kein Schalter)");
    }

    // == K5 / FAIL-CLOSED: alles ausser dem exakten "true" ist GESPERRT ==============================
    // Ledger: "Unbekannt -> gesperrt (sicherer Default)". Das Haus-Idiom ist der exakte Vergleich
    // gegen "true" (main.cpp:92 env_trimmed("COMDARE_BESTANDSLOG") != "true"); jede Aufweichung hier
    // waere eine zweite, abweichende Gate-Semantik im selben Programm.
    gate_setzen("1");
    pruefe_lauf(fahre(binary, "--debug version"), 8, kFehlerklasse,
                "K5: Freigabe=\"1\" ist NICHT \"true\" => gesperrt (fail-closed)");
    gate_setzen("TRUE");
    pruefe_lauf(fahre(binary, "--debug version"), 8, kFehlerklasse,
                "K5b: Freigabe=\"TRUE\" (Grossschrift) => gesperrt -- kein stilles Umbiegen");
    gate_setzen("");
    pruefe_lauf(fahre(binary, "--debug version"), 8, kFehlerklasse,
                "K5c: Freigabe=\"\" (leer gesetzt) => gesperrt");

    // == K6 / ORTHOGONALITAET: --debug ist mit JEDEM Subkommando kombinierbar ========================
    // Ledger: "orthogonal zu allen vier States und mit jedem kombinierbar". Als fuenfter Enum-Wert
    // waere genau diese Kombination unausdrueckbar gewesen -- deshalb ist sie hier eine Zusicherung
    // und kein Nebenbefund.
    gate_setzen("true");
    pruefe_lauf(fahre(binary, "--debug validate " + profil), 0, nullptr,
                "K6: --debug validate <profil> => das Subkommando laeuft (rc 0)");
    // Und die Stellung im Kommando ist frei: das Flag steht auch HINTER dem Subkommando.
    pruefe_lauf(fahre(binary, "validate " + profil + " --debug"), 0, nullptr,
                "K6b: --debug hinter dem Subkommando wirkt genauso (Flag, keine Position)");
    // K6c/K6d: DIE RISKANTEN SUBKOMMANDOS. `status` und `check-size` fuehren EIGENE Flag-Schleifen, die
    // jedes unbekannte '-'-Argument ABLEHNEN (status -> rc 1 "unbekanntes Flag", check-size -> rc 2).
    // Genau daran zerbraeche eine Auswertung, die --debug erst im Dispatch entfernt: das Flag erreichte
    // die Sub-Schleife und wuerde dort als Tippfehler abgewiesen. Diese beiden Zusicherungen sind der
    // Grund, warum die Extraktion VOR dem Dispatch steht -- ohne sie ist die Orthogonalitaet nur an
    // `version`/`validate` belegt, also an genau den zwei Kommandos ohne eigene Flag-Schleife.
    {
        auto const l = fahre(binary, "--debug status " + profil);
        pruefe_lauf(l, 0, nullptr, "K6c: --debug status => rc 0 (die status-Flag-Schleife sieht es nie)");
        pruefe(l.ausgabe.find("unbekanntes Flag") == std::string::npos,
               "K6c-b: status meldet KEIN unbekanntes Flag -- --debug ist vorher verbraucht");
    }
    {
        // Der Deckel ist absichtlich riesig, damit das Urteil "haelt" ist und rc 0 bleibt: gemessen wird
        // hier die Flag-Schleife, nicht der Deckel (dessen eigener Test ist test_check_size_cli_deckel).
        auto const l = fahre(binary, "--debug check-size " + profil + " --max-bytes=1000000000000000000");
        pruefe_lauf(l, 0, nullptr, "K6d: --debug check-size => rc 0, nicht rc 2 (kaputtes Flag)");
        pruefe(l.ausgabe.find("unbekanntes Flag") == std::string::npos,
               "K6d-b: check-size meldet KEIN unbekanntes Flag");
    }

    // == K7 / REIHENFOLGE FESTGENAGELT: die Sperre schlaegt den Subkommando-Fehler ===================
    // Ohne diese Zusicherung koennte ein spaeterer Umbau die Sperre hinter den Dispatch schieben --
    // dann meldete ein gesperrter Lauf "unbekanntes Subkommando" und die Sperre waere unbeobachtbar.
    gate_setzen(nullptr);
    pruefe_lauf(fahre(binary, "--debug bogus-subkommando"), 8, kFehlerklasse,
                "K7: gesperrtes --debug + unbekanntes Subkommando => rc 8, nicht rc 1");
    // Gegenprobe zu K7: OHNE --debug ist derselbe Aufruf weiterhin der gewoehnliche Usage-Fehler.
    pruefe_lauf(fahre(binary, "bogus-subkommando"), 1, "unbekanntes Subkommando",
                "K7b: ohne --debug bleibt der Usage-Fehler rc 1 (die Sperre verdraengt ihn nicht)");

    // == K8: die Sperre greift auch dort, wo sie "harmlos" aussieht ==================================
    // help/version sind lesend -- aber eine Zulassungsregel mit Ausnahmenliste ist keine Regel mehr,
    // sondern eine Liste, die beim naechsten Subkommando vergessen wird.
    pruefe_lauf(fahre(binary, "--debug help"), 8, kFehlerklasse,
                "K8: auch 'help' laeuft mit gesperrtem --debug nicht (keine Ausnahmenliste)");

    // == K9: die HILFE nennt Flag und Sperre -- sonst ist die Regel nur im Code =======================
    // T-3-Quelle: derselbe Hilfetext, aus dem die Soll-Exit-Codes dieses Tests stammen.
    {
        auto const l = fahre(binary, "help");
        pruefe(l.ausgabe.find("--debug") != std::string::npos,
               "K9: 'help' nennt das --debug-Flag");
        pruefe(l.ausgabe.find("8 ") != std::string::npos,
               "K9b: 'help' fuehrt den neuen Exit-Code 8 in der Exit-Code-Liste");
    }

    std::printf("%s: %d/%d Zusicherungen bestanden (Grundgesamtheit: alle Zusicherungen dieser Datei)\n",
                rot == 0 ? "GRUEN" : "ROT", gesamt - rot, gesamt);
    return rot == 0 ? 0 : 1;
}
