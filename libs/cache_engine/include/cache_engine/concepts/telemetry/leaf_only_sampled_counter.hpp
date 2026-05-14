#pragma once
// LeafOnlySampledCounter<N> — Kuehn NEU 2026-05-08 (Block AL, parametrisiert)
// Counter nur in Blaettern, nur jeder N-te Zugriff
// Termin 7 / 02_uml_cache_engine §4

#include <cache_engine/concepts/i_telemetry_strategy.hpp>

#include <atomic>
#include <vector>

namespace comdare::cache_engine {

template <std::size_t SamplingRateN = 1000>
class LeafOnlySampledCounter final : public ITelemetryStrategy {
    static_assert(SamplingRateN > 0, "SamplingRateN muss > 0 sein");

public:
    explicit LeafOnlySampledCounter(std::size_t num_leaves)
        : leaf_counters_(num_leaves) {}

    [[nodiscard]] TelemetryStrategyKind kind() const noexcept override {
        return TelemetryStrategyKind::LeafOnlySampledCounter;
    }

    [[nodiscard]] static constexpr std::size_t sampling_rate() noexcept {
        return SamplingRateN;
    }

    // if (++sample_counter % SamplingRateN == 0) leaf_counters[leaf_id]++
    void maybe_increment_leaf(LeafId leaf_id) noexcept {
        std::size_t prev = sample_counter_.fetch_add(1, std::memory_order_relaxed) + 1;
        if ((prev % SamplingRateN) != 0) return;
        if (leaf_id < leaf_counters_.size())
            leaf_counters_[leaf_id].fetch_add(1, std::memory_order_relaxed);
    }

    [[nodiscard]] std::uint64_t leaf_value(LeafId leaf_id) const noexcept {
        if (leaf_id >= leaf_counters_.size()) return 0;
        return leaf_counters_[leaf_id].load(std::memory_order_relaxed);
    }

    [[nodiscard]] std::size_t leaf_count() const noexcept {
        return leaf_counters_.size();
    }

    [[nodiscard]] std::size_t sample_position() const noexcept {
        return sample_counter_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<std::size_t>                sample_counter_{0};
    std::vector<std::atomic<std::uint64_t>> leaf_counters_;
};

}  // namespace comdare::cache_engine
