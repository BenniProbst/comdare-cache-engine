// Tests fuer Mikrobenchmark-Suite (Phase 6.6)

#include <comdare/benchmark_suite/custom_allocation_1_measurements.hpp>
#include <comdare/benchmark_suite/custom_allocation_2_state_log.hpp>
#include <comdare/benchmark_suite/benchmark_runner.hpp>
#include <comdare/benchmark_suite/binary_blob_writer.hpp>
#include <comdare/benchmark_suite/conversion/binary_to_csv.hpp>
#include <comdare/benchmark_suite/conversion/binary_to_json.hpp>
#include <comdare/benchmark_suite/conversion/binary_to_tikz.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <vector>

namespace bs = comdare::benchmark_suite;

// ─────────────────────────────────────────────────────────────────────────────
// CustomAllocation1 (32B Records, gross genug, nie fail)
// ─────────────────────────────────────────────────────────────────────────────

TEST(CustomAllocation1, AppendAndSnapshot) {
    bs::CustomAllocation1 alloc{1024 * 32}; // 1024 records
    EXPECT_EQ(alloc.records_used(), 0u);

    bs::MeasurementRecord32 r{};
    r.timestamp_ns = 12345;
    r.op_id        = 1;
    auto slot      = alloc.append(r);
    EXPECT_EQ(slot, 0u);
    EXPECT_EQ(alloc.records_used(), 1u);

    auto snap = alloc.snapshot();
    EXPECT_EQ(snap.size(), 1u);
    EXPECT_EQ(snap[0].timestamp_ns, 12345u);
}

TEST(CustomAllocation1, RecordSize32) {
    EXPECT_EQ(sizeof(bs::MeasurementRecord32), 32u);
    EXPECT_EQ(alignof(bs::MeasurementRecord32), 32u);
}

TEST(CustomAllocation1, ManyAppendsLockless) {
    bs::CustomAllocation1 alloc{1024 * 32};
    for (int i = 0; i < 100; ++i) {
        bs::MeasurementRecord32 r{};
        r.op_id = static_cast<std::uint64_t>(i);
        alloc.append(r);
    }
    EXPECT_EQ(alloc.records_used(), 100u);
}

// ─────────────────────────────────────────────────────────────────────────────
// CustomAllocation2 (Sparse Byte States)
// ─────────────────────────────────────────────────────────────────────────────

TEST(CustomAllocation2, PushStateAndSnapshot) {
    bs::CustomAllocation2 alloc{4096};
    EXPECT_EQ(alloc.bytes_used(), 0u);

    std::array<std::byte, 5> delta{std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF}, std::byte{0x42}};
    EXPECT_TRUE(alloc.push_state(0xAA, std::span<std::byte const>{delta}));
    EXPECT_GT(alloc.bytes_used(), 5u);
}

// ─────────────────────────────────────────────────────────────────────────────
// BenchmarkRunner (No-Deprecate Wrapper)
// ─────────────────────────────────────────────────────────────────────────────

TEST(BenchmarkRunner, BeginRecordEnd) {
    bs::BenchmarkRunner runner{1024 * 32, 4096};

    auto h = runner.begin_measurement("phase1");
    runner.record_event(h, bs::EventKind::CacheMiss, 42);
    runner.record_event(h, bs::EventKind::Allocation, 128);
    runner.end_measurement(h, 12345);

    // 4 records: begin, cache_miss, allocation, end
    EXPECT_EQ(runner.records_collected(), 4u);
}

TEST(BenchmarkRunner, MultiplePhases) {
    bs::BenchmarkRunner runner{1024 * 32, 4096};

    for (int p = 0; p < 5; ++p) {
        auto h = runner.begin_measurement("phase");
        for (int e = 0; e < 10; ++e) { runner.record_event(h, bs::EventKind::Custom, e); }
        runner.end_measurement(h, p);
    }
    // 5 phases * (1 begin + 10 events + 1 end) = 60
    EXPECT_EQ(runner.records_collected(), 60u);
}

TEST(BenchmarkRunner, SparseStateLog) {
    bs::BenchmarkRunner runner{1024 * 32, 4096};

    std::array<std::byte, 8> delta;
    delta.fill(std::byte{0x55});
    runner.log_sparse_state(0x01, std::span<std::byte const>{delta});
    runner.log_sparse_state(0x02, std::span<std::byte const>{delta});

    EXPECT_GT(runner.state_log_bytes(), 16u);
}

// ─────────────────────────────────────────────────────────────────────────────
// BinaryBlobWriter (End-of-Experiment Konsolidierung)
// ─────────────────────────────────────────────────────────────────────────────

TEST(BinaryBlobWriter, WritesValidBlob) {
    bs::BenchmarkRunner runner{1024 * 32, 4096};
    auto                h = runner.begin_measurement("test");
    runner.end_measurement(h, 999);

    auto tmp_dir = std::filesystem::temp_directory_path();
    auto path    = tmp_dir / "comdare_test_blob.cdb";

    runner.flush_to_binary_blob(path);
    EXPECT_TRUE(std::filesystem::exists(path));
    EXPECT_GT(std::filesystem::file_size(path), 24u); // header alone

    std::filesystem::remove(path);
}

// ─────────────────────────────────────────────────────────────────────────────
// Conversion-Routinen (POST-EXPERIMENT NUR!)
// ─────────────────────────────────────────────────────────────────────────────

TEST(ConversionRoutines, BinaryToCsv) {
    std::vector<bs::MeasurementRecord32> records(3);
    for (int i = 0; i < 3; ++i) {
        records[i].timestamp_ns    = i * 1000;
        records[i].op_id           = i;
        records[i].cycles_or_value = i * i;
    }
    bs::conversion::BinaryToCsv conv;
    auto                        path = std::filesystem::temp_directory_path() / "comdare_test.csv";
    conv.convert(std::span<bs::MeasurementRecord32 const>{records}, path);
    EXPECT_TRUE(std::filesystem::exists(path));
    std::filesystem::remove(path);
}

TEST(ConversionRoutines, BinaryToJson) {
    std::vector<bs::MeasurementRecord32> records(2);
    bs::conversion::BinaryToJson         conv;
    auto                                 path = std::filesystem::temp_directory_path() / "comdare_test.json";
    conv.convert(std::span<bs::MeasurementRecord32 const>{records}, path);
    EXPECT_TRUE(std::filesystem::exists(path));
    std::filesystem::remove(path);
}

TEST(ConversionRoutines, BinaryToTikz) {
    std::vector<bs::MeasurementRecord32> records(2);
    records[0].cycles_or_value = 100;
    records[1].cycles_or_value = 200;
    bs::conversion::BinaryToTikz conv;
    auto                         path = std::filesystem::temp_directory_path() / "comdare_test.tikz";
    conv.convert(std::span<bs::MeasurementRecord32 const>{records}, path);
    EXPECT_TRUE(std::filesystem::exists(path));
    std::filesystem::remove(path);
}
