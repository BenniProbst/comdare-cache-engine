#pragma once
// DataStructureDiscipline — Concurrency-Discipline auf gesamter Datenstruktur (F6)

#include <cache_engine/concepts/i_concurrency_discipline.hpp>

namespace comdare::cache_engine {

class DataStructureDiscipline final : public IConcurrencyDiscipline {
public:
    [[nodiscard]] ConcurrencyDisciplineKind kind() const noexcept override {
        return ConcurrencyDisciplineKind::DataStructure;
    }
    void on_event(Event const&) noexcept override {}
};

} // namespace comdare::cache_engine
