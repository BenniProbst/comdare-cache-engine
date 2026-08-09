// test_ms1_arenen_kein_alloc_im_fenster.cpp -- MS-1: DER AUFNAHME-PFAD ALLOZIERT IM FENSTER NICHT.
//
// SELBSTCHECK: dieser Test behauptet NICHTS ueber Messwerte, Latenzen oder Statistik. Er behauptet
// genau eine Sache und prueft sie aus mehreren Richtungen: zwischen dem Scharfstellen und dem
// Entschaerfen des Zaehlers ruft der Aufnahme-Pfad KEIN operator new. Wer hier eine Zeitnahme oder
// eine Perzentil-Rechnung sucht, ist in der falschen Datei. Die Seitenfehler-Frage haelt
// test_ms2_pre_touch_seitenfehler, und zwar mit einer anderen Observablen (ru_minflt).
//
// == DIE ROTE AUSGABE VOR DEM BAU, WOERTLICH =======================================================
// Erste Fassung dieser Datei, gebaut und gefahren am 09.08.2026, BEVOR es die Arenen gab. Damals
// existierte im Haus nur EIN Aufnahme-Pfad -- der Bestands-Pfad -- und genau der lief durch das
// Fenster, mit der Forderung "0 Allokationen":
//
//     == MS-1: der Aufnahme-Pfad alloziert im Fenster nicht ==
//     -- (1) Bestands-Pfad InMemoryMeasurementBuffer::append_record --
//          Appends im Fenster = 4096, Allokationen = 13, angeforderte Bytes = 524224
//       [ERR] Bestands-Pfad: Allokationen im Mess-Fenster
//     -- (2) Gegenprobe: der Zaehler sieht drei absichtliche Allokationen --
//          erwartet = 3, gezaehlt = 3
//     FEHLGESCHLAGEN: 1 Pruefung(en)
//     RC=1
//
// 13 Umlagerungen und 524224 angeforderte Bytes -- ueber ein halbes Megabyte Belegung samt Umkopieren
// MITTEN in einem Fenster von 4096 Operationen, deren Latenz gemessen werden soll.
//
// == WAS SICH AN ABSCHNITT (1) SEITHER GEAENDERT HAT, UND WARUM DAS KEINE AUFWEICHUNG IST =========
// Abschnitt (1) fordert heute das GEGENTEIL: der Bestands-Pfad MUSS allozieren. Das ist keine
// entschaerfte Zusage, sondern ein Rollenwechsel. Der Bestands-Pfad ist nicht mehr der Pruefling --
// das sind seit Abschnitt (3) die beiden Arenen. Er bleibt als KONTRAST stehen, und er leistet dabei
// zwei Dinge, die kein Kommentar leisten koennte: er belegt fortlaufend, dass der Zaehler echte
// Allokationen sieht (waere er blind, zeigte er auch hier 0), und er haelt den Bestands-Defekt
// sichtbar, statt ihn wegzuloeschen. Die Zusage "0" ist unveraendert; sie steht in (3).
//
// == DER BESTANDS-DEFEKT, am Objekt =================================================================
// ThreadArena (include/cache_engine/measurement/thread_arena.hpp) traegt den richtigen Namen, das
// alignas(64) und den Kommentar "append-only" -- und haelt einen std::vector als Member (Z.35), in
// den append() ohne jedes reserve hineinlegt (Z.19). Die Funktion ist zusaetzlich als noexcept
// deklariert, obwohl der Weg darunter werfen kann. InMemoryMeasurementBuffer setzt darauf auf und
// nennt seinen Pfad im Kommentar "Hot-Path: pro-Thread-Arena, kein Lock" (Z.42). Wer diese beiden
// als Vorlage nimmt, baut den Defekt nach. Der static_assert auf MessArenaHeiss (mess_arena.hpp)
// faengt diese FEHLERKLASSE beim Uebersetzen, WENN JEMAND SIE DORTHIN TRAEGT: ein Vektor-Member
// zerstoert trivially_copyable. Er faengt nicht den Defekt in ThreadArena selbst -- eine Zusicherung
// in mess_arena.hpp kann ueber eine fremde Datei nichts aussagen. Das stand hier bis zum 09.08.2026
// missverstaendlich; sichtbar bleibt der Bestands-Defekt ueber Abschnitt (1), der ihn MISST.
//
// == DER RUECKNAHME-BELEG HAENGT AM COMMIT, NICHT AN EINER /tmp-KOPIE (Nachtrag 09.08.2026) ========
//
// SELBSTCHECK dieses Blocks: jede Zahl unten ist an DIESER Maschine erhoben und woertlich
// uebernommen. Die md5-Werte sind gegen den COMMIT-BLOB gezogen, nicht gegen eine Datei im
// Arbeitsbaum. Der Koeder ist frisch gewuerfelt und NICHT aus dem Commit-Text abgeschrieben -- die
// drei dort protokollierten Mutanten sind ausdruecklich NICHT wiederverwendet.
//
// BEFUND, DER DIESEN NACHTRAG ERZWINGT: die Paketmeldung zu MS-1 belegte die Ruecknahme der drei
// Mutanten mit einem md5-Vergleich gegen eine Sicherung unter /tmp und nannte
// 3eee9322d14693997a8bb43649ebf788 als "erwartet". Der Commit f24c6ea9 traegt fuer stapel_arena.hpp
// aber e026d4c3fdd9d868ec6b17fe333ef926. Der Beleg reproduziert damit NICHT gegen den Commit:
// zwischen Ruecknahme (12:00) und Commit (12:06) lief ein clang-format-Pass (mtime 12:03) und brach
// die Byte-Identitaet.
//
// ZWEI DINGE STIMMEN TROTZDEM, beide nachgerechnet statt behauptet:
//   * Der Unterschied ist AUSSCHLIESSLICH Whitespace -- zwei Hunks: Member-Ausrichtung (Z.77-79) und
//     sauber() ein- statt dreizeilig (Z.118-120). Kein Zeichen Logik.
//   * MUTANT-Treffer in den vier Commit-Blobs: 0, 0, 0, 0. Gegenprobe, dass derselbe Aufruf
//     ueberhaupt zaehlt: dasselbe Kommando auf "StapelArena" -> 14 Treffer in stapel_arena.hpp.
//
// WARUM md5-IDENTITAET DAS FALSCHE WERKZEUG WAR. Byte-Gleichheit gegen eine private /tmp-Sicherung
// beweist nur "unveraendert seit MEINEM Schnappschuss". Sie ist (a) fuer niemanden sonst
// reproduzierbar, weil die Sicherung nicht im Repo liegt, und (b) bricht bei jedem Format-Lauf, ohne
// dass sich Logik geaendert haette. Das tragfaehige Werkzeug liegt bereits vor: DIESER TEST. Jeder
// Mutant, der eine Zusage verletzt, toetet hier eine BENANNTE Pruefung -- also ist ein gruener Lauf
// dieses Tests, gebaut aus dem COMMITTETEN Baum, der Ruecknahme-Beleg. Er ist format-immun, haengt am
// Commit und faehrt bei jedem ctest mit, statt als Prosa zu verjaehren.
//
// == DER BISS-BEWEIS FUER GENAU DIESES WERKZEUG, WOERTLICH =========================================
// Koeder-Wurf 09.08.2026: Rohwurf 181 aus /dev/urandom, Index 181 % 4 = 1 aus vier VORHER benannten
// Kandidaten, Kennzeichen 38c48a8fcbd5. Gewaehlt ist damit M1: in hinauf() wird die
// Kapazitaetspruefung ">=" zu ">". VOR dem Lauf vorhergesagt: die fuenfte Ablage bei top == 4 wird
// faelschlich angenommen, also muessen (6b), (6c) und (6e) reissen, waehrend Abschnitt (3) (Tiefe 64,
// Hoechststand 1) die Grenze nie beruehrt und gruen bleiben muss.
//
//   ROT, mit dem Koeder im Baum:
//     -- (6) Koeder Stapel-Arena: Ueberlauf ist ein PROGRAMMIERFEHLER --
//          stapel_arena: ausgeglichen (Hoechststand 5 von 4 Plaetzen)
//       [ERR] (6b) die fuenfte wird abgewiesen
//       [ERR] (6c) zu_tief = 1
//       [ERR] (6d) der abgewiesene Koeder liegt nicht auf dem Stapel
//       [ERR] (6e) der Hoechststand ist 4, nicht 5
//       [ERR] (6f) ein unpaariges AUS, eigener Zaehler
//       [ERR] (6h) die Meldung nennt die Fehlerklasse Programmierfehler
//     FEHLGESCHLAGEN: 6 Pruefung(en)        RC=1
//
//   GRUEN, nach `git checkout --` derselben Datei:
//     -- (6) Koeder Stapel-Arena: Ueberlauf ist ein PROGRAMMIERFEHLER --
//          stapel_arena: PROGRAMMIERFEHLER -- zu_tief = 1 (Verschachtelung ueber 4 Plaetze),
//          unpaariges_aus = 1, offen_geblieben = 0 (EIN ohne AUS, Hoechststand 4)
//     OK: MS-1                              RC=0
//
// Alle drei vorhergesagten Pruefungen sind gerissen (drei weitere dazu), und die Abschnitte (3), (5)
// und (7) blieben gruen -- der Koeder wird also von den GRENZ-Pruefungen gefangen und nicht von einer
// fremden Zeile. Nebenbefund, ehrlich benannt: die Text-Wache in (9) blieb bei "verbotene Woerter =
// 0", obwohl das Wort MUTANT im heissen Block stand. Sie ist eine Belegungs-Wache, KEIN
// Mutanten-Melder; wer sie dafuer haelt, verlaesst sich auf eine Null, die nichts zusichert.
//
// RUECKNAHME, gegen den COMMIT-BLOB gezogen -- das ist der Punkt dieses Nachtrags:
//     git show f24c6ea9:<pfad>/stapel_arena.hpp | md5sum -> e026d4c3fdd9d868ec6b17fe333ef926
//     md5sum <Arbeitsbaum>/stapel_arena.hpp              -> e026d4c3fdd9d868ec6b17fe333ef926
//     grep -c 38c48a8fcbd5 <Arbeitsbaum>                 -> 0  (rc=1; Gegenprobe: dasselbe Kommando
//                                                               auf der Wurf-Datei -> 1)
//     git status --short                                 -> leer
//
// REGEL FUER KUENFTIGE PAKETE: der Ruecknahme-Beleg wird gegen den COMMIT gezogen
// (`git show <sha>:<pfad> | md5sum`), nie gegen eine Sicherung ausserhalb des Repos, und er wird ERST
// NACH dem letzten clang-format-Lauf erhoben. Der tragende Beleg bleibt aber der gruene Lauf dieses
// Tests aus dem committeten Baum -- der ueberlebt jede Umformatierung.
//
// == DIE OPTIMIERUNGSSTUFE HAT DEN KOEDER GEFRESSEN (Nachtrag 09.08.2026) =========================
//
// SELBSTCHECK dieses Blocks: jede Zahl ist an dieser Maschine erhoben und woertlich uebernommen.
// Es wird KEINE Zusage abgeschwaecht -- (3) fordert unveraendert 0. Geaendert ist ausschliesslich,
// wie der KOEDER und der ABLESEVORGANG gegen den Uebersetzer gesichert sind.
//
// BEFUND AUS DER CI (Pipeline 15437, Job 369474, ce/development), woertlich:
//     -- (2) Gegenprobe: der Zaehler sieht drei absichtliche Allokationen --
//          erwartet = 3, gezaehlt = 1, Nutzsumme = 48
//       [ERR] Gegenprobe: der Zaehler zaehlt
//     -- (4) Gegenprobe K13: eine absichtliche Allokation im gemessenen Bereich --
//          Koeder-Groesse = 38763, gezaehlt = 0, Bytes = 0
//       [ERR] K13: der Zaehler sieht den Koeder im Arenen-Fenster
//       [ERR] K13: die Byte-Zahl ist exakt die Koeder-Groesse
//     FEHLGESCHLAGEN: 3 Pruefung(en)
//
// URSACHE, am Objekt entschieden: die Optimierungsstufe, NICHT die Arenen. Der lokale Bau faehrt mit
// CMAKE_BUILD_TYPE leer (also -O0), die CI faehrt den GNU-Weg ./configure.sh, und der setzt
// build_type="Release" -> -O3 -DNDEBUG. Nachgestellt mit DERSELBEN Uebersetzungseinheit, nur die
// Stufe getauscht: -O0 gruen, -O1 rot, -O2 rot, -O3 rot -- Zeichen fuer Zeichen die CI-Ausgabe,
// inklusive "Nutzsumme = 48". Es reisst bereits ab -O1, es ist nichts -O3-Eigenes.
//
// DER ZAEHLER WAR NICHT BLIND -- das ist die erste der beiden moeglichen Ursachen, und sie ist
// ausgeschlossen: Abschnitt (1) meldet bei -O3 EXAKT dieselben Zahlen wie bei -O0 (Allokationen = 13,
// angeforderte Bytes = 524224). Die operator-new-Ersetzung greift also auch im CI-Bau vollstaendig.
// Es war der KOEDER, und zwar ueber ZWEI VERSCHIEDENE Mechanismen, nicht einen:
//
//   * Abschnitt (2) starb an GCCs -fallocation-dce ("remove unused C++ allocations", per Vorgabe an).
//     Zwei der drei new/delete-Paare wurden ersatzlos entfernt; uebrig blieb der explizite
//     ::operator new(64) -> gezaehlt = 1. Gemessen: -O3 -fno-allocation-dce -> gezaehlt = 3.
//   * Abschnitt (4) starb an etwas ANDEREM. Die Belegung wurde bei GCC sehr wohl ausgefuehrt -- im
//     Assembler steht "movl $38763, %edi; call _Znwm". Trotzdem wurde eine KONSTANTE 0 gedruckt
//     (xorl %esi,%esi an beiden Druckstellen): unter -fassume-sane-operators-new-delete (Vorgabe)
//     nimmt GCC an, die ersetzbaren globalen Allokationsoperatoren lesen und schreiben keinen
//     globalen Speicher, und faltet g_neu_zaehler/g_neu_bytes ueber den Aufruf hinweg auf ihren
//     Wert VOR dem Aufruf. Der Zaehler zaehlte zur Laufzeit; sein Ergebnis wurde zur
//     UEBERSETZUNGSZEIT wegoptimiert.
//
// WARUM DAS ABSCHNITT (3) MITREISST, und warum das hier der eigentliche Punkt ist: derselbe
// Faltungs-Mechanismus trifft die ZUSAGE. Nach scharf_stellen() weiss der Uebersetzer
// "g_neu_zaehler == 0"; nimmt er zusaetzlich an, dass keine Allokation daran ruehrt, dann ist die
// gedruckte "Allokationen = 0" in (3) bei -O3 eine KONSTANTE und keine Messung. Sie waere auch dann
// 0, wenn die Arenen allozierten. Genau deshalb war (4) da, und genau deshalb hat (4) angeschlagen.
// Ein gruenes (3) ohne ein gruenes (4) ist bei -O1 und hoeher wertlos.
//
// WARUM KEIN UEBERSETZER-SCHALTER: -fno-assume-sane-operators-new-delete heilt (4) bei GCC, aber
// clangs Gegenstueck -fno-assume-sane-operator-new heilt es NICHT -- bei clang ist der Aufruf gar
// nicht mehr da (echte allocation elision, im -O3-Assembler kommt die Groesse nur noch an der
// Druckstelle vor). Auch die Zaehler-Globalen volatile zu machen heilt nur GCC, und dort mit
// -Wvolatile-Warnung. Beide uebersetzer-eigenen Wege sind unvollstaendig; das Haus baut auf beiden.
//
// DIE HEILUNG, und was sie ausdruecklich NICHT ist. Die billige Heilung waere gewesen, (2) und (4)
// zu entschaerfen -- sie sind der EINZIGE Grund, warum dieser Defekt sichtbar wurde; wer sie
// weglaesst, kauft sich ein Gruen, das nichts bedeutet. Geaendert ist stattdessen:
//   (a) eine undurchsichtige Schranke (leeres asm, "memory") auf jedem Koeder-Zeiger. Sie laesst den
//       Zeiger in ein dem Uebersetzer unbekanntes Register entkommen -- damit ist die Belegung nicht
//       mehr als tot beweisbar -- und erklaert zugleich den gesamten Speicher fuer veraendert.
//   (b) dieselbe Schranke IM FENSTERRAHMEN, in scharf_stellen()/entschaerfen()/bytes(). Damit ist
//       der Zaehlerstand aus dem SPEICHER zu lesen statt aus einer Annahme, und nichts darf ueber
//       die Fenstergrenzen hinweg verschoben werden. Erst dadurch ist die "0" in (3) eine gemessene.
//   (c) die Koeder-Groesse wird ZUR LAUFZEIT gewuerfelt (siehe koeder_groesse_wuerfeln). Was im
//       Quelltext steht, kennt der Uebersetzer und darf darueber rechnen; was erst zur Laufzeit
//       gelesen wird, kennt er nicht. Das erfuellt K13 im strengsten Sinn: frisch bei JEDEM Lauf,
//       nicht abschreibbar. Ein blosses "benutze den Wert" genuegt NICHT -- das tat der Test schon
//       (Nutzsumme = 48 wurde weiterhin korrekt gedruckt) und half nicht.
//
// WELCHES MITTEL TRAEGT WAS -- einzeln nachgemessen, statt drei Mittel pauschal zu behaupten:
//   * Ohne die Schranken, aber MIT gewuerfelter Groesse, faellt der Test bei -O3 wieder um. Das
//     Wuerfeln allein heilt also NICHT; die Schranke ist das tragende Mittel.
//   * Mit den Schranken, aber MIT fester Groesse im Quelltext, ist er gruen. Das Wuerfeln ist damit
//     nicht das, was heilt -- es ist die K13-Haerte (frisch je Lauf, nicht abschreibbar) und der
//     Riegel gegen kuenftige Uebersetzer, die ueber eine bekannte Groesse rechnen wollen.
// Beide Zahlen stehen im Paketbericht; sie sind der Grund, warum beide Mittel bleiben.
//
// GEMESSEN, beide Uebersetzer auf voller CI-Stufe, nach der Heilung:
//     /usr/bin/c++ -O3 -DNDEBUG -> (1) 13/524224  (2) gezaehlt = 3  (3) Allokationen = 0
//                                  (4) gezaehlt = 1, Bytes = <gewuerfelt>   OK: MS-1   RC=0
//     clang++      -O3 -DNDEBUG -> dieselben Werte                          OK: MS-1   RC=0
//
// DER BLINDE ZAEHLER WIRD WEITERHIN ROT, NICHT STILL GRUEN. Das ist keine Absicht auf dem Papier,
// sondern die Bauform: (1) fordert zahl > 0, (2) fordert genau 3, (4) fordert genau 1 UND die exakte
// Byte-Zahl -- und (4) tut das INNERHALB des Arenen-Fensters, also auf demselben Pfad wie die Zusage.
// Faellt die Zaehlung aus, faellt sie an allen dreien nach unten, und "0" ist dort ein FEHLER, kein
// Erfolg. Gilt auch fuer die Schranke selbst: waere sie auf einem kuenftigen Uebersetzer wirkungslos,
// verschwaende der Koeder wieder und (2)/(4) wuerden rot -- der Test kann sich nicht in ein stilles
// Gruen retten.
//
// NEBENBEFUND, der eigenstaendig zaehlt und NICHT hier geheilt wird: der lokale Bau laeuft ohne
// Optimierung, die CI mit -O3. Jeder Test, der eine Uebersetzer-Beobachtbarkeit zusichert, ist lokal
// strukturell schwaecher als in der CI. MS-1 war lokal gruen und fiel erst in der CI um -- das ist
// keine Flakiness, sondern eine Luecke zwischen lokalem und offiziellem Bauweg.
//
// Build: Standalone int main() (kein gtest -- ein Test-Framework alloziert im Fenster und machte die
// Messung wertlos). Globale operator-new/delete-Ersetzung in DIESER TU gilt programmweit.
//
// KEIN ZUFALL AUSSER BEI KOEDERN (Haus-Doktrin). Die Koeder-Werte unten sind am 09.08.2026 frisch
// aus /dev/urandom gewuerfelt und NICHT aus einer Doku abgeschrieben; die Koeder-GROESSE in (4) wird
// zusaetzlich bei jedem Lauf neu gewuerfelt.
//
// == DER STELLVERTRETER: NEUN ABSCHNITTE, UND KEINER LAS JE EINE MESSZEILE (Nachtrag 09.08.2026) ===
//
// SELBSTCHECK dieses Blocks: jede Zahl ist an dieser Maschine erhoben. Jeder genannte Mutant ist
// GEFAHREN, in BEIDEN Stufen -- lokal ohne Optimierung UND mit den CI-Flags (-O3 -DNDEBUG), weil
// eine Beobachtbarkeits-Zusage genau zwischen diesen beiden Stufen schon einmal auseinandergefallen
// ist (s. Block darueber). KEINE bestehende Pruefung wurde entschaerft; es sind nur welche
// dazugekommen.
//
// DER BEFUND. Die Abschnitte (1) bis (9) pruefen die Mess-Arena ausschliesslich NEGATIV oder
// INDIREKT: (5d) und (5c) fordern, dass etwas NICHT dasteht; (3) liest `belegt`, das aus dem
// Versuchs-Zaehler abgeleitet wird und die Nutzlast nie beruehrt; (7) liest den Stapel zurueck,
// nicht die Mess-Arena. Ein Mutant, der in anhaengen() statt der uebergebenen Zeile ein leeres
// MessCheckpointZeile{} ablegt, lief deshalb durch alle neun Abschnitte und meldete "OK: MS-1",
// RC=0. Die Arena haette nur Nullen schreiben koennen -- richtiges Messgeraet, falscher Gegenstand.
// Dagegen steht seit heute Abschnitt (10).
//
// DIE MUTANTEN, gefahren gegen den fertigen Baum. Links der Eingriff, rechts die Pruefungen, die
// gerissen sind -- in BEIDEN Stufen dieselben:
//   A1 leere Zeile geschrieben ............ (10c)(10d)(10e)(10f)(10g)(10i)
//   A2 Zeile auf platz^1 abgelegt .......... (5c) + (10c)(10d)(10e)(10f)(10g)(10i)
//   A3 zeit_ticks <-> messwert vertauscht .. (10c)(10d)(10i)  -- die drei anderen Felder blieben gruen
//   A4 auf kapazitaet-1-platz geschrieben .. (10c)..(10i) + (10j), das die Umkehrung ausdruecklich nennt
//   B1 sauber() ohne unpaarige_aus ......... (12g)(12h)(12i)(12k)
//   B2 sauber() ohne zu_tief ............... (12e)
//   B3 sauber() konstant false ............. (12b)(12c)  -- die Positivkontrolle, ohne die B1/B2 wertlos waeren
//   C1 befund() vertauscht die Zaehler ..... (11a)(11b)(11h)(11i)(12d)(12f)(12i)
//   C2 befund_zeile vertauscht die Zahlen .. (11h)(11i)(12i)
//   C3 befund() vertauscht Kap./Hoechst. ... (12j)(12k)
//   D1 Kapazitaets-Waechter entfernt ....... (13b)(13c)
//   D2 Planer-Formel ohne Waechter ......... UEBERSETZUNG BRICHT (beide static_asserts in (13))
//   E1 zu_tief zurueck auf uint32 .......... UEBERSETZUNG BRICHT (Breiten-Symmetrie, stapel_arena.hpp)
//   E2 nur StapelBefund auf uint16 ......... (14a)(14b)(14e)  -- der Riss, den E1 NICHT abdeckt
//   F1 geschrieben == snprintf-Rueckgabe ... (15a)(15b)(15c)(15d)(15g)(15h)
//   G1 Nenner 4 -> 40 in der Meldung ....... (5f)  -- die alte Suche "von 4" haette "von 40" durchgelassen
//   G2 heisser Zustand nicht auf Offset 0 .. (8e)
//   G3 reservieren(0) ohne freigeben() ..... (16c)
//   G4 speicher_aufbauen kurzgeschlossen ... (16e)
//   G6 kSeitenBytes auf 8192 verstellt ..... MS-2 (0) und (b)
// A3 ist der Beleg dafuer, dass die Feld-Zaehler in (10) DISJUNKT sind und nicht pauschal
// mitreissen. G6 ist zugleich der Beleg fuer die verschaerfte Schranke in MS-2 (b): die gemessenen
// 16384 Fehler sind bei kErwarteteSeiten = 8192 exakt das Doppelte -- die frueher dort stehende
// EINSCHLIESSENDE Grenze (<= kErwarteteSeiten * 2) haette diesen Mutanten durchgelassen.
//
// RUECKNAHME, gegen die COMMIT-BLOBS gezogen: `grep -c` auf den Wurf und auf "MUTANT-" liefert 0 in
// mess_arena.hpp, stapel_arena.hpp, checkpoint_speicher.hpp, mess_speicher_kanon.hpp und
// test_ms2 -- und ZWEI in dieser Datei. Beide sind Prosa, und die zweite ist DIESER SATZ selbst: er
// nennt das Suchmuster und faellt deshalb in seine eigene Suche. Das wird hier ausgeschrieben statt
// weggekuerzt, denn ein Ruecknahme-Beleg, der seine eigene Zahl nicht erklaert, ist keiner. Die
// erste ist Zeile 63, Prosa aus dem Vorgaenger-Nachtrag. Kein Eingriff blieb im Code stehen;
// Gegenprobe, dass derselbe Aufruf ueberhaupt zaehlt: "MessCheckpointZeile" -> 13 Treffer in
// mess_arena.hpp. Der tragende Beleg bleibt wie gehabt der gruene Lauf dieses Tests aus dem
// committeten Baum -- der ueberlebt jede Umformatierung und jede Zeilennummer.

#include <builder/measure_storage/checkpoint_speicher.hpp>
#include <builder/measure_storage/mess_arena.hpp>
#include <builder/measure_storage/stapel_arena.hpp>

#include <cache_engine/measurement/in_memory_measurement_buffer.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <new>
#include <string>
#include <string_view>

// ===================================================================================================
// DER INSTRUMENTIERTE ALLOCATOR -- globale Ersetzung, thread-unabhaengig (Measure ist 1-Faden).
//
// SELBSTCHECK: der Zaehler selbst darf NICHT allozieren. Deshalb rohe malloc/free und rohe
// std::uint64_t-Globale mit konstanter Initialisierung (kein dynamischer Initialisierer, keine
// Initialisierungs-Reihenfolge-Falle). Er zaehlt NUR, wenn er scharf ist -- alles ausserhalb des
// Fensters (Aufbau, Ausgabe, iostream) bleibt unberuehrt und darf frei allozieren.
//
// WAS ER NICHT SIEHT, ausdruecklich: C-malloc ohne operator new (z.B. in vendorierten C-Pfaden).
// Die Ersetzung von operator new faengt den C++-Pfad, nicht den C-Pfad. Fuer den hier geprueften
// Aufnahme-Pfad ist diese Luecke geschlossen, weil Abschnitt (9) zusaetzlich TEXTLICH prueft, dass
// die heissen Bloecke weder die eine noch die andere Belegung nennen -- zwei unabhaengige
// Beobachtungen derselben Zusage, von denen keine die andere verdeckt.
// ===================================================================================================
namespace ms_wache {

inline std::uint64_t g_neu_zaehler = 0;
inline std::uint64_t g_neu_bytes   = 0;
inline bool          g_scharf      = false;

// -- DIE UNDURCHSICHTIGE SCHRANKE ------------------------------------------------------------------
// Ein leeres asm-Stueck mit "memory"-Clobber. Es erzeugt keinen einzigen Befehl, erklaert dem
// Uebersetzer aber, dass an dieser Stelle beliebiger Speicher gelesen und geschrieben worden sein
// kann. Damit faellt genau die Annahme weg, die (4) in der CI toetete: dass ein Aufruf von
// operator new den Zaehlerstand nicht beruehrt. Der Stand wird wieder aus dem SPEICHER geholt.
//
// SELBSTCHECK: die Schranke ist kein Zaun um ein Ergebnis, sondern um eine ANNAHME. Sie unterdrueckt
// nichts und faelscht nichts -- sie verbietet dem Uebersetzer nur, eine Zahl zu erfinden, die er
// nicht gemessen hat. Wuerden die Arenen allozieren, zaehlte der Zaehler das MIT Schranke genauso
// wie ohne; ohne Schranke duerfte er es verschweigen.
#if !defined(__GNUC__) && !defined(__clang__)
inline void const volatile* volatile g_senke = nullptr;
#endif

inline void speicher_schranke() noexcept {
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__("" : : : "memory");
#else
    // SCHWAECHERER RUECKFALL fuer Uebersetzer ohne GNU-asm. Ausdruecklich als schwaecher benannt
    // statt als gleichwertig ausgegeben: er ordnet die volatile-Zugriffe, erklaert aber nicht den
    // uebrigen Speicher fuer veraendert. Traegt er nicht, wird der Test ROT -- die Koeder in (2) und
    // (4) verschwinden dann wieder --, er kann sich nicht in ein stilles Gruen retten.
    (void)g_senke;
#endif
}

// Laesst einen Zeiger in ein dem Uebersetzer unbekanntes Register entkommen. Danach ist die
// zugehoerige Belegung nicht mehr als tot beweisbar -- weder ueber -fallocation-dce (GCC) noch ueber
// die allocation elision von clang.
inline void nicht_wegoptimieren(void const volatile* p) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__("" : : "r"(p) : "memory");
#else
    g_senke = p;
#endif
}

inline void scharf_stellen() noexcept {
    g_neu_zaehler = 0;
    g_neu_bytes   = 0;
    g_scharf      = true;
    speicher_schranke(); // HIER beginnt das Fenster -- nichts darf darueber hinweg verschoben werden
}

[[nodiscard]] inline std::uint64_t entschaerfen() noexcept {
    speicher_schranke(); // ... und HIER endet es: der Stand wird gelesen, nicht geraten
    g_scharf = false;
    return g_neu_zaehler;
}

[[nodiscard]] inline std::uint64_t bytes() noexcept {
    speicher_schranke();
    return g_neu_bytes;
}

inline void* roh_holen(std::size_t n) noexcept {
    if (g_scharf) {
        ++g_neu_zaehler;
        g_neu_bytes += static_cast<std::uint64_t>(n);
    }
    return std::malloc(n != 0u ? n : 1u);
}

inline void* roh_holen_ausgerichtet(std::size_t n, std::size_t a) noexcept {
    if (g_scharf) {
        ++g_neu_zaehler;
        g_neu_bytes += static_cast<std::uint64_t>(n);
    }
    std::size_t const gepolstert = ((n != 0u ? n : 1u) + a - 1u) / a * a;
    return std::aligned_alloc(a, gepolstert);
}

} // namespace ms_wache

void* operator new(std::size_t n) {
    void* p = ms_wache::roh_holen(n);
    if (p == nullptr) throw std::bad_alloc{};
    return p;
}
void* operator new[](std::size_t n) {
    void* p = ms_wache::roh_holen(n);
    if (p == nullptr) throw std::bad_alloc{};
    return p;
}
void* operator new(std::size_t n, std::align_val_t a) {
    void* p = ms_wache::roh_holen_ausgerichtet(n, static_cast<std::size_t>(a));
    if (p == nullptr) throw std::bad_alloc{};
    return p;
}
void* operator new[](std::size_t n, std::align_val_t a) {
    void* p = ms_wache::roh_holen_ausgerichtet(n, static_cast<std::size_t>(a));
    if (p == nullptr) throw std::bad_alloc{};
    return p;
}
void* operator new(std::size_t n, std::nothrow_t const&) noexcept { return ms_wache::roh_holen(n); }
void* operator new[](std::size_t n, std::nothrow_t const&) noexcept { return ms_wache::roh_holen(n); }
void* operator new(std::size_t n, std::align_val_t a, std::nothrow_t const&) noexcept {
    return ms_wache::roh_holen_ausgerichtet(n, static_cast<std::size_t>(a));
}
void* operator new[](std::size_t n, std::align_val_t a, std::nothrow_t const&) noexcept {
    return ms_wache::roh_holen_ausgerichtet(n, static_cast<std::size_t>(a));
}

void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::nothrow_t const&) noexcept { std::free(p); }
void operator delete[](void* p, std::nothrow_t const&) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t, std::nothrow_t const&) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t, std::nothrow_t const&) noexcept { std::free(p); }

namespace {

int g_fail = 0;

void pruefe(bool ok, char const* was) {
    if (!ok) {
        ++g_fail;
        std::cout << "  [ERR] " << was << "\n";
    }
}

namespace mess = ::comdare::cache_engine::measurement;
namespace ms   = ::comdare::cache_engine::builder::measure_storage;

// Owner-KERN: Batch 4096. Der Haus-Anker fuer eine Mess-Gruppe, keine gegriffene Groesse.
inline constexpr std::uint64_t kAppends = 4096;

// Rueckleseprobe (10): dieselbe Haus-Groesse, und die Kapazitaet ist EXAKT so gross. Exakt, damit
// eine umgedrehte Reihenfolge (Schreiben auf kapazitaet-1-platz) eine ECHTE Umkehrung ist und als
// solche benannt werden kann -- bei einer groesseren Arena waere sie nur ein Versatz.
inline constexpr std::uint64_t kRueckleseZeilen = kAppends;
static_assert(kRueckleseZeilen % 2u == 0u,
              "die Umkehr-Diagnose in (10) braucht eine GERADE Zeilenzahl -- sonst faellt die mittlere "
              "Zeile mit sich selbst zusammen und die Umkehrung waere dort nicht unterscheidbar");

// == KOEDER, am 09.08.2026 frisch aus /dev/urandom gewuerfelt (K13: nie aus einer Doku) ============
inline constexpr std::uint64_t kKoederZeit1   = 3207928510ull; // passt in die Arena
inline constexpr std::uint64_t kKoederZeit2   = 2988986077ull; // passt
inline constexpr std::uint64_t kKoederZeit3   = 4129189578ull; // passt -- damit ist sie voll
inline constexpr std::uint64_t kKoederZeit4   = 139788480ull;  // MUSS verloren gehen
inline constexpr std::uint64_t kKoederStapel1 = 3044735384ull;
inline constexpr std::uint64_t kKoederStapel2 = 4182419343ull;
inline constexpr std::uint64_t kKoederStapel3 = 3816673831ull;
inline constexpr std::uint64_t kKoederStapel4 = 1938343354ull;
inline constexpr std::uint64_t kKoederZuTief  = 0xf9285cdf0d550d02ull; // MUSS abgewiesen werden
inline constexpr std::uint64_t kKoederOffen   = 0xa7ea73b0d04f92ecull; // MUSS offen liegenbleiben

// == DIE KOEDER-GROESSE WIRD ZUR LAUFZEIT GEWUERFELT ===============================================
// Der Zweck ist NICHT Statistik, sondern Undurchsichtigkeit. Eine Groesse, die im Quelltext steht,
// kennt der Uebersetzer, und er darf ueber sie rechnen -- die 38763, die hier stand, hat er bei -O3
// genau dafuer benutzt. Eine erst zur Laufzeit gelesene kennt er nicht. Zugleich ist K13 damit im
// strengsten Sinn erfuellt: der Koeder ist bei JEDEM Lauf frisch gewuerfelt und kann gar nicht aus
// einer Doku abgeschrieben sein.
//
// Laeuft AUSSERHALB jedes Fensters -- hier darf belegt werden. Der Rueckfall wird ausgesprochen und
// nicht verschwiegen: eine still eingesetzte Konstante waere genau der Zustand, den diese Funktion
// beseitigen soll.
[[nodiscard]] std::uint64_t wurf_holen(char const*& woher) noexcept {
    std::uint64_t roh = 0;
    woher             = "/dev/urandom";
    if (std::FILE* q = std::fopen("/dev/urandom", "rb"); q != nullptr) {
        if (std::fread(&roh, sizeof(roh), 1u, q) != 1u) roh = 0u;
        (void)std::fclose(q);
    }
    if (roh == 0u) {
        // Kein Rueckfall auf eine Konstante: die Adresse eines Automatikobjekts ist dem Uebersetzer
        // ebenfalls unbekannt (ASLR), taugt also fuer denselben Zweck.
        woher = "Stapeladresse/ASLR (/dev/urandom nicht lesbar)";
        roh   = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(&roh)) * 0x9e3779b97f4a7c15ull;
    }
    return roh;
}

[[nodiscard]] std::size_t koeder_groesse_wuerfeln(char const*& woher) noexcept {
    std::uint64_t const roh = wurf_holen(woher);
    // 4096 .. 65535 Byte: gross genug, dass die Belegung kein Kleinkram ist, klein genug fuer jede
    // Maschine -- und nie 0, damit die Byte-Pruefung unten eine echte Aussage bleibt.
    return static_cast<std::size_t>(4096u + (static_cast<std::uint32_t>(roh) % 61440u));
}

// == DIE ERWARTUNGS-QUELLE FUER DIE RUECKLESUNG (10) ===============================================
// splitmix64 -- die Mischfunktion aus Steele/Lea/Flood, dieselbe, die SplittableRandom und
// std::mt19937-Seeding im Haus schon benutzen. Sie steht hier NICHT fuer Statistik, sondern fuer
// UNDURCHSICHTIGKEIT und K13-Haerte: aus EINEM je Lauf frisch gewuerfelten Wurf entsteht fuer jede
// Zeile und jedes Feld ein eigener Wert. Damit ist keine einzige Erwartung im Quelltext
// abschreibbar, und der Uebersetzer kann keine davon vorausberechnen.
[[nodiscard]] constexpr std::uint64_t mischen(std::uint64_t x) noexcept {
    x += 0x9e3779b97f4a7c15ull;
    x = (x ^ (x >> 30u)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27u)) * 0x94d049bb133111ebull;
    return x ^ (x >> 31u);
}

/// Die Zeile mit dem Index i, abgeleitet aus dem Wurf des Laufs.
///
/// JEDES Feld haengt an einem EIGENEN Ableitungsweg -- deshalb faellt eine Vertauschung zweier
/// Felder auf. Und weil jeder Weg zusaetzlich am INDEX haengt, faellt ebenso eine falsche Position
/// und eine verdrehte Reihenfolge auf. Beides ist der Zweck; ein Wert, der fuer alle Zeilen gleich
/// waere, machte die Rueckleseprobe zu einer Prosa-Uebung.
///
/// Die Reserve-Felder bleiben 0 -- das ist die zugesagte Belegung (mess_arena.hpp), und die
/// Rueckleseprobe prueft sie mit: die Arena darf dort auch nichts ERFINDEN.
[[nodiscard]] ms::MessCheckpointZeile zeile_bauen(std::uint64_t wurf, std::uint64_t i) noexcept {
    ms::MessCheckpointZeile z{};
    z.zeit_ticks    = mischen(wurf ^ (4u * i + 0u));
    z.messwert      = mischen(wurf ^ (4u * i + 1u));
    z.deskriptor_ix = static_cast<std::uint32_t>(mischen(wurf ^ (4u * i + 2u)));
    z.thread_nr     = static_cast<std::uint16_t>(mischen(wurf ^ (4u * i + 3u)) >> 23u);
    // Auch das Tag-Byte traegt Positionsinformation: die Ebene wandert mit dem Index durch alle drei
    // benutzten Werte, die Richtung wechselt je Zeile.
    z.tag = ms::tag_bauen(static_cast<ms::MessEbene>(i % 3u), (i % 2u) == 0u ? ms::Richtung::Ein : ms::Richtung::Aus);
    return z;
}

/// Die erste Zahl aus /proc/self/statm: die GROESSE DES ADRESSRAUMS in Seiten (VmSize).
///
/// Das ist die einzige Observable, an der sich eine noch abgebildete Reservierung zeigen laesst --
/// die Arena gibt ihren Block nach aussen nicht preis, und ohne diese Messung waere "die Abbildung
/// ist wirklich weg" eine reine Behauptung. Rueckgabe 0 = nicht messbar (kein procfs); der Aufrufer
/// sagt das dann ausdruecklich, statt still durchzulaufen.
[[nodiscard]] std::uint64_t vm_seiten() noexcept {
    unsigned long long v = 0;
    if (std::FILE* q = std::fopen("/proc/self/statm", "rb"); q != nullptr) {
        if (std::fscanf(q, "%llu", &v) != 1) v = 0;
        (void)std::fclose(q);
    }
    return static_cast<std::uint64_t>(v);
}

// Liest eine Datei ganz ein. Laeuft ausserhalb jedes Fensters -- hier darf belegt werden.
[[nodiscard]] bool datei_lesen(char const* pfad, std::string& hinein) {
    std::ifstream f(pfad, std::ios::binary);
    if (!f) return false;
    hinein.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}

} // namespace

int main() {
    std::cout << "== MS-1: der Aufnahme-Pfad alloziert im Fenster nicht ==\n";

    // -- (1) KONTRAST: der Bestands-Pfad ------------------------------------------------------------
    // Die Thread-Arena wird VOR dem Fenster angelegt (ein Append ausserhalb), damit das Fenster
    // ausschliesslich den Anhaenge-Pfad misst und nicht den einmaligen std::map-Knoten. Gemessen wird
    // also der GUENSTIGSTE Fall des Bestands, nicht der schlechteste -- und er faellt trotzdem.
    {
        mess::InMemoryMeasurementBuffer puffer{};
        mess::MeasurementRecord         satz{};
        satz.timestamp_ns = 1;
        puffer.append_record(satz);

        ms_wache::scharf_stellen();
        for (std::uint64_t i = 0; i < kAppends; ++i) {
            satz.timestamp_ns = i;
            puffer.append_record(satz);
        }
        std::uint64_t const zahl  = ms_wache::entschaerfen();
        std::uint64_t const bytes = ms_wache::bytes();

        std::cout << "-- (1) Kontrast: Bestands-Pfad InMemoryMeasurementBuffer::append_record --\n";
        std::cout << "     Appends im Fenster = " << kAppends << ", Allokationen = " << zahl
                  << ", angeforderte Bytes = " << bytes << "\n";
        pruefe(zahl > 0, "Kontrast: der Bestands-Pfad muss allozieren (sonst waere der Zaehler blind)");
    }

    // -- (2) GEGENPROBE: sieht der Zaehler ueberhaupt etwas? -----------------------------------------
    // Ein Zaehler, der nie zaehlt, ist kein Zaehler. Genau drei angeforderte Blocks, drei erwartet.
    {
        // Die drei Bloecke werden auch BESCHRIEBEN UND GELESEN. Nicht aus Ordnungsliebe: ein
        // Uebersetzer darf eine Belegung, deren Ergebnis nie benutzt wird, ersatzlos entfernen
        // (C++ erlaubt das ausdruecklich fuer new/delete-Paare) -- dann zaehlte der Zaehler nichts
        // und die Gegenprobe waere selbst der wertlose Test, den sie ausschliessen soll.
        //
        // BENUTZEN GENUEGT NICHT -- am Objekt gelernt. Genau das stand hier schon, und bei -O3 hat
        // GCC trotzdem zwei der drei Paare ueber -fallocation-dce entfernt und die Nutzsumme 48 aus
        // den Konstanten ausgerechnet, statt sie aus dem Speicher zu lesen: "gezaehlt = 1,
        // Nutzsumme = 48". Erst die Schranke laesst die Zeiger entkommen; danach ist keine der drei
        // Belegungen mehr als tot beweisbar. Die drei Formen sind mit Absicht verschieden --
        // Skalar, Feld, expliziter ::operator new --, weil sie verschieden weit wegoptimierbar sind.
        ms_wache::scharf_stellen();
        int* a = new int(7);
        ms_wache::nicht_wegoptimieren(a);
        int* b = new int[16];
        ms_wache::nicht_wegoptimieren(b);
        int* c = static_cast<int*>(::operator new(64));
        ms_wache::nicht_wegoptimieren(c);
        b[0]  = 11;
        b[15] = 13;
        c[0]  = 17;
        ms_wache::nicht_wegoptimieren(b);
        ms_wache::nicht_wegoptimieren(c);
        std::uint64_t const zahl  = ms_wache::entschaerfen();
        int const           summe = *a + b[0] + b[15] + c[0];
        delete a;
        delete[] b;
        ::operator delete(c);

        std::cout << "-- (2) Gegenprobe: der Zaehler sieht drei absichtliche Allokationen --\n";
        std::cout << "     erwartet = 3, gezaehlt = " << zahl << ", Nutzsumme = " << summe << "\n";
        pruefe(zahl == 3, "Gegenprobe: der Zaehler zaehlt");
        pruefe(summe == 48, "Gegenprobe: die Bloecke wurden wirklich benutzt (7+11+13+17)");
    }

    // -- (3) DIE ZUSAGE: die beiden Arenen im Fenster ------------------------------------------------
    // Aufbau AUSSERHALB (dort ist Belegung erlaubt), dann das Fenster: je Operation ein EIN und ein
    // AUS -- Stapel hinauf, Zeile an, Spitze lesen, Zeile an, Stapel herunter. Das ist der volle
    // Verkehr beider Arenen, nicht nur ein Anhaengen.
    {
        ms::CheckpointSpeicher sp;
        ms::AufbauBefund const steht = ms::speicher_aufbauen(sp, 4u * kAppends, 64u, ms::VorabBeruehrung::Ja);
        pruefe(steht.steht(), "Aufbau: beide Arenen reserviert");

        ms::MessCheckpointZeile ein{};
        ein.tag = ms::tag_bauen(ms::MessEbene::Micro, ms::Richtung::Ein);
        ms::MessCheckpointZeile aus{};
        aus.tag = ms::tag_bauen(ms::MessEbene::Micro, ms::Richtung::Aus);

        std::uint64_t summe    = 0;
        std::uint64_t schlecht = 0;

        ms_wache::scharf_stellen();
        for (std::uint64_t i = 0; i < kAppends; ++i) {
            ein.zeit_ticks           = i;
            std::uint64_t const slot = sp.mess.anhaengen(ein);
            if (slot == ms::MessArena::kUeberlaufSlot) ++schlecht;

            ms::StapelEintrag e{};
            e.log_index     = slot;
            e.deskriptor_ix = static_cast<std::uint32_t>(i & 0xFFFFu);
            e.ebene         = static_cast<std::uint8_t>(ms::MessEbene::Micro);
            if (!sp.stapel.hinauf(e)) ++schlecht;

            ms::StapelEintrag const* oben = sp.stapel.spitze();
            if (oben != nullptr) summe += oben->log_index;

            aus.zeit_ticks = i;
            aus.messwert   = summe;
            if (sp.mess.anhaengen(aus) == ms::MessArena::kUeberlaufSlot) ++schlecht;
            if (!sp.stapel.herunter(nullptr)) ++schlecht;
        }
        std::uint64_t const zahl = ms_wache::entschaerfen();

        std::cout << "-- (3) DIE ZUSAGE: beide Arenen im Fenster --\n";
        std::cout << "     Anhaengen = " << (2u * kAppends) << ", Hinauf/Herunter = " << kAppends
                  << ", Spitze gelesen = " << kAppends << ", Allokationen = " << zahl << "\n";
        std::cout << "     Mess-Arena belegt = " << sp.mess.belegt() << " von " << sp.mess.kapazitaet()
                  << ", Stapel-Hoechststand = " << sp.stapel.hoechststand() << "\n";
        pruefe(zahl == 0, "DIE ZUSAGE: 0 Allokationen im Mess-Fenster");
        pruefe(schlecht == 0, "kein Ueberlauf im regulaeren Lauf");
        pruefe(sp.mess.belegt() == 2u * kAppends, "beide Zeilen je Operation liegen in der Arena");
        pruefe(sp.stapel.hoechststand() == 1, "der Stapel bleibt bei Tiefe 1 (kein Leck)");
        pruefe(sp.stapel.tiefe() == 0, "am Ende ist der Stapel leer (jedes EIN hat sein AUS)");
        pruefe(summe > 0, "die Spitze wurde wirklich gelesen (sonst misst (3) nichts)");
    }

    // -- (4) GEGENPROBE K13: eine absichtliche Allokation IM GEMESSENEN BEREICH ----------------------
    // Derselbe Arenen-Verkehr wie in (3), aber mit einem Koeder mitten drin. Der Zaehler MUSS ihn
    // sehen -- sonst beweist (3) nichts, weil dann jeder Pfad "0" ergaebe.
    // ZWEI unabhaengige Observablen: die ANZAHL und die BYTE-Zahl. Ein Mutant, der die Zaehlung
    // stillegte, faellt an beiden; einer, der nur die Byte-Summe verlore, faellt an der zweiten.
    //
    // DIESER ABSCHNITT TRAEGT (3). Ohne ihn waere die dort gedruckte 0 bei -O1 und hoeher nicht von
    // einer wegoptimierten 0 zu unterscheiden -- genau das war der CI-Defekt. Er misst deshalb
    // denselben Arenen-Verkehr, nur mit einem Koeder mitten drin, und ist dreifach gegen den
    // Uebersetzer gesichert: Groesse erst zur Laufzeit gewuerfelt, Zeiger durch die Schranke,
    // Zaehlerstand hinter der Schranke im Fensterrahmen gelesen.
    {
        char const*       woher        = nullptr;
        std::size_t const koeder_bytes = koeder_groesse_wuerfeln(woher); // VOR dem Fenster gewuerfelt

        ms::CheckpointSpeicher sp;
        pruefe(ms::speicher_aufbauen(sp, 1024u, 64u, ms::VorabBeruehrung::Ja).steht(), "Aufbau (4)");

        ms::MessCheckpointZeile z{};
        z.tag = ms::tag_bauen(ms::MessEbene::Macro, ms::Richtung::Ein);

        ms_wache::scharf_stellen();
        (void)sp.mess.anhaengen(z);
        char* koeder = new char[koeder_bytes]; // DER KOEDER -- absichtlich, im Fenster
        ms_wache::nicht_wegoptimieren(koeder);
        koeder[0]                 = 1;
        koeder[koeder_bytes - 1u] = 2;
        ms_wache::nicht_wegoptimieren(koeder);
        (void)sp.mess.anhaengen(z);
        std::uint64_t const zahl  = ms_wache::entschaerfen();
        std::uint64_t const bytes = ms_wache::bytes();
        delete[] koeder;

        std::cout << "-- (4) Gegenprobe K13: eine absichtliche Allokation im gemessenen Bereich --\n";
        std::cout << "     Koeder-Groesse = " << koeder_bytes << " (gewuerfelt aus " << woher
                  << "), gezaehlt = " << zahl << ", Bytes = " << bytes << "\n";
        pruefe(zahl == 1, "K13: der Zaehler sieht den Koeder im Arenen-Fenster");
        pruefe(bytes == koeder_bytes, "K13: die Byte-Zahl ist exakt die Koeder-Groesse");
    }

    // -- (5) KOEDER Mess-Arena: Ueberlauf ist DATENVERLUST -------------------------------------------
    // Kapazitaet 3, vier Zeilen angeboten. VIER unabhaengige Observablen, damit kein Mutant von einer
    // anderen Zeile verdeckt stirbt:
    //   (a) die Rueckgabe der vierten ist kUeberlaufSlot     -> faengt "Kapazitaets-Pruefung entfernt"
    //   (b) verloren == 1 und belegt == 3                    -> faengt "Verlust-Zaehlung entfernt"
    //   (c) der Platz HINTER der Kapazitaet ist unberuehrt   -> faengt "ueber die Kante geschrieben"
    //   (d) der Koeder-Wert steht in keiner ausgelesenen Zeile
    // Beobachtbarkeit geprueft: (a) und (c) unterscheiden sich zwischen MIT und OHNE Kapazitaets-
    // Pruefung; (b) unterscheidet sich zwischen MIT und OHNE weiterlaufendem Versuchs-Zaehler. Keine
    // der vier faengt den Mutanten der anderen, sie liegen auf verschiedenen Observablen.
    {
        ms::MessArena arena;
        pruefe(arena.reservieren(3u, ms::VorabBeruehrung::Ja), "Aufbau (5): Kapazitaet 3");

        ms::MessCheckpointZeile z{};
        z.zeit_ticks           = kKoederZeit1;
        std::uint64_t const s1 = arena.anhaengen(z);
        z.zeit_ticks           = kKoederZeit2;
        std::uint64_t const s2 = arena.anhaengen(z);
        z.zeit_ticks           = kKoederZeit3;
        std::uint64_t const s3 = arena.anhaengen(z);
        z.zeit_ticks           = kKoederZeit4; // der Koeder
        std::uint64_t const s4 = arena.anhaengen(z);

        ms::AusleseErgebnis const erg             = arena.auslesen();
        bool                      koeder_gefunden = false;
        for (auto const& zeile : erg.zeilen) {
            if (zeile.zeit_ticks == kKoederZeit4) koeder_gefunden = true;
        }

        auto const* roh   = static_cast<ms::MessCheckpointZeile const*>(arena.nutzlast_basis());
        bool const  kante = (roh != nullptr) && (roh[3].zeit_ticks == 0u);

        char text[256];
        (void)ms::befund_zeile(erg.befund, text, sizeof(text));

        std::cout << "-- (5) Koeder Mess-Arena: Ueberlauf ist DATENVERLUST --\n";
        std::cout << "     Slots = " << s1 << "," << s2 << "," << s3 << ","
                  << (s4 == ms::MessArena::kUeberlaufSlot ? std::uint64_t{9999} : s4)
                  << " (9999 == Ueberlauf-Kennung)\n";
        std::cout << "     " << text << "\n";
        pruefe(s1 == 0 && s2 == 1 && s3 == 2, "(5a) die ersten drei liegen auf 0,1,2");
        pruefe(s4 == ms::MessArena::kUeberlaufSlot, "(5a) die vierte meldet Ueberlauf");
        pruefe(erg.befund.verloren == 1 && erg.befund.belegt == 3, "(5b) verloren = 1 von 4, belegt = 3");
        pruefe(kante, "(5c) hinter der Kapazitaet wurde nicht geschrieben");
        pruefe(!koeder_gefunden, "(5d) der Koeder-Wert steht in keiner ausgelesenen Zeile");
        pruefe(std::string_view{text}.find("DATENVERLUST") != std::string_view::npos,
               "(5e) die Meldung nennt die Fehlerklasse Datenverlust");
        // RECHTE GRENZE, ergaenzt 09.08.2026: die Suche stand vorher auf "von 4" und war damit auch
        // von "von 40" oder "von 4096" erfuellt -- eine Nenner-Pruefung, die jeden groesseren Nenner
        // durchliess. Gesucht wird jetzt die ganze Wendung samt folgendem Wort.
        pruefe(std::string_view{text}.find("1 von 4 angebotenen Zeilen") != std::string_view::npos,
               "(5f) die Meldung nennt Zahl UND Nenner vollstaendig, nicht nur eine fuehrende Ziffer");
    }

    // -- (6) KOEDER Stapel-Arena: Ueberlauf ist ein PROGRAMMIERFEHLER, kein Datenverlust -------------
    // Dieselbe Lage, andere Arena -- und die Meldung MUSS eine andere sein. Das ist die
    // Fehlerklassen-Trennung, gebaut statt versprochen: die beiden Texte duerfen sich nicht teilen.
    {
        ms::StapelArena stapel;
        pruefe(stapel.reservieren(4u, ms::VorabBeruehrung::Ja), "Aufbau (6): Tiefe 4");

        ms::StapelEintrag e{};
        e.log_index   = kKoederStapel1;
        bool const h1 = stapel.hinauf(e);
        e.log_index   = kKoederStapel2;
        bool const h2 = stapel.hinauf(e);
        e.log_index   = kKoederStapel3;
        bool const h3 = stapel.hinauf(e);
        e.log_index   = kKoederStapel4;
        bool const h4 = stapel.hinauf(e);
        e.log_index   = kKoederZuTief; // der Koeder -- eine Ebene zu tief
        bool const h5 = stapel.hinauf(e);

        bool koeder_gefunden = false;
        for (auto const& offen : stapel.offene()) {
            if (offen.log_index == kKoederZuTief) koeder_gefunden = true;
        }

        // unpaariges AUS: fuenfmal herunter bei vier offenen -> eigene Fehlerklasse, eigener Zaehler
        for (int i = 0; i < 5; ++i) (void)stapel.herunter(nullptr);

        ms::StapelBefund const b = stapel.befund();
        char                   text[320];
        (void)ms::befund_zeile(b, text, sizeof(text));

        std::cout << "-- (6) Koeder Stapel-Arena: Ueberlauf ist ein PROGRAMMIERFEHLER --\n";
        std::cout << "     " << text << "\n";
        pruefe(h1 && h2 && h3 && h4, "(6a) vier Ebenen passen");
        pruefe(!h5, "(6b) die fuenfte wird abgewiesen");
        pruefe(b.zu_tief == 1, "(6c) zu_tief = 1");
        pruefe(!koeder_gefunden, "(6d) der abgewiesene Koeder liegt nicht auf dem Stapel");
        pruefe(b.hoechststand == 4, "(6e) der Hoechststand ist 4, nicht 5");
        pruefe(b.unpaarige_aus == 1, "(6f) ein unpaariges AUS, eigener Zaehler");
        pruefe(stapel.tiefe() == 0, "(6g) der Top-Zaehler laeuft nicht unter Null");
        pruefe(std::string_view{text}.find("PROGRAMMIERFEHLER") != std::string_view::npos,
               "(6h) die Meldung nennt die Fehlerklasse Programmierfehler");
        pruefe(std::string_view{text}.find("DATENVERLUST") == std::string_view::npos,
               "(6i) die Stapel-Meldung nennt NIEMALS Datenverlust -- getrennte Fehlerklassen");
    }

    // -- (7) IN/OUT-SYMMETRIE: ein EIN ohne AUS bleibt liegen und wird gemeldet ----------------------
    // Drei hinauf, zwei herunter. Das dritte EIN ist eine REGRESSION -- es muss beim Auslesen noch da
    // sein, mit seinem Koeder-Wert, und in der Meldung auftauchen. Wuerde der Stapel sich am Ende
    // selbst leeren, waere genau die Regression vernichtet, die er anzeigen sollte.
    {
        ms::StapelArena stapel;
        pruefe(stapel.reservieren(8u, ms::VorabBeruehrung::Ja), "Aufbau (7): Tiefe 8");

        ms::StapelEintrag e{};
        e.log_index     = kKoederOffen; // DAS EIN, das offen bleibt
        e.deskriptor_ix = 0x0d55u;
        e.ebene         = static_cast<std::uint8_t>(ms::MessEbene::Macro);
        (void)stapel.hinauf(e);
        e.log_index = 11;
        e.ebene     = static_cast<std::uint8_t>(ms::MessEbene::Micro);
        (void)stapel.hinauf(e);
        e.log_index = 22;
        (void)stapel.hinauf(e);
        (void)stapel.herunter(nullptr);
        (void)stapel.herunter(nullptr);

        auto const             offen = stapel.offene();
        ms::StapelBefund const b     = stapel.befund();
        char                   text[320];
        (void)ms::befund_zeile(b, text, sizeof(text));

        std::cout << "-- (7) IN/OUT-Symmetrie: ein EIN ohne AUS bleibt liegen --\n";
        std::cout << "     offen = " << offen.size() << ", log_index der Leiche = 0x" << std::hex
                  << (offen.empty() ? 0ull : offen[0].log_index) << std::dec << "\n";
        std::cout << "     " << text << "\n";
        pruefe(offen.size() == 1, "(7a) genau ein EIN blieb offen");
        pruefe(!offen.empty() && offen[0].log_index == kKoederOffen, "(7b) es ist das Koeder-EIN, nicht irgendeines");
        pruefe(!offen.empty() && offen[0].ebene == static_cast<std::uint8_t>(ms::MessEbene::Macro),
               "(7c) seine Mess-Ebene reist mit -- die Meldung kann sagen, WELCHES offen blieb");
        pruefe(b.offen == 1 && !b.sauber(), "(7d) der Befund nennt es und gilt als nicht sauber");
        pruefe(std::string_view{text}.find("offen_geblieben = 1") != std::string_view::npos,
               "(7e) die Meldung nennt die offene Klammer ausdruecklich");
    }

    // -- (8) DIE HAUSKONSTANTE gegen die Maschine, und die Trennung am Objekt ------------------------
    {
        std::cout << "-- (8) Hauskonstante und Trennung --\n";
        std::string roh;
        bool const  gelesen = datei_lesen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", roh);
        if (gelesen) {
            long const gemessen = std::strtol(roh.c_str(), nullptr, 10);
            std::cout << "     sysfs coherency_line_size = " << gemessen << ", Hauskonstante = " << ms::kCacheLineBytes
                      << "\n";
            pruefe(static_cast<std::size_t>(gemessen) == ms::kCacheLineBytes,
                   "(8a) die Hauskonstante stimmt mit der Maschine ueberein");
        } else {
            // KEIN stiller Durchlauf: die Nichtpruefbarkeit wird ausgesprochen.
            std::cout << "     sysfs nicht lesbar -- die Hauskonstante ist auf dieser Maschine UNGEPRUEFT\n";
        }

        ms::CheckpointSpeicher sp;
        pruefe(ms::speicher_aufbauen(sp, 256u, 16u, ms::VorabBeruehrung::Ja).steht(), "Aufbau (8)");

        // DIE OFFSET-0-ZUSAGE, NACHGEMESSEN (ergaenzt 09.08.2026). checkpoint_speicher.hpp begruendet
        // die Cacheline-Trennung damit, dass der heisse Zustand auf Offset 0 seiner Arena beginnt --
        // und genau dafuer gab es bis heute keinen einzigen Beleg, weder einen offsetof-Assert (das
        // Mitglied ist privat) noch eine Messung. Der Abstands-Assert dort misst die Objekte, nicht
        // die heissen Zustaende; ohne diese zwei Zeilen war die Kette zwischen beidem unbewacht.
        pruefe(static_cast<void const*>(&sp.mess) == sp.mess.heisse_basis(),
               "(8e) der heisse Zustand der Mess-Arena beginnt auf Offset 0 ihres Objekts");
        pruefe(static_cast<void const*>(&sp.stapel) == sp.stapel.heisse_basis(),
               "(8f) der heisse Zustand der Stapel-Arena beginnt auf Offset 0 ihres Objekts");

        auto const linie_mess   = reinterpret_cast<std::uintptr_t>(sp.mess.heisse_basis()) / ms::kCacheLineBytes;
        auto const linie_stapel = reinterpret_cast<std::uintptr_t>(sp.stapel.heisse_basis()) / ms::kCacheLineBytes;
        auto const seite_mess   = reinterpret_cast<std::uintptr_t>(sp.mess.nutzlast_basis()) / ms::kSeitenBytes;
        auto const seite_stapel = reinterpret_cast<std::uintptr_t>(sp.stapel.nutzlast_basis()) / ms::kSeitenBytes;

        std::cout << "     heisse Cacheline-Nummern: mess = " << linie_mess << ", stapel = " << linie_stapel
                  << " (Abstand " << (linie_stapel - linie_mess) << " Linien)\n";
        std::cout << "     Nutzlast-Seiten: mess = " << seite_mess << ", stapel = " << seite_stapel << "\n";
        pruefe(linie_mess != linie_stapel, "(8b) die heissen Zustaende liegen in verschiedenen Cachelines");
        pruefe(seite_mess != seite_stapel, "(8c) die Nutzlasten liegen auf verschiedenen Seiten");
        pruefe(sizeof(ms::MessCheckpointZeile) == 32 && sizeof(ms::StapelEintrag) == 16,
               "(8d) die POD-Groessen sind 32 und 16 Byte");
    }

    // -- (9) TEXT-WACHE auf den heissen Bloecken ----------------------------------------------------
    // Sie schliesst die Luecke, die eine operator-new-Ersetzung strukturell offenlaesst: eine
    // C-Belegung waere fuer den Zaehler unsichtbar. ZWEI Zusicherungen, damit kein Mutant still
    // durchkommt: die Marker MUESSEN gefunden werden (sonst ist eine leere Wache eine gruene Wache),
    // UND der eingeklammerte Block muss frei von den verbotenen Woertern sein. Zusaetzlich muss der
    // Block die geprueften Funktionsnamen enthalten -- wer die Funktion aus der Klammer heraustraegt,
    // bricht die Wache statt sie zu umgehen.
    {
        std::cout << "-- (9) Text-Wache auf den heissen Bloecken --\n";
        char const* quellen[2] = {COMDARE_MS_QUELLE_MESS, COMDARE_MS_QUELLE_STAPEL};
        char const* anker[2]   = {"anhaengen", "hinauf"};
        char const* verboten[] = {"new",       "malloc",      "calloc",     "realloc",   "std::vector",
                                  "push_back", "std::string", "std::stack", "std::deque"};

        for (int q = 0; q < 2; ++q) {
            std::string inhalt;
            if (!datei_lesen(quellen[q], inhalt)) {
                pruefe(false, "(9) heisse Quelle lesbar");
                continue;
            }
            std::size_t const a         = inhalt.find("[MS-HEISS]");
            std::size_t const e         = inhalt.rfind("[MS-HEISS-ENDE]");
            bool const        marker_da = (a != std::string::npos) && (e != std::string::npos) && (e > a);
            pruefe(marker_da, "(9a) beide Marker stehen in der heissen Quelle");
            if (!marker_da) continue;

            std::string const block = inhalt.substr(a, e - a);
            pruefe(block.find(anker[q]) != std::string::npos,
                   "(9b) die gepruefte Funktion steht INNERHALB der Klammer");

            int treffer = 0;
            for (char const* wort : verboten) {
                if (block.find(wort) != std::string::npos) {
                    ++treffer;
                    std::cout << "     [FUND] verbotenes Wort im heissen Block: " << wort << "\n";
                }
            }
            std::cout << "     " << quellen[q] << ": Block = " << block.size()
                      << " Byte, verbotene Woerter = " << treffer << "\n";
            pruefe(treffer == 0, "(9c) der heisse Block nennt keine Belegung und keinen STL-Behaelter");
        }

        // GEGENPROBE zur Wache selbst: sie muss auf einem Text, der die Woerter enthaelt, anschlagen.
        // Ohne diese Zeile waere nicht gezeigt, dass die Suche ueberhaupt etwas finden KANN.
        std::string const probe    = "auto* p = new int; void* q = malloc(8); std::vector<int> v; v.push_back(1);";
        int               gefunden = 0;
        for (char const* wort : verboten) {
            if (probe.find(wort) != std::string::npos) ++gefunden;
        }
        std::cout << "     Gegenprobe der Wache: " << gefunden << " von 9 verbotenen Woertern im Probetext\n";
        pruefe(gefunden >= 4, "(9d) die Wache findet die Woerter, wenn sie da sind");
    }

    // -- (10) DIE POSITIVE RUECKLESUNG: die Arena gibt zurueck, was hineingeschrieben wurde ----------
    // DAS IST DIE ZUSAGE, UM DIE ES BEIM GANZEN MODUL GEHT, und bis zum 09.08.2026 war sie
    // UNBEWACHT. Alle uebrigen Abschnitte pruefen die Arena NEGATIV oder INDIREKT: (5d) und (5c)
    // fordern, dass etwas NICHT dasteht; (3) leitet `belegt` aus dem Versuchs-Zaehler ab und ruehrt
    // die Nutzlast nie an; (7) liest den STAPEL zurueck, nicht die Mess-Arena. Gemessener Befund:
    // ein Mutant, der in anhaengen() statt der Zeile ein leeres MessCheckpointZeile{} ablegt, lief
    // durch alle neun Abschnitte und meldete "OK: MS-1", RC=0. Die Arena haette nur Nullen schreiben
    // koennen und der Test waere gruen geblieben -- richtiges Messgeraet, falscher Gegenstand.
    //
    // VIER MUTANTEN MUESSEN HIER STERBEN, und jeder hat seinen eigenen Eingang:
    //   (a) leere Zeile geschrieben        -> die Feld-Zaehler (10c)..(10g) und die Byte-Gleichheit
    //   (b) Zeile an falscher Position     -> dieselben Zaehler, weil jede Erwartung am Index haengt
    //   (c) ein Feld vertauscht            -> die BEIDEN betroffenen Feld-Zaehler, benannt und einzeln
    //   (d) Reihenfolge verdreht           -> zusaetzlich (10j), das die Umkehrung ausdruecklich nennt
    //
    // WARUM DIE ERWARTUNG NICHT IM QUELLTEXT STEHT (K13): jede Zeile wird aus EINEM je Lauf frisch
    // gewuerfelten Wurf abgeleitet (zeile_bauen). Der Vergleich rechnet dieselbe Ableitung beim
    // Lesen NEU -- er liest keine gespeicherte Kopie. Eine Erwartung, die aus demselben Puffer
    // stammte, in den geschrieben wurde, verglichen die Arena mit sich selbst.
    //
    // DIE NUMMER SAGT NICHTS UEBER DAS GEWICHT. Dieser Abschnitt ist die Kernzusage des Moduls und
    // steht nur deshalb hinten, weil die Nummern (5)..(9) bereits als Mutanten-Beleg in Commit
    // f24c6ea9 protokolliert sind -- ein Umnummerieren entwertete diesen Beleg.
    {
        char const*         woher = nullptr;
        std::uint64_t const wurf  = wurf_holen(woher);

        ms::MessArena arena;
        pruefe(arena.reservieren(kRueckleseZeilen, ms::VorabBeruehrung::Ja), "Aufbau (10)");

        std::uint64_t falsch_slot = 0;
        for (std::uint64_t i = 0; i < kRueckleseZeilen; ++i) {
            ms::MessCheckpointZeile const z = zeile_bauen(wurf, i);
            if (arena.anhaengen(z) != i) ++falsch_slot;
        }

        ms::AusleseErgebnis const erg = arena.auslesen();

        std::uint64_t falsch_zeit    = 0;
        std::uint64_t falsch_wert    = 0;
        std::uint64_t falsch_desk    = 0;
        std::uint64_t falsch_thread  = 0;
        std::uint64_t falsch_tag     = 0;
        std::uint64_t falsch_reserve = 0;
        std::uint64_t falsch_bytes   = 0;
        std::uint64_t umkehr_treffer = 0;

        std::size_t const kKeine = ~static_cast<std::size_t>(0);
        std::size_t       erste  = kKeine;
        std::size_t const n      = erg.zeilen.size();
        for (std::size_t i = 0; i < n; ++i) {
            ms::MessCheckpointZeile const e = zeile_bauen(wurf, static_cast<std::uint64_t>(i));
            ms::MessCheckpointZeile const g = erg.zeilen[i];

            bool zeile_ok = true;
            if (g.zeit_ticks != e.zeit_ticks) {
                ++falsch_zeit;
                zeile_ok = false;
            }
            if (g.messwert != e.messwert) {
                ++falsch_wert;
                zeile_ok = false;
            }
            if (g.deskriptor_ix != e.deskriptor_ix) {
                ++falsch_desk;
                zeile_ok = false;
            }
            if (g.thread_nr != e.thread_nr) {
                ++falsch_thread;
                zeile_ok = false;
            }
            if (g.tag != e.tag) {
                ++falsch_tag;
                zeile_ok = false;
            }
            if (g.reserviert_64 != 0u || g.reserviert_8 != 0u) {
                ++falsch_reserve;
                zeile_ok = false;
            }
            // Die Byte-Gleichheit ist die SCHAERFSTE der Beobachtungen: sie deckt auch das ab, was
            // kein benanntes Feld traegt. Sie ist wohldefiniert, weil sizeof == 32 bei 8+8+8+4+2+1+1
            // Byte Mitgliedern kein Polster laesst -- der static_assert in mess_arena.hpp haelt das.
            if (std::memcmp(&g, &e, sizeof(ms::MessCheckpointZeile)) != 0) {
                ++falsch_bytes;
                zeile_ok = false;
            }
            if (!zeile_ok && erste == kKeine) erste = i;

            // UMKEHR-DIAGNOSE: passt die gelesene Zeile zur Erwartung des GESPIEGELTEN Index, dann
            // ist nicht der Inhalt falsch, sondern die REIHENFOLGE. Das wird ausgesprochen, statt in
            // einem pauschalen "irgendetwas stimmt nicht" unterzugehen.
            ms::MessCheckpointZeile const s = zeile_bauen(wurf, static_cast<std::uint64_t>(n - 1u - i));
            if (n > 1u && g.zeit_ticks == s.zeit_ticks) ++umkehr_treffer;
        }

        std::cout << "-- (10) DIE POSITIVE RUECKLESUNG: die Arena gibt zurueck, was hineingeschrieben wurde --\n";
        std::cout << "     Wurf = 0x" << std::hex << wurf << std::dec << " (gewuerfelt aus " << woher
                  << "), geschrieben = " << kRueckleseZeilen << ", zurueckgelesen = " << n << "\n";
        std::cout << "     Abweichungen: zeit_ticks = " << falsch_zeit << ", messwert = " << falsch_wert
                  << ", deskriptor_ix = " << falsch_desk << ", thread_nr = " << falsch_thread
                  << ", tag = " << falsch_tag << ", Reserve = " << falsch_reserve
                  << ", Byte-Gleichheit = " << falsch_bytes << ", Umkehr-Treffer = " << umkehr_treffer << "\n";
        if (erste != kKeine) {
            ms::MessCheckpointZeile const e = zeile_bauen(wurf, static_cast<std::uint64_t>(erste));
            ms::MessCheckpointZeile const g = erg.zeilen[erste];
            std::cout << "     erste abweichende Zeile = " << erste << std::hex << "\n"
                      << "       erwartet: zeit=0x" << e.zeit_ticks << " wert=0x" << e.messwert << " desk=0x"
                      << e.deskriptor_ix << " thread=0x" << e.thread_nr << " tag=0x" << unsigned{e.tag} << "\n"
                      << "       gelesen : zeit=0x" << g.zeit_ticks << " wert=0x" << g.messwert << " desk=0x"
                      << g.deskriptor_ix << " thread=0x" << g.thread_nr << " tag=0x" << unsigned{g.tag} << std::dec
                      << "\n";
        }

        pruefe(falsch_slot == 0, "(10a) anhaengen() gibt fortlaufend 0..n-1 zurueck");
        pruefe(n == kRueckleseZeilen && erg.befund.belegt == kRueckleseZeilen && erg.befund.verloren == 0,
               "(10b) es kommen genau so viele Zeilen zurueck, wie hineingeschrieben wurden, ohne Verlust");
        pruefe(falsch_zeit == 0, "(10c) zeit_ticks kommt Zeile fuer Zeile unveraendert und an seinem Platz zurueck");
        pruefe(falsch_wert == 0, "(10d) messwert kommt Zeile fuer Zeile unveraendert und an seinem Platz zurueck");
        pruefe(falsch_desk == 0, "(10e) deskriptor_ix kommt unveraendert zurueck");
        pruefe(falsch_thread == 0, "(10f) thread_nr kommt unveraendert zurueck");
        pruefe(falsch_tag == 0, "(10g) das Tag-Byte (Ebene und Richtung) kommt unveraendert zurueck");
        pruefe(falsch_reserve == 0, "(10h) die Reserve-Felder sind 0 -- die Arena erfindet dort nichts");
        pruefe(falsch_bytes == 0, "(10i) alle 32 Byte jeder Zeile sind Byte fuer Byte identisch");
        pruefe(umkehr_treffer == 0, "(10j) die Rueckgabe ist NICHT die umgekehrte Reihenfolge");
    }

    // -- (11) DIE FUENF STAPEL-ZAHLEN SIND EINZELN UNTERSCHEIDBAR -----------------------------------
    // BEFUND, der diesen Abschnitt erzwingt: in (6) sind zu_tief und unpaarige_aus BEIDE 1. Ein
    // Mutant, der die beiden im befund()-Aggregat vertauscht, war deshalb unbeobachtbar -- gemessen,
    // RC=0. Dasselbe gilt fuer die Meldezeile: (6h) und (6i) pruefen nur, WELCHE Woerter vorkommen,
    // nie, WELCHE ZAHL an welcher Stelle steht.
    //
    // Hier stehen deshalb Werte, die sich paarweise unterscheiden: zu_tief = 2, unpaarige_aus = 3,
    // offen = 1. Jede Vertauschung zweier dieser Zahlen -- im Aggregat wie im Text -- faellt auf.
    // kapazitaet und hoechststand sind hier zwangslaeufig gleich (4): zu_tief steigt nur, wenn der
    // Stapel voll ist, und dann IST der Hoechststand die Kapazitaet. Diese beiden trennt Abschnitt
    // (12), wo zu_tief = 0 bleibt und Kapazitaet 8 auf Hoechststand 2 trifft.
    {
        ms::StapelArena stapel;
        pruefe(stapel.reservieren(4u, ms::VorabBeruehrung::Ja), "Aufbau (11): Tiefe 4");

        ms::StapelEintrag e{};
        e.log_index = kKoederStapel1;
        for (int i = 0; i < 4; ++i) (void)stapel.hinauf(e); // fuellt bis 4 -- Hoechststand 4
        for (int i = 0; i < 2; ++i) (void)stapel.hinauf(e); // zwei zu tief
        for (int i = 0; i < 4; ++i) (void)stapel.herunter(nullptr);
        for (int i = 0; i < 3; ++i) (void)stapel.herunter(nullptr); // drei unpaarige AUS
        e.log_index = kKoederOffen;
        (void)stapel.hinauf(e); // eines bleibt offen liegen

        ms::StapelBefund const b = stapel.befund();
        char                   text[320];
        (void)ms::befund_zeile(b, text, sizeof(text));
        std::string_view const sicht{text};

        std::cout << "-- (11) die fuenf Stapel-Zahlen sind einzeln unterscheidbar --\n";
        std::cout << "     " << text << "\n";
        pruefe(b.zu_tief == 2, "(11a) zu_tief = 2");
        pruefe(b.unpaarige_aus == 3, "(11b) unpaarige_aus = 3 -- eine ANDERE Zahl als zu_tief");
        pruefe(b.offen == 1, "(11c) offen = 1");
        pruefe(b.hoechststand == 4, "(11d) hoechststand = 4");
        pruefe(b.kapazitaet == 4, "(11e) kapazitaet = 4");
        pruefe(!b.sauber(), "(11f) der Befund gilt als nicht sauber");
        // Zweiter, unabhaengiger Weg auf dieselben Zahlen: die Zugriffsfunktionen lesen den heissen
        // Zustand direkt. Weicht das Aggregat von ihnen ab, sitzt der Fehler in befund().
        pruefe(stapel.zu_tief() == 2 && stapel.unpaarige_aus() == 3 && stapel.tiefe() == 1 &&
                   stapel.hoechststand() == 4,
               "(11g) die Zugriffsfunktionen nennen dieselben Zahlen wie das Aggregat");
        pruefe(sicht.find("zu_tief = 2 ") != std::string_view::npos, "(11h) die Meldung nennt zu_tief = 2");
        pruefe(sicht.find("unpaariges_aus = 3,") != std::string_view::npos,
               "(11i) die Meldung nennt unpaariges_aus = 3 -- nicht die Zahl von zu_tief");
        pruefe(sicht.find("offen_geblieben = 1 ") != std::string_view::npos,
               "(11j) die Meldung nennt offen_geblieben = 1");
        pruefe(sicht.find("Hoechststand 4)") != std::string_view::npos, "(11k) die Meldung nennt Hoechststand 4");
    }

    // -- (12) JEDER TERM IN sauber() TRAEGT -- einzeln nachgewiesen ---------------------------------
    // BEFUND, der diesen Abschnitt erzwingt: sauber() verundet drei Terme, und zwei davon waren
    // strukturell unbeobachtbar. Streicht man `unpaarige_aus == 0u`, bleibt der ganze Testlauf gruen
    // (gemessen, RC=0) -- in (6) wird sauber() gar nicht abgefragt, und in (7) entscheidet bereits
    // `offen`. Fuer `zu_tief == 0u` gilt genau dasselbe.
    //
    // DREI LOSE, die sich in GENAU EINEM Zaehler unterscheiden. Das erste ist die Positivkontrolle:
    // ohne sie waere ein sauber(), das immer false liefert, von einem richtigen nicht zu
    // unterscheiden -- und die beiden Negativ-Lose waeren wertlos.
    {
        std::cout << "-- (12) jeder Term in sauber() traegt --\n";

        // LOS A -- alles ausgeglichen. sauber() MUSS true sein, und die Meldung ist die andere.
        {
            ms::StapelArena stapel;
            pruefe(stapel.reservieren(8u, ms::VorabBeruehrung::Ja), "Aufbau (12A): Tiefe 8");
            ms::StapelEintrag e{};
            e.log_index = kKoederStapel2;
            (void)stapel.hinauf(e);
            (void)stapel.hinauf(e);
            (void)stapel.herunter(nullptr);
            (void)stapel.herunter(nullptr);

            ms::StapelBefund const b = stapel.befund();
            char                   text[320];
            (void)ms::befund_zeile(b, text, sizeof(text));
            std::cout << "     A: " << text << "\n";
            pruefe(b.offen == 0 && b.zu_tief == 0 && b.unpaarige_aus == 0, "(12a) Los A: alle drei Zaehler sind 0");
            pruefe(b.sauber(), "(12b) Los A: sauber() ist TRUE -- die Positivkontrolle");
            pruefe(std::string_view{text}.find("ausgeglichen") != std::string_view::npos,
                   "(12c) Los A: die Meldung ist die ausgeglichene, nicht die Fehler-Meldung");
        }

        // LOS B -- NUR zu_tief. Kapazitaet 2 voll, ein Hinauf zu viel, danach sauber abgeraeumt.
        {
            ms::StapelArena stapel;
            pruefe(stapel.reservieren(2u, ms::VorabBeruehrung::Ja), "Aufbau (12B): Tiefe 2");
            ms::StapelEintrag e{};
            e.log_index = kKoederZuTief;
            (void)stapel.hinauf(e);
            (void)stapel.hinauf(e);
            (void)stapel.hinauf(e); // zu tief
            (void)stapel.herunter(nullptr);
            (void)stapel.herunter(nullptr);

            ms::StapelBefund const b = stapel.befund();
            char                   text[320];
            (void)ms::befund_zeile(b, text, sizeof(text));
            std::cout << "     B: " << text << "\n";
            pruefe(b.zu_tief == 1 && b.unpaarige_aus == 0 && b.offen == 0,
                   "(12d) Los B: NUR zu_tief steht, die beiden anderen sind 0");
            pruefe(!b.sauber(), "(12e) Los B: zu_tief allein macht den Befund unsauber");
        }

        // LOS C -- NUR unpaarige_aus. Nichts zu tief, nichts offen, ein AUS zu viel.
        // Zugleich trennt dieses Los kapazitaet (8) von hoechststand (2) -- in (11) fallen die
        // beiden zwangslaeufig zusammen.
        {
            ms::StapelArena stapel;
            pruefe(stapel.reservieren(8u, ms::VorabBeruehrung::Ja), "Aufbau (12C): Tiefe 8");
            ms::StapelEintrag e{};
            e.log_index = kKoederStapel3;
            (void)stapel.hinauf(e);
            (void)stapel.hinauf(e);
            (void)stapel.herunter(nullptr);
            (void)stapel.herunter(nullptr);
            (void)stapel.herunter(nullptr); // eines zu viel

            ms::StapelBefund const b = stapel.befund();
            char                   text[320];
            (void)ms::befund_zeile(b, text, sizeof(text));
            std::string_view const sicht{text};
            std::cout << "     C: " << text << "\n";
            pruefe(b.unpaarige_aus == 1 && b.zu_tief == 0 && b.offen == 0,
                   "(12f) Los C: NUR unpaarige_aus steht, die beiden anderen sind 0");
            pruefe(!b.sauber(), "(12g) Los C: unpaarige_aus allein macht den Befund unsauber");
            pruefe(sicht.find("PROGRAMMIERFEHLER") != std::string_view::npos,
                   "(12h) Los C: die Meldung ist die Fehler-Meldung, nicht die ausgeglichene");
            pruefe(sicht.find("unpaariges_aus = 1,") != std::string_view::npos,
                   "(12i) Los C: die Meldung nennt unpaariges_aus = 1");
            pruefe(b.kapazitaet == 8 && b.hoechststand == 2,
                   "(12j) Los C: Kapazitaet 8 und Hoechststand 2 sind hier verschieden");
            pruefe(sicht.find("ueber 8 Plaetze") != std::string_view::npos &&
                       sicht.find("Hoechststand 2)") != std::string_view::npos,
                   "(12k) Los C: die Meldung setzt Kapazitaet und Hoechststand an ihre eigene Stelle");
        }
    }

    // -- (13) DER KAPAZITAETS-UEBERLAUF WIRD ABGEWIESEN, nicht still zugesagt ------------------------
    // BEFUND: 2^59 Zeilen a 32 Byte sind exakt 2^64 Byte. Die Multiplikation in reservieren()
    // wickelte, und reservieren(2^59 + 1) lieferte TRUE mit kapazitaet() == 576460752303423489 --
    // auf einem 4096-Byte-Block. Die Arena sagte 576 Billiarden Zeilen zu und hatte 128; der erste
    // Checkpoint schriebe hinter die Abbildung.
    //
    // Der Waechter muss ZWEI Dinge leisten, und beide werden hier gemessen: den Ueberlauf abweisen
    // UND jeden gueltigen Wert durchlassen. Ein Waechter, der pauschal abweist, waere ebenso falsch
    // und von einem richtigen nur an der Positivkontrolle (13d) zu unterscheiden.
    {
        std::cout << "-- (13) der Kapazitaets-Ueberlauf wird abgewiesen --\n";
        ms::MessArena arena;

        std::uint64_t const kExakt2Hoch64 = 1ull << 59u; // * 32 Byte == 2^64, wickelt auf 0
        bool const          a             = arena.reservieren(kExakt2Hoch64, ms::VorabBeruehrung::Nein);
        std::uint64_t const kap_a         = arena.kapazitaet();
        bool const          basis_a       = arena.nutzlast_basis() == nullptr;

        bool const          b       = arena.reservieren(kExakt2Hoch64 + 1u, ms::VorabBeruehrung::Nein);
        std::uint64_t const kap_b   = arena.kapazitaet();
        bool const          basis_b = arena.nutzlast_basis() == nullptr;

        bool const gut = arena.reservieren(1024u, ms::VorabBeruehrung::Ja); // Positivkontrolle

        std::cout << "     reservieren(2^59)   -> " << (a ? "TRUE" : "false") << ", kapazitaet = " << kap_a << "\n";
        std::cout << "     reservieren(2^59+1) -> " << (b ? "TRUE" : "false") << ", kapazitaet = " << kap_b << "\n";
        std::cout << "     reservieren(1024)   -> " << (gut ? "TRUE" : "false")
                  << ", kapazitaet = " << arena.kapazitaet() << "\n";
        pruefe(!a, "(13a) 2^59 Zeilen (== 2^64 Byte) werden abgewiesen");
        pruefe(!b, "(13b) 2^59+1 Zeilen werden abgewiesen -- der Fall, der frueher TRUE lieferte");
        pruefe(kap_a == 0 && kap_b == 0 && basis_a && basis_b,
               "(13c) nach der Abweisung sagt die Arena KEINE Kapazitaet zu und haelt keine Basis");
        pruefe(gut && arena.kapazitaet() == 1024, "(13d) Positivkontrolle: ein gueltiger Wert kommt weiter durch");

        // DIESELBE FEHLERKLASSE IM FEEDER (T-6, Schwesterpflicht). Die Planer-Formel rechnete in
        // nacktem uint64: gemessen uebersetzte frueher `static_assert(rechnen(2^63+1, 2, 1) == 2)`.
        // Compile-hart UND zur Laufzeit, weil ein constexpr-Ergebnis sonst nur beim Uebersetzen
        // geprueft waere und niemand die Zahl im Lauf zu sehen bekaeme.
        constexpr ms::KapazitaetRechnung kNormal = ms::kapazitaet_zeilen_rechnen(1000u, 6u, 18u);
        constexpr ms::KapazitaetRechnung kKipp   = ms::kapazitaet_zeilen_rechnen((1ull << 63u) + 1u, 2u, 1u);
        constexpr ms::KapazitaetRechnung kSpaet  = ms::kapazitaet_zeilen_rechnen(1ull << 40u, 1ull << 20u, 64u);
        static_assert(kNormal.darstellbar && kNormal.zeilen == 108000u,
                      "die Planer-Formel muss gewoehnliche Zahlen unveraendert liefern");
        static_assert(!kKipp.darstellbar, "der Ueberlauf in der ERSTEN Multiplikation muss auffallen");
        static_assert(!kSpaet.darstellbar, "der Ueberlauf in der ZWEITEN Multiplikation muss ebenso auffallen");

        std::cout << "     Planer-Formel: 1000*6*18 -> darstellbar = " << (kNormal.darstellbar ? 1 : 0)
                  << ", zeilen = " << kNormal.zeilen
                  << " | (2^63+1)*2*1 -> darstellbar = " << (kKipp.darstellbar ? 1 : 0) << ", zeilen = " << kKipp.zeilen
                  << "\n";
        pruefe(kNormal.darstellbar && kNormal.zeilen == 108000u, "(13e) die Planer-Formel rechnet gewoehnlich richtig");
        pruefe(!kKipp.darstellbar && !kSpaet.darstellbar,
               "(13f) beide Multiplikationen der Planer-Formel melden ihren Ueberlauf");
    }

    // -- (14) DIE FEHLERZAEHLER DES STAPELS WICKELN NICHT AUF "AUSGEGLICHEN" -------------------------
    // BEFUND: zu_tief und unpaarige_aus waren uint32, waehrend der Versuchs-Zaehler der Mess-Arena
    // uint64 ist -- ausdruecklich "damit der Verlust exakt ist". Gemessen: nach 2^32 Abweisungen
    // meldete zu_tief() wieder 0, sauber() wieder TRUE und die Meldezeile woertlich "stapel_arena:
    // ausgeglichen (Hoechststand 1 von 1 Plaetzen)".
    //
    // WAS HIER GEMESSEN WIRD UND WAS NICHT -- ehrlich benannt. Der Wickel bei 2^64 ist nicht mehr
    // fahrbar, und ein Test, der 2^32 Runden dreht, gehoert nicht in einen Sekundenlauf. Gemessen
    // wird deshalb ueber 2^16 hinaus: das faengt jede Verengung auf 8 oder 16 Bit am Objekt. Gegen
    // eine Verengung auf 32 Bit steht der compile-harte Breiten-Assert in stapel_arena.hpp, der die
    // Zaehler an MessArenaHeiss::versuche bindet -- also an genau die Zusage, aus der die Exaktheit
    // ueberhaupt stammt.
    {
        constexpr std::uint64_t kAbweisungen = 70000; // > 2^16, in Millisekunden zu fahren
        ms::StapelArena         stapel;
        pruefe(stapel.reservieren(1u, ms::VorabBeruehrung::Ja), "Aufbau (14): Tiefe 1");

        ms::StapelEintrag e{};
        e.log_index = kKoederZuTief;
        (void)stapel.hinauf(e);                                                          // der eine Platz ist belegt
        for (std::uint64_t i = 0; i < kAbweisungen; ++i) (void)stapel.hinauf(e);         // alle zu tief
        (void)stapel.herunter(nullptr);                                                  // wieder leer
        for (std::uint64_t i = 0; i < kAbweisungen; ++i) (void)stapel.herunter(nullptr); // alle unpaarig

        ms::StapelBefund const b = stapel.befund();
        char                   text[320];
        (void)ms::befund_zeile(b, text, sizeof(text));

        std::cout << "-- (14) die Fehlerzaehler wickeln nicht --\n";
        std::cout << "     " << kAbweisungen << " Abweisungen je Klasse -> zu_tief = " << b.zu_tief
                  << ", unpaarige_aus = " << b.unpaarige_aus << "\n";
        std::cout << "     " << text << "\n";
        pruefe(b.zu_tief == kAbweisungen, "(14a) zu_tief zaehlt ueber 2^16 hinaus exakt weiter");
        pruefe(b.unpaarige_aus == kAbweisungen, "(14b) unpaarige_aus zaehlt ueber 2^16 hinaus exakt weiter");
        pruefe(!b.sauber(), "(14c) der Befund bleibt unsauber -- er kippt nicht auf ausgeglichen zurueck");
        pruefe(std::string_view{text}.find("ausgeglichen") == std::string_view::npos,
               "(14d) die Meldung sagt NICHT ausgeglichen");
        pruefe(sizeof(stapel.zu_tief()) == sizeof(std::uint64_t) &&
                   sizeof(stapel.unpaarige_aus()) == sizeof(std::uint64_t) &&
                   sizeof(b.zu_tief) == sizeof(std::uint64_t),
               "(14e) die Zaehler bleiben auch auf dem Weg nach draussen 64 Bit breit");
    }

    // -- (15) DER RUECKGABE-VERTRAG DER MELDEZEILEN IST EHRLICH -------------------------------------
    // BEFUND: beide befund_zeile()-Ueberladungen gaben den snprintf-Rueckgabewert weiter und
    // versprachen in der Doku "geschriebene Zeichen". Gemessen an einem 16-Byte-Puffer: Rueckgabe
    // 124, strlen 15. Mit [[nodiscard]] laedt eine solche Zahl zum Weiterrechnen ein (puffer + rc),
    // und das zeigte 109 Byte hinter das Puffer-Ende.
    //
    // Geprueft werden BEIDE Ueberladungen -- die Zusage gilt fuer das Modul, nicht fuer eine Datei.
    {
        std::cout << "-- (15) der Rueckgabe-Vertrag der Meldezeilen --\n";

        ms::UeberlaufBefund ub{};
        ub.kapazitaet = 3;
        ub.versuche   = 4;
        ub.belegt     = 3;
        ub.verloren   = 1;

        ms::StapelBefund sb{};
        sb.kapazitaet    = 4;
        sb.offen         = 1;
        sb.hoechststand  = 4;
        sb.zu_tief       = 2;
        sb.unpaarige_aus = 3;

        char klein[16];
        char gross[400];

        ms::MeldungsLaenge const m_kurz = ms::befund_zeile(ub, klein, sizeof(klein));
        std::size_t const        m_len  = std::strlen(klein);
        ms::MeldungsLaenge const m_voll = ms::befund_zeile(ub, gross, sizeof(gross));
        ms::MeldungsLaenge const m_mass = ms::befund_zeile(ub, nullptr, 0u);

        ms::MeldungsLaenge const s_kurz = ms::befund_zeile(sb, klein, sizeof(klein));
        std::size_t const        s_len  = std::strlen(klein);
        ms::MeldungsLaenge const s_voll = ms::befund_zeile(sb, gross, sizeof(gross));
        ms::MeldungsLaenge const s_mass = ms::befund_zeile(sb, nullptr, 0u);

        std::cout << "     mess_arena  16-Byte-Puffer: geschrieben = " << m_kurz.geschrieben << ", strlen = " << m_len
                  << ", benoetigt = " << m_kurz.benoetigt << ", abgeschnitten = " << (m_kurz.abgeschnitten() ? 1 : 0)
                  << "\n";
        std::cout << "     stapel_arena 16-Byte-Puffer: geschrieben = " << s_kurz.geschrieben << ", strlen = " << s_len
                  << ", benoetigt = " << s_kurz.benoetigt << ", abgeschnitten = " << (s_kurz.abgeschnitten() ? 1 : 0)
                  << "\n";

        pruefe(m_kurz.geschrieben == m_len && s_kurz.geschrieben == s_len,
               "(15a) `geschrieben` ist die Zahl der Zeichen IM Puffer -- gegen strlen gemessen");
        pruefe(m_kurz.geschrieben == sizeof(klein) - 1u && s_kurz.geschrieben == sizeof(klein) - 1u,
               "(15b) der kleine Puffer ist bis auf die Abschluss-Null gefuellt");
        pruefe(m_kurz.abgeschnitten() && s_kurz.abgeschnitten(),
               "(15c) das Abschneiden wird gemeldet und nicht verschwiegen");
        pruefe(m_kurz.benoetigt > m_kurz.geschrieben && s_kurz.benoetigt > s_kurz.geschrieben,
               "(15d) `benoetigt` ist die volle Laenge und damit groesser als das Geschriebene");
        pruefe(m_kurz.benoetigt == m_voll.benoetigt && s_kurz.benoetigt == s_voll.benoetigt,
               "(15e) `benoetigt` haengt am Text, nicht an der Puffergroesse");
        pruefe(!m_voll.abgeschnitten() && !s_voll.abgeschnitten() && m_voll.geschrieben == m_voll.benoetigt &&
                   s_voll.geschrieben == s_voll.benoetigt,
               "(15f) im grossen Puffer ist nichts abgeschnitten und beide Zahlen fallen zusammen");
        pruefe(m_mass.geschrieben == 0 && s_mass.geschrieben == 0 && m_mass.benoetigt == m_voll.benoetigt &&
                   s_mass.benoetigt == s_voll.benoetigt,
               "(15g) (nullptr, 0) misst den Puffer aus, ohne zu schreiben");
        pruefe(m_mass.abgeschnitten() && s_mass.abgeschnitten(),
               "(15h) auch der Ausmess-Aufruf gilt als abgeschnitten -- er hat nichts abgeliefert");
        pruefe(!m_kurz.formatfehler && !s_kurz.formatfehler, "(15i) kein Formatfehler auf dem regulaeren Weg");
    }

    // -- (16) DER AUFBAU SAGT, WAS ER GETAN HAT ------------------------------------------------------
    // Zwei Befunde in einem Abschnitt, beide ueber den AUFBAU:
    //   * reservieren(0) kehrte um, BEVOR die alte Reservierung freigegeben war. Das Objekt meldete
    //     danach "leer" -- kapazitaet 0, Basis nullptr --, waehrend der alte Block weiter abgebildet
    //     blieb. Sichtbar ist das nicht an der Schnittstelle, sondern am ADRESSRAUM; also wird dort
    //     gemessen und nicht behauptet.
    //   * speicher_aufbauen() gab ein nacktes `a && b` und verlor damit, WELCHE der beiden
    //     Reservierungen gefehlt hat -- bei zwei Arenen mit voellig verschiedener Dimensionierung
    //     ist genau das die Auskunft, die der Meldende braucht.
    {
        std::cout << "-- (16) der Aufbau sagt, was er getan hat --\n";

        // Die Schranke wird GERECHNET, nicht gegriffen, und der Nenner kommt aus ms::kSeitenBytes --
        // dieselbe Quelle, aus der auch MS-2 rechnet, und dort gegen sysconf(_SC_PAGESIZE) gemessen.
        // 15/16 der Abbildung lassen genug Luft fuer die uebrige Prozess-Aktivitaet zwischen zwei
        // Ablesungen und sind trotzdem weit von jedem Rauschen entfernt.
        constexpr std::uint64_t kGrossBytes  = 64ull * 1024ull * 1024ull;
        constexpr std::uint64_t kGross       = kGrossBytes / sizeof(ms::MessCheckpointZeile);
        constexpr std::uint64_t kGrossSeiten = kGrossBytes / ms::kSeitenBytes;
        constexpr std::uint64_t kSchranke    = kGrossSeiten - (kGrossSeiten / 16u);

        std::uint64_t const vor = vm_seiten();
        if (vor == 0u) {
            // KEIN stiller Durchlauf: die Nichtmessbarkeit wird ausgesprochen.
            std::cout << "     /proc/self/statm nicht lesbar -- die Freigabe ist hier UNGEPRUEFT\n";
        } else {
            ms::MessArena arena;
            pruefe(arena.reservieren(kGross, ms::VorabBeruehrung::Nein), "Aufbau (16): 64 MiB");
            std::uint64_t const mit  = vm_seiten();
            bool const          weg  = !arena.reservieren(0u, ms::VorabBeruehrung::Nein);
            std::uint64_t const nach = vm_seiten();

            std::cout << "     VmSize in Seiten: vor = " << vor << ", mit 64 MiB = " << mit
                      << ", nach reservieren(0) = " << nach << "\n";
            pruefe(weg, "(16a) reservieren(0) weist ab");
            pruefe(mit > vor + kSchranke, "(16b) die Reservierung ist im Adressraum sichtbar (Positivkontrolle)");
            pruefe(mit > nach + kSchranke, "(16c) reservieren(0) gibt die alte Abbildung wirklich frei");
            pruefe(arena.kapazitaet() == 0 && arena.nutzlast_basis() == nullptr,
                   "(16d) und die Zugriffsfunktionen beschreiben denselben leeren Zustand");
        }

        ms::CheckpointSpeicher sp_a;
        ms::AufbauBefund const ohne_mess = ms::speicher_aufbauen(sp_a, 0u, 16u, ms::VorabBeruehrung::Ja);
        ms::CheckpointSpeicher sp_b;
        ms::AufbauBefund const ohne_stapel = ms::speicher_aufbauen(sp_b, 16u, 0u, ms::VorabBeruehrung::Ja);
        ms::CheckpointSpeicher sp_c;
        ms::AufbauBefund const beide = ms::speicher_aufbauen(sp_c, 16u, 16u, ms::VorabBeruehrung::Ja);

        std::cout << "     Aufbau-Befund: (0,16) -> mess = " << (ohne_mess.mess ? 1 : 0)
                  << ", stapel = " << (ohne_mess.stapel ? 1 : 0) << " | (16,0) -> mess = " << (ohne_stapel.mess ? 1 : 0)
                  << ", stapel = " << (ohne_stapel.stapel ? 1 : 0) << "\n";
        pruefe(!ohne_mess.steht() && !ohne_mess.mess && ohne_mess.stapel,
               "(16e) fehlt die MESS-Arena, sagt der Befund genau das -- und dass der Stapel steht");
        pruefe(!ohne_stapel.steht() && ohne_stapel.mess && !ohne_stapel.stapel,
               "(16f) fehlt die STAPEL-Arena, sagt der Befund genau das -- und dass die Mess-Arena steht");
        pruefe(beide.steht() && beide.mess && beide.stapel, "(16g) Positivkontrolle: beide stehen, steht() ist TRUE");
    }

    if (g_fail == 0) {
        std::cout << "OK: MS-1\n";
        return 0;
    }
    std::cout << "FEHLGESCHLAGEN: " << g_fail << " Pruefung(en)\n";
    return 1;
}
