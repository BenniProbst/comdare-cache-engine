#pragma once
// pressure_state.hpp - REV 5.2 State-Pattern via std::variant
// Quelle: U09 (UML), extract_state_visitor_per_paper.md, extract_state_visitor_pattern.md

#include <chrono>
#include <cstdint>
#include <variant>

namespace comdare::cache_engine::state {

// -----------------------------------------------------------------------------
// 5 Plattform-Zustaende - aus 33-Paper-Tieflektuere abgeleitet
// -----------------------------------------------------------------------------

/// Idle: kein Druck, Cache-Auslastung < 30 %. Default-Initialzustand.
struct Idle {
    std::uint64_t last_check_ns = 0;
};

/// Warmup: Cache fuellt sich, MPKI sinkt. Khan 2010 Live-Block-Phase.
struct Warmup {
    double      mpki         = 0.0; ///< Misses per kilo-instruction
    std::size_t pages_loaded = 0;
};

/// Saturated: Bandbreite limitiert, MSHR-Stalls.
/// Mahling 2025 weak-reliability-Schwelle.
struct Saturated {
    double        bandwidth_util = 0.0; ///< 0..1
    std::uint32_t mshr_pressure  = 0;
};

/// CoherenceStorm: Inter-Socket-Traffic > Threshold.
/// Block AN (Kuehn-Mail 2026-05-08), Masstree P03 cross-socket.
struct CoherenceStorm {
    std::uint32_t inter_socket_msgs_per_us = 0;
    std::uint8_t  affected_sockets         = 0;
};

/// Recovery: nach Burst, Caches kuehlen ab.
/// Naderan-Tahan 2016 negative-state.
struct Recovery {
    std::chrono::milliseconds remaining{0};
};

// -----------------------------------------------------------------------------
// PressureState - moderne C++23 std::variant statt klassischer GoF-Hierarchie
// -----------------------------------------------------------------------------
using PressureState = std::variant<Idle, Warmup, Saturated, CoherenceStorm, Recovery>;

} // namespace comdare::cache_engine::state
