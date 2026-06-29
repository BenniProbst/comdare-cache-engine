#pragma once
// PerNodeCounter — P28 Original (WARNING: Cache-Coherence-Anti-Pattern Block AI)
// Termin 7 / 02_uml_cache_engine §4

#include <cache_engine/concepts/i_telemetry_strategy.hpp>

#include <atomic>
#include <vector>

namespace comdare::cache_engine {

class PerNodeCounter final : public ITelemetryStrategy {
public:
    explicit PerNodeCounter(std::size_t num_nodes) : counters_(num_nodes) {}

    [[nodiscard]] TelemetryStrategyKind kind() const noexcept override { return TelemetryStrategyKind::PerNodeCounter; }

    void increment(NodeId node_id) noexcept {
        if (node_id < counters_.size()) counters_[node_id].fetch_add(1, std::memory_order_relaxed);
    }

    [[nodiscard]] std::uint64_t value(NodeId node_id) const noexcept {
        if (node_id >= counters_.size()) return 0;
        return counters_[node_id].load(std::memory_order_relaxed);
    }

    [[nodiscard]] std::size_t size() const noexcept { return counters_.size(); }

private:
    std::vector<std::atomic<std::uint64_t>> counters_;
};

} // namespace comdare::cache_engine
