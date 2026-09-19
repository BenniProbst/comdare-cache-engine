// test_pa1_tote_ausnahme -- ERLOSCHEN hat ZWEI Richtungen.                 (2026-08-10)
// =============================================================================
// PRUEFLING: scripts/ci_test_registrierungs_wache.sh
//
// DER BEFUND, GEGEN DEN DIESE DATEI GEBAUT IST
//
// Die Wache definierte ERLOSCHEN in EINER Richtung: "existiert der Gegenstand WIEDER,
// traegt die Ausnahme nicht mehr". Das faengt den Fall, dass der Gegenstand zurueck-
// kehrt. Es faengt NICHT den Fall, dass er NIE zurueckkehren kann -- und genau der lag
// vor: alle vier prt-art-Zeilen der Allowlist banden an
//     datei:prt_art/include/prt_art/prt_art.hpp
// einen Pfad, den nichts mehr erzeugen kann. Am Objekt gemessen (2026-08-10):
//   prt_art.hpp .................. 0 Treffer unter /home/comdare
//   Gegenprobe cache_engine.hpp .. 45 Treffer -- das Werkzeug sucht, der Nullbefund gilt
//   Historie ..................... in ce vom 2026-05-12 (5c210581) bis 2026-05-14
//                                  getrackt, dann per 'git mv' nach
//                                  libs/deprecated/prt_art_legacy/ (b9abc720, R100),
//                                  dieser Baum am 2026-06-01 entfernt (6b3ed0d9).
//                                  Im Nachbarrepo comdare-prt-art NIE getrackt.
// FOLGE: die vier Zeilen konnten per Konstruktion nie erloeschen. Sie waren die
// Freistellung mit unbegrenzter Laufzeit, vor der der Kopf ihrer eigenen Datei warnt.
//
// DER ERSTLAUF, WOERTLICH (Rot zuerst, T-1). Gegen einen Bau-Baum, dem GENAU diese vier
// Dateien fehlten, meldete die Wache VOR der Heilung:
//     davon 4 begruendet, 0 mit ERLOSCHENER, 0 mit UNPRUEFBARER Begruendung, 0 ohne.
//     TEST-REGISTRIERUNGS-WACHE: OK (470 Quelldateien, 0 ohne Begruendung ausserhalb).
// Vier Ausnahmen ohne Ablauf, und die Wache sagte "OK".
//
// WAS DIE WACHE JETZT MISST -- und was das NICHT beweist:
// "Kann nie entstehen" ist nicht entscheidbar, solange man nur den eigenen Baum kennt.
// Gemessen wird deshalb etwas Engeres: KEINE QUELLE DIESES REPOS ERKLAERT DEN
// ZWEIG, IN DEM ER LIEGT -- weder der Git-Index (getrackte Datei ODER Submodul-Gitlink)
// noch eine .gitignore-Regel, und das bis hinauf zur Wurzel. Dann hat er in diesem
// Repository keinen Erzeuger.
// DAS BEWEIST NICHT, dass er nie existieren kann: jemand kann ihn morgen 'git add'en.
// Diese Datei prueft deshalb GENAU DIESE ENGERE AUSSAGE und keine groessere -- Fall (2),
// (3) und (4) belegen ausdruecklich, dass ein Pfad mit Erzeuger NICHT als tot gilt.
//
// WIE DIE VIER GRUEN-FAELLE MESSEN, und warum nicht anders (am Objekt gelernt): sie
// fragen NICHT, ob die Zeichenkette "TOTE AUSNAHME" in der Ausgabe fehlt. Der NENNER
// der Wache nennt die Klasse bei JEDEM Lauf ("... 0 TOTE AUSNAHME ..."), ein solches
// EXPECT_FALSE waere also immer verletzt -- beim ersten Lauf dieser Datei ist genau das
// passiert. Geprueft wird deshalb der ZAEHLER: "0 TOTE AUSNAHME" fuer gruen,
// "1 TOTE AUSNAHME" fuer den Biss. Dieselbe Klasse wie '/build' gegen '/builds': eine
// Teilzeichenkette ist kein Feld.
//
// WARUM ROT UND KEINE EIGENE WARNKLASSE (begruendete Entscheidung, PA-1): eine Warnung
// reproduziert exakt die Fehlerklasse -- an genau so einer Stelle sind vier Dateien 14
// bzw. 68 Tage unbemerkt geblieben. Die Hausregel kennt ZELLE=Warnung, JOB=hart rot.
// Der Preis ist ein moeglicher Fehlalarm; er ist billig zu beheben (erreichbaren
// Gegenstand nennen oder 'frist:' setzen) und faellt sofort auf, waehrend das Gegenteil
// -- eine tote Zeile -- per Konstruktion nie auffaellt.
//
// WARUM EIN GOOGLE TEST UND KEINE WEITERE SHELL-PROBE (Owner-KERN 09.08.2026):
//   "Ich sehe einen Haufen shells statt vernuenftiger google tests, was soll das? ...
//    SKRIPTE SAGEN GAR NICHTS."
// Debug UND Release, ueber die normale ctest-Registrierung.
//
// K13 BEIDSEITIG: jeder Fall wuerfelt frisch aus /dev/urandom. Fall (1) verlangt, dass
// ein unmoeglicher Pfad BEISST; Fall (2) und (3) verlangen, dass ein moeglicher Pfad
// NICHT beisst. Ein Orakel, das immer ROT liefert, faellt an (2)/(3); eines, das immer
// GRUEN liefert, faellt an (1).
//
// GRENZE, EHRLICH BENANNT (T-9): Fall (8) faehrt die ECHTE Allowlist, laesst dabei aber
// die 'isa:'-Zeile ausdruecklich AUS dem Pruefsatz -- ihre Auswertung braucht den
// CMakeCache des gemessenen Baums, und ein kuenstlich erzeugtes Fehlen wuerde sie auf
// einem AVX-512-Host als ERLOSCHEN melden, obwohl sie im echten Baum nie gefragt wird.
// Geprueft sind dort also die vier 'frist:'-Zeilen; NICHT geprueft ist die 'isa:'-Zeile.
// Beide Mengen stehen in der Ausgabe des Falls.
//
// NACHTRAG (2026-09-17, Owner-Order 206 "OV-2 Archiv-Variante gilt und frist Zeilen entfernen"):
// der Satz "geprueft sind dort die vier 'frist:'-Zeilen" ist UEBERHOLT -- die vier Zeilen sind aus
// der Allowlist entfernt. An ihre Stelle tritt die ARCHIV-Klasse der Wache: eine getrackte
// Test-Quelldatei unter tests/deprecated/<ordner>/ zaehlt nicht zum SOLL, wenn und nur wenn
// tests/deprecated/<ordner>/VERMERK.md im Index liegt UND die Form eines Ankers hat (regulaeres Blob
// 100644/100755 auf Index-Stufe 0 mit Nicht-Leerraum-Inhalt; NACHTRAG 3). Fall (8) misst ab jetzt GENAU DAS am echten
// Repo (die vier fehlen im Bauweg und muessen trotzdem gruen sein, weil sie ARCHIV sind), Fall (10)
// die Regel selbst an einem Wegwerf-Repo -- beidseitig, samt Nachbar-Anker-Probe.
// UNVERAENDERT: die 'frist:'-ART der Wache bleibt in Gebrauch und wird von Fall (7) beidseitig
// gefahren; sie hat nach diesem Entscheid nur keinen Gegenstand mehr in der committeten Allowlist.
//
// NACHTRAG 2 (2026-09-18, Lens-Funde r1 des OV-2-Zuges: Lens A LA-04, Lens B LB-01/LB-02/LB-03/LB-05):
// Fall (10) traegt jetzt die Stufen (d)/(e) -- eine Allowlist-Zeile fuer eine ARCHIV-Datei ist eine
// Ausnahme OHNE ANLASS und ROT; die Faelle (11a-c) geben den drei im Wachen-Kopf behaupteten Grenzen
// der Archiv-Regel je einen Traeger; Fall (12) verlangt einen Anker mit Inhalt und Form (0 Byte oder
// Symlink ankern nicht); Fall (13) faengt die Schwesterklasse "Allowlist-Zeile ohne Gegenstand im
// SOLL-Bestand". Alle Archiv-Nenner sind ab hier FELD-verankert gepinnt (", 4 archiviert)" mit Komma davor
// und Klammer danach statt "4 archiviert" -- Teilzeichenkette von "14 archiviert", Klasse s. oben).
//
// NACHTRAG 3 (2026-09-18, Fix-r2 des OV-2-Zuges: Lens A r3, Lens B r2, Lens C r2): Fall (8) pinnt die
// Nenner-Zeilen der Wache mit den GEMESSENEN Zahlen (LCT-09); jeder Zaehler-Pin der Faelle (10d/e), (12),
// (13) und der neuen Faelle ist eine GANZE Nenner-Zeile (Helfer nenner_ohne_bauweg, nenner_davon,
// endzeile_rot, endzeile_ok; LCT-10/LCT-11, Klasse LB-03). Fall (12) verlangt zusaetzlich ein
// Nicht-Leerraum-Zeichen im Anker (F1) und traegt Modus 100755 als gruene Stufe (LCT-12); Fall (12b) haelt
// fest, dass ein Anker im MERGE-KONFLIKT (Index-Stufe 1-3) nicht ankert (LCW-01). Fall (13) heisst jetzt
// ...ImSollBestand... (das Etikett "ohne Gegenstand im Index" war falsch: eine Datei unter ext/ steht im
// Index, aber nicht im SOLL-Bestand; F2) und ueberspringt Leerraum-Zeilen (F3). Neue Faelle: (14) letzte
// Zeile ohne Zeilenumbruch (LCW-04), (15) Allowlist-Zeile fuer einen Pfad unter tests/deprecated/ (LCW-05),
// (16) doppelte Zeile je Pfad (LCW-08), (17) leeres Feld 3 (LCW-09), (18) Werkzeug-Ausfall in der
// Nenner-Pipeline = Exit 2 per PATH-Koeder (LCW-02/LCW-03). Rot zuerst je Stufe gegen die Wache d8e8f53d
// bzw. einen Mutanten -- Belege im Beweisort des OV-2-Zuges (FIX-r2.md).
//
// NACHTRAG 4 (2026-09-18, Fix-r3 des OV-2-Zuges: Lens C r3 LC3W-01..08): neue Faelle (19) leeres ISA-
// Teilmerkmal (LC3W-07), (20) ISA-Belege eindeutig (LC3W-08), (21) Werkzeug-Ausfall im ISA-Pfad per
// wc-Koeder an der ISA-Zwischendatei (LC3W-01; sed ist aus der Wache entfernt, der Koeder trifft das
// verbliebene PATH-Werkzeug), (22) grep-Status 2 = Exit 2 (LC3W-02; IST-Datei unlesbar per chmod 000
// oder procfs), (23) git-Fehler 128 in der Erreichbarkeits-Probe (LC3W-03), (24) der Anker muss ein
// Blob in der Objektdatenbank sein (LC3W-06; cat-file-Fehler = Exit 2, LC3W-03), (25) date-Ausfall
// (LC3W-04), (26) Formfehler der stummen Zeile einer Datei im Bauweg (LC3W-05 -- die Test-Auflage der
// Triage zeigte Exit 0 gegen 806629ca, der Fund war echt). Die Nenner-Zeile "dazu UNPRUEFBAR ..." traegt
// ein sechstes Feld ("N mit Formfehler fuer Dateien im Bauweg"); nenner_ohne_bauweg() hat dafuer einen
// sechsten Parameter mit Vorgabe 0, alle aelteren Pins bleiben ganze Zeilen. Rot zuerst je Stufe gegen
// die Wache 806629ca (das neue Binary gegen die alte Wache per COMDARE_PA1_WACHE_PFAD) -- FIX-r3.md.
//
// NACHTRAG 5 (2026-09-19, Fix-r4 des OV-2-Zuges: Fund N-1 der Fix-r3-Berichtsfassung, Lead-Objektprobe K205):
// Fall (27) -- ein VERZEICHNIS ist kein Gitlink, auch wenn 'git ls-files -s' ueber das Verzeichnis mit einem
// Gitlink-Eintrag beginnt (ist_gitlink prueft jetzt Modus 160000 UND das exakte Pfadfeld je Zeile). Beidseitig:
// Tiefe 2 unter dem Verzeichnis ist TOT, unter dem echten Gitlink bleibt es erreichbar. Rot zuerst gegen die
// Wache 63f8abd4 (per COMDARE_PA1_WACHE_PFAD) -- FIX-r4.md.
//
// NACHTRAG 6 (2026-09-19, Fix-r5 des OV-2-Zuges: Lens A r5 LA5-01/02/03/06, Lens B r4 LB4-01/02/03/I1, Lead-
// Entscheide O-12/O-14): Fall (27) traegt die Stufen (d) Symlink an Stelle des Gitlinks -> UNPRUEFBAR (die
// Modus-Haelfte des Vergleichs; Mutant M1 ohne Modus ueberlebte 30/30), (e) Gitlink nur auf Index-Stufe 1 ->
// erreichbar (O-12 Teil 1; Mutant M7 'nur Stufe 0' ueberlebte 30/30), (f) Typ-Konflikt Datei gegen Gitlink je
// Stufe -> UNPRUEFBAR (O-12 Teil 2, wie der Anker im Konflikt). Fall (28): eine getrackte DATEI als Vorfahr ist
// kein Verzeichnis -- der Gegenstand darunter ist TOT (gegen c62cfc7e Exit 0: fail-open seit 806629ca, am
// echten Objekt Koeder 'ext/queuing/REPOS_OVERVIEW.md/x.hpp'). Fall (29): ein 'datei:'-Pfad mit quotierbarem
// Zeichen (Tabulator, Backslash, Anfuehrungszeichen) oder ohne kanonische Form ('./', '//', '/' am Ende, Segment
// '.') ist UNPRUEFBAR, auch als stumme Zeile; ein Nicht-ASCII-Gitlink-Pfad bleibt erreichbar (die Wache liest
// den Index mit core.quotePath=false). Die Marke-Pins der Faelle (2) und (27b) fordern 'Koeder <marke>' -- Feld 3
// der GELESENEN Zeile -- statt der blossen Marke, die als Pfadname in jeder Ausgabe der Fixture steht (Lens B
// LB4-02: der Pin war vakuoes). Rot zuerst je Stufe gegen die Wache c62cfc7e bzw. die Mutanten M1/M7 und einen
// Mutanten ohne Feld-3-Ausgabe (fuer die Marke-Pins) -- FIX-r5.md.
//
// NACHTRAG 7 (2026-09-19, Fix-r6 des OV-2-Zuges: Lens A r6 LA6-01..06, Lens B r5 LB5-I3, Lens C r4 LC3T-01..09
// und LC3W-15..18): PINS ALS GANZE ZEILEN (LC3T-04) -- zeile_exakt() und zeile_beginnt() zerlegen die Ausgabe an
// '\n'; enthaelt() bleibt fuer Fragmente mitten in einer Zeile und fuer EXPECT_FALSE. Die Zaehler-Fragmente
// "0 TOTE AUSNAHME" / "1 TOTE AUSNAHME" der Faelle (1)-(6) und (8) sind durch nenner_davon() ersetzt (LC3T-03:
// "1 TOTE AUSNAHME" ist Teilzeichenkette von "11 TOTE AUSNAHME"); Fall (5) pinnt die AUSSERHALB-Zeile des
// Nenners mit Zahl und die begruendete Zeile selbst (LC3T-01: das Wort stand bei jedem Lauf im Nenner), Fall (6)
// die ERLOSCHEN-Zeile (LC3T-02: das Wort traf immer "0 mit ERLOSCHENER"); die Archiv-Stufen (10) und (11) pinnen
// Nenner und Endzeile ganz (LC3T-05); Arrangement-ASSERTs sind exakte Index-Zeilen, (12b)/(28e) assertieren
// Stufe 2, (27f) schliesst Stufe 0 aus (LC3T-08); Fall (8) sichert seinen Wegwerf-Baum per RAII-Waechter und
// prueft die Loeschung (LC3T-09). NEUE STUFEN: (6b) Symlink ohne Ziel am Gegenstand = ERLOSCHEN (LA6-06);
// (27e2)/(27e3) Gitlink auf den Stufen 1/2/3 bzw. 1+3 (LC3T-08); (27f2) Typ-Konflikt in der anderen Reihenfolge,
// Gitlink Stufe 2 gegen Datei Stufe 3, und die Meldung nennt die Typen je Stufe (LC3T-07, LC3W-16); (27g)-(27g3)
// D/F-Konflikt, Eintrag gegen Verzeichnis darunter (LA6-01; gegen 9223cbd5 Exit 0 = fail-open); (27h) Symlink
// NUR im Arbeitsbaum als Ahne (LA6-03; gegen 9223cbd5 Exit 2); (28f) Datei-Ahne mit Modus 100755 (LC3T-06);
// Fall (31) Nicht-ASCII-Pfade im SOLL und am Anker (LA6-02 = LC3W-15; gegen 9223cbd5 still gruen bzw. "0
// archiviert"). Fall (8) liest den SOLL wie die Wache mit core.quotePath=false; der git-Koeder in Fall (18)
// trifft die Anker-Lesung am Argument-Ende 'ls-files -s' (vor ihr steht jetzt '-c core.quotePath=false'). Der
// Wortlaut 'in jedem Ausgang des Merges' in (27e) ist zu 'in mindestens einem' berichtigt (LA6-05). Rot zuerst
// je Stufe gegen die Wache 9223cbd5 bzw. Mutanten -- FIX-r6.md. LC3T-10 ENTLASTET (Lead-Probe K77: 240 Zeilen).
//
// NACHTRAG 8 (2026-09-19, Fix-r7 des OV-2-Zuges: Lens A r7 LA7-01..03, Lens B r6 LB6-02/03/05, Lens C r5 LC5T-01..06
// und LC5W-01..08): Fall (8) liest den SOLL nur noch per 'git -c core.quotePath=false ls-files' (Status geprueft),
// die drei grep-Filter und 'sort -u' sind C++ (LB6-02 = LC5T-01 = I-4; bis 8ae59179 sah soll.code nur 'sort -u').
// Fall (5) traegt die '..'-Stufen (5b) Wurzel verlassend mit '.'-Segment, (5c) repo-intern aufloesend = UNPRUEFBAR
// (Tiefenzaehler LA7-02; gegen 8ae59179 Exit 0), (5d) '..' vor der Quote-Pruefung, (5e) mittleres '..' ueber die
// Wurzel hinaus (LB6-03 = LC5T-02 = I-10). (6b) pinnt den Index-Eintrag des Links exakt (LC5T-05a), (6c) ist der
// UNTRACKED Symlink ohne Ziel (LC5T-03 = I-13; gegen 9223cbd5 Exit 2). WegwerfBaum nimmt seinen Pfad EXKLUSIV in
// Besitz (create_directory; Vorhandenes wird nie geloescht, ok()/fehler() statt stillem Erfolg), Destruktoren nur
// mit error_code-Ueberladungen, und Fall::baum_ ist derselbe Waechter (LC5T-04). (27g2) schliesst Stufe 0 aus
// (LB6-05); NEUE STUFEN (27g4) Datei auf Stufe 0 gegen Eintraege darunter auf Stufe 2 (Probe X36), (27g5) Eintraege
// darunter auf Stufe 1 UND 3 -> 'Stufe 1, Stufe 3' (LC5W-07; vorher 'Stufe 1 3'), (27i) Datei-Ahne unter einem
// D/F-Konflikt (Probe X37; gegen 8ae59179 TOT statt UNPRUEFBAR, LC5W-01), (27j) Datei-Ahne unter einem
// Arbeitsbaum-Symlink (Probe X38, LC5W-02); Fall (31) traegt (31d) eine von git auch mit core.quotePath=false
// quotierte Test-Quelldatei und (31e) einen so quotierten Anker-Ordner (LA7-01 = LC5W-06; gegen 8ae59179 STILL aus
// dem SOLL, "2 getrackte" und Exit 0); der Nenner hat ein siebtes Feld ('N mit von git quotiertem Pfad') und die
// Zeile 'Quotierte Index-Pfade' (nenner_quotiert). Anker-Stufen (d) Commit-Objekt unter 100644 und (e) Index-Modus
// 160000 (LC5T-06; beide per 'update-index --cacheinfo' anlegbar, Probe t7-machbarkeit). Rot zuerst je Stufe gegen
// 8ae59179 bzw. 9223cbd5 (COMDARE_PA1_WACHE_PFAD) und gegen die Mutanten -- FIX-r7.md.
//
// NACHTRAG 9 (2026-09-19, Fix-r8 des OV-2-Zuges: Lens B r7 LB7-01..06, Lens C r6 LC6T-01..05 und LC6W-01..09, Lens A
// r8 LA8-01..06): (27j) liest den Index als EIGENEN Lauf mit rc-Pruefung und exakter Ausgabe statt 'git | awk' (dash
// ohne pipefail: der Status war der von awk, ein git mit Exit 1 nach der Ausgabe blieb unentdeckt -- Koeder git-j,
// LB7-01 = LC6T-01); Fall (8) und (31c) pinnen die Nullseite der Nenner-Zeile 'Quotierte Index-Pfade' (LB7-02 =
// LC6T-02; Mutant M-LB7-quot-nullseite-stumm ueberlebte 33/33); Fall (24) traegt eindeutige Stufen-Etiketten
// (a)-(f) (LB7-03 = LC6T-03). NEUE STUFE (27k): ein Datei-Ahne, der im Arbeitsbaum ein Symlink ist, ist UNPRUEFBAR
// (LC6W-09; gegen 89cf7103 TOT); (27i) legt I-sub/x auf Stufe 0 statt 3, weil eine Datei NUR auf einer Konflikt-
// stufe seit Fix-r8 selbst 'konflikt' ist (modify/delete, LC6W-06) und als naeherer Ahne zuerst traefe. NEUE
// FAELLE: (32) SignaleUndZwischendateiFehlerSindExit2 -- die sechs Fix-r7-Mutanten M-W2a/b/c, M-W3a/b, M-W4a, die
// der Google-Test 33/33 ueberlebte ('nicht herstellbar' hiess es), fallen mit mktemp-, wc- und Signal-Koedern ueber
// koeder_bin_anlegen() und den Vorspann von fahren() (LB7-06, Rezepte LENS-B-r7.md Abschn. 6); (33)
// BerichtsUndEingabekanalFehlerSindExit2 -- Allowlist 0200 (Eingabe-Umleitung), stdout auf /dev/full und
// geschlossen, stderr auf /dev/full am Abbruchpfad, SIGPIPE ueber eine FIFO ohne Leser (LA8-01..04; gegen 89cf7103
// rc 2 ohne ABBRUCH-Zeile, 1, 1, 1 bzw. 141). Rot zuerst je Stufe gegen 89cf7103 bzw. den jeweiligen Mutanten
// (COMDARE_PA1_WACHE_PFAD) -- FIX-r8.md. Etikett 'Prueflig' in berichten() berichtigt.
//
// NACHTRAG 9a (2026-09-19, Fixer r8b): NEUER Fall (34) AnkerDFTeilrestGrepZielModifyDeleteGitlinkLinkSind...
// pinnt die fuenf Fix-r8-Klassen (15e) Anker im D/F-Konflikt, (15g) Byte-Abgleich, (15h) grep-Ziel vorher
// leeren, (15i) modify/delete und (15j) Gitlink als Arbeitsbaum-Symlink google-seitig -- ihre Mutanten M-R8-05,
// M-R8-07, M-R8-08, M-R8-09 und M-R8-12b ueberlebten den Google-Test 35/35 und waren nur shell-seitig
// getoetet (LB7-06-Klasse). Rot zuerst je Stufe am Mutanten, gruen gegen HEAD in 4 Zellen -- FIX-r8.md Abschn. 5b.
//
// ASCII-only, Zeilen <= 120 Byte.
// =============================================================================

#if defined(_WIN32)

#include <gtest/gtest.h>

TEST(Pa1ToteAusnahme, NurPosix) {
    GTEST_SKIP() << "scripts/ci_test_registrierungs_wache.sh ist ein POSIX-sh-Skript "
                    "(kein Windows-Job faehrt es).";
}

#else

#include "support/wachen_werkbank.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using comdare::test::wachen::enthaelt;
using comdare::test::wachen::fahre;
using comdare::test::wachen::koeder;
using comdare::test::wachen::Lauf;
using comdare::test::wachen::WegwerfRepo;
using comdare::test::wachen::zitiert;

namespace {

// Die Wache verlangt diese Datei als MESSGERAET-GEGENPROBE: sie muss existieren UND im
// Bauweg stehen, sonst faellt die Wache kein Urteil (Exit 2).
constexpr char const* kGegenprobe = "tests/unit/test_pressure_state.cpp";

// Die vier geparkten Posten. Sie stehen hier als Literale, weil Fall (8) und Fall (9)
// genau sie gegen die beiden committeten Allowlists fahren.
// W-B (2026-08-15, Vorlage-B2-GO): per 'git mv' nach tests/deprecated/prt_art_legacy_waisen/
// archiviert (VERMERK.md dort). Der SOLL ist am DATEINAMEN verankert, die vier bleiben also
// im SOLL und in der Allowlist -- nur der Pfad in Feld 1 und hier ist nachgezogen.
// UEBERHOLT (2026-09-17, Owner-Order 206): der letzte Satz gilt nicht mehr. Die vier stehen in
// KEINER Allowlist mehr und sind auch nicht mehr im SOLL der Registrierungs-Wache -- sie sind
// ARCHIV, weil tests/deprecated/prt_art_legacy_waisen/VERMERK.md im Index liegt und die Wache
// diesen Anker liest. Die Literale hier bleiben trotzdem noetig: Fall (8) nimmt sie aus dem
// Bauweg heraus (Anlass des Archiv-Abzugs) und Fall (9) sucht ihre Namen in der Schwesterliste.
constexpr char const* kGeparkt[] = {
    "tests/deprecated/prt_art_legacy_waisen/test_concepts_compile.cpp",
    "tests/deprecated/prt_art_legacy_waisen/test_value_handle.cpp",
    "tests/deprecated/prt_art_legacy_waisen/test_three_layer_audit.cpp",
    "tests/deprecated/prt_art_legacy_waisen/test_six_page_structures.cpp",
};
constexpr std::size_t kGeparktN = sizeof kGeparkt / sizeof kGeparkt[0];
// PFLEGE-KOPPLUNG (Lens B LB-03-Rest, Lens C LCT-09; 2026-09-18): Fall (8) pinnt die Archiv-Zahlen der Wache
// aus DIESEN Literalen -- die Datei-Zahl aus kGeparktN, die Ordner-Zahl aus kGeparktOrdnerN (alle vier liegen
// in EINEM Ordner). Ein fuenfter Archiv-Ordner oder eine fuenfte archivierte Datei im echten Repo aendert die
// Zahlen der Wache; dann sind kGeparkt UND kGeparktOrdnerN nachzuziehen, sonst faellt Fall (8) laut (nie
// still gruen: der Nenner der Wache und der SOLL des Falls stammen aus derselben Quelle, git ls-files).
constexpr std::size_t kGeparktOrdnerN = 1;

// Ein EHRLICHER CMakeCache fuer die ISA-Faelle (19)-(21) (2026-09-18, Fix-r3): avx2 vorhanden (Wert 1,
// _EXITCODE 0), avx512f fehlt (Wert leer, _EXITCODE 1) -- dieselbe Form wie kEhrlichAvx2 in
// tests/unit/test_mt_l4_registrierungs_wache_isa.cpp. Damit traegt 'isa:avx512f' (das Merkmal fehlt dem
// Bau-Host) -- die Faelle hier brauchen die tragende Seite als Arrangement und Gegenrichtung.
constexpr char const* kIsaCacheAvx2DaAvx512fFehlt = "COMDARE_HOST_RUNS_AVX2:INTERNAL=1\n"
                                                    "COMDARE_HOST_RUNS_AVX2_COMPILED:INTERNAL=TRUE\n"
                                                    "COMDARE_HOST_RUNS_AVX2_EXITCODE:INTERNAL=0\n"
                                                    "COMDARE_HOST_RUNS_AVX512F:INTERNAL=\n"
                                                    "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=TRUE\n"
                                                    "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=1\n";

[[nodiscard]] std::string wachen_pfad() {
    if (char const* const ueberschrieben = std::getenv("COMDARE_PA1_WACHE_PFAD");
        ueberschrieben != nullptr && *ueberschrieben != '\0') {
        return std::string{ueberschrieben};
    }
    return std::string{COMDARE_PA1_WACHE};
}

[[nodiscard]] std::string repo_wurzel() { return std::string{COMDARE_PA1_REPO}; }

void berichten(char const* fall, Lauf const& lauf, std::string const& marke) {
    std::cout << "  [PA-1] Fall '" << fall << "' | Koeder " << marke << " | Pruefling " << wachen_pfad() << " | Exit "
              << lauf.code << "\n";
}

// ---------------------------------------------------------------------------
// DIE NENNER-ZEILEN DER WACHE ALS GANZE ZEILEN (Lens C LCT-10/LCT-11, 2026-09-18). Ein Zaehler-Pin wie
// "1 unpruefbar" ist eine Teilzeichenkette von "11 unpruefbar", "1 ohne Gegenstand" eine von "11 ohne
// Gegenstand" -- dieselbe Klasse wie '/build' gegen '/builds' (Kopf). Gepinnt wird deshalb die ganze Zeile
// mit allen Feldern: wer ein Feld umbenennt, verschiebt oder seinen Zaehler in einen fremden Kontext
// traegt, faellt hier laut. Die Reihenfolge der Felder ist die der Wache (scripts/ci_test_registrierungs_
// wache.sh, Block NENNER und Endzeile); eine Aenderung dort zieht diese vier Helfer nach.
// SECHSTES FELD (2026-09-18, Fix-r3, Lens C LC3W-05): "N mit Formfehler fuer Dateien im Bauweg" -- der
// Parameter 'form' hat die Vorgabe 0, damit jeder aeltere Pin weiter die GANZE Zeile prueft.
// SIEBTES FELD (2026-09-19, Fix-r7, Lens A r7 LA7-01 = Lens C r5 LC5W-06): "N mit von git quotiertem Pfad" --
// 'quotiert' mit Vorgabe 0, wie 'form'; dazu die eigene Nenner-Zeile 'Quotierte Index-Pfade' (nenner_quotiert).
// ---------------------------------------------------------------------------
[[nodiscard]] std::string z(std::size_t n) { return std::to_string(n); }

[[nodiscard]] std::string nenner_ohne_bauweg(std::size_t anker, std::size_t archiv_zeilen, std::size_t ort_zeilen,
                                             std::size_t geist, std::size_t doppelt, std::size_t form = 0,
                                             std::size_t quotiert = 0) {
    return "dazu UNPRUEFBAR ohne Bezug zum Bauweg: " + z(anker) + " ARCHIV-Anker ohne Inhalt/Form, " +
           z(archiv_zeilen) + " Allowlist-Zeile(n) fuer ARCHIV-Dateien, " + z(ort_zeilen) +
           " fuer Pfade unter tests/deprecated/ ohne wirksamen Anker, " + z(geist) +
           " ohne Gegenstand im SOLL-Bestand, " + z(doppelt) + " Pfad(e) mit doppelter Zeile, " + z(form) +
           " mit Formfehler fuer Dateien im Bauweg, " + z(quotiert) + " mit von git quotiertem Pfad.";
}

[[nodiscard]] std::string nenner_quotiert(std::size_t n, std::size_t soll, std::size_t anker) {
    return "Quotierte Index-Pfade: " + z(n) + " von git auch mit core.quotePath=false quotiert (Tabulator," +
           " Steuerzeichen, Anfuehrungszeichen, Backslash), davon " + z(soll) + " im SOLL-Muster und " + z(anker) +
           " als VERMERK.md-Anker (beide UNPRUEFBAR).";
}

[[nodiscard]] std::string nenner_davon(std::size_t begruendet, std::size_t erloschen, std::size_t tot,
                                       std::size_t unpruefbar, std::size_t ohne) {
    return "davon " + z(begruendet) + " begruendet, " + z(erloschen) + " mit ERLOSCHENER, " + z(tot) +
           " TOTE AUSNAHME, " + z(unpruefbar) + " mit UNPRUEFBARER Begruendung, " + z(ohne) + " ohne.";
}

[[nodiscard]] std::string endzeile_rot(std::size_t ohne, std::size_t soll, std::size_t erloschen, std::size_t tot,
                                       std::size_t unpruefbar, std::size_t archiviert) {
    return "TEST-REGISTRIERUNGS-WACHE: ROT (" + z(ohne) + " von " + z(soll) + " ohne Begruendung, " + z(erloschen) +
           " erloschen, " + z(tot) + " tot, " + z(unpruefbar) + " unpruefbar, " + z(archiviert) + " archiviert).";
}

[[nodiscard]] std::string endzeile_ok(std::size_t soll, std::size_t archiviert) {
    return "TEST-REGISTRIERUNGS-WACHE: OK (" + z(soll) + " Quelldateien, 0 ohne Begruendung ausserhalb, " +
           z(archiviert) + " archiviert).";
}

// Die beiden Bestands-Zeilen des Nenners und die AUSSERHALB-Zeile, ebenfalls als ganze Zeilen (Fix-r6).
[[nodiscard]] std::string nenner_getrackt(std::size_t n) {
    return z(n) + " getrackte Test-Quelldatei(en) im Baum (ohne ext/).";
}

[[nodiscard]] std::string nenner_archiv(std::size_t archiv, std::size_t ordner, std::size_t soll) {
    return "davon " + z(archiv) + " ARCHIV-Datei(en) in " + z(ordner) +
           " Ordner(n) unter tests/deprecated/ mit VERMERK.md-Anker abgezogen -- SOLL: " + z(soll) + ".";
}

[[nodiscard]] std::string nenner_ausserhalb(std::size_t n, std::size_t begruendet) {
    return "Erreichbarkeit: " + z(n) + " der " + z(begruendet) + " begruendeten nennen einen Gegenstand AUSSERHALB";
}

// ---------------------------------------------------------------------------
// ZEILEN-PINS (Lens C r4 LC3T-04, Fix-r6). enthaelt() findet den Text auch mitten in einer laengeren oder
// praefigierten Zeile -- ein Mutant, der an eine Nenner-Zeile etwas anhaengt (Probe: " (Vorbehalt)") oder eine
// Endzeile mit fremden Zahlen zusaetzlich druckt, faellt so nicht auf. zeile_exakt() zerlegt die Ausgabe an
// '\n' und verlangt eine Zeile, die dem Text GENAU gleicht -- wahlweise mit der zweistelligen Einrueckung der
// Listen und des Nenners (eingerueckt() der Wache); zeile_beginnt() verlangt eine Zeile, die so BEGINNT (fuer
// Meldungen, deren Rest Pfad oder Diagnose ist). enthaelt() bleibt fuer Fragmente mitten in einer Zeile und
// fuer EXPECT_FALSE (dort ist die Teilzeichenkette die staerkere Forderung).
// ---------------------------------------------------------------------------
[[nodiscard]] bool zeile_passt(std::string const& ausgabe, std::string const& text, bool exakt) {
    for (std::size_t start = 0; start <= ausgabe.size();) {
        std::size_t const ende = ausgabe.find('\n', start);
        std::string const zl   = ausgabe.substr(start, ende == std::string::npos ? std::string::npos : ende - start);
        std::string const ohne = (zl.size() >= 2 && zl[0] == ' ' && zl[1] == ' ') ? zl.substr(2) : zl;
        bool const trifft = exakt ? (zl == text || ohne == text)
                                  : (zl.compare(0, text.size(), text) == 0 || ohne.compare(0, text.size(), text) == 0);
        if (trifft) { return true; }
        if (ende == std::string::npos) { break; }
        start = ende + 1;
    }
    return false;
}

[[nodiscard]] bool zeile_exakt(std::string const& ausgabe, std::string const& text) {
    return zeile_passt(ausgabe, text, true);
}

[[nodiscard]] bool zeile_beginnt(std::string const& ausgabe, std::string const& text) {
    return zeile_passt(ausgabe, text, false);
}

// Ein Wegwerf-Repo-Kommando in der Umgebung der Werkbank (git im Repo, Ausgabe getrimmt).
[[nodiscard]] Lauf im_repo(WegwerfRepo const& repo, std::string const& befehl) {
    Lauf l = fahre("cd " + zitiert(repo.pfad()) + " && " + WegwerfRepo::umgebung() + " " + befehl);
    while (!l.ausgabe.empty() && (l.ausgabe.back() == '\n' || l.ausgabe.back() == '\r')) { l.ausgabe.pop_back(); }
    return l;
}

// Eine Datei mit Anfuehrungszeichen im Namen anlegen und in den Index nehmen (Fix-r7, Stufen (31d)/(31e)):
// die Werkbank zitiert Pfade fuer die Shell in doppelten Anfuehrungszeichen (zitiert()), ein '"' im Namen
// braeche dort die Kommandozeile -- hier einfache Anfuehrungszeichen, der Name traegt kein '\''. Die
// Gegenprobe fragt git selbst (ls-files -s mit :(literal)-Pathspec), wie schreibe_und_verfolge().
[[nodiscard]] testing::AssertionResult
schreibe_und_verfolge_einfach_zitiert(WegwerfRepo const& repo, std::string const& relativ, std::string const& inhalt) {
    if (relativ.find('\'') != std::string::npos) {
        return testing::AssertionFailure() << "Pfad traegt ein einfaches Anfuehrungszeichen: " << relativ;
    }
    testing::AssertionResult const r = repo.schreibe(relativ, inhalt);
    if (!r) { return r; }
    Lauf const add = im_repo(repo, "git add -- '" + relativ + "'");
    if (add.code != 0) {
        return testing::AssertionFailure() << "git add '" << relativ << "' fehlgeschlagen (Exit " << add.code << "):\n"
                                           << add.ausgabe;
    }
    Lauf const probe = im_repo(repo, "git -c core.quotePath=false ls-files -s -- ':(literal)" + relativ + "'");
    if (probe.code != 0 || probe.ausgabe.empty()) {
        return testing::AssertionFailure()
               << "'" << relativ << "' steht nach dem git add nicht im Index (Exit " << probe.code << "):\n"
               << probe.ausgabe;
    }
    return testing::AssertionSuccess();
}

// Ein PATH-Koeder: ein Werkzeug gleichen Namens VOR dem echten im PATH (Fall (18)).
[[nodiscard]] testing::AssertionResult koeder_bin_anlegen(WegwerfRepo const& repo, std::string const& name,
                                                          std::string const& inhalt) {
    testing::AssertionResult const r = repo.schreibe("koeder_bin/" + name, inhalt);
    if (!r) { return r; }
    std::error_code ec;
    fs::permissions(repo.pfad() / "koeder_bin" / name,
                    fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec | fs::perms::others_read |
                        fs::perms::others_exec,
                    fs::perm_options::replace, ec);
    if (ec) { return testing::AssertionFailure() << "chmod +x '" << name << "' fehlgeschlagen: " << ec.message(); }
    return testing::AssertionSuccess();
}

// ---------------------------------------------------------------------------
// RAII fuer einen Wegwerf-Baum (Lens C r4 LC3T-09, Fix-r6; Besitz-Haertung Lens C r5 LC5T-04, Fix-r7): der
// Waechter nimmt seinen Pfad EXKLUSIV in Besitz -- fs::create_directory, nicht create_directories: ein schon
// vorhandener Pfad (Verzeichnis, Datei oder Link) wird NIE geloescht und nie als eigener angesehen; ok() und
// fehler() sagen es dem Fall, der laut faellt statt fremden Bestand zu raeumen. Bis 8ae59179 legte
// create_directories still ueber Vorhandenes, und der Destruktor loeschte es mit; sein fs::exists(pfad) ohne
// error_code haette im Destruktor werfen koennen (std::terminate). Jetzt nur error_code-Ueberladungen; ein
// Loeschfehler ist eine ADD_FAILURE mit Grund. Fall::baum_ ist derselbe Waechter (Probe t5-ownership, FIX-r7.md).
// ---------------------------------------------------------------------------
class WegwerfBaum {
public:
    explicit WegwerfBaum(fs::path pfad) : pfad_{std::move(pfad)} {
        std::error_code ec;
        besitz_ = fs::create_directory(pfad_, ec);
        if (ec) {
            besitz_ = false;
            fehler_ = "create_directory '" + pfad_.string() + "' fehlgeschlagen: " + ec.message();
        } else if (!besitz_) {
            fehler_ = "Pfad '" + pfad_.string() + "' bestand schon -- nicht in Besitz genommen, nichts geloescht";
        }
    }
    WegwerfBaum(WegwerfBaum const&)            = delete;
    WegwerfBaum& operator=(WegwerfBaum const&) = delete;
    ~WegwerfBaum() {
        if (!besitz_) { return; }
        std::error_code ec;
        fs::remove_all(pfad_, ec);
        std::error_code ec_rest;
        bool const      rest = fs::exists(pfad_, ec_rest);
        if (ec || ec_rest || rest) {
            ADD_FAILURE() << "Wegwerf-Baum '" << pfad_.string() << "' nicht geraeumt: " << ec.message()
                          << (ec_rest ? " / exists: " + ec_rest.message() : std::string{})
                          << (rest ? " / Pfad besteht noch" : "");
        }
    }
    [[nodiscard]] bool               ok() const { return besitz_; }
    [[nodiscard]] std::string const& fehler() const { return fehler_; }
    [[nodiscard]] fs::path const&    pfad() const { return pfad_; }

private:
    fs::path    pfad_;
    bool        besitz_ = false;
    std::string fehler_;
};

// ---------------------------------------------------------------------------
// Ein Fall: ein Wegwerf-Repo mit Gegenprobe-Datei, einer Waise und einem Bau-Baum, in
// dem die Waise fehlt. Was die Allowlist-Zeile dann in Feld 2 nennt, ist der Gegenstand
// der Untersuchung.
// ---------------------------------------------------------------------------
class Fall {
public:
    explicit Fall(std::string const& marke) : Fall{marke, "tests/unit/test_waise_" + marke + ".cpp"} {}

    // Zweiter Konstruktor (2026-09-17): der Waisen-PFAD ist waehlbar. Die ARCHIV-Klasse der Wache
    // haengt am ORT der Datei, nicht an ihrem Namen -- ein Fall dafuer braucht eine Waise unter
    // tests/deprecated/<ordner>/ statt unter tests/unit/. Alles andere bleibt identisch.
    // Der Bau-Baum ist ein WegwerfBaum (Fix-r7, LC5T-04): exklusiv in Besitz genommen, init() prueft ok().
    Fall(std::string const& marke, std::string const& waisen_pfad)
        : marke_{marke}, repo_{marke}, baum_{fs::path{repo_.pfad().string() + "_baum"}}, waise_{waisen_pfad} {}
    Fall(Fall const&)            = delete;
    Fall& operator=(Fall const&) = delete;

    [[nodiscard]] testing::AssertionResult init() {
        if (!baum_.ok()) { return testing::AssertionFailure() << "Bau-Baum: " << baum_.fehler(); }
        testing::AssertionResult r = repo_.init();
        if (!r) { return r; }
        r = repo_.ist_eigene_wurzel();
        if (!r) { return r; }
        r = repo_.schreibe_und_verfolge(kGegenprobe, "// Gegenprobe-Datei des Falls\n");
        if (!r) { return r; }
        // Die Waise ist getrackt und fehlt gleich im Bauweg -- sie ist der Anlass,
        // aus dem die Wache ueberhaupt in die Allowlist schaut.
        r = repo_.schreibe_und_verfolge(waise_, "// Waise des Falls " + marke_ + "\n");
        if (!r) { return r; }
        // Nur die Gegenprobe steht im Bauweg, die Waise nicht.
        return bauweg_schreiben({kGegenprobe});
    }

    // Feld 2 der einen Allowlist-Zeile setzen. Feld 3 traegt die Koeder-Marke, damit
    // ein GRUEN-Fall belegen kann, dass die Wache GENAU DIESE Zeile gelesen hat.
    [[nodiscard]] testing::AssertionResult allowlist_setzen(std::string const& feld2) const {
        return repo_.schreibe("scripts/ci_test_registrierungs_allowlist.txt", "# Allowlist des Falls " + marke_ + "\n" +
                                                                                  waise_ + " | " + feld2 +
                                                                                  " | Koeder " + marke_ + "\n");
    }

    [[nodiscard]] testing::AssertionResult bauweg_schreiben(std::vector<std::string> const& relativ) const {
        std::ofstream aus{baum_.pfad() / "compile_commands.json", std::ios::trunc};
        if (!aus.good()) { return testing::AssertionFailure() << "compile_commands.json nicht schreibbar"; }
        aus << "[\n";
        for (std::size_t i = 0; i < relativ.size(); ++i) {
            aus << "  {\"directory\": \"" << baum_.pfad().string() << "\",\n"
                << "   \"command\": \"c++ -c " << (repo_.pfad() / relativ[i]).string() << "\",\n"
                << "   \"file\": \"" << (repo_.pfad() / relativ[i]).string() << "\"}"
                << (i + 1 == relativ.size() ? "\n" : ",\n");
        }
        aus << "]\n";
        aus.close();
        if (!fs::exists(baum_.pfad() / "compile_commands.json")) {
            return testing::AssertionFailure() << "compile_commands.json fehlt nach dem Schreiben";
        }
        return testing::AssertionSuccess();
    }

    // 'heute' leer = Systemuhr. Sonst wird der Wache ihr Heute vorgegeben, damit beide
    // Seiten einer Frist gefahren werden koennen, ohne die Systemuhr zu stellen.
    // 'zusatz' (2026-09-18, Fall (18)): weitere Umgebung fuer den Wachen-Aufruf, z. B. ein PATH mit
    // einem Koeder-Werkzeug davor; die Shell des Aufrufs expandiert '$PATH' darin.
    [[nodiscard]] Lauf fahren(std::string const& heute = "", std::string const& zusatz = "") const {
        std::string vorspann = WegwerfRepo::umgebung();
        if (!heute.empty()) { vorspann += " COMDARE_WACHE_HEUTE=" + heute; }
        if (!zusatz.empty()) { vorspann += " " + zusatz; }
        return fahre("cd " + zitiert(repo_.pfad()) + " && " + vorspann + " sh " + zitiert(wachen_pfad()) + " " +
                     zitiert(baum_.pfad()));
    }

    // Ein CMakeCache.txt im Bau-Baum (2026-09-18, Fix-r3, Faelle (19)-(21)): 'isa:'-Zeilen lesen ihn;
    // ohne ihn ist jede 'isa:'-Zeile Exit 2. Der Inhalt kommt vom Fall, damit jede Stufe ihren Cache sieht.
    [[nodiscard]] testing::AssertionResult isa_cache_schreiben(std::string const& inhalt) const {
        std::ofstream aus{baum_.pfad() / "CMakeCache.txt", std::ios::trunc};
        if (!aus.good()) { return testing::AssertionFailure() << "CMakeCache.txt nicht schreibbar"; }
        aus << inhalt;
        aus.close();
        if (!fs::exists(baum_.pfad() / "CMakeCache.txt")) {
            return testing::AssertionFailure() << "CMakeCache.txt fehlt nach dem Schreiben";
        }
        return testing::AssertionSuccess();
    }

    [[nodiscard]] WegwerfRepo const& repo() const { return repo_; }
    [[nodiscard]] std::string const& waise() const { return waise_; }
    [[nodiscard]] fs::path const&    baum() const { return baum_.pfad(); }

private:
    std::string marke_;
    WegwerfRepo repo_;
    WegwerfBaum baum_;
    std::string waise_;
};

} // namespace

// =============================================================================
// (1) DER UNMOEGLICHE GEGENSTAND BEISST. Der Pfad kommt frisch aus /dev/urandom und
//     liegt unter einem Verzeichnis, das dieses Wegwerf-Repo nicht kennt -- weder im
//     Index noch in einer .gitignore-Regel. Vor der Heilung war dieser Fall GRUEN.
// =============================================================================
TEST(Pa1ToteAusnahme, UnmoeglicherGegenstandWirdTOT) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    // Der gewuerfelte Pfad liegt in einem gewuerfelten Verzeichnis: beide Segmente
    // kommen in keiner Datei dieses Repos vor.
    std::string const tot = "nirgends_" + marke + "/tief/" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + tot));

    Lauf const lauf = fall.fahren();
    berichten("UnmoeglicherGegenstandWirdTOT", lauf, marke);

    EXPECT_EQ(lauf.code, 1) << "Eine Ausnahme, die per Konstruktion nie erloeschen kann, muss ROT sein.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, "TOTE AUSNAHME -- der Gegenstand kann in KEINEM erklaerten Baum entstehen:"))
        << "Der Befund muss seinen Namen tragen -- sonst ist er von jedem anderen Rot nicht zu trennen.\n"
        << lauf.ausgabe;
    // Der Koeder muss WOERTLICH zurueckkommen: nur dann stammt die Meldung aus dem Gegenstand, den dieser Fall
    // angelegt hat, und nicht aus einer anderen Quelle -- als ganze Meldungszeile mit der allgemeinen Diagnose
    // (kein Vorfahr ist Datei, Symlink oder Konflikt: 'keine Quelle dieses Repos kennt den Zweig').
    EXPECT_TRUE(zeile_beginnt(lauf.ausgabe, fall.waise() + " -- TOTE AUSNAHME: \"" + tot +
                                                "\" existiert nicht, und keine Quelle dieses Repos kennt den Zweig"))
        << "Der gewuerfelte Pfad fehlt in der Ausgabe.\n"
        << lauf.ausgabe;
    // GANZE NENNER-ZEILE statt "1 TOTE AUSNAHME" (Lens C r4 LC3T-03, Fix-r6): das Fragment traf auch "11 TOTE".
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_davon(0, 0, 1, 0, 0))) << "Der Nenner muss die Klasse zaehlen (V-1).\n"
                                                                        << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, endzeile_rot(0, 2, 0, 1, 0, 0))) << lauf.ausgabe;
}

// =============================================================================
// (2) DER GEGENKOEDER (K13, andere Richtung). Derselbe Aufbau, derselbe Wuerfel -- aber
//     der Pfad liegt unter tests/unit/, einem Verzeichnis MIT getracktem Inhalt. Er
//     existiert heute nicht und kann trotzdem jederzeit entstehen. Er darf NICHT
//     beissen. Ohne diesen Fall waere Fall (1) auch von einer Wache erfuellt, die
//     jeden abwesenden Gegenstand fuer tot erklaert.
// =============================================================================
TEST(Pa1ToteAusnahme, MoeglicherGegenstandBleibtGRUEN) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    // tests/unit/ traegt in diesem Repo die Gegenprobe-Datei und die Waise -- das
    // Verzeichnis ist dem Index also bekannt.
    std::string const lebendig = "tests/unit/kommt_vielleicht_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + lebendig));

    Lauf const lauf = fall.fahren();
    berichten("MoeglicherGegenstandBleibtGRUEN", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << "Ein Pfad unter einem bekannten Verzeichnis kann entstehen -- kein Befund.\n"
                            << lauf.ausgabe;
    // GANZE NENNER-ZEILEN statt "0 TOTE AUSNAHME" (Lens C r4 LC3T-03, Fix-r6): das Fragment traf auch "10 TOTE".
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_davon(1, 0, 0, 0, 0)))
        << "Fehlalarm: der Gegenstand hat einen Erzeuger.\n"
        << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, endzeile_ok(2, 0))) << lauf.ausgabe;
    // 'Koeder <marke>' ist Feld 3 der Zeile und steht NUR in der Ausgabe, wenn die Wache die Zeile gelesen hat;
    // die blosse Marke stuende auch als Pfadname (Bau-Baum, Waise) in jeder Ausgabe (Lens B r4 LB4-I1, Fix-r5).
    // Seit Fix-r6 als Zeilenanfang der begruendeten Zeile: '<waise> -- Koeder <marke>' (LC3T-04).
    EXPECT_TRUE(zeile_beginnt(lauf.ausgabe, fall.waise() + " -- Koeder " + marke))
        << "Die Wache muss GENAU diese Allowlist-Zeile gelesen haben -- sonst belegt das Gruen nichts.\n"
        << lauf.ausgabe;
}

// =============================================================================
// (3) DAS BAUPRODUKT IST ERREICHBAR. Ein Pfad unter einer .gitignore-Regel ist ein
//     angemeldeter Ablageort fuer Erzeugtes: er existiert nicht, ist nicht getrackt --
//     und kann bei jedem Bau entstehen. Die zweite Haelfte des Gegenkoeders, und die
//     Stelle, an der die Falle 'check-ignore <dir>' vs. '<dir>/' sitzt.
// =============================================================================
TEST(Pa1ToteAusnahme, BauproduktUnterGitignoreIstErreichbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(".gitignore", "erzeugt_" + marke + "/\n"));
    std::string const bauprodukt = "erzeugt_" + marke + "/artefakt.hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + bauprodukt));

    Lauf const lauf = fall.fahren();
    berichten("BauproduktUnterGitignoreIstErreichbar", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << "Ein ignoriertes Verzeichnis ist ein angemeldeter Ablageort -- kein Befund.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << "Fehlalarm auf einem Bauprodukt.\n"
                                                                        << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, endzeile_ok(2, 0))) << lauf.ausgabe;
}

// =============================================================================
// (4) DER SUBMODUL-PFAD IST ERREICHBAR. Ein nicht ausgechecktes Submodul steht als
//     GITLINK im Index -- sein Inhalt nicht. Ein Gegenstand darunter existiert also
//     nicht, ist nicht getrackt, ist nicht ignoriert, und kann trotzdem jederzeit
//     durch ein 'submodule update' auftauchen. Genau der Fall, den die Regel NICHT
//     faelschlich toeten darf.
// =============================================================================
TEST(Pa1ToteAusnahme, SubmodulGitlinkIstErreichbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());

    std::string const modul = "ext/fremd_" + marke;
    // Einen Gitlink von Hand in den Index legen: 160000 ist der Modus fuer 'commit'.
    // Der SHA muss kein existierendes Objekt sein -- der Index haelt ihn trotzdem.
    Lauf const idx = fahre("cd " + zitiert(fall.repo().pfad()) + " && " + WegwerfRepo::umgebung() +
                           " git update-index --add --cacheinfo 160000,"
                           "0000000000000000000000000000000000000001," +
                           modul);
    ASSERT_EQ(idx.code, 0) << "Gitlink konnte nicht in den Index gelegt werden:\n" << idx.ausgabe;

    std::string const drin = modul + "/include/kopf.hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + drin));

    Lauf const lauf = fall.fahren();
    berichten("SubmodulGitlinkIstErreichbar", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << "Ein Pfad unter einem Gitlink kann jederzeit ausgecheckt werden -- kein Befund.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_davon(1, 0, 0, 0, 0)))
        << "Fehlalarm: ein nicht initialisiertes Submodul ist kein toter Gegenstand.\n"
        << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, endzeile_ok(2, 0))) << lauf.ausgabe;
}

// =============================================================================
// (5) AUSSERHALB DES REPOS WIRD NICHT GEURTEILT. Ein absoluter Pfad ist mit den
//     Quellen dieses Repos nicht beurteilbar. Die Wache darf ihn deshalb NIE tot
//     nennen -- und muss sagen, dass sie ihn nicht beurteilt hat (T-9, V-1).
// =============================================================================
TEST(Pa1ToteAusnahme, AusserhalbDesRepoWirdNichtBeurteilt) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    std::string const draussen = "/opt/gibtsnicht_" + marke + "/kopf.hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + draussen));

    Lauf const lauf = fall.fahren();
    berichten("AusserhalbDesRepoWirdNichtBeurteilt", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << "Ein nicht beurteilbarer Pfad ist kein Befund.\n" << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_davon(1, 0, 0, 0, 0)))
        << "Die Wache hat ueber etwas geurteilt, das sie nicht messen kann.\n"
        << lauf.ausgabe;
    // NICHT das blosse Wort 'AUSSERHALB' (Lens C r4 LC3T-01, Fix-r6): es steht bei JEDEM Lauf im Nenner ("0 der 0
    // begruendeten nennen einen Gegenstand AUSSERHALB"), der alte Pin war vakuoes. Gepinnt wird die Zahl in der
    // ganzen Zeile UND die begruendete Zeile selbst mit dem Vermerk, dass die Erreichbarkeit NICHT beurteilt ist.
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_ausserhalb(1, 1)))
        << "Der Nenner muss die nicht beurteilte Menge ausweisen -- sonst sieht sie wie geprueft aus.\n"
        << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, fall.waise() + " -- Koeder " + marke + " (datei abwesend: " + draussen +
                                              " -- ausserhalb des Repos, Erreichbarkeit NICHT beurteilt)"))
        << "Die begruendete Zeile muss den Vorbehalt tragen.\n"
        << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, endzeile_ok(2, 0))) << lauf.ausgabe;

    // (5b) EIN '..', DAS DIE WURZEL VERLAESST (Lens B r6 LB6-03 = Lens C r5 LC5T-02 = I-10, Fix-r7): der Pfad
    //      '../aussen_<marke>/./y.hpp' ist Grenze (a), NICHT beurteilt -- und das '..' gewinnt vor der Formregel
    //      fuer das '.'-Segment (die Wache 9223cbd5 sagte 'kein kanonischer repo-relativer Pfad', rot; Probe X20).
    //      Exit 0, gepinnt wie (5): begruendete Zeile mit Vorbehalt, Nenner, AUSSERHALB-Zeile, Endzeile.
    std::string const dd_raus = "../aussen_" + marke + "/./y.hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + dd_raus));
    Lauf const raus = fall.fahren();
    berichten("AusserhalbDesRepoWirdNichtBeurteilt/dotdot-verlaesst-Wurzel", raus, marke);
    EXPECT_EQ(raus.code, 0) << "Ein Pfad, der per '..' die Wurzel verlaesst, ist Grenze (a) -- kein Befund.\n"
                            << raus.ausgabe;
    EXPECT_TRUE(zeile_exakt(raus.ausgabe, fall.waise() + " -- Koeder " + marke + " (datei abwesend: " + dd_raus +
                                              " -- ausserhalb des Repos, Erreichbarkeit NICHT beurteilt)"))
        << raus.ausgabe;
    EXPECT_FALSE(enthaelt(raus.ausgabe, "kein kanonischer")) << "Das '..' muss VOR der '.'-Formregel gewinnen.\n"
                                                             << raus.ausgabe;
    EXPECT_TRUE(zeile_exakt(raus.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << raus.ausgabe;
    EXPECT_TRUE(zeile_exakt(raus.ausgabe, nenner_ausserhalb(1, 1))) << raus.ausgabe;
    EXPECT_TRUE(zeile_exakt(raus.ausgabe, endzeile_ok(2, 0))) << raus.ausgabe;

    // (5c) EIN '..', DAS INNERHALB DES REPOS AUFLOEST (Lens A r7 LA7-02, Tiefenzaehler; Fix-r7): der Pfad
    //      'ext/../aussen_<marke>/y.hpp' verlaesst die Wurzel NICHT (Tiefe 1 -> 0) -- eine nicht kanonische
    //      Schreibweise, so steht kein Pfad im Index: UNPRUEFBAR, ROT, mit eigener Diagnose. Bis 8ae59179 lief
    //      sie als 'ausserhalb des Repos' begruendet durch (Exit 0, Probe X20b; am echten Baum Koeder K6) und
    //      umging so die TOT-Frage.
    std::string const dd_innen = "ext/../aussen_" + marke + "/y.hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + dd_innen));
    Lauf const innen = fall.fahren();
    berichten("AusserhalbDesRepoWirdNichtBeurteilt/dotdot-loest-intern-auf", innen, marke);
    EXPECT_EQ(innen.code, 1) << "Ein repo-intern aufloesendes '..' ist keine Grenze -- UNPRUEFBAR, ROT.\n"
                             << innen.ausgabe;
    EXPECT_TRUE(zeile_exakt(innen.ausgabe, fall.waise() + " -- UNPRUEFBAR: \"datei:" + dd_innen +
                                               "\" ist kein kanonischer repo-relativer Pfad (Segment '..', das" +
                                               " INNERHALB des Repos aufloest) -- so steht kein Pfad im Index;" +
                                               " nenne den aufgeloesten Pfad"))
        << innen.ausgabe;
    EXPECT_FALSE(enthaelt(innen.ausgabe, "Erreichbarkeit NICHT beurteilt")) << innen.ausgabe;
    EXPECT_TRUE(zeile_exakt(innen.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << innen.ausgabe;
    EXPECT_TRUE(zeile_exakt(innen.ausgabe, nenner_ausserhalb(0, 0))) << innen.ausgabe;
    EXPECT_TRUE(zeile_exakt(innen.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << innen.ausgabe;

    // (5d) '..' VOR DER QUOTE-PRUEFUNG: '../a"b_<marke>/y.hpp' verlaesst die Wurzel -- Grenze (a) gewinnt vor
    //      dem Anfuehrungszeichen (Probe X21): Exit 0, kein 'quotiert', die begruendete Zeile traegt den Pfad roh.
    std::string const dd_quote = "../a\"b_" + marke + "/y.hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + dd_quote));
    Lauf const quote = fall.fahren();
    berichten("AusserhalbDesRepoWirdNichtBeurteilt/dotdot-vor-Quote-Pruefung", quote, marke);
    EXPECT_EQ(quote.code, 0) << quote.ausgabe;
    EXPECT_TRUE(zeile_exakt(quote.ausgabe, fall.waise() + " -- Koeder " + marke + " (datei abwesend: " + dd_quote +
                                               " -- ausserhalb des Repos, Erreichbarkeit NICHT beurteilt)"))
        << quote.ausgabe;
    EXPECT_FALSE(enthaelt(quote.ausgabe, "enthaelt ein Zeichen, das git in seiner Ausgabe quotiert"))
        << "Die Quote-Diagnose darf nicht greifen (die Nenner-Zeile 'Quotierte Index-Pfade' steht immer).\n"
        << quote.ausgabe;
    EXPECT_TRUE(zeile_exakt(quote.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << quote.ausgabe;
    EXPECT_TRUE(zeile_exakt(quote.ausgabe, nenner_ausserhalb(1, 1))) << quote.ausgabe;
    EXPECT_TRUE(zeile_exakt(quote.ausgabe, endzeile_ok(2, 0))) << quote.ausgabe;

    // (5e) EIN MITTLERES '..', DAS DIE WURZEL VERLAESST: 'ext/../../aussen_<marke>/y.hpp' (Tiefe 1 -> 0 -> -1) --
    //      nicht nur ein fuehrendes '..' ist Grenze (a) (Probe X30c). Exit 0 wie (5b).
    std::string const dd_mitte = "ext/../../aussen_" + marke + "/y.hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + dd_mitte));
    Lauf const mitte = fall.fahren();
    berichten("AusserhalbDesRepoWirdNichtBeurteilt/mittleres-dotdot-verlaesst-Wurzel", mitte, marke);
    EXPECT_EQ(mitte.code, 0) << mitte.ausgabe;
    EXPECT_TRUE(zeile_exakt(mitte.ausgabe, fall.waise() + " -- Koeder " + marke + " (datei abwesend: " + dd_mitte +
                                               " -- ausserhalb des Repos, Erreichbarkeit NICHT beurteilt)"))
        << mitte.ausgabe;
    EXPECT_TRUE(zeile_exakt(mitte.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << mitte.ausgabe;
    EXPECT_TRUE(zeile_exakt(mitte.ausgabe, nenner_ausserhalb(1, 1))) << mitte.ausgabe;
    EXPECT_TRUE(zeile_exakt(mitte.ausgabe, endzeile_ok(2, 0))) << mitte.ausgabe;
}

// =============================================================================
// (6) DIE ERSTE RICHTUNG BLEIBT ERHALTEN. Regressionsschutz: die neue Richtung darf die
//     alte nicht verdraengen. Existiert der Gegenstand, ist die Ausnahme ERLOSCHEN --
//     und ausdruecklich NICHT 'tot'. Die beiden Klassen verlangen verschiedene Abhilfen.
// =============================================================================
TEST(Pa1ToteAusnahme, RichtungEinsBleibtErhalten) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    std::string const wieder_da = "tests/unit/wieder_da_" + marke + ".hpp";
    ASSERT_TRUE(fall.repo().schreibe(wieder_da, "// der Gegenstand ist zurueck\n"));
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + wieder_da));

    Lauf const lauf = fall.fahren();
    berichten("RichtungEinsBleibtErhalten", lauf, marke);

    EXPECT_EQ(lauf.code, 1) << "Ein wieder vorhandener Gegenstand muss ROT sein.\n" << lauf.ausgabe;
    // GANZE ERLOSCHEN-ZEILE (Lens C r4 LC3T-02, Fix-r6): das Wort 'ERLOSCHEN' traf bei jedem Lauf "0 mit
    // ERLOSCHENER" im Nenner -- der alte Pin war vakuoes; Exit 1 allein kaeme auch aus einer anderen Rot-Klasse.
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, fall.waise() + " -- ERLOSCHEN: \"" + wieder_da +
                                              "\" existiert wieder, die Ausnahme traegt nicht mehr"))
        << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_davon(0, 1, 0, 0, 0)))
        << "Ein vorhandener Gegenstand ist erloschen, nicht tot -- die Abhilfe waere sonst falsch.\n"
        << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, endzeile_rot(0, 2, 1, 0, 0, 0))) << lauf.ausgabe;

    // (6b) EIN SYMLINK OHNE ZIEL am Gegenstand (Lens A r6 LA6-06, Fix-r6): '[ -e ]' folgt dem Link und sah
    //      'abwesend' -- die Wache 9223cbd5 meldete die Zeile als begruendet "(datei abwesend: ...)", Exit 0,
    //      obwohl der Pfad belegt ist (Probe X08). Jetzt ERLOSCHEN, der Link steht in der Meldung. Der Link ist
    //      per 'git add' auch im Index (Modus 120000); ein untracked Link ohne Ziel lief bis 9223cbd5 sogar in
    //      check-ignore 128 (Probe X08b) und ist jetzt ebenso ERLOSCHEN.
    std::string const link = "tests/unit/link_" + marke;
    std::error_code   ec;
    fs::create_symlink("nirgends_" + marke, fall.repo().pfad() / link, ec);
    ASSERT_FALSE(ec) << "Symlink nicht anlegbar: " << ec.message();
    Lauf const add = im_repo(fall.repo(), "git add -- " + zitiert(link));
    ASSERT_EQ(add.code, 0) << "git add des Symlinks fehlgeschlagen:\n" << add.ausgabe;
    // Arrangement als EXAKTE Index-Zeile (Lens C r5 LC5T-05a, Fix-r7): Modus 120000, das Blob des Link-Texts,
    // Stufe 0, Pfad -- nicht nur 'git add' war erfolgreich.
    Lauf const lsha6 = im_repo(fall.repo(), "printf '%s' 'nirgends_" + marke + "' | git hash-object --stdin");
    ASSERT_EQ(lsha6.ausgabe.size(), 40U) << "kein SHA-1: '" << lsha6.ausgabe << "'";
    Lauf const leintrag = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + link));
    ASSERT_TRUE(zeile_exakt(leintrag.ausgabe, "120000 " + lsha6.ausgabe + " 0\t" + link))
        << "Arrangement: der Link steht nicht als 120000/Stufe 0 im Index:\n"
        << leintrag.ausgabe;
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + link));
    Lauf const dangling = fall.fahren();
    berichten("RichtungEinsBleibtErhalten/Symlink-ohne-Ziel", dangling, marke);
    EXPECT_EQ(dangling.code, 1) << "Ein Symlink ohne Ziel belegt den Pfad -- ERLOSCHEN, ROT.\n" << dangling.ausgabe;
    EXPECT_TRUE(zeile_exakt(dangling.ausgabe, fall.waise() + " -- ERLOSCHEN: \"" + link +
                                                  "\" existiert wieder (als SYMLINK ohne Ziel), die Ausnahme" +
                                                  " traegt nicht mehr"))
        << dangling.ausgabe;
    EXPECT_FALSE(enthaelt(dangling.ausgabe, "datei abwesend")) << "Ein belegter Pfad darf nicht 'abwesend' heissen.\n"
                                                               << dangling.ausgabe;
    EXPECT_TRUE(zeile_exakt(dangling.ausgabe, nenner_davon(0, 1, 0, 0, 0))) << dangling.ausgabe;
    EXPECT_TRUE(zeile_exakt(dangling.ausgabe, endzeile_rot(0, 2, 1, 0, 0, 0))) << dangling.ausgabe;

    // (6c) DERSELBE LINK OHNE 'git add' (Lens C r5 LC5T-03 = I-13, Fix-r7): der UNTRACKED Symlink ohne Ziel
    //      (Probe X08b) lief bis 9223cbd5 in check-ignore 128 = Exit 2 mit der falschen Diagnose. ist_verfolgt
    //      ist hier ASSERT_FALSE -- sonst maesse die Stufe (6b). Dieselben Pins: ERLOSCHEN mit '(als SYMLINK ohne
    //      Ziel)', Nenner und Endzeile; kein ABBRUCH.
    std::string const link2 = "tests/unit/link2_" + marke;
    fs::create_symlink("nirgends2_" + marke, fall.repo().pfad() / link2, ec);
    ASSERT_FALSE(ec) << "Symlink nicht anlegbar: " << ec.message();
    ASSERT_FALSE(fall.repo().ist_verfolgt(link2)) << "Arrangement: der Link steht im Index, die Stufe maesse (6b).";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + link2));
    Lauf const untracked = fall.fahren();
    berichten("RichtungEinsBleibtErhalten/Symlink-ohne-Ziel-untracked", untracked, marke);
    EXPECT_EQ(untracked.code, 1) << "Ein untracked Symlink ohne Ziel belegt den Pfad -- ERLOSCHEN, ROT, kein Exit 2.\n"
                                 << untracked.ausgabe;
    EXPECT_TRUE(zeile_exakt(untracked.ausgabe, fall.waise() + " -- ERLOSCHEN: \"" + link2 +
                                                   "\" existiert wieder (als SYMLINK ohne Ziel), die Ausnahme" +
                                                   " traegt nicht mehr"))
        << untracked.ausgabe;
    EXPECT_FALSE(enthaelt(untracked.ausgabe, "ABBRUCH: Werkzeug-Ausfall"))
        << "Die falsche Diagnose (check-ignore 128) darf nicht mehr erscheinen.\n"
        << untracked.ausgabe;
    EXPECT_FALSE(enthaelt(untracked.ausgabe, "datei abwesend")) << untracked.ausgabe;
    EXPECT_TRUE(zeile_exakt(untracked.ausgabe, nenner_davon(0, 1, 0, 0, 0))) << untracked.ausgabe;
    EXPECT_TRUE(zeile_exakt(untracked.ausgabe, endzeile_rot(0, 2, 1, 0, 0, 0))) << untracked.ausgabe;
}

// =============================================================================
// (7) DIE FRIST, BEIDE SEITEN. 'frist:' ist die Form fuer die ehrlich unbeweisbare
//     Ausnahme. Sie muss tragen, solange das Datum laeuft, und ROT werden, sobald es
//     verstrichen ist -- sonst waere sie nur die naechste unbegrenzte Freistellung.
//     Beide Seiten in EINEM Fall, mit demselben Aufbau: nur das Heute unterscheidet sie.
// =============================================================================
TEST(Pa1ToteAusnahme, FristTraegtBisZumTagUndDannNichtMehr) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.allowlist_setzen("frist:2026-09-15"));

    Lauf const vorher = fall.fahren("2026-09-15");
    berichten("FristTraegtBisZumTagUndDannNichtMehr/am-Tag", vorher, marke);
    EXPECT_EQ(vorher.code, 0) << "Am Tag der Frist traegt die Ausnahme noch.\n" << vorher.ausgabe;
    // GANZE ZEILEN (Lens C r4 LC3T-04, Fix-r6): die begruendete Zeile mit Frist und Heute, Nenner und Endzeile.
    EXPECT_TRUE(zeile_exakt(vorher.ausgabe,
                            fall.waise() + " -- Koeder " + marke + " (Frist laeuft bis 2026-09-15, heute 2026-09-15)"))
        << "Das Datum gehoert in die Ausgabe (V-1).\n"
        << vorher.ausgabe;
    EXPECT_TRUE(zeile_exakt(vorher.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << vorher.ausgabe;
    EXPECT_TRUE(zeile_exakt(vorher.ausgabe, endzeile_ok(2, 0))) << vorher.ausgabe;

    Lauf const nachher = fall.fahren("2026-09-16");
    berichten("FristTraegtBisZumTagUndDannNichtMehr/danach", nachher, marke);
    EXPECT_EQ(nachher.code, 1) << "Einen Tag nach der Frist muss die Ausnahme ROT sein.\n" << nachher.ausgabe;
    EXPECT_TRUE(zeile_exakt(nachher.ausgabe, fall.waise() + " -- ABGELAUFEN: die Frist 2026-09-15 ist am 2026-09-16" +
                                                 " verstrichen -- Koeder " + marke))
        << nachher.ausgabe;
    EXPECT_TRUE(zeile_exakt(nachher.ausgabe, nenner_davon(0, 1, 0, 0, 0))) << nachher.ausgabe;
    EXPECT_TRUE(zeile_exakt(nachher.ausgabe, endzeile_rot(0, 2, 1, 0, 0, 0))) << nachher.ausgabe;

    // Die Herkunft des Heute muss im Nenner stehen: ein verschiebbarer Zeitbegriff, den
    // niemand sieht, waere selbst wieder ein Freibrief.
    EXPECT_TRUE(zeile_exakt(nachher.ausgabe, "Heute (fuer 'frist:'): 2026-09-16 -- Herkunft: COMDARE_WACHE_HEUTE"
                                             " (ueberschrieben)."))
        << "Die Wache verschweigt, dass ihr Heute vorgegeben wurde.\n"
        << nachher.ausgabe;
}

// =============================================================================
// (8) AM ECHTEN OBJEKT (T-7): die COMMITTETE Allowlist darf keine tote Zeile tragen.
//     Verfahren: ein Bau-Baum ueber dem ECHTEN Repo, dem GENAU die vier geparkten
//     Dateien fehlen -- damit werden genau ihre Zeilen ausgewertet, von der echten
//     Wache, gegen den echten Baum. Das ist derselbe Aufbau, der den Befund gefunden
//     hat; er bleibt ab hier stehen.
//
//     BEIDE MENGEN: geprueft sind die vier 'frist:'-Zeilen. NICHT geprueft ist die
//     'isa:'-Zeile (test_ap5_simd_extension_coherence) -- ihr Fehlen laesst sich hier
//     nicht ehrlich herstellen, s. Kopf.
//
//     NACHTRAG 2026-09-17 (Owner-Order 206): die vier 'frist:'-Zeilen gibt es nicht mehr; der
//     AUFBAU dieses Falls bleibt Byte fuer Byte derselbe, aber der GEGENSTAND ist jetzt die
//     ARCHIV-Klasse. Die vier Dateien fehlen dem Baum weiterhin -- gruen sind sie nur, weil die
//     Wache sie wegen tests/deprecated/prt_art_legacy_waisen/VERMERK.md gar nicht erst in ihren
//     SOLL nimmt. Genau das prueft der Fall ab hier: Exit 0 UND eine ARCHIV-Zeile, die alle vier
//     namentlich auffuehrt. Faellt der Anker weg, faellt dieser Fall -- und zwar laut.
//     Die 'isa:'-Zeile bleibt aus demselben Grund wie oben ungeprueft; sie ist nach dem Entscheid
//     die einzige verbliebene wirksame Zeile der committeten Allowlist.
// =============================================================================
// RAII fuer den Wegwerf-Baum: die Klasse WegwerfBaum steht seit Fix-r7 im anonymen Namensraum vor Fall (sie
// traegt jetzt auch Fall::baum_; Lens C r5 LC5T-04). Bis 9223cbd5 stand remove_all erst am Ende dieses Falls
// (Lens C r4 LC3T-09, Fix-r6): jedes fatale ASSERT davor liess den Baum unter user_tmp_dir() stehen.
TEST(Pa1ToteAusnahme, EchteAllowlistTraegtKeineToteZeile) {
    std::string const marke = koeder();
    WegwerfBaum const waechter{comdare::test::user_tmp_dir() / ("pa1_echt_" + marke)};
    ASSERT_TRUE(waechter.ok()) << waechter.fehler();
    fs::path const& baum = waechter.pfad();
    ASSERT_TRUE(fs::is_directory(baum)) << "Wegwerf-Baum nicht anlegbar: " << baum.string();

    // Der SOLL der Wache, aus derselben Quelle wie bei ihr: 'git -c core.quotePath=false ls-files' (Fix-r6, Lens
    // A r6 LA6-02: ein Nicht-ASCII-Pfad zaehlt bei beiden gleich). NUR git laeuft in der Shell, und sein Status
    // wird geprueft (Lens B r6 LB6-02 = Lens C r5 LC5T-01 = I-4, Fix-r7): bis 8ae59179 hing hinter git ein Rohr
    // aus drei grep und 'sort -u' -- fahre() nutzt popen (/bin/sh = dash, kein pipefail), soll.code war der
    // Status von 'sort -u', und ein git, das die halbe Liste liefert und stirbt, blieb unentdeckt (Rot zuerst:
    // git-Koeder 'head -n 200; exit 1', FIX-r7.md). Die drei Filter und sort/unique sind jetzt C++.
    // PFLEGE-KOPPLUNG mit der Wache (scripts/ci_test_registrierungs_wache.sh, SOLL-Lesung ueber index.txt:
    // grep -v '^ext/', grep -v '/ext/', grep -E '(^|/)test_[^/]*\.cpp$', dann sort -u): aendert sich dort ein
    // Filter, ist er HIER nachzuziehen, sonst faellt dieser Fall laut (der SOLL des Falls und der Nenner der
    // Wache weichen ab). Eine von git quotierte Zeile (beginnt mit '"') faellt bei der Wache aus dem SOLL heraus
    // (UNPRUEFBAR, Fix-r7 Folge (14d)) und wird hier ebenso uebergangen; am echten Baum gibt es keine (Nenner-
    // Zeile 'Quotierte Index-Pfade: 0', Fall (31d) belegt die Klasse im Wegwerf-Repo).
    Lauf const soll = fahre("cd " + zitiert(fs::path{repo_wurzel()}) + " && " + WegwerfRepo::umgebung() +
                            " git -c core.quotePath=false ls-files");
    ASSERT_EQ(soll.code, 0) << "git ls-files im echten Repo fehlgeschlagen:\n" << soll.ausgabe;

    std::set<std::string> gefiltert;
    for (std::size_t start = 0; start < soll.ausgabe.size();) {
        std::size_t const ende = soll.ausgabe.find('\n', start);
        std::string const z    = soll.ausgabe.substr(start, ende == std::string::npos ? ende : ende - start);
        if (!z.empty() && z.front() != '"' && z.rfind("ext/", 0) != 0 && z.find("/ext/") == std::string::npos) {
            std::size_t const schraeg = z.rfind('/');
            std::string const name    = schraeg == std::string::npos ? z : z.substr(schraeg + 1);
            if (name.rfind("test_", 0) == 0 && name.size() > 4 && name.compare(name.size() - 4, 4, ".cpp") == 0) {
                gefiltert.insert(z);
            }
        }
        if (ende == std::string::npos) { break; }
        start = ende + 1;
    }
    std::vector<std::string> const alle(gefiltert.begin(), gefiltert.end());
    // GEGENPROBE des Messgeraets: ohne belastbaren SOLL beweist der Fall nichts.
    ASSERT_GT(alle.size(), 100U) << "Der SOLL ist unglaubwuerdig klein (" << alle.size() << ") -- fail-closed.";

    std::size_t   weggelassen = 0;
    std::ofstream aus{baum / "compile_commands.json", std::ios::trunc};
    ASSERT_TRUE(aus.good()) << "compile_commands.json nicht schreibbar";
    aus << "[\n";
    bool erste = true;
    for (auto const& datei : alle) {
        bool geparkt = false;
        for (char const* const g : kGeparkt) {
            if (datei == g) { geparkt = true; }
        }
        if (geparkt) {
            ++weggelassen;
            continue;
        }
        if (!erste) { aus << ",\n"; }
        erste = false;
        aus << "  {\"directory\": \"" << baum.string() << "\",\n"
            << "   \"command\": \"c++ -c " << repo_wurzel() << "/" << datei << "\",\n"
            << "   \"file\": \"" << repo_wurzel() << "/" << datei << "\"}";
    }
    aus << "\n]\n";
    aus.close();

    ASSERT_EQ(weggelassen, kGeparktN)
        << "Nicht alle vier geparkten Dateien standen im SOLL -- der Fall maesse etwas anderes.";
    std::size_t const soll_n = alle.size() - weggelassen;

    Lauf const lauf = fahre("cd " + zitiert(fs::path{repo_wurzel()}) + " && " + WegwerfRepo::umgebung() + " sh " +
                            zitiert(wachen_pfad()) + " " + zitiert(baum));
    // ETIKETT (Lens B LB-05, 2026-09-18): 'alle' ist der ROH-Bestand aus git ls-files; der SOLL der Wache
    // ist die Zahl NACH dem Archiv-Abzug. Beide stehen hier, in derselben Sprache wie der Nenner der
    // Wache ("N getrackte ... davon M ARCHIV ... -- SOLL: N-M").
    std::cout << "  [PA-1] Fall 'EchteAllowlistTraegtKeineToteZeile' | getrackt " << alle.size() << " | SOLL "
              << (alle.size() - weggelassen) << " | ARCHIV " << weggelassen
              << " (die vier Archiv-Dateien fehlen dem Baum) | NICHT geprueft: die 'isa:'-Zeile"
              << " | Exit " << lauf.code << "\n";

    EXPECT_EQ(lauf.code, 0) << "Die committete Allowlist traegt eine Zeile, die nie erloeschen kann.\n" << lauf.ausgabe;
    // GANZE davon-ZEILE statt "0 TOTE AUSNAHME" (Lens C r4 LC3T-03, Fix-r6): keine dem Bauweg fehlende Datei des
    // SOLL wird bewertet (die vier fehlen, sind aber ARCHIV und nicht im SOLL).
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_davon(0, 0, 0, 0, 0))) << "Der Nenner muss die Null ausweisen (V-1).\n"
                                                                        << lauf.ausgabe;
    // Die vier fehlen dem Baum und sind trotzdem gruen -- das traegt NUR die ARCHIV-Klasse.
    // Ohne diese Erwartungen waere der Fall auch von einer Wache erfuellt, die sie stillschweigend
    // uebersieht: gemessen wird deshalb die AUSGEWIESENE Menge, nicht nur der Exit.
    // FELD-ANKER statt Teilzeichenkette (Lens B LB-03, 2026-09-18): "4 archiviert" ist Teilzeichenkette
    // von "14 archiviert" -- dieselbe Klasse wie '/build' gegen '/builds' (Kopf). Gepinnt wird deshalb
    // das ganze Feld mit Komma davor und Klammer danach, und die ARCHIV-Zeile mit Datei- UND Ordner-Zahl.
    // MIT DEN GEMESSENEN ZAHLEN (Lens C LCT-09, 2026-09-18): getrackt = alle.size(), ARCHIV = weggelassen
    // (== kGeparktN), SOLL = Differenz -- alle drei Nenner-Zeilen der Wache und ihre Endzeile WOERTLICH.
    // Ein Mutant, der den SOLL falsch zaehlt oder eine Endzeile mit fremden Zahlen druckt, fiele sonst
    // durch die blossen Fragmente "0 TOTE AUSNAHME" und ", 4 archiviert)" hindurch.
    EXPECT_TRUE(zeile_beginnt(lauf.ausgabe, "ARCHIV (tests/deprecated/, VERMERK.md-Anker): " + z(weggelassen) +
                                                " Datei(en) in " + z(kGeparktOrdnerN) + " Ordner(n),"))
        << "Die vier archivierten Dateien muessen als Menge ausgewiesen sein (4 Dateien, 1 Ordner).\n"
        << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_getrackt(alle.size())))
        << "Der Nenner der Wache muss den getrackten Bestand nennen, den dieser Fall selbst gezaehlt hat.\n"
        << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_archiv(weggelassen, kGeparktOrdnerN, soll_n)))
        << "Der Archiv-Abzug muss mit Datei-Zahl, Ordner-Zahl und dem SOLL nach dem Abzug ausgewiesen sein.\n"
        << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0)))
        << "Am echten Objekt muss die Zeile der Klassen ohne Bezug zum Bauweg leer sein (0/0/0/0/0).\n"
        << lauf.ausgabe;
    // DIE NULLSEITE DER QUOTE-NENNER-ZEILE (Lens B r7 LB7-02 = Lens C r6 LC6T-02, Fix-r8): die Zeile 'Quotierte
    // Index-Pfade' steht in JEDEM vollstaendigen Bericht, auch mit 0/0/0 -- ein Mutant, der sie nur bei QUOT_N > 0
    // druckt, ueberlebte 33/33 (M-LB7-quot-nullseite-stumm). Am echten Baum gibt es keinen quotierten Pfad.
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_quotiert(0, 0, 0)))
        << "Die Quote-Nenner-Zeile muss auch mit 0/0/0 stehen (Klasse gehoert leer in den Nenner).\n"
        << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, endzeile_ok(soll_n, weggelassen)))
        << "Die Endzeile muss den SOLL und den Archiv-Nenner mit den gemessenen Zahlen tragen.\n"
        << lauf.ausgabe;
    for (char const* const g : kGeparkt) {
        EXPECT_TRUE(zeile_exakt(lauf.ausgabe, std::string{g})) << "'" << g << "' fehlt in der ARCHIV-Liste der Wache.\n"
                                                               << lauf.ausgabe;
    }
    // Der Wegwerf-Baum faellt mit dem Waechter (RAII, LC3T-09) -- kein remove_all mehr am Ende des Falls.
}

// =============================================================================
// (9) T-6 SCHWESTERPFLICHT -- und eine BERICHTIGUNG der urspruenglichen Annahme.
//
//     Die Annahme lautete: dieselben vier stehen "aus dem spiegelbildlichen Grund" auch
//     in scripts/ci_test_sichtbarkeit_allowlist.txt. Am Objekt gemessen (2026-08-10)
//     stimmt das NICHT MEHR, und zwar zu Recht. Die beiden Stufen haben verschiedene
//     SOLL-Quellen:
//       Stufe 1  SOLL = git ls-files          -> die vier .cpp liegen weiter im Baum
//       Stufe 2  SOLL = die Registrierungs-AUFRUFE im CMake-Quelltext -> seit 8a8f6877
//                                                 entfernt, die vier kommen dort nicht
//                                                 mehr vor
//     Eine Zeile in der Schwesterliste waere daher eine Ausnahme OHNE ANLASS -- und die
//     faengt nur das naechste echte Verschwinden desselben Namens lautlos weg. Genau
//     diese Fehlerform steht im Kopf jener Datei.
//
//     DIESER FALL VERLANGT ALSO DAS GEGENTEIL DER ANNAHME: keiner der vier Namen darf
//     dort als WIRKSAME Zeile stehen. 'Wirksam' heisst: keine Kommentarzeile. Die vier
//     Namen kommen im Kopf jener Datei sehr wohl vor -- in der Begruendung, warum sie
//     entfernt wurden. Ein Orakel, das die ganze Datei nach dem Namen absucht, wuerde
//     diesen Kommentar fuer eine Zeile halten und waere hohl; deshalb wird hier Zeile
//     fuer Zeile das ERSTE FELD verglichen und nicht die Datei nach Teilzeichenketten.
//
//     GEGENPROBE (sonst bewiese ein Nullbefund nichts): der Parser MUSS wirksame Zeilen
//     finden. Findet er keine, ist nicht die Liste sauber, sondern der Parser kaputt.
// =============================================================================
TEST(Pa1ToteAusnahme, T6SchwesterlisteFuehrtDieVierNichtMehr) {
    fs::path const schwester = fs::path{repo_wurzel()} / "scripts/ci_test_sichtbarkeit_allowlist.txt";
    std::ifstream  quelle{schwester};
    ASSERT_TRUE(quelle.good()) << "Schwesterliste nicht lesbar: " << schwester.string();

    std::vector<std::string> wirksam;
    std::string              zeile;
    while (std::getline(quelle, zeile)) {
        std::size_t const erst = zeile.find_first_not_of(" \t\r");
        if (erst == std::string::npos) { continue; }
        if (zeile[erst] == '#') { continue; }
        std::size_t const ende = zeile.find_first_of(" \t", erst);
        wirksam.push_back(zeile.substr(erst, ende == std::string::npos ? ende : ende - erst));
    }

    // GEGENPROBE des Messgeraets vor jedem Nullbefund.
    ASSERT_FALSE(wirksam.empty()) << "Der Parser fand KEINE wirksame Zeile in " << schwester.string()
                                  << " -- ein Nullbefund waere dann eine Aussage ueber den Parser, "
                                     "nicht ueber die Liste. Fail-closed.";

    for (char const* const g : kGeparkt) {
        std::string const datei = std::string{g};
        std::string const name  = datei.substr(datei.rfind('/') + 1, datei.size() - datei.rfind('/') - 1 - 4);
        bool              steht = false;
        for (auto const& w : wirksam) {
            if (w == name) { steht = true; }
        }
        EXPECT_FALSE(steht) << "'" << name << "' steht wieder als wirksame Zeile in " << schwester.string()
                            << ". Seine Registrierung ist aus tests/unit/CMakeLists.txt entfernt, der Name "
                               "kommt im SOLL jener Wache also gar nicht vor -- die Zeile deckt nichts und "
                               "faengt nur das naechste echte Verschwinden desselben Namens lautlos weg.";
    }

    std::cout << "  [PA-1/T-6] Schwesterliste " << schwester.filename().string() << ": " << wirksam.size()
              << " wirksame Zeile(n), davon 0 der 4 geparkten -- richtig, ihr SOLL kennt sie nicht mehr.\n"
              << "  [PA-1/T-6] NICHT geprueft und ausdruecklich OFFEN: jene Liste hat ueberhaupt kein\n"
                 "             Gegenstand-Feld (Format '<name> <begruendung>'). Ihre Wache kennt daher\n"
                 "             KEINE Erloschen-Probe -- weder Richtung 1 noch Richtung 2. Struktur-Nenner:\n"
              << "             " << wirksam.size() << " von " << wirksam.size()
              << " Zeilen ohne nachpruefbaren Gegenstand. Eigenes Paket.\n";
}

// =============================================================================
// (10) DIE ARCHIV-KLASSE, BEIDSEITIG UND MIT NACHBAR-PROBE (Owner 2026-09-17, Order 206).
//      Die Regel lautet: eine getrackte Test-Quelldatei unter tests/deprecated/<ordner>/
//      faellt aus dem SOLL, WENN UND NUR WENN tests/deprecated/<ordner>/VERMERK.md im
//      Index liegt. Dieser Fall faehrt alle drei Zustaende am SELBEN Gegenstand:
//        (a) kein Anker            -> ROT, die Waise steht namentlich als unbegruendet
//        (b) Anker im NACHBARordner -> weiter ROT (ein Anker traegt NUR seinen Ordner)
//        (c) Anker im eigenen Ordner -> GRUEN, die Waise steht in der ARCHIV-Zeile
//      K13 beidseitig: ein Orakel, das immer GRUEN liefert, faellt an (a) und (b); eines,
//      das immer ROT liefert, faellt an (c). (b) ist der Teil, der aus dem "wenn" ein
//      "wenn und nur wenn" macht -- ohne ihn waere die Regel "irgendwo unter
//      tests/deprecated/ liegt ein VERMERK.md" und damit genau der Freibrief, gegen den
//      diese Wache gebaut ist.
//      (a)-(c) AUSDRUECKLICH OHNE ALLOWLIST-ZEILE: die Archiv-Klasse darf nicht an einer Ausnahme
//      haengen, sonst pruefte der Fall die Allowlist und nicht den Ort.
//        (d) Anker PLUS Allowlist-Zeile -> ROT: die Zeile ist eine Ausnahme OHNE ANLASS (LB-01)
//        (e) Anker, Allowlist ohne die Zeile -> wieder GRUEN (Gegenrichtung zu (d))
// =============================================================================
TEST(Pa1ToteAusnahme, ArchivOrdnerZaehltNurMitVermerkAnker) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/archiv_" + marke;
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());

    Lauf const ohne = fall.fahren();
    berichten("ArchivOrdnerZaehltNurMitVermerkAnker/ohne-Anker", ohne, marke);
    EXPECT_EQ(ohne.code, 1) << "Ohne VERMERK.md bleibt die Datei im SOLL -- und fehlt im Bauweg.\n" << ohne.ausgabe;
    EXPECT_TRUE(zeile_beginnt(ohne.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << ohne.ausgabe;
    EXPECT_TRUE(zeile_exakt(ohne.ausgabe, fall.waise()))
        << "Der gewuerfelte Pfad fehlt in der Ausgabe -- der Befund stammt dann nicht aus diesem Fall.\n"
        << ohne.ausgabe;
    // GANZE ZEILEN statt ", 0 archiviert)" (Lens C r4 LC3T-05, Fix-r6): Endzeile, davon-Zeile und Archiv-Nenner --
    // ein Mutant mit falschem SOLL oder Nachbarfeld ueberlebte das blosse Feld-Fragment.
    EXPECT_TRUE(zeile_exakt(ohne.ausgabe, endzeile_rot(1, 2, 0, 0, 0, 0)))
        << "Die Archiv-Klasse gehoert auch dann in den Nenner, wenn sie leer ist (V-1).\n"
        << ohne.ausgabe;
    EXPECT_TRUE(zeile_exakt(ohne.ausgabe, nenner_davon(0, 0, 0, 0, 1))) << ohne.ausgabe;
    EXPECT_TRUE(zeile_exakt(ohne.ausgabe, nenner_archiv(0, 0, 2))) << ohne.ausgabe;

    ASSERT_TRUE(fall.repo().schreibe_und_verfolge("tests/deprecated/anderer_" + marke + "/VERMERK.md",
                                                  "# Nachbar-Anker " + marke + "\n"));
    Lauf const nachbar = fall.fahren();
    berichten("ArchivOrdnerZaehltNurMitVermerkAnker/Nachbar-Anker", nachbar, marke);
    EXPECT_EQ(nachbar.code, 1) << "Ein Anker im NACHBARordner darf nicht tragen.\n" << nachbar.ausgabe;
    EXPECT_TRUE(zeile_exakt(nachbar.ausgabe, endzeile_rot(1, 2, 0, 0, 0, 0)))
        << "Der fremde Anker hat etwas archiviert.\n"
        << nachbar.ausgabe;
    EXPECT_TRUE(zeile_exakt(nachbar.ausgabe, nenner_archiv(0, 0, 2))) << nachbar.ausgabe;
    // ROT AM GEGENSTAND (Lens C LCT-08, 2026-09-18): nicht irgendein Rot, sondern die Waise OHNE BEGRUENDUNG.
    EXPECT_TRUE(zeile_beginnt(nachbar.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << nachbar.ausgabe;
    EXPECT_TRUE(zeile_exakt(nachbar.ausgabe, fall.waise()))
        << "Der Waisen-Pfad fehlt -- das Rot stammt dann nicht aus diesem Fall.\n"
        << nachbar.ausgabe;

    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/VERMERK.md", "# Archiv-Anker " + marke + "\n"));
    Lauf const mit = fall.fahren();
    berichten("ArchivOrdnerZaehltNurMitVermerkAnker/mit-Anker", mit, marke);
    EXPECT_EQ(mit.code, 0) << "Mit eigenem VERMERK.md ist die Datei ARCHIV und nicht mehr im SOLL.\n" << mit.ausgabe;
    EXPECT_TRUE(zeile_beginnt(mit.ausgabe, "ARCHIV (tests/deprecated/, VERMERK.md-Anker): 1 Datei(en) in 1 Ordner(n),"))
        << "Die Archiv-Menge muss mit Datei- und Ordner-Zahl ausgewiesen sein, sonst schrumpft der Nenner lautlos.\n"
        << mit.ausgabe;
    EXPECT_TRUE(zeile_exakt(mit.ausgabe, fall.waise())) << "Die archivierte Datei muss namentlich erscheinen.\n"
                                                        << mit.ausgabe;
    EXPECT_TRUE(zeile_exakt(mit.ausgabe, endzeile_ok(1, 1))) << "Die Endzeile muss den Archiv-Nenner tragen.\n"
                                                             << mit.ausgabe;
    EXPECT_TRUE(zeile_exakt(mit.ausgabe, nenner_getrackt(2))) << mit.ausgabe;
    EXPECT_TRUE(zeile_exakt(mit.ausgabe, nenner_archiv(1, 1, 1))) << mit.ausgabe;
    EXPECT_TRUE(zeile_exakt(mit.ausgabe, nenner_davon(0, 0, 0, 0, 0))) << mit.ausgabe;

    // (d) EINE ALLOWLIST-ZEILE FUER DIE ARCHIVIERTE DATEI (Lens B LB-01, 2026-09-18). Die Allowlist-
    //     Schleife der Wache laeuft nur ueber die Dateien, die dem SOLL fehlen; eine archivierte Datei
    //     steht nicht im SOLL, ihre Zeile wuerde also NIE ausgewertet -- eine abgelaufene Frist bliebe
    //     still gruen (Lens-B-Probe P10: 'frist:2000-01-01' -> Exit 0). Genau das ist die "Ausnahme
    //     OHNE ANLASS" aus Fall (9). Die Wache muss die Zeile als UNPRUEFBAR melden: ROT, namentlich,
    //     und mit "0 erloschen" -- sie darf die Frist gar nicht erst bewerten, der Ort traegt.
    //     ROT ZUERST: gegen die Wache vom Stand 54296857 war diese Stufe GRUEN (Exit 0) -- Beleg im
    //     Beweisort des OV-2-Zuges (FIX-r1.md).
    ASSERT_TRUE(fall.allowlist_setzen("frist:2000-01-01"));
    Lauf const zeile = fall.fahren();
    berichten("ArchivOrdnerZaehltNurMitVermerkAnker/Anker-plus-Allowlist-Zeile", zeile, marke);
    EXPECT_EQ(zeile.code, 1) << "Eine Allowlist-Zeile fuer eine ARCHIV-Datei ist eine Ausnahme ohne Anlass -- ROT.\n"
                             << zeile.ausgabe;
    EXPECT_TRUE(zeile_beginnt(zeile.ausgabe, fall.waise() + " -- UNPRUEFBAR: ARCHIV-Datei mit Allowlist-Zeile"))
        << "Die Zeile muss namentlich und mit ihrer Klasse gemeldet werden.\n"
        << zeile.ausgabe;
    EXPECT_TRUE(enthaelt(zeile.ausgabe, "Ausnahme ohne Anlass")) << zeile.ausgabe;
    // GANZE ZEILEN statt Fragmente (Lens C LCT-10/LCT-11): Endzeile mit "1 unpruefbar" und ", 1 archiviert)",
    // die Klassen-Zeile mit "1 Allowlist-Zeile(n) fuer ARCHIV-Dateien", und "0 erloschen" in der Endzeile --
    // die Frist darf nicht bewertet worden sein, die Zeile hat keinen Gegenstand im SOLL.
    EXPECT_TRUE(zeile_exakt(zeile.ausgabe, endzeile_rot(0, 1, 0, 0, 1, 1)))
        << "Die Endzeile muss die Zeile als UNPRUEFBAR zaehlen, die Datei bleibt ARCHIV, 0 erloschen.\n"
        << zeile.ausgabe;
    EXPECT_TRUE(zeile_exakt(zeile.ausgabe, nenner_ohne_bauweg(0, 1, 0, 0, 0)))
        << "Der Nenner muss die Klasse getrennt zaehlen (V-1), in der ganzen Zeile.\n"
        << zeile.ausgabe;
    EXPECT_TRUE(zeile_exakt(zeile.ausgabe, nenner_davon(0, 0, 0, 0, 0)))
        << "Keine dem Bauweg fehlende Datei ist bewertet worden -- der Ort traegt.\n"
        << zeile.ausgabe;

    // (e) GEGENRICHTUNG zu (d): dieselbe Allowlist-Datei OHNE die Zeile -> wieder GRUEN. Damit ist
    //     belegt, dass (d) an der ZEILE hing und nicht an der blossen Anwesenheit einer Allowlist.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt", "# ohne Zeile " + marke + "\n"));
    Lauf const ohne_zeile = fall.fahren();
    berichten("ArchivOrdnerZaehltNurMitVermerkAnker/Anker-ohne-Allowlist-Zeile", ohne_zeile, marke);
    EXPECT_EQ(ohne_zeile.code, 0) << "Ohne die Zeile muss der Anker allein wieder tragen.\n" << ohne_zeile.ausgabe;
    EXPECT_TRUE(zeile_exakt(ohne_zeile.ausgabe, endzeile_ok(1, 1))) << ohne_zeile.ausgabe;
    EXPECT_TRUE(zeile_exakt(ohne_zeile.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0))) << ohne_zeile.ausgabe;
}

// =============================================================================
// (11) DIE DREI GRENZEN DER ARCHIV-REGEL, je mit Test-Traeger (Lens B LB-02, 2026-09-18).
//      Der Kopf der Wache behauptet drei Grenzen; am Objekt stimmten sie (Lens-Proben P1/P2/P4),
//      aber "Skripte sagen gar nichts" -- ohne Google-Test-Deckung bleibt jede Grenze Behauptung:
//        (a) eine Datei DIREKT unter tests/deprecated/ (ohne Ordner) hat keinen Anker und bleibt im
//            SOLL -- auch wenn tests/deprecated/VERMERK.md im Index liegt;
//        (b) ein VERMERK.md TIEFER als das dritte Pfadsegment ankert nichts -- der Anker des dritten
//            Segments dagegen traegt auch tiefer liegende Dateien (Gegenrichtung);
//        (c) ein VERMERK.md, das nur im ARBEITSBAUM liegt (nicht im Index), ankert nichts --
//            dieselbe Datei per 'git add' traegt (Gegenrichtung).
//      Jede Stufe misst Exit-Code UND Literal: die OHNE-BEGRUENDUNG-Zeile mit dem Waisen-Pfad und
//      den Archiv-Nenner ", 0 archiviert)" (Feld-Anker, s. Fall (8)).
//      ROT ZUERST am Mutanten: jede Stufe wurde gegen eine absichtlich aufgeweichte Kopie der Wache
//      gefahren (Anker-Muster ohne Segment-Grenze; Anker aus dem Arbeitsbaum statt aus dem Index)
//      und war dort rot -- Beleg im Beweisort des OV-2-Zuges (FIX-r1.md).
// =============================================================================
TEST(Pa1ToteAusnahme, ArchivGrenzeDateiDirektUnterDeprecatedBleibtImSoll) {
    std::string const marke = koeder();
    Fall              fall{marke, "tests/deprecated/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());
    // Der Anker liegt eine Ebene ZU HOCH: tests/deprecated/VERMERK.md ist kein <ordner>-Anker.
    ASSERT_TRUE(
        fall.repo().schreibe_und_verfolge("tests/deprecated/VERMERK.md", "# kein Ordner-Anker " + marke + "\n"));

    Lauf const lauf = fall.fahren();
    berichten("ArchivGrenzeDateiDirektUnterDeprecatedBleibtImSoll", lauf, marke);
    EXPECT_EQ(lauf.code, 1) << "Eine Datei direkt unter tests/deprecated/ hat keinen Anker und bleibt im SOLL.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(zeile_beginnt(lauf.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, fall.waise())) << "Der gewuerfelte Pfad fehlt in der Ausgabe.\n"
                                                         << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, endzeile_rot(1, 2, 0, 0, 0, 0)))
        << "tests/deprecated/VERMERK.md hat etwas archiviert.\n"
        << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_davon(0, 0, 0, 0, 1))) << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_archiv(0, 0, 2))) << lauf.ausgabe;

    // (b) GEGENRICHTUNG (Lens C LCT-07, 2026-09-18): DERSELBE Baum, dieselbe Regel. Eine zweite Waise unter
    //     einem <ordner> mit eigenem VERMERK.md wird archiviert; die Datei direkt unter tests/deprecated/
    //     nimmt der Bauweg jetzt auf, damit allein der ORT den Unterschied zwischen (a) und (b) macht.
    std::string const ordner = "tests/deprecated/mit_ordner_" + marke;
    std::string const zweite = ordner + "/test_waise2_" + marke + ".cpp";
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(zweite, "// zweite Waise " + marke + "\n"));
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/VERMERK.md", "# Ordner-Anker " + marke + "\n"));
    ASSERT_TRUE(fall.bauweg_schreiben({kGegenprobe, fall.waise()}));
    Lauf const mit_ordner = fall.fahren();
    berichten("ArchivGrenzeDateiDirektUnterDeprecatedBleibtImSoll/Gegenrichtung-Ordner-Anker", mit_ordner, marke);
    EXPECT_EQ(mit_ordner.code, 0) << "Unter einem <ordner> mit Anker muss dieselbe Regel gruen tragen.\n"
                                  << mit_ordner.ausgabe;
    EXPECT_TRUE(zeile_exakt(mit_ordner.ausgabe, zweite)) << "Die archivierte Datei muss namentlich erscheinen.\n"
                                                         << mit_ordner.ausgabe;
    EXPECT_TRUE(zeile_exakt(mit_ordner.ausgabe, endzeile_ok(2, 1))) << mit_ordner.ausgabe;
    EXPECT_TRUE(zeile_exakt(mit_ordner.ausgabe, nenner_archiv(1, 1, 2))) << mit_ordner.ausgabe;
    EXPECT_TRUE(zeile_exakt(mit_ordner.ausgabe, nenner_davon(0, 0, 0, 0, 0))) << mit_ordner.ausgabe;
}

TEST(Pa1ToteAusnahme, ArchivGrenzeAnkerTieferAlsDrittesSegmentAnkertNicht) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/tief_" + marke;
    Fall              fall{marke, ordner + "/unter/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());
    // Der Anker liegt NEBEN der Datei, aber im VIERTEN Segment -- <ordner> ist genau das dritte.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/unter/VERMERK.md", "# zu tiefer Anker " + marke + "\n"));

    Lauf const tief = fall.fahren();
    berichten("ArchivGrenzeAnkerTieferAlsDrittesSegmentAnkertNicht/Anker-im-vierten-Segment", tief, marke);
    EXPECT_EQ(tief.code, 1) << "Ein VERMERK.md tiefer als das dritte Pfadsegment darf nicht ankern.\n" << tief.ausgabe;
    EXPECT_TRUE(zeile_beginnt(tief.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << tief.ausgabe;
    EXPECT_TRUE(zeile_exakt(tief.ausgabe, fall.waise())) << tief.ausgabe;
    EXPECT_TRUE(zeile_exakt(tief.ausgabe, endzeile_rot(1, 2, 0, 0, 0, 0)))
        << "Der zu tiefe Anker hat etwas archiviert.\n"
        << tief.ausgabe;
    EXPECT_TRUE(zeile_exakt(tief.ausgabe, nenner_archiv(0, 0, 2))) << tief.ausgabe;

    // GEGENRICHTUNG: der Anker im dritten Segment traegt auch die tiefer liegende Datei.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/VERMERK.md", "# Ordner-Anker " + marke + "\n"));
    Lauf const drittes = fall.fahren();
    berichten("ArchivGrenzeAnkerTieferAlsDrittesSegmentAnkertNicht/Anker-im-dritten-Segment", drittes, marke);
    EXPECT_EQ(drittes.code, 0) << "Der Anker im dritten Segment muss den ganzen Ordner tragen.\n" << drittes.ausgabe;
    EXPECT_TRUE(zeile_exakt(drittes.ausgabe, fall.waise())) << drittes.ausgabe;
    EXPECT_TRUE(zeile_exakt(drittes.ausgabe, endzeile_ok(1, 1))) << drittes.ausgabe;
    EXPECT_TRUE(zeile_exakt(drittes.ausgabe, nenner_archiv(1, 1, 1))) << drittes.ausgabe;
}

TEST(Pa1ToteAusnahme, ArchivGrenzeAnkerNurImArbeitsbaumAnkertNicht) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/ungetrackt_" + marke;
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());
    // repo().schreibe statt schreibe_und_verfolge: die Datei liegt im Arbeitsbaum, NICHT im Index.
    ASSERT_TRUE(fall.repo().schreibe(ordner + "/VERMERK.md", "# ungetrackter Anker " + marke + "\n"));
    ASSERT_FALSE(fall.repo().ist_verfolgt(ordner + "/VERMERK.md"))
        << "Das Arrangement ist falsch: der Anker steht im Index, die Stufe maesse nichts.";

    Lauf const ungetrackt = fall.fahren();
    berichten("ArchivGrenzeAnkerNurImArbeitsbaumAnkertNicht/nur-Arbeitsbaum", ungetrackt, marke);
    EXPECT_EQ(ungetrackt.code, 1) << "Ein VERMERK.md nur im Arbeitsbaum darf nicht ankern.\n" << ungetrackt.ausgabe;
    EXPECT_TRUE(zeile_beginnt(ungetrackt.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << ungetrackt.ausgabe;
    EXPECT_TRUE(zeile_exakt(ungetrackt.ausgabe, fall.waise())) << ungetrackt.ausgabe;
    EXPECT_TRUE(zeile_exakt(ungetrackt.ausgabe, endzeile_rot(1, 2, 0, 0, 0, 0)))
        << "Der ungetrackte Anker hat etwas archiviert.\n"
        << ungetrackt.ausgabe;
    EXPECT_TRUE(zeile_exakt(ungetrackt.ausgabe, nenner_archiv(0, 0, 2))) << ungetrackt.ausgabe;

    // GEGENRICHTUNG: dieselbe Datei im Index traegt.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/VERMERK.md", "# jetzt getrackter Anker " + marke + "\n"));
    Lauf const getrackt = fall.fahren();
    berichten("ArchivGrenzeAnkerNurImArbeitsbaumAnkertNicht/im-Index", getrackt, marke);
    EXPECT_EQ(getrackt.code, 0) << "Derselbe Anker im Index muss tragen.\n" << getrackt.ausgabe;
    EXPECT_TRUE(zeile_exakt(getrackt.ausgabe, endzeile_ok(1, 1))) << getrackt.ausgabe;
    EXPECT_TRUE(zeile_exakt(getrackt.ausgabe, nenner_archiv(1, 1, 1))) << getrackt.ausgabe;
}

// =============================================================================
// (12) DIE FORM DES ANKERS (Lens A LA-04, 2026-09-18). Der Anker traegt die Begruendung der
//      Ablage. Ein Index-Eintrag namens VERMERK.md, der ein Blob mit 0 Byte oder kein regulaeres
//      Blob ist (Symlink, Modus 120000), traegt nichts -- und darf deshalb NICHT ankern. Die
//      Doktrin der Wache macht aus einer leeren Begruendung ROT (Feld 2 leer = UNPRUEFBAR); vor
//      diesem Fall war der Anker die einzige Stelle, an der Leere gruen trug. Drei Stufen am
//      selben Gegenstand:
//        (a) VERMERK.md mit 0 Byte im Index   -> ROT: UNPRUEFBARER ANKER, Waise im SOLL, 0 archiviert
//        (a2) VERMERK.md nur aus Leerraum      -> ROT: UNPRUEFBARER ANKER (ohne Nicht-Leerraum-Zeichen)
//        (b) VERMERK.md als Symlink ins Nichts -> ROT: UNPRUEFBARER ANKER (Modus 120000), 0 archiviert
//        (c) VERMERK.md mit Inhalt             -> GRUEN, 1 archiviert (Gegenrichtung)
//        (d) dasselbe Blob mit Modus 100755    -> GRUEN, 1 archiviert (ausfuehrbar ist regulaer)
//      ROT ZUERST: (a) und (b) waren gegen die Wache vom Stand 54296857 GRUEN (Exit 0, "1 archiviert")
//      -- Beleg im Beweisort des OV-2-Zuges (FIX-r1.md); (a2) war gegen d8e8f53d GRUEN (Lens A LA3-01,
//      Lens B LB2-01, Fix-r2 F1) und (d) faellt am Mutanten, der nur 100644 zulaesst (Lens C LCT-12) --
//      Belege in FIX-r2.md.
// =============================================================================
TEST(Pa1ToteAusnahme, ArchivAnkerOhneInhaltOderFormAnkertNicht) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/hohl_" + marke;
    std::string const anker  = ordner + "/VERMERK.md";
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());

    // (a) 0 Byte, im Index.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(anker, ""));
    Lauf const leer = fall.fahren();
    berichten("ArchivAnkerOhneInhaltOderFormAnkertNicht/0-Byte", leer, marke);
    EXPECT_EQ(leer.code, 1) << "Ein Anker mit 0 Byte traegt keine Begruendung und darf nicht ankern.\n" << leer.ausgabe;
    EXPECT_TRUE(zeile_beginnt(leer.ausgabe, anker + " -- UNPRUEFBARER ANKER"))
        << "Der Anker muss namentlich und mit seiner Klasse gemeldet werden.\n"
        << leer.ausgabe;
    EXPECT_TRUE(zeile_beginnt(leer.ausgabe, anker + " -- UNPRUEFBARER ANKER: Blob mit 0 Byte --")) << leer.ausgabe;
    EXPECT_TRUE(zeile_beginnt(leer.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << leer.ausgabe;
    EXPECT_TRUE(zeile_exakt(leer.ausgabe, fall.waise())) << leer.ausgabe;
    // GANZE ZEILEN (Lens C LCT-10/LCT-11): Endzeile "1 von 2 ohne Begruendung ... 1 unpruefbar, 0 archiviert",
    // Klassen-Zeile "1 ARCHIV-Anker ohne Inhalt/Form" mit allen Nachbarfeldern.
    EXPECT_TRUE(zeile_exakt(leer.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0)))
        << "Die Endzeile muss den Anker als UNPRUEFBAR zaehlen und 0 archiviert melden.\n"
        << leer.ausgabe;
    EXPECT_TRUE(zeile_exakt(leer.ausgabe, nenner_ohne_bauweg(1, 0, 0, 0, 0)))
        << "Der Nenner muss die Klasse getrennt zaehlen (V-1).\n"
        << leer.ausgabe;

    // (a2) NUR LEERRAUM (Lens A LA3-01, Lens B LB2-01; Fix-r2 F1): fuenf Byte aus Zeilenumbruechen, einem
    //      Leerzeichen und einem Tab -- Bytes ohne Inhalt. Die Groesse allein hatte getragen (Exit 0 gegen
    //      d8e8f53d, "1 archiviert"); jetzt zaehlt der Inhalt.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(anker, "\n\n \t\n"));
    Lauf const leerraum = fall.fahren();
    berichten("ArchivAnkerOhneInhaltOderFormAnkertNicht/nur-Leerraum", leerraum, marke);
    EXPECT_EQ(leerraum.code, 1) << "Ein Anker aus Leerraum traegt keine Begruendung und darf nicht ankern.\n"
                                << leerraum.ausgabe;
    EXPECT_TRUE(zeile_beginnt(leerraum.ausgabe,
                              anker + " -- UNPRUEFBARER ANKER: Blob mit 5 Byte, aber ohne Nicht-Leerraum-Zeichen"))
        << "Der Anker muss namentlich, mit Byte-Zahl und Klasse gemeldet werden.\n"
        << leerraum.ausgabe;
    EXPECT_TRUE(zeile_exakt(leerraum.ausgabe, fall.waise())) << leerraum.ausgabe;
    EXPECT_TRUE(zeile_exakt(leerraum.ausgabe, nenner_ohne_bauweg(1, 0, 0, 0, 0))) << leerraum.ausgabe;
    EXPECT_TRUE(zeile_exakt(leerraum.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0))) << leerraum.ausgabe;

    // (b) derselbe Pfad als Symlink auf ein Ziel, das es nicht gibt. 'git add' legt ihn mit Modus
    //     120000 in den Index; das Blob traegt nur den Link-Text.
    fs::path const  anker_abs = fall.repo().pfad() / anker;
    std::error_code ec;
    fs::remove(anker_abs, ec);
    ASSERT_FALSE(fs::exists(anker_abs)) << "Der 0-Byte-Anker liess sich nicht entfernen.";
    fs::create_symlink("nirgends_" + marke, anker_abs, ec);
    ASSERT_FALSE(ec) << "Symlink nicht anlegbar: " << ec.message();
    Lauf const add =
        fahre("cd " + zitiert(fall.repo().pfad()) + " && " + WegwerfRepo::umgebung() + " git add -- " + zitiert(anker));
    ASSERT_EQ(add.code, 0) << "git add des Symlinks fehlgeschlagen:\n" << add.ausgabe;
    // EXAKTE INDEX-ZEILE (Lens C r4 LC3T-08, Fix-r6): Modus, Blob des Link-Texts, Stufe 0 und Pfad in EINER Zeile.
    Lauf const lsha = im_repo(fall.repo(), "printf '%s' 'nirgends_" + marke + "' | git hash-object --stdin");
    ASSERT_EQ(lsha.ausgabe.size(), 40U) << "kein SHA-1: '" << lsha.ausgabe << "'";
    Lauf const modus = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(anker));
    ASSERT_TRUE(zeile_exakt(modus.ausgabe, "120000 " + lsha.ausgabe + " 0\t" + anker))
        << "Das Arrangement ist falsch: kein Symlink-Eintrag als exakte Index-Zeile:\n"
        << modus.ausgabe;
    Lauf const symlink = fall.fahren();
    berichten("ArchivAnkerOhneInhaltOderFormAnkertNicht/Symlink", symlink, marke);
    EXPECT_EQ(symlink.code, 1) << "Ein Symlink namens VERMERK.md ist kein Anker.\n" << symlink.ausgabe;
    EXPECT_TRUE(zeile_beginnt(symlink.ausgabe, anker + " -- UNPRUEFBARER ANKER")) << symlink.ausgabe;
    EXPECT_TRUE(
        zeile_beginnt(symlink.ausgabe, anker + " -- UNPRUEFBARER ANKER: Index-Modus 120000 ist kein regulaeres Blob"))
        << "Der Modus gehoert in die Meldung (V-1).\n"
        << symlink.ausgabe;
    // ROT AM GEGENSTAND (Lens C LCT-08): die Waise steht OHNE BEGRUENDUNG, die Klassen-Zeile zaehlt den Anker.
    EXPECT_TRUE(zeile_beginnt(symlink.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << symlink.ausgabe;
    EXPECT_TRUE(zeile_exakt(symlink.ausgabe, fall.waise())) << symlink.ausgabe;
    EXPECT_TRUE(zeile_exakt(symlink.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0))) << symlink.ausgabe;
    EXPECT_TRUE(zeile_exakt(symlink.ausgabe, nenner_ohne_bauweg(1, 0, 0, 0, 0))) << symlink.ausgabe;

    // (c) GEGENRICHTUNG: derselbe Pfad mit Inhalt traegt.
    fs::remove(anker_abs, ec);
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(anker, "# Archiv-Anker mit Inhalt " + marke + "\n"));
    Lauf const voll = fall.fahren();
    berichten("ArchivAnkerOhneInhaltOderFormAnkertNicht/mit-Inhalt", voll, marke);
    EXPECT_EQ(voll.code, 0) << "Derselbe Pfad mit Inhalt muss tragen.\n" << voll.ausgabe;
    EXPECT_TRUE(zeile_exakt(voll.ausgabe, endzeile_ok(1, 1))) << voll.ausgabe;
    EXPECT_TRUE(zeile_exakt(voll.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0))) << voll.ausgabe;

    // (d) MODUS 100755 (Lens C LCT-12, 2026-09-18): ein ausfuehrbares Blob ist ein regulaeres Blob und ankert.
    //     ROT ZUERST am Mutanten M4 (nur 100644 zugelassen): dort ist diese Stufe Exit 1 -- Beweisort FIX-r2.md.
    Lauf const chmod = im_repo(fall.repo(), "git update-index --chmod=+x -- " + zitiert(anker));
    ASSERT_EQ(chmod.code, 0) << "git update-index --chmod=+x fehlgeschlagen:\n" << chmod.ausgabe;
    Lauf const asha = im_repo(fall.repo(), "git hash-object -- " + zitiert(anker));
    ASSERT_EQ(asha.ausgabe.size(), 40U) << "kein SHA-1: '" << asha.ausgabe << "'";
    Lauf const modus_x = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(anker));
    ASSERT_TRUE(zeile_exakt(modus_x.ausgabe, "100755 " + asha.ausgabe + " 0\t" + anker))
        << "Das Arrangement ist falsch: kein 100755 als exakte Index-Zeile:\n"
        << modus_x.ausgabe;
    Lauf const ausfuehrbar = fall.fahren();
    berichten("ArchivAnkerOhneInhaltOderFormAnkertNicht/Modus-100755", ausfuehrbar, marke);
    EXPECT_EQ(ausfuehrbar.code, 0) << "Ein Blob mit Modus 100755 ist regulaer und muss ankern.\n"
                                   << ausfuehrbar.ausgabe;
    EXPECT_TRUE(zeile_exakt(ausfuehrbar.ausgabe, endzeile_ok(1, 1))) << ausfuehrbar.ausgabe;
    EXPECT_TRUE(zeile_exakt(ausfuehrbar.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0))) << ausfuehrbar.ausgabe;
}

// =============================================================================
// (12b) DER ANKER IM MERGE-KONFLIKT (Lens C LCW-01, 2026-09-18). Im Konflikt fuehrt der Index einen Pfad
//       auf den Stufen 1 (Basis), 2 (ours) und 3 (theirs) -- ohne Stufe 0, also ohne aufgeloeste Fassung.
//       'git ls-files -s' listet ihn dann bis zu dreimal, jede Zeile mit gueltigem Modus und lesbarem
//       Blob; die Fassung d8e8f53d nahm jede davon als Anker (Exit 0, "1 archiviert" -- Lens A Probe Z1).
//       Ein Anker ohne aufgeloeste Fassung traegt keine Begruendung: UNPRUEFBAR, einmal gemeldet (nicht
//       dreimal), die Datei bleibt im SOLL. Gegenrichtung: derselbe Pfad per 'git add' aufgeloest traegt.
// =============================================================================
TEST(Pa1ToteAusnahme, ArchivAnkerImMergeKonfliktAnkertNicht) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/konflikt_" + marke;
    std::string const anker  = ordner + "/VERMERK.md";
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());

    // (a) aufgeloest (Stufe 0): traegt -- das Arrangement vor dem Konflikt.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(anker, "# Anker vor dem Konflikt " + marke + "\n"));
    Lauf const vorher = fall.fahren();
    berichten("ArchivAnkerImMergeKonfliktAnkertNicht/Stufe-0", vorher, marke);
    EXPECT_EQ(vorher.code, 0) << vorher.ausgabe;
    EXPECT_TRUE(zeile_exakt(vorher.ausgabe, endzeile_ok(1, 1))) << vorher.ausgabe;

    // (b) der Konflikt: das Blob bleibt in der Objektdatenbank, Stufe 0 wird entfernt, Stufen 1-3 gesetzt
    //     (git update-index --index-info, das Rezept der git-Dokumentation).
    Lauf const sha = im_repo(fall.repo(), "git hash-object -w -- " + zitiert(anker));
    ASSERT_EQ(sha.code, 0) << "git hash-object fehlgeschlagen:\n" << sha.ausgabe;
    ASSERT_EQ(sha.ausgabe.size(), 40U) << "kein SHA-1: '" << sha.ausgabe << "'";
    std::string const rezept = "0 0000000000000000000000000000000000000000\t" + anker + "\n" + "100644 " + sha.ausgabe +
                               " 1\t" + anker + "\n" + "100644 " + sha.ausgabe + " 2\t" + anker + "\n" + "100644 " +
                               sha.ausgabe + " 3\t" + anker + "\n";
    ASSERT_TRUE(fall.repo().schreibe("konflikt_" + marke + ".txt", rezept));
    Lauf const konflikt = im_repo(fall.repo(), "git update-index --index-info < " +
                                                   zitiert(fall.repo().pfad() / ("konflikt_" + marke + ".txt")));
    ASSERT_EQ(konflikt.code, 0) << "git update-index --index-info fehlgeschlagen:\n" << konflikt.ausgabe;
    // EXAKTE INDEX-ZEILEN je Stufe, auch Stufe 2 (Lens C r4 LC3T-08, Fix-r6): das Rezept legt drei Zeilen an.
    Lauf const stufen = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(anker));
    ASSERT_TRUE(zeile_exakt(stufen.ausgabe, "100644 " + sha.ausgabe + " 1\t" + anker))
        << "Arrangement: keine Stufe 1:\n"
        << stufen.ausgabe;
    ASSERT_TRUE(zeile_exakt(stufen.ausgabe, "100644 " + sha.ausgabe + " 2\t" + anker))
        << "Arrangement: keine Stufe 2:\n"
        << stufen.ausgabe;
    ASSERT_TRUE(zeile_exakt(stufen.ausgabe, "100644 " + sha.ausgabe + " 3\t" + anker))
        << "Arrangement: keine Stufe 3:\n"
        << stufen.ausgabe;
    ASSERT_FALSE(enthaelt(stufen.ausgabe, " 0\t" + anker)) << "Arrangement: Stufe 0 steht noch:\n" << stufen.ausgabe;

    Lauf const im_konflikt = fall.fahren();
    berichten("ArchivAnkerImMergeKonfliktAnkertNicht/Stufen-1-2-3", im_konflikt, marke);
    EXPECT_EQ(im_konflikt.code, 1) << "Ein Anker im Merge-Konflikt hat keine aufgeloeste Fassung -- ROT.\n"
                                   << im_konflikt.ausgabe;
    EXPECT_TRUE(
        zeile_beginnt(im_konflikt.ausgabe,
                      anker + " -- UNPRUEFBARER ANKER: Index-Stufe 1 statt 0 (Merge-Konflikt, keine aufgeloeste"))
        << "Der Anker muss namentlich, mit Stufe und Klasse gemeldet werden.\n"
        << im_konflikt.ausgabe;
    EXPECT_TRUE(zeile_beginnt(im_konflikt.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << im_konflikt.ausgabe;
    EXPECT_TRUE(zeile_exakt(im_konflikt.ausgabe, fall.waise())) << im_konflikt.ausgabe;
    // EINMAL gezaehlt, obwohl der Index den Pfad dreimal fuehrt.
    EXPECT_TRUE(zeile_exakt(im_konflikt.ausgabe, nenner_ohne_bauweg(1, 0, 0, 0, 0)))
        << "Drei Stufen sind EIN unpruefbarer Anker, nicht drei.\n"
        << im_konflikt.ausgabe;
    EXPECT_TRUE(zeile_exakt(im_konflikt.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0))) << im_konflikt.ausgabe;

    // (c) GEGENRICHTUNG: 'git add' loest den Konflikt (Stufe 0, Stufen 1-3 weg) -- derselbe Pfad traegt.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(anker, "# Anker nach dem Konflikt " + marke + "\n"));
    Lauf const sha_neu = im_repo(fall.repo(), "git hash-object -- " + zitiert(anker));
    ASSERT_EQ(sha_neu.ausgabe.size(), 40U) << "kein SHA-1: '" << sha_neu.ausgabe << "'";
    Lauf const geloest_probe = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(anker));
    ASSERT_TRUE(zeile_exakt(geloest_probe.ausgabe, "100644 " + sha_neu.ausgabe + " 0\t" + anker))
        << geloest_probe.ausgabe;
    ASSERT_FALSE(enthaelt(geloest_probe.ausgabe, " 1\t" + anker)) << geloest_probe.ausgabe;
    Lauf const geloest = fall.fahren();
    berichten("ArchivAnkerImMergeKonfliktAnkertNicht/aufgeloest", geloest, marke);
    EXPECT_EQ(geloest.code, 0) << "Der aufgeloeste Anker muss wieder tragen.\n" << geloest.ausgabe;
    EXPECT_TRUE(zeile_exakt(geloest.ausgabe, endzeile_ok(1, 1))) << geloest.ausgabe;
}

// =============================================================================
// (13) DIE SCHWESTERKLASSE ZU (10d) (Lens A LA-08, 2026-09-18): eine Allowlist-Zeile, deren Feld 1
//      KEINE Datei des SOLL-Bestands nennt (getrackte Test-Quelldatei ausserhalb ext/: geloescht,
//      umbenannt, unter ext/, anders geschrieben), wird genauso nie ausgewertet -- und erwacht mit dem
//      naechsten Namensgleichen als Freibrief. Der Fall hiess bis Fix-r2 "...ImIndex..."; das Etikett
//      war falsch (Lens A LA3-02, Lens C LCW-07): eine Datei unter ext/ steht im Index, aber nicht im
//      SOLL-Bestand -- die Referenzmenge ist soll_roh.txt, nicht der Index.
//      Aufbau: eine BEGRUENDETE Waise (Zeile wie in Fall (2), sie traegt) plus eine zweite Zeile fuer
//      einen Geist. ROT darf dann NUR die Geist-Zeile sein: "davon 1 begruendet" bleibt stehen.
//      Gegenrichtung: dieselbe Allowlist ohne die Geist-Zeile -> GRUEN.
//        (c) Zeilen NUR aus Leerraum sind Leerzeilen, keine Datenzeilen (Lens A LA3-03, Fix-r2 F3):
//            gruen, so wie test_mt_l4_registrierungs_wache_isa Fall (8) sie liest. Gegen d8e8f53d ROT.
//        (d) ein EINGERUECKTER Kommentar ist fuer beide Leser eine Datenzeile ohne Gegenstand (Lens A
//            LA3-04, Lens B LB2-02): rot, und die Meldung nennt die naheliegende Ursache (M1).
// =============================================================================
TEST(Pa1ToteAusnahme, AllowlistZeileOhneGegenstandImSollBestandIstUnpruefbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    std::string const lebendig = "tests/unit/kommt_vielleicht_" + marke + ".hpp";
    std::string const geist    = "tests/unit/test_geist_" + marke + ".cpp";
    std::string const tragend  = fall.waise() + " | datei:" + lebendig + " | Koeder " + marke + "\n";
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + tragend + geist +
                                         " | frist:2999-12-31 | Geist " + marke + "\n"));

    Lauf const mit = fall.fahren();
    berichten("AllowlistZeileOhneGegenstandImSollBestandIstUnpruefbar/mit-Geist-Zeile", mit, marke);
    EXPECT_EQ(mit.code, 1) << "Eine Zeile ohne Gegenstand im SOLL-Bestand ist ein schlafender Freibrief -- ROT.\n"
                           << mit.ausgabe;
    EXPECT_TRUE(zeile_beginnt(mit.ausgabe, geist + " -- UNPRUEFBAR: Allowlist-Zeile ohne Gegenstand -- Feld 1 steht"
                                                   " nicht im SOLL-Bestand"))
        << mit.ausgabe;
    // GANZE ZEILEN (Lens C LCT-10/LCT-11): "davon 1 begruendet" mit allen Feldern (die tragende Zeile darf
    // nicht mit rot werden), die Klassen-Zeile mit "1 ohne Gegenstand im SOLL-Bestand", die Endzeile mit
    // "0 erloschen" (die Frist der Geist-Zeile darf nicht bewertet worden sein) und "1 unpruefbar".
    EXPECT_TRUE(zeile_exakt(mit.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << mit.ausgabe;
    EXPECT_TRUE(zeile_exakt(mit.ausgabe, nenner_ohne_bauweg(0, 0, 0, 1, 0))) << mit.ausgabe;
    EXPECT_TRUE(zeile_exakt(mit.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << mit.ausgabe;

    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + tragend));
    Lauf const ohne = fall.fahren();
    berichten("AllowlistZeileOhneGegenstandImSollBestandIstUnpruefbar/ohne-Geist-Zeile", ohne, marke);
    EXPECT_EQ(ohne.code, 0) << "Ohne die Geist-Zeile muss die tragende Zeile allein gruen sein.\n" << ohne.ausgabe;
    EXPECT_TRUE(zeile_exakt(ohne.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << ohne.ausgabe;
    EXPECT_TRUE(zeile_exakt(ohne.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0))) << ohne.ausgabe;
    EXPECT_TRUE(zeile_exakt(ohne.ausgabe, endzeile_ok(2, 0))) << ohne.ausgabe;

    // (c) Zeilen nur aus Leerraum (drei Leerzeichen; ein Tab) zwischen Kopf und tragender Zeile: Leerzeilen.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n   \n\t\n" + tragend));
    Lauf const leerraum = fall.fahren();
    berichten("AllowlistZeileOhneGegenstandImSollBestandIstUnpruefbar/Leerraum-Zeilen", leerraum, marke);
    EXPECT_EQ(leerraum.code, 0) << "Eine Zeile nur aus Leerraum ist eine Leerzeile, kein Befund.\n" << leerraum.ausgabe;
    EXPECT_TRUE(zeile_exakt(leerraum.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0))) << leerraum.ausgabe;
    EXPECT_TRUE(zeile_exakt(leerraum.ausgabe, endzeile_ok(2, 0))) << leerraum.ausgabe;

    // (d) ein eingerueckter Kommentar ist eine Datenzeile ohne Gegenstand -- rot, mit der Ursache im Text.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n  # eingerueckt " + marke + "\n" + tragend));
    Lauf const eingerueckt = fall.fahren();
    berichten("AllowlistZeileOhneGegenstandImSollBestandIstUnpruefbar/eingerueckter-Kommentar", eingerueckt, marke);
    EXPECT_EQ(eingerueckt.code, 1) << "Ein eingerueckter Kommentar ist fuer die Wache eine Datenzeile -- ROT.\n"
                                   << eingerueckt.ausgabe;
    EXPECT_TRUE(
        zeile_beginnt(eingerueckt.ausgabe, "# eingerueckt " + marke +
                                               " -- UNPRUEFBAR: Allowlist-Zeile ohne Gegenstand -- Feld 1 beginnt"
                                               " mit '#': ein EINGERUECKTER Kommentar?"))
        << "Die Meldung muss die naheliegende Ursache nennen.\n"
        << eingerueckt.ausgabe;
    EXPECT_TRUE(zeile_exakt(eingerueckt.ausgabe, nenner_ohne_bauweg(0, 0, 0, 1, 0))) << eingerueckt.ausgabe;
    EXPECT_TRUE(zeile_exakt(eingerueckt.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << eingerueckt.ausgabe;
}

// =============================================================================
// (14) DIE LETZTE ZEILE OHNE ZEILENUMBRUCH (Lens C LCW-04 = Lens A LA3-05, hochgestuft; 2026-09-18).
//      'read' liefert am Dateiende nach gelesenen Zeichen einen Status ungleich 0 -- die Schleife lief
//      dort ohne Rumpf, in BEIDEN Lesern der Wache. Eine Geist- oder ARCHIV-Zeile als letzte Zeile ohne
//      Zeilenumbruch blieb ungezaehlt (Exit 0), eine tragende Zeile dort blieb ungelesen (Exit 1, OHNE
//      BEGRUENDUNG). Beide Richtungen; beide gegen d8e8f53d falsch herum.
// =============================================================================
TEST(Pa1ToteAusnahme, AllowlistLetzteZeileOhneZeilenumbruchWirdGelesen) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    std::string const lebendig = "tests/unit/kommt_vielleicht_" + marke + ".hpp";
    std::string const geist    = "tests/unit/test_geist_" + marke + ".cpp";
    std::string const tragend  = fall.waise() + " | datei:" + lebendig + " | Koeder " + marke;

    // (a) die Geist-Zeile als LETZTE Zeile ohne Zeilenumbruch: muss gezaehlt werden -> ROT.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + tragend + "\n" + geist +
                                         " | frist:2999-12-31 | Geist " + marke));
    Lauf const geist_ohne_lf = fall.fahren();
    berichten("AllowlistLetzteZeileOhneZeilenumbruchWirdGelesen/Geist-Zeile-ohne-LF", geist_ohne_lf, marke);
    EXPECT_EQ(geist_ohne_lf.code, 1) << "Die letzte Zeile ohne Zeilenumbruch ist eine Zeile -- der Nachscan muss"
                                        " sie sehen.\n"
                                     << geist_ohne_lf.ausgabe;
    EXPECT_TRUE(zeile_beginnt(geist_ohne_lf.ausgabe, geist + " -- UNPRUEFBAR: Allowlist-Zeile ohne Gegenstand"))
        << geist_ohne_lf.ausgabe;
    EXPECT_TRUE(zeile_exakt(geist_ohne_lf.ausgabe, nenner_ohne_bauweg(0, 0, 0, 1, 0))) << geist_ohne_lf.ausgabe;
    EXPECT_TRUE(zeile_exakt(geist_ohne_lf.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << geist_ohne_lf.ausgabe;

    // (b) die TRAGENDE Zeile als letzte Zeile ohne Zeilenumbruch: muss gelesen werden -> GRUEN.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + tragend));
    Lauf const tragend_ohne_lf = fall.fahren();
    berichten("AllowlistLetzteZeileOhneZeilenumbruchWirdGelesen/tragende-Zeile-ohne-LF", tragend_ohne_lf, marke);
    EXPECT_EQ(tragend_ohne_lf.code, 0) << "Die tragende Zeile ohne Zeilenumbruch muss tragen.\n"
                                       << tragend_ohne_lf.ausgabe;
    EXPECT_TRUE(zeile_beginnt(tragend_ohne_lf.ausgabe, fall.waise() + " -- Koeder " + marke))
        << "Die Wache muss GENAU diese Zeile gelesen haben.\n"
        << tragend_ohne_lf.ausgabe;
    EXPECT_TRUE(zeile_exakt(tragend_ohne_lf.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << tragend_ohne_lf.ausgabe;
    EXPECT_TRUE(zeile_exakt(tragend_ohne_lf.ausgabe, endzeile_ok(2, 0))) << tragend_ohne_lf.ausgabe;
}

// =============================================================================
// (15) EINE ALLOWLIST-ZEILE FUER EINEN PFAD UNTER tests/deprecated/ (Lens C LCW-05, 2026-09-18). Ohne
//      Anker steht die Datei im SOLL, und eine 'frist:'-Zeile truege sie regulaer als begruendet: das
//      Archiv waere durch die Hintertuer wieder eine Frist -- gegen den Geist der Owner-Order 206
//      ("frist Zeilen entfernen"). Gegen d8e8f53d war das GRUEN. Jetzt: die Zeile ist UNPRUEFBAR (ORT),
//      die Datei bleibt OHNE BEGRUENDUNG; mit Anker ist die Zeile weiter rot (ARCHIV-Klasse, Fall 10d);
//      gruen ist allein der Anker OHNE Zeile.
// =============================================================================
TEST(Pa1ToteAusnahme, AllowlistZeileFuerPfadUnterDeprecatedIstUnpruefbar) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/ort_" + marke;
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.allowlist_setzen("frist:2999-12-31"));

    // (a) ohne Anker, mit Zeile: die Zeile ist UNPRUEFBAR, die Datei OHNE BEGRUENDUNG -- nicht begruendet.
    Lauf const ohne_anker = fall.fahren();
    berichten("AllowlistZeileFuerPfadUnterDeprecatedIstUnpruefbar/ohne-Anker-mit-Zeile", ohne_anker, marke);
    EXPECT_EQ(ohne_anker.code, 1) << "Eine Allowlist-Zeile darf einen Archiv-Pfad nie tragen -- ROT.\n"
                                  << ohne_anker.ausgabe;
    EXPECT_TRUE(zeile_beginnt(ohne_anker.ausgabe,
                              fall.waise() + " -- UNPRUEFBAR: Allowlist-Zeile fuer einen Pfad unter tests/deprecated/"))
        << "Die Zeile muss namentlich und mit ihrer Klasse gemeldet werden.\n"
        << ohne_anker.ausgabe;
    EXPECT_TRUE(zeile_beginnt(ohne_anker.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << ohne_anker.ausgabe;
    EXPECT_TRUE(zeile_exakt(ohne_anker.ausgabe, nenner_davon(0, 0, 0, 0, 1)))
        << "Die Frist darf die Datei NICHT begruendet haben.\n"
        << ohne_anker.ausgabe;
    EXPECT_TRUE(zeile_exakt(ohne_anker.ausgabe, nenner_ohne_bauweg(0, 0, 1, 0, 0))) << ohne_anker.ausgabe;
    EXPECT_TRUE(zeile_exakt(ohne_anker.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0))) << ohne_anker.ausgabe;

    // (b) mit Anker, mit Zeile: die Zeile bleibt rot -- jetzt als ARCHIV-Zeile (Fall 10d), der Ort traegt.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/VERMERK.md", "# Anker " + marke + "\n"));
    Lauf const mit_anker = fall.fahren();
    berichten("AllowlistZeileFuerPfadUnterDeprecatedIstUnpruefbar/mit-Anker-mit-Zeile", mit_anker, marke);
    EXPECT_EQ(mit_anker.code, 1) << mit_anker.ausgabe;
    EXPECT_TRUE(zeile_exakt(mit_anker.ausgabe, nenner_ohne_bauweg(0, 1, 0, 0, 0))) << mit_anker.ausgabe;
    EXPECT_TRUE(zeile_exakt(mit_anker.ausgabe, endzeile_rot(0, 1, 0, 0, 1, 1))) << mit_anker.ausgabe;

    // (c) mit Anker, ohne Zeile: der einzige gruene Weg fuer einen Archiv-Pfad.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt", "# ohne Zeile " + marke + "\n"));
    Lauf const nur_anker = fall.fahren();
    berichten("AllowlistZeileFuerPfadUnterDeprecatedIstUnpruefbar/mit-Anker-ohne-Zeile", nur_anker, marke);
    EXPECT_EQ(nur_anker.code, 0) << "Der Anker allein muss tragen.\n" << nur_anker.ausgabe;
    EXPECT_TRUE(zeile_exakt(nur_anker.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0))) << nur_anker.ausgabe;
    EXPECT_TRUE(zeile_exakt(nur_anker.ausgabe, endzeile_ok(1, 1))) << nur_anker.ausgabe;
}

// =============================================================================
// (16) DOPPELTE ALLOWLIST-ZEILEN JE PFAD (Lens C LCW-08, 2026-09-18). allow_zeile() nimmt die erste
//      Zeile; jede weitere schlief und erwachte allein durch die Reihenfolge -- eine abgelaufene oder
//      unpruefbare zweite Zeile blieb still (Lens A Probe X11: Exit 0). Jetzt: der Pfad wird als
//      DOPPELT gemeldet (einmal, mit Haeufigkeit), die erste Zeile traegt weiterhin sichtbar.
// =============================================================================
TEST(Pa1ToteAusnahme, AllowlistDoppelteZeileJePfadIstUnpruefbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    std::string const lebendig = "tests/unit/kommt_vielleicht_" + marke + ".hpp";
    std::string const erste    = fall.waise() + " | datei:" + lebendig + " | erste " + marke + "\n";
    std::string const zweite   = fall.waise() + " | datei:" + lebendig + " | zweite " + marke + "\n";
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + erste + zweite));

    Lauf const doppelt = fall.fahren();
    berichten("AllowlistDoppelteZeileJePfadIstUnpruefbar/zwei-Zeilen", doppelt, marke);
    EXPECT_EQ(doppelt.code, 1) << "Zwei Zeilen fuer einen Pfad: die zweite schlaeft -- ROT.\n" << doppelt.ausgabe;
    EXPECT_TRUE(zeile_beginnt(doppelt.ausgabe, fall.waise() + " -- UNPRUEFBAR: DOPPELTE ALLOWLIST-ZEILE -- Feld 1 steht"
                                                              " 2-mal in der Allowlist"))
        << "Der Pfad muss namentlich, mit Klasse und Haeufigkeit gemeldet werden.\n"
        << doppelt.ausgabe;
    EXPECT_TRUE(zeile_beginnt(doppelt.ausgabe, fall.waise() + " -- erste " + marke))
        << "Die erste Zeile muss weiter sichtbar tragen.\n"
        << doppelt.ausgabe;
    EXPECT_FALSE(enthaelt(doppelt.ausgabe, "zweite " + marke)) << "Die zweite Zeile darf nie gelesen worden sein.\n"
                                                               << doppelt.ausgabe;
    EXPECT_TRUE(zeile_exakt(doppelt.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << doppelt.ausgabe;
    EXPECT_TRUE(zeile_exakt(doppelt.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 1))) << doppelt.ausgabe;
    EXPECT_TRUE(zeile_exakt(doppelt.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << doppelt.ausgabe;

    // GEGENRICHTUNG: dieselbe Allowlist mit genau EINER Zeile fuer den Pfad -> GRUEN.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + erste));
    Lauf const einfach = fall.fahren();
    berichten("AllowlistDoppelteZeileJePfadIstUnpruefbar/eine-Zeile", einfach, marke);
    EXPECT_EQ(einfach.code, 0) << "Eine Zeile je Pfad muss tragen.\n" << einfach.ausgabe;
    EXPECT_TRUE(zeile_exakt(einfach.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0))) << einfach.ausgabe;
    EXPECT_TRUE(zeile_exakt(einfach.ausgabe, endzeile_ok(2, 0))) << einfach.ausgabe;
}

// =============================================================================
// (17) EIN LEERES FELD 3 (Lens C LCW-09, 2026-09-18). Der Drei-Feld-Vertrag der Allowlist verlangt die
//      Begruendung; gegen d8e8f53d lief eine Zeile mit leerem oder fehlendem dritten Feld als BEGRUENDET
//      durch (Exit 0). Jetzt: UNPRUEFBAR, in derselben Liste wie eine unbekannte Art. Gegenrichtung: mit
//      Text traegt dieselbe Zeile.
// =============================================================================
TEST(Pa1ToteAusnahme, AllowlistLeeresFeld3IstUnpruefbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    std::string const lebendig = "tests/unit/kommt_vielleicht_" + marke + ".hpp";
    std::string const klasse   = fall.waise() + " -- UNPRUEFBAR: Feld 3 (Begruendung) ist leer";

    // (a) drittes Feld vorhanden, aber leer.
    ASSERT_TRUE(
        fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                             "# Allowlist des Falls " + marke + "\n" + fall.waise() + " | datei:" + lebendig + " |\n"));
    Lauf const leer = fall.fahren();
    berichten("AllowlistLeeresFeld3IstUnpruefbar/Feld-3-leer", leer, marke);
    EXPECT_EQ(leer.code, 1) << "Eine Ausnahme ohne Begruendungstext ist keine -- ROT.\n" << leer.ausgabe;
    EXPECT_TRUE(zeile_beginnt(leer.ausgabe, klasse)) << leer.ausgabe;
    EXPECT_TRUE(zeile_exakt(leer.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << leer.ausgabe;
    EXPECT_TRUE(zeile_exakt(leer.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << leer.ausgabe;

    // (b) nur zwei Felder, kein dritter Trenner.
    ASSERT_TRUE(
        fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                             "# Allowlist des Falls " + marke + "\n" + fall.waise() + " | datei:" + lebendig + "\n"));
    Lauf const zwei = fall.fahren();
    berichten("AllowlistLeeresFeld3IstUnpruefbar/nur-zwei-Felder", zwei, marke);
    EXPECT_EQ(zwei.code, 1) << "Zwei Felder sind kein Drei-Feld-Vertrag -- ROT.\n" << zwei.ausgabe;
    EXPECT_TRUE(zeile_beginnt(zwei.ausgabe, klasse)) << zwei.ausgabe;
    EXPECT_TRUE(zeile_exakt(zwei.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << zwei.ausgabe;

    // (c) GEGENRICHTUNG: dieselbe Zeile mit Begruendungstext traegt.
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + lebendig));
    Lauf const mit_text = fall.fahren();
    berichten("AllowlistLeeresFeld3IstUnpruefbar/mit-Text", mit_text, marke);
    EXPECT_EQ(mit_text.code, 0) << "Mit Begruendungstext muss dieselbe Zeile tragen.\n" << mit_text.ausgabe;
    EXPECT_TRUE(zeile_exakt(mit_text.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << mit_text.ausgabe;
    EXPECT_TRUE(zeile_exakt(mit_text.ausgabe, endzeile_ok(2, 0))) << mit_text.ausgabe;
}

// =============================================================================
// (18) WERKZEUG-AUSFALL IN DER NENNER-PIPELINE IST EXIT 2 (Lens C LCW-02/LCW-03, 2026-09-18). POSIX-sh
//      kennt kein 'pipefail': in 'git ls-files -s | grep' zaehlte nur der Status von grep, und ein 'wc',
//      das scheitert, hinterliess leere Zaehler. Gegen d8e8f53d meldete die Wache in beiden Faellen
//      "OK ( Quelldateien, ...)" mit Exit 0 -- fail-open an der Stelle, die den Nenner erhebt. Aufbau:
//      ein PATH-Koeder gleichen Namens VOR dem echten Werkzeug; (a) 'git' liefert bei 'ls-files -s' die
//      erste Zeile und stirbt (Teilausgabe = der Anker), (b) 'wc' stirbt ohne Ausgabe. Beide: Exit 2 mit
//      ABBRUCH-Zeile, nie ein OK. (c) ohne Koeder wieder Exit 0.
// =============================================================================
TEST(Pa1ToteAusnahme, WerkzeugAusfallInDerNennerPipelineIstExit2) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/werkzeug_" + marke;
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/VERMERK.md", "# Anker " + marke + "\n"));
    Lauf const gesund = fall.fahren();
    berichten("WerkzeugAusfallInDerNennerPipelineIstExit2/ohne-Koeder", gesund, marke);
    ASSERT_EQ(gesund.code, 0) << "Das Arrangement ist falsch: der gesunde Baum ist nicht gruen.\n" << gesund.ausgabe;
    ASSERT_TRUE(zeile_exakt(gesund.ausgabe, endzeile_ok(1, 1))) << gesund.ausgabe;

    Lauf const wo = im_repo(fall.repo(), "command -v git");
    ASSERT_EQ(wo.code, 0) << wo.ausgabe;
    std::string const echtes_git = wo.ausgabe;
    ASSERT_FALSE(echtes_git.empty());
    fs::path const    bin  = fall.repo().pfad() / "koeder_bin";
    std::string const pfad = "PATH=\"" + bin.string() + ":$PATH\"";

    // (a) 'git ls-files -s' mit Teilausgabe und Exit 1. Der Koeder trifft die ANKER-Lesung an ihrem Argument-Ende
    //     'ls-files -s' (ohne Pathspec): seit Fix-r6 steht '-c core.quotePath=false' davor (Lens A r6 LA6-02), und
    //     die Index-Frage der Erreichbarkeits-Probe ('ls-files -s -- :(literal)...') laeuft weiter echt.
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "git",
                                   "#!/bin/sh\n# PATH-Koeder des Falls " + marke +
                                       ": 'git ... ls-files -s' (Anker-Lesung) liefert die erste Zeile und"
                                       " scheitert dann.\n"
                                       "case \"$*\" in *\"ls-files -s\") " +
                                       echtes_git + " \"$@\" | head -n 1; exit 1 ;; esac\nexec " + echtes_git +
                                       " \"$@\"\n"));
    Lauf const probe_git = im_repo(fall.repo(), pfad + " git ls-files -s");
    ASSERT_EQ(probe_git.code, 1) << "Arrangement: der git-Koeder scheitert nicht:\n" << probe_git.ausgabe;
    ASSERT_TRUE(enthaelt(probe_git.ausgabe, "VERMERK.md")) << "Arrangement: die Teilausgabe ist nicht der Anker:\n"
                                                           << probe_git.ausgabe;
    Lauf const teilausgabe = fall.fahren("", pfad);
    berichten("WerkzeugAusfallInDerNennerPipelineIstExit2/git-ls-files-s-Teilausgabe", teilausgabe, marke);
    EXPECT_EQ(teilausgabe.code, 2) << "Ein Werkzeug-Ausfall ist 'konnte nicht pruefen' -- Exit 2, nie gruen.\n"
                                   << teilausgabe.ausgabe;
    EXPECT_TRUE(zeile_beginnt(teilausgabe.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'git ls-files -s'"))
        << "Der Abbruch muss das Werkzeug nennen.\n"
        << teilausgabe.ausgabe;
    EXPECT_FALSE(enthaelt(teilausgabe.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << teilausgabe.ausgabe;
    std::error_code ec;
    fs::remove(bin / "git", ec);
    ASSERT_FALSE(fs::exists(bin / "git"));

    // (b) 'wc' scheitert ohne Ausgabe -- der erste Zaehler der Wache.
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "wc",
                                   "#!/bin/sh\n# PATH-Koeder des Falls " + marke +
                                       ": 'wc' scheitert ohne Ausgabe.\n"
                                       "exit 1\n"));
    Lauf const probe_wc = im_repo(fall.repo(), pfad + " wc -l < /dev/null");
    ASSERT_EQ(probe_wc.code, 1) << "Arrangement: der wc-Koeder scheitert nicht:\n" << probe_wc.ausgabe;
    Lauf const zaehler = fall.fahren("", pfad);
    berichten("WerkzeugAusfallInDerNennerPipelineIstExit2/wc-Ausfall", zaehler, marke);
    EXPECT_EQ(zaehler.code, 2) << "Ein Zaehler ohne Zahl ist kein Nenner -- Exit 2, nie gruen.\n" << zaehler.ausgabe;
    EXPECT_TRUE(zeile_beginnt(zaehler.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'wc -l'"))
        << "Der Abbruch muss das Werkzeug nennen.\n"
        << zaehler.ausgabe;
    EXPECT_FALSE(enthaelt(zaehler.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << zaehler.ausgabe;
    fs::remove(bin / "wc", ec);
    ASSERT_FALSE(fs::exists(bin / "wc"));

    // (c) GEGENRICHTUNG: ohne Koeder wieder gruen -- die Abbrueche stammten aus den Koedern.
    Lauf const wieder = fall.fahren("", pfad);
    berichten("WerkzeugAusfallInDerNennerPipelineIstExit2/Koeder-entfernt", wieder, marke);
    EXPECT_EQ(wieder.code, 0) << wieder.ausgabe;
    EXPECT_TRUE(zeile_exakt(wieder.ausgabe, endzeile_ok(1, 1))) << wieder.ausgabe;
}

// =============================================================================
// (19) LEERE ISA-TEILMERKMALE SIND UNPRUEFBAR (Lens C LC3W-07, Fix-r3). Bis 806629ca wurde ein leeres
//      Teilmerkmal still uebersprungen: 'isa:+avx512f' galt wie 'isa:avx512f' (Exit 0). Die Form ist
//      isa:<merkmal>[+<merkmal>] -- kein '+' am Rand, kein '++'. Geprueft wird die FORM vor dem Cache:
//      der Nenner sagt "ISA-Gegenprobe: nicht gefragt". Gegenrichtung (a): dieselbe Zeile wohlgeformt
//      traegt, weil avx512f dem Bau-Host des Falls fehlt (kIsaCacheAvx2DaAvx512fFehlt).
// =============================================================================
TEST(Pa1ToteAusnahme, IsaLeeresTeilmerkmalIstUnpruefbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.isa_cache_schreiben(kIsaCacheAvx2DaAvx512fFehlt));

    // (a) Arrangement und Gegenrichtung: wohlgeformt, avx512f fehlt -> begruendet.
    ASSERT_TRUE(fall.allowlist_setzen("isa:avx512f"));
    Lauf const gesund = fall.fahren();
    berichten("IsaLeeresTeilmerkmalIstUnpruefbar/wohlgeformt", gesund, marke);
    ASSERT_EQ(gesund.code, 0) << "Das Arrangement ist falsch: 'isa:avx512f' traegt nicht.\n" << gesund.ausgabe;
    EXPECT_TRUE(zeile_exakt(gesund.ausgabe, fall.waise() + " -- Koeder " + marke + " (ISA am Bau-Host: avx512f=nein )"))
        << gesund.ausgabe;
    EXPECT_TRUE(zeile_exakt(gesund.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << gesund.ausgabe;
    EXPECT_TRUE(zeile_exakt(gesund.ausgabe, endzeile_ok(2, 0))) << gesund.ausgabe;

    // (b)-(d) ein leeres Teilmerkmal am Anfang, am Ende, in der Mitte.
    for (char const* const merkmal : {"+avx512f", "avx512f+", "avx2++avx512f"}) {
        ASSERT_TRUE(fall.allowlist_setzen(std::string{"isa:"} + merkmal));
        Lauf const leer = fall.fahren();
        berichten((std::string{"IsaLeeresTeilmerkmalIstUnpruefbar/isa:"} + merkmal).c_str(), leer, marke);
        EXPECT_EQ(leer.code, 1) << "'isa:" << merkmal << "' hat ein leeres Teilmerkmal -- ROT.\n" << leer.ausgabe;
        EXPECT_TRUE(zeile_beginnt(leer.ausgabe,
                                  fall.waise() + " -- UNPRUEFBAR: \"isa:" + merkmal + "\" hat ein leeres Teilmerkmal"))
            << leer.ausgabe;
        EXPECT_TRUE(zeile_beginnt(leer.ausgabe, "ISA-Gegenprobe: nicht gefragt"))
            << "Die Form wird VOR dem Cache geprueft -- der Cache darf nicht gelesen worden sein.\n"
            << leer.ausgabe;
        EXPECT_TRUE(zeile_exakt(leer.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << leer.ausgabe;
        EXPECT_TRUE(zeile_exakt(leer.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << leer.ausgabe;
    }
}

// =============================================================================
// (20) ISA-BELEGE MUESSEN EINDEUTIG SEIN (Lens C LC3W-08, Fix-r3). Die Wertzeile musste schon GENAU
//      EINMAL im Cache stehen; _COMPILED und _EXITCODE nicht -- eine zweite Zeile schlief oder gewann
//      je nach Reihenfolge (gegen 806629ca je Exit 0). Und bei leerem Wert ('nein') liess ein LEERER
//      _EXITCODE durch, obwohl try_run beide Zeilen gemeinsam schreibt. Jede Stufe ist Exit 2 (die
//      ISA-Frage ist unbeantwortbar), nie ein Befund; Gegenrichtung: der ehrliche Cache traegt.
// =============================================================================
TEST(Pa1ToteAusnahme, IsaBelegeMuessenEindeutigSein) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.allowlist_setzen("isa:avx512f"));
    std::string const gesund_cache{kIsaCacheAvx2DaAvx512fFehlt};

    ASSERT_TRUE(fall.isa_cache_schreiben(gesund_cache));
    Lauf const gesund = fall.fahren();
    berichten("IsaBelegeMuessenEindeutigSein/ehrlicher-Cache", gesund, marke);
    ASSERT_EQ(gesund.code, 0) << "Das Arrangement ist falsch: der ehrliche Cache traegt nicht.\n" << gesund.ausgabe;

    struct Stufe {
        char const* name;
        std::string cache;
        char const* meldung;
    };
    std::vector<Stufe> const stufen{
        {"_COMPILED-doppelt", gesund_cache + "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=FALSE\n",
         "'COMDARE_HOST_RUNS_AVX512F_COMPILED' steht 2-mal in"},
        {"_EXITCODE-doppelt", gesund_cache + "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=0\n",
         "'COMDARE_HOST_RUNS_AVX512F_EXITCODE' steht 2-mal in"},
        {"nein-mit-leerem-_EXITCODE",
         "COMDARE_HOST_RUNS_AVX512F:INTERNAL=\n"
         "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=TRUE\n"
         "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=\n",
         "'COMDARE_HOST_RUNS_AVX512F' ist leer und 'COMDARE_HOST_RUNS_AVX512F_EXITCODE' ist es auch"},
    };
    for (auto const& s : stufen) {
        ASSERT_TRUE(fall.isa_cache_schreiben(s.cache));
        Lauf const lauf = fall.fahren();
        berichten((std::string{"IsaBelegeMuessenEindeutigSein/"} + s.name).c_str(), lauf, marke);
        EXPECT_EQ(lauf.code, 2) << s.name << ": ein nicht eindeutiger Beleg ist UNBEANTWORTBAR -- Exit 2.\n"
                                << lauf.ausgabe;
        EXPECT_TRUE(zeile_beginnt(lauf.ausgabe, std::string{"ABBRUCH: "} + s.meldung))
            << s.name << ": die Meldung muss den Beleg nennen (ganze Zeile, LC3T-04).\n"
            << lauf.ausgabe;
        EXPECT_FALSE(enthaelt(lauf.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << lauf.ausgabe;
    }

    // Gegenrichtung: der ehrliche Cache traegt wieder.
    ASSERT_TRUE(fall.isa_cache_schreiben(gesund_cache));
    Lauf const wieder = fall.fahren();
    berichten("IsaBelegeMuessenEindeutigSein/ehrlicher-Cache-wieder", wieder, marke);
    EXPECT_EQ(wieder.code, 0) << wieder.ausgabe;
    EXPECT_TRUE(zeile_exakt(wieder.ausgabe, endzeile_ok(2, 0))) << wieder.ausgabe;
}

// =============================================================================
// (21) WERKZEUG-AUSFALL IM ISA-PFAD IST EXIT 2 (Lens C LC3W-01, Fix-r3). Die Fassung 806629ca zaehlte die
//      Cache-Zeilen mit 'sed | wc -l | tr': ein Ausfall vor dem letzten Glied war unsichtbar, und ein
//      ausgefallenes wc hinterliess einen leeren Zaehler, den jedes '-eq 0' still uebersprang -- Exit 0
//      (Shell-Probe im Beweisort FIX-r3.md, stdin-lesender wc-Koeder). Jetzt laeuft der ISA-Teil ueber
//      grep_in_datei und zeilen_zaehlen; zeilen_zaehlen reicht den DATEINAMEN an wc, deshalb kann ein
//      PATH-Koeder gezielt an der ISA-Zwischendatei scheitern. Gegenrichtung: ohne Koeder Exit 0.
// =============================================================================
TEST(Pa1ToteAusnahme, WerkzeugAusfallImIsaPfadIstExit2) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.isa_cache_schreiben(kIsaCacheAvx2DaAvx512fFehlt));
    ASSERT_TRUE(fall.allowlist_setzen("isa:avx512f"));
    Lauf const gesund = fall.fahren();
    berichten("WerkzeugAusfallImIsaPfadIstExit2/ohne-Koeder", gesund, marke);
    ASSERT_EQ(gesund.code, 0) << "Das Arrangement ist falsch: der gesunde Baum ist nicht gruen.\n" << gesund.ausgabe;

    Lauf const wo = im_repo(fall.repo(), "command -v wc");
    ASSERT_EQ(wo.code, 0) << wo.ausgabe;
    std::string const echtes_wc = wo.ausgabe;
    ASSERT_FALSE(echtes_wc.empty());
    fs::path const    bin  = fall.repo().pfad() / "koeder_bin";
    std::string const pfad = "PATH=\"" + bin.string() + ":$PATH\"";
    // Der Koeder scheitert NUR, wenn seine Argumente die ISA-Zwischendatei nennen; alle anderen
    // Zaehlungen der Wache laufen durch das echte wc.
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "wc",
                                   "#!/bin/sh\n# PATH-Koeder des Falls " + marke +
                                       ": 'wc' scheitert an der ISA-Zwischendatei, sonst echtes wc.\n"
                                       "case \"$*\" in *isa_*) exit 1 ;; esac\nexec " +
                                       echtes_wc + " \"$@\"\n"));
    ASSERT_TRUE(fall.repo().schreibe("isa_wert_probe.txt", "x\n"));
    Lauf const probe_isa = im_repo(fall.repo(), pfad + " wc -l isa_wert_probe.txt");
    ASSERT_EQ(probe_isa.code, 1) << "Arrangement: der wc-Koeder scheitert nicht an 'isa_':\n" << probe_isa.ausgabe;
    Lauf const probe_echt = im_repo(fall.repo(), "wc -l isa_wert_probe.txt");
    ASSERT_EQ(probe_echt.code, 0) << "Arrangement: das echte wc zaehlt die Probedatei nicht:\n" << probe_echt.ausgabe;
    Lauf const probe_rest = im_repo(fall.repo(), pfad + " wc -l /dev/null");
    ASSERT_EQ(probe_rest.code, 0) << "Arrangement: der wc-Koeder reicht andere Aufrufe nicht durch:\n"
                                  << probe_rest.ausgabe;

    Lauf const ausfall = fall.fahren("", pfad);
    berichten("WerkzeugAusfallImIsaPfadIstExit2/wc-Koeder-an-isa_wert", ausfall, marke);
    EXPECT_EQ(ausfall.code, 2) << "Ein Werkzeug-Ausfall im ISA-Pfad ist 'konnte nicht pruefen' -- Exit 2.\n"
                               << ausfall.ausgabe;
    EXPECT_TRUE(zeile_beginnt(ausfall.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'wc -l' ueber")) << ausfall.ausgabe;
    EXPECT_TRUE(enthaelt(ausfall.ausgabe, "isa_wert.txt")) << "Der Abbruch muss die ISA-Zwischendatei nennen.\n"
                                                           << ausfall.ausgabe;
    EXPECT_FALSE(enthaelt(ausfall.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << ausfall.ausgabe;

    std::error_code ec;
    fs::remove(bin / "wc", ec);
    ASSERT_FALSE(fs::exists(bin / "wc"));
    Lauf const wieder = fall.fahren("", pfad);
    berichten("WerkzeugAusfallImIsaPfadIstExit2/Koeder-entfernt", wieder, marke);
    EXPECT_EQ(wieder.code, 0) << wieder.ausgabe;
    EXPECT_TRUE(zeile_exakt(wieder.ausgabe, endzeile_ok(2, 0))) << wieder.ausgabe;
}

// =============================================================================
// (22) GREP-STATUS 2 IST EXIT 2, KEIN NICHTTREFFER (Lens C LC3W-02, Fix-r3). 'if ! grep -q' nahm einen
//      Status 2 (Datei unlesbar) als "nicht gefunden": an der Gegenprobe wurde daraus "steht NICHT in
//      compile_commands.json" (Exit 2 mit falscher Diagnose), an der Zuordnung im Nachscan ein
//      Datenbefund. Jetzt laeuft jeder grep durch grep_in_datei: 0/1 sind Antworten, ab 2 ist ein
//      Werkzeug-Ausfall. Arrangement: die IST-Datei des Bau-Baums wird unlesbar gemacht -- per chmod 000;
//      liest sie der Nutzer trotzdem (root, CAP_DAC_OVERRIDE), ersatzweise als Symlink auf /proc/self/mem
//      (eine regulaere Datei, deren Lesen mit EIO scheitert); ist auch das nicht herstellbar, wird der
//      Fall LAUT uebersprungen, nie still gruen. Gegenrichtung: wieder lesbar -> Exit 0.
// =============================================================================
TEST(Pa1ToteAusnahme, GrepStatusZweiIstExit2) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.allowlist_setzen("datei:tests/unit/kommt_vielleicht_" + marke + ".hpp"));
    Lauf const gesund = fall.fahren();
    berichten("GrepStatusZweiIstExit2/lesbar", gesund, marke);
    ASSERT_EQ(gesund.code, 0) << gesund.ausgabe;

    fs::path const  ist = fall.baum() / "compile_commands.json";
    std::error_code ec;
    fs::permissions(ist, fs::perms::none, fs::perm_options::replace, ec);
    ASSERT_FALSE(ec) << "chmod 000 fehlgeschlagen: " << ec.message();
    bool procfs = false;
    Lauf lesbar = fahre("cat " + zitiert(ist) + " > /dev/null");
    if (lesbar.code == 0) {
        // root liest trotz chmod 000: zweite Herstellung ueber procfs.
        fs::permissions(ist, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace, ec);
        fs::remove(ist, ec);
        fs::create_symlink("/proc/self/mem", ist, ec);
        procfs = true;
        lesbar = fahre("cat " + zitiert(ist) + " > /dev/null");
        if (ec || lesbar.code == 0 || !fs::is_regular_file(ist)) {
            GTEST_SKIP() << "Die IST-Datei laesst sich auf diesem Host nicht unlesbar machen (root ohne "
                            "procfs?) -- ohne dieses Arrangement waere die Stufe kein Beweis.";
        }
    }

    Lauf const ausfall = fall.fahren();
    berichten(procfs ? "GrepStatusZweiIstExit2/IST-Datei-EIO" : "GrepStatusZweiIstExit2/IST-Datei-chmod-000", ausfall,
              marke);
    EXPECT_EQ(ausfall.code, 2) << "Eine unlesbare IST-Datei ist ein Werkzeug-Ausfall -- Exit 2.\n" << ausfall.ausgabe;
    EXPECT_TRUE(zeile_beginnt(ausfall.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'grep' Gegenprobe")) << ausfall.ausgabe;
    EXPECT_FALSE(enthaelt(ausfall.ausgabe, "steht NICHT in"))
        << "Die alte Fehldiagnose (Nichttreffer statt Werkzeug-Ausfall) darf nicht mehr erscheinen.\n"
        << ausfall.ausgabe;
    EXPECT_FALSE(enthaelt(ausfall.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << ausfall.ausgabe;

    // Gegenrichtung: wieder lesbar.
    if (procfs) {
        fs::remove(ist, ec);
        ASSERT_TRUE(fall.bauweg_schreiben({kGegenprobe}));
    } else {
        fs::permissions(ist,
                        fs::perms::owner_read | fs::perms::owner_write | fs::perms::group_read | fs::perms::others_read,
                        fs::perm_options::replace, ec);
        ASSERT_FALSE(ec) << ec.message();
    }
    Lauf const wieder = fall.fahren();
    berichten("GrepStatusZweiIstExit2/wieder-lesbar", wieder, marke);
    EXPECT_EQ(wieder.code, 0) << wieder.ausgabe;
    EXPECT_TRUE(zeile_exakt(wieder.ausgabe, endzeile_ok(2, 0))) << wieder.ausgabe;
}

// =============================================================================
// (23) GIT-FEHLER IN DER ERREICHBARKEITS-PROBE SIND EXIT 2 (Lens C LC3W-03, Fix-r3). 'git check-ignore -q'
//      kennt drei Antworten: 0 ignoriert, 1 nicht ignoriert, 128 git scheitert. Die Fassung 806629ca
//      nahm 128 als "nicht ignoriert" und erklaerte ein Bauprodukt unter .gitignore fuer TOT (Exit 1,
//      ein Datenurteil aus einem Werkzeugfehler); ein 'git ls-files' mit 128 galt als "kein Inhalt"
//      (Shell-Proben im Beweisort FIX-r3.md). Aufbau wie Fall (3), dazu PATH-Koeder fuer git, die genau
//      EINEN Unterbefehl mit 128 beenden. Gegenrichtung: ohne Koeder Exit 0.
// =============================================================================
TEST(Pa1ToteAusnahme, GitFehlerInDerErreichbarkeitsProbeIstExit2) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(".gitignore", "erzeugt_" + marke + "/\n"));
    std::string const bauprodukt = "erzeugt_" + marke + "/artefakt.hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + bauprodukt));
    Lauf const gesund = fall.fahren();
    berichten("GitFehlerInDerErreichbarkeitsProbeIstExit2/ohne-Koeder", gesund, marke);
    ASSERT_EQ(gesund.code, 0) << "Das Arrangement ist falsch: das Bauprodukt ist nicht erreichbar.\n" << gesund.ausgabe;

    Lauf const wo = im_repo(fall.repo(), "command -v git");
    ASSERT_EQ(wo.code, 0) << wo.ausgabe;
    std::string const echtes_git = wo.ausgabe;
    ASSERT_FALSE(echtes_git.empty());
    fs::path const    bin  = fall.repo().pfad() / "koeder_bin";
    std::string const pfad = "PATH=\"" + bin.string() + ":$PATH\"";
    std::error_code   ec;

    // (a) 'git check-ignore' endet mit 128.
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "git",
                                   "#!/bin/sh\n# PATH-Koeder des Falls " + marke +
                                       ": 'check-ignore' scheitert mit 128.\n"
                                       "if [ \"$1\" = check-ignore ]; then echo 'fatal: Koeder' >&2; exit 128; fi\n"
                                       "exec " +
                                       echtes_git + " \"$@\"\n"));
    Lauf const probe_ci = im_repo(fall.repo(), pfad + " git check-ignore -q -- x");
    ASSERT_EQ(probe_ci.code, 128) << "Arrangement: der git-Koeder liefert keine 128:\n" << probe_ci.ausgabe;
    Lauf const ci = fall.fahren("", pfad);
    berichten("GitFehlerInDerErreichbarkeitsProbeIstExit2/check-ignore-128", ci, marke);
    EXPECT_EQ(ci.code, 2) << "Ein git-Fehler ist keine Antwort auf 'ignoriert?' -- Exit 2.\n" << ci.ausgabe;
    EXPECT_TRUE(zeile_beginnt(ci.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'git check-ignore' fuer " + bauprodukt))
        << ci.ausgabe;
    EXPECT_FALSE(enthaelt(ci.ausgabe, "TOTE AUSNAHME -- der Gegenstand"))
        << "Aus einem Werkzeugfehler darf kein Datenurteil werden.\n"
        << ci.ausgabe;
    EXPECT_FALSE(enthaelt(ci.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << ci.ausgabe;
    fs::remove(bin / "git", ec);
    ASSERT_FALSE(fs::exists(bin / "git"));

    // (b) 'git ls-files -- <pfad>' (die Index-Frage der Probe) endet mit 128; 'ls-files' ohne '--' (SOLL)
    //     und 'ls-files -s' (Anker) laufen echt.
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "git",
                                   "#!/bin/sh\n# PATH-Koeder des Falls " + marke +
                                       ": 'ls-files --' scheitert mit 128.\n"
                                       "if [ \"$1\" = ls-files ] && [ \"$2\" = -- ]; then echo 'fatal: Koeder' >&2; "
                                       "exit 128; fi\nexec " +
                                       echtes_git + " \"$@\"\n"));
    Lauf const probe_lf = im_repo(fall.repo(), pfad + " git ls-files -- x");
    ASSERT_EQ(probe_lf.code, 128) << probe_lf.ausgabe;
    Lauf const probe_soll = im_repo(fall.repo(), pfad + " git ls-files");
    ASSERT_EQ(probe_soll.code, 0) << "Arrangement: der Koeder trifft auch das SOLL:\n" << probe_soll.ausgabe;
    Lauf const lf = fall.fahren("", pfad);
    berichten("GitFehlerInDerErreichbarkeitsProbeIstExit2/ls-files-128", lf, marke);
    EXPECT_EQ(lf.code, 2) << lf.ausgabe;
    EXPECT_TRUE(zeile_beginnt(lf.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'git ls-files' fuer " + bauprodukt))
        << lf.ausgabe;
    EXPECT_FALSE(enthaelt(lf.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << lf.ausgabe;
    fs::remove(bin / "git", ec);
    ASSERT_FALSE(fs::exists(bin / "git"));

    // (c) Gegenrichtung.
    Lauf const wieder = fall.fahren("", pfad);
    berichten("GitFehlerInDerErreichbarkeitsProbeIstExit2/Koeder-entfernt", wieder, marke);
    EXPECT_EQ(wieder.code, 0) << wieder.ausgabe;
    EXPECT_TRUE(zeile_exakt(wieder.ausgabe, endzeile_ok(2, 0))) << wieder.ausgabe;
}

// =============================================================================
// (24) DER ANKER MUSS EIN BLOB IN DER OBJEKTDATENBANK SEIN (Lens C LC3W-06 / LC3W-03, Fix-r3). Der
//      Index-Modus 100644 verspricht ein Blob, prueft es aber nicht: 'git update-index --cacheinfo' legt
//      jedes Objekt unter jedem Modus ab. Ein TREE unter 100644 ankerte gegen 806629ca (Exit 0, sein
//      'cat-file -p' hat Nicht-Leerraum-Zeichen). Jetzt: 'cat-file -e' (Objekt da? 1 = fehlt -> UNPRUEFBAR,
//      ein Datenbefund) und 'cat-file -t' == blob (sonst UNPRUEFBAR); scheitert git selbst, ist es Exit 2.
//      Stufen: (a) Tree, (b) Fantasie-SHA, (c) echtes Blob (Gegenrichtung), (d) Commit-Objekt unter 100644,
//      (e) Index-Modus 160000 (Fix-r7), (f) git-Koeder 'cat-file -t' 128 [Etiketten berichtigt mit Fix-r8, Lens B
//      r7 LB7-03 = Lens C r6 LC6T-03: bis 89cf7103 hiessen Commit und git-Koeder beide '(d)'].
// =============================================================================
TEST(Pa1ToteAusnahme, ArchivAnkerMussBlobInDerObjektdatenbankSein) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/objekt_" + marke;
    std::string const anker  = ordner + "/VERMERK.md";
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());

    // (a) ein Tree-Objekt unter 100644.
    Lauf const tree = im_repo(fall.repo(), "git write-tree");
    ASSERT_EQ(tree.code, 0) << tree.ausgabe;
    ASSERT_EQ(tree.ausgabe.size(), 40U) << "kein SHA-1: '" << tree.ausgabe << "'";
    Lauf const typ = im_repo(fall.repo(), "git cat-file -t " + tree.ausgabe);
    ASSERT_EQ(typ.ausgabe, "tree") << "Arrangement: das Objekt ist kein Tree:\n" << typ.ausgabe;
    Lauf const cacheinfo =
        im_repo(fall.repo(), "git update-index --add --cacheinfo 100644," + tree.ausgabe + "," + anker);
    ASSERT_EQ(cacheinfo.code, 0) << cacheinfo.ausgabe;
    Lauf const eintrag = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(anker));
    ASSERT_TRUE(zeile_exakt(eintrag.ausgabe, "100644 " + tree.ausgabe + " 0\t" + anker)) << "Arrangement:\n"
                                                                                         << eintrag.ausgabe;
    Lauf const als_tree = fall.fahren();
    berichten("ArchivAnkerMussBlobInDerObjektdatenbankSein/Tree-unter-100644", als_tree, marke);
    EXPECT_EQ(als_tree.code, 1) << "Ein Tree ist kein Anker -- die Datei bleibt im SOLL, ROT.\n" << als_tree.ausgabe;
    EXPECT_TRUE(zeile_beginnt(als_tree.ausgabe, anker +
                                                    " -- UNPRUEFBARER ANKER: Objekttyp tree ist kein Blob (Index-Modus"
                                                    " 100644 verspricht eines)"))
        << als_tree.ausgabe;
    EXPECT_TRUE(zeile_beginnt(als_tree.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << als_tree.ausgabe;
    EXPECT_TRUE(zeile_exakt(als_tree.ausgabe, fall.waise())) << als_tree.ausgabe;
    EXPECT_TRUE(zeile_exakt(als_tree.ausgabe, nenner_ohne_bauweg(1, 0, 0, 0, 0))) << als_tree.ausgabe;
    EXPECT_TRUE(zeile_exakt(als_tree.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0))) << als_tree.ausgabe;

    // (b) ein SHA ohne Objekt.
    std::string const fantasie = "0123456789012345678901234567890123456789";
    Lauf const cacheinfo2 = im_repo(fall.repo(), "git update-index --add --cacheinfo 100644," + fantasie + "," + anker);
    ASSERT_EQ(cacheinfo2.code, 0) << cacheinfo2.ausgabe;
    Lauf const fehlt_probe = im_repo(fall.repo(), "git cat-file -e " + fantasie);
    ASSERT_EQ(fehlt_probe.code, 1) << "Arrangement: das Objekt existiert doch:\n" << fehlt_probe.ausgabe;
    Lauf const ohne_objekt = fall.fahren();
    berichten("ArchivAnkerMussBlobInDerObjektdatenbankSein/Objekt-fehlt", ohne_objekt, marke);
    EXPECT_EQ(ohne_objekt.code, 1) << ohne_objekt.ausgabe;
    EXPECT_TRUE(zeile_beginnt(ohne_objekt.ausgabe,
                              anker + " -- UNPRUEFBARER ANKER: Blob " + fantasie + " fehlt in der Objektdatenbank"))
        << ohne_objekt.ausgabe;
    EXPECT_TRUE(zeile_exakt(ohne_objekt.ausgabe, nenner_ohne_bauweg(1, 0, 0, 0, 0))) << ohne_objekt.ausgabe;
    EXPECT_TRUE(zeile_exakt(ohne_objekt.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0))) << ohne_objekt.ausgabe;

    // (d) EIN COMMIT-OBJEKT UNTER 100644 (Lens C r5 LC5T-06, Fix-r7): 'update-index --cacheinfo' legt jedes Objekt
    //     unter jedem Modus ab; ein Commit hinter dem Anker-Eintrag ist kein Blob und ankert nicht (Probe
    //     t7-machbarkeit). Der Commit entsteht per 'commit-tree' mit -c user.name/user.email (Werkbank ohne Config).
    Lauf const commit = im_repo(fall.repo(), "git -c user.name=pa1 -c user.email=pa1@invalid commit-tree " +
                                                 tree.ausgabe + " -m anker_" + marke);
    ASSERT_EQ(commit.code, 0) << commit.ausgabe;
    ASSERT_EQ(commit.ausgabe.size(), 40U) << "kein SHA-1: '" << commit.ausgabe << "'";
    Lauf const ctyp = im_repo(fall.repo(), "git cat-file -t " + commit.ausgabe);
    ASSERT_EQ(ctyp.ausgabe, "commit") << "Arrangement: das Objekt ist kein Commit:\n" << ctyp.ausgabe;
    Lauf const cacheinfo3 =
        im_repo(fall.repo(), "git update-index --add --cacheinfo 100644," + commit.ausgabe + "," + anker);
    ASSERT_EQ(cacheinfo3.code, 0) << cacheinfo3.ausgabe;
    Lauf const eintrag3 = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(anker));
    ASSERT_TRUE(zeile_exakt(eintrag3.ausgabe, "100644 " + commit.ausgabe + " 0\t" + anker)) << "Arrangement:\n"
                                                                                            << eintrag3.ausgabe;
    Lauf const als_commit = fall.fahren();
    berichten("ArchivAnkerMussBlobInDerObjektdatenbankSein/Commit-unter-100644", als_commit, marke);
    EXPECT_EQ(als_commit.code, 1) << "Ein Commit ist kein Anker -- die Datei bleibt im SOLL, ROT.\n"
                                  << als_commit.ausgabe;
    EXPECT_TRUE(zeile_beginnt(als_commit.ausgabe,
                              anker + " -- UNPRUEFBARER ANKER: Objekttyp commit ist kein Blob (Index-Modus 100644" +
                                  " verspricht eines)"))
        << als_commit.ausgabe;
    EXPECT_TRUE(zeile_exakt(als_commit.ausgabe, fall.waise())) << als_commit.ausgabe;
    EXPECT_TRUE(zeile_exakt(als_commit.ausgabe, nenner_ohne_bauweg(1, 0, 0, 0, 0))) << als_commit.ausgabe;
    EXPECT_TRUE(zeile_exakt(als_commit.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0))) << als_commit.ausgabe;

    // (e) DER ANKER MIT INDEX-MODUS 160000 (Gitlink; LC5T-06): das Objekt ist ein echtes Blob, der Modus ist es
    //     nicht -- 'Index-Modus 160000 ist kein regulaeres Blob', ankert nichts. Die Modus-Pruefung greift VOR der
    //     Objektpruefung.
    Lauf const anker_blob = im_repo(fall.repo(), "printf '%s\\n' '# Anker " + marke + "' | git hash-object -w --stdin");
    ASSERT_EQ(anker_blob.code, 0) << anker_blob.ausgabe;
    ASSERT_EQ(anker_blob.ausgabe.size(), 40U) << "kein SHA-1: '" << anker_blob.ausgabe << "'";
    Lauf const cacheinfo4 =
        im_repo(fall.repo(), "git update-index --add --cacheinfo 160000," + anker_blob.ausgabe + "," + anker);
    ASSERT_EQ(cacheinfo4.code, 0) << cacheinfo4.ausgabe;
    Lauf const eintrag4 = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(anker));
    ASSERT_TRUE(zeile_exakt(eintrag4.ausgabe, "160000 " + anker_blob.ausgabe + " 0\t" + anker)) << "Arrangement:\n"
                                                                                                << eintrag4.ausgabe;
    Lauf const als_gitlink = fall.fahren();
    berichten("ArchivAnkerMussBlobInDerObjektdatenbankSein/Anker-Modus-160000", als_gitlink, marke);
    EXPECT_EQ(als_gitlink.code, 1) << als_gitlink.ausgabe;
    EXPECT_TRUE(zeile_beginnt(als_gitlink.ausgabe,
                              anker + " -- UNPRUEFBARER ANKER: Index-Modus 160000 ist kein regulaeres Blob (Symlink" +
                                  " 120000 oder Gitlink 160000)"))
        << als_gitlink.ausgabe;
    EXPECT_TRUE(zeile_exakt(als_gitlink.ausgabe, nenner_ohne_bauweg(1, 0, 0, 0, 0))) << als_gitlink.ausgabe;
    EXPECT_TRUE(zeile_exakt(als_gitlink.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0))) << als_gitlink.ausgabe;

    // (c) Gegenrichtung: ein echtes Blob traegt.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(anker, "# Anker mit Inhalt " + marke + "\n"));
    Lauf const blob = fall.fahren();
    berichten("ArchivAnkerMussBlobInDerObjektdatenbankSein/echtes-Blob", blob, marke);
    EXPECT_EQ(blob.code, 0) << blob.ausgabe;
    EXPECT_TRUE(zeile_exakt(blob.ausgabe, endzeile_ok(1, 1))) << blob.ausgabe;

    // (f) git selbst scheitert an 'cat-file -t': Exit 2, kein Befund (bis 89cf7103 als '(d)' etikettiert).
    Lauf const wo = im_repo(fall.repo(), "command -v git");
    ASSERT_EQ(wo.code, 0) << wo.ausgabe;
    Lauf const blob_sha = im_repo(fall.repo(), "git hash-object -- " + zitiert(anker));
    ASSERT_EQ(blob_sha.ausgabe.size(), 40U) << blob_sha.ausgabe;
    fs::path const    bin  = fall.repo().pfad() / "koeder_bin";
    std::string const pfad = "PATH=\"" + bin.string() + ":$PATH\"";
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "git",
                                   "#!/bin/sh\n# PATH-Koeder des Falls " + marke +
                                       ": 'cat-file -t' scheitert mit 128.\n"
                                       "if [ \"$1\" = cat-file ] && [ \"$2\" = -t ]; then echo 'fatal: Koeder' >&2; "
                                       "exit 128; fi\nexec " +
                                       wo.ausgabe + " \"$@\"\n"));
    Lauf const probe = im_repo(fall.repo(), pfad + " git cat-file -t " + blob_sha.ausgabe);
    ASSERT_EQ(probe.code, 128) << "Arrangement: der git-Koeder liefert keine 128:\n" << probe.ausgabe;
    Lauf const probe_echt = im_repo(fall.repo(), "git cat-file -t " + blob_sha.ausgabe);
    ASSERT_EQ(probe_echt.ausgabe, "blob") << "Arrangement: das echte git kennt das Blob nicht:\n" << probe_echt.ausgabe;
    Lauf const ausfall = fall.fahren("", pfad);
    berichten("ArchivAnkerMussBlobInDerObjektdatenbankSein/cat-file-t-128", ausfall, marke);
    EXPECT_EQ(ausfall.code, 2) << ausfall.ausgabe;
    EXPECT_TRUE(
        zeile_beginnt(ausfall.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'git cat-file -t' fuer den Anker " + anker))
        << ausfall.ausgabe;
    EXPECT_FALSE(enthaelt(ausfall.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << ausfall.ausgabe;
    std::error_code ec;
    fs::remove(bin / "git", ec);
    ASSERT_FALSE(fs::exists(bin / "git"));
    Lauf const wieder = fall.fahren("", pfad);
    berichten("ArchivAnkerMussBlobInDerObjektdatenbankSein/Koeder-entfernt", wieder, marke);
    EXPECT_EQ(wieder.code, 0) << wieder.ausgabe;
    EXPECT_TRUE(zeile_exakt(wieder.ausgabe, endzeile_ok(1, 1))) << wieder.ausgabe;
}

// =============================================================================
// (25) EIN AUSFALL VON 'date' IST EXIT 2 (Lens C LC3W-04, Fix-r3). HEUTE=$(date ...) ohne Statuspruefung
//      endete unter 'set -e' mit dem Rohstatus von date -- Exit 1, von einem Befund nicht zu unterscheiden
//      (Shell-Probe im Beweisort FIX-r3.md). Jetzt: werkzeug_abbruch, Exit 2. Mit COMDARE_WACHE_HEUTE
//      wird date nicht gefragt -- derselbe Koeder ist dann wirkungslos (Gegenrichtung 1); ohne Koeder
//      Exit 0 (Gegenrichtung 2). Die uebrigen direkten Werkzeuge der Fassung 806629ca (sed fuer die
//      eingerueckte Ausgabe und die Ordnernamen, cut/sed/tr fuer die Felder) sind aus der Wache entfernt
//      -- ohne Werkzeug kein Ausfall (Kopf der Wache, Folge (6)).
// =============================================================================
TEST(Pa1ToteAusnahme, DateAusfallIstExit2) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.allowlist_setzen("datei:tests/unit/kommt_vielleicht_" + marke + ".hpp"));
    fs::path const    bin  = fall.repo().pfad() / "koeder_bin";
    std::string const pfad = "PATH=\"" + bin.string() + ":$PATH\"";
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "date",
                                   "#!/bin/sh\n# PATH-Koeder des Falls " + marke + ": 'date' scheitert.\nexit 1\n"));
    Lauf const probe = im_repo(fall.repo(), pfad + " date +%Y-%m-%d");
    ASSERT_EQ(probe.code, 1) << "Arrangement: der date-Koeder scheitert nicht:\n" << probe.ausgabe;

    Lauf const ausfall = fall.fahren("", pfad);
    berichten("DateAusfallIstExit2/date-Koeder", ausfall, marke);
    EXPECT_EQ(ausfall.code, 2) << "Ohne belastbares Heute kann die Wache nicht pruefen -- Exit 2, nie Rohstatus.\n"
                               << ausfall.ausgabe;
    EXPECT_TRUE(zeile_beginnt(ausfall.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'date +%Y-%m-%d'")) << ausfall.ausgabe;
    EXPECT_FALSE(enthaelt(ausfall.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << ausfall.ausgabe;

    // Gegenrichtung 1: das Heute vorgegeben -> date wird nicht gefragt, der Koeder bleibt wirkungslos.
    Lauf const vorgegeben = fall.fahren("2026-09-18", pfad);
    berichten("DateAusfallIstExit2/Heute-vorgegeben", vorgegeben, marke);
    EXPECT_EQ(vorgegeben.code, 0) << vorgegeben.ausgabe;
    EXPECT_TRUE(zeile_exakt(vorgegeben.ausgabe, "Heute (fuer 'frist:'): 2026-09-18 -- Herkunft: COMDARE_WACHE_HEUTE"
                                                " (ueberschrieben)."))
        << vorgegeben.ausgabe;

    // Gegenrichtung 2: ohne Koeder.
    std::error_code ec;
    fs::remove(bin / "date", ec);
    ASSERT_FALSE(fs::exists(bin / "date"));
    Lauf const wieder = fall.fahren("", pfad);
    berichten("DateAusfallIstExit2/Koeder-entfernt", wieder, marke);
    EXPECT_EQ(wieder.code, 0) << wieder.ausgabe;
    // Fragment, bewusst: das Datum der Systemuhr ist nicht pinnbar; der Zeilenrest ' -- Herkunft: Systemuhr.' schon.
    EXPECT_TRUE(enthaelt(wieder.ausgabe, " -- Herkunft: Systemuhr.")) << wieder.ausgabe;
}

// =============================================================================
// (26) DIE FORM GILT AUCH FUER DIE STUMME ZEILE EINER DATEI IM BAUWEG (Lens C LC3W-05, Fix-r3). Die
//      Schleife der Wache prueft Feld 2 und 3 nur an Zeilen, die sie auswertet (Dateien, die dem Bauweg
//      fehlen). Eine Zeile fuer eine Datei IM Bauweg ist stumm -- das ist Design (Lens A LA3-08: die
//      'isa:'-Zeile ist auf dem AVX-512-Host genau so stumm) -- aber ein Formfehler darin lag in Reserve:
//      gegen 806629ca war jede der Stufen (a)-(d) Exit 0 (Shell-Proben im Beweisort FIX-r3.md; die
//      Lead-Triage hatte den Fund als widerlegt gefuehrt, die Test-Auflage widerlegte die Widerlegung).
//      Jetzt prueft der Nachscan die Form ALLER Datenzeilen mit demselben Helfer wie die Schleife und
//      zaehlt Formfehler stummer Zeilen als eigene Klasse (sechstes Nenner-Feld). Gegenstand: die
//      Gegenprobe-Datei des Falls (sie steht immer im Bauweg); die Waise traegt daneben eine gueltige
//      Zeile und muss begruendet bleiben. Die ISA-Stufe hat KEINEN CMakeCache.txt und ist trotzdem Exit 1,
//      nicht Exit 2: Form vor Cache. (e) Gegenrichtung: eine WOHLGEFORMTE stumme Zeile bleibt stumm --
//      Exit 0, ihr Feld 3 erscheint nie in der Ausgabe.
// =============================================================================
TEST(Pa1ToteAusnahme, AllowlistFormfehlerFuerDateiImBauwegIstUnpruefbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    std::string const lebendig = "tests/unit/kommt_vielleicht_" + marke + ".hpp";
    std::string const tragend  = fall.waise() + " | datei:" + lebendig + " | Koeder " + marke + "\n";
    std::string const gebaut   = kGegenprobe; // steht im Bauweg des Falls

    struct Stufe {
        char const* name;
        std::string zeile;
        std::string meldung;
    };
    std::vector<Stufe> const rot{
        {"Feld-3-leer", gebaut + " | datei:tests/unit/x_" + marke + ".hpp |\n",
         gebaut + " -- UNPRUEFBAR: Feld 3 (Begruendung) ist leer"},
        {"Art-unbekannt", gebaut + " | foo:x | Text " + marke + "\n",
         gebaut + " -- UNPRUEFBAR: Feld 2 ist \"foo:x\" -- keine bekannte Art"},
        {"ISA-Merkmal-unbekannt", gebaut + " | isa:sse9 | Text " + marke + "\n",
         gebaut + " -- UNPRUEFBAR: unbekannte(s) ISA-Merkmal(e): sse9 (bekannt: avx2, avx512f)"},
        {"kein-Datum", gebaut + " | frist:gestern | Text " + marke + "\n",
         gebaut + " -- UNPRUEFBAR: \"frist:gestern\" ist kein Datum JJJJ-MM-TT"},
    };
    for (auto const& s : rot) {
        ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                         "# Allowlist des Falls " + marke + "\n" + tragend + s.zeile));
        Lauf const lauf = fall.fahren();
        berichten((std::string{"AllowlistFormfehlerFuerDateiImBauwegIstUnpruefbar/"} + s.name).c_str(), lauf, marke);
        EXPECT_EQ(lauf.code, 1) << s.name << ": ein Formfehler in einer stummen Zeile ist ein Freibrief in Reserve"
                                << " -- ROT.\n"
                                << lauf.ausgabe;
        EXPECT_TRUE(zeile_beginnt(lauf.ausgabe, s.meldung)) << s.name << "\n" << lauf.ausgabe;
        EXPECT_TRUE(enthaelt(lauf.ausgabe, "die Zeile gilt einer Datei IM Bauweg und ist stumm")) << lauf.ausgabe;
        EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_davon(1, 0, 0, 0, 0)))
            << "Die tragende Zeile der Waise darf nicht mit rot werden.\n"
            << lauf.ausgabe;
        EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0, 1))) << lauf.ausgabe;
        EXPECT_TRUE(zeile_exakt(lauf.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << lauf.ausgabe;
    }

    // (e) Gegenrichtung: wohlgeformt und stumm.
    std::string const stumm = gebaut + " | datei:tests/unit/y_" + marke + ".hpp | stumm " + marke + "\n";
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + tragend + stumm));
    Lauf const gruen = fall.fahren();
    berichten("AllowlistFormfehlerFuerDateiImBauwegIstUnpruefbar/wohlgeformt-stumm", gruen, marke);
    EXPECT_EQ(gruen.code, 0) << "Eine wohlgeformte Zeile fuer eine Datei im Bauweg ist stumm -- kein Befund.\n"
                             << gruen.ausgabe;
    EXPECT_FALSE(enthaelt(gruen.ausgabe, "stumm " + marke))
        << "Die stumme Zeile darf nie ausgewertet worden sein -- ihr Feld 3 gehoert nicht in die Ausgabe.\n"
        << gruen.ausgabe;
    EXPECT_TRUE(zeile_exakt(gruen.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << gruen.ausgabe;
    EXPECT_TRUE(zeile_exakt(gruen.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0, 0))) << gruen.ausgabe;
    EXPECT_TRUE(zeile_exakt(gruen.ausgabe, endzeile_ok(2, 0))) << gruen.ausgabe;
}

// =============================================================================
// (27) EIN VERZEICHNIS IST KEIN GITLINK, AUCH WENN SEIN ERSTER INDEX-EINTRAG EINER IST (Fund N-1 der
//      Fix-r3-Berichtsfassung, Lead-Objektprobe K205; Fix-r4). ist_gitlink() fragte 'git ls-files -s' mit
//      dem Pfad als Pathspec -- fuer ein VERZEICHNIS liefert das ALLE Eintraege darunter -- und prueft nur,
//      ob die Ausgabe mit '160000 ' BEGINNT. Steht als erster Eintrag unter dem Verzeichnis ein Gitlink,
//      galt das Verzeichnis selbst als Gitlink, und jeder Gegenstand in Tiefe >= 2 darunter war 'erreichbar'
//      statt TOT: fail-open in der PA-1-Richtung, die Wache blieb gruen. Am echten Repo: ext/queuing (erster
//      Eintrag ext/queuing/Q01-concurrentqueue, Modus 160000), Gegenstand ext/queuing/nicht_da/x.hpp.
//      Vorbestand seit 806629ca (:686-687); seit Fix-r3 die ERSTE Frage der Erreichbarkeits-Probe.
//      Arrangement wie Fall (4): Gitlink ext/dir/A-sub per 'update-index --cacheinfo 160000' (der SHA muss
//      kein Objekt sein) und getrackte Datei ext/dir/B.txt; 'ls-files -s' ueber das Verzeichnis beginnt mit
//      dem Gitlink (ASSERT). (a) ext/dir/nicht/da.hpp -> TOTE AUSNAHME, Exit 1 (gegen 63f8abd4: Exit 0).
//      (b) Gegenrichtung: ext/dir/A-sub/x.hpp liegt unter dem ECHTEN Gitlink und bleibt erreichbar, Exit 0,
//      wie Fall (4). (c) Das direkte Kind ext/dir/nicht_da.hpp bleibt erreichbar (Fall (2): neue Datei in
//      einem vorhandenen Verzeichnis) -- es war auch vorher unbetroffen, der Pin haelt die Grenze fest.
// =============================================================================
TEST(Pa1ToteAusnahme, VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());

    std::string const dir     = "ext/dir_" + marke;
    std::string const gitlink = dir + "/A-sub";
    std::string const datei   = dir + "/B.txt";
    std::string const sha     = "0000000000000000000000000000000000000002";
    // Erst die Datei, dann der Gitlink: die Reihenfolge im Index haengt am Pfad ('A-sub' < 'B.txt'),
    // nicht an der Reihenfolge des Einfuegens -- genau wie ext/queuing/Q01-... vor ext/queuing/REPOS_....
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(datei, "B " + marke + "\n"));
    Lauf const idx = im_repo(fall.repo(), "git update-index --add --cacheinfo 160000," + sha + "," + gitlink);
    ASSERT_EQ(idx.code, 0) << "Gitlink konnte nicht in den Index gelegt werden:\n" << idx.ausgabe;
    // Arrangement: 'ls-files -s' ueber das VERZEICHNIS liefert zwei Zeilen, die erste ist der Gitlink --
    // das Muster, das die Fassung 63f8abd4 als "Verzeichnis ist Gitlink" las.
    Lauf const eintraege = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + dir));
    ASSERT_EQ(eintraege.code, 0) << eintraege.ausgabe;
    std::size_t const umbruch = eintraege.ausgabe.find('\n');
    ASSERT_NE(umbruch, std::string::npos) << "Arrangement: nur eine Zeile unter dem Verzeichnis:\n"
                                          << eintraege.ausgabe;
    ASSERT_EQ(eintraege.ausgabe.substr(0, umbruch), "160000 " + sha + " 0\t" + gitlink) << eintraege.ausgabe;
    // EXAKTE INDEX-ZEILE der Datei statt zweier Teilstring-ASSERTs (Lens C r4 LC3T-08, Fix-r6).
    Lauf const bsha = im_repo(fall.repo(), "git hash-object -- " + zitiert(datei));
    ASSERT_EQ(bsha.ausgabe.size(), 40U) << "kein SHA-1: '" << bsha.ausgabe << "'";
    ASSERT_TRUE(zeile_exakt(eintraege.ausgabe, "100644 " + bsha.ausgabe + " 0\t" + datei))
        << "Arrangement: die Datei fehlt unter dem Verzeichnis:\n"
        << eintraege.ausgabe;

    // (a) Gegenstand in Tiefe 2 unter dem Verzeichnis: keine Quelle dieses Repos kennt 'nicht/'.
    std::string const tot = dir + "/nicht/da_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + tot));
    Lauf const lauf = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/Tiefe-2", lauf, marke);
    EXPECT_EQ(lauf.code, 1) << "Das Verzeichnis ist kein Gitlink -- der Zweig 'nicht/' hat keinen Erzeuger, ROT.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, "TOTE AUSNAHME -- der Gegenstand kann in KEINEM erklaerten Baum entstehen:"))
        << lauf.ausgabe;
    EXPECT_TRUE(zeile_beginnt(lauf.ausgabe, fall.waise() + " -- TOTE AUSNAHME: \"" + tot + "\" existiert nicht"))
        << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_davon(0, 0, 1, 0, 0))) << lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(lauf.ausgabe, endzeile_rot(0, 2, 0, 1, 0, 0))) << lauf.ausgabe;

    // (b) Gegenrichtung: unter dem ECHTEN Gitlink bleibt jeder Pfad erreichbar (Fall (4)).
    std::string const drin = gitlink + "/include/kopf_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + drin));
    Lauf const gruen = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/unter-Gitlink", gruen, marke);
    EXPECT_EQ(gruen.code, 0) << "Ein Pfad unter einem Gitlink kann jederzeit ausgecheckt werden -- kein Befund.\n"
                             << gruen.ausgabe;
    // Feld 3 der gelesenen Zeile, nicht die blosse Marke (die steht als Pfadname in jeder Ausgabe; LB4-02).
    EXPECT_TRUE(zeile_beginnt(gruen.ausgabe, fall.waise() + " -- Koeder " + marke)) << gruen.ausgabe;
    EXPECT_TRUE(zeile_exakt(gruen.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << gruen.ausgabe;
    EXPECT_TRUE(zeile_exakt(gruen.ausgabe, endzeile_ok(2, 0))) << gruen.ausgabe;

    // (c) Das direkte Kind des Verzeichnisses: erreichbar, weil der Elternteil getrackten Inhalt hat.
    std::string const kind = dir + "/nicht_da_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + kind));
    Lauf const direkt = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/direktes-Kind", direkt, marke);
    EXPECT_EQ(direkt.code, 0) << "Eine neue Datei in einem vorhandenen Verzeichnis ist der Normalfall.\n"
                              << direkt.ausgabe;
    EXPECT_TRUE(zeile_exakt(direkt.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << direkt.ausgabe;
    EXPECT_TRUE(zeile_exakt(direkt.ausgabe, endzeile_ok(2, 0))) << direkt.ausgabe;

    // (d) DER MODUS (Lens B r4 LB4-01, Lens A r5 LA5-06; Fix-r5): ein SYMLINK (Index-Modus 120000, nur im
    //     Index -- kein Link im Arbeitsbaum) an Stelle eines Gitlinks, Gegenstand direkt darunter. Die Wache
    //     c62cfc7e sah 'getrackten Inhalt' und hielt den Pfad fuer erreichbar (Exit 0, fail-open); ein Mutant
    //     ohne den Modus-Vergleich (M1) haelt jeden exakten Eintrag fuer einen Gitlink (Exit 0). Jetzt:
    //     UNPRUEFBAR, Exit 1 -- die Wache loest keinen Link auf und raet nicht, was dahinter liegt.
    std::string const link = dir + "/link";
    Lauf const        blob = im_repo(fall.repo(), "git hash-object -w -- " + zitiert(datei));
    ASSERT_EQ(blob.code, 0) << blob.ausgabe;
    ASSERT_EQ(blob.ausgabe.size(), 40U) << "kein SHA-1: '" << blob.ausgabe << "'";
    Lauf const lidx = im_repo(fall.repo(), "git update-index --add --cacheinfo 120000," + blob.ausgabe + "," + link);
    ASSERT_EQ(lidx.code, 0) << "Symlink-Eintrag konnte nicht in den Index gelegt werden:\n" << lidx.ausgabe;
    Lauf const lmodus = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + link));
    // EXAKTE INDEX-ZEILE statt zweier Teilstring-ASSERTs (Lens C r4 LC3T-08, Fix-r6).
    ASSERT_TRUE(zeile_exakt(lmodus.ausgabe, "120000 " + blob.ausgabe + " 0\t" + link))
        << "Arrangement: kein Symlink-Eintrag als exakte Index-Zeile:\n"
        << lmodus.ausgabe;
    std::string const unter_link = link + "/x_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter_link));
    Lauf const symlink = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/Symlink-statt-Gitlink", symlink, marke);
    EXPECT_EQ(symlink.code, 1) << "Ein Symlink ist kein Gitlink und kein Verzeichnis -- UNPRUEFBAR, ROT.\n"
                               << symlink.ausgabe;
    EXPECT_TRUE(zeile_beginnt(symlink.ausgabe, fall.waise() + " -- UNPRUEFBAR: \"" + unter_link +
                                                   "\" existiert nicht, und sein Vorfahr " + link +
                                                   " ist im Index ein SYMLINK (Modus 120000)"))
        << symlink.ausgabe;
    EXPECT_FALSE(enthaelt(symlink.ausgabe, "TOTE AUSNAHME -- der Gegenstand"))
        << "Ein Symlink-Ahne ist unpruefbar, nicht tot: die Wache weiss nicht, was hinter dem Link liegt.\n"
        << symlink.ausgabe;
    EXPECT_TRUE(zeile_exakt(symlink.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << symlink.ausgabe;
    EXPECT_TRUE(zeile_exakt(symlink.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << symlink.ausgabe;

    // (e) DIE INDEX-STUFE (Lens B r4 LB4-03, Lead-Entscheid O-12 Teil 1; Fix-r5): ein Gitlink, der NUR auf
    //     Stufe 1 steht (Merge-Konflikt, keine Stufe 0; 'update-index --index-info'), ist ein Gitlink -- in
    //     MINDESTENS EINEM Ausgang des Merges bleibt der Pfad darunter erreichbar, und 'erreichbar' ist hier die
    //     Vorsichtsregel (TOT waere die starke Behauptung; Wortlaut berichtigt mit Fix-r6, Lens A r6 LA6-05 = Lens
    //     B r5 LB5-I3: 'in jedem Ausgang' war zu viel). Ein Mutant, der nur Stufe 0 zaehlt (M7),
    //     liesse den Pfad in check-ignore laufen (Exit 2). Jetzt wie bisher: erreichbar, Exit 0 -- gepinnt.
    std::string const stufig = dir + "/S-sub";
    std::string const sha1   = "0000000000000000000000000000000000000003";
    ASSERT_TRUE(fall.repo().schreibe("stufe1_" + marke + ".txt", "160000 " + sha1 + " 1\t" + stufig + "\n"));
    Lauf const sidx = im_repo(fall.repo(), "git update-index --index-info < " +
                                               zitiert(fall.repo().pfad() / ("stufe1_" + marke + ".txt")));
    ASSERT_EQ(sidx.code, 0) << "git update-index --index-info fehlgeschlagen:\n" << sidx.ausgabe;
    Lauf const sstufen = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + stufig));
    ASSERT_TRUE(zeile_exakt(sstufen.ausgabe, "160000 " + sha1 + " 1\t" + stufig)) << "Arrangement: keine Stufe 1:\n"
                                                                                  << sstufen.ausgabe;
    ASSERT_FALSE(enthaelt(sstufen.ausgabe, " 0\t" + stufig)) << "Arrangement: Stufe 0 steht:\n" << sstufen.ausgabe;
    std::string const unter_stufig = stufig + "/x_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter_stufig));
    Lauf const stufe = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/Gitlink-nur-Stufe-1", stufe, marke);
    EXPECT_EQ(stufe.code, 0) << "Ein Gitlink im Merge-Konflikt ist ein Gitlink -- der Pfad darunter ist erreichbar.\n"
                             << stufe.ausgabe;
    EXPECT_TRUE(zeile_beginnt(stufe.ausgabe, fall.waise() + " -- Koeder " + marke)) << stufe.ausgabe;
    EXPECT_TRUE(zeile_exakt(stufe.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << stufe.ausgabe;
    EXPECT_TRUE(zeile_exakt(stufe.ausgabe, endzeile_ok(2, 0))) << stufe.ausgabe;

    // (f) DER TYP-KONFLIKT (Lens A r5 LA5-02, Lead-Entscheid O-12 Teil 2; Fix-r5): derselbe Pfad ist auf Stufe 2
    //     eine DATEI (100644) und auf Stufe 3 ein GITLINK (160000). Loest der Merge zur Datei auf, ist der
    //     Pfad darunter tot; zum Gitlink, erreichbar -- waehrend des Konflikts ist die Antwort unentscheidbar.
    //     Die Wache c62cfc7e nahm die Gitlink-Zeile und meldete erreichbar (Exit 0), reihenfolge-unabhaengig
    //     gruen. Jetzt: UNPRUEFBAR, Exit 1 -- wie der Anker im Konflikt (Fall (12b)).
    std::string const zwiesp = dir + "/K-sub";
    std::string const sha3   = "0000000000000000000000000000000000000004";
    ASSERT_TRUE(fall.repo().schreibe("konflikt_" + marke + ".txt", "100644 " + blob.ausgabe + " 2\t" + zwiesp +
                                                                       "\n160000 " + sha3 + " 3\t" + zwiesp + "\n"));
    Lauf const kidx = im_repo(fall.repo(), "git update-index --index-info < " +
                                               zitiert(fall.repo().pfad() / ("konflikt_" + marke + ".txt")));
    ASSERT_EQ(kidx.code, 0) << "git update-index --index-info fehlgeschlagen:\n" << kidx.ausgabe;
    Lauf const kstufen = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + zwiesp));
    ASSERT_TRUE(zeile_exakt(kstufen.ausgabe, "100644 " + blob.ausgabe + " 2\t" + zwiesp)) << kstufen.ausgabe;
    ASSERT_TRUE(zeile_exakt(kstufen.ausgabe, "160000 " + sha3 + " 3\t" + zwiesp)) << kstufen.ausgabe;
    // AUSSCHLUSS DER STUFE 0 (Lens C r4 LC3T-08, Fix-r6): ohne ihn bewiese das Arrangement keinen Konflikt.
    ASSERT_FALSE(enthaelt(kstufen.ausgabe, " 0\t" + zwiesp)) << "Arrangement: Stufe 0 steht:\n" << kstufen.ausgabe;
    std::string const unter_zwiesp = zwiesp + "/x_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter_zwiesp));
    Lauf const konflikt = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/Typ-Konflikt-Datei-Gitlink", konflikt, marke);
    EXPECT_EQ(konflikt.code, 1) << "Datei gegen Gitlink im Konflikt: die Antwort haengt vom Merge-Ausgang ab -- "
                                   "UNPRUEFBAR, ROT.\n"
                                << konflikt.ausgabe;
    EXPECT_TRUE(zeile_beginnt(konflikt.ausgabe, fall.waise() + " -- UNPRUEFBAR: \"" + unter_zwiesp +
                                                    "\" existiert nicht, und sein Vorfahr " + zwiesp +
                                                    " steht im Index im MERGE-KONFLIKT mit ungleichen Typen je Stufe" +
                                                    " (Stufe 2: Datei, Stufe 3: Gitlink) -- ohne aufgeloeste Fassung"))
        << "Die Meldung muss die TATSAECHLICHEN Typen je Stufe nennen (Lens C r4 LC3W-16, Fix-r6).\n"
        << konflikt.ausgabe;
    EXPECT_TRUE(zeile_exakt(konflikt.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << konflikt.ausgabe;
    EXPECT_TRUE(zeile_exakt(konflikt.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << konflikt.ausgabe;

    // (f2) DIE ANDERE REIHENFOLGE (Lens C r4 LC3T-07, Fix-r6): Gitlink auf Stufe 2, Datei auf Stufe 3. Ein Mutant,
    //      der den ersten Typ gewinnen laesst, sobald er ein Gitlink ist, ueberlebt (f) und stirbt hier (Beweisort
    //      FIX-r6.md, M-gitlink-zuerst). Die Meldung nennt die Typen je Stufe in dieser Reihenfolge (LC3W-16);
    //      bis 9223cbd5 stand fest '(Datei gegen Gitlink)' -- auch fuer Symlink gegen Gitlink (Probe X22).
    std::string const zwiesp2 = dir + "/K2-sub";
    ASSERT_TRUE(fall.repo().schreibe("konflikt2_" + marke + ".txt", "160000 " + sha3 + " 2\t" + zwiesp2 + "\n100644 " +
                                                                        blob.ausgabe + " 3\t" + zwiesp2 + "\n"));
    Lauf const kidx2 = im_repo(fall.repo(), "git update-index --index-info < " +
                                                zitiert(fall.repo().pfad() / ("konflikt2_" + marke + ".txt")));
    ASSERT_EQ(kidx2.code, 0) << "git update-index --index-info fehlgeschlagen:\n" << kidx2.ausgabe;
    Lauf const kstufen2 = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + zwiesp2));
    ASSERT_TRUE(zeile_exakt(kstufen2.ausgabe, "160000 " + sha3 + " 2\t" + zwiesp2)) << kstufen2.ausgabe;
    ASSERT_TRUE(zeile_exakt(kstufen2.ausgabe, "100644 " + blob.ausgabe + " 3\t" + zwiesp2)) << kstufen2.ausgabe;
    ASSERT_FALSE(enthaelt(kstufen2.ausgabe, " 0\t" + zwiesp2)) << kstufen2.ausgabe;
    std::string const unter_zwiesp2 = zwiesp2 + "/x_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter_zwiesp2));
    Lauf const konflikt2 = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/Typ-Konflikt-Gitlink-Datei", konflikt2, marke);
    EXPECT_EQ(konflikt2.code, 1) << "Gitlink gegen Datei im Konflikt, andere Reihenfolge -- UNPRUEFBAR, ROT.\n"
                                 << konflikt2.ausgabe;
    EXPECT_TRUE(zeile_beginnt(konflikt2.ausgabe, fall.waise() + " -- UNPRUEFBAR: \"" + unter_zwiesp2 +
                                                     "\" existiert nicht, und sein Vorfahr " + zwiesp2 +
                                                     " steht im Index im MERGE-KONFLIKT mit ungleichen Typen je Stufe" +
                                                     " (Stufe 2: Gitlink, Stufe 3: Datei) -- ohne aufgeloeste Fassung"))
        << konflikt2.ausgabe;
    EXPECT_TRUE(zeile_exakt(konflikt2.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << konflikt2.ausgabe;
    EXPECT_TRUE(zeile_exakt(konflikt2.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << konflikt2.ausgabe;

    // (e2) GITLINK AUF DEN STUFEN 1, 2 UND 3 und (e3) auf den Stufen 1 und 3 (Lens C r4 LC3T-08, Fix-r6): alle
    //      Zeilen vom selben Typ, kein Stufe-0-Eintrag -- ein Gitlink, erreichbar (O-12 Teil 1). Rot zuerst am
    //      Mutanten, der zwei Gitlink-Stufen als Konflikt liest (M-gl-mehrstufig ueberlebt (e), (f) und (28e)).
    std::string const drei  = dir + "/T-sub";
    std::string const sha_b = "0000000000000000000000000000000000000008";
    std::string const sha_c = "0000000000000000000000000000000000000009";
    ASSERT_TRUE(fall.repo().schreibe("stufen123_" + marke + ".txt", "160000 " + sha1 + " 1\t" + drei + "\n160000 " +
                                                                        sha_b + " 2\t" + drei + "\n160000 " + sha_c +
                                                                        " 3\t" + drei + "\n"));
    Lauf const didx = im_repo(fall.repo(), "git update-index --index-info < " +
                                               zitiert(fall.repo().pfad() / ("stufen123_" + marke + ".txt")));
    ASSERT_EQ(didx.code, 0) << didx.ausgabe;
    Lauf const dstufen = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + drei));
    ASSERT_TRUE(zeile_exakt(dstufen.ausgabe, "160000 " + sha1 + " 1\t" + drei)) << dstufen.ausgabe;
    ASSERT_TRUE(zeile_exakt(dstufen.ausgabe, "160000 " + sha_b + " 2\t" + drei)) << dstufen.ausgabe;
    ASSERT_TRUE(zeile_exakt(dstufen.ausgabe, "160000 " + sha_c + " 3\t" + drei)) << dstufen.ausgabe;
    ASSERT_FALSE(enthaelt(dstufen.ausgabe, " 0\t" + drei)) << dstufen.ausgabe;
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + drei + "/x_" + marke + ".hpp"));
    Lauf const stufen3 = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/Gitlink-Stufen-1-2-3", stufen3, marke);
    EXPECT_EQ(stufen3.code, 0) << "Drei Gitlink-Stufen sind ein Gitlink -- erreichbar.\n" << stufen3.ausgabe;
    EXPECT_TRUE(zeile_beginnt(stufen3.ausgabe, fall.waise() + " -- Koeder " + marke)) << stufen3.ausgabe;
    EXPECT_TRUE(zeile_exakt(stufen3.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << stufen3.ausgabe;
    EXPECT_TRUE(zeile_exakt(stufen3.ausgabe, endzeile_ok(2, 0))) << stufen3.ausgabe;
    std::string const eins_drei = dir + "/U-sub";
    ASSERT_TRUE(fall.repo().schreibe("stufen13_" + marke + ".txt", "160000 " + sha1 + " 1\t" + eins_drei + "\n160000 " +
                                                                       sha_c + " 3\t" + eins_drei + "\n"));
    Lauf const uidx = im_repo(fall.repo(), "git update-index --index-info < " +
                                               zitiert(fall.repo().pfad() / ("stufen13_" + marke + ".txt")));
    ASSERT_EQ(uidx.code, 0) << uidx.ausgabe;
    Lauf const ustufen = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + eins_drei));
    ASSERT_TRUE(zeile_exakt(ustufen.ausgabe, "160000 " + sha1 + " 1\t" + eins_drei)) << ustufen.ausgabe;
    ASSERT_TRUE(zeile_exakt(ustufen.ausgabe, "160000 " + sha_c + " 3\t" + eins_drei)) << ustufen.ausgabe;
    ASSERT_FALSE(enthaelt(ustufen.ausgabe, " 0\t" + eins_drei)) << ustufen.ausgabe;
    ASSERT_FALSE(enthaelt(ustufen.ausgabe, " 2\t" + eins_drei)) << ustufen.ausgabe;
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + eins_drei + "/x_" + marke + ".hpp"));
    Lauf const stufen13 = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/Gitlink-Stufen-1-3", stufen13, marke);
    EXPECT_EQ(stufen13.code, 0) << "Gitlink auf den Stufen 1 und 3 ist ein Gitlink -- erreichbar.\n"
                                << stufen13.ausgabe;
    EXPECT_TRUE(zeile_beginnt(stufen13.ausgabe, fall.waise() + " -- Koeder " + marke)) << stufen13.ausgabe;
    EXPECT_TRUE(zeile_exakt(stufen13.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << stufen13.ausgabe;
    EXPECT_TRUE(zeile_exakt(stufen13.ausgabe, endzeile_ok(2, 0))) << stufen13.ausgabe;

    // (g) EIN EINTRAG GEGEN EIN VERZEICHNIS DARUNTER (D/F-Konflikt; Lens A r6 LA6-01, Fix-r6): Gitlink D-sub auf
    //     Stufe 2, Datei D-sub/x.txt auf Stufe 3 -- git verweigert das nur auf Stufe 0. Loest der Merge zum Gitlink
    //     auf, ist Tiefe 2 darunter erreichbar; zum Verzeichnis, TOT. Die Wache 9223cbd5 sah nur den exakten
    //     Gitlink und meldete erreichbar (Exit 0, fail-open: Probe X01). Jetzt UNPRUEFBAR; die Meldung nennt den
    //     Eintrag UND das Verzeichnis darunter mit ihren Stufen.
    std::string const dfsub = dir + "/D-sub";
    std::string const sha_d = "000000000000000000000000000000000000000a";
    ASSERT_TRUE(fall.repo().schreibe("df_" + marke + ".txt", "160000 " + sha_d + " 2\t" + dfsub + "\n100644 " +
                                                                 blob.ausgabe + " 3\t" + dfsub + "/x.txt\n"));
    Lauf const fidx = im_repo(fall.repo(), "git update-index --index-info < " +
                                               zitiert(fall.repo().pfad() / ("df_" + marke + ".txt")));
    ASSERT_EQ(fidx.code, 0) << "git update-index --index-info (D/F) fehlgeschlagen:\n" << fidx.ausgabe;
    Lauf const fstufen = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + dfsub));
    ASSERT_TRUE(zeile_exakt(fstufen.ausgabe, "160000 " + sha_d + " 2\t" + dfsub)) << fstufen.ausgabe;
    ASSERT_TRUE(zeile_exakt(fstufen.ausgabe, "100644 " + blob.ausgabe + " 3\t" + dfsub + "/x.txt")) << fstufen.ausgabe;
    ASSERT_FALSE(enthaelt(fstufen.ausgabe, " 0\t" + dfsub)) << fstufen.ausgabe;
    std::string const unter_df = dfsub + "/tief/y_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter_df));
    Lauf const df = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/DF-Gitlink-2-gegen-Verzeichnis-3", df, marke);
    EXPECT_EQ(df.code, 1) << "Eintrag gegen Verzeichnis im Konflikt: der Ausgang entscheidet -- UNPRUEFBAR, ROT.\n"
                          << df.ausgabe;
    EXPECT_TRUE(zeile_beginnt(df.ausgabe,
                              fall.waise() + " -- UNPRUEFBAR: \"" + unter_df + "\" existiert nicht, und sein Vorfahr " +
                                  dfsub + " steht im Index im MERGE-KONFLIKT mit ungleichen Typen je Stufe" +
                                  " (Stufe 2: Gitlink; dazu Eintraege DARUNTER auf Stufe 3 = Verzeichnis" + " (D/F))"))
        << df.ausgabe;
    EXPECT_FALSE(enthaelt(df.ausgabe, "TOTE AUSNAHME -- der Gegenstand")) << df.ausgabe;
    EXPECT_TRUE(zeile_exakt(df.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << df.ausgabe;
    EXPECT_TRUE(zeile_exakt(df.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << df.ausgabe;

    // (g2) DIE ANDERE SEITE: Datei E-sub auf Stufe 2, Gitlink E-sub/c darunter auf Stufe 3, Gegenstand UNTER dem
    //      Gitlink. Die Wache 9223cbd5 kehrte am Gitlink um, bevor sie die Datei darueber sah (Exit 0, Probe X03);
    //      jetzt laeuft die Ahnenschleife ueber den Gitlink weiter hinauf und findet den Konflikt. (g3) derselbe
    //      Index, Gegenstand DIREKT unter der Datei: 9223cbd5 sagte TOT ('ist eine getrackte DATEI', Probe X03b),
    //      der Ausgang 'Verzeichnis' machte ihn aber erreichbar -- UNPRUEFBAR, nicht TOT.
    std::string const esub  = dir + "/E-sub";
    std::string const sha_e = "000000000000000000000000000000000000000b";
    ASSERT_TRUE(fall.repo().schreibe("df2_" + marke + ".txt", "100644 " + blob.ausgabe + " 2\t" + esub + "\n160000 " +
                                                                  sha_e + " 3\t" + esub + "/c\n"));
    Lauf const eidx = im_repo(fall.repo(), "git update-index --index-info < " +
                                               zitiert(fall.repo().pfad() / ("df2_" + marke + ".txt")));
    ASSERT_EQ(eidx.code, 0) << eidx.ausgabe;
    Lauf const estufen = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + esub));
    ASSERT_TRUE(zeile_exakt(estufen.ausgabe, "100644 " + blob.ausgabe + " 2\t" + esub)) << estufen.ausgabe;
    ASSERT_TRUE(zeile_exakt(estufen.ausgabe, "160000 " + sha_e + " 3\t" + esub + "/c")) << estufen.ausgabe;
    ASSERT_FALSE(enthaelt(estufen.ausgabe, " 0\t" + esub)) << "Arrangement: Stufe 0 steht:\n" << estufen.ausgabe;
    std::string const unter_e = esub + "/c/x_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter_e));
    Lauf const df2 = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/DF-Datei-2-gegen-Gitlink-darunter-3", df2, marke);
    EXPECT_EQ(df2.code, 1) << "Datei ueber einem Gitlink im Konflikt -- UNPRUEFBAR, ROT (nicht erreichbar).\n"
                           << df2.ausgabe;
    EXPECT_TRUE(zeile_beginnt(df2.ausgabe,
                              fall.waise() + " -- UNPRUEFBAR: \"" + unter_e + "\" existiert nicht, und sein Vorfahr " +
                                  esub + " steht im Index im MERGE-KONFLIKT mit ungleichen Typen je Stufe" +
                                  " (Stufe 2: Datei; dazu Eintraege DARUNTER auf Stufe 3 = Verzeichnis" + " (D/F))"))
        << df2.ausgabe;
    EXPECT_TRUE(zeile_exakt(df2.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << df2.ausgabe;
    EXPECT_TRUE(zeile_exakt(df2.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << df2.ausgabe;
    std::string const kind_e = esub + "/y_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + kind_e));
    Lauf const df3 = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/DF-Datei-2-Kind-direkt-darunter", df3, marke);
    EXPECT_EQ(df3.code, 1) << df3.ausgabe;
    EXPECT_TRUE(zeile_beginnt(df3.ausgabe, fall.waise() + " -- UNPRUEFBAR: \"" + kind_e +
                                               "\" existiert nicht, und sein Vorfahr " + esub +
                                               " steht im Index im MERGE-KONFLIKT mit ungleichen Typen je Stufe"))
        << df3.ausgabe;
    EXPECT_FALSE(enthaelt(df3.ausgabe, "ist eine getrackte DATEI"))
        << "Im D/F-Konflikt ist die Datei-Diagnose die falsche Gewissheit -- UNPRUEFBAR, nicht TOT.\n"
        << df3.ausgabe;
    EXPECT_TRUE(zeile_exakt(df3.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << df3.ausgabe;
    EXPECT_TRUE(zeile_exakt(df3.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << df3.ausgabe;

    // (h) EIN SYMLINK NUR IM ARBEITSBAUM als Ahne (Lens A r6 LA6-03, Fix-r6): 'ln -s' ohne 'git add'. Die Index-
    //     Lesung sah ihn nicht, 'git check-ignore' starb darunter mit 128 ("beyond a symbolic link") -- die Wache
    //     9223cbd5 brach mit Exit 2 und der falschen Diagnose 'Werkzeug-Ausfall' ab (Probe X04). Jetzt UNPRUEFBAR
    //     mit Grund, wie beim Index-Symlink (d), und kein check-ignore wird gefragt.
    std::string const wlink = dir + "/wlink";
    std::error_code   ec;
    fs::create_directories(fall.repo().pfad() / dir / "real", ec);
    ASSERT_FALSE(ec) << ec.message();
    fs::create_symlink("real", fall.repo().pfad() / wlink, ec);
    ASSERT_FALSE(ec) << "Symlink nicht anlegbar: " << ec.message();
    ASSERT_FALSE(fall.repo().ist_verfolgt(wlink)) << "Arrangement: der Link steht im Index, die Stufe maesse (d).";
    std::string const unter_wlink = wlink + "/x_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter_wlink));
    Lauf const arbeitsbaum = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/Symlink-nur-im-Arbeitsbaum", arbeitsbaum, marke);
    EXPECT_EQ(arbeitsbaum.code, 1) << "Ein Symlink im Arbeitsbaum ist kein Verzeichnis -- UNPRUEFBAR, ROT, kein"
                                      " Exit 2.\n"
                                   << arbeitsbaum.ausgabe;
    EXPECT_TRUE(zeile_beginnt(arbeitsbaum.ausgabe, fall.waise() + " -- UNPRUEFBAR: \"" + unter_wlink +
                                                       "\" existiert nicht, und sein Vorfahr " + wlink +
                                                       " ist im Arbeitsbaum ein SYMLINK (nicht im Index)"))
        << arbeitsbaum.ausgabe;
    EXPECT_FALSE(enthaelt(arbeitsbaum.ausgabe, "ABBRUCH: Werkzeug-Ausfall"))
        << "Die falsche Diagnose (check-ignore 128) darf nicht mehr erscheinen.\n"
        << arbeitsbaum.ausgabe;
    EXPECT_TRUE(zeile_exakt(arbeitsbaum.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << arbeitsbaum.ausgabe;
    EXPECT_TRUE(zeile_exakt(arbeitsbaum.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << arbeitsbaum.ausgabe;

    // (g4) D/F MIT DEM EXAKTEN EINTRAG AUF STUFE 0 (Lens A r7 LA7-03, Fix-r7; Probe X36): Datei F-sub auf Stufe 0,
    //      Eintrag F-sub/x.txt darunter auf Stufe 2 -- git verweigert nur BEIDES auf Stufe 0 (Probe X17), Stufe 0
    //      gegen Stufe 1-3 darunter ist per 'update-index --index-info' anlegbar. Arrangement exakt: der Eintrag
    //      NUR auf Stufe 0, der Untereintrag NUR auf Stufe 2. Die Meldung nennt 'Stufe 0: Datei' und 'Stufe 2'.
    std::string const fsub = dir + "/F-sub";
    ASSERT_TRUE(fall.repo().schreibe("df4_" + marke + ".txt", "100644 " + blob.ausgabe + " 0\t" + fsub + "\n100644 " +
                                                                  blob.ausgabe + " 2\t" + fsub + "/x.txt\n"));
    Lauf const fidx4 = im_repo(fall.repo(), "git update-index --index-info < " +
                                                zitiert(fall.repo().pfad() / ("df4_" + marke + ".txt")));
    ASSERT_EQ(fidx4.code, 0) << "git update-index --index-info (D/F Stufe 0) fehlgeschlagen:\n" << fidx4.ausgabe;
    Lauf const fstufen4 = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + fsub));
    ASSERT_TRUE(zeile_exakt(fstufen4.ausgabe, "100644 " + blob.ausgabe + " 0\t" + fsub)) << fstufen4.ausgabe;
    ASSERT_TRUE(zeile_exakt(fstufen4.ausgabe, "100644 " + blob.ausgabe + " 2\t" + fsub + "/x.txt")) << fstufen4.ausgabe;
    ASSERT_FALSE(enthaelt(fstufen4.ausgabe, " 1\t" + fsub)) << "Arrangement: Stufe 1 steht:\n" << fstufen4.ausgabe;
    ASSERT_FALSE(enthaelt(fstufen4.ausgabe, " 2\t" + fsub + "\n")) << "Arrangement: F-sub auf Stufe 2:\n"
                                                                   << fstufen4.ausgabe;
    ASSERT_FALSE(enthaelt(fstufen4.ausgabe, " 3\t" + fsub)) << "Arrangement: Stufe 3 steht:\n" << fstufen4.ausgabe;
    ASSERT_FALSE(enthaelt(fstufen4.ausgabe, " 0\t" + fsub + "/x.txt")) << "Arrangement: x.txt auf Stufe 0:\n"
                                                                       << fstufen4.ausgabe;
    std::string const unter_f4 = fsub + "/tief/y_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter_f4));
    Lauf const df4 = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/DF-Datei-0-gegen-Verzeichnis-2", df4, marke);
    EXPECT_EQ(df4.code, 1) << "Datei auf Stufe 0 gegen Eintraege darunter auf Stufe 2 -- UNPRUEFBAR, ROT.\n"
                           << df4.ausgabe;
    EXPECT_TRUE(zeile_beginnt(df4.ausgabe,
                              fall.waise() + " -- UNPRUEFBAR: \"" + unter_f4 + "\" existiert nicht, und sein Vorfahr " +
                                  fsub + " steht im Index im MERGE-KONFLIKT mit ungleichen Typen je Stufe" +
                                  " (Stufe 0: Datei; dazu Eintraege DARUNTER auf Stufe 2 = Verzeichnis (D/F))"))
        << df4.ausgabe;
    EXPECT_FALSE(enthaelt(df4.ausgabe, "ist eine getrackte DATEI")) << df4.ausgabe;
    EXPECT_TRUE(zeile_exakt(df4.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << df4.ausgabe;
    EXPECT_TRUE(zeile_exakt(df4.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << df4.ausgabe;

    // (g5) UNTEREINTRAEGE AUF ZWEI STUFEN (Lens C r5 LC5W-07, Fix-r7; Probe X39): Gitlink G-sub auf Stufe 2, darunter
    //      x.txt auf Stufe 1 und y.txt auf Stufe 3. Die Meldung nennt die Stufen als Liste 'Stufe 1, Stufe 3' in
    //      Stufenreihenfolge -- bis 8ae59179 stand 'Stufe 1 3', mehrdeutig und an der Pfadsortierung haengend.
    std::string const gsub  = dir + "/G-sub";
    std::string const sha_g = "000000000000000000000000000000000000000c";
    ASSERT_TRUE(fall.repo().schreibe("df5_" + marke + ".txt", "160000 " + sha_g + " 2\t" + gsub + "\n100644 " +
                                                                  blob.ausgabe + " 1\t" + gsub + "/x.txt\n100644 " +
                                                                  blob.ausgabe + " 3\t" + gsub + "/y.txt\n"));
    Lauf const fidx5 = im_repo(fall.repo(), "git update-index --index-info < " +
                                                zitiert(fall.repo().pfad() / ("df5_" + marke + ".txt")));
    ASSERT_EQ(fidx5.code, 0) << fidx5.ausgabe;
    Lauf const fstufen5 = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + gsub));
    ASSERT_TRUE(zeile_exakt(fstufen5.ausgabe, "160000 " + sha_g + " 2\t" + gsub)) << fstufen5.ausgabe;
    ASSERT_TRUE(zeile_exakt(fstufen5.ausgabe, "100644 " + blob.ausgabe + " 1\t" + gsub + "/x.txt")) << fstufen5.ausgabe;
    ASSERT_TRUE(zeile_exakt(fstufen5.ausgabe, "100644 " + blob.ausgabe + " 3\t" + gsub + "/y.txt")) << fstufen5.ausgabe;
    ASSERT_FALSE(enthaelt(fstufen5.ausgabe, " 0\t" + gsub)) << "Arrangement: Stufe 0 steht:\n" << fstufen5.ausgabe;
    std::string const unter_g5 = gsub + "/tief/y_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter_g5));
    Lauf const df5 = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/DF-Untereintraege-Stufe-1-und-3", df5, marke);
    EXPECT_EQ(df5.code, 1) << df5.ausgabe;
    EXPECT_TRUE(zeile_beginnt(
        df5.ausgabe, fall.waise() + " -- UNPRUEFBAR: \"" + unter_g5 + "\" existiert nicht, und sein Vorfahr " + gsub +
                         " steht im Index im MERGE-KONFLIKT mit ungleichen Typen je Stufe" +
                         " (Stufe 2: Gitlink; dazu Eintraege DARUNTER auf Stufe 1, Stufe 3 = Verzeichnis" + " (D/F))"))
        << df5.ausgabe;
    EXPECT_FALSE(enthaelt(df5.ausgabe, "Stufe 1 3")) << "Die alte, mehrdeutige Form darf nicht mehr erscheinen.\n"
                                                     << df5.ausgabe;
    EXPECT_TRUE(zeile_exakt(df5.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << df5.ausgabe;
    EXPECT_TRUE(zeile_exakt(df5.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << df5.ausgabe;

    // (i) DIE AHNENREIHE BIS ZUR WURZEL (Lens C r5 LC5W-01, Fix-r7; Probe X37): Gitlink I-sub auf Stufe 2, DATEI
    //     I-sub/x als regulaerer Eintrag auf Stufe 0 (= D/F an I-sub), Gegenstand I-sub/x/y.hpp. Bis 8ae59179 endete
    //     der Ahnenscan an der naechsten Datei I-sub/x mit TOT -- der D/F-Konflikt am hoeheren Ahnen I-sub wurde nie
    //     erreicht. Rangfolge jetzt: UNPRUEFBAR (naechster Konflikt/Symlink) vor TOT (naechste Datei) vor Gitlink.
    //     ARRANGEMENT SEIT FIX-R8 auf Stufe 0 statt 3 (Lens C r6 LC6W-06, Folge (15i)): eine Datei NUR auf einer
    //     Konfliktstufe ist selbst 'konflikt' (modify/delete) und traefe als naeherer Ahne zuerst -- die Stufe soll
    //     aber den DATEI-Ahnen unter dem D/F-Konflikt pruefen. git nimmt Stufe 0 unter einem Stufe-2-Gitlink an.
    std::string const isub  = dir + "/I-sub";
    std::string const sha_i = "000000000000000000000000000000000000000d";
    ASSERT_TRUE(fall.repo().schreibe("df6_" + marke + ".txt", "160000 " + sha_i + " 2\t" + isub + "\n100644 " +
                                                                  blob.ausgabe + " 0\t" + isub + "/x\n"));
    Lauf const fidx6 = im_repo(fall.repo(), "git update-index --index-info < " +
                                                zitiert(fall.repo().pfad() / ("df6_" + marke + ".txt")));
    ASSERT_EQ(fidx6.code, 0) << fidx6.ausgabe;
    Lauf const fstufen6 = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + isub));
    ASSERT_TRUE(zeile_exakt(fstufen6.ausgabe, "160000 " + sha_i + " 2\t" + isub)) << fstufen6.ausgabe;
    ASSERT_TRUE(zeile_exakt(fstufen6.ausgabe, "100644 " + blob.ausgabe + " 0\t" + isub + "/x")) << fstufen6.ausgabe;
    ASSERT_FALSE(enthaelt(fstufen6.ausgabe, " 0\t" + isub + "\n")) << "Arrangement: I-sub auf Stufe 0:\n"
                                                                   << fstufen6.ausgabe;
    ASSERT_FALSE(enthaelt(fstufen6.ausgabe, " 3\t" + isub)) << "Arrangement: Stufe 3 steht:\n" << fstufen6.ausgabe;
    std::string const unter_i = isub + "/x/y_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter_i));
    Lauf const tief_i = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/Datei-Ahne-unter-DF-Konflikt", tief_i, marke);
    EXPECT_EQ(tief_i.code, 1) << tief_i.ausgabe;
    EXPECT_TRUE(zeile_beginnt(tief_i.ausgabe,
                              fall.waise() + " -- UNPRUEFBAR: \"" + unter_i + "\" existiert nicht, und sein Vorfahr " +
                                  isub + " steht im Index im MERGE-KONFLIKT mit ungleichen Typen je Stufe" +
                                  " (Stufe 2: Gitlink; dazu Eintraege DARUNTER auf Stufe 0 = Verzeichnis (D/F))"))
        << tief_i.ausgabe;
    EXPECT_FALSE(enthaelt(tief_i.ausgabe, "ist eine getrackte DATEI"))
        << "Der Scan darf nicht an der naechsten Datei enden -- der Konflikt darueber gewinnt.\n"
        << tief_i.ausgabe;
    EXPECT_TRUE(zeile_exakt(tief_i.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << tief_i.ausgabe;
    EXPECT_TRUE(zeile_exakt(tief_i.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << tief_i.ausgabe;

    // (j) DATEI-AHNE UNTER EINEM ARBEITSBAUM-SYMLINK (Lens C r5 LC5W-02, Fix-r7; Probe X38): J-sub/x getrackt
    //     (Stufe 0), danach J-sub im Arbeitsbaum durch einen Symlink ersetzt; Gegenstand J-sub/x/y.hpp. Bis
    //     8ae59179 gewann die Datei J-sub/x (TOT), der Link an J-sub wurde nie geprueft -- jetzt UNPRUEFBAR.
    std::string const jsub = dir + "/J-sub";
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(jsub + "/x", "x " + marke + "\n"));
    Lauf const jx_sha = im_repo(fall.repo(), "git hash-object -- " + zitiert(jsub + "/x"));
    ASSERT_EQ(jx_sha.code, 0) << jx_sha.ausgabe;
    ASSERT_EQ(jx_sha.ausgabe.size(), 40U) << "kein SHA-1: '" << jx_sha.ausgabe << "'";
    fs::remove_all(fall.repo().pfad() / jsub, ec);
    ASSERT_FALSE(ec) << ec.message();
    fs::create_directories(fall.repo().pfad() / dir / "jreal", ec);
    ASSERT_FALSE(ec) << ec.message();
    fs::create_symlink("jreal", fall.repo().pfad() / jsub, ec);
    ASSERT_FALSE(ec) << "Symlink nicht anlegbar: " << ec.message();
    ASSERT_TRUE(fs::is_symlink(fall.repo().pfad() / jsub)) << "Arrangement: J-sub ist kein Link.";
    // ist_verfolgt(jsub) traefe auch J-sub/x (git ls-files matcht Verzeichnis-Praefixe) -- der Index wird deshalb
    // pfadgenau gelesen, als EIGENER LAUF mit rc-Pruefung und EXAKTER Ausgabe (Lens B r7 LB7-01 = Lens C r6 LC6T-01,
    // Fix-r8): bis 89cf7103 hing hinter git ein Rohr nach awk -- fahre() nutzt popen (/bin/sh = dash, kein
    // pipefail), j_index.code war der Status von awk, und ein git, das nach der Ausgabe mit 1 stirbt, blieb
    // unentdeckt (Koeder git-j: NEU-Binary PASSED gegen 89cf7103, FIX-r8.md). Erwartet ist GENAU eine Zeile:
    // J-sub/x als Blob auf Stufe 0; ein Eintrag fuer den Link J-sub selbst darf nicht stehen.
    Lauf const j_index = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + jsub));
    ASSERT_EQ(j_index.code, 0) << "git ls-files -s fuer J-sub fehlgeschlagen:\n" << j_index.ausgabe;
    ASSERT_EQ(j_index.ausgabe, "100644 " + jx_sha.ausgabe + " 0\t" + jsub + "/x")
        << "Arrangement: genau J-sub/x auf Stufe 0, kein Eintrag fuer den Link J-sub selbst.\n"
        << j_index.ausgabe;
    std::string const unter_j = jsub + "/x/y_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter_j));
    Lauf const tief_j = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/Datei-Ahne-unter-Arbeitsbaum-Symlink", tief_j,
              marke);
    EXPECT_EQ(tief_j.code, 1) << tief_j.ausgabe;
    EXPECT_TRUE(zeile_beginnt(tief_j.ausgabe, fall.waise() + " -- UNPRUEFBAR: \"" + unter_j +
                                                  "\" existiert nicht, und sein Vorfahr " + jsub +
                                                  " ist im Arbeitsbaum ein SYMLINK (nicht im Index)"))
        << tief_j.ausgabe;
    EXPECT_FALSE(enthaelt(tief_j.ausgabe, "ist eine getrackte DATEI")) << tief_j.ausgabe;
    EXPECT_TRUE(zeile_exakt(tief_j.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << tief_j.ausgabe;
    EXPECT_TRUE(zeile_exakt(tief_j.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << tief_j.ausgabe;

    // (k) DATEI-AHNE, DER IM ARBEITSBAUM EIN SYMLINK IST (Lens C r6 LC6W-09, Fix-r8; Probe R8-12): K-file getrackt
    //     (Stufe 0, regulaeres Blob), danach im Arbeitsbaum durch einen Symlink ersetzt; Gegenstand K-file/y.hpp. Bis
    //     89cf7103 gewann die Index-Datei (TOT) -- die -L-Probe stand nur im Zweig OHNE Index-Eintrag. Die Wache
    //     loest keinen Link auf: UNPRUEFBAR, wie bei (h) und (j).
    std::string const kfile = dir + "/K-file";
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(kfile, "k " + marke + "\n"));
    fs::remove(fall.repo().pfad() / kfile, ec);
    ASSERT_FALSE(ec) << ec.message();
    fs::create_symlink("jreal", fall.repo().pfad() / kfile, ec);
    ASSERT_FALSE(ec) << "Symlink nicht anlegbar: " << ec.message();
    ASSERT_TRUE(fs::is_symlink(fall.repo().pfad() / kfile)) << "Arrangement: K-file ist kein Link.";
    Lauf const k_index = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + kfile));
    ASSERT_EQ(k_index.code, 0) << k_index.ausgabe;
    ASSERT_TRUE(zeile_beginnt(k_index.ausgabe, "100644 ")) << "Arrangement: K-file ist im Index kein Blob:\n"
                                                           << k_index.ausgabe;
    ASSERT_TRUE(enthaelt(k_index.ausgabe, " 0\t" + kfile)) << "Arrangement: K-file nicht auf Stufe 0:\n"
                                                           << k_index.ausgabe;
    std::string const unter_k = kfile + "/y_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter_k));
    Lauf const tief_k = fall.fahren();
    berichten("VerzeichnisMitGitlinkAlsErstemEintragIstKeinGitlink/Datei-Ahne-im-Arbeitsbaum-Symlink", tief_k, marke);
    EXPECT_EQ(tief_k.code, 1) << tief_k.ausgabe;
    EXPECT_TRUE(zeile_beginnt(tief_k.ausgabe, fall.waise() + " -- UNPRUEFBAR: \"" + unter_k +
                                                  "\" existiert nicht, und sein Vorfahr " + kfile +
                                                  " ist im Index eine DATEI, im Arbeitsbaum aber ein SYMLINK"))
        << tief_k.ausgabe;
    EXPECT_FALSE(enthaelt(tief_k.ausgabe, "ist eine getrackte DATEI")) << tief_k.ausgabe;
    EXPECT_TRUE(zeile_exakt(tief_k.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << tief_k.ausgabe;
    EXPECT_TRUE(zeile_exakt(tief_k.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << tief_k.ausgabe;
}

// =============================================================================
// (28) EINE GETRACKTE DATEI ALS VORFAHR IST KEIN VERZEICHNIS (Lens A r5 LA5-01; Fix-r5). Die Aufwaerts-Schleife
//      der Erreichbarkeits-Probe fragte den direkten Elternteil nur 'hat getrackten Inhalt?' -- und 'git ls-files
//      <pfad>' listet fuer eine getrackte DATEI die Datei selbst. 'ext/README.md/x.hpp' galt so als 'neue Datei
//      in einem vorhandenen Verzeichnis' = erreichbar (Exit 0), obwohl unter einer Datei nie ein Kind entstehen
//      kann, weder im Index noch im Arbeitsbaum: fail-open in der PA-1-Richtung, Vorbestand seit 806629ca; am
//      echten Objekt Koeder K4 'ext/queuing/REPOS_OVERVIEW.md/x.hpp' (Lens A r5). Jetzt liest die Probe je Ahnen
//      den exakten Index-Eintrag (Modus, Stufe, Pfadfeld): eine Datei -> TOT, mit dem Vorfahren in der Meldung.
//      (a) direkt unter der Datei -> TOT, Exit 1 (gegen c62cfc7e: Exit 0). (b) Tiefe 2 unter der Datei -> TOT
//      mit derselben Diagnose (gegen c62cfc7e Exit 1, aber ohne den Vorfahren -- die 'weiter oben'-Regel griff).
//      (c) Gegenrichtung: die getrackte Datei SELBST als Gegenstand, im Arbeitsbaum entfernt -> erreichbar
//      (ein Checkout bringt sie zurueck). (d) Gegenrichtung: das direkte Kind des VERZEICHNISSES -> erreichbar.
//      (e) die Datei im Merge-Konflikt (Stufen 1-3, alle 100644, keine Stufe 0): in jedem Ausgang eine Datei
//      oder geloescht -> TOT (Lead-Entscheid O-12: alle Stufen vom selben Typ zaehlen als dieser Typ).
// =============================================================================
TEST(Pa1ToteAusnahme, DateiAlsVorfahrIstKeinVerzeichnis) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());

    std::string const dir   = "ext/dir_" + marke;
    std::string const datei = dir + "/README.md";
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(datei, "# README " + marke + "\n"));
    Lauf const rsha = im_repo(fall.repo(), "git hash-object -- " + zitiert(datei));
    ASSERT_EQ(rsha.ausgabe.size(), 40U) << "kein SHA-1: '" << rsha.ausgabe << "'";
    Lauf const eintrag = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + datei));
    ASSERT_EQ(eintrag.code, 0) << eintrag.ausgabe;
    // EXAKTE INDEX-ZEILE statt zweier Teilstring-ASSERTs (Lens C r4 LC3T-08, Fix-r6).
    ASSERT_TRUE(zeile_exakt(eintrag.ausgabe, "100644 " + rsha.ausgabe + " 0\t" + datei))
        << "Arrangement: die Datei ist kein Blob 100644 auf Stufe 0:\n"
        << eintrag.ausgabe;

    // (a) direkt unter der Datei.
    std::string const unter = datei + "/x_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter));
    Lauf const direkt = fall.fahren();
    berichten("DateiAlsVorfahrIstKeinVerzeichnis/direkt-unter-Datei", direkt, marke);
    EXPECT_EQ(direkt.code, 1) << "Unter einer Datei kann nie ein Kind entstehen -- TOTE AUSNAHME, ROT.\n"
                              << direkt.ausgabe;
    EXPECT_TRUE(
        zeile_exakt(direkt.ausgabe, "TOTE AUSNAHME -- der Gegenstand kann in KEINEM erklaerten Baum entstehen:"))
        << direkt.ausgabe;
    EXPECT_TRUE(zeile_beginnt(direkt.ausgabe, fall.waise() + " -- TOTE AUSNAHME: \"" + unter +
                                                  "\" existiert nicht, und sein Vorfahr " + datei +
                                                  " ist eine getrackte DATEI (Index-Modus 100644/100755)"))
        << "Die Meldung muss den Vorfahren und seine Klasse nennen.\n"
        << direkt.ausgabe;
    EXPECT_TRUE(zeile_exakt(direkt.ausgabe, nenner_davon(0, 0, 1, 0, 0))) << direkt.ausgabe;
    EXPECT_TRUE(zeile_exakt(direkt.ausgabe, endzeile_rot(0, 2, 0, 1, 0, 0))) << direkt.ausgabe;

    // (b) Tiefe 2 unter der Datei: dieselbe Diagnose (nicht die allgemeine 'keine Quelle kennt den Zweig').
    std::string const tief = datei + "/tief/x_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + tief));
    Lauf const tiefer = fall.fahren();
    berichten("DateiAlsVorfahrIstKeinVerzeichnis/Tiefe-2-unter-Datei", tiefer, marke);
    EXPECT_EQ(tiefer.code, 1) << tiefer.ausgabe;
    EXPECT_TRUE(zeile_beginnt(tiefer.ausgabe, fall.waise() + " -- TOTE AUSNAHME: \"" + tief +
                                                  "\" existiert nicht, und sein Vorfahr " + datei +
                                                  " ist eine getrackte DATEI (Index-Modus 100644/100755)"))
        << tiefer.ausgabe;
    EXPECT_TRUE(zeile_exakt(tiefer.ausgabe, nenner_davon(0, 0, 1, 0, 0))) << tiefer.ausgabe;
    EXPECT_TRUE(zeile_exakt(tiefer.ausgabe, endzeile_rot(0, 2, 0, 1, 0, 0))) << tiefer.ausgabe;

    // (c) Gegenrichtung: die getrackte Datei selbst, im Arbeitsbaum entfernt -- sie kann wiederkommen.
    std::error_code ec;
    fs::remove(fall.repo().pfad() / datei, ec);
    ASSERT_FALSE(fs::exists(fall.repo().pfad() / datei)) << "Arrangement: die Datei liegt noch im Arbeitsbaum.";
    ASSERT_TRUE(fall.repo().ist_verfolgt(datei));
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + datei));
    Lauf const selbst = fall.fahren();
    berichten("DateiAlsVorfahrIstKeinVerzeichnis/Datei-selbst-als-Gegenstand", selbst, marke);
    EXPECT_EQ(selbst.code, 0) << "Eine getrackte, im Arbeitsbaum fehlende Datei ist erreichbar (Checkout).\n"
                              << selbst.ausgabe;
    EXPECT_TRUE(zeile_beginnt(selbst.ausgabe, fall.waise() + " -- Koeder " + marke)) << selbst.ausgabe;
    EXPECT_TRUE(zeile_exakt(selbst.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << selbst.ausgabe;
    EXPECT_TRUE(zeile_exakt(selbst.ausgabe, endzeile_ok(2, 0))) << selbst.ausgabe;

    // (d) Gegenrichtung: das direkte Kind des VERZEICHNISSES (es hat getrackten Inhalt: die Datei).
    std::string const kind = dir + "/neu_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + kind));
    Lauf const verzeichnis = fall.fahren();
    berichten("DateiAlsVorfahrIstKeinVerzeichnis/direktes-Kind-des-Verzeichnisses", verzeichnis, marke);
    EXPECT_EQ(verzeichnis.code, 0) << "Eine neue Datei in einem vorhandenen Verzeichnis ist der Normalfall.\n"
                                   << verzeichnis.ausgabe;
    EXPECT_TRUE(zeile_beginnt(verzeichnis.ausgabe, fall.waise() + " -- Koeder " + marke)) << verzeichnis.ausgabe;
    EXPECT_TRUE(zeile_exakt(verzeichnis.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << verzeichnis.ausgabe;
    EXPECT_TRUE(zeile_exakt(verzeichnis.ausgabe, endzeile_ok(2, 0))) << verzeichnis.ausgabe;

    // (e) die Datei im Merge-Konflikt: Stufen 1-3, alle 100644, keine Stufe 0 -- in jedem Ausgang eine Datei.
    Lauf const sha = im_repo(fall.repo(), "git hash-object -w --stdin < /dev/null");
    ASSERT_EQ(sha.code, 0) << sha.ausgabe;
    ASSERT_EQ(sha.ausgabe.size(), 40U) << "kein SHA-1: '" << sha.ausgabe << "'";
    std::string const rezept = "0 0000000000000000000000000000000000000000\t" + datei + "\n" + "100644 " + sha.ausgabe +
                               " 1\t" + datei + "\n" + "100644 " + sha.ausgabe + " 2\t" + datei + "\n" + "100644 " +
                               sha.ausgabe + " 3\t" + datei + "\n";
    ASSERT_TRUE(fall.repo().schreibe("konflikt_" + marke + ".txt", rezept));
    Lauf const konflikt = im_repo(fall.repo(), "git update-index --index-info < " +
                                                   zitiert(fall.repo().pfad() / ("konflikt_" + marke + ".txt")));
    ASSERT_EQ(konflikt.code, 0) << "git update-index --index-info fehlgeschlagen:\n" << konflikt.ausgabe;
    // EXAKTE INDEX-ZEILEN je Stufe, auch Stufe 2 (Lens C r4 LC3T-08, Fix-r6).
    Lauf const stufen = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + datei));
    ASSERT_TRUE(zeile_exakt(stufen.ausgabe, "100644 " + sha.ausgabe + " 1\t" + datei))
        << "Arrangement: keine Stufe 1:\n"
        << stufen.ausgabe;
    ASSERT_TRUE(zeile_exakt(stufen.ausgabe, "100644 " + sha.ausgabe + " 2\t" + datei))
        << "Arrangement: keine Stufe 2:\n"
        << stufen.ausgabe;
    ASSERT_TRUE(zeile_exakt(stufen.ausgabe, "100644 " + sha.ausgabe + " 3\t" + datei))
        << "Arrangement: keine Stufe 3:\n"
        << stufen.ausgabe;
    ASSERT_FALSE(enthaelt(stufen.ausgabe, " 0\t" + datei)) << "Arrangement: Stufe 0 steht noch:\n" << stufen.ausgabe;
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter));
    Lauf const im_konflikt = fall.fahren();
    berichten("DateiAlsVorfahrIstKeinVerzeichnis/Datei-im-Konflikt-Stufen-1-2-3", im_konflikt, marke);
    EXPECT_EQ(im_konflikt.code, 1) << "Drei Datei-Stufen sind eine Datei -- unter ihr entsteht nichts, TOT.\n"
                                   << im_konflikt.ausgabe;
    EXPECT_TRUE(zeile_beginnt(im_konflikt.ausgabe, fall.waise() + " -- TOTE AUSNAHME: \"" + unter +
                                                       "\" existiert nicht, und sein Vorfahr " + datei +
                                                       " ist eine getrackte DATEI (Index-Modus 100644/100755)"))
        << im_konflikt.ausgabe;
    EXPECT_TRUE(zeile_exakt(im_konflikt.ausgabe, nenner_davon(0, 0, 1, 0, 0))) << im_konflikt.ausgabe;
    EXPECT_TRUE(zeile_exakt(im_konflikt.ausgabe, endzeile_rot(0, 2, 0, 1, 0, 0))) << im_konflikt.ausgabe;

    // (f) MODUS 100755 (Lens C r4 LC3T-06, Fix-r6): dieselbe Datei ausfuehrbar -- ein regulaeres Blob wie 100644,
    //     unter dem nie ein Kind entsteht: TOT mit derselben Diagnose. Rot zuerst am Mutanten, der nur 100644 als
    //     Datei liest (M-nur644 ueberlebte (a)-(e), die alle 100644 fahren; Beweisort FIX-r6.md). Der Konflikt aus
    //     (e) wird dafuer per 'git add' aufgeloest (Stufe 0, derselbe Inhalt), dann '--chmod=+x'.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(datei, "# README " + marke + "\n"));
    Lauf const chmodx = im_repo(fall.repo(), "git update-index --chmod=+x -- " + zitiert(datei));
    ASSERT_EQ(chmodx.code, 0) << "git update-index --chmod=+x fehlgeschlagen:\n" << chmodx.ausgabe;
    Lauf const eintrag_x = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + datei));
    ASSERT_TRUE(zeile_exakt(eintrag_x.ausgabe, "100755 " + rsha.ausgabe + " 0\t" + datei)) << "Arrangement:\n"
                                                                                           << eintrag_x.ausgabe;
    ASSERT_FALSE(enthaelt(eintrag_x.ausgabe, " 1\t" + datei)) << "Arrangement: Stufe 1 steht noch:\n"
                                                              << eintrag_x.ausgabe;
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter));
    Lauf const ausfuehrbar = fall.fahren();
    berichten("DateiAlsVorfahrIstKeinVerzeichnis/Datei-Modus-100755", ausfuehrbar, marke);
    EXPECT_EQ(ausfuehrbar.code, 1) << "Eine ausfuehrbare Datei ist eine Datei -- unter ihr entsteht nichts, TOT.\n"
                                   << ausfuehrbar.ausgabe;
    EXPECT_TRUE(zeile_beginnt(ausfuehrbar.ausgabe, fall.waise() + " -- TOTE AUSNAHME: \"" + unter +
                                                       "\" existiert nicht, und sein Vorfahr " + datei +
                                                       " ist eine getrackte DATEI (Index-Modus 100644/100755)"))
        << ausfuehrbar.ausgabe;
    EXPECT_TRUE(zeile_exakt(ausfuehrbar.ausgabe, nenner_davon(0, 0, 1, 0, 0))) << ausfuehrbar.ausgabe;
    EXPECT_TRUE(zeile_exakt(ausfuehrbar.ausgabe, endzeile_rot(0, 2, 0, 1, 0, 0))) << ausfuehrbar.ausgabe;
}

// =============================================================================
// (29) DIE SCHREIBWEISE DES GEGENSTANDS MUSS DER DES INDEX ENTSPRECHEN (Lens A r5 LA5-03; Fix-r5). Der Pfad-
//      vergleich der Erreichbarkeits-Probe ist exakt; 'git ls-files' quotiert aber Pfade mit Nicht-ASCII-Bytes
//      (core.quotePath), Tabulator, Steuerzeichen, Anfuehrungszeichen und Backslash, und ein Gegenstand mit
//      './', '//', '/' am Ende oder Segment '.' steht so in keinem Index. Gegen c62cfc7e traf keiner dieser
//      Pfade seinen Gitlink-Ahnen und lief in 'git check-ignore' -- Exit 2 mit der falschen Diagnose 'is in
//      submodule' ('/' am Ende: zufaellig Exit 0). Jetzt: (a) nicht kanonische Form -> UNPRUEFBAR, Exit 1, mit
//      der Form-Diagnose; (b) quotierbares Zeichen -> UNPRUEFBAR, Exit 1; (c) Gegenrichtung: ein Gitlink mit
//      Nicht-ASCII-Pfad (UTF-8 'ae-<U+00E4>', in der Ausgabe von 'git ls-files' quotiert "\303\244") bleibt
//      erreichbar, weil die Wache den Index mit core.quotePath=false liest -- Exit 0 (gegen c62cfc7e: Exit 2);
//      (d) Gegenrichtung: ein Leerzeichen im Pfad ist erlaubt -> Exit 0; (e) die stumme Zeile einer Datei IM
//      Bauweg mit nicht kanonischem Pfad ist ein Formfehler (Nachscan, Kopf Folge (8)) -> Exit 1, sechstes
//      Nenner-Feld 1 (gegen c62cfc7e: Exit 0). Die Quelle bleibt ASCII: die UTF-8-Bytes stehen als \x-Folgen.
// =============================================================================
TEST(Pa1ToteAusnahme, DateiPfadMussKanonischUndVergleichbarSein) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());

    std::string const dir     = "ext/dir_" + marke;
    std::string const gitlink = dir + "/A-sub";
    std::string const datei   = dir + "/B.txt";
    std::string const sha     = "0000000000000000000000000000000000000005";
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(datei, "B " + marke + "\n"));
    Lauf const idx = im_repo(fall.repo(), "git update-index --add --cacheinfo 160000," + sha + "," + gitlink);
    ASSERT_EQ(idx.code, 0) << "Gitlink konnte nicht in den Index gelegt werden:\n" << idx.ausgabe;

    // (a) nicht kanonische Formen desselben, an sich erreichbaren Pfads unter dem Gitlink.
    std::string const form_diagnose = "ist kein kanonischer repo-relativer Pfad (Segment '.' oder leeres Segment";
    std::vector<std::string> const unkanonisch = {"./" + gitlink + "/x_" + marke + ".hpp",
                                                  dir + "//A-sub/x_" + marke + ".hpp", gitlink + "/",
                                                  dir + "/./A-sub/x_" + marke + ".hpp"};
    for (std::string const& pfad : unkanonisch) {
        ASSERT_TRUE(fall.allowlist_setzen("datei:" + pfad));
        Lauf const lauf = fall.fahren();
        berichten(("DateiPfadMussKanonischUndVergleichbarSein/nicht-kanonisch '" + pfad + "'").c_str(), lauf, marke);
        EXPECT_EQ(lauf.code, 1) << "Ein Pfad, der so in keinem Index steht, ist UNPRUEFBAR -- ROT, nicht Exit 2.\n"
                                << lauf.ausgabe;
        EXPECT_TRUE(
            zeile_beginnt(lauf.ausgabe, fall.waise() + " -- UNPRUEFBAR: \"datei:" + pfad + "\" " + form_diagnose))
            << lauf.ausgabe;
        EXPECT_FALSE(enthaelt(lauf.ausgabe, "ABBRUCH: Werkzeug-Ausfall"))
            << "Die falsche Diagnose (check-ignore 128) darf nicht mehr erscheinen.\n"
            << lauf.ausgabe;
        EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << lauf.ausgabe;
        EXPECT_TRUE(zeile_exakt(lauf.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << lauf.ausgabe;
    }

    // (b) quotierbare Zeichen: Tabulator, Backslash, Anfuehrungszeichen.
    std::string const quot_diagnose =
        "enthaelt ein Zeichen, das git in seiner Ausgabe quotiert (Tabulator, Steuerzeichen,";
    std::vector<std::string> const quotierbar = {dir + "/a\tb/x_" + marke + ".hpp", dir + "/a\\b/x_" + marke + ".hpp",
                                                 dir + "/a\"b/x_" + marke + ".hpp"};
    for (std::string const& pfad : quotierbar) {
        ASSERT_TRUE(fall.allowlist_setzen("datei:" + pfad));
        Lauf const lauf = fall.fahren();
        berichten("DateiPfadMussKanonischUndVergleichbarSein/quotierbares-Zeichen", lauf, marke);
        EXPECT_EQ(lauf.code, 1) << "Ein Pfad, den git quotiert, ist nicht vergleichbar -- UNPRUEFBAR, ROT.\n"
                                << lauf.ausgabe;
        EXPECT_TRUE(
            zeile_beginnt(lauf.ausgabe, fall.waise() + " -- UNPRUEFBAR: \"datei:" + pfad + "\" " + quot_diagnose))
            << lauf.ausgabe;
        EXPECT_TRUE(zeile_exakt(lauf.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << lauf.ausgabe;
        EXPECT_TRUE(zeile_exakt(lauf.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << lauf.ausgabe;
    }

    // (c) Gegenrichtung: ein Gitlink mit Nicht-ASCII-Pfad. 'git ls-files -s' quotiert ihn (Arrangement-ASSERT),
    //     die Wache liest mit core.quotePath=false und trifft ihn trotzdem.
    std::string const umlaut = dir + "/ae-" + std::string{"\xc3\xa4"};
    std::string const sha_u  = "0000000000000000000000000000000000000006";
    Lauf const        uidx = im_repo(fall.repo(), "git update-index --add --cacheinfo 160000," + sha_u + "," + umlaut);
    ASSERT_EQ(uidx.code, 0) << "Gitlink mit Nicht-ASCII-Pfad konnte nicht in den Index gelegt werden:\n"
                            << uidx.ausgabe;
    Lauf const quotiert = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + umlaut));
    ASSERT_EQ(quotiert.code, 0) << quotiert.ausgabe;
    ASSERT_TRUE(zeile_exakt(quotiert.ausgabe, "160000 " + sha_u + " 0\t\"" + dir + "/ae-\\303\\244\""))
        << "Arrangement: git quotiert den Pfad nicht (core.quotePath?):\n"
        << quotiert.ausgabe;
    std::string const unter_umlaut = umlaut + "/x_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter_umlaut));
    Lauf const roh = fall.fahren();
    berichten("DateiPfadMussKanonischUndVergleichbarSein/Nicht-ASCII-Gitlink", roh, marke);
    EXPECT_EQ(roh.code, 0) << "Ein Gitlink mit Nicht-ASCII-Pfad ist ein Gitlink -- der Pfad darunter ist erreichbar.\n"
                           << roh.ausgabe;
    EXPECT_TRUE(zeile_beginnt(roh.ausgabe, fall.waise() + " -- Koeder " + marke)) << roh.ausgabe;
    EXPECT_TRUE(zeile_exakt(roh.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << roh.ausgabe;
    EXPECT_TRUE(zeile_exakt(roh.ausgabe, endzeile_ok(2, 0))) << roh.ausgabe;

    // (d) Gegenrichtung: ein Leerzeichen im Pfad quotiert git nicht -- erlaubt, erreichbar.
    std::string const leer  = dir + "/my sub";
    std::string const sha_l = "0000000000000000000000000000000000000007";
    Lauf const lidx = im_repo(fall.repo(), "git update-index --add --cacheinfo 160000," + sha_l + "," + zitiert(leer));
    ASSERT_EQ(lidx.code, 0) << lidx.ausgabe;
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + leer + "/x_" + marke + ".hpp"));
    Lauf const mit_leer = fall.fahren();
    berichten("DateiPfadMussKanonischUndVergleichbarSein/Leerzeichen-im-Pfad", mit_leer, marke);
    EXPECT_EQ(mit_leer.code, 0) << mit_leer.ausgabe;
    EXPECT_TRUE(zeile_beginnt(mit_leer.ausgabe, fall.waise() + " -- Koeder " + marke)) << mit_leer.ausgabe;
    EXPECT_TRUE(zeile_exakt(mit_leer.ausgabe, endzeile_ok(2, 0))) << mit_leer.ausgabe;

    // (e) die stumme Zeile einer Datei im Bauweg mit nicht kanonischem Pfad: Formfehler, sechstes Nenner-Feld.
    std::string const tragend = fall.waise() + " | datei:" + gitlink + "/x_" + marke + ".hpp | Koeder " + marke + "\n";
    std::string const stumm =
        std::string{kGegenprobe} + " | datei:./tests/unit/neu_" + marke + ".hpp | stumm " + marke + "\n";
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + tragend + stumm));
    Lauf const nachscan = fall.fahren();
    berichten("DateiPfadMussKanonischUndVergleichbarSein/stumme-Zeile-nicht-kanonisch", nachscan, marke);
    EXPECT_EQ(nachscan.code, 1) << "Die Form gilt auch fuer die stumme Zeile (Folge (8)).\n" << nachscan.ausgabe;
    EXPECT_TRUE(zeile_beginnt(nachscan.ausgabe, std::string{kGegenprobe} + " -- UNPRUEFBAR: \"datei:./tests/unit/neu_" +
                                                    marke + ".hpp\" " + form_diagnose))
        << nachscan.ausgabe;
    EXPECT_TRUE(zeile_beginnt(nachscan.ausgabe, fall.waise() + " -- Koeder " + marke))
        << "Die tragende Zeile muss weiter tragen.\n"
        << nachscan.ausgabe;
    EXPECT_TRUE(zeile_exakt(nachscan.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << nachscan.ausgabe;
    EXPECT_TRUE(zeile_exakt(nachscan.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0, 1))) << nachscan.ausgabe;
    EXPECT_TRUE(zeile_exakt(nachscan.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << nachscan.ausgabe;
}

// =============================================================================
// (31) NICHT-ASCII-PFADE IM SOLL UND AM ANKER (Lens A r6 LA6-02 = Lens C r4 LC3W-15; Fix-r6). 'git ls-files'
//      quotiert einen Pfad mit Nicht-ASCII-Bytes ("tests/unit/test_\303\244.cpp"): die Zeile endet auf '"', das
//      Muster '\.cpp$' der SOLL-Lesung traf nicht, und die Datei fiel STILL aus dem SOLL -- die Wache 9223cbd5
//      meldete "2 getrackte" statt 3 und OK (Exit 0), obwohl die Datei dem Bauweg fehlte (Probe X05); ein Anker-
//      Ordner mit Nicht-ASCII-Namen ankerte nicht ("0 archiviert", Probe X07). Seit Fix-r6 lesen SOLL und Anker
//      den Index mit core.quotePath=false. Die Quelle bleibt ASCII: die UTF-8-Bytes stehen als \x-Folgen.
//        (a) test_<marke>_<U+00E4>.cpp getrackt, nicht im Bauweg -> OHNE BEGRUENDUNG, Exit 1, "3 getrackte"
//        (b) dieselbe Datei im Bauweg -> Exit 0, "3 getrackte" (Gegenrichtung: gezaehlt, nichts ist still)
//        (c) Anker-Ordner tests/deprecated/archiv_<U+00E4>_<marke>/ mit VERMERK.md und Test-Datei -> archiviert
// =============================================================================
TEST(Pa1ToteAusnahme, NichtAsciiPfadZaehltImSollUndAnkert) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    // Die Waise des Falls traegt eine gueltige Zeile; allein die Umlaut-Datei entscheidet den Befund.
    std::string const lebendig = "tests/unit/kommt_vielleicht_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + lebendig));

    std::string const umlaut = "tests/unit/test_" + marke + "_" + std::string{"\xc3\xa4"} + ".cpp";
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(umlaut, "// Umlaut-Datei " + marke + "\n"));
    Lauf const quotiert = im_repo(fall.repo(), "git ls-files -- " + zitiert(":(literal)" + umlaut));
    ASSERT_EQ(quotiert.code, 0) << quotiert.ausgabe;
    ASSERT_TRUE(zeile_exakt(quotiert.ausgabe, "\"tests/unit/test_" + marke + "_\\303\\244.cpp\""))
        << "Arrangement: git quotiert den Pfad nicht (core.quotePath?):\n"
        << quotiert.ausgabe;

    // (a) getrackt, nicht im Bauweg: OHNE BEGRUENDUNG -- sichtbar, mit dem rohen Pfad in der Liste.
    Lauf const fehlt = fall.fahren();
    berichten("NichtAsciiPfadZaehltImSollUndAnkert/nicht-im-Bauweg", fehlt, marke);
    EXPECT_EQ(fehlt.code, 1) << "Eine Nicht-ASCII-Testdatei ausserhalb des Bauwegs ist OHNE BEGRUENDUNG -- ROT.\n"
                             << fehlt.ausgabe;
    EXPECT_TRUE(zeile_exakt(fehlt.ausgabe, umlaut)) << "Die Datei muss namentlich (roh) in der Liste stehen.\n"
                                                    << fehlt.ausgabe;
    EXPECT_TRUE(zeile_beginnt(fehlt.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << fehlt.ausgabe;
    EXPECT_TRUE(zeile_exakt(fehlt.ausgabe, nenner_getrackt(3))) << "Der SOLL muss die Datei zaehlen.\n"
                                                                << fehlt.ausgabe;
    EXPECT_TRUE(zeile_exakt(fehlt.ausgabe, nenner_davon(1, 0, 0, 0, 1))) << fehlt.ausgabe;
    EXPECT_TRUE(zeile_exakt(fehlt.ausgabe, endzeile_rot(1, 3, 0, 0, 0, 0))) << fehlt.ausgabe;

    // (b) Gegenrichtung: im Bauweg -> gruen, und der Nenner zaehlt sie weiterhin.
    ASSERT_TRUE(fall.bauweg_schreiben({kGegenprobe, umlaut}));
    Lauf const gebaut = fall.fahren();
    berichten("NichtAsciiPfadZaehltImSollUndAnkert/im-Bauweg", gebaut, marke);
    EXPECT_EQ(gebaut.code, 0) << gebaut.ausgabe;
    EXPECT_TRUE(zeile_exakt(gebaut.ausgabe, nenner_getrackt(3))) << gebaut.ausgabe;
    EXPECT_TRUE(zeile_exakt(gebaut.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << gebaut.ausgabe;
    EXPECT_TRUE(zeile_exakt(gebaut.ausgabe, endzeile_ok(3, 0))) << gebaut.ausgabe;

    // (c) ein Anker-Ordner mit Nicht-ASCII-Namen ankert, die Datei darin ist ARCHIV und steht roh in der Liste.
    std::string const ordner     = "tests/deprecated/archiv_" + std::string{"\xc3\xa4"} + "_" + marke;
    std::string const archiviert = ordner + "/test_x_" + marke + ".cpp";
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/VERMERK.md", "# Anker " + marke + "\n"));
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(archiviert, "// archivierte Datei " + marke + "\n"));
    Lauf const anker = fall.fahren();
    berichten("NichtAsciiPfadZaehltImSollUndAnkert/Nicht-ASCII-Anker-Ordner", anker, marke);
    EXPECT_EQ(anker.code, 0) << "Der Anker mit Nicht-ASCII-Ordnernamen muss tragen.\n" << anker.ausgabe;
    EXPECT_TRUE(zeile_exakt(anker.ausgabe, archiviert))
        << "Die archivierte Datei muss (roh) in der ARCHIV-Liste stehen.\n"
        << anker.ausgabe;
    EXPECT_TRUE(zeile_exakt(anker.ausgabe, nenner_getrackt(4))) << anker.ausgabe;
    EXPECT_TRUE(zeile_exakt(anker.ausgabe, nenner_archiv(1, 1, 3))) << anker.ausgabe;
    // Nullseite der Quote-Nenner-Zeile im Wegwerf-Repo (LB7-02, Fix-r8): vor (d) gibt es keinen quotierten Pfad.
    EXPECT_TRUE(zeile_exakt(anker.ausgabe, nenner_quotiert(0, 0, 0))) << anker.ausgabe;
    EXPECT_TRUE(zeile_exakt(anker.ausgabe, endzeile_ok(3, 1))) << anker.ausgabe;

    // (d) EINE VON GIT AUCH MIT core.quotePath=false QUOTIERTE TEST-QUELLDATEI (Lens A r7 LA7-01 = Lens C r5
    //     LC5W-06, Fix-r7; Probe X23): ein Anfuehrungszeichen im Namen quotiert git IMMER (ebenso Tabulator,
    //     Steuerzeichen, Backslash) -- die Index-Zeile beginnt mit '"' und endet auf '"', das SOLL-Muster traf
    //     nicht, die Datei fiel bis 8ae59179 STILL aus dem SOLL ("2 getrackte", Exit 0). Jetzt: nicht im SOLL
    //     (weiter '4 getrackte'), aber im Nenner gezaehlt ('Quotierte Index-Pfade: 1 ... davon 1 im SOLL-Muster')
    //     und als eigene UNPRUEFBAR-Klasse mit Zeile gelistet -- Exit 1; die Datei ist nicht im Bauweg.
    std::string const quotiert_datei = "tests/unit/test_" + marke + "_a\"b.cpp";
    ASSERT_TRUE(
        schreibe_und_verfolge_einfach_zitiert(fall.repo(), quotiert_datei, "// quotierte Datei " + marke + "\n"));
    Lauf const q_index =
        im_repo(fall.repo(), "git -c core.quotePath=false ls-files -- ':(literal)" + quotiert_datei + "'");
    ASSERT_EQ(q_index.code, 0) << q_index.ausgabe;
    ASSERT_TRUE(zeile_exakt(q_index.ausgabe, "\"tests/unit/test_" + marke + "_a\\\"b.cpp\""))
        << "Arrangement: git quotiert den Pfad auch mit core.quotePath=false nicht:\n"
        << q_index.ausgabe;
    Lauf const q_soll = fall.fahren();
    berichten("NichtAsciiPfadZaehltImSollUndAnkert/quotierte-Testdatei", q_soll, marke);
    EXPECT_EQ(q_soll.code, 1) << "Eine von git quotierte Test-Quelldatei ist UNPRUEFBAR -- ROT, nie still.\n"
                              << q_soll.ausgabe;
    EXPECT_TRUE(zeile_exakt(q_soll.ausgabe, "\"tests/unit/test_" + marke + "_a\\\"b.cpp\" -- UNPRUEFBAR:" +
                                                " Test-Quelldatei, deren Pfad git auch mit core.quotePath=false" +
                                                " quotiert (Tabulator, Steuerzeichen, Anfuehrungszeichen oder" +
                                                " Backslash) -- die Wache kann sie weder im SOLL zaehlen noch im" +
                                                " Bauweg suchen; die Datei umbenennen"))
        << q_soll.ausgabe;
    EXPECT_TRUE(zeile_exakt(q_soll.ausgabe, nenner_getrackt(4))) << "Die quotierte Datei steht NICHT im SOLL.\n"
                                                                 << q_soll.ausgabe;
    EXPECT_TRUE(zeile_exakt(q_soll.ausgabe, nenner_quotiert(1, 1, 0))) << q_soll.ausgabe;
    EXPECT_TRUE(zeile_exakt(q_soll.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0, 0, 1))) << q_soll.ausgabe;
    EXPECT_TRUE(zeile_exakt(q_soll.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << q_soll.ausgabe;
    EXPECT_TRUE(zeile_exakt(q_soll.ausgabe, endzeile_rot(0, 3, 0, 0, 1, 1))) << q_soll.ausgabe;

    // (e) EIN QUOTIERTER ANKER-ORDNER (Probe X27): tests/deprecated/a"b_<marke>/VERMERK.md und eine Test-Datei
    //     darin -- beide Zeilen quotiert; der Anker ankert nichts (UNPRUEFBARER ANKER), die Datei ist UNPRUEFBAR,
    //     nichts davon zaehlt still als archiviert. Der Nenner: 3 quotierte Pfade (mit (d)), 2 im SOLL-Muster,
    //     1 als Anker; Exit 1.
    std::string const q_ordner = "tests/deprecated/a\"b_" + marke;
    std::string const q_anker  = q_ordner + "/VERMERK.md";
    std::string const q_test   = q_ordner + "/test_x_" + marke + ".cpp";
    ASSERT_TRUE(schreibe_und_verfolge_einfach_zitiert(fall.repo(), q_anker, "# quotierter Anker " + marke + "\n"));
    ASSERT_TRUE(
        schreibe_und_verfolge_einfach_zitiert(fall.repo(), q_test, "// Datei im quotierten Ordner " + marke + "\n"));
    std::string const q_ordner_roh = "\"tests/deprecated/a\\\"b_" + marke;
    Lauf const        q_anker_lauf = fall.fahren();
    berichten("NichtAsciiPfadZaehltImSollUndAnkert/quotierter-Anker-Ordner", q_anker_lauf, marke);
    EXPECT_EQ(q_anker_lauf.code, 1) << q_anker_lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(q_anker_lauf.ausgabe, q_ordner_roh + "/VERMERK.md\" -- UNPRUEFBARER ANKER: Pfad von git" +
                                                      " auch mit core.quotePath=false quotiert (Tabulator," +
                                                      " Steuerzeichen, Anfuehrungszeichen oder Backslash) -- ankert" +
                                                      " nichts, die Dateien seines Ordners bleiben fuer diese Wache" +
                                                      " unsichtbar; den Ordner umbenennen"))
        << q_anker_lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(
        q_anker_lauf.ausgabe,
        q_ordner_roh + "/test_x_" + marke +
            ".cpp\" -- UNPRUEFBAR:" + " Test-Quelldatei, deren Pfad git auch mit core.quotePath=false" +
            " quotiert (Tabulator, Steuerzeichen, Anfuehrungszeichen oder" +
            " Backslash) -- die Wache kann sie weder im SOLL zaehlen noch im" + " Bauweg suchen; die Datei umbenennen"))
        << q_anker_lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(q_anker_lauf.ausgabe, nenner_getrackt(4))) << q_anker_lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(q_anker_lauf.ausgabe, nenner_archiv(1, 1, 3)))
        << "Der quotierte Ordner darf nicht als zweiter Archiv-Ordner zaehlen.\n"
        << q_anker_lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(q_anker_lauf.ausgabe, nenner_quotiert(3, 2, 1))) << q_anker_lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(q_anker_lauf.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0, 0, 3))) << q_anker_lauf.ausgabe;
    EXPECT_TRUE(zeile_exakt(q_anker_lauf.ausgabe, endzeile_rot(0, 3, 0, 0, 3, 1))) << q_anker_lauf.ausgabe;
}

namespace {

// Reste unter einem fall-eigenen TMPDIR (Faelle (32) und (33)): Eintraege direkt darunter; raeumen() setzt vorher
// Schreibrechte (Stufe (32c) hinterlaesst absichtlich ein 555-Verzeichnis), damit der WegwerfBaum sauber faellt.
[[nodiscard]] std::size_t tmp_reste(fs::path const& tmp) {
    std::size_t     n = 0;
    std::error_code ec;
    for (auto const& eintrag : fs::directory_iterator(tmp, ec)) {
        (void)eintrag;
        ++n;
    }
    return n;
}

void tmp_raeumen(fs::path const& tmp) {
    std::error_code ec;
    for (auto const& eintrag : fs::recursive_directory_iterator(tmp, ec)) {
        fs::permissions(eintrag.path(), fs::perms::owner_all, fs::perm_options::add, ec);
    }
    for (auto const& eintrag : fs::directory_iterator(tmp, ec)) { fs::remove_all(eintrag.path(), ec); }
}

} // namespace

// =============================================================================
// (32) SIGNALE UND ZWISCHENDATEI-FEHLER SIND EXIT 2 (Lens B r7 LB7-06; Fix-r8). Die sechs Fix-r7-Mutanten
//      M-W2a/b/c (traps nach mktemp, HUP ungefangen, Status im EXIT-trap verloren), M-W3a/b (anhaengen bzw.
//      datei_leeren roh) und M-W4a (lese_abgleich aus) ueberlebten den Google-Test 33/33 -- 'Signale und
//      Schreibfehler sind im Test nicht herstellbar' hiess es. Widerlegt (LENS-B-r7.md Abschn. 6): mktemp-, wc-
//      und Signal-Koeder brauchen nur koeder_bin_anlegen() und den Vorspann 'zusatz' von fahren(). Je Stufe:
//      Exit 2, ABBRUCH-Zeile bzw. keine OK-Zeile, Reste unter einem fall-eigenen TMPDIR. Rot zuerst je Stufe am
//      jeweiligen Beweisort-Mutanten (COMDARE_PA1_WACHE_PFAD) -- FIX-r8.md.
//      Stufen: (a) TERM waehrend mktemp (M-W2a), (b) HUP waehrend mktemp (M-W2b), (c) rm-Ausfall im EXIT-trap
//      nach einem Abbruch -- der Status 2 bleibt (M-W2c), (d) anhaengen auf /dev/full (M-W3a), (e) datei_leeren
//      auf ein Verzeichnis (M-W3b), (f) wc zaehlt soll.txt um 1 zu hoch = Teilbestand (M-W4a).
// =============================================================================
TEST(Pa1ToteAusnahme, SignaleUndZwischendateiFehlerSindExit2) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.allowlist_setzen("datei:tests/unit/kommt_vielleicht_" + marke + ".hpp"));
    Lauf const gesund = fall.fahren();
    berichten("SignaleUndZwischendateiFehlerSindExit2/ohne-Koeder", gesund, marke);
    ASSERT_EQ(gesund.code, 0) << "Das Arrangement ist falsch: der gesunde Baum ist nicht gruen.\n" << gesund.ausgabe;

    fs::path const  tmp = fall.baum() / "tmp";
    std::error_code ec;
    fs::create_directory(tmp, ec);
    ASSERT_FALSE(ec) << ec.message();
    Lauf const wo_mktemp = im_repo(fall.repo(), "command -v mktemp");
    ASSERT_EQ(wo_mktemp.code, 0) << wo_mktemp.ausgabe;
    Lauf const wo_timeout = im_repo(fall.repo(), "command -v timeout");
    ASSERT_EQ(wo_timeout.code, 0) << "coreutils 'timeout' fehlt (Arrangement der Signal-Stufen):\n"
                                  << wo_timeout.ausgabe;
    Lauf const wo_wc = im_repo(fall.repo(), "command -v wc");
    ASSERT_EQ(wo_wc.code, 0) << wo_wc.ausgabe;
    fs::path const    bin        = fall.repo().pfad() / "koeder_bin";
    std::string const pfad       = "PATH=\"" + bin.string() + ":$PATH\" TMPDIR=" + zitiert(tmp);
    auto const        koeder_weg = [&bin, &ec](char const* name) {
        fs::remove(bin / name, ec);
        return !fs::exists(bin / name);
    };

    // (a)/(b) SIGNAL WAEHREND MKTEMP: der mktemp-Koeder schlaeft 1 s; 'timeout --foreground' sendet das Signal
    //     NUR an die Wachen-Shell (das Koeder-Kind laeuft weiter und legt sein Verzeichnis bei t = 1 s an). Mit den
    //     traps VOR mktemp wartet die Shell den Aufruf ab, laeuft den trap 'exit 2' und raeumt (Reste 0); der Mutant
    //     stirbt mit 143/129 und laesst das Verzeichnis liegen -- deshalb die Zaehlung erst nach 1.6 s.
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "mktemp",
                                   "#!/bin/sh\n# PATH-Koeder des Falls " + marke +
                                       ": mktemp schlaeft 1 s.\n"
                                       "sleep 1\nexec " +
                                       wo_mktemp.ausgabe + " \"$@\"\n"));
    for (char const* const sig : {"TERM", "HUP"}) {
        Lauf const signal = fall.fahren("", pfad + " timeout --foreground --preserve-status -s " + sig + " 0.5");
        berichten((std::string{"SignaleUndZwischendateiFehlerSindExit2/"} + sig + "-waehrend-mktemp").c_str(), signal,
                  marke);
        EXPECT_EQ(signal.code, 2) << sig << " waehrend mktemp muss Exit 2 sein (trap vor mktemp).\n" << signal.ausgabe;
        EXPECT_FALSE(enthaelt(signal.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << signal.ausgabe;
        std::this_thread::sleep_for(std::chrono::milliseconds(1600));
        EXPECT_EQ(tmp_reste(tmp), 0U) << "Das Zwischenverzeichnis blieb nach " << sig << " liegen.";
        tmp_raeumen(tmp);
    }
    ASSERT_TRUE(koeder_weg("mktemp"));

    // (c) rm SCHEITERT IM EXIT-TRAP NACH EINEM ABBRUCH: der Koeder legt archiv_anker.txt als Verzeichnis an (das
    //     erste datei_leeren-Ziel -> werkzeug_abbruch, Exit 2) UND halt/ 555 mit Datei (rm -rf scheitert). Der
    //     Status 2 muss bleiben; der Mutant ohne geretteten Status endet mit 1. Reste 3 sind hier gewollt.
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "mktemp",
                                   "#!/bin/sh\nd=$(" + wo_mktemp.ausgabe +
                                       " \"$@\") || exit 1\nmkdir \"$d/archiv_anker.txt\" \"$d/halt\"\n"
                                       ": > \"$d/halt/x\"\nchmod 555 \"$d/halt\"\nprintf '%s\\n' \"$d\"\n"));
    Lauf const halt = fall.fahren("", pfad);
    berichten("SignaleUndZwischendateiFehlerSindExit2/rm-scheitert-nach-Abbruch", halt, marke);
    EXPECT_EQ(halt.code, 2) << "Der Status des Abbruchs muss den rm-Ausfall im EXIT-trap ueberleben.\n" << halt.ausgabe;
    EXPECT_TRUE(zeile_beginnt(halt.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- Zwischendatei ")) << halt.ausgabe;
    EXPECT_TRUE(enthaelt(halt.ausgabe, "/archiv_anker.txt nicht anlegbar")) << halt.ausgabe;
    EXPECT_FALSE(enthaelt(halt.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << halt.ausgabe;
    tmp_raeumen(tmp);
    ASSERT_TRUE(koeder_weg("mktemp"));

    // (d) anhaengen AUF /dev/full: begruendet.txt ist ein Symlink auf /dev/full -- datei_leeren (Truncate ohne
    //     Schreiben) gelingt, das erste anhaengen scheitert mit ENOSPC. Der Mutant endet mit dem Rohstatus 1 ohne
    //     ABBRUCH-Zeile.
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "mktemp",
                                   "#!/bin/sh\nd=$(" + wo_mktemp.ausgabe +
                                       " \"$@\") || exit 1\nln -s /dev/full \"$d/begruendet.txt\"\n"
                                       "printf '%s\\n' \"$d\"\n"));
    Lauf const voll = fall.fahren("", pfad);
    berichten("SignaleUndZwischendateiFehlerSindExit2/anhaengen-auf-dev-full", voll, marke);
    EXPECT_EQ(voll.code, 2) << voll.ausgabe;
    EXPECT_TRUE(zeile_beginnt(voll.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- Schreiben nach ")) << voll.ausgabe;
    EXPECT_TRUE(enthaelt(voll.ausgabe, "/begruendet.txt (Exit ")) << voll.ausgabe;
    EXPECT_FALSE(enthaelt(voll.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << voll.ausgabe;
    tmp_raeumen(tmp);
    ASSERT_TRUE(koeder_weg("mktemp"));

    // (e) datei_leeren AUF EIN VERZEICHNIS: archiv_anker.txt liegt als Verzeichnis vor. In dash beendet ein
    //     rohes ': >' die Shell mit 2 OHNE Zeile -- rc allein unterscheidet dort nicht, die ABBRUCH-Zeile immer.
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "mktemp",
                                   "#!/bin/sh\nd=$(" + wo_mktemp.ausgabe +
                                       " \"$@\") || exit 1\nmkdir \"$d/archiv_anker.txt\"\nprintf '%s\\n' \"$d\"\n"));
    Lauf const verz = fall.fahren("", pfad);
    berichten("SignaleUndZwischendateiFehlerSindExit2/datei-leeren-auf-Verzeichnis", verz, marke);
    EXPECT_EQ(verz.code, 2) << verz.ausgabe;
    EXPECT_TRUE(zeile_beginnt(verz.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- Zwischendatei ")) << verz.ausgabe;
    EXPECT_TRUE(enthaelt(verz.ausgabe, "/archiv_anker.txt nicht anlegbar")) << verz.ausgabe;
    EXPECT_FALSE(enthaelt(verz.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << verz.ausgabe;
    tmp_raeumen(tmp);
    ASSERT_TRUE(koeder_weg("mktemp"));

    // (f) wc ZAEHLT soll.txt UM 1 ZU HOCH (wc erfolgreich, die read-Schleife liest weniger = Teilbestand-Klasse,
    //     Lens A r8 Kuerzungs-Probe): der Abgleich meldet '2 von 3 Zeile(n)'; der Mutant ohne Abgleich rechnet mit
    //     dem falschen SOLL weiter und meldet OK.
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "wc",
                                   "#!/bin/sh\ncase \"$*\" in\n    */soll.txt) out=$(" + wo_wc.ausgabe +
                                       " \"$@\") || exit $?; n=${out%% *}; rest=${out#* }; "
                                       "printf '%s %s\\n' \"$((n+1))\" \"$rest\"; exit 0 ;;\nesac\nexec " +
                                       wo_wc.ausgabe + " \"$@\"\n"));
    Lauf const plus1 = fall.fahren("", pfad);
    berichten("SignaleUndZwischendateiFehlerSindExit2/wc-plus-1-an-soll", plus1, marke);
    EXPECT_EQ(plus1.code, 2) << "Ein Teilbestand ist ein Werkzeug-Ausfall -- Exit 2, nie OK.\n" << plus1.ausgabe;
    EXPECT_TRUE(zeile_beginnt(plus1.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'read' ueber ")) << plus1.ausgabe;
    EXPECT_TRUE(enthaelt(plus1.ausgabe, "/soll.txt endete nach 2 von 3 Zeile(n)")) << plus1.ausgabe;
    EXPECT_FALSE(enthaelt(plus1.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << plus1.ausgabe;
    tmp_raeumen(tmp);
    ASSERT_TRUE(koeder_weg("wc"));

    Lauf const wieder = fall.fahren("", pfad);
    berichten("SignaleUndZwischendateiFehlerSindExit2/Koeder-entfernt", wieder, marke);
    EXPECT_EQ(wieder.code, 0) << wieder.ausgabe;
    EXPECT_TRUE(zeile_exakt(wieder.ausgabe, endzeile_ok(2, 0))) << wieder.ausgabe;
    EXPECT_EQ(tmp_reste(tmp), 0U);
}

// =============================================================================
// (33) BERICHTS- UND EINGABEKANAL-FEHLER SIND EXIT 2 (Lens A r8 LA8-01..04; Fix-r8). Vier Vertragsklassen, alle
//      Vorbestand seit 806629ca, durch die Fix-r7-Kopfsaetze 'Andere Exit-Werte gibt es nicht' erstmals als
//      Anspruch formuliert: (a) die Eingabe-Umleitung 'done < Allowlist' scheitert (0200) -- dash 2 OHNE Zeile,
//      bash 1, busybox leere Schleife (LA8-01); (b) stdout auf /dev/full -- rc 1 = 'ohne Begruendung' fuer einen
//      gruenen Baum (LA8-03); (c) stdout geschlossen -- dieselbe Klasse (EBADF); (d) stderr auf /dev/full am
//      werkzeug_abbruch (git-Koeder Exit 3 an 'ls-files -s') -- rc 1 statt 2 (LA8-04); (e) SIGPIPE: der Bericht
//      geht in eine FIFO, deren Leser sofort schliesst -- rc 141 und TMP-Rest in dash (LA8-02). Der Vorspann
//      'zusatz' von fahren() traegt 'sh -c ... pa1', die Wache laeuft darin per exec; ihr stderr faengt fahre().
//      Rot zuerst gegen 89cf7103 (rc 2 ohne ABBRUCH, 1, 1, 1, 141) -- FIX-r8.md.
// =============================================================================
TEST(Pa1ToteAusnahme, BerichtsUndEingabekanalFehlerSindExit2) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.allowlist_setzen("datei:tests/unit/kommt_vielleicht_" + marke + ".hpp"));
    Lauf const gesund = fall.fahren();
    berichten("BerichtsUndEingabekanalFehlerSindExit2/ohne-Koeder", gesund, marke);
    ASSERT_EQ(gesund.code, 0) << "Das Arrangement ist falsch: der gesunde Baum ist nicht gruen.\n" << gesund.ausgabe;
    fs::path const  tmp = fall.baum() / "tmp";
    std::error_code ec;
    fs::create_directory(tmp, ec);
    ASSERT_FALSE(ec) << ec.message();
    std::string const tmpdir = "TMPDIR=" + zitiert(tmp);

    // (b) stdout auf /dev/full: die erste Berichtszeile scheitert (ENOSPC).
    Lauf const devfull = fall.fahren("", tmpdir + " sh -c 'exec \"$@\" >/dev/full' pa1");
    berichten("BerichtsUndEingabekanalFehlerSindExit2/stdout-dev-full", devfull, marke);
    EXPECT_EQ(devfull.code, 2) << "Ein Schreibfehler am Bericht ist Exit 2, nie 1 (= Befund).\n" << devfull.ausgabe;
    EXPECT_TRUE(zeile_beginnt(devfull.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- Schreiben des Berichts nach stdout"))
        << devfull.ausgabe;
    EXPECT_EQ(tmp_reste(tmp), 0U);

    // (c) stdout geschlossen (EBADF): dieselbe Klasse.
    Lauf const zu = fall.fahren("", tmpdir + " sh -c 'exec \"$@\" >&-' pa1");
    berichten("BerichtsUndEingabekanalFehlerSindExit2/stdout-geschlossen", zu, marke);
    EXPECT_EQ(zu.code, 2) << zu.ausgabe;
    EXPECT_TRUE(zeile_beginnt(zu.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- Schreiben des Berichts nach stdout"))
        << zu.ausgabe;
    EXPECT_EQ(tmp_reste(tmp), 0U);

    // (e) SIGPIPE: eine FIFO, deren Leser nach dem Oeffnen sofort schliesst; die Wache schreibt ihren Bericht
    //     hinein -- ohne PIPE in der trap-Liste stirbt sie mit 141 (dash/busybox ohne EXIT-trap, TMP bleibt).
    fs::path const fifo = fall.baum() / ("fifo_" + marke);
    Lauf const     pipe = fall.fahren("", tmpdir +
                                              " sh -c 'f=\"$1\"; shift; mkfifo \"$f\" || exit 99; "
                                              "( exec 3<\"$f\"; exec 3<&- ) & exec \"$@\" >\"$f\"' pa1 " +
                                              zitiert(fifo));
    berichten("BerichtsUndEingabekanalFehlerSindExit2/SIGPIPE-Leser-schliesst", pipe, marke);
    EXPECT_EQ(pipe.code, 2) << "PIPE muss wie INT/TERM/HUP Exit 2 sein und TMP raeumen.\n" << pipe.ausgabe;
    EXPECT_EQ(tmp_reste(tmp), 0U) << "Das Zwischenverzeichnis blieb nach SIGPIPE liegen.";
    fs::remove(fifo, ec);

    // (d) stderr auf /dev/full am werkzeug_abbruch: ein git-Koeder scheitert an 'ls-files -s' mit Exit 3; die
    //     Abbruchmeldung kann nicht ankommen, der Status muss trotzdem 2 sein (der CI-Konsument liest rc).
    Lauf const wo = im_repo(fall.repo(), "command -v git");
    ASSERT_EQ(wo.code, 0) << wo.ausgabe;
    fs::path const    bin  = fall.repo().pfad() / "koeder_bin";
    std::string const pfad = "PATH=\"" + bin.string() + ":$PATH\"";
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "git",
                                   "#!/bin/sh\n# PATH-Koeder des Falls " + marke +
                                       ": 'ls-files -s' scheitert mit 3.\ncase \"$*\" in *'ls-files -s'*) exit 3 ;; "
                                       "esac\nexec " +
                                       wo.ausgabe + " \"$@\"\n"));
    Lauf const kontrolle = fall.fahren("", pfad + " " + tmpdir);
    berichten("BerichtsUndEingabekanalFehlerSindExit2/git-Koeder-Kontrolle", kontrolle, marke);
    ASSERT_EQ(kontrolle.code, 2) << "Arrangement: der git-Koeder fuehrt nicht zum Werkzeug-Abbruch.\n"
                                 << kontrolle.ausgabe;
    ASSERT_TRUE(zeile_beginnt(kontrolle.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'git ls-files -s'"))
        << kontrolle.ausgabe;
    Lauf const stderr_voll = fall.fahren("", pfad + " " + tmpdir + " sh -c 'exec \"$@\" 2>/dev/full' pa1");
    berichten("BerichtsUndEingabekanalFehlerSindExit2/stderr-dev-full-am-Abbruch", stderr_voll, marke);
    EXPECT_EQ(stderr_voll.code, 2) << "Der Abbruchpfad muss seinen Status 2 auch ohne stderr halten.\n"
                                   << stderr_voll.ausgabe;
    EXPECT_FALSE(enthaelt(stderr_voll.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << stderr_voll.ausgabe;
    EXPECT_EQ(tmp_reste(tmp), 0U);
    fs::remove(bin / "git", ec);
    ASSERT_FALSE(fs::exists(bin / "git"));

    // (a) ALLOWLIST NUR SCHREIBBAR (0200): 'done < Allowlist' scheitert am Oeffnen -- ohne lesbar() beendet dash
    //     die Shell mit 2 OHNE ABBRUCH-Zeile, bash mit 1. Zuletzt, weil root die Datei trotzdem liest (dann
    //     GTEST_SKIP wie in Fall (22)); danach werden die Rechte zurueckgesetzt.
    fs::path const allow = fall.repo().pfad() / "scripts" / "ci_test_registrierungs_allowlist.txt";
    fs::permissions(allow, fs::perms::owner_write, fs::perm_options::replace, ec);
    ASSERT_FALSE(ec) << "chmod 0200 fehlgeschlagen: " << ec.message();
    Lauf const lesbar = fahre("cat " + zitiert(allow) + " > /dev/null");
    if (lesbar.code == 0) {
        fs::permissions(allow,
                        fs::perms::owner_read | fs::perms::owner_write | fs::perms::group_read | fs::perms::others_read,
                        fs::perm_options::replace, ec);
        GTEST_SKIP() << "Die Allowlist laesst sich auf diesem Host nicht unlesbar machen (root?) -- ohne dieses "
                        "Arrangement waere Stufe (a) kein Beweis; (b)-(e) sind gefahren.";
    }
    Lauf const unlesbar = fall.fahren("", tmpdir);
    fs::permissions(allow,
                    fs::perms::owner_read | fs::perms::owner_write | fs::perms::group_read | fs::perms::others_read,
                    fs::perm_options::replace, ec);
    berichten("BerichtsUndEingabekanalFehlerSindExit2/Allowlist-0200", unlesbar, marke);
    EXPECT_EQ(unlesbar.code, 2) << unlesbar.ausgabe;
    EXPECT_TRUE(zeile_beginnt(unlesbar.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- Eingabedatei "
                                                "scripts/ci_test_registrierungs_allowlist.txt nicht lesbar "
                                                "(Eingabe-Umleitung)"))
        << "Ohne lesbar() gibt es keine ABBRUCH-Zeile (dash: Rohstatus 2, bash: 1).\n"
        << unlesbar.ausgabe;
    EXPECT_FALSE(enthaelt(unlesbar.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << unlesbar.ausgabe;
    EXPECT_EQ(tmp_reste(tmp), 0U);
    Lauf const wieder = fall.fahren("", tmpdir);
    berichten("BerichtsUndEingabekanalFehlerSindExit2/Rechte-zurueck", wieder, marke);
    EXPECT_EQ(wieder.code, 0) << wieder.ausgabe;
    EXPECT_TRUE(zeile_exakt(wieder.ausgabe, endzeile_ok(2, 0))) << wieder.ausgabe;
}

// =============================================================================
// (34) FUENF FIX-R8-KLASSEN DER WACHE GOOGLE-SEITIG GEPINNT (Fixer r8b, 2026-09-19). Die Fix-r8-Mutanten M-R8-05,
//      M-R8-07, M-R8-08, M-R8-09 und M-R8-12b (messungen/fix-r8/mutanten/) ueberlebten den Google-Test 35/35 und
//      waren nur shell-seitig getoetet -- ein Rueckbau bliebe in der CI unbemerkt (Klasse LB7-06). Je Stufe der
//      Fix-r8-Fund, das Arrangement und der Pin: (a) ARCHIV-Anker auf Stufe 0 gegen einen Eintrag DARUNTER auf
//      Stufe 2 = UNPRUEFBARER ANKER, ankert nichts (LC6W-01, Wache Folge (15e); M-R8-05: ankert, OK = fail-open);
//      (b) 'wc -c' der Allowlist um 1 Byte zu hoch = Byte-Abgleich Exit 2 (LC6W-03, Folge (15g); M-R8-07: OK);
//      (c) grep-Ziel soll_1.txt liegt schon als 444-Datei mit stale Inhalt = datei_leeren meldet die Zwischendatei
//      (LC6W-05, Folge (15h); M-R8-08: ebenfalls Exit 2, aber als grep-Ausfall gemeldet -- in bash/busybox ein
//      Status-1-Nichttreffer mit stale SOLL, Probe R8-08); (d) Datei NUR auf Stufe 1 und 2, Stufe 3 fehlt
//      (modify/delete) = UNPRUEFBAR (LC6W-06, Folge (15i); M-R8-09: TOT); (e) Gitlink im Index, im Arbeitsbaum ein
//      Symlink = UNPRUEFBAR (LC6W-09, Folge (15j); M-R8-12b: erreichbar, OK). Rot zuerst je Stufe am jeweiligen
//      Mutanten (COMDARE_PA1_WACHE_PFAD), gruen gegen HEAD in 4 Zellen -- FIX-r8.md Abschn. 5b.
// =============================================================================
TEST(Pa1ToteAusnahme, AnkerDFTeilrestGrepZielModifyDeleteGitlinkLinkSindUnpruefbarOderExit2) {
    std::error_code ec;

    // (a) ARCHIV-Anker im D/F-Konflikt: eigener Fall, Waise im Archiv-Ordner (wie Fall (12d)).
    {
        std::string const marke  = koeder();
        std::string const ordner = "tests/deprecated/df_" + marke;
        std::string const anker  = ordner + "/VERMERK.md";
        Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
        ASSERT_TRUE(fall.init());
        ASSERT_TRUE(fall.repo().schreibe_und_verfolge(anker, "# Anker " + marke + "\n"));
        Lauf const vorher = fall.fahren();
        berichten("AnkerDFTeilrest.../Anker-auf-Stufe-0-traegt", vorher, marke);
        ASSERT_EQ(vorher.code, 0) << "Arrangement: der Anker traegt vor dem Konflikt nicht.\n" << vorher.ausgabe;
        ASSERT_TRUE(zeile_exakt(vorher.ausgabe, endzeile_ok(1, 1))) << vorher.ausgabe;
        Lauf const asha = im_repo(fall.repo(), "git hash-object -- " + zitiert(anker));
        ASSERT_EQ(asha.ausgabe.size(), 40U) << "kein SHA-1: '" << asha.ausgabe << "'";
        Lauf const usha = im_repo(fall.repo(), "printf 'unter %s' " + zitiert(marke) + " | git hash-object -w --stdin");
        ASSERT_EQ(usha.code, 0) << usha.ausgabe;
        ASSERT_EQ(usha.ausgabe.size(), 40U) << "kein SHA-1: '" << usha.ausgabe << "'";
        ASSERT_TRUE(fall.repo().schreibe("df_" + marke + ".txt", "100644 " + usha.ausgabe + " 2\t" + anker + "/x\n"));
        Lauf const idx = im_repo(fall.repo(), "git update-index --index-info < " +
                                                  zitiert(fall.repo().pfad() / ("df_" + marke + ".txt")));
        ASSERT_EQ(idx.code, 0) << "git nimmt den Eintrag unter dem Stufe-0-Anker nicht an:\n" << idx.ausgabe;
        Lauf const stufen = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + anker) + " " +
                                                     zitiert(":(literal)" + anker + "/x"));
        ASSERT_TRUE(zeile_exakt(stufen.ausgabe, "100644 " + asha.ausgabe + " 0\t" + anker))
            << "Arrangement: der Anker steht nicht auf Stufe 0:\n"
            << stufen.ausgabe;
        ASSERT_TRUE(zeile_exakt(stufen.ausgabe, "100644 " + usha.ausgabe + " 2\t" + anker + "/x"))
            << "Arrangement: kein Eintrag DARUNTER auf Stufe 2:\n"
            << stufen.ausgabe;
        Lauf const df = fall.fahren();
        berichten("AnkerDFTeilrest.../Anker-Stufe-0-gegen-Eintrag-darunter-Stufe-2", df, marke);
        EXPECT_EQ(df.code, 1) << "Ein Anker im D/F-Konflikt ankert nicht -- ROT (bis 89cf7103 fail-open OK).\n"
                              << df.ausgabe;
        EXPECT_TRUE(zeile_beginnt(df.ausgabe, anker + " -- UNPRUEFBARER ANKER: Stufe-0-Eintrag gegen Index-Eintraege" +
                                                  " DARUNTER (" + anker + "/... auf einer Konfliktstufe, D/F)"))
            << df.ausgabe;
        EXPECT_TRUE(zeile_beginnt(df.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << df.ausgabe;
        EXPECT_TRUE(zeile_exakt(df.ausgabe, fall.waise())) << "Die Waise zaehlt ohne Anker im SOLL.\n" << df.ausgabe;
        EXPECT_TRUE(zeile_exakt(df.ausgabe, nenner_archiv(0, 0, 2))) << df.ausgabe;
        EXPECT_TRUE(zeile_exakt(df.ausgabe, nenner_ohne_bauweg(1, 0, 0, 0, 0))) << df.ausgabe;
        EXPECT_TRUE(zeile_exakt(df.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0))) << df.ausgabe;
    }

    // (b)-(e) in EINEM Fall mit Waise unter tests/unit/; Koeder ueber PATH, TMPDIR fall-eigen (Reste 0).
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    fs::path const tmp = fall.baum() / "tmp";
    fs::create_directory(tmp, ec);
    ASSERT_FALSE(ec) << ec.message();
    std::string const tmpdir = "TMPDIR=" + zitiert(tmp);
    fs::path const    bin    = fall.repo().pfad() / "koeder_bin";
    std::string const pfad   = "PATH=\"" + bin.string() + ":$PATH\" " + tmpdir;
    Lauf const        wo_wc  = im_repo(fall.repo(), "command -v wc");
    ASSERT_EQ(wo_wc.code, 0) << wo_wc.ausgabe;
    Lauf const wo_mktemp = im_repo(fall.repo(), "command -v mktemp");
    ASSERT_EQ(wo_mktemp.code, 0) << wo_mktemp.ausgabe;
    ASSERT_TRUE(fall.allowlist_setzen("datei:tests/unit/kommt_vielleicht_" + marke + ".hpp"));
    Lauf const gesund = fall.fahren("", tmpdir);
    berichten("AnkerDFTeilrest.../ohne-Koeder", gesund, marke);
    ASSERT_EQ(gesund.code, 0) << "Das Arrangement ist falsch: der gesunde Baum ist nicht gruen.\n" << gesund.ausgabe;

    // (b) wc -c der Allowlist um 1 Byte zu hoch (Rezept wie (32f), Muster '-c' + Allowlist-Pfad).
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "wc",
                                   "#!/bin/sh\ncase \"$*\" in\n    -c*ci_test_registrierungs_allowlist.txt*) out=$(" +
                                       wo_wc.ausgabe +
                                       " \"$@\") || exit $?; n=${out%% *}; rest=${out#* }; "
                                       "printf '%s %s\\n' \"$((n+1))\" \"$rest\"; exit 0 ;;\nesac\nexec " +
                                       wo_wc.ausgabe + " \"$@\"\n"));
    Lauf const plus1 = fall.fahren("", pfad);
    berichten("AnkerDFTeilrest.../wc-c-plus-1-an-der-Allowlist", plus1, marke);
    EXPECT_EQ(plus1.code, 2) << "Ein Byte-Teilrest ist ein Werkzeug-Ausfall -- Exit 2, nie OK.\n" << plus1.ausgabe;
    EXPECT_TRUE(zeile_beginnt(plus1.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'read' ueber "
                                             "scripts/ci_test_registrierungs_allowlist.txt lieferte "))
        << plus1.ausgabe;
    EXPECT_TRUE(enthaelt(plus1.ausgabe, " Byte(s) -- Lesefehler, Teilrest oder NUL-Byte (Exit 1).")) << plus1.ausgabe;
    EXPECT_FALSE(enthaelt(plus1.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << plus1.ausgabe;
    EXPECT_EQ(tmp_reste(tmp), 0U);
    fs::remove(bin / "wc", ec);
    ASSERT_FALSE(fs::exists(bin / "wc"));

    // (c) grep-Ziel soll_1.txt liegt schon (444, stale Inhalt): der mktemp-Koeder legt es im frischen TMP an.
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "mktemp",
                                   "#!/bin/sh\nd=$(" + wo_mktemp.ausgabe +
                                       " \"$@\") || exit 1\nprintf 'stale\\n' > \"$d/soll_1.txt\"\n"
                                       "chmod 444 \"$d/soll_1.txt\"\nprintf '%s\\n' \"$d\"\n"));
    Lauf const stale = fall.fahren("", pfad);
    berichten("AnkerDFTeilrest.../grep-Ziel-stale-und-444", stale, marke);
    EXPECT_EQ(stale.code, 2) << stale.ausgabe;
    EXPECT_TRUE(zeile_beginnt(stale.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- Zwischendatei ")) << stale.ausgabe;
    EXPECT_TRUE(enthaelt(stale.ausgabe, "/soll_1.txt nicht anlegbar (Exit ")) << stale.ausgabe;
    EXPECT_FALSE(enthaelt(stale.ausgabe, "'grep' -v"))
        << "Das unbeschreibbare Ziel muss VOR dem grep gemeldet werden, nicht als grep-Ausfall.\n"
        << stale.ausgabe;
    EXPECT_FALSE(enthaelt(stale.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << stale.ausgabe;
    EXPECT_EQ(tmp_reste(tmp), 0U) << "Der EXIT-trap muss das Zwischenverzeichnis samt 444-Datei raeumen.";
    tmp_raeumen(tmp);
    fs::remove(bin / "mktemp", ec);
    ASSERT_FALSE(fs::exists(bin / "mktemp"));

    // (d) modify/delete: ext/dm_<marke>/d als Datei NUR auf Stufe 1 und 2 (zwei Blobs), Stufe 3 fehlt.
    std::string const dm = "ext/dm_" + marke + "/d";
    Lauf const        b1 = im_repo(fall.repo(), "printf 'v1 %s' " + zitiert(marke) + " | git hash-object -w --stdin");
    Lauf const        b2 = im_repo(fall.repo(), "printf 'v2 %s' " + zitiert(marke) + " | git hash-object -w --stdin");
    ASSERT_EQ(b1.ausgabe.size(), 40U) << "kein SHA-1: '" << b1.ausgabe << "'";
    ASSERT_EQ(b2.ausgabe.size(), 40U) << "kein SHA-1: '" << b2.ausgabe << "'";
    ASSERT_NE(b1.ausgabe, b2.ausgabe);
    ASSERT_TRUE(fall.repo().schreibe("dm_" + marke + ".txt", "100644 " + b1.ausgabe + " 1\t" + dm + "\n100644 " +
                                                                 b2.ausgabe + " 2\t" + dm + "\n"));
    Lauf const didx = im_repo(fall.repo(), "git update-index --index-info < " +
                                               zitiert(fall.repo().pfad() / ("dm_" + marke + ".txt")));
    ASSERT_EQ(didx.code, 0) << didx.ausgabe;
    Lauf const dst = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + dm));
    ASSERT_TRUE(zeile_exakt(dst.ausgabe, "100644 " + b1.ausgabe + " 1\t" + dm)) << dst.ausgabe;
    ASSERT_TRUE(zeile_exakt(dst.ausgabe, "100644 " + b2.ausgabe + " 2\t" + dm)) << dst.ausgabe;
    ASSERT_FALSE(enthaelt(dst.ausgabe, " 3\t" + dm)) << "Arrangement: Stufe 3 steht:\n" << dst.ausgabe;
    ASSERT_FALSE(enthaelt(dst.ausgabe, " 0\t" + dm)) << "Arrangement: Stufe 0 steht:\n" << dst.ausgabe;
    std::string const unter_d = dm + "/x_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter_d));
    Lauf const md = fall.fahren("", tmpdir);
    berichten("AnkerDFTeilrest.../modify-delete-Stufe-3-fehlt", md, marke);
    EXPECT_EQ(md.code, 1) << md.ausgabe;
    EXPECT_TRUE(zeile_beginnt(
        md.ausgabe,
        fall.waise() + " -- UNPRUEFBAR: \"" + unter_d + "\" existiert nicht, und sein Vorfahr " + dm +
            " steht im Index im MERGE-KONFLIKT mit ungleichen Typen je Stufe" +
            " (Stufe 1: Datei, Stufe 2: Datei; Stufe 3 fehlt (modify/delete:" + " ein Ausgang loescht die Datei))"))
        << md.ausgabe;
    EXPECT_FALSE(enthaelt(md.ausgabe, "ist eine getrackte DATEI"))
        << "Eine Datei nur auf zwei Konfliktstufen ist kein Datei-Ahne (TOT): der Ausgang, der sie loescht, kennt"
           " keinen.\n"
        << md.ausgabe;
    EXPECT_TRUE(zeile_exakt(md.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << md.ausgabe;
    EXPECT_TRUE(zeile_exakt(md.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << md.ausgabe;
    EXPECT_EQ(tmp_reste(tmp), 0U);

    // (e) Gitlink im Index, im Arbeitsbaum ein Symlink auf ein echtes Verzeichnis.
    std::string const gl   = "ext/gl_" + marke;
    std::string const gsha = "0000000000000000000000000000000000000002";
    Lauf const        gidx = fahre("cd " + zitiert(fall.repo().pfad()) + " && " + WegwerfRepo::umgebung() +
                                   " git update-index --add --cacheinfo 160000," + gsha + "," + gl);
    ASSERT_EQ(gidx.code, 0) << "Gitlink konnte nicht in den Index gelegt werden:\n" << gidx.ausgabe;
    fs::create_directories(fall.repo().pfad() / "ext" / ("glreal_" + marke), ec);
    ASSERT_FALSE(ec) << ec.message();
    fs::create_symlink("glreal_" + marke, fall.repo().pfad() / gl, ec);
    ASSERT_FALSE(ec) << "Symlink nicht anlegbar: " << ec.message();
    ASSERT_TRUE(fs::is_symlink(fall.repo().pfad() / gl)) << "Arrangement: der Gitlink-Pfad ist kein Link.";
    Lauf const gst = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(":(literal)" + gl));
    ASSERT_TRUE(zeile_exakt(gst.ausgabe, "160000 " + gsha + " 0\t" + gl)) << gst.ausgabe;
    std::string const unter_g = gl + "/y_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + unter_g));
    Lauf const gs = fall.fahren("", tmpdir);
    berichten("AnkerDFTeilrest.../Gitlink-im-Arbeitsbaum-Symlink", gs, marke);
    EXPECT_EQ(gs.code, 1) << "Index und Arbeitsbaum widersprechen sich -- UNPRUEFBAR, nicht 'erreichbar'.\n"
                          << gs.ausgabe;
    EXPECT_TRUE(zeile_beginnt(gs.ausgabe, fall.waise() + " -- UNPRUEFBAR: \"" + unter_g +
                                              "\" existiert nicht, und sein Vorfahr " + gl +
                                              " ist im Index ein GITLINK, im Arbeitsbaum aber ein SYMLINK"))
        << gs.ausgabe;
    EXPECT_TRUE(zeile_exakt(gs.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << gs.ausgabe;
    EXPECT_TRUE(zeile_exakt(gs.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << gs.ausgabe;
    EXPECT_EQ(tmp_reste(tmp), 0U);
}

#endif // _WIN32
