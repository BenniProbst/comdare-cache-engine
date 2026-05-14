#pragma once
// C01 ICostEngine — Kostenmodell-Aggregation (Hankins/Patel + Chen-Gibbons-Mowry + START)
// Termin 7 / 12_md §4.1 + 13_md §1+§2

#include <cstdint>
#include <string>

namespace comdare::cache_engine::subsystems::cost {

struct CostEstimate {
    double cycles_estimate = 0.0;
    double cache_lines_touched = 0.0;
    double tlb_pressure = 0.0;
    double branch_predictions = 0.0;
};

class ICostEngine {
public:
    virtual ~ICostEngine() = default;

    [[nodiscard]] virtual std::string name() const = 0;

    // Liefert eine Kostenschaetzung fuer eine konkrete Operation auf einer Zielseite.
    [[nodiscard]] virtual CostEstimate estimate(std::uint64_t target_page,
                                                std::uint16_t op_kind) const noexcept = 0;

    // Annahme akzeptieren / verwerfen — fuer Selbstkalibrierung.
    virtual void record_observed_cycles(double observed) noexcept = 0;
};

}  // namespace comdare::cache_engine::subsystems::cost
