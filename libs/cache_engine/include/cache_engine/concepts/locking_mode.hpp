#pragma once
// ---------------------------------------------------------------------------
// DEPRECATED (2026-07-16): HISTORISCHER V32.EE.5-ALGORITHMUS-ACHSEN-ENTWURF
// (vtable) -- 0 Konsumenten. Gattungs-Substanz lebt als CRTP:
//   axis_08/T08 concurrency -- axes/concurrency_axis/
//   axis_08_concurrency_registry.hpp (9 CRTP-Organe: BlockingConcurrency =
//   coarse-grained std::mutex, ReaderWriterConcurrency, OlcOptimisticConcurrency
//   u.a.; Sub-Achse CC1 synchronization_pattern_tag in
//   axis_08_concurrency_subaxes_cc1_to_cc2.hpp).
// Die SYSTEM-Seite (Pflicht-Systemachse im CacheEngineBuilder) wird NEU gebaut
//   -- Dossier 23 (Abschnitt 6, S-2; User-Entscheid 2026-07-16).
// NICHT reaktivieren, NICHT loeschen; Rest-Semantik ohne Live-Gegenstueck
//   (LockingMode::Upgradeable, aktive NumaStrategy, AtomicFamily) in der
//   Delta-Matrix (Dossier 23 Abschnitt 2) als TODO getrackt.
// ---------------------------------------------------------------------------
// V32.EE.5 (2026-05-18 spaet) - Achse 8.2 Locking-Mode (Sub-Achse Erweiterung)
//
// @achse 8.2
// @subsystem CE
// @reuse_status (b)
//
// Sub-Achse von Achse 8 CONCURRENCY (N-Phase Split).
// Bestand: implizit in disciplines/memory_*_discipline.hpp - V32 explizit.

#include <cstdint>

namespace comdare::cache_engine::concepts {

/**
 * @brief LockingMode - Achse 8.2 Locking-Modus
 * @achse 8.2
 * @subsystem CE
 */
enum class LockingMode : std::uint8_t {
    ReadOnly             = 0, ///< std::shared_mutex shared
    ReadWrite            = 1, ///< std::mutex exclusive
    Upgradeable          = 2, ///< boost::shared_mutex upgrade
    OptimisticValidation = 3  ///< OLC, HTM
};

/**
 * @brief ILockingMode - Concept fuer Algorithmus-spezifische Locking-Wahl
 * @achse 8.2
 * @subsystem CE
 */
class ILockingMode {
public:
    [[nodiscard]] virtual LockingMode preferred_mode() const noexcept = 0;
    virtual ~ILockingMode()                                           = default;
};

} // namespace comdare::cache_engine::concepts
