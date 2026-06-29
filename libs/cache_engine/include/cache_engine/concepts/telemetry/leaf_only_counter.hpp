#pragma once
// LeafOnlyCounter — Kuehn NEU 2026-05-08 (Block AJ)
// Counter NUR in Blatt-Knoten — vermeidet Cache-Coherence-Anti-Pattern
// Termin 7 / 02_uml_cache_engine §4
// Pflichtbegleiter: RetroactiveAggregation fuer innere Knoten

#include <cache_engine/concepts/i_telemetry_strategy.hpp>

#include <atomic>
#include <vector>

namespace comdare::cache_engine {

class LeafOnlyCounter final : public ITelemetryStrategy {
public:
    explicit LeafOnlyCounter(std::size_t num_leaves) : leaf_counters_(num_leaves) {}

    [[nodiscard]] TelemetryStrategyKind kind() const noexcept override {
        return TelemetryStrategyKind::LeafOnlyCounter;
    }

    void increment_leaf(LeafId leaf_id) noexcept {
        if (leaf_id < leaf_counters_.size()) leaf_counters_[leaf_id].fetch_add(1, std::memory_order_relaxed);
    }

    [[nodiscard]] std::uint64_t leaf_value(LeafId leaf_id) const noexcept {
        if (leaf_id >= leaf_counters_.size()) return 0;
        return leaf_counters_[leaf_id].load(std::memory_order_relaxed);
    }

    [[nodiscard]] std::size_t leaf_count() const noexcept { return leaf_counters_.size(); }

private:
    std::vector<std::atomic<std::uint64_t>> leaf_counters_;
};

} // namespace comdare::cache_engine
