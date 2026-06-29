#pragma once
// HybridCompositionCommand — Sammler atomarer Strategy-Commands
// Termin 7 / 10_korrektur §K3.4 (17 Hybrid-Aufloesungen)
// Beispiele: Wormhole P07 Triple-Layer, HOT P02 9-Layout, LOUDS P10 Dense+Sparse

#include <cache_engine/strategy_command/i_composition_rule.hpp>
#include <cache_engine/strategy_command/i_strategy_command.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace comdare::cache_engine::strategy_command {

class HybridCompositionCommand final : public IStrategyCommand {
public:
    explicit HybridCompositionCommand(std::string label, std::unique_ptr<ICompositionRule> rule)
        : label_(std::move(label)), rule_(std::move(rule)) {}

    void add_part(IStrategyCommand* part) {
        if (part) parts_.push_back(part);
    }

    [[nodiscard]] std::size_t             part_count() const noexcept { return parts_.size(); }
    [[nodiscard]] ICompositionRule const* rule() const noexcept { return rule_.get(); }

    [[nodiscard]] std::string name() const override { return label_; }

    [[nodiscard]] CommandResult execute(StrategyContext& ctx) override {
        CommandResult agg{};
        agg.success = true;
        if (!rule_) return agg;
        ExecutionPlan plan = rule_->applies(parts_, ctx);
        for (std::size_t idx : plan.ordered_indices) {
            if (idx >= parts_.size()) continue;
            CommandResult r = parts_[idx]->execute(ctx);
            agg.success     = agg.success && r.success;
            agg.output_metric += r.output_metric;
            if (!r.diagnostic.empty()) {
                if (!agg.diagnostic.empty()) agg.diagnostic += "; ";
                agg.diagnostic += r.diagnostic;
            }
        }
        return agg;
    }

    [[nodiscard]] bool can_compose_with(IStrategyCommand const& other) const noexcept override {
        // Hybrid ist mit allem komponierbar, was die einzelnen Parts erlauben
        for (auto* p : parts_) {
            if (!p->can_compose_with(other)) return false;
        }
        return true;
    }

private:
    std::string                       label_;
    std::unique_ptr<ICompositionRule> rule_;
    std::vector<IStrategyCommand*>    parts_{};
};

} // namespace comdare::cache_engine::strategy_command
