// Tests fuer WorkloadGenerator + ResultAggregator + ExperimentDemo (Phase 7)

#include <comdare/workload_generator/workload_generator.hpp>
#include <comdare/experiment/result_aggregator.hpp>
#include <comdare/experiment/experiment_demo.hpp>

#include <cache_engine/allocators/families/a01_hoard/hoard_adapter.hpp>
#include <cache_engine/allocators/families/a04_mimalloc/mimalloc_adapter.hpp>
#include <cache_engine/allocators/families/a06_tcmalloc/tcmalloc_adapter.hpp>

#include <gtest/gtest.h>

#include <filesystem>

namespace wg = comdare::workload_generator;
namespace expt = comdare::experiment;
namespace fam = comdare::cache_engine::allocator::families;

// ─────────────────────────────────────────────────────────────────────────────
// WorkloadGenerator
// ─────────────────────────────────────────────────────────────────────────────

TEST(WorkloadGenerator, ConfigConstructible) {
    wg::WorkloadConfig cfg;
    cfg.num_keys       = 1000;
    cfg.num_operations = 5000;
    wg::WorkloadGenerator gen{cfg};
    EXPECT_EQ(gen.config().num_keys, 1000u);
    EXPECT_EQ(gen.config().num_operations, 5000u);
}

TEST(WorkloadGenerator, UniformReadsCorrectCount) {
    wg::WorkloadConfig cfg;
    cfg.num_keys       = 100;
    cfg.num_operations = 1000;
    wg::WorkloadGenerator gen{cfg};
    auto ops = gen.generate_uniform_reads();
    EXPECT_EQ(ops.size(), 1000u);
    for (auto const& op : ops) {
        EXPECT_EQ(op.op, wg::OperationKind::Read);
        EXPECT_LT(op.key_id, 100u);
    }
}

TEST(WorkloadGenerator, ZipfianReadsCorrectCount) {
    wg::WorkloadConfig cfg;
    cfg.num_keys       = 1000;
    cfg.num_operations = 2000;
    wg::WorkloadGenerator gen{cfg};
    auto ops = gen.generate_zipfian_reads();
    EXPECT_EQ(ops.size(), 2000u);
    for (auto const& op : ops) {
        EXPECT_EQ(op.op, wg::OperationKind::Read);
        EXPECT_LT(op.key_id, 1000u);
    }
}

TEST(WorkloadGenerator, SequentialReadsAreCyclic) {
    wg::WorkloadConfig cfg;
    cfg.num_keys       = 10;
    cfg.num_operations = 30;
    wg::WorkloadGenerator gen{cfg};
    auto ops = gen.generate_sequential_reads();
    ASSERT_EQ(ops.size(), 30u);
    EXPECT_EQ(ops[0].key_id, 0u);
    EXPECT_EQ(ops[9].key_id, 9u);
    EXPECT_EQ(ops[10].key_id, 0u);  // wrap
    EXPECT_EQ(ops[20].key_id, 0u);
}

TEST(WorkloadGenerator, YcsbAHasReadsAndUpdates) {
    wg::WorkloadConfig cfg;
    cfg.num_keys       = 100;
    cfg.num_operations = 1000;
    wg::WorkloadGenerator gen{cfg};
    auto ops = gen.generate_ycsb(wg::YcsbWorkload::A);
    EXPECT_EQ(ops.size(), 1000u);

    int reads = 0, updates = 0;
    for (auto const& op : ops) {
        if (op.op == wg::OperationKind::Read) ++reads;
        if (op.op == wg::OperationKind::Update) ++updates;
    }
    EXPECT_GT(reads, 300);     // ~50% +/- variance
    EXPECT_GT(updates, 300);
}

TEST(WorkloadGenerator, YcsbCAllReads) {
    wg::WorkloadConfig cfg;
    cfg.num_keys       = 100;
    cfg.num_operations = 500;
    wg::WorkloadGenerator gen{cfg};
    auto ops = gen.generate_ycsb(wg::YcsbWorkload::C);
    for (auto const& op : ops) {
        EXPECT_EQ(op.op, wg::OperationKind::Read);
    }
}

TEST(WorkloadGenerator, AbiDescriptorMatchesOps) {
    wg::WorkloadConfig cfg;
    cfg.num_keys       = 100;
    cfg.num_operations = 200;
    wg::WorkloadGenerator gen{cfg};
    auto ops = gen.generate_uniform_reads();
    auto d = gen.to_abi_descriptor(std::span<wg::Operation const>{ops});
    EXPECT_EQ(d.version, static_cast<std::uint32_t>(COMDARE_ABI_VERSION));
    EXPECT_EQ(d.num_operations, 200u);
    EXPECT_EQ(d.dataset_bytes, 200u * sizeof(wg::Operation));
    EXPECT_EQ(d.dataset_ptr, ops.data());
}

// ─────────────────────────────────────────────────────────────────────────────
// ResultAggregator
// ─────────────────────────────────────────────────────────────────────────────

TEST(ResultAggregator, EmptyInitially) {
    expt::ResultAggregator agg;
    EXPECT_EQ(agg.result_count(), 0u);
}

TEST(ResultAggregator, AddsResults) {
    expt::ResultAggregator agg;
    expt::PermutationResult r;
    r.permutation_id = "test1";
    r.succeeded      = true;
    r.record.op_count    = 1000;
    r.record.total_cycles = 10000;
    agg.add(r);
    EXPECT_EQ(agg.result_count(), 1u);
}

TEST(ResultAggregator, CompareAgainstBaseline) {
    expt::ResultAggregator agg;

    expt::PermutationResult baseline;
    baseline.permutation_id     = "baseline";
    baseline.succeeded          = true;
    baseline.record.op_count    = 1000;
    baseline.record.total_cycles = 10000;
    baseline.record.bytes_in_use_peak = 1024 * 1024;
    agg.add(baseline);

    expt::PermutationResult faster;
    faster.permutation_id     = "candidate";
    faster.succeeded          = true;
    faster.record.op_count    = 1000;
    faster.record.total_cycles = 5000;          // 2x schneller
    faster.record.bytes_in_use_peak = 2 * 1024 * 1024;  // 2x mehr Memory
    agg.add(faster);

    agg.set_baseline("baseline");
    auto reports = agg.compare_against_baseline();
    ASSERT_EQ(reports.size(), 1u);
    EXPECT_DOUBLE_EQ(reports[0].throughput_speedup, 2.0);
    EXPECT_DOUBLE_EQ(reports[0].memory_ratio, 2.0);
}

TEST(ResultAggregator, ExportCsv) {
    expt::ResultAggregator agg;
    expt::PermutationResult r;
    r.permutation_id = "test_perm";
    r.fingerprint    = 0xCAFE;
    r.succeeded      = true;
    r.record.op_count = 100;
    agg.add(r);

    auto path = std::filesystem::temp_directory_path() / "comdare_agg_test.csv";
    agg.export_csv(path);
    EXPECT_TRUE(std::filesystem::exists(path));
    std::filesystem::remove(path);
}

TEST(ResultAggregator, ExportJson) {
    expt::ResultAggregator agg;
    agg.set_baseline("base");
    expt::PermutationResult r;
    r.permutation_id = "test";
    r.succeeded      = true;
    agg.add(r);

    auto path = std::filesystem::temp_directory_path() / "comdare_agg_test.json";
    agg.export_json(path);
    EXPECT_TRUE(std::filesystem::exists(path));
    std::filesystem::remove(path);
}

// ─────────────────────────────────────────────────────────────────────────────
// ExperimentDemo End-to-End (Phase 7.4)
// ─────────────────────────────────────────────────────────────────────────────

TEST(ExperimentDemo, RunSingleAgainstHoard) {
    wg::WorkloadConfig cfg;
    cfg.num_keys         = 100;
    cfg.num_operations   = 500;
    cfg.value_size_bytes = 64;
    wg::WorkloadGenerator gen{cfg};
    auto ops = gen.generate_ycsb(wg::YcsbWorkload::A);

    fam::a01_hoard::HoardAdapter<> alloc;
    auto result = expt::run_single_experiment(
        "hoard_demo", 0xDEAD, alloc,
        std::span<wg::Operation const>{ops}, cfg.value_size_bytes);

    EXPECT_TRUE(result.succeeded);
    EXPECT_EQ(result.record.op_count, 500u);
    EXPECT_GT(result.record.total_cycles, 0u);
}

TEST(ExperimentDemo, CompareHoardVsMimallocVsTcmalloc) {
    wg::WorkloadConfig cfg;
    cfg.num_keys         = 100;
    cfg.num_operations   = 1000;
    cfg.value_size_bytes = 128;
    wg::WorkloadGenerator gen{cfg};
    auto ops = gen.generate_ycsb(wg::YcsbWorkload::A);

    expt::ResultAggregator agg;

    fam::a01_hoard::HoardAdapter<>          hoard;
    fam::a04_mimalloc::MimallocAdapter<>    mimal;
    fam::a06_tcmalloc::TcmallocAdapter<>    tcmal;

    agg.add(expt::run_single_experiment("hoard", 1, hoard,
        std::span<wg::Operation const>{ops}, cfg.value_size_bytes));
    agg.add(expt::run_single_experiment("mimalloc", 2, mimal,
        std::span<wg::Operation const>{ops}, cfg.value_size_bytes));
    agg.add(expt::run_single_experiment("tcmalloc", 3, tcmal,
        std::span<wg::Operation const>{ops}, cfg.value_size_bytes));

    EXPECT_EQ(agg.result_count(), 3u);

    agg.set_baseline("hoard");
    auto reports = agg.compare_against_baseline();
    EXPECT_EQ(reports.size(), 2u);   // mimalloc + tcmalloc gegen hoard

    // CSV/JSON-Export funktioniert
    auto csv_path  = std::filesystem::temp_directory_path() / "comdare_demo.csv";
    auto json_path = std::filesystem::temp_directory_path() / "comdare_demo.json";
    agg.export_csv(csv_path);
    agg.export_json(json_path);
    EXPECT_TRUE(std::filesystem::exists(csv_path));
    EXPECT_TRUE(std::filesystem::exists(json_path));
    std::filesystem::remove(csv_path);
    std::filesystem::remove(json_path);
}
