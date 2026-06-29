#pragma once
// ValueHandleSelectionTree<HandleT> — VALUEHANDLE_DYNAMIC switch_array-Wahl
// Termin 7 / 02_uml_cache_engine §2

#include <cache_engine/concepts/i_decision_lambda_tree.hpp>

namespace comdare::cache_engine {

template <typename HandleT>
class ValueHandleSelectionTree final : public IDecisionLambdaTree<TelemetryUpdateEvent> {
public:
    void configure(NodeTreeConfig const& config) override { config_ = config; }

    [[nodiscard]] Decision evaluate(TelemetryUpdateEvent const&, DecisionContext const&) const noexcept override {
        ++state_.total_evaluations;
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
