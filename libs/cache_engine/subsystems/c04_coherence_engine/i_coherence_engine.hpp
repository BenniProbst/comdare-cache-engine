#pragma once
// C04 ICoherenceEngine — Cache-Coherence-Cost-Verwaltung (Block AN Kuehn)
// Termin 7 / 02_md §3 (MemoryAccessConcurrencyWrite) + 13_md §6 (Interconnect)

#include <cstdint>

namespace comdare::cache_engine::subsystems::coherence {

struct CoherenceMetrics {
    double cross_core_traffic_gbps = 0.0;
    double mesi_invalidation_rate  = 0.0;
    double avg_dirty_line_age_ns   = 0.0;
};

class ICoherenceEngine {
public:
    virtual ~ICoherenceEngine() = default;

    // Cost-Funktion (Block AN): f(node_depth, num_cores_sharing, cache_line_size)
    [[nodiscard]] virtual double compute_coherence_cost(std::uint16_t node_depth,
                                                        std::uint16_t num_cores_sharing,
                                                        std::uint16_t cache_line_size_bytes) const noexcept = 0;

    // Live-Beobachtung der Plattform
    [[nodiscard]] virtual CoherenceMetrics current_metrics() const noexcept = 0;

    // Empfehlung: ist ein Write momentan kostenguenstig?
    [[nodiscard]] virtual bool recommend_write_now(double cost_threshold) const noexcept = 0;
};

}  // namespace comdare::cache_engine::subsystems::coherence
