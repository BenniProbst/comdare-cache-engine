#pragma once
// MemoryReadWriteDiscipline — Mixed Read/Write Memory Access (F6)

#include <cache_engine/concepts/i_concurrency_discipline.hpp>

namespace comdare::cache_engine {

class MemoryReadWriteDiscipline final : public IConcurrencyDiscipline {
public:
    [[nodiscard]] ConcurrencyDisciplineKind kind() const noexcept override {
        return ConcurrencyDisciplineKind::MemoryReadWrite;
    }
    void on_event(Event const&) noexcept override {}
};

} // namespace comdare::cache_engine
