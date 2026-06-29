#pragma once
// IConcurrencyProtocol — Plattform-seitige Concurrency-Eigenschaften
// Termin 7 / REV 5 K08 (6-Flavor-RCU-Klassifikation)

#include <cstdint>

namespace comdare::cache_engine::platform {

enum class RcuFlavor : std::uint8_t {
    Classic    = 0,
    Qsbr       = 1,
    Bp         = 2,
    Membarrier = 3,
    Signal     = 4,
    Sysmemb    = 5,
};

class IConcurrencyProtocol {
public:
    virtual ~IConcurrencyProtocol() = default;

    [[nodiscard]] virtual bool      supports_htm() const noexcept                = 0; // Hardware Transactional Memory
    [[nodiscard]] virtual bool      supports_atomic_int128() const noexcept      = 0; // 128-Bit Atomics (CAS2)
    [[nodiscard]] virtual bool      supports_ll_sc_restricted() const noexcept   = 0; // PowerPC/Alpha LL/SC
    [[nodiscard]] virtual bool      supports_membarrier_syscall() const noexcept = 0;
    [[nodiscard]] virtual RcuFlavor preferred_rcu_flavor() const noexcept        = 0;
    [[nodiscard]] virtual double    cache_coherence_latency_cycles() const noexcept = 0; // Cross-Core MESI-Cost
};

} // namespace comdare::cache_engine::platform
