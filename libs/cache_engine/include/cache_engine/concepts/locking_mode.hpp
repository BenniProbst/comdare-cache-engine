#pragma once
// ===== DEPRECATED (Muster-C Voll-Review 2026-07-12, verifiziert tot 2026-07-13) =====
// V32.EE.5 vtable-Achse (ILockingMode + enum LockingMode) gegen die CRTP+Concept-Doktrin.
// Kein Konsument (grep=0: Symbole 0 Treffer; kein #include von locking_mode.hpp).
// Nachfolger: Produktiv-Achsen in axes/ und topics/ (concurrency).
// Datei-Entfernung = separater je-GO (Doku-nie-loeschen-Doktrin G4).
// KEIN [[deprecated]]-Attribut (haelt -Werror fuer Restnutzer).
//
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
