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
    r.profile_name = "YCSB_C_read_only";
    r.op_count     = 300;
    r.two_phase    = true;
    r.lookup_ns    = {100, 200, 300};   // merged p50 = 200
    r.observer.axis_stats[0][3] = 822;    // search_insert_count
    r.observer.axis_stats[0][0] = 1088;   // search_lookup_count
    r.observer.axis_stats[0][1] = 236;    // search_hit_count
    r.observer.axis_stats[0][2] = 852;    // search_miss_count
    r.observer.axis_stats[0][4] = 255;    // search_erase_count
    r.observer.axis_stats[0][5] = 157;    // search_peak_occupancy
    r.observer.axis_stats[6][0] = 4096;   // alloc_bytes_allocated
    r.observer.axis_stats[6][1] = 3376;   // alloc_bytes_in_use
    return r;
}

std::size_t count_cols(std::string const& csv_first_line) {
    return static_cast<std::size_t>(std::count(csv_first_line.begin(), csv_first_line.end(), ',') + 1);
}
std::string first_line(std::string const& s) { return s.substr(0, s.find('\n')); }

}  // namespace

// Der POD wird mit ECHTEN Observer-Werten befüllt; HW-Counter ehrlich als nicht-verfügbar markiert.
TEST(V5MeasurementSnapshot, BuildsFromWorkloadResultWithRealObserverData) {
    auto const m = b::measurement_from_workload_result(make_result(), "ArtComposition");
    EXPECT_EQ(m.op_count, 300u);
    EXPECT_EQ(m.total_cycles, 200u);              // merged p50 der Op-Latenzen
    EXPECT_EQ(m.search_insert, 822u);
    EXPECT_EQ(m.search_lookup, 1088u);
    EXPECT_EQ(m.search_hit, 236u);
    EXPECT_EQ(m.search_miss, 852u);
    EXPECT_EQ(m.search_peak_occupancy, 157u);
    EXPECT_EQ(m.bytes_allocated, 4096u);
    EXPECT_EQ(m.bytes_in_use_peak, 3376u);
    EXPECT_EQ(m.pmc_available, 0u);               // PMC NICHT angebunden — ehrlich, kein Schein-Datum
    EXPECT_EQ(m.cache_misses_l1, 0u);             // HW-Counter 0 (nur gültig bei pmc_available==1)
    EXPECT_NE(m.fingerprint, 0u);
}

// Kanonischer 16+6-Serializer: 23 Spalten (16 Pipeline + 6 Observer + pmc_available), eine Datenzeile.
TEST(V5MeasurementSnapshot, Serialize16Plus6FullView) {
    std::vector<b::ComdareMeasurementSnapshotV1> rows{
        b::measurement_from_workload_result(make_result(), "ArtComposition")};
    std::vector<std::string> ids{"ArtComposition_0"}, wls{"YCSB_C_read_only"};
    auto const csv = b::serialize_measurements_csv(rows, ids, wls);
    auto const hdr = first_line(csv);
    EXPECT_EQ(count_cols(hdr), 23u);                                  // 16 + 6 + pmc_available
    EXPECT_NE(hdr.find("search_hit"), std::string::npos);            // die „+6" sind drin
    EXPECT_NE(hdr.find("pmc_available"), std::string::npos);
    EXPECT_NE(csv.find("\nArtComposition_0,"), std::string::npos);   // Datenzeile vorhanden
    EXPECT_EQ(std::count(csv.begin(), csv.end(), '\n'), 2);          // Header + 1 Datenzeile
}

// Pipeline-16-Sicht: exakt die 16 kanonischen Spalten, die Stufe 04/05 erwartet — speist die PDF.
TEST(V5MeasurementSnapshot, Pipeline16IsConsumableByStage04) {
    std::vector<b::ComdareMeasurementSnapshotV1> rows{
        b::measurement_from_workload_result(make_result(), "ArtComposition")};
    std::vector<std::string> ids{"ArtComposition_0"}, wls{"YCSB_C_read_only"};
    auto const csv = b::serialize_measurements_pipeline16_csv(rows, ids, wls);
    auto const hdr = first_line(csv);
    EXPECT_EQ(count_cols(hdr), 16u);                                  // exakt 16 (Pipeline-kanonisch)
    EXPECT_EQ(hdr.rfind("permutation_id,fingerprint,succeeded,workload_used,op_count,total_cycles,", 0), 0u);
    EXPECT_NE(hdr.find("internal_frag"), std::string::npos);
}

// #26 PMC-Quelle: NullPmcSource meldet EHRLICH „nicht verfügbar" → pmc_available=0, HW-Spalten bleiben 0.
TEST(V5MeasurementSnapshot, NullPmcSourceReportsUnavailable) {
    b::NullPmcSource pmc;
    EXPECT_FALSE(pmc.available());
    pmc.begin();
    auto const c = pmc.end();
    EXPECT_FALSE(c.available);
    auto const m = b::measurement_from_workload_result(make_result(), "ArtComposition", c);
    EXPECT_EQ(m.pmc_available, 0u);
    EXPECT_EQ(m.cache_misses_l1, 0u);
    EXPECT_EQ(m.energy_micro_joules, 0u);
}

// #26 PMC-Quelle: eine VERFÜGBARE Quelle (Mock) speist die 6 HW-Spalten + setzt pmc_available=1.
TEST(V5MeasurementSnapshot, AvailablePmcFillsHwColumns) {
    b::PmcCounters c;
    c.available = true;
    c.cache_misses_l1 = 1111; c.cache_misses_l2 = 222; c.cache_misses_l3 = 33;
    c.dtlb_misses = 7; c.coherence_invalidations = 5; c.energy_micro_joules = 99000;
    auto const m = b::measurement_from_workload_result(make_result(), "ArtComposition", c);
    EXPECT_EQ(m.pmc_available, 1u);
    EXPECT_EQ(m.cache_misses_l1, 1111u);
    EXPECT_EQ(m.cache_misses_l3, 33u);
    EXPECT_EQ(m.energy_micro_joules, 99000u);
    EXPECT_EQ(m.search_lookup, 1088u);   // Observer-Daten unverändert daneben
}
