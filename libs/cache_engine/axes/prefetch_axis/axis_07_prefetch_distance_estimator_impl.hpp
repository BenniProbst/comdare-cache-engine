#pragma once
// V41.F.6.1.F.6 axis_07_prefetch DistanceEstimatorImpl (2026-05-29)
//
// @topic prefetch @achse 07 @impl distance_estimator
//
// **Herkunft:** F.6-Migration aus prt-art `prefetch/distance_estimator.hpp`
// (PrefetchDistanceEstimator, REV 6 §5.17). Density-basierte Prefetch-Distanz-
// Schaetzung mit Cache-Hierarchy-Tier-Latenz-Faktor — native Logik, die der
// metadaten-only-Wrapper DistanceEstimatorPrefetch (S01/PF1) bisher nur
// beschrieb. Hier als wiederverwendbare StrategyImpl verankert.
//
// Heuristik (unveraendert ggü. prt-art-Original):
//   sparseness     = 1 - density/100        (spaerliche Knoten → mehr Distanz)
//   latency_factor = tier_latency_cycles/10 (~1 bei L1, ~30 bei DRAM)
//   estimate       = clamp(1 + sparseness*8 + latency_factor, [1,16])
//
// Stateless + constexpr — keine Allokation, kein Runtime-State.

#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::prefetch_axis::impl {

/// Density- + Latenz-basierte Prefetch-Distanz-Heuristik (prt-art REV 6 §5.17).
struct DistanceEstimatorImpl {
    static constexpr std::uint8_t kMinDistance = 1;
    static constexpr std::uint8_t kMaxDistance = 16;

    /// Schaetzt Prefetch-Distanz (Cache-Lines im Voraus) aus Density [%] + Tier-Latenz [cycles].
    [[nodiscard]] static constexpr std::uint8_t estimate(double density_percent,
                                                         double tier_latency_cycles) noexcept {
        double sparseness     = 1.0 - (density_percent / 100.0);
        double latency_factor = tier_latency_cycles / 10.0;
        double estimate       = 1.0 + sparseness * 8.0 + latency_factor;
        if (estimate < static_cast<double>(kMinDistance)) estimate = kMinDistance;
        if (estimate > static_cast<double>(kMaxDistance)) estimate = kMaxDistance;
        return static_cast<std::uint8_t>(estimate);
    }

    /// Klemmt einen rohen Distanz-Wert in den gueltigen Bereich [kMinDistance, kMaxDistance].
    [[nodiscard]] static constexpr std::uint8_t clamp(int raw) noexcept {
        if (raw < kMinDistance) return kMinDistance;
        if (raw > kMaxDistance) return kMaxDistance;
        return static_cast<std::uint8_t>(raw);
    }
};

}  // namespace comdare::cache_engine::prefetch_axis::impl
