#pragma once
// HotPathRecognitionTree<TraversalT> — z.B. P26 Zhang FGCS Hot-Path-Detection per Read-Counter
// Termin 7 / 02_uml_cache_engine §2

#include <cache_engine/concepts/i_decision_lambda_tree.hpp>

namespace comdare::cache_engine {

template <typename TraversalT>
class HotPathRecognitionTree final : public IDecisionLambdaTree<HotPathRecognitionEvent> {
public:
    void configure(NodeTreeConfig const& config) override { config_ = config; }

    [[nodiscard]] Decision evaluate(HotPathRecognitionEvent const& event,
                                    DecisionContext const&) const noexcept override {
        ++state_.total_evaluations;
        // P26 Zhang FGCS: Hot-Path-Score >= 0.80 → EXECUTE
        if (event.hot_score >= 0.80) {
            ++state_.total_executes;
            return Decision::EXECUTE;
        }
        if (event.hot_score >= 0.50) {
            ++state_.total_delays;
            return Decision::DELAY;
        }
        ++state_.total_skips;
        return Decision::SKIP;
    }

    [[nodiscard]] NodeTreeState save_state() const noexcept override { return state_; }
    void restore_state(NodeTreeState const& s) noexcept override { state_ = s; }

private:
    NodeTreeConfig config_{};
    mutable NodeTreeState state_{};
};

}  // namespace comdare::cache_engine
