#pragma once
// PrefetchAdjustmentTree<TraversalT> — z.B. P23 Khan adaptive Prefetch-Distanz
// Termin 7 / 02_uml_cache_engine §2

#include <cache_engine/concepts/i_decision_lambda_tree.hpp>

namespace comdare::cache_engine {

template <typename TraversalT>
class PrefetchAdjustmentTree final : public IDecisionLambdaTree<PrefetchAdjustmentEvent> {
public:
    void configure(NodeTreeConfig const& config) override { config_ = config; }

    [[nodiscard]] Decision evaluate(PrefetchAdjustmentEvent const& event,
                                    DecisionContext const&) const noexcept override {
        ++state_.total_evaluations;
        // P23 Khan: hoher miss_rate -> distance erhoehen (EXECUTE)
        if (event.measured_miss_rate > 0.30) {
            ++state_.total_executes;
            return Decision::EXECUTE;
        }
        // P24 Naderan: niedriger miss_rate -> Prefetch unproductive (SKIP)
        if (event.measured_miss_rate < 0.005) {
            ++state_.total_skips;
            return Decision::SKIP;
        }
        // Mid-band: warten bis stabilere Messung
        ++state_.total_delays;
        return Decision::DELAY;
    }

    [[nodiscard]] NodeTreeState save_state() const noexcept override { return state_; }
    void restore_state(NodeTreeState const& s) noexcept override { state_ = s; }

private:
    NodeTreeConfig config_{};
    mutable NodeTreeState state_{};
};

}  // namespace comdare::cache_engine
