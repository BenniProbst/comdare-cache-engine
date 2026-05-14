#pragma once
// NodeDiscipline — Concurrency-Discipline auf Node-Ebene (F6)

#include <cache_engine/concepts/i_concurrency_discipline.hpp>

namespace comdare::cache_engine {

class NodeDiscipline final : public IConcurrencyDiscipline {
public:
    [[nodiscard]] ConcurrencyDisciplineKind kind() const noexcept override {
        return ConcurrencyDisciplineKind::Node;
    }
    void on_event(Event const&) noexcept override {}
};

}  // namespace comdare::cache_engine
