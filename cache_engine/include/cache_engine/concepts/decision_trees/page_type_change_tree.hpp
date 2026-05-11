#pragma once
// PageTypeChangeTree<PageT> — z.B. Node4 -> Node16 -> Node48 -> Node256 (P01)
// Termin 7 / 02_uml_cache_engine §2

#include <cache_engine/concepts/i_decision_lambda_tree.hpp>

namespace comdare::cache_engine {

template <typename PageT>
class PageTypeChangeTree final : public IDecisionLambdaTree<PageTypeChangeEvent> {
public:
    void configure(NodeTreeConfig const& config) override { config_ = config; }

    [[nodiscard]] Decision evaluate(PageTypeChangeEvent const& event,
                                    DecisionContext const&) const noexcept override {
        ++state_.total_evaluations;
        if (event.current_type == event.suggested_type) {
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
