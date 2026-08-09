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
// faengt genau diese Fehlerklasse beim Uebersetzen: ein Vektor-Member zerstoert trivially_copyable.
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
[[nodiscard]] std::size_t koeder_groesse_wuerfeln(char const*& woher) noexcept {
    std::uint32_t roh = 0;
    woher             = "/dev/urandom";
    if (std::FILE* q = std::fopen("/dev/urandom", "rb"); q != nullptr) {
        if (std::fread(&roh, sizeof(roh), 1u, q) != 1u) roh = 0u;
        (void)std::fclose(q);
    }
    if (roh == 0u) {
        // Kein Rueckfall auf eine Konstante: die Adresse eines Automatikobjekts ist dem Uebersetzer
        // ebenfalls unbekannt (ASLR), taugt also fuer denselben Zweck.
        woher = "Stapeladresse/ASLR (/dev/urandom nicht lesbar)";
        roh   = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&roh) >> 4);
    }
    // 4096 .. 65535 Byte: gross genug, dass die Belegung kein Kleinkram ist, klein genug fuer jede
    // Maschine -- und nie 0, damit die Byte-Pruefung unten eine echte Aussage bleibt.
    return static_cast<std::size_t>(4096u + (roh % 61440u));
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
        bool const             steht = ms::speicher_aufbauen(sp, 4u * kAppends, 64u, ms::VorabBeruehrung::Ja);
        pruefe(steht, "Aufbau: beide Arenen reserviert");

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
        pruefe(ms::speicher_aufbauen(sp, 1024u, 64u, ms::VorabBeruehrung::Ja), "Aufbau (4)");

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
        pruefe(std::string_view{text}.find("von 4") != std::string_view::npos,
               "(5f) die Meldung nennt den Nenner, nicht nur die Zahl");
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
        pruefe(ms::speicher_aufbauen(sp, 256u, 16u, ms::VorabBeruehrung::Ja), "Aufbau (8)");

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

    if (g_fail == 0) {
        std::cout << "OK: MS-1\n";
        return 0;
    }
    std::cout << "FEHLGESCHLAGEN: " << g_fail << " Pruefung(en)\n";
    return 1;
}
