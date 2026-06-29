#pragma once
// C03 IPrefetchEngine — Software-Prefetch-Steuerung (P21/P22/P23/P25/P26/P27)
// Termin 7 / 11_md F5 + 02_md PrefetchAdjustmentTree

#include <cstdint>

namespace comdare::cache_engine::subsystems::prefetch {

enum class PrefetchPolicy : std::uint8_t {
    None               = 0,
    SoftwareFixed      = 1, // P21
    AdaptiveDistance   = 2, // P23
    HierarchicalBundle = 3, // P27
    FillBufferAware    = 4, // P25
    HotPath            = 5, // P26
    JumpPointerArray   = 6, // P21/P22
};

class IPrefetchEngine {
public:
    virtual ~IPrefetchEngine() = default;

    [[nodiscard]] virtual PrefetchPolicy policy() const noexcept = 0;

    // Prefetch fuer Adresse + Distanz (No-op falls nicht aktiviert)
    virtual void prefetch(std::uint64_t addr, std::uint8_t distance_lines) noexcept = 0;

    // Live-Update der Distanz (P23 adaptive)
    virtual void update_distance(std::uint8_t new_distance) noexcept = 0;

    [[nodiscard]] virtual std::uint8_t  current_distance() const noexcept        = 0;
    [[nodiscard]] virtual std::uint64_t total_prefetches_issued() const noexcept = 0;
};

} // namespace comdare::cache_engine::subsystems::prefetch
