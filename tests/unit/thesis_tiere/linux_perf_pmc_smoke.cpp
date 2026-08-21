// linux_perf_pmc_smoke — CE-DL2 Klein-Pilot (gate-frei): beweist die GESCHLOSSENE PMC-Naht für den LINUX-Zweig
// (LinuxPerfPmcSource via perf_event_open) LITERAL, OHNE reale DLLs zu bauen. Spiegel von m3v2_pmc_smoke.cpp.
//
// REGISTRIERUNG (B5/M-2-Korrektur 2026-08-06, am Objekt nachgelesen): das Target haengt AUSSCHLIESSLICH an
// `if(UNIX AND NOT APPLE)` -- tests/unit/CMakeLists.txt (Suchbegriff: `add_executable(linux_perf_pmc_smoke`).
// Der Windows-Build sieht es NIE. Die frueher hier behauptete Zusatzbedingung "+ COMDARE_ENABLE_PMC" gab es
// NICHT und darf es auch nicht geben: der #37-PMC-Preflight im emittierten Mess-Batch baut dieses Target
// namentlich, er braeche an einem flag-abhaengigen Target. Die Flag-Abhaengigkeit sitzt stattdessen IM Test
// (kPmcExpected, s.u.) -- das Target existiert immer, sein VERDIKT folgt dem Bau.
//
// Auf Linux ist der Test EHRLICH-degradierend:
//   (1) make_pmc_source() liefert mit -DCOMDARE_ENABLE_PMC=ON auf Linux eine LinuxPerfPmcSource.
//   (2) begin()/end() klammern ZWEI Lastformen, weil die vier Zaehler zwei verschiedene Dinge messen:
//       (2a) eine SPEICHERRUEHRENDE Schleife (pointer-chasing ueber einen grossen Puffer -> garantiert
//            echte L1/LL-Cache-Misses, sofern die HW-Counter zugaenglich sind);
//       (2b) seit M-3a eine SPRUNG-ruehrende Schleife (zu 50% unvorhersagbare, datenabhaengige Zweige ->
//            garantiert echte Branch-Misses). Speicherdruck allein erzeugt KEINE Branch-Misses -- die
//            Chasing-Schleife hat genau einen, perfekt vorhersagbaren Rueck-Sprung. Begruendung und die
//            am Objekt gemessenen Gegenproben stehen an der Last selbst.
//   (3) Bei Zugriff (perf_event_paranoid erlaubt self-monitoring, exclude_kernel=1) -> available()==1 UND
//       mindestens ein Counter (l1/l3/dtlb/branch) != 0 -> PASS. Zusaetzlich gilt seit M-3a: ein
//       GEOEFFNETER Branch-Counter, der nach dieser Last exakt 0 liefert, ist SMOKE_FAIL (s. Gate unten).
//   (4) KEIN Zugriff -> das Verdikt haengt seit M-2/B3 am BAU: ohne einkompilierte Quelle sauberer Skip
//       (Exit 0), MIT einkompilierter Quelle SMOKE_FAIL (Exit 1). Begruendung am kPmcExpected-Anker unten.
//
// Es wird NIE behauptet, dass auf jedem Runner counter!=0 gilt -- das ist HW-/Policy-abhaengig (prod-Runner).
// Geprueft wird die Implikation "available => mind. ein Counter befuellt" und "Bau erwartet PMC => Zugriff".

#include "pmc_source_factory.hpp"   // make_pmc_source / IPmcSource / PmcCounters
#include "pmc_startup_pruefung.hpp" // #83 C-1(c): Gegeneingang -- der Skip nennt, was der Host gekonnt haette

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

namespace bld = comdare::cache_engine::builder;

// M-2 / B3 (P-PMC-1, 2026-08-06) -- DIE ERWARTUNG FOLGT DEM BAU, NICHT DER LAUFZEIT.
//
// Bisher war "kein PMC-Zugriff" IMMER ein ehrlicher Skip mit Exit 0. Das war richtig, solange die Frage
// lautete "existiert die Quelle?" -- ohne -DCOMDARE_ENABLE_PMC gibt es auf Linux gar keine PMC-Quelle, der
// Code kompiliert sich weg, und Nicht-Vorhandensein IST kein Fehler.
//
// Es ist aber die falsche Frage, sobald der Bau die Quelle ausdruecklich einkompiliert hat. Dann heisst
// available==0 nicht "nicht gebaut", sondern "gebaut und kommt nicht an die Zaehler" -- und genau dieser
// Zustand hat den #37-Preflight (experiment_plan_director.hpp, Mess-Batch) bisher gruen passieren lassen:
// eine Lane mit blockiertem perf_event_open haette eine MEHRTAEGIGE Messung mit lauter 0-Zaehlern
// durchlaufen und dabei "pmc=ok" testiert.
//
// Owner-Auflage F3 (2026-08-06): "Es ist PFLICHT, dass das gemessen wird, was gemessen werden kann."
// Ein Zaehler, den die Maschine liefern koennte und den wir nicht bekommen, ist damit ein FEHLER.
//
// Der 13.07.-Inversionsfix (Muster-F, s. m3v2_pmc_smoke.cpp) wird dabei NICHT zurueckgenommen: ein
// live-PMC (available==1) bleibt der Erfolgsfall und kippt nie SMOKE_FAIL.
//
// WIRKUNG JE BAU:
//   -DCOMDARE_ENABLE_PMC=ON auf Linux (pmc:amd / pmc:intel, und seit M-2/B1 die emittierten Mess-Jobs)
//       => kPmcExpected == true  => available==0 ist SMOKE_FAIL (Exit 1).
//   Default-Bau (test:unit, Flag OFF)
//       => kPmcExpected == false => der ehrliche Skip bleibt unveraendert (Exit 0).
#if defined(COMDARE_ENABLE_PMC) && defined(__linux__)
constexpr bool kPmcExpected = true;
#else
constexpr bool kPmcExpected = false;
#endif

int main() {
    // (1) Die EINE PMC-Quelle (Factory wählt build-/OS-abhängig). Auf Linux+PMC → LinuxPerfPmcSource.
    std::unique_ptr<::comdare::cache_engine::measurement::IPmcSource> pmc = bld::make_pmc_source();
    std::cout << "pmc_source.name      = " << pmc->name() << "\n";
    bool const avail_before = pmc->available();
    std::cout << "pmc_source.available = " << (avail_before ? "1" : "0") << "\n";

    // (2) Speicherrührende Last: Pointer-Chasing über einen Puffer >> LLC-Größe, damit echte Cache-Misses
    // entstehen. Der Index-Sprung (großschrittig, prim-teilerfremd) verhindert Hardware-Prefetch.
    constexpr std::size_t      kN = 1u << 22; // 4M * 8B = 32 MiB (> typ. LLC) → garantierte LL-Misses.
    std::vector<std::uint64_t> buf(kN);
    for (std::size_t i = 0; i < kN; ++i) buf[i] = (i * 2654435761u + 1u) & (kN - 1); // Permutations-Verkettung.

    // (2b) M-3a (2026-08-07): SPRUNG-ruehrende Last fuer den vierten Zaehler. Das Pointer-Chasing oben
    // erzeugt Cache-Misses, aber so gut wie KEINE Branch-Misses -- seine Schleife hat genau einen
    // Rueck-Sprung, den jeder Predictor perfekt lernt (am Objekt gemessen: 704 Misses auf 4M Iterationen,
    // also Rauschen). Ein Branch-Miss-Zaehler braucht UNVORHERSAGBARE Zweige, nicht Speicherdruck.
    //
    // Lehrbuchmuster (branch prediction): ein Feld PSEUDO-ZUFAELLIGER Bytes, verglichen gegen die Mitte
    // seines Wertebereichs. Der Ausgang ist je Element ~50/50 und traegt keine lernbare Struktur -- die
    // theoretische Miss-Rate ist 50%, und genau die wird auf dieser Maschine auch erreicht.
    //
    // DER COMPILER-BARRIER IST NICHT SCHMUCK, SONDERN TRAGEND: ohne ihn faltet -O3 den Zweig zu einem
    // branchlosen cmov, und dann gibt es NICHTS mehr falsch vorherzusagen. Am Objekt gemessen (gcc -O3,
    // identische Last, 16.777.216 Zweige): MIT Barrier 8.396.125 Misses (50,0%), OHNE Barrier 54 (0,0%).
    // Zur Absicherung, dass der Zaehler wirklich die VORHERSAGBARKEIT misst und keinen Nebeneffekt, wurde
    // dieselbe Last zusaetzlich SORTIERT gefahren (dann sind die Ausgaenge langlaeufig vorhersagbar):
    // 265 Misses (0,0%). Der Zaehler folgt also der Zweig-Struktur, nicht dem Speicherzugriff.
    // Die Datei ist Linux-only registriert (tests/unit/CMakeLists.txt, `if(UNIX AND NOT APPLE)`), gebaut
    // mit gcc UND clang -- beide kennen diese Barrier-Form; ein MSVC-Pfad existiert hier nicht.
    constexpr std::size_t      kBranchN    = 1u << 20; // 1M Elemente
    constexpr std::size_t      kBranchReps = 8;        // -> 8.388.608 unvorhersagbare Zweige
    constexpr std::uint32_t    kThreshold  = 128;      // Mitte von [0,255] -> ~50/50
    std::vector<std::uint32_t> bdata(kBranchN);
    std::uint64_t              rng = 88172645463325252ULL; // xorshift64: deterministisch, aber musterlos
    for (auto& v : bdata) {
        rng ^= rng << 13;
        rng ^= rng >> 7;
        rng ^= rng << 17;
        v = static_cast<std::uint32_t>(rng & 0xFFU);
    }

    pmc->begin();
    std::uint64_t idx = 0;
    std::uint64_t acc = 0;
    for (std::size_t step = 0; step < kN; ++step) { // kN Sprünge durch den Puffer (zeiger-verkettet).
        idx = buf[idx];
        acc += idx;
    }
    std::uint64_t branch_acc = 0;
    for (std::size_t rep = 0; rep < kBranchReps; ++rep)
        for (std::size_t i = 0; i < kBranchN; ++i)
            if (bdata[i] >= kThreshold) {
                asm volatile("" ::: "memory"); // verhindert die cmov-Faltung (s. Messwerte oben)
                branch_acc += bdata[i];
            }
    ::comdare::cache_engine::measurement::PmcCounters const delta = pmc->end();
    // Compiler-Eliminierung der Loops verhindern (beide Akkumulatoren muessen beobachtbar bleiben).
    std::cout << "workload_acc(checksum)=" << acc << "\n";
    std::cout << "branch_acc(checksum)  =" << branch_acc << "\n";
    std::cout << "branch_total_zweige   =" << static_cast<unsigned long long>(kBranchReps) * kBranchN << "\n";

    std::cout << "delta.available             = " << (delta.available ? "1" : "0") << "\n";
    std::cout << "delta.cache_misses_l1       = " << delta.cache_misses_l1 << "\n";
    std::cout << "delta.cache_misses_l2       = " << delta.cache_misses_l2 << "\n";
    std::cout << "delta.cache_misses_l3       = " << delta.cache_misses_l3 << "\n";
    std::cout << "delta.dtlb_misses           = " << delta.dtlb_misses << "\n";
    std::cout << "delta.branch_misses         = " << delta.branch_misses << "\n";
    std::cout << "delta.coherence_invalidations = " << delta.coherence_invalidations << "\n";
    std::cout << "delta.energy_micro_joules   = " << delta.energy_micro_joules << "\n";
    // M-3a: die QUELLEN-Flags sichtbar machen -- sie sind die Ehrlichkeits-Aussage der Zeile, und ohne
    // Ausdruck waere im Log nicht unterscheidbar, ob eine 0 gemessen oder nicht erhoben wurde.
    std::cout << "delta.cache_misses_l3_source_available   = " << (delta.cache_misses_l3_source_available ? "1" : "0")
              << "\n";
    std::cout << "delta.branch_misses_source_available     = " << (delta.branch_misses_source_available ? "1" : "0")
              << "\n";
    std::cout << "delta.energy_micro_joules_source_available= "
              << (delta.energy_micro_joules_source_available ? "1" : "0") << "\n";
    if (delta.branch_misses_source_available)
        std::cout << "branch_miss_rate            = "
                  << (100.0 * static_cast<double>(delta.branch_misses) /
                      static_cast<double>(static_cast<unsigned long long>(kBranchReps) * kBranchN))
                  << " %\n";

    std::cout << "pmc_expected_by_build       = " << (kPmcExpected ? "1" : "0")
              << "   (COMDARE_ENABLE_PMC && __linux__)\n";

    if (!delta.available) {
        // M-2/B3: die Erwartung folgt dem BAU.
        if (kPmcExpected) {
            // FAIL-CLOSED: die Quelle ist einkompiliert und kommt trotzdem nicht an die Zaehler. Das ist kein
            // Skip, sondern der Zustand, gegen den dieser Smoke als Preflight gebaut wurde -- eine mehrtaegige
            // Messung wuerde hier strukturelle 0-Zaehler schreiben und sie als gemessen ausgeben.
            std::cout << "SMOKE_FAIL (COMDARE_ENABLE_PMC einkompiliert, aber KEIN Counter-Zugriff: available=0. "
                         "Ursache pruefen: perf_event_paranoid (sysctl kernel.perf_event_paranoid), "
                         "CAP_PERFMON/Executor-Rechte, Container ohne perf, keine HW-Counter. "
                         "F3-PFLICHT: was gemessen werden KANN, MUSS gemessen werden.)\n";
            return 1;
        }
        // EHRLICHER Skip (Flag NICHT einkompiliert): es gibt gar keine PMC-Quelle -- make_pmc_source() liefert
        // die NullPmcSource. Kein Crash, kein erfundener Wert; die Source meldete korrekt "nicht verfuegbar".
        //
        // #83 (C-1(c), Owner-GO KON103 17.08.): DER SKIP SPRICHT. Bis #83 war er wortkarg -- ob dieser Host
        // PMC gekonnt HAETTE, stand nirgends. Der Gegeneingang stellt genau diese Frage (derselbe Koeder wie
        // die Planer-Probe) und warnt auf stderr, wenn PMC VORHANDEN, ABER NICHT VERWENDET ist. Der Exit
        // bleibt 0: im test:unit-Kontext (Default-Bau, Flag OFF) ist Nicht-Einbau kein Fehler (Bau-Seite
        // soft, KON28-02) -- die Kette erreicht diesen Pfad seit #83 ohnehin nicht mehr (der emittierte
        // Preflight scheitert bei flagloser Emission VOR den Smokes, experiment_plan_director.hpp).
        auto const startup = bld::pmc_startup_pruefe_und_melde(kPmcExpected, delta.available);
        std::cout << "pmc_startup_lage            = " << std::string(bld::pmc_startup_label(startup.lage)) << "\n";
        std::cout << "SMOKE_SKIP (no PMC access - honest available=0, COMDARE_ENABLE_PMC nicht einkompiliert)\n";
        return 0;
    }

    // available==1 → mindestens ein echter Counter MUSS befüllt sein (sonst wäre available=true unehrlich).
    bool const any_counter = delta.cache_misses_l1 != 0 || delta.cache_misses_l3 != 0 || delta.dtlb_misses != 0 ||
                             delta.branch_misses != 0 || delta.energy_micro_joules != 0;

    // M-3a (2026-08-07) -- EIGENES, FAIL-CLOSED GATE FUER DEN VIERTEN ZAEHLER, nach demselben Muster wie
    // kPmcExpected oben: die Erwartung folgt der QUELLE, nicht der Hoffnung. Meldet die Source, dass der
    // Branch-Counter GEOEFFNET wurde, dann hat dieser Lauf gerade 8.388.608 zu 50% unvorhersagbare Zweige
    // ausgefuehrt -- ein Ergebnis von exakt 0 waere dann kein Messwert, sondern ein Beleg dafuer, dass das
    // Feld nur umbenannt und nicht angebunden wurde (genau der Fehler, gegen den dieses Gate steht).
    // Ist die Quelle NICHT offen (Flag false), wird hier nichts gefordert -- das ist die ehrliche
    // Teilverfuegbarkeit, kein Fehler, und die CSV rendert dafuer "n/a".
    if (delta.branch_misses_source_available && delta.branch_misses == 0) {
        std::cout << "SMOKE_FAIL (branch_misses_source_available=1, aber branch_misses=0 nach "
                  << (static_cast<unsigned long long>(kBranchReps) * kBranchN)
                  << " unvorhersagbaren Zweigen -- der Zaehler ist geoeffnet, aber nicht angebunden. "
                     "M-3a-Regression.)\n";
        return 1;
    }

    if (any_counter) {
        std::cout << "SMOKE_OK (live PMC, >=1 counter populated)\n";
        return 0;
    }
    std::cout << "SMOKE_FAIL (available=1 but all counters 0 — dishonest)\n";
    return 1;
}
