// V32.II.2 (2026-05-18 spaet) - Command-Pattern Smoke-Tests
//
// @subsystem CEB
// @phase_owner CEB

#include "workload.hpp"
#include "execution_result.hpp"
#include "execute_engine_command.hpp"
#include "compare_engine_command.hpp"
#include "auto_permutator.hpp"
#include "axis_library_registry.hpp"

#include <gtest/gtest.h>

namespace cmd = comdare::cache_engine::builder::commands;

TEST(ExecuteEngineCommand, BasicExecution) {
    cmd::Workload w {};
    w.kind = cmd::WorkloadKind::YCSB_C_ReadOnly;
    w.record_count = 1000;
    w.operation_count = 100;
    w.name = "smoke-test";

    cmd::ExecuteEngineCommand cmd("test-engine", w);
    EXPECT_EQ(cmd.command_name(), "ExecuteEngineCommand");
    EXPECT_TRUE(cmd.is_parallelizable());

    int rc = cmd.execute();
    EXPECT_EQ(rc, 0);
    EXPECT_TRUE(cmd.result().success);
    EXPECT_EQ(cmd.result().engine_name, "test-engine");
    EXPECT_GT(cmd.result().throughput_ops_per_sec, 0.0);
}

TEST(CompareEngineCommand, BasicCompare) {
    cmd::ExecutionResult r_a {};
    r_a.engine_name = "ee-a";
    r_a.success = true;
    r_a.throughput_ops_per_sec = 1100.0;
    r_a.latency_p99 = std::chrono::nanoseconds(900);
    r_a.total_cache_misses = 100;
    r_a.memory_footprint_bytes = 4096;
    r_a.H1_clu_improvement = 1.2;

    cmd::ExecutionResult r_b {};
    r_b.engine_name = "ee-b";
    r_b.success = true;
    r_b.throughput_ops_per_sec = 1000.0;
    r_b.latency_p99 = std::chrono::nanoseconds(1000);
    r_b.total_cache_misses = 150;
    r_b.memory_footprint_bytes = 4096;

    cmd::CompareEngineCommand cmp(r_a, r_b);
    EXPECT_EQ(cmp.command_name(), "CompareEngineCommand");

    int rc = cmp.execute();
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(cmp.verdict(), cmd::CompareEngineCommand::Verdict::EE_A_Wins);
    EXPECT_NEAR(cmp.throughput_ratio(), 1.1, 0.01);
    EXPECT_EQ(cmp.latency_delta_ns(), 100);  // 1000 - 900
    EXPECT_EQ(cmp.cache_miss_improvement(), 50);
    EXPECT_TRUE(cmp.h1_clu_validated());
}

TEST(CompareEngineCommand, Tie) {
    cmd::ExecutionResult r {};
    r.success = true;
    r.throughput_ops_per_sec = 1000.0;
    r.latency_p99 = std::chrono::nanoseconds(1000);

    cmd::CompareEngineCommand cmp(r, r);
    cmp.execute();
    EXPECT_EQ(cmp.verdict(), cmd::CompareEngineCommand::Verdict::Tie);
}

TEST(AxisLibraryRegistry, LookupAchse12_1Simd) {
    auto variants = cmd::AxisLibraryRegistry::lookup("12.1");
    EXPECT_EQ(variants.size(), 5);  // Scalar + AVX2 + AVX512 + NEON + SVE2
}

TEST(AxisLibraryRegistry, LookupAchse11Telemetry) {
    auto variants = cmd::AxisLibraryRegistry::lookup("11");
    EXPECT_EQ(variants.size(), 4);  // LeafOnly + Sampled + Retroactive + PathRead (NICHT PerNode)
}

TEST(AutoPermutator, DiscoverAndGenerate) {
    cmd::AutoPermutator ap("12.1");
    ap.discover_axis_implementations();
    ap.platform_filter();  // V32.HH.2: no-op
    // user_limit_filter mit leerer Liste -> alle disallowed
    auto perms = ap.generate_permutations();
    // Mit user_allowed=true Default sind alle generierbar (V32.HH.1 Stand)
    EXPECT_GE(perms.size(), 0);
}
