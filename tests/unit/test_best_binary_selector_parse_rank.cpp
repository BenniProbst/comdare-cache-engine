// Characterization-/Golden-Master-Test (Feathers) der bisher UNGETESTETEN, AUSGELIEFERTEN Produktionslogik
// des best_binary_selector-CLI (#172 „Versprechen 1"): parse_measurement_csv (header-getriebener Parser) +
// rank_binaries (Strategy ueber RankingCriterion). Das Tool ist in tools/best_binary_selector als
// add_executable registriert, aber NIE als add_test — parse/rank sind Produktionscode ohne Coverage. Dieser
// Test schliesst die Luecke gate-frei (synthetische Golden-Fixture, KEINE #156-Messdaten, kein Treiber-Lauf).
//
// Deckt ab: Pflichtspalten (binary_id/ns_per_op/two_phase_valid), optionale op_*-Spalten (praesent + FEHLEND ->
// graceful 0/na), Zeilen-Filter (two_phase_valid=0 + Wert<=0 ausgeschlossen), Median (nearest-rank untere Mitte
// vals[(n-1)/2]) und den Tie-Break (gleicher Median -> mehr Samples zuerst, dann binary_id).

#include "best_binary_selector.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace bb = comdare::cache_engine::best_binary;

static int  g_fail = 0;
static void check(bool ok, char const* msg) {
    std::printf("  [%s] %s\n", ok ? "OK" : "FAIL", msg);
    if (!ok) ++g_fail;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("FAIL: Fixture-Pfad fehlt (argv[1])\n");
        return 2;
    }
    std::string const fixture = argv[1];

    // ── parse_measurement_csv ────────────────────────────────────────────────────────────────────
    std::vector<bb::MeasurementRow> rows;
    int const                       n = bb::parse_measurement_csv(fixture, rows);
    check(n == 9, "parse: 9 Datenzeilen gelesen");
    check(rows.size() == 9u, "parse: 9 MeasurementRow");
    check(rows[0].binary_id == "bin_fast" && rows[0].ns_per_op == 100.0 && rows[0].two_phase_valid,
          "parse: row0 binary_id/ns_per_op/two_phase_valid header-getrieben gemappt");
    check(rows[0].op_insert_p50 == 10.0 && rows[0].op_scan_p50 == 40.0, "parse: row0 optionale op_*-Spalten gemappt");
    check(rows[0].op_rmw_p50 == 0.0, "parse: fehlende op_rmw_p50-Spalte -> 0 (graceful, header-getrieben)");
    check(!rows[4].two_phase_valid, "parse: row4 two_phase_valid=0 korrekt false");

    // ── rank_binaries: Metric::ns_per_op ─────────────────────────────────────────────────────────
    // bin_fast {100,110,120} -> Median vals[1]=110, samples 3; bin_mid2 {200,200} -> 200/2; bin_mid {200} -> 200/1;
    // bin_slow {300} -> 300/1; bin_invalid (valid=0) + bin_zero (Wert 0) ausgeschlossen.
    auto const r_ns = bb::rank_binaries(rows, bb::RankingCriterion{bb::Metric::ns_per_op});
    check(r_ns.size() == 4u, "rank ns_per_op: 4 (bin_invalid two_phase_valid=0 + bin_zero Wert 0 ausgeschlossen)");
    check(r_ns[0].binary_id == "bin_fast" && r_ns[0].median_value == 110.0 && r_ns[0].samples == 3,
          "rank ns_per_op[0]: bin_fast, Median 110 (nearest-rank), 3 Samples");
    check(r_ns[1].binary_id == "bin_mid2" && r_ns[1].median_value == 200.0 && r_ns[1].samples == 2,
          "rank ns_per_op[1]: bin_mid2 (Tie 200 -> mehr Samples zuerst)");
    check(r_ns[2].binary_id == "bin_mid" && r_ns[2].samples == 1, "rank ns_per_op[2]: bin_mid (Tie 200, 1 Sample)");
    check(r_ns[3].binary_id == "bin_slow" && r_ns[3].median_value == 300.0, "rank ns_per_op[3]: bin_slow 300");

    // ── rank_binaries: Metric::insert (optionale Spalte praesent) ─────────────────────────────────
    // bin_fast {10,11,12}->11/3; bin_slow {13}->13/1; bin_mid {14}->14/1; bin_mid2 {15,16}->vals[0]=15/2.
    auto const r_ins = bb::rank_binaries(rows, bb::RankingCriterion{bb::Metric::insert});
    check(r_ins.size() == 4u, "rank insert: 4");
    check(r_ins[0].binary_id == "bin_fast" && r_ins[0].median_value == 11.0, "rank insert[0]: bin_fast Median 11");
    check(r_ins[3].binary_id == "bin_mid2" && r_ins[3].median_value == 15.0,
          "rank insert[3]: bin_mid2 Median 15 (nearest-rank untere Mitte von {15,16})");

    // ── rank_binaries: Metric::rmw (Spalte FEHLT -> alle 0 -> n/a -> leer) ────────────────────────
    auto const r_rmw = bb::rank_binaries(rows, bb::RankingCriterion{bb::Metric::rmw});
    check(r_rmw.empty(), "rank rmw: leer (fehlende op_rmw_p50-Spalte -> alle Werte 0 -> ausgeschlossen)");

    std::printf(g_fail == 0 ? "BEST-BINARY PARSE+RANK: ALLE OK\n" : "BEST-BINARY PARSE+RANK: %d FAIL\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
