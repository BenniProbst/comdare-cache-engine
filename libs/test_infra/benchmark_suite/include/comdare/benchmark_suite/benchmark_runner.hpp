#pragma once
// BenchmarkRunner - No-deprecate Wrapper aller Testmethoden (REV 7 §8.2.1)
//
// API-Garantie: NIEMALS aendern (no-deprecate).
// Akkumuliert Messdaten in 2 separaten Custom-Allokationen.

#include "custom_allocation_1_measurements.hpp"
#include "custom_allocation_2_state_log.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>

namespace comdare::benchmark_suite {

enum class EventKind : std::uint32_t {
    BeginPhase   = 0,
    EndPhase     = 1,
    CacheMiss    = 2,
    Allocation   = 3,
    Deallocation = 4,
    NodeSplit    = 5,
    NodeMerge    = 6,
    Custom       = 7,
};

class BenchmarkRunner {
public:
    using id_t = std::uint64_t;

    explicit BenchmarkRunner(std::size_t measurements_capacity_bytes = 1ULL << 30,
                             std::size_t state_log_capacity_bytes    = 64ULL << 20)
        : measurements_{measurements_capacity_bytes}, state_log_{state_log_capacity_bytes} {}

    // Pflicht-API (no-deprecate Garantie)
    [[nodiscard]] id_t begin_measurement(std::string_view tag) noexcept {
        id_t                handle = next_handle_.fetch_add(1, std::memory_order_relaxed);
        MeasurementRecord32 rec{};
        rec.timestamp_ns    = now_ns();
        rec.op_id           = handle;
        rec.op_kind         = static_cast<std::uint32_t>(EventKind::BeginPhase);
        rec.cycles_or_value = 0;
        measurements_.append(rec);
        return handle;
    }

    void record_event(id_t handle, EventKind kind, std::uint64_t aux = 0) noexcept {
        MeasurementRecord32 rec{};
        rec.timestamp_ns    = now_ns();
        rec.op_id           = handle;
        rec.op_kind         = static_cast<std::uint32_t>(kind);
        rec.cycles_or_value = aux;
        measurements_.append(rec);
    }

    void end_measurement(id_t handle, std::uint64_t observed_cycles) noexcept {
        MeasurementRecord32 rec{};
        rec.timestamp_ns    = now_ns();
        rec.op_id           = handle;
        rec.op_kind         = static_cast<std::uint32_t>(EventKind::EndPhase);
        rec.cycles_or_value = observed_cycles;
        measurements_.append(rec);
    }

    void log_sparse_state(std::uint8_t marker, std::span<std::byte const> delta) noexcept {
        state_log_.push_state(marker, delta);
    }

    // Conversion (Phase 8 — NUR Post-Experiment): binary blob -> handy formats
    void flush_to_binary_blob(std::filesystem::path const& output) const;

    [[nodiscard]] std::uint64_t records_collected() const noexcept { return measurements_.records_used(); }
    [[nodiscard]] std::size_t   state_log_bytes() const noexcept { return state_log_.bytes_used(); }

    [[nodiscard]] CustomAllocation1 const& measurements() const noexcept { return measurements_; }
    [[nodiscard]] CustomAllocation2 const& state_log() const noexcept { return state_log_; }

private:
    [[nodiscard]] static std::uint64_t now_ns() noexcept {
        auto const t = std::chrono::steady_clock::now().time_since_epoch();
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t).count());
    }

    CustomAllocation1          measurements_;
    CustomAllocation2          state_log_;
    std::atomic<std::uint64_t> next_handle_{0};
};

} // namespace comdare::benchmark_suite
