#pragma once
// PathDiscipline — Concurrency-Discipline auf Traversal-Pfad-Ebene (F6)

#include <cache_engine/concepts/i_concurrency_discipline.hpp>

namespace comdare::cache_engine {

class PathDiscipline final : public IConcurrencyDiscipline {
public:
    [[nodiscard]] ConcurrencyDisciplineKind kind() const noexcept override { return ConcurrencyDisciplineKind::Path; }
    void                                    on_event(Event const&) noexcept override {}
};

} // namespace comdare::cache_engine
