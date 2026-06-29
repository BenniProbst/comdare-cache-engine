#pragma once
// C09 IHeuristikEngine — Aggregation der ~80 Paper-Heuristiken (12_md §4)
// Termin 7 / 12_md §4 + REV 5 §9

#include <cache_engine/platform/i_heuristic.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace comdare::cache_engine::subsystems::heuristik {

struct HeuristicEvaluation {
    std::string heuristic_name;
    double      result_value = 0.0;
    bool        applicable   = true;
};

class IHeuristikEngine {
public:
    virtual ~IHeuristikEngine() = default;

    // Heuristiken registrieren — z.B. OptimalNodeSizeHeuristic, AdaptivePrefetchDistance
    virtual void register_heuristic(comdare::cache_engine::platform::IHeuristic* h) = 0;
    virtual void unregister_all() noexcept                                          = 0;

    [[nodiscard]] virtual std::size_t heuristic_count() const noexcept = 0;

    // Massen-Auswertung: alle anwendbaren Heuristiken mit gleichen Inputs auswerten
    [[nodiscard]] virtual std::vector<HeuristicEvaluation> evaluate_all(std::vector<double> const& inputs) const = 0;
};

} // namespace comdare::cache_engine::subsystems::heuristik
