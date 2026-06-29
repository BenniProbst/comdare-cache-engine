#pragma once
// ConcurrencyDisciplineSwitchTree<DiscT> — OLC ↔ ROWEX ↔ RCU bei Last-Aenderung
// Termin 7 / 02_uml_cache_engine §2

#include <cache_engine/concepts/i_decision_lambda_tree.hpp>

namespace comdare::cache_engine {

template <typename DisciplineT>
class ConcurrencyDisciplineSwitchTree final : public IDecisionLambdaTree<WriteEvent> {
public:
    void configure(NodeTreeConfig const& config) override { config_ = config; }

    [[nodiscard]] Decision evaluate(WriteEvent const& event, DecisionContext const&) const noexcept override {
        ++state_.total_evaluations;
        // Hohe Core-Konkurrenz an Wurzelnaehe -> SKIP discipline switch (Block AN)
        if (event.node_depth <= 2 && event.num_cores_sharing > 8) {
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
