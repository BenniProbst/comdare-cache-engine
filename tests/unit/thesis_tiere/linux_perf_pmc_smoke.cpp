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
//   (2) begin()/end() klammern eine SPEICHERRUEHRENDE Schleife (pointer-chasing ueber einen grossen Puffer ->
//       garantiert echte L1/LL-Cache-Misses, sofern die HW-Counter zugaenglich sind).
//   (3) Bei Zugriff (perf_event_paranoid erlaubt self-monitoring, exclude_kernel=1) -> available()==1 UND
//       mindestens ein Counter (l1/l3/dtlb) != 0 -> PASS.
//   (4) KEIN Zugriff -> das Verdikt haengt seit M-2/B3 am BAU: ohne einkompilierte Quelle sauberer Skip
//       (Exit 0), MIT einkompilierter Quelle SMOKE_FAIL (Exit 1). Begruendung am kPmcExpected-Anker unten.
//
// Es wird NIE behauptet, dass auf jedem Runner counter!=0 gilt -- das ist HW-/Policy-abhaengig (prod-Runner).
// Geprueft wird die Implikation "available => mind. ein Counter befuellt" und "Bau erwartet PMC => Zugriff".

#include "pmc_source_factory.hpp" // make_pmc_source / IPmcSource / PmcCounters

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

    pmc->begin();
    std::uint64_t idx = 0;
    std::uint64_t acc = 0;
    for (std::size_t step = 0; step < kN; ++step) { // kN Sprünge durch den Puffer (zeiger-verkettet).
        idx = buf[idx];
        acc += idx;
    }
    ::comdare::cache_engine::measurement::PmcCounters const delta = pmc->end();
    // Compiler-Eliminierung des Loops verhindern (acc muss beobachtbar bleiben).
    std::cout << "workload_acc(checksum)=" << acc << "\n";

    std::cout << "delta.available             = " << (delta.available ? "1" : "0") << "\n";
    std::cout << "delta.cache_misses_l1       = " << delta.cache_misses_l1 << "\n";
    std::cout << "delta.cache_misses_l2       = " << delta.cache_misses_l2 << "\n";
    std::cout << "delta.cache_misses_l3       = " << delta.cache_misses_l3 << "\n";
    std::cout << "delta.dtlb_misses           = " << delta.dtlb_misses << "\n";
    std::cout << "delta.coherence_invalidations = " << delta.coherence_invalidations << "\n";
    std::cout << "delta.energy_micro_joules   = " << delta.energy_micro_joules << "\n";

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
        std::cout << "SMOKE_SKIP (no PMC access - honest available=0, COMDARE_ENABLE_PMC nicht einkompiliert)\n";
        return 0;
    }

    // available==1 → mindestens ein echter Counter MUSS befüllt sein (sonst wäre available=true unehrlich).
    bool const any_counter = delta.cache_misses_l1 != 0 || delta.cache_misses_l3 != 0 || delta.dtlb_misses != 0 ||
                             delta.energy_micro_joules != 0;

    if (any_counter) {
        std::cout << "SMOKE_OK (live PMC, >=1 counter populated)\n";
        return 0;
    }
    std::cout << "SMOKE_FAIL (available=1 but all counters 0 — dishonest)\n";
    return 1;
}
