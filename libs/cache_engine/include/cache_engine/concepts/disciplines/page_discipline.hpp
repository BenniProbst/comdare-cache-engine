#pragma once
// PageDiscipline — Concurrency-Discipline auf Page-Ebene (F6)

#include <cache_engine/concepts/i_concurrency_discipline.hpp>

namespace comdare::cache_engine {

class PageDiscipline final : public IConcurrencyDiscipline {
public:
    [[nodiscard]] ConcurrencyDisciplineKind kind() const noexcept override { return ConcurrencyDisciplineKind::Page; }
    void                                    on_event(Event const&) noexcept override {}
};

} // namespace comdare::cache_engine
