#pragma once
// MemoryWriteDiscipline — Write-only Memory Access (F6)
// Block AN: MemoryAccessConcurrencyWrite enthaelt CacheCoherenceCost-Funktion (Kuehn 2026-05-09)
// Termin 7 / 02_uml_cache_engine §3 + decision_trees/coherence_aware_write_decision_tree.hpp

#include <cache_engine/concepts/decision_trees/coherence_aware_write_decision_tree.hpp>
#include <cache_engine/concepts/i_concurrency_discipline.hpp>

namespace comdare::cache_engine {

class MemoryWriteDiscipline final : public IConcurrencyDiscipline {
public:
    [[nodiscard]] ConcurrencyDisciplineKind kind() const noexcept override {
        return ConcurrencyDisciplineKind::MemoryWrite;
    }

    void on_event(Event const& event) noexcept override {
        if (event.kind != EventKind::Write) return;
        // Block AN: erfasse Cost ohne den CoherenceTree zu bemuehen
        last_event_ = static_cast<WriteEvent const&>(event);
        last_cost_  = CoherenceAwareWriteDecisionTree::compute_cache_coherence_cost(last_event_);
    }

    // Block AN: Cost-Funktion (oeffentlich)
    [[nodiscard]] double compute_cache_coherence_cost(WriteEvent const& w) const noexcept {
        return CoherenceAwareWriteDecisionTree::compute_cache_coherence_cost(w);
    }

    [[nodiscard]] CoherenceAwareWriteDecisionTree& write_decision_tree() noexcept { return tree_; }

    [[nodiscard]] double            last_cost() const noexcept { return last_cost_; }
    [[nodiscard]] WriteEvent const& last_event() const noexcept { return last_event_; }

private:
    CoherenceAwareWriteDecisionTree tree_{};
    WriteEvent                      last_event_{};
    double                          last_cost_ = 0.0;
};

} // namespace comdare::cache_engine
