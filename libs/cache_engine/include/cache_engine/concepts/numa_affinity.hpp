#pragma once
// V32.EE.5 (2026-05-18 spaet) - Achse 6.3 NUMA-Affinity (Erweiterung)
//
// @achse 6.3
// @subsystem CE
// @reuse_status (b)
//
// Sub-Achse von Achse 6 ALLOCATOR (N-Phase Split).

#include "hardware_strategy.hpp" // NumaStrategy enum
#include <cstdint>

namespace comdare::cache_engine::concepts {

/**
 * @brief INumaAffinity - Sub-Achse 6.3 (Algorithmus-spez. NUMA-Wahl)
 * @achse 6.3
 * @subsystem CE
 *
 * Allokator-Sub-Strategy. Konsumiert NumaStrategy aus hardware_strategy.hpp.
 */
class INumaAffinity {
public:
    [[nodiscard]] virtual NumaStrategy preferred_strategy() const noexcept = 0;
    virtual ~INumaAffinity()                                               = default;
};

} // namespace comdare::cache_engine::concepts
