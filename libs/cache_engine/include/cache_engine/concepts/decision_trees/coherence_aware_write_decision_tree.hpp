#pragma once
// CoherenceAwareWriteDecisionTree — Block AN Kuehn (NEU 2026-05-09)
// MemoryAccessConcurrency::Write Cost-Funktion
// Termin 7 / 02_uml_cache_engine §2

#include <cache_engine/concepts/i_decision_lambda_tree.hpp>

namespace comdare::cache_engine {

class CoherenceAwareWriteDecisionTree final : public IDecisionLambdaTree<WriteEvent> {
public:
    void configure(NodeTreeConfig const& config) override { config_ = config; }

    // Cost-Berechnung (oeffentlich fuer Heuristik-Konsumenten)
    [[nodiscard]] static double compute_cache_coherence_cost(WriteEvent const& w) noexcept {
        // f(node_depth_from_root, num_cores_sharing_node, write_frequency, cache_line_size)
        double depth_factor = 1.0 / static_cast<double>(w.node_depth + 1);
        double cores_factor = static_cast<double>(w.num_cores_sharing);
        double cl_factor    = static_cast<double>(w.cache_line_size_bytes) / 64.0;
        return depth_factor * cores_factor * cl_factor;
    }

    [[nodiscard]] Decision evaluate(WriteEvent const& event, DecisionContext const& ctx) const noexcept override {
        ++state_.total_evaluations;
        double cost = compute_cache_coherence_cost(event);
        // SKIP bei Wurzelnaehe + Multi-Core-Last
        if (event.node_depth <= 1 && event.num_cores_sharing > 8) {
            ++state_.total_skips;
            return Decision::SKIP;
        }
        // DELAY bei hoher Cost (warten bis ConsolidationBarrier)
        if (cost > ctx.cost_threshold) {
            ++state_.total_delays;
            return Decision::DELAY;
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
