#pragma once
// C05 ITelemetryEngine — Aggregation der Achse-11-Strategien (Kuehn 2026-05-09)
// Termin 7 / 02_md §4

#include <cache_engine/concepts/i_telemetry_strategy.hpp>

#include <cstdint>
#include <vector>

namespace comdare::cache_engine::subsystems::telemetry {

class ITelemetryEngine {
public:
    virtual ~ITelemetryEngine() = default;

    // Strategien registrieren — eine Engine kann mehrere Strategien verwalten
    virtual void register_strategy(comdare::cache_engine::ITelemetryStrategy* s) = 0;
    virtual void unregister_all() noexcept = 0;

    [[nodiscard]] virtual std::size_t strategy_count() const noexcept = 0;

    // Aggregation: liefert pro registrierter Strategy einen Snapshot-Wert
    [[nodiscard]] virtual std::vector<std::uint64_t> snapshot_values() const = 0;
};

}  // namespace comdare::cache_engine::subsystems::telemetry
