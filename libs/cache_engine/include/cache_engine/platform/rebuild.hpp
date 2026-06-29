#pragma once
// IRebuildScheduler + IRebuildCostModel
// Termin 7 / REV 5 K07 — fuer adaptive Layout-Reorganisation (P05 START, P19 Saikkonen)

#include <cstdint>

namespace comdare::cache_engine::platform {

enum class RebuildTrigger : std::uint8_t {
    OfflineSelfTuning      = 0, // P05 START
    OnlineProbabilityBased = 1, // P28 Kuehn / P26 Zhang
    OnEveryStructureMod    = 2, // P19 Saikkonen Local Relocation
    PeriodicGlobal         = 3, // P18 Saikkonen
    BulkLoadLevelByLevel   = 4, // P11/P12/P14
};

class IRebuildScheduler {
public:
    virtual ~IRebuildScheduler() = default;

    [[nodiscard]] virtual RebuildTrigger trigger() const noexcept             = 0;
    [[nodiscard]] virtual std::uint64_t  next_rebuild_due_ns() const noexcept = 0;
    virtual void                         schedule_rebuild() noexcept          = 0;
    virtual void                         on_completed() noexcept              = 0;
    [[nodiscard]] virtual std::uint64_t  total_rebuilds() const noexcept      = 0;
};

class IRebuildCostModel {
public:
    virtual ~IRebuildCostModel() = default;

    // Erwartete Kosten + erwarteter Nutzen einer Reorganisation (Cycles)
    [[nodiscard]] virtual double expected_cost_cycles() const noexcept    = 0;
    [[nodiscard]] virtual double expected_benefit_cycles() const noexcept = 0;
    [[nodiscard]] virtual double payback_query_count() const noexcept     = 0;
    [[nodiscard]] virtual bool   should_rebuild() const noexcept          = 0;
};

} // namespace comdare::cache_engine::platform
