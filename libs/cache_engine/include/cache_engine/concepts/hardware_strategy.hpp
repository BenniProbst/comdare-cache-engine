#pragma once
// ===== DEPRECATED (Muster-C Voll-Review 2026-07-12, verifiziert tot 2026-07-13) =====
// V32.EE.5 vtable-Achse (IHardwareStrategy/DefaultHardwareStrategy + Enums) gegen die
// CRTP+Concept-Doktrin. Kein Konsument (grep=0: Symbole 0 externe Treffer; Header nur
// intra-Cluster von numa_affinity.hpp inkludiert + Doku, kein Live-Nutzer). Nachfolger:
// Produktiv-Achsen in axes/ und topics/. Datei-Entfernung = separater je-GO
// (Doku-nie-loeschen-Doktrin G4). KEIN [[deprecated]]-Attribut (haelt -Werror fuer Restnutzer).
//
// V32.EE.5 (2026-05-18 spaet) - Achse 12 HARDWARE-STRATEGY (NEU)
//
// @achse 12
// @subsystem CE
// @reuse_status (b)
//
// User-Direktive 2026-05-18: NEUE Algorithmus-Permutations-Achse fuer
// verwendete Hardware-Features.
//
// WICHTIG: NICHT identisch mit K09 C8 Hardware-Probing-Heuristik!
//   - K09 C8 = wie ENTDECKT die CE die Hardware (CE-Service)
//   - Achse 12 = welche Hardware NUTZT der Algorithmus AKTIV

#include <cstdint>

namespace comdare::cache_engine::concepts {

/// Achse 12.1 SIMD-Family
enum class SimdFamily : std::uint8_t { Scalar = 0, AVX2 = 1, AVX512 = 2, NEON = 3, SVE2 = 4 };

/// Achse 12.2 Cache-Level-Targeting
enum class CacheLevelTarget : std::uint8_t { L1Aware = 0, L2Aware = 1, L3Aware = 2, HBMAware = 3 };

/// Achse 12.3 NUMA-Strategy
enum class NumaStrategy : std::uint8_t { Local = 0, Interleave = 1, Preferred = 2, Bind = 3 };

/// Achse 12.4 Prefetch-Hardware
enum class PrefetchHwInstruction : std::uint8_t { None = 0, Prefetch = 1, PrefetchNta = 2, PrefetchW = 3 };

/// Achse 12.5 Atomic-Instruction-Family
enum class AtomicFamily : std::uint8_t { None = 0, CAS = 1, LLSC = 2, RmwExtended = 3 };

/**
 * @brief IHardwareStrategy - Concept Achse 12 (NEU)
 * @achse 12
 * @subsystem CE
 * @reuse_status (b)
 *
 * Algorithmus-aktive Hardware-Strategie. CEB-AutoPermutator iteriert
 * verfuegbare Variants bei fehlender Achsen-Spec im Profil.
 */
class IHardwareStrategy {
public:
    [[nodiscard]] virtual SimdFamily            get_simd_family() const noexcept        = 0;
    [[nodiscard]] virtual CacheLevelTarget      get_cache_level_target() const noexcept = 0;
    [[nodiscard]] virtual NumaStrategy          get_numa_strategy() const noexcept      = 0;
    [[nodiscard]] virtual PrefetchHwInstruction get_prefetch_hw() const noexcept        = 0;
    [[nodiscard]] virtual AtomicFamily          get_atomic_family() const noexcept      = 0;

    virtual ~IHardwareStrategy() = default;
};

/**
 * @brief DefaultHardwareStrategy - PRT-ART-Default + Auto-Permutator-Basis
 * @achse 12
 * @reuse_status (a)
 */
struct DefaultHardwareStrategy : IHardwareStrategy {
    SimdFamily            simd{SimdFamily::AVX2};
    CacheLevelTarget      cache_level{CacheLevelTarget::L1Aware};
    NumaStrategy          numa{NumaStrategy::Local};
    PrefetchHwInstruction prefetch_hw{PrefetchHwInstruction::Prefetch};
    AtomicFamily          atomic{AtomicFamily::CAS};

    [[nodiscard]] SimdFamily            get_simd_family() const noexcept override { return simd; }
    [[nodiscard]] CacheLevelTarget      get_cache_level_target() const noexcept override { return cache_level; }
    [[nodiscard]] NumaStrategy          get_numa_strategy() const noexcept override { return numa; }
    [[nodiscard]] PrefetchHwInstruction get_prefetch_hw() const noexcept override { return prefetch_hw; }
    [[nodiscard]] AtomicFamily          get_atomic_family() const noexcept override { return atomic; }
};

} // namespace comdare::cache_engine::concepts
