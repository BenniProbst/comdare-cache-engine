#pragma once
// ArrayDiscipline — Concurrency-Discipline auf Array/Slot-Ebene (F6)

#include <cache_engine/concepts/i_concurrency_discipline.hpp>

namespace comdare::cache_engine {

class ArrayDiscipline final : public IConcurrencyDiscipline {
public:
    [[nodiscard]] ConcurrencyDisciplineKind kind() const noexcept override { return ConcurrencyDisciplineKind::Array; }
    void                                    on_event(Event const&) noexcept override {}
};

} // namespace comdare::cache_engine
