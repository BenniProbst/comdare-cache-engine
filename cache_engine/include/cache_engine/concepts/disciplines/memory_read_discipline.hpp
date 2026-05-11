#pragma once
// MemoryReadDiscipline — Read-only Memory Access (F6)

#include <cache_engine/concepts/i_concurrency_discipline.hpp>

namespace comdare::cache_engine {

class MemoryReadDiscipline final : public IConcurrencyDiscipline {
public:
    [[nodiscard]] ConcurrencyDisciplineKind kind() const noexcept override {
        return ConcurrencyDisciplineKind::MemoryRead;
    }
    void on_event(Event const&) noexcept override {}
};

}  // namespace comdare::cache_engine
