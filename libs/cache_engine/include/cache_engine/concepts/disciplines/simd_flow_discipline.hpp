#pragma once
// SimdFlowDiscipline — SIMD-Flow-Pipeline Discipline (F6)

#include <cache_engine/concepts/i_concurrency_discipline.hpp>

namespace comdare::cache_engine {

class SimdFlowDiscipline final : public IConcurrencyDiscipline {
public:
    [[nodiscard]] ConcurrencyDisciplineKind kind() const noexcept override {
        return ConcurrencyDisciplineKind::SimdFlow;
    }
    void on_event(Event const&) noexcept override {}
};

}  // namespace comdare::cache_engine
