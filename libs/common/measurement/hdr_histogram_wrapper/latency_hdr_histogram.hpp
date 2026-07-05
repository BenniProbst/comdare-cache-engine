#pragma once

#include <cstdint>
#include <span>
#include <utility>

extern "C" {
#include <hdr/hdr_histogram.h>
}

namespace comdare::cache_engine::measurement {

class LatencyHdrHistogram {
    hdr_histogram* h_ = nullptr;

public:
    explicit LatencyHdrHistogram(std::int64_t lowest = 1, std::int64_t highest = 3'600'000'000'000LL,
                                 int significant_figures = 3) noexcept {
        if (hdr_init(lowest, highest, significant_figures, &h_) != 0) h_ = nullptr;
    }

    ~LatencyHdrHistogram() {
        if (h_ != nullptr) hdr_close(h_);
    }

    LatencyHdrHistogram(LatencyHdrHistogram&& other) noexcept : h_(std::exchange(other.h_, nullptr)) {}

    LatencyHdrHistogram& operator=(LatencyHdrHistogram&& other) noexcept {
        if (this != &other) {
            if (h_ != nullptr) hdr_close(h_);
            h_ = std::exchange(other.h_, nullptr);
        }
        return *this;
    }

    LatencyHdrHistogram(LatencyHdrHistogram const&)            = delete;
    LatencyHdrHistogram& operator=(LatencyHdrHistogram const&) = delete;

    void record(std::int64_t ns) noexcept {
        if (h_ != nullptr && ns > 0) (void)hdr_record_value(h_, ns);
    }

    [[nodiscard]] std::int64_t value_at(double q) const noexcept {
        if (h_ == nullptr || h_->total_count == 0) return 0;
        if (q < 0.0) q = 0.0;
        if (q > 1.0) q = 1.0;
        return hdr_value_at_percentile(h_, q * 100.0);
    }

    [[nodiscard]] std::int64_t p50() const noexcept { return value_at(0.50); }
    [[nodiscard]] std::int64_t p95() const noexcept { return value_at(0.95); }
    [[nodiscard]] std::int64_t p99() const noexcept { return value_at(0.99); }

    [[nodiscard]] std::int64_t min() const noexcept {
        return (h_ != nullptr && h_->total_count != 0) ? hdr_min(h_) : 0;
    }

    [[nodiscard]] std::int64_t max() const noexcept {
        return (h_ != nullptr && h_->total_count != 0) ? hdr_max(h_) : 0;
    }

    [[nodiscard]] std::int64_t count() const noexcept { return h_ != nullptr ? h_->total_count : 0; }

    [[nodiscard]] double mean() const noexcept { return (h_ != nullptr && h_->total_count != 0) ? hdr_mean(h_) : 0.0; }

    [[nodiscard]] static LatencyHdrHistogram from_samples(std::span<std::int64_t const> ns) noexcept {
        LatencyHdrHistogram hist;
        for (std::int64_t const value : ns) hist.record(value);
        return hist;
    }
};

} // namespace comdare::cache_engine::measurement