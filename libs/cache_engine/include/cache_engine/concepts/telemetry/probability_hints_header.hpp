#pragma once
// ProbabilityHintsHeader — PRT-ART eigen T4
// Quantisierte Wahrscheinlichkeits-Sketches im Page-Header
// Termin 7 / 02_uml_cache_engine §4

#include <cache_engine/concepts/i_telemetry_strategy.hpp>

#include <array>
#include <cstdint>

namespace comdare::cache_engine {

template <std::size_t BitLayoutBytes = 16>
class ProbabilityHintsHeader final : public ITelemetryStrategy {
    static_assert(BitLayoutBytes > 0, "BitLayoutBytes muss > 0 sein");

public:
    [[nodiscard]] TelemetryStrategyKind kind() const noexcept override {
        return TelemetryStrategyKind::ProbabilityHints;
    }

    void record_access(std::uint8_t byte) noexcept {
        std::size_t idx = byte % BitLayoutBytes;
        if (hint_bits_[idx] < 255) ++hint_bits_[idx];
    }

    [[nodiscard]] std::uint8_t hint(std::uint8_t byte) const noexcept { return hint_bits_[byte % BitLayoutBytes]; }

    [[nodiscard]] std::array<std::uint8_t, BitLayoutBytes> const& bits() const noexcept { return hint_bits_; }

    [[nodiscard]] static constexpr std::size_t layout_bytes() noexcept { return BitLayoutBytes; }

private:
    std::array<std::uint8_t, BitLayoutBytes> hint_bits_{};
};

} // namespace comdare::cache_engine
