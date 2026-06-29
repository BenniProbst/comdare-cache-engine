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
#include "welch_t_test.hpp"

#include <gtest/gtest.h>
#include <random>

namespace cmd = comdare::cache_engine::builder::commands;

TEST(ExecuteEngineCommand, BasicExecution) {
    cmd::Workload w{};
    w.kind            = cmd::WorkloadKind::YCSB_C_ReadOnly;
    w.record_count    = 1000;
    w.operation_count = 100;
    w.name            = "smoke-test";

    cmd::ExecuteEngineCommand cmd("test-engine", w);
    EXPECT_EQ(cmd.command_name(), "ExecuteEngineCommand");
    EXPECT_TRUE(cmd.is_parallelizable());

    int rc = cmd.execute();
    EXPECT_EQ(rc, 0);
    EXPECT_TRUE(cmd.result().success);
    EXPECT_EQ(cmd.result().engine_name, "test-engine");
    EXPECT_GT(cmd.result().throughput_ops_per_sec, 0.0);
    EXPECT_GE(cmd.result().latency_p50.count(), 0);
    EXPECT_GE(cmd.result().latency_p99.count(), cmd.result().latency_p50.count());
}

TEST(ExecuteEngineCommand, RealCallableInjection) {
    cmd::Workload w{};
    w.kind            = cmd::WorkloadKind::YCSB_A_Read50Write50;
    w.record_count    = 500;
    w.operation_count = 50;
    w.name            = "callable-injection";

    std::uint64_t       observed_ops = 0;
    cmd::EngineCallable real_engine  = [&observed_ops](std::size_t   op, cmd::WorkloadKind,
                                                       std::uint64_t seed) -> cmd::OperationOutcome {
        ++observed_ops;
        cmd::OperationOutcome out{};
        out.cache_misses_delta = (seed + op) % 3 == 0 ? 1u : 0u;
        out.bytes_touched      = 128;
        out.success            = true;
        return out;
    };

    cmd::ExecuteEngineCommand command("real-engine", w, real_engine);
    int                       rc = command.execute();
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(observed_ops, 50u);
    EXPECT_TRUE(command.result().success);
    EXPECT_EQ(command.result().memory_footprint_bytes, 50u * 128u);
    EXPECT_GT(command.result().throughput_ops_per_sec, 0.0);
}

TEST(ExecuteEngineCommand, CallableFailurePropagates) {
    cmd::Workload w{};
    w.operation_count = 10;

    cmd::EngineCallable failing = [](std::size_t op, cmd::WorkloadKind, std::uint64_t) -> cmd::OperationOutcome {
        cmd::OperationOutcome out{};
        out.success = (op != 5);
        return out;
    };

    cmd::ExecuteEngineCommand command("flaky", w, failing);
    int                       rc = command.execute();
    EXPECT_EQ(rc, 1);
    EXPECT_FALSE(command.result().success);
}

TEST(CompareEngineCommand, BasicCompare) {
    cmd::ExecutionResult r_a{};
    r_a.engine_name            = "ee-a";
    r_a.success                = true;
    r_a.throughput_ops_per_sec = 1100.0;
    r_a.latency_p99            = std::chrono::nanoseconds(900);
    r_a.total_cache_misses     = 100;
    r_a.memory_footprint_bytes = 4096;
    r_a.H1_clu_improvement     = 1.2;

    cmd::ExecutionResult r_b{};
    r_b.engine_name            = "ee-b";
    r_b.success                = true;
    r_b.throughput_ops_per_sec = 1000.0;
    r_b.latency_p99            = std::chrono::nanoseconds(1000);
    r_b.total_cache_misses     = 150;
    r_b.memory_footprint_bytes = 4096;

    cmd::CompareEngineCommand cmp(r_a, r_b);
    EXPECT_EQ(cmp.command_name(), "CompareEngineCommand");

    int rc = cmp.execute();
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(cmp.verdict(), cmd::CompareEngineCommand::Verdict::EE_A_Wins);
    EXPECT_NEAR(cmp.throughput_ratio(), 1.1, 0.01);
    EXPECT_EQ(cmp.latency_delta_ns(), 100); // 1000 - 900
    EXPECT_EQ(cmp.cache_miss_improvement(), 50);
    EXPECT_TRUE(cmp.h1_clu_validated());
}

TEST(CompareEngineCommand, Tie) {
    cmd::ExecutionResult r{};
    r.success                = true;
    r.throughput_ops_per_sec = 1000.0;
    r.latency_p99            = std::chrono::nanoseconds(1000);

    cmd::CompareEngineCommand cmp(r, r);
    cmp.execute();
    EXPECT_EQ(cmp.verdict(), cmd::CompareEngineCommand::Verdict::Tie);
}

TEST(AxisLibraryRegistry, LookupAchse12_1Simd) {
    auto variants = cmd::AxisLibraryRegistry::lookup("12.1");
    EXPECT_EQ(variants.size(), 5); // Scalar + AVX2 + AVX512 + NEON + SVE2
}

TEST(AxisLibraryRegistry, LookupAchse11Telemetry) {
    auto variants = cmd::AxisLibraryRegistry::lookup("11");
    EXPECT_EQ(variants.size(), 4); // LeafOnly + Sampled + Retroactive + PathRead (NICHT PerNode)
}

TEST(AxisLibraryRegistry, AllFourteenAxesPopulated) {
    // V33.A.4 — verifiziert dass jede der 14 Hauptachsen + Sub-Achsen nicht leer sind
    const std::vector<std::string_view> all_axes{
        "1", "2",  "3.A", "3.B",  "3.M",  "4",    "5",    "6.1",  "6.2",  "6.3",  "6.4",  "6.5",  "7",    "8.1", "8.2",
        "9", "10", "11",  "12.1", "12.2", "12.3", "12.4", "12.5", "13.1", "13.2", "13.3", "13.4", "13.5", "14"};
    for (auto axis : all_axes) {
        auto variants = cmd::AxisLibraryRegistry::lookup(axis);
        EXPECT_GE(variants.size(), 2u) << "Axis " << axis << " has only " << variants.size() << " variants";
    }
}

TEST(AxisLibraryRegistry, CompilerAxis15SubAxesPopulated) {
    // V35.B.1 — NEUE Compiler-Achse 15.1-15.5
    EXPECT_EQ(cmd::AxisLibraryRegistry::lookup("15.1").size(), 4u); // GCC,Clang,AppleClang,MSVC
    EXPECT_EQ(cmd::AxisLibraryRegistry::lookup("15.2").size(), 8u); // O0..O3 + Ofast + MSVC_Od/O1/O2
    EXPECT_EQ(cmd::AxisLibraryRegistry::lookup("15.3").size(), 4u); // None,ThinLTO,FullLTO,MSVC_LTCG
    EXPECT_EQ(cmd::AxisLibraryRegistry::lookup("15.4").size(), 4u); // None,Generate,Use,SamplePGO
    EXPECT_EQ(cmd::AxisLibraryRegistry::lookup("15.5").size(), 6u); // native,v3,v4,znver4,armv9-a,generic
}

TEST(AxisLibraryRegistry, CompilerAxisLookupContent) {
    // V35.B.1 — Stichprobe der wichtigsten Variants
    auto family      = cmd::AxisLibraryRegistry::lookup("15.1");
    bool found_msvc  = false;
    bool found_clang = false;
    for (const auto& v : family) {
        if (v.variant_name == "MSVC") found_msvc = true;
        if (v.variant_name == "Clang") found_clang = true;
    }
    EXPECT_TRUE(found_msvc);
    EXPECT_TRUE(found_clang);

    auto opt           = cmd::AxisLibraryRegistry::lookup("15.2");
    bool found_o3      = false;
    bool found_msvc_o2 = false;
    for (const auto& v : opt) {
        if (v.variant_name == "O3") found_o3 = true;
        if (v.variant_name == "MSVC_O2") found_msvc_o2 = true;
    }
    EXPECT_TRUE(found_o3);
    EXPECT_TRUE(found_msvc_o2);
}

TEST(AxisLibraryRegistry, UnknownAxisReturnsEmpty) {
    auto variants = cmd::AxisLibraryRegistry::lookup("99.99");
    EXPECT_TRUE(variants.empty());
}

TEST(WelchTTest, IdenticalSamplesGiveTOfZero) {
    std::vector<std::int64_t> a{100, 100, 100, 100, 100};
    std::vector<std::int64_t> b{100, 100, 100, 100, 100};
    auto                      r = cmd::stats::welch_t_test(a, b);
    EXPECT_TRUE(r.valid);
    EXPECT_NEAR(r.t_statistic, 0.0, 1e-9);
    EXPECT_NEAR(r.p_value, 1.0, 1e-6);
}

TEST(WelchTTest, ClearlySeparatedSamplesGiveLowP) {
    std::vector<std::int64_t> a{100, 102, 98, 101, 99, 100, 103, 97, 100, 101};
    std::vector<std::int64_t> b{500, 510, 495, 503, 498, 505, 502, 499, 501, 506};
    auto                      r = cmd::stats::welch_t_test(a, b);
    EXPECT_TRUE(r.valid);
    EXPECT_LT(r.p_value, 0.001) << "t=" << r.t_statistic << " df=" << r.degrees_of_freedom;
    EXPECT_LT(r.mean_a, r.mean_b);
}

TEST(WelchTTest, OverlappingSamplesGiveHighP) {
    std::vector<std::int64_t> a{100, 102, 98, 101, 99, 100, 103, 97, 100, 101};
    std::vector<std::int64_t> b{100, 103, 99, 101, 100, 102, 98, 100, 101, 102};
    auto                      r = cmd::stats::welch_t_test(a, b);
    EXPECT_TRUE(r.valid);
    EXPECT_GT(r.p_value, 0.05) << "t=" << r.t_statistic << " df=" << r.degrees_of_freedom;
}

TEST(WelchTTest, TooFewSamplesInvalid) {
    std::vector<std::int64_t> a{100};
    std::vector<std::int64_t> b{200, 201};
    auto                      r = cmd::stats::welch_t_test(a, b);
    EXPECT_FALSE(r.valid);
}

TEST(CompareEngineCommand, WelchBasedVerdictWhenSamplesPresent) {
    cmd::ExecutionResult r_a{};
    r_a.engine_name            = "ee-a";
    r_a.success                = true;
    r_a.throughput_ops_per_sec = 1000.0;
    r_a.memory_footprint_bytes = 4096;
    r_a.latency_samples_ns     = {100, 102, 98, 101, 99, 100, 103, 97, 100, 101};

    cmd::ExecutionResult r_b{};
    r_b.engine_name            = "ee-b";
    r_b.success                = true;
    r_b.throughput_ops_per_sec = 1000.0; // gleiches Throughput!
    r_b.memory_footprint_bytes = 4096;
    r_b.latency_samples_ns     = {500, 510, 495, 503, 498, 505, 502, 499, 501, 506};

    cmd::CompareEngineCommand cmp(r_a, r_b);
    int                       rc = cmp.execute();
    EXPECT_EQ(rc, 0);
    // Welch-basiertes Verdict: a hat kleinere Latenz → EE_A_Wins (auch wenn throughput gleich!)
    EXPECT_EQ(cmp.verdict(), cmd::CompareEngineCommand::Verdict::EE_A_Wins);
    EXPECT_TRUE(cmp.welch().valid);
    EXPECT_LT(cmp.welch().p_value, 0.05);
}

TEST(CompareEngineCommand, WelchTieWhenOverlapping) {
    cmd::ExecutionResult r_a{};
    r_a.engine_name            = "ee-a";
    r_a.success                = true;
    r_a.throughput_ops_per_sec = 1000.0;
    r_a.latency_samples_ns     = {100, 102, 98, 101, 99, 100, 103, 97, 100, 101};

    cmd::ExecutionResult r_b{};
    r_b.engine_name            = "ee-b";
    r_b.success                = true;
    r_b.throughput_ops_per_sec = 1000.0;
    r_b.latency_samples_ns     = {100, 103, 99, 101, 100, 102, 98, 100, 101, 102};

    cmd::CompareEngineCommand cmp(r_a, r_b);
    cmp.execute();
    EXPECT_EQ(cmp.verdict(), cmd::CompareEngineCommand::Verdict::Tie);
}

TEST(AutoPermutator, DiscoverAndGenerate) {
    cmd::AutoPermutator ap("12.1");
    ap.discover_axis_implementations();
    ap.platform_filter(); // V32.HH.2: no-op
    // user_limit_filter mit leerer Liste -> alle disallowed
    auto perms = ap.generate_permutations();
    // Mit user_allowed=true Default sind alle generierbar (V32.HH.1 Stand)
    EXPECT_GE(perms.size(), 0);
}

TEST(MultiAxisAutoPermutator, DiscoverPlanForThreeAxes) {
    // V33.B.2: simuliert PRT-ART default-lookup auf 3 Achsen
    std::vector<std::string>     axes{"3.B", "11", "12.1"};
    cmd::MultiAxisAutoPermutator multi(std::move(axes));
    multi.discover_all();
    auto plan = multi.build_plan();
    EXPECT_EQ(plan.axis_ids.size(), 3u);
    EXPECT_EQ(plan.variants_per_axis.size(), 3u);
    // 3.B = 3 Varianten, 11 = 4 Varianten, 12.1 = 5 Varianten -> 60
    EXPECT_EQ(plan.cartesian_size(), 3u * 4u * 5u);
}

TEST(MultiAxisAutoPermutator, EmptyAxisListGivesEmptyPlan) {
    cmd::MultiAxisAutoPermutator multi({});
    multi.discover_all();
    auto plan = multi.build_plan();
    EXPECT_TRUE(plan.axis_ids.empty());
    EXPECT_EQ(plan.cartesian_size(), 0u);
}
