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
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace bb = comdare::cache_engine::best_binary;

// ── K-5-Paritaets-Gate (2026-07-19): der hart gespiegelte ABI-Spiegel des self-contained Tools MUSS ──
// mit dem autoritativen Decl-Header uebereinstimmen, sonst schreibt das Manifest FALSCHE Provenienz
// (der 5/".A5."-Drift des Bau-INC-2b-Stands blieb genau so unbemerkt). Der Vergleich lebt HIER im Test
// (nicht im Tool): best_binary_selector bleibt engine-include-frei/self-contained (C++17-bat-Build).
// Relative Includes (Decl-Schliessung ist relativ/std-only) -> KEINE tests-CMake-Aenderung
// (Hotspot-8-Regel der Parallelisierungs-ANALYSE 20260719).
#include "../../libs/cache_engine/include/cache_engine/abi/anatomy_module_abi_v1_decl.hpp"
static_assert(bb::kAbiMajor == COMDARE_ANATOMY_ABI_MAJOR,
              "K-5: kAbiMajor-Spiegel (best_binary_selector.hpp) driftet vom Decl-Header -- syncen!");
static_assert(bb::kAbiMinor == COMDARE_ANATOMY_ABI_MINOR,
              "K-5: kAbiMinor-Spiegel (best_binary_selector.hpp) driftet vom Decl-Header -- syncen!");
static_assert(bb::kAbiMagic == COMDARE_ANATOMY_ABI_MAGIC,
              "K-5: kAbiMagic-Spiegel (best_binary_selector.hpp) driftet vom Decl-Header -- syncen!");

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

    // ── (REV-DATA-04, WP-5 2026-07-16): strikter Zahl-Parser + isfinite ──────────────────────────
    // Fixture 2 (argv[2]): nan/inf/-inf/12junk/leerer Pflichtwert/op_*-junk werden mit Diagnose verworfen;
    // gueltige Dezimal- + Exponentialwerte bleiben; leeres optionales op_*-Feld bleibt 0 (n/a-Semantik).
    if (argc >= 3) {
        std::vector<bb::MeasurementRow> srows;
        std::vector<std::string>        rejects;
        int const                       sn = bb::parse_measurement_csv(argv[2], srows, &rejects);
        check(sn == 3, "strict: 3 von 9 Zeilen akzeptiert (12junk/nan/inf/-inf/leer-Pflicht/op-junk verworfen)");
        check(rejects.size() == 6u, "strict: 6 Verwerfungs-Diagnosen gesammelt");
        check(srows.size() == 3u && srows[0].binary_id == "bin_ok" && srows[0].ns_per_op == 100.5,
              "strict: Dezimalwert 100.5 akzeptiert");
        check(srows.size() >= 2u && srows[1].ns_per_op == 120.5, "strict: Exponentialwert 1.205e2 akzeptiert");
        // bin_opt_junk hat '17junk' im op_insert-Feld -> Zeile verworfen; bin_opt_empty (leer) bleibt mit 0.
        bool found_opt_junk = false, found_opt_empty = false;
        for (auto const& r : srows) {
            if (r.binary_id == "bin_opt_junk") found_opt_junk = true;
            if (r.binary_id == "bin_opt_empty") found_opt_empty = true;
        }
        check(!found_opt_junk, "strict: '17junk' in optionalem op_*-Feld -> Zeile verworfen");
        check(found_opt_empty, "strict: LEERES optionales op_*-Feld -> Zeile bleibt (Wert 0 = n/a)");
        // NaN/Inf duerfen nie mehr ins Ranking: sort-Ordnung deterministisch, kein NaN-Median moeglich.
        auto const r_strict = bb::rank_binaries(srows, bb::RankingCriterion{bb::Metric::ns_per_op});
        check(r_strict.size() == 2u, "strict rank: 2 binary_ids (bin_ok + bin_opt_empty), kein NaN/Inf im Ranking");
    } else {
        check(false, "strict: Fixture 2 (argv[2]) fehlt");
    }

    // ── (REV-DATA-07, WP-5 2026-07-16): Zell-Raster-Stratifizierung ──────────────────────────────
    // Fixture 3 (argv[3]): bin_cherry deckt nur die leichte ycsb_a-Zelle ab (Median 50 < alles) und haette
    // im binary_id-Aggregat gewonnen; mit Zell-Raster (union grid) wird es DISQUALIFIZIERT — bin_complete
    // (beide Zellen; Median der Zell-Mediane {100,500} = 100, untere Mitte) gewinnt. Wiederholungs-Segmente
    // im setting fallen aus dem Zell-Schluessel (2 Reps = 2 Samples EINER Zelle, nicht 2 Zellen).
    if (argc >= 4) {
        std::vector<bb::MeasurementRow> crows;
        int const                       cn = bb::parse_measurement_csv(argv[3], crows);
        check(cn == 6, "cells: 6 Datenzeilen gelesen");
        check(crows.size() >= 2u && crows[0].cell_key() == crows[1].cell_key(),
              "cells: Wiederholungs-Segment faellt aus dem Zell-Schluessel (rep0/rep1 = EINE Zelle)");
        check(crows.size() >= 3u && crows[0].cell_key() != crows[2].cell_key(),
              "cells: ycsb_a- und ycsb_e-Zeilen sind verschiedene Zellen");

        std::vector<std::string> disq;
        auto const               r_cells = bb::rank_binaries(crows, bb::RankingCriterion{bb::Metric::ns_per_op}, &disq);
        check(disq.size() == 1u && disq[0].rfind("bin_cherry", 0) == 0,
              "cells: bin_cherry (nur 1/2 Zellen) disqualifiziert (Diagnose)");
        check(r_cells.size() == 1u, "cells: nur der vollstaendige Kandidat wird gerankt");
        check(!r_cells.empty() && r_cells[0].binary_id == "bin_complete" && r_cells[0].median_value == 100.0,
              "cells: bin_complete gewinnt mit Median der Zell-Mediane {100,500} = 100");
        check(!r_cells.empty() && r_cells[0].samples == 4u && r_cells[0].cells == 2u,
              "cells: samples=4 (2 Reps x 2 Zellen), cells=2");
    } else {
        check(false, "cells: Fixture 3 (argv[3]) fehlt");
    }

    // ── (REV-DATA-05, WP-5 2026-07-16): Artefaktnamen-Allowlist (Pfad-Traversal-Sperre) ──────────
    check(bb::valid_artifact_stem("best_lookup"), "stem: 'best_lookup' zulaessig");
    check(bb::valid_artifact_stem("Best-Binary_42"), "stem: alnum/_/- zulaessig");
    check(!bb::valid_artifact_stem(""), "stem: leer abgelehnt");
    check(!bb::valid_artifact_stem("../evil"), "stem: dotdot-Traversal abgelehnt");
    check(!bb::valid_artifact_stem("..\\evil"), "stem: Windows-Traversal abgelehnt");
    check(!bb::valid_artifact_stem("a/b"), "stem: Verzeichnistrenner abgelehnt");
    check(!bb::valid_artifact_stem("a\\b"), "stem: Backslash abgelehnt");
    check(!bb::valid_artifact_stem("/abs/path"), "stem: absoluter Pfad abgelehnt");
    check(!bb::valid_artifact_stem("C:evil"), "stem: Drive-Form abgelehnt (':' nicht in Allowlist)");
    check(!bb::valid_artifact_stem("name.dll"), "stem: '.' abgelehnt (keine Erweiterungs-Tricks)");
    check(!bb::valid_artifact_stem("CON"), "stem: reservierter Windows-Name CON abgelehnt");
    check(!bb::valid_artifact_stem("nul"), "stem: reservierter Windows-Name nul (case-insensitiv) abgelehnt");
    check(!bb::valid_artifact_stem("COM1"), "stem: reservierter Windows-Name COM1 abgelehnt");
    check(!bb::valid_artifact_stem("lpt9"), "stem: reservierter Windows-Name lpt9 abgelehnt");
    check(bb::valid_artifact_stem("CONSOLE"), "stem: CONSOLE zulaessig (nur exakte reservierte Namen)");
    check(!bb::valid_artifact_stem(std::string(121, 'a')), "stem: > kStemMax (120) abgelehnt");

    // ── (REV-DATA-05/06, WP-5 2026-07-16): ShippedArtifactBuilder E2E (tmp-Publish + Namens-Wache) ──
    {
        namespace fs       = std::filesystem;
        fs::path const  tb = fs::temp_directory_path() / "bb_selector_wp5_test";
        std::error_code ig;
        fs::remove_all(tb, ig);
        fs::path const src = tb / "src";
        fs::path const out = tb / "out";
        fs::create_directories(src, ig);
        {
            std::ofstream d{src / "perm.dll", std::ios::binary};
            d << "FAKE_DLL_BYTES";
            std::ofstream v{src / "perm.dll.version", std::ios::binary};
            v << "wp5-test-v1";
        }
        bb::RankedBinary const winner{"bin_ok", 110.0, 3};
        std::string            err;

        // Negativ: Traversal-Name wird VOR jedem Schreib-Effekt abgelehnt; out_dir bleibt unangelegt.
        bb::ShippedArtifactBuilder bad{out, "../evil"};
        auto const                 bad_art = bad.build(winner, bb::Metric::ns_per_op, src, err);
        check(!bad_art.has_value(), "builder: Traversal-Name '../evil' abgelehnt");
        check(!err.empty(), "builder: Ablehnung traegt Fehlermeldung");
        check(!fs::exists(tb / "evil.dll", ig) && !fs::exists(tb.parent_path() / "evil.dll", ig),
              "builder: keine Datei ausserhalb out_dir entstanden");

        // Positiv: atomarer Publish liefert DLL + Sidecar + Manifest; tmp-Verzeichnis ist aufgeraeumt.
        err.clear();
        bb::ShippedArtifactBuilder good{out, "best_test"};
        auto const                 art = good.build(winner, bb::Metric::ns_per_op, src, err);
        check(art.has_value(), ("builder: Publish erfolgreich (err='" + err + "')").c_str());
        if (art) {
            check(fs::exists(art->shipped_dll, ig), "builder: DLL ausgeliefert");
            check(fs::exists(out / "best_test.dll.version", ig), "builder: Version-Sidecar ausgeliefert");
            check(fs::exists(art->manifest_path, ig), "builder: Manifest ausgeliefert");
            check(art->dll_build_version == "wp5-test-v1", "builder: Sidecar-Inhalt uebernommen");
            check(fs::file_size(art->shipped_dll, ig) == fs::file_size(src / "perm.dll", ig),
                  "builder: DLL-Groesse == Quelle (Kopie verifiziert)");
            check(!fs::exists(out / ".tmp_publish_best_test", ig), "builder: tmp-Publish-Verzeichnis aufgeraeumt");
        }
        fs::remove_all(tb, ig);
    }

    std::printf(g_fail == 0 ? "BEST-BINARY PARSE+RANK: ALLE OK\n" : "BEST-BINARY PARSE+RANK: %d FAIL\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
