// linux_perf_pmc_smoke — CE-DL2 Klein-Pilot (gate-frei): beweist die GESCHLOSSENE PMC-Naht für den LINUX-Zweig
// (LinuxPerfPmcSource via perf_event_open) LITERAL, OHNE reale DLLs zu bauen. Spiegel von m3v2_pmc_smoke.cpp.
//
// Linux-conditional registriert (if(UNIX AND NOT APPLE) + COMDARE_ENABLE_PMC) → der Windows-Build sieht dieses
// Target NIE. Auf Linux ist der Test EHRLICH-degradierend:
//   (1) make_pmc_source() liefert mit -DCOMDARE_ENABLE_PMC=ON auf Linux eine LinuxPerfPmcSource.
//   (2) begin()/end() klammern eine SPEICHERRÜHRENDE Schleife (pointer-chasing über einen großen Puffer →
//       garantiert echte L1/LL-Cache-Misses, sofern die HW-Counter zugänglich sind).
//   (3) Bei Zugriff (perf_event_paranoid erlaubt self-monitoring, exclude_kernel=1) → available()==1 UND
//       mindestens ein Counter (l1/l3/dtlb) != 0 → PASS.
//   (4) KEIN Zugriff (EACCES/EPERM durch strengen paranoid, keine HW-Counter, Container ohne perf) → die Source
//       meldet EHRLICH available()==0; der Test SKIPpt sauber (Exit 0), ohne zu crashen und ohne erfundene Werte.
//
// Es wird NIE behauptet, dass auf jedem Runner counter!=0 gilt — das ist HW-/Policy-abhängig (prod-Runner).
// Geprüft wird nur die Implikation "available ⇒ mind. ein Counter befüllt" und "kein-Zugriff ⇒ sauberer Skip".

#include "pmc_source_factory.hpp" // make_pmc_source / IPmcSource / PmcCounters

#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

namespace bld = comdare::cache_engine::builder;

int main() {
    // (1) Die EINE PMC-Quelle (Factory wählt build-/OS-abhängig). Auf Linux+PMC → LinuxPerfPmcSource.
    std::unique_ptr<bld::IPmcSource> pmc = bld::make_pmc_source();
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
    bld::PmcCounters const delta = pmc->end();
    // Compiler-Eliminierung des Loops verhindern (acc muss beobachtbar bleiben).
    std::cout << "workload_acc(checksum)=" << acc << "\n";

    std::cout << "delta.available             = " << (delta.available ? "1" : "0") << "\n";
    std::cout << "delta.cache_misses_l1       = " << delta.cache_misses_l1 << "\n";
    std::cout << "delta.cache_misses_l2       = " << delta.cache_misses_l2 << "\n";
    std::cout << "delta.cache_misses_l3       = " << delta.cache_misses_l3 << "\n";
    std::cout << "delta.dtlb_misses           = " << delta.dtlb_misses << "\n";
    std::cout << "delta.coherence_invalidations = " << delta.coherence_invalidations << "\n";
    std::cout << "delta.energy_micro_joules   = " << delta.energy_micro_joules << "\n";

    if (!delta.available) {
        // EHRLICHER Skip: kein Counter-Zugriff (strenger perf_event_paranoid / keine HW / Container ohne perf).
        // Kein Crash, kein erfundener Wert — die Source meldete korrekt "nicht verfügbar".
        std::cout << "SMOKE_SKIP (no PMC access — honest available=0)\n";
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
