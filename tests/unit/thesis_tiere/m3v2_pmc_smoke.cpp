// m3v2_pmc_smoke — #156-De-Risk Klein-Pilot (gate-frei, 2026-06-20): beweist die GESCHLOSSENE PMC-Naht im LAZY
// WIDE-Mess-Pfad LITERAL, OHNE reale DLLs zu bauen (orthogonal zur schweren E2E-Suite).
//
// Was bewiesen wird:
//   (1) make_pmc_source() liefert lokal (COMDARE_ENABLE_PMC=OFF) eine NullPmcSource → available()=false,
//       begin()/end() ⇒ PmcCounters{} (alle 0, available=false) — KEIN Mess-Overzeug, ehrliche 0.
//   (2) lazy_csv_header() trägt die 7 NEUEN PMC-Spalten GANZ AM ENDE (nach quality_flag), header-getrieben.
//   (3) format_csv_row() emittiert die PMC-Werte als LETZTE Spalten in identischer Reihenfolge → mit NullPmcSource
//       erscheinen sie seit #83 (C-1(c), KON103) als n/a-Tokens (pmc_available=0) -- die fruehere
//       "0/.../0/0"-Konvention war die stille 0 des Teil-Laufs und ist gefallen (pmc_zelle,
//       cache_engine_builder_iterator.hpp). Mit einkompilierter Quelle sind sie real (Zahl je geliefertem Feld).
//
// Der Pilot stellt EINE LazyMeasuredRow haendisch zusammen (wie der Iterator es aus PermResult taete: row.pmc = pr.pmc)
// und befuellt row.pmc EXAKT ueber die EINE PMC-Quelle (begin()->[ECHTES Messfenster]->end()), genau wie
// run_observable_perm. Damit ist die Spalten-Existenz + die Default-0/available=0-Belegung literal nachweisbar.
//
// MESSFENSTER-KORREKTUR (2026-08-06, Owner-Auflage nach pmc:intel-Rotfund): begin()/end() klammerten bis hierher
// einen LEEREN Batch -- auf Intel bedeutet t_running==0 zwischen begin/end zwingend delta.available=false (die
// LinuxPerfPmcSource verwirft ein Delta ohne Laufzeit als ungueltig, s. PerfCounter::read_scaled), der Smoke fiel
// also nicht wegen fehlendem PMC-Zugriff, sondern weil es NICHTS zu messen gab. Auf AMD "bestand" derselbe Test
// nur zufaellig ueber den Syscall-Overhead selbst (2-stellige Zaehlwerte, Rauschgrenze). Das Fenster klammert
// jetzt DASSELBE speicherruehrende Pointer-Chasing wie linux_perf_pmc_smoke.cpp (32 MiB, kN=1u<<22) -- exakt der
// Workload, der im selben rotgewordenen Job auf BEIDEN Vendoren bereits Millionen-Bereich-Zaehler lieferte
// (prod1 cache_misses_l1~4.19M/dtlb~2.1M; prod2 cache_misses_l1=6701028/l3=4048837/dtlb=3314004) -- also weit
// oberhalb jeder Rauschgrenze auf beiden Herstellern, nicht nur zufaellig auf einem.

#include "experiment_tree/cache_engine_builder_iterator.hpp" // lazy_csv_header / format_csv_row / LazyMeasuredRow
#include "pmc_source_factory.hpp"                            // make_pmc_source / IPmcSource / PmcCounters
#include "pmc_startup_pruefung.hpp" // #83 C-1(c): Gegeneingang -- der Skip nennt, was der Host gekonnt haette

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace bld = comdare::cache_engine::builder;

// M-2 / B3 (P-PMC-1, 2026-08-06) -- DIE ERWARTUNG FOLGT DEM BAU, NICHT DER LAUFZEIT.
// Spiegel des Ankers in linux_perf_pmc_smoke.cpp; Begruendung dort ausgeschrieben. Kurz:
// ohne -DCOMDARE_ENABLE_PMC gibt es auf Linux gar keine PMC-Quelle (der Code kompiliert sich weg), also ist
// available==0 der ehrliche Normalfall. MIT dem Flag heisst available==0 dagegen "gebaut, aber kein Zugriff"
// -- und genau dieser Zustand hat den #37-Preflight bisher gruen passieren lassen.
#if defined(COMDARE_ENABLE_PMC) && defined(__linux__)
constexpr bool kPmcExpected = true;
#else
constexpr bool kPmcExpected = false;
#endif

int main() {
    // (1) Die EINE PMC-Quelle (Factory wählt build-/OS-abhängig; lokal OFF → NullPmcSource).
    std::unique_ptr<::comdare::cache_engine::measurement::IPmcSource> pmc = bld::make_pmc_source();
    std::cout << "pmc_source.name      = " << pmc->name() << "\n";
    std::cout << "pmc_source.available = " << (pmc->available() ? "1" : "0") << "\n";

    // (1) begin()/end() um ein ECHTES Messfenster -- genau das Muster aus run_observable_perm, jetzt mit realer
    // Arbeit dazwischen statt eines leeren Batches (s. Kopf-Kommentar: Messfenster-Korrektur 2026-08-06).
    // Speicherruehrende Last, identisch zu linux_perf_pmc_smoke.cpp: Pointer-Chasing ueber einen Puffer >>
    // LLC-Groesse, damit echte Cache-Misses entstehen. Der Index-Sprung (grossschrittig, prim-teilerfremd)
    // verhindert Hardware-Prefetch.
    constexpr std::size_t      kN = 1u << 22; // 4M * 8B = 32 MiB (> typ. LLC) -> garantierte LL-Misses.
    std::vector<std::uint64_t> buf(kN);
    for (std::size_t i = 0; i < kN; ++i) buf[i] = (i * 2654435761u + 1u) & (kN - 1); // Permutations-Verkettung.

    pmc->begin();
    std::uint64_t idx = 0;
    std::uint64_t acc = 0;
    for (std::size_t step = 0; step < kN; ++step) { // kN Spruenge durch den Puffer (zeiger-verkettet).
        idx = buf[idx];
        acc += idx;
    }
    ::comdare::cache_engine::measurement::PmcCounters const delta = pmc->end();
    // Compiler-Eliminierung des Loops verhindern (acc muss beobachtbar bleiben) -- derselbe Zweck wie
    // workload_acc in linux_perf_pmc_smoke.cpp, hier zusaetzlich als spaeter genutzter Beleg fuer die Zeile
    // unten (n_ops/total_ns bleiben bewusst die alten synthetischen CSV-Platzhalterwerte, s.u.).
    std::cout << "workload_acc(checksum)=" << acc << "\n";

    // EINE Mess-Zeile (wie der Iterator sie aus PermResult zusammensetzt): row.pmc = pr.pmc (= delta).
    ex::LazyMeasuredRow row;
    row.binary_id     = "search_algo=Array256#smoke";
    row.setting_label = "rep=1";
    row.n_ops         = 1000;
    row.timed_ops     = 2000;
    row.total_ns      = 123456;
    row.profile_name  = "YCSB_C_read_only";
    row.build_version = "m3v2";
    row.pmc           = delta; // #156-De-Risk: die HW-PMC-Counter DIESER Messung in die Endspalten

    std::string const header = ex::lazy_csv_header();
    std::string const line   = ex::format_csv_row(row);

    std::cout << "=== HEADER ===\n" << header;
    std::cout << "=== ROW ===\n" << line;

    // Maschinen-lesbarer Beleg: die 7 neuen Spalten existieren im Header (additiv ans Ende).
    // M-3a (2026-08-07): pmc_branch_misses mitgefuehrt. Die Spalte steht nicht im positionsstabilen
    // 7er-Block, sondern im Klasse-C-Anhang -- die Pruefung ist aber dieselbe (Existenz nach NAMEN, nie
    // positional), und seit M-3a traegt sie einen real erhobenen Zaehler statt einer honest-0.
    char const* const pmc_cols[] = {
        "pmc_cache_misses_l1",         "pmc_cache_misses_l2",     "pmc_cache_misses_l3", "pmc_dtlb_misses",
        "pmc_coherence_invalidations", "pmc_energy_micro_joules", "pmc_available",       "pmc_branch_misses"};
    int missing = 0;
    for (char const* c : pmc_cols)
        if (header.find(c) == std::string::npos) {
            ++missing;
            std::cout << "[ERR] header fehlt: " << c << "\n";
        }

    // Seam-Verdikt (NICHT invertiert — Fix M-CE-25/Muster-F 2026-07-13): die geschlossene PMC-Naht ist gueltig,
    // wenn ENTWEDER die Quelle live ist (available=1, z.B. Intel-PCM Montag → reale Counter sind ERFOLG, nicht
    // Fehler) ODER die NullPmcSource ehrliche Null liefert (available=0 → alle Counter MUESSEN 0 sein). FEHLER
    // nur bei available=0 UND einem Counter != 0 (unehrliche Nicht-Null ohne Verfuegbarkeit). Vorher war das
    // Verdikt `honest_null` (= !available && all-zero) hart gefordert → ein ehrlich-live-PMC (available=1) kippte
    // SMOKE_FAIL, obwohl genau das der zu beweisende Erfolgsfall ist.
    bool const counters_all_zero = delta.cache_misses_l1 == 0 && delta.cache_misses_l2 == 0 &&
                                   delta.cache_misses_l3 == 0 && delta.dtlb_misses == 0 &&
                                   delta.coherence_invalidations == 0 && delta.energy_micro_joules == 0;
    // M-2/B3 (2026-08-06): die zweite Haelfte des Verdikts gilt nur noch, WENN der Bau die Quelle gar nicht
    // einkompiliert hat. Vorher war "alle Zaehler 0" das Erfolgskriterium AUCH dann, wenn die Quelle
    // ausdruecklich eingebaut war -- damit testierte der Smoke seine eigene Abwesenheit als Erfolg.
    // Der 13.07.-Inversionsfix bleibt unangetastet: `delta.available` steht unveraendert als erster Term,
    // ein live-PMC (available=1) ist und bleibt der Erfolgsfall und kann nie SMOKE_FAIL kippen.
    bool const pmc_seam_ok = delta.available || (!kPmcExpected && counters_all_zero);

    std::cout << "missing_pmc_cols=" << missing << "  pmc_available=" << (delta.available ? "1" : "0")
              << "  counters_all_zero=" << (counters_all_zero ? "1" : "0")
              << "  pmc_expected_by_build=" << (kPmcExpected ? "1" : "0")
              << "  pmc_seam_ok=" << (pmc_seam_ok ? "1" : "0") << "\n";
    if (kPmcExpected && !delta.available)
        std::cout << "[PMC-FEHLER] COMDARE_ENABLE_PMC ist einkompiliert, die Quelle meldet aber available=0. "
                     "Ursache pruefen: perf_event_paranoid, CAP_PERFMON/Executor-Rechte, Container ohne perf. "
                     "F3-PFLICHT: was gemessen werden KANN, MUSS gemessen werden.\n";
    // #83 (C-1(c), Owner-GO KON103 17.08.): DER SKIP SPRICHT. Im Nicht-einkompiliert-Fall fragt der
    // Gegeneingang den Host (derselbe Koeder wie die Planer-Probe) und warnt auf stderr, wenn PMC
    // VORHANDEN, ABER NICHT VERWENDET ist. Exit bleibt 0 (Bau-Seite soft, KON28-02; die dynamische
    // Kette erreicht den flaglosen Fall seit #83 nicht mehr -- ihr Preflight scheitert vorher).
    if (!kPmcExpected) {
        auto const startup = bld::pmc_startup_pruefe_und_melde(kPmcExpected, delta.available);
        std::cout << "pmc_startup_lage=" << std::string(bld::pmc_startup_label(startup.lage)) << "\n";
    }
    std::cout << ((missing == 0 && pmc_seam_ok) ? "SMOKE_OK\n" : "SMOKE_FAIL\n");
    return (missing == 0 && pmc_seam_ok) ? 0 : 1;
}
