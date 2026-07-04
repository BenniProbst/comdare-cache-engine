// AP-8/#242 -- HdrHistogram_c wrapper + p95 host serializers.

#include <latency_hdr_histogram.hpp>

#include <builder/anatomy_commands/tier_observe_trace_abi.hpp>
#include <builder/commands/latency_stats.hpp>
#include <builder/measurement_snapshot.hpp>
#include <builder/workload_driver/workload_orchestrator.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <span>
#include <string>
#include <vector>

namespace measurement = ::comdare::cache_engine::measurement;
namespace stats       = ::comdare::cache_engine::builder::commands::stats;
namespace builder     = ::comdare::cache_engine::builder;
namespace workload    = ::comdare::cache_engine::builder::workload_driver;
namespace trace_abi   = ::comdare::cache_engine::builder::anatomy_commands;

namespace {

std::span<std::int64_t const> as_span(std::vector<std::int64_t> const& values) {
    return {values.data(), values.size()};
}

std::string first_line(std::string const& s) {
    return s.substr(0, s.find('\n'));
}

std::size_t count_cols(std::string const& csv_first_line) {
    return static_cast<std::size_t>(std::count(csv_first_line.begin(), csv_first_line.end(), ',') + 1);
}

void expect_hdr_near_ref(std::int64_t actual, std::int64_t ref) {
    EXPECT_NEAR(static_cast<double>(actual), static_cast<double>(ref), static_cast<double>(ref) * 0.01);
}

workload::WorkloadRunResult make_workload_result() {
    workload::WorkloadRunResult r;
    r.profile_name              = "AP8_profile";
    r.op_count                  = 24;
    r.total_ns                  = 2400;
    r.two_phase                 = true;
    r.insert_ns                 = {10, 20, 30, 40};
    r.lookup_ns                 = {11, 21, 31, 41};
    r.erase_ns                  = {12, 22, 32, 42};
    r.clear_ns                  = {13, 23, 33, 43};
    r.scan_ns                   = {14, 24, 34, 44};
    r.rmw_ns                    = {15, 25, 35, 45};
    r.observer.axis_stats[0][3] = 4;
    r.observer.axis_stats[0][0] = 5;
    r.observer.axis_stats[0][1] = 3;
    r.observer.axis_stats[0][2] = 2;
    r.observer.axis_stats[0][4] = 1;
    r.observer.axis_stats[0][5] = 6;
    r.observer.axis_stats[6][1] = 128;
    r.observer.axis_stats[6][2] = 7;
    r.observer.observable_axis_count = 17;
    return r;
}

} // namespace

TEST(AP8HdrHistogram, MatchesNearestRankWithinHdrPrecision) {
    std::vector<std::int64_t> ns(1000);
    std::iota(ns.begin(), ns.end(), 1);

    auto const hdr = measurement::LatencyHdrHistogram::from_samples(as_span(ns));

    std::int64_t const ref_p50 = stats::percentile_ns(as_span(ns), 0.50).count();
    std::int64_t const ref_p95 = stats::percentile_ns(as_span(ns), 0.95).count();
    std::int64_t const ref_p99 = stats::percentile_ns(as_span(ns), 0.99).count();

    EXPECT_EQ(hdr.count(), static_cast<std::int64_t>(ns.size()));
    expect_hdr_near_ref(hdr.p50(), ref_p50);
    expect_hdr_near_ref(hdr.p95(), ref_p95);
    expect_hdr_near_ref(hdr.p99(), ref_p99);
}

TEST(AP8HdrHistogram, DeterministicForSameSequence) {
    std::vector<std::int64_t> const ns{7, 11, 13, 17, 19, 23, 29, 31, 10'000, 20'000, 30'000};

    auto const a = measurement::LatencyHdrHistogram::from_samples(as_span(ns));
    auto const b = measurement::LatencyHdrHistogram::from_samples(as_span(ns));

    EXPECT_EQ(a.p50(), b.p50());
    EXPECT_EQ(a.p95(), b.p95());
    EXPECT_EQ(a.p99(), b.p99());
}

TEST(AP8HdrHistogram, EmptyHistogramReturnsZero) {
    measurement::LatencyHdrHistogram hdr;

    EXPECT_EQ(hdr.count(), 0);
    EXPECT_EQ(hdr.p50(), 0);
    EXPECT_EQ(hdr.p95(), 0);
    EXPECT_EQ(hdr.p99(), 0);
    EXPECT_EQ(hdr.min(), 0);
    EXPECT_EQ(hdr.max(), 0);
    EXPECT_DOUBLE_EQ(hdr.mean(), 0.0);
}

TEST(AP8HdrHistogram, P95ColumnsPresentAndPipeline16StaysSixteenColumns) {
    auto const r = make_workload_result();

    std::string const workload_csv = workload::serialize_workload_run_results_csv({r});
    EXPECT_NE(workload_csv.find("insert_p95_ns"), std::string::npos);
    EXPECT_NE(workload_csv.find("lookup_p95_ns"), std::string::npos);
    EXPECT_NE(workload_csv.find("erase_p95_ns"), std::string::npos);
    EXPECT_NE(workload_csv.find("clear_p95_ns"), std::string::npos);
    EXPECT_NE(workload_csv.find("scan_p95_ns"), std::string::npos);
    EXPECT_NE(workload_csv.find("rmw_p95_ns"), std::string::npos);

    trace_abi::AbiTierObserveTrace trace;
    trace_abi::AbiFillLevelSnapshot cp;
    cp.fill_level = 4;
    cp.write_ns   = {10, 20, 30, 40};
    cp.read_ns    = {11, 21, 31, 41};
    cp.delete_ns  = {12, 22, 32, 42};
    trace.checkpoints.push_back(cp);

    std::string const json = trace_abi::serialize_abi_tier_trace_json(trace);
    EXPECT_NE(json.find("\"write_p95_ns\""), std::string::npos);
    EXPECT_NE(json.find("\"read_p95_ns\""), std::string::npos);
    EXPECT_NE(json.find("\"delete_p95_ns\""), std::string::npos);

    std::vector<builder::ComdareMeasurementSnapshotV1> rows{
        builder::measurement_from_workload_result(r, "AP8Composition")};
    std::string const pipeline16 = builder::serialize_measurements_pipeline16_csv(rows, {"AP8Composition_0"}, {r.profile_name});
    EXPECT_EQ(count_cols(first_line(pipeline16)), 16u);
    EXPECT_EQ(pipeline16.find("_p95_ns"), std::string::npos);
}