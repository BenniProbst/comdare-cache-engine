#pragma once
// SamplingRateAdjustmentTree<TelemetryT> — Block AM Kuehn (NEU 2026-05-09)
// Adaptive N-Anpassung fuer LeafOnlySampledCounter<N>
// Termin 7 / 02_uml_cache_engine §2

#include <cache_engine/concepts/i_decision_lambda_tree.hpp>

#include <atomic>

namespace comdare::cache_engine {

template <typename TelemetryT>
class SamplingRateAdjustmentTree final : public IDecisionLambdaTree<SamplingEvent> {
public:
    SamplingRateAdjustmentTree() : current_n_(1000) {}

    void configure(NodeTreeConfig const& config) override { config_ = config; }

    [[nodiscard]] Decision evaluate(SamplingEvent const& event, DecisionContext const&) const noexcept override {
        ++state_.total_evaluations;
        // Hoher CPU-Load -> N erhoehen (weniger sampling) -> EXECUTE
        if (event.cpu_load > 0.85) {
            ++state_.total_executes;
            return Decision::EXECUTE;
        }
        // Niedriger Load + hohe Variabilitaet -> N reduzieren -> EXECUTE
        if (event.cpu_load < 0.30 && event.cache_miss_rate > 0.10) {
            ++state_.total_executes;
            return Decision::EXECUTE;
        }
        ++state_.total_skips;
        return Decision::SKIP;
    }

    void adjust_n(std::size_t target_n) noexcept { current_n_.store(target_n, std::memory_order_relaxed); }

    [[nodiscard]] std::size_t current_n() const noexcept { return current_n_.load(std::memory_order_relaxed); }

    [[nodiscard]] NodeTreeState save_state() const noexcept override { return state_; }
    void                        restore_state(NodeTreeState const& s) noexcept override { state_ = s; }

private:
    NodeTreeConfig           config_{};
    std::atomic<std::size_t> current_n_;
    mutable NodeTreeState    state_{};
};

} // namespace comdare::cache_engine
