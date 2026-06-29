#pragma once
// IConcurrencyMechanic — F6 Concept-Wurzel fuer 3 Mechaniken (OLC, ROWEX, ComdareRcu)
// Termin 7 / 02_uml_cache_engine §3

#include <cstdint>

namespace comdare::cache_engine {

enum class ConcurrencyMechanicKind : std::uint8_t {
    OLC        = 0, // P01/P08 Optimistic Lock Coupling
    ROWEX      = 1, // P02/P08 Read-Optimized Write Exclusion
    ComdareRcu = 2, // P29 Quiescent-State-Based RCU (Task #104)
};

class IConcurrencyMechanic {
public:
    virtual ~IConcurrencyMechanic() = default;

    [[nodiscard]] virtual ConcurrencyMechanicKind kind() const noexcept = 0;

    // Lifecycle / pro-Thread-Hook
    virtual void register_thread() noexcept {}
    virtual void deregister_thread() noexcept {}

    // Reader-Eintrittspunkt — aequivalent zu OLC readLockOrRestart / RCU read_lock
    virtual void begin_read() noexcept = 0;
    virtual void end_read() noexcept   = 0;

    // Writer-Eintrittspunkt
    virtual void begin_write() noexcept = 0;
    virtual void end_write() noexcept   = 0;

    // Synchronisations-Barrier (RCU synchronize / OLC kompletter Restart)
    virtual void synchronize() noexcept {}
};

} // namespace comdare::cache_engine
