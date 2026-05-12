#pragma once
// CustomAllocation #1 - Append-only Buffer fuer 32B-MeasurementRecords (REV 7 §8.2.2)
//
// Pre-allocated, gross genug (z.B. 1 GiB), nie fail, nie erweiterbar.
// Append-only, lock-free pro Thread, 32-Byte aligned.

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <span>

namespace comdare::benchmark_suite {

inline constexpr std::size_t kMeasurementRecordBytes = 32;

struct alignas(32) MeasurementRecord32 {
    std::uint64_t timestamp_ns;
    std::uint64_t op_id;
    std::uint32_t op_kind;
    std::uint32_t flags;
    std::uint64_t cycles_or_value;
};
static_assert(sizeof(MeasurementRecord32) == 32);

class CustomAllocation1 {
public:
    explicit CustomAllocation1(std::size_t capacity_bytes = 1ULL << 30) {  // 1 GiB default
        capacity_records_ = capacity_bytes / sizeof(MeasurementRecord32);
        buffer_ = static_cast<MeasurementRecord32*>(
            std::aligned_alloc(alignof(MeasurementRecord32),
                               capacity_records_ * sizeof(MeasurementRecord32)));
        if (!buffer_) throw std::bad_alloc{};
        // Pre-touch (Pflicht: kein page-fault zur Laufzeit!)
        std::memset(buffer_, 0, capacity_records_ * sizeof(MeasurementRecord32));
    }

    ~CustomAllocation1() {
        std::free(buffer_);
    }

    CustomAllocation1(CustomAllocation1 const&) = delete;
    CustomAllocation1& operator=(CustomAllocation1 const&) = delete;

    // Lock-free append: returns slot-index oder UINT64_MAX bei Ueberlauf
    [[nodiscard]] std::uint64_t append(MeasurementRecord32 const& record) noexcept {
        std::uint64_t slot = next_slot_.fetch_add(1, std::memory_order_relaxed);
        if (slot >= capacity_records_) {
            return std::numeric_limits<std::uint64_t>::max();
        }
        buffer_[slot] = record;
        return slot;
    }

    [[nodiscard]] std::span<MeasurementRecord32 const> snapshot() const noexcept {
        std::uint64_t const used = next_slot_.load(std::memory_order_relaxed);
        std::uint64_t const safe_used = std::min<std::uint64_t>(used, capacity_records_);
        return std::span<MeasurementRecord32 const>{buffer_, safe_used};
    }

    [[nodiscard]] std::uint64_t records_used() const noexcept {
        return std::min<std::uint64_t>(next_slot_.load(std::memory_order_relaxed),
                                         capacity_records_);
    }

    [[nodiscard]] std::size_t capacity_records() const noexcept { return capacity_records_; }

private:
    std::size_t                capacity_records_ = 0;
    MeasurementRecord32*       buffer_           = nullptr;
    std::atomic<std::uint64_t> next_slot_{0};
};

}  // namespace comdare::benchmark_suite
