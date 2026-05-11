#pragma once
// CacheCoherenceDetectionTree — Block AI Kuehn (NEU 2026-05-09)
// Anti-Pattern-Erkennung fuer PerNodeCounter-Updates
// Termin 7 / 02_uml_cache_engine §2

#include <cache_engine/concepts/i_decision_lambda_tree.hpp>

namespace comdare::cache_engine {

class CacheCoherenceDetectionTree final : public IDecisionLambdaTree<TelemetryUpdateEvent> {
public:
    void configure(NodeTreeConfig const& config) override { config_ = config; }

    // SKIP Per-Node-Counter-Updates an Wurzelnaehe-Knoten unter Multi-Core
    // (pro Knoten-Tiefe + cores)
    [[nodiscard]] Decision evaluate(TelemetryUpdateEvent const& event,
                                    DecisionContext const&) const noexcept override {
        ++state_.total_evaluations;
        // Heuristic: an PerNodeCounter (telemetry_strategy == 0) bei tiefer Verschachtelung
        // cores tracken wir nicht direkt im Event — wir nehmen counter_delta>1 als Proxy
        if (event.telemetry_strategy == 0 && event.counter_delta >= 1 && event.node <= 16) {
            ++state_.total_skips;
            return Decision::SKIP;
        }
        ++state_.total_executes;
        return Decision::EXECUTE;
    }

    [[nodiscard]] NodeTreeState save_state() const noexcept override { return state_; }
    void restore_state(NodeTreeState const& s) noexcept override { state_ = s; }

private:
    NodeTreeConfig config_{};
    mutable NodeTreeState state_{};
};

}  // namespace comdare::cache_engine
