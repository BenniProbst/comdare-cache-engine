#pragma once
// IConcurrencyDiscipline — F6 Concept-Wurzel fuer 10 Disziplinen
// Termin 7 / 02_uml_cache_engine §3

#include <cache_engine/concepts/event.hpp>

namespace comdare::cache_engine {

enum class ConcurrencyDisciplineKind : std::uint8_t {
    Page,
    Node,
    Array,
    DataStructure,
    Path,
    MemoryRead,
    MemoryWrite,
    MemoryReadWrite,
    SimdThread,
    SimdFlow,
};

class IConcurrencyDiscipline {
public:
    virtual ~IConcurrencyDiscipline() = default;

    [[nodiscard]] virtual ConcurrencyDisciplineKind kind() const noexcept                 = 0;
    virtual void                                    on_event(Event const& event) noexcept = 0;
};

} // namespace comdare::cache_engine
