#pragma once
// ITelemetryStrategy — Achse 11 (NEU 2026-05-09 Kuehn)
// Termin 7 / 02_uml_cache_engine §4

#include <cache_engine/concepts/event.hpp>

#include <cstdint>

namespace comdare::cache_engine {

enum class TelemetryStrategyKind : std::uint8_t {
    PerNodeCounter        = 0,   // P28 Original (WARNING: Cache-Coherence-Anti-Pattern)
    LeafOnlyCounter       = 1,   // Kuehn NEU 2026-05-08
    LeafOnlySampledCounter= 2,   // Kuehn NEU 2026-05-08 (parametrisiert<N>)
    RetroactiveAggregation= 3,   // Kuehn NEU 2026-05-08 (BARRIER)
    PathReadCounter       = 4,   // P26 Zhang FGCS
    ProbabilityHints      = 5,   // PRT-ART eigen T4
};

class ITelemetryStrategy {
public:
    virtual ~ITelemetryStrategy() = default;

    [[nodiscard]] virtual TelemetryStrategyKind kind() const noexcept = 0;
};

}  // namespace comdare::cache_engine
