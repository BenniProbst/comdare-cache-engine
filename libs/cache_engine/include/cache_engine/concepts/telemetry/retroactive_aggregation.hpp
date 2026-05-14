#pragma once
// RetroactiveAggregation — Kuehn NEU 2026-05-08 (Block AK, BARRIER)
// Wurzel-Up-Traversal vor Reordering — laeuft Single-Threaded
// Termin 7 / 02_uml_cache_engine §4

#include <cache_engine/concepts/event.hpp>
#include <cache_engine/concepts/i_telemetry_strategy.hpp>

#include <vector>

namespace comdare::cache_engine {

struct AggregatedHistogram {
    std::vector<std::uint64_t> per_inner_node_count;
    std::uint64_t              total_aggregations = 0;
};

class RetroactiveAggregation final : public ITelemetryStrategy {
public:
    [[nodiscard]] TelemetryStrategyKind kind() const noexcept override {
        return TelemetryStrategyKind::RetroactiveAggregation;
    }

    // Iterativer Walk; sammelt Blatt-Counter zu inneren Knoten.
    // Wir nehmen hier eine simple Bottom-Up-Aggregation an: Summe der Blatt-Counter.
    AggregatedHistogram run_aggregation(std::vector<std::uint64_t> const& leaf_counts,
                                         std::vector<std::size_t> const& leaf_to_inner) {
        AggregatedHistogram h{};
        std::uint64_t inner_max = 0;
        for (auto idx : leaf_to_inner) {
            if (idx >= inner_max) inner_max = idx + 1;
        }
        h.per_inner_node_count.assign(inner_max, 0);
        for (std::size_t i = 0; i < leaf_counts.size() && i < leaf_to_inner.size(); ++i) {
            h.per_inner_node_count[leaf_to_inner[i]] += leaf_counts[i];
        }
        h.total_aggregations = ++total_runs_;
        return h;
    }

    void on_consolidation_barrier(ConsolidationBarrierEvent const&) noexcept {
        // Trigger fuer Aggregation: laeuft single-threaded
        ++barrier_triggers_;
    }

    [[nodiscard]] std::uint64_t total_runs() const noexcept { return total_runs_; }
    [[nodiscard]] std::uint64_t barrier_triggers() const noexcept { return barrier_triggers_; }

private:
    std::uint64_t total_runs_ = 0;
    std::uint64_t barrier_triggers_ = 0;
};

}  // namespace comdare::cache_engine
