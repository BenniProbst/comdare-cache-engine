// Test fuer IStrategyCommand + 4 CompositionRules + HybridCompositionCommand
// Termin 7 / 10_korrektur §K3.4 + 22_architektur_skizze_REV5 §2.4

#include <cache_engine/strategy_command/hybrid_composition_command.hpp>
#include <cache_engine/strategy_command/i_composition_rule.hpp>
#include <cache_engine/strategy_command/i_strategy_command.hpp>

#include <gtest/gtest.h>

namespace sc = comdare::cache_engine::strategy_command;

namespace {

class CountingCommand final : public sc::IStrategyCommand {
public:
    explicit CountingCommand(std::string n, int metric) : name_(std::move(n)), metric_(metric) {}

    [[nodiscard]] std::string name() const override { return name_; }

    [[nodiscard]] sc::CommandResult execute(sc::StrategyContext&) override {
        ++calls;
        sc::CommandResult r{};
        r.success       = true;
        r.output_metric = metric_;
        r.diagnostic    = name_;
        return r;
    }

    [[nodiscard]] bool can_compose_with(sc::IStrategyCommand const&) const noexcept override { return true; }

    int calls = 0;

private:
    std::string name_;
    int         metric_;
};

} // namespace

TEST(StrategyCommand, CountingCommandRunsAndCounts) {
    CountingCommand     c("x", 7);
    sc::StrategyContext ctx{};
    auto                r = c.execute(ctx);
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.output_metric, 7);
    EXPECT_EQ(c.calls, 1);
}

TEST(SequentialRule, ProducesOrderInIndexOrder) {
    sc::SequentialRule                 rule;
    CountingCommand                    a{"a", 1}, b{"b", 2}, c{"c", 3};
    std::vector<sc::IStrategyCommand*> parts = {&a, &b, &c};
    sc::StrategyContext                ctx{};
    auto                               plan = rule.applies(parts, ctx);
    EXPECT_FALSE(plan.parallelizable);
    ASSERT_EQ(plan.ordered_indices.size(), 3u);
    EXPECT_EQ(plan.ordered_indices[0], 0u);
    EXPECT_EQ(plan.ordered_indices[2], 2u);
}

TEST(ParallelRule, ProducesParallelPlan) {
    sc::ParallelRule                   rule;
    CountingCommand                    a{"a", 1}, b{"b", 2};
    std::vector<sc::IStrategyCommand*> parts = {&a, &b};
    sc::StrategyContext                ctx{};
    auto                               plan = rule.applies(parts, ctx);
    EXPECT_TRUE(plan.parallelizable);
    EXPECT_EQ(plan.ordered_indices.size(), 2u);
}

TEST(ConditionalRule, PicksFirstPartOnTrue) {
    auto                               pred_true = [](sc::StrategyContext const&) noexcept { return true; };
    sc::ConditionalRule                rule(pred_true);
    CountingCommand                    a{"a", 1}, b{"b", 2};
    std::vector<sc::IStrategyCommand*> parts = {&a, &b};
    sc::StrategyContext                ctx{};
    auto                               plan = rule.applies(parts, ctx);
    ASSERT_EQ(plan.ordered_indices.size(), 1u);
    EXPECT_EQ(plan.ordered_indices[0], 0u);
}

TEST(ConditionalRule, PicksSecondPartOnFalse) {
    auto                               pred_false = [](sc::StrategyContext const&) noexcept { return false; };
    sc::ConditionalRule                rule(pred_false);
    CountingCommand                    a{"a", 1}, b{"b", 2};
    std::vector<sc::IStrategyCommand*> parts = {&a, &b};
    sc::StrategyContext                ctx{};
    auto                               plan = rule.applies(parts, ctx);
    ASSERT_EQ(plan.ordered_indices.size(), 1u);
    EXPECT_EQ(plan.ordered_indices[0], 1u);
}

TEST(RecursiveRule, RepeatsPartsByDepthLimit) {
    sc::RecursiveRule                  rule(3);
    CountingCommand                    a{"a", 1}, b{"b", 2};
    std::vector<sc::IStrategyCommand*> parts = {&a, &b};
    sc::StrategyContext                ctx{};
    auto                               plan = rule.applies(parts, ctx);
    EXPECT_EQ(plan.ordered_indices.size(), 6u); // 3 * 2
    EXPECT_EQ(rule.depth_limit(), 3u);
}

TEST(HybridCompositionCommand, AggregatesPartResults) {
    // Beispiel: Wormhole P07 Triple-Layer (HashAnchor + BPlusHop + LeafLinkedListScan)
    auto                         seq = std::make_unique<sc::SequentialRule>();
    sc::HybridCompositionCommand hybrid("WormholeTripleLayer", std::move(seq));

    CountingCommand hash_anchor{"HashAnchor", 10};
    CountingCommand bplus_hop{"BPlusHop", 20};
    CountingCommand leaf_scan{"LeafScan", 30};
    hybrid.add_part(&hash_anchor);
    hybrid.add_part(&bplus_hop);
    hybrid.add_part(&leaf_scan);

    sc::StrategyContext ctx{};
    auto                r = hybrid.execute(ctx);
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.output_metric, 60); // 10 + 20 + 30
    EXPECT_EQ(hash_anchor.calls, 1);
    EXPECT_EQ(bplus_hop.calls, 1);
    EXPECT_EQ(leaf_scan.calls, 1);
}

TEST(HybridCompositionCommand, EmptyPartsIsSafe) {
    auto                         seq = std::make_unique<sc::SequentialRule>();
    sc::HybridCompositionCommand hybrid("Empty", std::move(seq));
    sc::StrategyContext          ctx{};
    auto                         r = hybrid.execute(ctx);
    EXPECT_TRUE(r.success);
    EXPECT_EQ(hybrid.part_count(), 0u);
}

TEST(HybridCompositionCommand, RecursiveExecutionMultipliesCalls) {
    // Beispiel: B^2-Tree P06 RecursiveCommonPrefixDecomposition
    auto                         rec = std::make_unique<sc::RecursiveRule>(2);
    sc::HybridCompositionCommand hybrid("B2RecursiveCPD", std::move(rec));

    CountingCommand decision{"Decision", 1};
    CountingCommand span{"Span", 2};
    hybrid.add_part(&decision);
    hybrid.add_part(&span);

    sc::StrategyContext ctx{};
    hybrid.execute(ctx);
    EXPECT_EQ(decision.calls, 2); // 2 (depth) * 1 (parts)
    EXPECT_EQ(span.calls, 2);
}

TEST(CompositionRule, FourKindsAreDistinct) {
    sc::SequentialRule  s;
    sc::ParallelRule    p;
    sc::ConditionalRule c{nullptr};
    sc::RecursiveRule   r;
    EXPECT_EQ(s.kind(), sc::CompositionRuleKind::Sequential);
    EXPECT_EQ(p.kind(), sc::CompositionRuleKind::Parallel);
    EXPECT_EQ(c.kind(), sc::CompositionRuleKind::Conditional);
    EXPECT_EQ(r.kind(), sc::CompositionRuleKind::Recursive);
}
