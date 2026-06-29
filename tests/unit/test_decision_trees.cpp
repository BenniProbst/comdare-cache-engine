// Test fuer DecisionLambdaTrees (Termin 7 / 02_uml_cache_engine §2)

#include <cache_engine/concepts/decision_trees/allocator_rebalance_tree.hpp>
#include <cache_engine/concepts/decision_trees/cache_coherence_detection_tree.hpp>
#include <cache_engine/concepts/decision_trees/coherence_aware_write_decision_tree.hpp>
#include <cache_engine/concepts/decision_trees/concurrency_discipline_switch_tree.hpp>
#include <cache_engine/concepts/decision_trees/hot_path_recognition_tree.hpp>
#include <cache_engine/concepts/decision_trees/page_relocation_tree.hpp>
#include <cache_engine/concepts/decision_trees/page_type_change_tree.hpp>
#include <cache_engine/concepts/decision_trees/prefetch_adjustment_tree.hpp>
#include <cache_engine/concepts/decision_trees/sampling_rate_adjustment_tree.hpp>
#include <cache_engine/concepts/decision_trees/value_handle_selection_tree.hpp>

#include <gtest/gtest.h>

namespace ce = comdare::cache_engine;

namespace {
struct DummyPage {};
struct DummyTraversal {};
struct DummyAllocator {};
struct DummyDiscipline {};
struct DummyHandle {};
struct DummyTelemetry {};
} // namespace

TEST(DecisionEnum, ThreeValuesExistAndAreDistinct) {
    EXPECT_EQ(static_cast<int>(ce::Decision::EXECUTE), 0);
    EXPECT_EQ(static_cast<int>(ce::Decision::DELAY), 1);
    EXPECT_EQ(static_cast<int>(ce::Decision::SKIP), 2);
}

TEST(PageRelocationTree, LowLoadFactorTriggersExecute) {
    ce::PageRelocationTree<DummyPage> t;
    ce::PageRelocationEvent           e{};
    e.load_factor = 0.10;
    EXPECT_EQ(t.evaluate(e, ce::DecisionContext{}), ce::Decision::EXECUTE);
}

TEST(PageRelocationTree, HighLoadFactorTriggersExecute) {
    ce::PageRelocationTree<DummyPage> t;
    ce::PageRelocationEvent           e{};
    e.load_factor = 0.99;
    EXPECT_EQ(t.evaluate(e, ce::DecisionContext{}), ce::Decision::EXECUTE);
}

TEST(PageRelocationTree, AntiSpiraleSkip) {
    ce::PageRelocationTree<DummyPage> t;
    ce::NodeTreeConfig                cfg;
    cfg.max_per_interval = 5;
    t.configure(cfg);

    ce::PageRelocationEvent e{};
    e.load_factor = 0.50; // im Mid-Bereich
    ce::DecisionContext ctx;
    ctx.recent_relocations_count = 100; // ueber Limit
    EXPECT_EQ(t.evaluate(e, ctx), ce::Decision::SKIP);
}

TEST(PageRelocationTree, StateTracksEvaluations) {
    ce::PageRelocationTree<DummyPage> t;
    ce::PageRelocationEvent           e{};
    e.load_factor = 0.10;
    t.evaluate(e, ce::DecisionContext{});
    t.evaluate(e, ce::DecisionContext{});
    auto s = t.save_state();
    EXPECT_EQ(s.total_evaluations, 2u);
    EXPECT_EQ(s.total_executes, 2u);
}

TEST(PageTypeChangeTree, IdenticalTypesAreSkipped) {
    ce::PageTypeChangeTree<DummyPage> t;
    ce::PageTypeChangeEvent           e{};
    e.current_type   = 4;
    e.suggested_type = 4;
    EXPECT_EQ(t.evaluate(e, ce::DecisionContext{}), ce::Decision::SKIP);
}

TEST(PageTypeChangeTree, DifferentTypesAreExecuted) {
    ce::PageTypeChangeTree<DummyPage> t;
    ce::PageTypeChangeEvent           e{};
    e.current_type   = 4;
    e.suggested_type = 16;
    EXPECT_EQ(t.evaluate(e, ce::DecisionContext{}), ce::Decision::EXECUTE);
}

TEST(PrefetchAdjustmentTree, HighMissTriggersExecute) {
    ce::PrefetchAdjustmentTree<DummyTraversal> t;
    ce::PrefetchAdjustmentEvent                e{};
    e.measured_miss_rate = 0.50;
    EXPECT_EQ(t.evaluate(e, ce::DecisionContext{}), ce::Decision::EXECUTE);
}

TEST(PrefetchAdjustmentTree, NaderanLowMissSkipsPrefetch) {
    ce::PrefetchAdjustmentTree<DummyTraversal> t;
    ce::PrefetchAdjustmentEvent                e{};
    e.measured_miss_rate = 0.001; // P24 Naderan-Tahan: nicht nuetzlich
    EXPECT_EQ(t.evaluate(e, ce::DecisionContext{}), ce::Decision::SKIP);
}

TEST(HotPathRecognitionTree, HighHotScoreExecutes) {
    ce::HotPathRecognitionTree<DummyTraversal> t;
    ce::HotPathRecognitionEvent                e{};
    e.hot_score = 0.85;
    EXPECT_EQ(t.evaluate(e, ce::DecisionContext{}), ce::Decision::EXECUTE);
}

TEST(HotPathRecognitionTree, LowHotScoreSkips) {
    ce::HotPathRecognitionTree<DummyTraversal> t;
    ce::HotPathRecognitionEvent                e{};
    e.hot_score = 0.20;
    EXPECT_EQ(t.evaluate(e, ce::DecisionContext{}), ce::Decision::SKIP);
}

TEST(AllocatorRebalanceTree, ExtremeLoadFactorExecutes) {
    ce::AllocatorRebalanceTree<DummyAllocator> t;
    ce::PageRelocationEvent                    e{};
    e.load_factor = 0.05;
    EXPECT_EQ(t.evaluate(e, ce::DecisionContext{}), ce::Decision::EXECUTE);
}

TEST(ConcurrencyDisciplineSwitchTree, RootDepthMultiCoreSkips) {
    ce::ConcurrencyDisciplineSwitchTree<DummyDiscipline> t;
    ce::WriteEvent                                       e{};
    e.node_depth        = 1;
    e.num_cores_sharing = 16;
    EXPECT_EQ(t.evaluate(e, ce::DecisionContext{}), ce::Decision::SKIP);
}

TEST(ValueHandleSelectionTree, AlwaysExecutes) {
    ce::ValueHandleSelectionTree<DummyHandle> t;
    ce::TelemetryUpdateEvent                  e{};
    EXPECT_EQ(t.evaluate(e, ce::DecisionContext{}), ce::Decision::EXECUTE);
}

TEST(SamplingRateAdjustmentTree, HighCpuLoadAdjusts) {
    ce::SamplingRateAdjustmentTree<DummyTelemetry> t;
    ce::SamplingEvent                              e{};
    e.cpu_load = 0.90;
    EXPECT_EQ(t.evaluate(e, ce::DecisionContext{}), ce::Decision::EXECUTE);
}

TEST(SamplingRateAdjustmentTree, AdjustNStoresCurrentN) {
    ce::SamplingRateAdjustmentTree<DummyTelemetry> t;
    EXPECT_EQ(t.current_n(), 1000u);
    t.adjust_n(2000);
    EXPECT_EQ(t.current_n(), 2000u);
}

TEST(CoherenceAwareWriteDecisionTree, RootDepthMultiCoreSkips) {
    ce::CoherenceAwareWriteDecisionTree t;
    ce::WriteEvent                      e{};
    e.node_depth        = 1;
    e.num_cores_sharing = 16;
    EXPECT_EQ(t.evaluate(e, ce::DecisionContext{}), ce::Decision::SKIP);
}

TEST(CoherenceAwareWriteDecisionTree, HighCostDelays) {
    ce::CoherenceAwareWriteDecisionTree t;
    ce::WriteEvent                      e{};
    e.node_depth            = 5;
    e.num_cores_sharing     = 32;
    e.cache_line_size_bytes = 128;
    ce::DecisionContext ctx;
    ctx.cost_threshold = 0.0; // alles >0 ist 'high cost' → DELAY
    EXPECT_EQ(t.evaluate(e, ctx), ce::Decision::DELAY);
}

TEST(CoherenceAwareWriteDecisionTree, CostFunctionMonotonicWithCores) {
    ce::WriteEvent e1{};
    e1.node_depth        = 5;
    e1.num_cores_sharing = 8;
    ce::WriteEvent e2    = e1;
    e2.num_cores_sharing = 64;
    EXPECT_LT(ce::CoherenceAwareWriteDecisionTree::compute_cache_coherence_cost(e1),
              ce::CoherenceAwareWriteDecisionTree::compute_cache_coherence_cost(e2));
}

TEST(CacheCoherenceDetectionTree, PerNodeCounterAtHotNodeIsSkipped) {
    ce::CacheCoherenceDetectionTree t;
    ce::TelemetryUpdateEvent        e{};
    e.telemetry_strategy = 0; // PER_NODE_COUNTER
    e.counter_delta      = 1;
    e.node               = 4; // wurzelnah
    EXPECT_EQ(t.evaluate(e, ce::DecisionContext{}), ce::Decision::SKIP);
}

TEST(CacheCoherenceDetectionTree, LeafOnlyBypassesAntiPattern) {
    ce::CacheCoherenceDetectionTree t;
    ce::TelemetryUpdateEvent        e{};
    e.telemetry_strategy = 1; // LEAFONLY_COUNTER
    e.counter_delta      = 1;
    e.node               = 4;
    EXPECT_EQ(t.evaluate(e, ce::DecisionContext{}), ce::Decision::EXECUTE);
}
