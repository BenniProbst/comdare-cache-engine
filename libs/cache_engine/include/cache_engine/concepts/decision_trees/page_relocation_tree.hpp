#pragma once
// PageRelocationTree<PageT> — F-EXTRA-6 (Termin 7 / 02_uml_cache_engine §2 Beispiel)

#include <cache_engine/concepts/i_decision_lambda_tree.hpp>

namespace comdare::cache_engine {

template <typename PageT>
class PageRelocationTree final : public IDecisionLambdaTree<PageRelocationEvent> {
public:
    PageRelocationTree() = default;

    void configure(NodeTreeConfig const& config) override { config_ = config; }

    [[nodiscard]] Decision evaluate(PageRelocationEvent const& event,
                                    DecisionContext const&     ctx) const noexcept override {
        ++state_.total_evaluations;
        // Lambda 1: Pruefe Load-Factor
        if (event.load_factor < 0.20) {
            ++state_.total_executes;
            return Decision::EXECUTE;
        }
        if (event.load_factor > 0.95) {
            ++state_.total_executes;
            return Decision::EXECUTE;
        }
        // Lambda 2: Block AN Cache-Coherence-Cost
        if (ctx.cost_threshold > 0.0 && event.load_factor * event.load_factor > ctx.cost_threshold) {
            ++state_.total_delays;
            return Decision::DELAY;
        }
        // Lambda 3: Anti-Last-Spirale (F2)
        if (config_.max_per_interval > 0 && ctx.recent_relocations_count > config_.max_per_interval) {
            ++state_.total_skips;
            return Decision::SKIP;
        }
        ++state_.total_executes;
        return Decision::EXECUTE;
    }

    [[nodiscard]] NodeTreeState save_state() const noexcept override { return state_; }
    void                        restore_state(NodeTreeState const& s) noexcept override { state_ = s; }

private:
    NodeTreeConfig        config_{};
    mutable NodeTreeState state_{};
};

} // namespace comdare::cache_engine
