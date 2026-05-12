#pragma once
// CustomAllocation #2 - Sparse-Serialized-Byte-States Log (REV 7 §8.2.2 + §8.2.3)
//
// Pre-allocated, separate von #1. Sparse-Repraesentation:
//   [state_marker:1B] [delta_length:varint] [delta_bytes:variable]
// Pro Test-Phase ein State-Marker. Reduziert OS-Einfluss auf Minimum.

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <span>

namespace comdare::benchmark_suite {

class CustomAllocation2 {
public:
    explicit CustomAllocation2(std::size_t capacity_bytes = 64ULL * 1024 * 1024) {  // 64 MiB
        capacity_bytes_ = capacity_bytes;
        buffer_ = static_cast<std::byte*>(bs_aligned_alloc(64, capacity_bytes_));
        if (!buffer_) throw std::bad_alloc{};
        std::memset(buffer_, 0, capacity_bytes_);
    }

    ~CustomAllocation2() {
        bs_aligned_free(buffer_);
    }

    CustomAllocation2(CustomAllocation2 const&) = delete;
    CustomAllocation2& operator=(CustomAllocation2 const&) = delete;

    // Sparse byte state push: marker + varint length + delta bytes
    [[nodiscard]] bool push_state(std::uint8_t state_marker,
                                   std::span<std::byte const> delta) noexcept
    {
        // Worst-case Pflicht-Bytes: 1 (marker) + 9 (varint max) + delta.size()
        std::size_t const required = 10 + delta.size();
        std::size_t const offset   = next_offset_.fetch_add(required, std::memory_order_relaxed);
        if (offset + required > capacity_bytes_) {
            return false;  // Overflow
        }
        // 1. Marker
        buffer_[offset] = static_cast<std::byte>(state_marker);
        std::size_t cursor = offset + 1;
        // 2. Varint length
        std::uint64_t value = delta.size();
        while (value >= 0x80) {
            buffer_[cursor++] = static_cast<std::byte>((value & 0x7F) | 0x80);
            value >>= 7;
        }
        buffer_[cursor++] = static_cast<std::byte>(value & 0x7F);
        // 3. Delta payload
        std::memcpy(buffer_ + cursor, delta.data(), delta.size());
        return true;
    }

    [[nodiscard]] std::span<std::byte const> snapshot() const noexcept {
        std::uint64_t const used = (std::min)(
            next_offset_.load(std::memory_order_relaxed),
            static_cast<std::uint64_t>(capacity_bytes_));
        return std::span<std::byte const>{buffer_, used};
    }

    [[nodiscard]] std::size_t bytes_used() const noexcept {
        return static_cast<std::size_t>((std::min)(
            next_offset_.load(std::memory_order_relaxed),
            static_cast<std::uint64_t>(capacity_bytes_)));
    }

    [[nodiscard]] std::size_t capacity_bytes() const noexcept { return capacity_bytes_; }

private:
    std::size_t                capacity_bytes_ = 0;
    std::byte*                 buffer_         = nullptr;
    std::atomic<std::uint64_t> next_offset_{0};
};

}  // namespace comdare::benchmark_suite
