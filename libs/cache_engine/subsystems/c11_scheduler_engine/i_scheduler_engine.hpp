#pragma once
// C11 ISchedulerEngine — Page-Type-Wechsel + Operations-Scheduling
// Termin 7 / 02_md §2 (PageTypeChangeTree) + P05 START Self-Tuning

#include <cstdint>

namespace comdare::cache_engine::subsystems::scheduler {

enum class SchedulingKind : std::uint8_t {
    PageTypeUpgrade   = 0, // z.B. Node4 → Node16 → Node48 → Node256
    PageTypeDowngrade = 1,
    PageRebalance     = 2, // P05 START Multilevel-Switch
    OperationDeferral = 3, // DELAY-Decision aus Lambda-Tree
};

class ISchedulerEngine {
public:
    virtual ~ISchedulerEngine() = default;

    // Schedule eine Operation. Gibt Zeitpunkt zurueck (oder 0 = sofort)
    [[nodiscard]] virtual std::uint64_t schedule(SchedulingKind kind, std::uint64_t target_id) noexcept = 0;

    virtual void run_due_operations(std::uint64_t now_ns) noexcept = 0;

    [[nodiscard]] virtual std::uint64_t total_scheduled() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t total_executed() const noexcept  = 0;
};

} // namespace comdare::cache_engine::subsystems::scheduler
