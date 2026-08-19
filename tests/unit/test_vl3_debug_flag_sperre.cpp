// test_vl3_debug_flag_sperre.cpp -- VL-3 (#15-Vorlauf): die PROZESS-PROBE des --debug-FLAGS der
// Planer-Shell und seiner ANWENDER-SPERRE am echten Kommando.
// (Die compile-time-Haelfte der Sache liegt im Header -- s. Block vor main().)
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

// == WO DIE COMPILE-TIME-HAELFTE GEPRUEFT WIRD -- und warum NICHT hier ==============================
// Die AdmissionStatus-Umhaengung (Ledger-Bauliste 4b) ist durch static_asserts IM HEADER gedeckt
// (measurement/axis_error.hpp, direkt bei debug_flag_admission): beide Richtungen plus die
// Etiketten-Naht auf der Zugelassen-Seite.
//
// Eine frueherere Fassung dieser Datei hat genau diese drei Asserts WOERTLICH wiederholt, mit der
// Begruendung, sie "greifen auch dann, wenn der Prozess-Teil mangels Binary gar nicht erst laeuft".
// Diese Begruendung traegt NICHT, und das ist am Objekt nachgemessen: axis_error.hpp enthaelt ausser
// `#pragma once` keinen einzigen #if-Guard, die Header-Asserts sind also in JEDER uebersetzten
// Einheit unbedingt aktiv -- auch in dieser. Die Duplikate konnten nur brechen, wenn die Originale
// schon gebrochen waren; sie hatten null eigene Erkennungskraft und sind entfernt.
// Damit ist dieser Test, was er der Sache nach ist: die PROZESS-Probe der Sperre am echten Kommando.

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
    pruefe_lauf(fahre(binary, "--debug version"), 8, kFehlerklasse, "K5c: Freigabe=\"\" (leer gesetzt) => gesperrt");
    // K5d ZEMENTIERT die Marge, die der Vergleich TATSAECHLICH hat -- sie beisst heute nicht rot,
    // sondern haelt eine bestehende Eigenschaft fest: env_trimmed() trimmt (planner_cli_env.hpp), der
    // Vergleich sieht also " true " als "true". Das ist das Haus-Idiom (COMDARE_BESTANDSLOG ebenso) und
    // in CI erwuenscht -- ein YAML-Blockskalar oder $(cat datei) haengt sonst ein Newline an und das
    // Gate ginge unerklaerlich zu. Ohne diese Zusicherung waere die Marge nur ein Implementierungs-
    // Detail, das ein spaeterer Wechsel auf getenv() still entfernen koennte.
    gate_setzen(" true ");
    pruefe_lauf(fahre(binary, "--debug version"), 0, "[debug] AKTIV",
                "K5d: Freigabe=\" true \" wird getrimmt und ZUGELASSEN (Haus-Idiom, bewusst)");

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

    // == K10: --debug NIMMT KEINEN WERT -- und zwar an JEDER Position gleich laut ====================
    // Das Flag ist ein Schalter, kein Wert-Paar. Vor dem Fix zerfiel die '='-Schreibweise in ZWEI
    // Verhalten, je nach Stellung (am Objekt gemessen, mit Gate=true):
    //   "version --debug=true"      -> rc 0, KEIN [debug]-Vermerk, KEINE Meldung  == still geschluckt
    //   "cache-key|fingerprint|validate --debug=true" -> ebenso rc 0 und stumm
    //   "--debug=true version"      -> rc 1, aber "unbekanntes Subkommando '--debug=true'"
    // Die stille Haelfte ist die gefaehrliche: wer `--debug=true` schreibt, glaubt im Debug-Modus zu
    // fahren, faehrt aber im normalen -- und bekommt spaeter Zahlen, die er fuer Debug-Ausschuss haelt
    // (oder umgekehrt). Die laute Haelfte war zwar nicht still, nannte aber die falsche Ursache.
    // Beide Positionen muessen dieselbe, zutreffende Diagnose liefern.
    char const* const kWertNadel = "--debug nimmt keinen Wert";
    gate_setzen("true");
    {
        auto const l = fahre(binary, "version --debug=true");
        pruefe_lauf(l, 1, kWertNadel, "K10: 'version --debug=true' => rc 1 mit Wert-Diagnose");
        pruefe(l.ausgabe.find("[debug] AKTIV") == std::string::npos,
               "K10b: der abgewiesene Lauf schaltet KEINEN Debug-Modus ein");
        pruefe(l.ausgabe.find(kVersionsNadel) == std::string::npos,
               "K10c: der abgewiesene Lauf fuehrt das Subkommando NICHT aus");
    }
    pruefe_lauf(fahre(binary, "--debug=true version"), 1, kWertNadel,
                "K10d: dieselbe Diagnose auch VOR dem Subkommando (nicht 'unbekanntes Subkommando')");
    pruefe_lauf(fahre(binary, "validate " + profil + " --debug=false"), 1, kWertNadel,
                "K10e: auch --debug=false wird abgewiesen (der Schalter kennt keinen Wert, auch keinen "
                "harmlosen)");
    // Und die Sperre bleibt vorrangig: ohne Gate ist die Zulassung das erste Urteil, nicht die Form.
    gate_setzen(nullptr);
    pruefe_lauf(fahre(binary, "version --debug"), 8, kFehlerklasse,
                "K10f: das WOHLGEFORMTE Flag faellt weiterhin in die Sperre (rc 8, nicht rc 1)");
    gate_setzen("true");

    // == K11: '--' BEENDET DIE OPTIONEN -- danach ist --debug ein WORT, kein Schalter ===============
    // Die Extraktion greift jedes Vorkommen ab Position 1. Heute ist keine Wert-Option davon
    // betroffen (alle Planer-Optionen sind '='-gefuegt, die Positionals sind Profil-Pfade), die
    // Unversehrtheit von Wert-Argumenten ruht damit allein auf dieser Eigenschaft der CLI-Flaeche --
    // nicht auf einer Regel im Code. Die erste leerzeichen-getrennte Wert-Option wuerde sie brechen,
    // ohne dass irgendetwas laut wird. '--' macht die Regel explizit und ist zugleich die einzige
    // Stelle, an der sie heute schon beobachtbar ist: vor dem Fix schaltete `validate -- --debug`
    // den Debug-Modus EIN und verschluckte das '--' obendrein.
    gate_setzen("true");
    {
        auto const l = fahre(binary, "validate " + profil + " -- --debug");
        pruefe(l.ausgabe.find("[debug] AKTIV") == std::string::npos,
               "K11: nach '--' ist --debug ein Wort -- der Debug-Modus bleibt AUS");
    }
    // Gegenprobe: OHNE Terminator wirkt dasselbe Flag unveraendert (die Regel schaltet nichts ab).
    {
        auto const l = fahre(binary, "validate " + profil + " --debug");
        pruefe(l.ausgabe.find("[debug] AKTIV") != std::string::npos,
               "K11b: ohne '--' wirkt --debug weiterhin (der Terminator verengt nur, was hinter ihm steht)");
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
        pruefe(l.ausgabe.find("--debug") != std::string::npos, "K9: 'help' nennt das --debug-Flag");
        // Die Nadel ist der LISTEN-EINTRAG, nicht der blosse Teilstring "8 ". Erste Fassung war
        // `find("8 ")` -- und die wurde bereits von der Fliesstext-Zeile "... mit Exit 8 abgewiesen"
        // erfuellt, die derselbe Commit hinzufuegt. Loeschte jemand den Eintrag aus der Exit-Code-Liste,
        // blieb die Zusicherung GRUEN: eine Wache, die nie beisst (V-2) -- genau die Klasse, die diese
        // Datei ein paar Zeilen weiter oben (kVersionsNadel) schon einmal selbst dokumentiert hat.
        // T-1 dazu gefahren: Listen-Eintrag entfernt -> mit "8 " GRUEN, mit dieser Nadel ROT.
        pruefe(l.ausgabe.find("| 8 --debug-Zulassung gesperrt") != std::string::npos,
               "K9b: 'help' fuehrt Exit 8 als EINTRAG der Exit-Code-Liste (nicht bloss im Fliesstext)");
        // K9c GEGENPROBE zur Codeset-Behauptung des Datei-Kopfes: 7 ist im Haus das Lane-Fehlrouting
        // der CEB und darf in DIESER Binary nicht als eigener Code auftauchen. Ohne diese Zusicherung
        // ist "8 statt 7, weil 7 vergeben ist" nur eine Kommentar-Behauptung. Geprueft wird die
        // Listen-Form "| 7 " -- der Fliesstext nennt "Exit 7" bewusst (Rollen-Trennung) und bleibt
        // damit erlaubt.
        pruefe(l.ausgabe.find("| 7 ") == std::string::npos,
               "K9c: 'help' fuehrt KEINEN Exit-Code 7 in der Liste (der gehoert der CEB)");
        // K9d: die Hilfe darf nicht SCHAERFER sein als die Implementierung. Ihre erste Fassung sagte
        // "exakt dieser Wert; alles andere bleibt gesperrt" -- der Eingang ist aber vorgetrimmt (K5d),
        // also war die Zusage um genau diese Marge zu stark. Eine Doku, die mehr verspricht als der
        // Code haelt, ist dieselbe Klasse Fehler wie eine Wache, die nicht beisst: beide erzeugen
        // Vertrauen ohne Deckung.
        pruefe(l.ausgabe.find("umgebende Leerzeichen werden ignoriert") != std::string::npos,
               "K9d: 'help' nennt die Trim-Marge des Freigabe-Vergleichs");
    }

    std::printf("%s: %d/%d Zusicherungen bestanden (Grundgesamtheit: alle Zusicherungen dieser Datei)\n",
                rot == 0 ? "GRUEN" : "ROT", gesamt - rot, gesamt);
    return rot == 0 ? 0 : 1;
}
