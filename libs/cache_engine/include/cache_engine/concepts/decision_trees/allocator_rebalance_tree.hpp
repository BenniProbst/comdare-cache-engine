#pragma once
// AllocatorRebalanceTree<AllocatorT> — Pool-Per-PageType ↔ Arena-Per-Subtree
// Termin 7 / 02_uml_cache_engine §2

#include <cache_engine/concepts/i_decision_lambda_tree.hpp>

namespace comdare::cache_engine {

template <typename AllocatorT>
class AllocatorRebalanceTree final : public IDecisionLambdaTree<PageRelocationEvent> {
public:
    void configure(NodeTreeConfig const& config) override { config_ = config; }

    [[nodiscard]] Decision evaluate(PageRelocationEvent const& event,
                                    DecisionContext const&) const noexcept override {
        ++state_.total_evaluations;
        // Bei Page-Relocation auf andere Allocator-Pool umstellen wenn load_factor stark abweicht
        if (event.load_factor < 0.10 || event.load_factor > 0.90) {
            ++state_.total_executes;
            return Decision::EXECUTE;
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
