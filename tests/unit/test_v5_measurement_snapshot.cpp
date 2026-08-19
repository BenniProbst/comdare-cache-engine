// V5-I1-SUBSTANZ (Task #50) — ComdareMeasurementSnapshotV1: EIN autoritativer 16+6-Mess-POD.
//
// Beweist: der POD wird aus einem host-seitigen Lastprofil-Lauf (WorkloadRunResult) mit ECHTEN Observer-
// Daten befüllt (16 Performance/Meta + 6 funktionale Observer), die 6 HW-Counter ehrlich pmc_available=0;
// der kanonische Serializer schreibt die volle 16+6-Sicht UND eine pipeline-kompatible 16-col-Sicht.

#include "builder/measurement_snapshot.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

namespace b  = ::comdare::cache_engine::builder;
namespace wd = ::comdare::cache_engine::builder::workload_driver;

namespace {

wd::WorkloadRunResult make_result() {
    wd::WorkloadRunResult r;
    r.profile_name              = "YCSB_C_read_only";
    r.op_count                  = 300;
    r.total_ns                  = 1'500'000'000; // 300 ops / 1.5 s = 200 ops/s
    r.two_phase                 = true;
    r.lookup_ns                 = {100, 200, 300}; // merged p50 = 200
    r.observer.axis_stats[0][3] = 822;             // search_insert_count
    r.observer.axis_stats[0][0] = 1088;            // search_lookup_count
    r.observer.axis_stats[0][1] = 236;             // search_hit_count
    r.observer.axis_stats[0][2] = 852;             // search_miss_count
    r.observer.axis_stats[0][4] = 255;             // search_erase_count
    r.observer.axis_stats[0][5] = 157;             // search_peak_occupancy
    r.observer.axis_stats[6][0] = 4096;            // alloc_bytes_allocated
    r.observer.axis_stats[6][1] = 3376;            // alloc_bytes_in_use
    return r;
}

std::size_t count_cols(std::string const& csv_first_line) {
    return static_cast<std::size_t>(std::count(csv_first_line.begin(), csv_first_line.end(), ',') + 1);
}
std::string first_line(std::string const& s) { return s.substr(0, s.find('\n')); }

} // namespace

// Der POD wird mit ECHTEN Observer-Werten befüllt; HW-Counter ehrlich als nicht-verfügbar markiert.
TEST(V5MeasurementSnapshot, BuildsFromWorkloadResultWithRealObserverData) {
    auto const m = b::measurement_from_workload_result(make_result(), "ArtComposition");
    EXPECT_EQ(m.op_count, 300u);
    EXPECT_EQ(m.total_cycles, 200u); // merged p50 der Op-Latenzen
    EXPECT_EQ(m.search_insert, 822u);
    EXPECT_EQ(m.search_lookup, 1088u);
    EXPECT_EQ(m.search_hit, 236u);
    EXPECT_EQ(m.search_miss, 852u);
    EXPECT_EQ(m.search_peak_occupancy, 157u);
    EXPECT_EQ(m.bytes_allocated, 4096u);
    EXPECT_EQ(m.bytes_in_use_peak, 3376u);
    EXPECT_EQ(m.pmc_available, 0u);   // PMC NICHT angebunden — ehrlich, kein Schein-Datum
    EXPECT_EQ(m.cache_misses_l1, 0u); // HW-Counter 0 (nur gültig bei pmc_available==1)
    EXPECT_EQ(m.branch_misses, 0u);
    EXPECT_DOUBLE_EQ(m.throughput_ops_per_sec, 200.0);
    EXPECT_NE(m.fingerprint, 0u);
}

// Kanonischer Full-Serializer: 32 Spalten (16 Pipeline + 6 Observer + pmc_available + 2 Host-Messspalten
// + 7 PMC-Quell-Flags). NP-23 (#15-Bruch, 19.08.2026): die Flag-Spalten sind BEZIFFERT = SIEBEN, eine je
// im POD transportiertem PMC-Zaehler (6 HW-Counter + branch_misses). Sie stehen am ZEILENENDE hinter
// throughput_ops_per_sec -- die 25er-Altsicht ist ein echtes Praefix, kein zweites Schema.
TEST(V5MeasurementSnapshot, Serialize16Plus6FullView) {
    std::vector<b::ComdareMeasurementSnapshotV1> rows{
        b::measurement_from_workload_result(make_result(), "ArtComposition")};
    std::vector<std::string> ids{"ArtComposition_0"}, wls{"YCSB_C_read_only"};
    auto const               csv = b::serialize_measurements_csv(rows, ids, wls);
    auto const               hdr = first_line(csv);
    EXPECT_EQ(count_cols(hdr), 32u);                      // 25 Altsicht + 7 Quell-Flags (NP-23)
    EXPECT_NE(hdr.find("search_hit"), std::string::npos); // die „+6" sind drin
    EXPECT_NE(hdr.find("pmc_available"), std::string::npos);
    EXPECT_NE(hdr.find("branch_misses"), std::string::npos);
    EXPECT_NE(hdr.find("throughput_ops_per_sec"), std::string::npos);
    // NP-23: alle 7 Flag-Spalten namentlich im Header (Reihenfolge = POD-/Quell-Reihenfolge).
    EXPECT_NE(hdr.find("cache_misses_l1_source_available"), std::string::npos);
    EXPECT_NE(hdr.find("cache_misses_l2_source_available"), std::string::npos);
    EXPECT_NE(hdr.find("cache_misses_l3_source_available"), std::string::npos);
    EXPECT_NE(hdr.find("dtlb_misses_source_available"), std::string::npos);
    EXPECT_NE(hdr.find("coherence_invalidations_source_available"), std::string::npos);
    EXPECT_NE(hdr.find("energy_micro_joules_source_available"), std::string::npos);
    EXPECT_NE(hdr.find("branch_misses_source_available"), std::string::npos);
    // Ohne PMC-Quelle: throughput 200, dahinter 7 Flag-Nullen (echt-0 ist jetzt von keine-Quelle trennbar).
    EXPECT_NE(csv.find(",0,200,0,0,0,0,0,0,0\n"), std::string::npos);
    EXPECT_NE(csv.find("\nArtComposition_0,"), std::string::npos); // Datenzeile vorhanden
    EXPECT_EQ(std::count(csv.begin(), csv.end(), '\n'), 2);        // Header + 1 Datenzeile
}

TEST(V5MeasurementSnapshot, ThroughputUsesDivZeroGuard) {
    auto r       = make_result();
    r.total_ns   = 0;
    auto const m = b::measurement_from_workload_result(r, "ArtComposition");
    EXPECT_DOUBLE_EQ(m.throughput_ops_per_sec, 0.0);
}

// Pipeline-16-Sicht: exakt die 16 kanonischen Spalten, die Stufe 04/05 erwartet — speist die PDF.
TEST(V5MeasurementSnapshot, Pipeline16IsConsumableByStage04) {
    std::vector<b::ComdareMeasurementSnapshotV1> rows{
        b::measurement_from_workload_result(make_result(), "ArtComposition")};
    std::vector<std::string> ids{"ArtComposition_0"}, wls{"YCSB_C_read_only"};
    auto const               csv = b::serialize_measurements_pipeline16_csv(rows, ids, wls);
    auto const               hdr = first_line(csv);
    EXPECT_EQ(count_cols(hdr), 16u); // exakt 16 (Pipeline-kanonisch)
    EXPECT_EQ(hdr.rfind("permutation_id,fingerprint,succeeded,workload_used,op_count,total_cycles,", 0), 0u);
    EXPECT_NE(hdr.find("internal_frag"), std::string::npos);
}

// #26 PMC-Quelle: NullPmcSource meldet EHRLICH „nicht verfügbar" → pmc_available=0, HW-Spalten bleiben 0.
TEST(V5MeasurementSnapshot, NullPmcSourceReportsUnavailable) {
    ::comdare::cache_engine::measurement::NullPmcSource pmc;
    EXPECT_FALSE(pmc.available());
    pmc.begin();
    auto const c = pmc.end();
    EXPECT_FALSE(c.available);
    auto const m = b::measurement_from_workload_result(make_result(), "ArtComposition", c);
    EXPECT_EQ(m.pmc_available, 0u);
    EXPECT_EQ(m.cache_misses_l1, 0u);
    EXPECT_EQ(m.branch_misses, 0u);
    EXPECT_EQ(m.energy_micro_joules, 0u);
    // NP-23: ohne Quelle bleiben ALLE 7 POD-Quell-Flags 0 -- kein Schein-Beleg.
    EXPECT_EQ(m.cache_misses_l1_source_available, 0u);
    EXPECT_EQ(m.cache_misses_l2_source_available, 0u);
    EXPECT_EQ(m.cache_misses_l3_source_available, 0u);
    EXPECT_EQ(m.dtlb_misses_source_available, 0u);
    EXPECT_EQ(m.coherence_invalidations_source_available, 0u);
    EXPECT_EQ(m.energy_micro_joules_source_available, 0u);
    EXPECT_EQ(m.branch_misses_source_available, 0u);
}

// #26 PMC-Quelle: eine VERFÜGBARE Quelle (Mock) speist die 6 HW-Spalten + setzt pmc_available=1.
TEST(V5MeasurementSnapshot, AvailablePmcFillsHwColumns) {
    ::comdare::cache_engine::measurement::PmcCounters c;
    c.available               = true;
    c.cache_misses_l1         = 1111;
    c.cache_misses_l2         = 222;
    c.cache_misses_l3         = 33;
    c.dtlb_misses             = 7;
    c.branch_misses           = 17;
    c.coherence_invalidations = 5;
    c.energy_micro_joules     = 99000;
    // M-3a (2026-08-07): measurement_from_workload_result kopiert die flag-tragenden Zaehler nicht mehr
    // blind bei `available`, sondern verlangt je Zaehler den eigenen Quellen-Beleg (der Gegenfall steht
    // als eigene TU unten). Der Mock stellt den Beleg deshalb mit.
    c.cache_misses_l2_source_available         = true;
    c.cache_misses_l3_source_available         = true;
    c.branch_misses_source_available           = true;
    c.coherence_invalidations_source_available = true;
    c.energy_micro_joules_source_available     = true;
    // NP-23 (#15-Bruch): l1+dtlb tragen seit dem POD-Flag-Anbau denselben Einzel-Beleg wie die
    // uebrigen fuenf (B-5-Paritaet zur WIDE-Pipeline); der Mock stellt ihn wie bei M-3a mit.
    c.cache_misses_l1_source_available = true;
    c.dtlb_misses_source_available     = true;
    auto const m = b::measurement_from_workload_result(make_result(), "ArtComposition", c);
    EXPECT_EQ(m.pmc_available, 1u);
    EXPECT_EQ(m.cache_misses_l1, 1111u);
    EXPECT_EQ(m.cache_misses_l3, 33u);
    EXPECT_EQ(m.branch_misses, 17u);
    EXPECT_EQ(m.energy_micro_joules, 99000u);
    EXPECT_EQ(m.search_lookup, 1088u); // Observer-Daten unverändert daneben
    // NP-23: die 7 Quell-Flags reisen 1:1 in den POD (CSV kann echt-0 von keine-Quelle trennen).
    EXPECT_EQ(m.cache_misses_l1_source_available, 1u);
    EXPECT_EQ(m.cache_misses_l2_source_available, 1u);
    EXPECT_EQ(m.cache_misses_l3_source_available, 1u);
    EXPECT_EQ(m.dtlb_misses_source_available, 1u);
    EXPECT_EQ(m.coherence_invalidations_source_available, 1u);
    EXPECT_EQ(m.energy_micro_joules_source_available, 1u);
    EXPECT_EQ(m.branch_misses_source_available, 1u);
}

// M-3a (2026-08-07) -- KEINE BLINDE KOPIE MEHR (Pipeline B, die zweite CSV-Naht).
// Vorher uebernahm dieser Pfad bei `available` ALLE sieben Zaehler, ohne die feinkoernigen
// *_source_available zu befragen. Auf AMD Zen5 ist das der Regelfall: available==true (L1D oeffnete),
// waehrend cache_misses_l3 mangels generischem LL-Event beim POD-Default 0 bleibt.
//
// EHRLICHE EINORDNUNG (die TU soll nicht mehr behaupten, als sie zeigt): am heutigen Bestand setzen alle
// IPmcSource-Implementierungen Wert und Flag stets gemeinsam, ein Wert ohne Beleg entsteht nirgends. Diese
// TU haelt deshalb eine WACHE gegen kuenftige Drift fest -- eine neue Quelle, die einen Zaehler ohne
// Oeffnungs-Beleg setzt, darf ihn nicht bis in den Mess-POD durchreichen.
TEST(V5MeasurementSnapshot, PmcCountersWithoutOwnSourceAreNotCopied) {
    ::comdare::cache_engine::measurement::PmcCounters c;
    c.available       = true; // die ZEILE ist Messung (L1D lieferte) ...
    c.cache_misses_l1 = 1111;
    c.dtlb_misses     = 7;
    // ... aber diese Werte haben KEINEN Quellen-Beleg (alle *_source_available bleiben Default false).
    c.cache_misses_l2         = 222;
    c.cache_misses_l3         = 33;
    c.branch_misses           = 17;
    c.coherence_invalidations = 5;
    c.energy_micro_joules     = 99000;

    auto const m = b::measurement_from_workload_result(make_result(), "ArtComposition", c);
    EXPECT_EQ(m.pmc_available, 1u); // die grobkoernige Zeilen-Marke bleibt unveraendert
    // NP-23 (#15-Bruch): l1/dtlb tragen jetzt EIGENE Flags im POD -- ohne Quellen-Beleg werden auch
    // sie NICHT mehr uebernommen (vorher: an die Zeilen-Aussage gebunden und blind kopiert).
    EXPECT_EQ(m.cache_misses_l1, 0u);
    EXPECT_EQ(m.dtlb_misses, 0u);
    // Die fuenf flag-tragenden: NICHT uebernommen, bleiben POD-Default 0.
    EXPECT_EQ(m.cache_misses_l2, 0u);
    EXPECT_EQ(m.cache_misses_l3, 0u);
    EXPECT_EQ(m.branch_misses, 0u);
    EXPECT_EQ(m.coherence_invalidations, 0u);
    EXPECT_EQ(m.energy_micro_joules, 0u);
    // NP-23: und die POD-Flags sagen ehrlich "keine Quelle" -- alle sieben bleiben 0.
    EXPECT_EQ(m.cache_misses_l1_source_available, 0u);
    EXPECT_EQ(m.dtlb_misses_source_available, 0u);
    EXPECT_EQ(m.cache_misses_l2_source_available, 0u);
    EXPECT_EQ(m.cache_misses_l3_source_available, 0u);
    EXPECT_EQ(m.branch_misses_source_available, 0u);
    EXPECT_EQ(m.coherence_invalidations_source_available, 0u);
    EXPECT_EQ(m.energy_micro_joules_source_available, 0u);
}
