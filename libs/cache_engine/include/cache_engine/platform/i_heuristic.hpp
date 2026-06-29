#pragma once
// IHeuristic — Wert-Berechnungen auf dem Live-Modell (REV 2 §2.6)
// Termin 7 / 12_algorithmus_strategie_taxonomie §4 (~80 paper-abgeleitete Heuristiken)

#include <cstdint>
#include <string>
#include <vector>

namespace comdare::cache_engine::platform {

enum class HeuristicCategory : std::uint8_t {
    StaticCostModel    = 0, // Hankins/Patel, Chen-Gibbons-Mowry
    AdaptiveRuntime    = 1, // Khan, Mahling
    WorkloadAdaptation = 2, // P20, P26
    LayoutInvariant    = 3, // P19 Saikkonen, P16 Bender
    NegativeFinding    = 4, // P24 Naderan, P28 Kuehn LeafOnly
    TelemetryDriven    = 5, // P26 Zhang, P28 Kuehn
    HardwareProbe      = 6, // P05 START clflush, P32 To-Stride
};

// IHeuristic ist generisch — input/output sind doubles + ID; konkrete Heuristiken sind frei
class IHeuristic {
public:
    virtual ~IHeuristic() = default;

    [[nodiscard]] virtual std::string       name() const              = 0;
    [[nodiscard]] virtual HeuristicCategory category() const noexcept = 0;

    // Generic API: input → output (z.B. 2 Inputs → 1 Output)
    [[nodiscard]] virtual double compute(std::vector<double> const& inputs) const = 0;

    // Pflicht-Anzahl Inputs (validation)
    [[nodiscard]] virtual std::size_t expected_input_count() const noexcept = 0;
};

} // namespace comdare::cache_engine::platform
