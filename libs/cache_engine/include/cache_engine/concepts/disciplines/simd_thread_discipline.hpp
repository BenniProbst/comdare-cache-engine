#pragma once
// SimdThreadDiscipline — SIMD-Thread-Coupling Discipline (F6)

#include <cache_engine/concepts/i_concurrency_discipline.hpp>

namespace comdare::cache_engine {

class SimdThreadDiscipline final : public IConcurrencyDiscipline {
public:
    [[nodiscard]] ConcurrencyDisciplineKind kind() const noexcept override {
        return ConcurrencyDisciplineKind::SimdThread;
    }
    void on_event(Event const&) noexcept override {}
};

} // namespace comdare::cache_engine
