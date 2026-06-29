#pragma once
// platform_snapshot.hpp - REV 5.2 Immutable Plattform-Snapshot
// Quelle: U09 (UML) - by-value weitergegeben an Sub-Engines

#include "cache_engine/concepts/pressure_state.hpp"

#include <chrono>
#include <cstdint>

namespace comdare::cache_engine {

/// HwCounters - PMU-Werte (Telemetry-Snapshot)
struct HwCounters {
    std::uint64_t cycles         = 0;
    std::uint64_t instructions   = 0;
    std::uint64_t cache_misses   = 0; ///< gesamtgeschaetzt
    std::uint64_t dtlb_misses    = 0;
    std::uint64_t branch_misses  = 0;
    std::uint32_t fb_full_events = 0; ///< Intel L1D_PEND_MISS.FB_FULL (P25)
    double        mpki() const noexcept {
        return (instructions > 0) ? (1000.0 * static_cast<double>(cache_misses) / static_cast<double>(instructions))
                                  : 0.0;
    }
};

/// NumaTopologySnapshot - kompakter NUMA-Snapshot (volle Topo siehe ICacheTopology)
struct NumaTopologySnapshot {
    std::uint8_t numa_node_count            = 1;
    std::uint8_t current_socket             = 0;
    double       local_remote_latency_ratio = 1.0;
};

/// PlatformSnapshot - by-value immutable, an alle Sub-Engines verteilt
struct PlatformSnapshot {
    state::PressureState                  pressure = state::Idle{};
    HwCounters                            counters{};
    NumaTopologySnapshot                  topology{};
    std::chrono::steady_clock::time_point taken_at = std::chrono::steady_clock::now();
};

} // namespace comdare::cache_engine
