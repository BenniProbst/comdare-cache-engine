// m3v2_pmc_smoke — #156-De-Risk Klein-Pilot (gate-frei, 2026-06-20): beweist die GESCHLOSSENE PMC-Naht im LAZY
// WIDE-Mess-Pfad LITERAL, OHNE reale DLLs zu bauen (orthogonal zur schweren E2E-Suite).
//
// Was bewiesen wird:
//   (1) make_pmc_source() liefert lokal (COMDARE_ENABLE_PMC=OFF) eine NullPmcSource → available()=false,
//       begin()/end() ⇒ PmcCounters{} (alle 0, available=false) — KEIN Mess-Overzeug, ehrliche 0.
//   (2) lazy_csv_header() trägt die 7 NEUEN PMC-Spalten GANZ AM ENDE (nach quality_flag), header-getrieben.
//   (3) format_csv_row() emittiert die PMC-Werte als LETZTE Spalten in identischer Reihenfolge → mit NullPmcSource
//       erscheinen sie als 0/…/0/0 (pmc_available=0). Mit Intel-PCM=ON (Montag Linux+PMC) wären sie real.
//
// Der Pilot stellt EINE LazyMeasuredRow händisch zusammen (wie der Iterator es aus PermResult täte: row.pmc = pr.pmc)
// und befüllt row.pmc EXAKT über die EINE PMC-Quelle (begin()→[leerer Batch]→end()), genau wie run_observable_perm.
// Damit ist die Spalten-Existenz + die Default-0/available=0-Belegung literal nachweisbar.

#include "experiment_tree/cache_engine_builder_iterator.hpp" // lazy_csv_header / format_csv_row / LazyMeasuredRow
#include "pmc_source_factory.hpp"                            // make_pmc_source / IPmcSource / PmcCounters

#include <iostream>
#include <string>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace bld = comdare::cache_engine::builder;

int main() {
    // (1) Die EINE PMC-Quelle (Factory wählt build-/OS-abhängig; lokal OFF → NullPmcSource).
    std::unique_ptr<bld::IPmcSource> pmc = bld::make_pmc_source();
    std::cout << "pmc_source.name      = " << pmc->name() << "\n";
    std::cout << "pmc_source.available = " << (pmc->available() ? "1" : "0") << "\n";

    // (1) begin()/end() um den (hier leeren) Mess-Batch — genau das Muster aus run_observable_perm. Delta = 0/false.
    pmc->begin();
    bld::PmcCounters const delta = pmc->end();

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
    char const* const pmc_cols[] = {"pmc_cache_misses_l1", "pmc_cache_misses_l2",         "pmc_cache_misses_l3",
                                    "pmc_dtlb_misses",     "pmc_coherence_invalidations", "pmc_energy_micro_joules",
                                    "pmc_available"};
    int               missing    = 0;
    for (char const* c : pmc_cols)
        if (header.find(c) == std::string::npos) {
            ++missing;
            std::cout << "[ERR] header fehlt: " << c << "\n";
        }

    // available=0 + alle Counter 0 bei NullPmcSource (ehrlich; mit PCM=ON real).
    bool const honest_null = !delta.available && delta.cache_misses_l1 == 0 && delta.cache_misses_l2 == 0 &&
                             delta.cache_misses_l3 == 0 && delta.dtlb_misses == 0 &&
                             delta.coherence_invalidations == 0 && delta.energy_micro_joules == 0;

    std::cout << "missing_pmc_cols=" << missing << "  honest_null=" << (honest_null ? "1" : "0") << "\n";
    std::cout << ((missing == 0 && honest_null) ? "SMOKE_OK\n" : "SMOKE_FAIL\n");
    return (missing == 0 && honest_null) ? 0 : 1;
}
