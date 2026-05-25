#pragma once
// V41.F.6.1.A CRTP-Basis-Klasse fuer Allocator-Achse 6 (2026-05-25, W1-revidiert)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A
//
// CRTP-Basis-Klasse mit Concept-Guard fuer alle Allokator-Familien.
//
// Pattern (siehe docs/architektur/11_konzept_achsen_extension_visitor_pattern.md §3.6):
//   - static_assert Concept-Check im Konstruktor (NICHT als template-constraint —
//     wegen CRTP-Henne-Ei: Derived ist incomplete bei Basis-Instantiation)
//   - Default-Methoden via CRTP-Inlining (Zero-Cost, KEINE virtual)
//
// API ist Standard-konform (PMR-Naming):
//   - allocate(bytes, alignment)
//   - deallocate(p, bytes, alignment)  noexcept
//
// Vendor-spezifische Sub-Refinements (calloc/realloc/usable_size/...) sind als
// separate Concepts vorhanden (siehe concepts/axis_06_allocator_*_strategy_concept.hpp).

#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"

#include <concepts>
#include <cstddef>
#include <memory_resource>
#include <type_traits>

namespace comdare::cache_engine::allocator::axis_06_allocator {

/**
 * @brief AllocatorStrategyBase - CRTP-Basis fuer alle Allokator-Familien
 * @topic allocator
 * @achse 6
 *
 * HINWEIS V41.F.6.1.A: Template-Constraint `requires AllocatorStrategy<Derived>` ist
 * bei CRTP NICHT moeglich — Derived ist zur Zeit der Basis-Klassen-Instanziierung
 * noch incomplete. Loesung: Concept-Check NUR per static_assert im Konstruktor
 * (Derived ist dann vollstaendig).
 *
 * **CRTP-Pattern:** Default-Methoden delegieren via static_cast an Derived.
 * Compile-Time-Polymorphie, KEINE virtual call, Inlining-faehig.
 *
 * **TODO V41.F.6.1.B:** Adapter-Methoden as_std_allocator + as_pmr_resource
 * sind als TODO-Stubs vorhanden — Implementation folgt mit neuen Wrappern.
 */
template <typename Derived>
class AllocatorStrategyBase {
public:
    /// Concept-Check im Konstruktor: Pflicht-Set AllocatorStrategy + CacheEnginePermutationStrategy
    constexpr AllocatorStrategyBase() noexcept {
        static_assert(concepts::AllocatorStrategy<Derived>,
            "Derived must satisfy AllocatorStrategy concept "
            "(see concepts/axis_06_allocator_concept.hpp)");
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>,
            "Derived must satisfy CacheEnginePermutationStrategy concept "
            "(see concepts/axis_06_allocator_cache_engine_permutation_concept.hpp)");
    }

    // ───────────────────────────────────────────────────────────────────────
    // CRTP-Delegate-Methoden zu Derived (Standard-PMR-Naming)
    // ───────────────────────────────────────────────────────────────────────

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) {
        return derived().allocate(bytes, alignment);
    }

    void deallocate(void* p, std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept {
        derived().deallocate(p, bytes, alignment);
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    [[nodiscard]] concepts::AllocationStatistics statistics() const noexcept {
        return derived_const().statistics();
    }

    void reset() noexcept {
        // V41.F.6.1.A User-Klarstellung: reset() = Statistik-Reset, NICHT Pool-Reset!
        derived().reset();
    }
#endif

    // ───────────────────────────────────────────────────────────────────────
    // TODO V41.F.6.1.B Adapter-Methoden (NICHT in F.6.1.A implementiert)
    // ───────────────────────────────────────────────────────────────────────

    /**
     * @brief TODO V41.F.6.1.B as_std_allocator
     *
     * Stub: nicht implementiert, Aufruf fuehrt zu static_assert beim Instanziieren.
     */
    template <typename T>
    [[nodiscard]] auto as_std_allocator() {
        static_assert(sizeof(T) == 0,
            "TODO V41.F.6.1.B: as_std_allocator<T>() Wrapper noch nicht implementiert.");
    }

    /**
     * @brief TODO V41.F.6.1.B as_pmr_resource
     */
    [[nodiscard]] std::pmr::memory_resource* as_pmr_resource() {
        static_assert(sizeof(Derived) == 0,
            "TODO V41.F.6.1.B: as_pmr_resource() Wrapper noch nicht implementiert.");
        return nullptr;
    }

protected:
    [[nodiscard]] Derived&        derived() noexcept       { return static_cast<Derived&>(*this); }
    [[nodiscard]] Derived const&  derived_const() const noexcept { return static_cast<Derived const&>(*this); }
};

}  // namespace comdare::cache_engine::allocator::axis_06_allocator
