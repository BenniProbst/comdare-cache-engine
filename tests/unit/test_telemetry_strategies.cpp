// Test fuer 5 TelemetryStrategy-Konkretisierungen Achse 11 Kuehn
// Termin 7 / 02_uml_cache_engine §4

#include <cache_engine/concepts/telemetry/leaf_only_counter.hpp>
#include <cache_engine/concepts/telemetry/leaf_only_sampled_counter.hpp>
#include <cache_engine/concepts/telemetry/path_read_counter.hpp>
#include <cache_engine/concepts/telemetry/per_node_counter.hpp>
#include <cache_engine/concepts/telemetry/probability_hints_header.hpp>
#include <cache_engine/concepts/telemetry/retroactive_aggregation.hpp>

#include <gtest/gtest.h>

namespace ce = comdare::cache_engine;

TEST(PerNodeCounter, KindIsCorrect) {
    ce::PerNodeCounter c{8};
    EXPECT_EQ(c.kind(), ce::TelemetryStrategyKind::PerNodeCounter);
    EXPECT_EQ(c.size(), 8u);
}

TEST(PerNodeCounter, IncrementBumpsValue) {
    ce::PerNodeCounter c{4};
    c.increment(2);
    c.increment(2);
    c.increment(2);
    EXPECT_EQ(c.value(2), 3u);
    EXPECT_EQ(c.value(0), 0u);
}

TEST(PerNodeCounter, OutOfRangeIsSafe) {
    ce::PerNodeCounter c{2};
    c.increment(99);          // out-of-range
    EXPECT_EQ(c.value(99), 0u);
}

TEST(LeafOnlyCounter, KindIsCorrect) {
    ce::LeafOnlyCounter c{16};
    EXPECT_EQ(c.kind(), ce::TelemetryStrategyKind::LeafOnlyCounter);
    EXPECT_EQ(c.leaf_count(), 16u);
}

TEST(LeafOnlyCounter, OnlyLeavesAreUpdated) {
    ce::LeafOnlyCounter c{4};
    c.increment_leaf(1);
    c.increment_leaf(1);
    EXPECT_EQ(c.leaf_value(1), 2u);
}

TEST(LeafOnlySampledCounter, SamplingRateExposed) {
    ce::LeafOnlySampledCounter<10> c{4};
    EXPECT_EQ(c.sampling_rate(), 10u);
    EXPECT_EQ(c.kind(), ce::TelemetryStrategyKind::LeafOnlySampledCounter);
}

TEST(LeafOnlySampledCounter, OnlyEveryNthIncrementsLeaf) {
    ce::LeafOnlySampledCounter<5> c{2};
    for (int i = 0; i < 9; ++i) c.maybe_increment_leaf(0);  // 9 calls, N=5
    // call 5 trifft → leaf_value(0) == 1
    EXPECT_EQ(c.leaf_value(0), 1u);
    c.maybe_increment_leaf(0);  // 10. call → leaf_value(0) == 2
    EXPECT_EQ(c.leaf_value(0), 2u);
}

TEST(RetroactiveAggregation, KindIsCorrect) {
    ce::RetroactiveAggregation a;
    EXPECT_EQ(a.kind(), ce::TelemetryStrategyKind::RetroactiveAggregation);
}

TEST(RetroactiveAggregation, AggregatesLeafCountsToInnerNodes) {
    ce::RetroactiveAggregation a;
    std::vector<std::uint64_t> leaves = {3, 5, 7, 11};
    std::vector<std::size_t>   leaf_to_inner = {0, 0, 1, 1};
    auto h = a.run_aggregation(leaves, leaf_to_inner);
    ASSERT_EQ(h.per_inner_node_count.size(), 2u);
    EXPECT_EQ(h.per_inner_node_count[0], 8u);
    EXPECT_EQ(h.per_inner_node_count[1], 18u);
    EXPECT_EQ(a.total_runs(), 1u);
}

TEST(RetroactiveAggregation, BarrierTriggersAreCounted) {
    ce::RetroactiveAggregation a;
    ce::ConsolidationBarrierEvent e{};
    a.on_consolidation_barrier(e);
    a.on_consolidation_barrier(e);
    EXPECT_EQ(a.barrier_triggers(), 2u);
}

TEST(PathReadCounter, KindIsCorrect) {
    ce::PathReadCounter p;
    EXPECT_EQ(p.kind(), ce::TelemetryStrategyKind::PathReadCounter);
}

TEST(PathReadCounter, RecordsBlocksAndDetectsHotness) {
    ce::PathReadCounter p{5};
    for (int i = 0; i < 7; ++i) p.record_block_read(42);
    EXPECT_EQ(p.block_read_count(42), 7u);
    EXPECT_TRUE(p.is_hot(42));
    EXPECT_FALSE(p.is_hot(99));
}

TEST(ProbabilityHintsHeader, KindIsCorrect) {
    ce::ProbabilityHintsHeader<16> h;
    EXPECT_EQ(h.kind(), ce::TelemetryStrategyKind::ProbabilityHints);
    EXPECT_EQ(h.layout_bytes(), 16u);
}

TEST(ProbabilityHintsHeader, RecordAccessSaturatesAt255) {
    ce::ProbabilityHintsHeader<8> h;
    for (int i = 0; i < 300; ++i) h.record_access(0);
    EXPECT_EQ(h.hint(0), 255);
}

TEST(ProbabilityHintsHeader, BytesAreDistributedByModulo) {
    ce::ProbabilityHintsHeader<4> h;
    h.record_access(5);    // 5 % 4 == 1
    h.record_access(9);    // 9 % 4 == 1
    EXPECT_EQ(h.hint(1), 2);
    EXPECT_EQ(h.hint(0), 0);
}
