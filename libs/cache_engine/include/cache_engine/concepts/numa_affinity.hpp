#pragma once
// ---------------------------------------------------------------------------
// DEPRECATED (2026-07-16): HISTORISCHER V32.EE.5-ALGORITHMUS-ACHSEN-ENTWURF
// (vtable) -- 0 Konsumenten. Gattungs-Substanz lebt als CRTP:
//   axis_06/T06 allocator -- axes/alloc/ (AA5 allocation_policy-Tag in
//   axis_06_allocator_subaxes_aa1_to_aa7.hpp + NUMA/Page-Kante
//   alloc_hw_config.hpp::AllocNumaNode{Auto,Node0,Node1}, compile-time
//   Unterachse, if-constexpr ueber numa_capable gegatet).
// Die SYSTEM-Seite (Pflicht-Systemachse im CacheEngineBuilder) wird NEU gebaut
//   -- Dossier 23 (Abschnitt 6, S-2; User-Entscheid 2026-07-16).
// NICHT reaktivieren, NICHT loeschen; Rest-Semantik ohne Live-Gegenstueck
//   (LockingMode::Upgradeable, aktive NumaStrategy, AtomicFamily) in der
//   Delta-Matrix (Dossier 23 Abschnitt 2) als TODO getrackt.
// ---------------------------------------------------------------------------
// V32.EE.5 (2026-05-18 spaet) - Achse 6.3 NUMA-Affinity (Erweiterung)
//
// @achse 6.3
// @subsystem CE
// @reuse_status (b)
//
// Sub-Achse von Achse 6 ALLOCATOR (N-Phase Split).

#include "hardware_strategy.hpp" // NumaStrategy enum
#include <cstdint>

namespace comdare::cache_engine::concepts {

/**
 * @brief INumaAffinity - Sub-Achse 6.3 (Algorithmus-spez. NUMA-Wahl)
 * @achse 6.3
 * @subsystem CE
 *
 * Allokator-Sub-Strategy. Konsumiert NumaStrategy aus hardware_strategy.hpp.
 */
class INumaAffinity {
public:
    [[nodiscard]] virtual NumaStrategy preferred_strategy() const noexcept = 0;
    virtual ~INumaAffinity()                                               = default;
};

} // namespace comdare::cache_engine::concepts
