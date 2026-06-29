#pragma once
// ThreadArena — cache-line aligned, append-only per-thread Arena
// Termin 7 / 06_uml_persistence §2

#include <cache_engine/measurement/measurement_record.hpp>

#include <cstdint>
#include <thread>
#include <vector>

namespace comdare::cache_engine::measurement {

class alignas(64) ThreadArena {
public:
    ThreadArena() = default;
    explicit ThreadArena(std::thread::id tid) : thread_id_(tid) {}

    void append(MeasurementRecord const& record) noexcept {
        records_.push_back(record);
        bytes_consumed_ += sizeof(MeasurementRecord);
    }

    [[nodiscard]] std::thread::id                       thread_id() const noexcept { return thread_id_; }
    [[nodiscard]] std::size_t                           size() const noexcept { return records_.size(); }
    [[nodiscard]] std::size_t                           bytes() const noexcept { return bytes_consumed_; }
    [[nodiscard]] std::vector<MeasurementRecord> const& records() const noexcept { return records_; }

    void reset() noexcept {
        records_.clear();
        bytes_consumed_ = 0;
    }

private:
    std::thread::id                thread_id_{};
    std::vector<MeasurementRecord> records_{};
    std::size_t                    bytes_consumed_ = 0;
};

} // namespace comdare::cache_engine::measurement
