// Test fuer DecisionLambdaTreeBundle + Registry (Termin 7 / 02_uml_cache_engine §2)

#include <cache_engine/concepts/decision_lambda_tree_bundle.hpp>
#include <cache_engine/concepts/decision_lambda_tree_registry.hpp>

#include <gtest/gtest.h>

namespace ce = comdare::cache_engine;

namespace { struct DummyPage {}; }

TEST(AnyDecisionTreeBundle, EmptyBundlePopulatedCountIsZero) {
    ce::AnyDecisionTreeBundle b{};
    EXPECT_EQ(b.populated_count(), 0u);
}

TEST(AnyDecisionTreeBundle, PopulatedCountReflectsAssignedSlots) {
    ce::PageRelocationTree<DummyPage> tree_relocation;
    ce::PageTypeChangeTree<DummyPage> tree_type_change;
    ce::AnyDecisionTreeBundle b{};
    b.page_relocation = &tree_relocation;
    EXPECT_EQ(b.populated_count(), 1u);
    b.page_type_change = &tree_type_change;
    EXPECT_EQ(b.populated_count(), 2u);
    b.page_relocation = nullptr;
    EXPECT_EQ(b.populated_count(), 1u);
}

TEST(DecisionLambdaTreeRegistry, EmptyRegistryDispatchExecutesByDefault) {
    ce::DecisionLambdaTreeRegistry r;
    ce::Event e{};
    e.module_id = 1;
    e.kind = ce::EventKind::PageRelocation;
    EXPECT_EQ(r.dispatch(1, e, ce::DecisionContext{}), ce::Decision::EXECUTE);
    EXPECT_EQ(r.module_count(), 0u);
}

TEST(DecisionLambdaTreeRegistry, RegisterAndUnregisterTracksModules) {
    ce::DecisionLambdaTreeRegistry r;
    ce::AnyDecisionTreeBundle b{};
    r.register_module_trees(7, b);
    EXPECT_TRUE(r.has_module(7));
    EXPECT_EQ(r.module_count(), 1u);
    r.unregister_module(7);
    EXPECT_FALSE(r.has_module(7));
}

TEST(DecisionLambdaTreeRegistry, DispatchPageRelocationRoutesToBundleTree) {
    ce::PageRelocationTree<DummyPage> tree;
    ce::AnyDecisionTreeBundle b{};
    b.page_relocation = &tree;

    ce::DecisionLambdaTreeRegistry r;
    r.register_module_trees(1, b);

    ce::PageRelocationEvent e{};
    e.module_id = 1;
    e.kind = ce::EventKind::PageRelocation;
    e.load_factor = 0.05;        // sehr klein → EXECUTE im Tree
    EXPECT_EQ(r.dispatch(1, e, ce::DecisionContext{}), ce::Decision::EXECUTE);
}

TEST(DecisionLambdaTreeRegistry, DispatchWriteEventPrefersCoherenceTree) {
    ce::CoherenceAwareWriteDecisionTree write_tree;
    ce::AnyDecisionTreeBundle b{};
    b.coherence_aware_write = &write_tree;

    ce::DecisionLambdaTreeRegistry r;
    r.register_module_trees(2, b);

    ce::WriteEvent e{};
    e.module_id = 2;
    e.kind = ce::EventKind::Write;
    e.node_depth = 0;
    e.num_cores_sharing = 16;     // SKIP-Bedingung im Tree
    EXPECT_EQ(r.dispatch(2, e, ce::DecisionContext{}), ce::Decision::SKIP);
}

TEST(DecisionLambdaTreeRegistry, ClearRemovesAll) {
    ce::DecisionLambdaTreeRegistry r;
    r.register_module_trees(1, {});
    r.register_module_trees(2, {});
    r.clear();
    EXPECT_EQ(r.module_count(), 0u);
}

TEST(DecisionLambdaTreeRegistry, UnknownEventKindIsExecuteByDefault) {
    ce::DecisionLambdaTreeRegistry r;
    r.register_module_trees(1, {});

    ce::Event e{};
    e.module_id = 1;
    e.kind = ce::EventKind::ConsolidationBarrier;     // ohne Tree
    EXPECT_EQ(r.dispatch(1, e, ce::DecisionContext{}), ce::Decision::EXECUTE);
}
