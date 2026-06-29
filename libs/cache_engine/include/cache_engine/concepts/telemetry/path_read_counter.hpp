#pragma once
// PathReadCounter — P26 Zhang FGCS Hot-Path-Detection per Block-Counter
// Termin 7 / 02_uml_cache_engine §4

#include <cache_engine/concepts/i_telemetry_strategy.hpp>

#include <atomic>
#include <unordered_map>

namespace comdare::cache_engine {

class PathReadCounter final : public ITelemetryStrategy {
public:
    explicit PathReadCounter(std::uint64_t hot_threshold = 100) : hot_threshold_(hot_threshold) {}

    [[nodiscard]] TelemetryStrategyKind kind() const noexcept override {
        return TelemetryStrategyKind::PathReadCounter;
    }

    void record_block_read(std::uint64_t block_id) noexcept { ++counters_[block_id]; }

    [[nodiscard]] std::uint64_t block_read_count(std::uint64_t block_id) const noexcept {
        auto it = counters_.find(block_id);
        return it == counters_.end() ? 0u : it->second;
    }

    [[nodiscard]] bool is_hot(std::uint64_t block_id) const noexcept {
        return block_read_count(block_id) >= hot_threshold_;
    }

    [[nodiscard]] std::uint64_t hot_threshold() const noexcept { return hot_threshold_; }

private:
    std::uint64_t                                    hot_threshold_;
    std::unordered_map<std::uint64_t, std::uint64_t> counters_;
};

} // namespace comdare::cache_engine
