#pragma once
// V41.F.6.1.A CRTP-Basis-Klasse fuer Allocator-Achse 6 (2026-05-25)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A Pilot
//
// CRTP-Basis-Klasse mit Concept-Guard fuer alle Allokator-Familien (A01-A23).
//
// Pattern (siehe docs/architektur/11_konzept_achsen_extension_visitor_pattern.md §3.6):
//   - Template-Constraint `requires concepts::AllocatorStrategy<Derived>`
//   - Backup static_assert im Konstruktor (klarere Fehlermeldung)
//   - Default-Methoden via CRTP-Inlining (Zero-Cost, KEINE virtual)
//
// Nutzung:
//   struct StdMalloc : public AllocatorStrategyBase<StdMalloc> {
//       using topic_tag = concepts::AllocatorTopicTag;
//       using axis_tag  = subaxes::size_class_schema_tag;
//       using family_id = std::integral_constant<int, 22>;
//       // ... Pflicht-API gemaess AllocatorStrategy-Concept
//   };

#include "concepts/axis_06_allocator_concept.hpp"
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
 * Template-Constraint `requires concepts::AllocatorStrategy<Derived>` erzwingt
 * dass Derived alle Pflicht-API erfuellt — sonst Build-Error mit klarer Diagnostik.
 *
 * **CRTP-Pattern:** Default-Methoden delegieren via static_cast an Derived.
 * Compile-Time-Polymorphie, KEINE virtual call, Inlining moeglich.
 *
 * **Hinweis F.6.1.A:** Adapter-Methoden as_std_allocator + as_pmr_resource
 * sind als TODO-Stubs vorhanden — Implementation folgt in F.6.1.B (Wrapper-Files
 * im neuen Topic+Achsen-Pfad, kompatibel mit AllocatorStrategy-Concept statt
 * altem IAllocationStrategy).
 */
template <typename Derived>
    requires concepts::AllocatorStrategy<Derived>
class AllocatorStrategyBase {
public:
    /// Backup-Check im Konstruktor (zusaetzlich zu Template-Constraint, klarere Diagnostik)
    constexpr AllocatorStrategyBase() noexcept {
        static_assert(concepts::AllocatorStrategy<Derived>,
            "Derived class must satisfy AllocatorStrategy concept "
            "(see topics/allocator/axis_06_allocator/concepts/axis_06_allocator_concept.hpp)");
    }

    // ───────────────────────────────────────────────────────────────────────
    // CRTP-Delegate-Methoden zu Derived
    // (statische Polymorphie, KEINE virtual, Inlining-faehig)
    // ───────────────────────────────────────────────────────────────────────

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) {
        return derived().raw_allocate(bytes, alignment);
    }

    void deallocate(void* p, std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) {
        derived().raw_deallocate(p, bytes, alignment);
    }

    [[nodiscard]] concepts::AllocationStatistics statistics() const noexcept {
        return derived_const().statistics();
    }

    void reset() noexcept {
        derived().reset();
    }

    // ───────────────────────────────────────────────────────────────────────
    // TODO V41.F.6.1.B Adapter-Methoden (NICHT in F.6.1.A implementiert)
    // ───────────────────────────────────────────────────────────────────────

    /**
     * @brief TODO V41.F.6.1.B as_std_allocator
     *
     * Liefert einen std::allocator-konformen Wrapper, der Derived ueber das
     * AllocatorStrategy-Concept verwendet (NICHT das alte IAllocationStrategy).
     *
     * Implementation kommt in F.6.1.B mit neuem topics/allocator/axis_06_allocator/
     * axis_06_allocator_std_allocator_wrapper.hpp (kompatibel mit neuem Concept).
     *
     * Stub: nicht implementiert, Aufruf fuehrt zu static_assert beim Instanziieren.
     */
    template <typename T>
    [[nodiscard]] auto as_std_allocator() {
        static_assert(sizeof(T) == 0,
            "TODO V41.F.6.1.B: as_std_allocator<T>() Wrapper noch nicht implementiert. "
            "Schreibe axis_06_allocator_std_allocator_wrapper.hpp im neuen Topic+Achsen-Pfad "
            "kompatibel mit AllocatorStrategy-Concept (statt altem IAllocationStrategy).");
    }

    /**
     * @brief TODO V41.F.6.1.B as_pmr_resource
     *
     * Liefert std::pmr::memory_resource-konformen Wrapper.
     *
     * Implementation kommt in F.6.1.B mit neuem topics/allocator/axis_06_allocator/
     * axis_06_allocator_pmr_resource_wrapper.hpp.
     */
    [[nodiscard]] std::pmr::memory_resource* as_pmr_resource() {
        static_assert(sizeof(Derived) == 0,
            "TODO V41.F.6.1.B: as_pmr_resource() Wrapper noch nicht implementiert. "
            "Schreibe axis_06_allocator_pmr_resource_wrapper.hpp im neuen Topic+Achsen-Pfad.");
        return nullptr;  // unreachable due to static_assert
    }

protected:
    [[nodiscard]] Derived&        derived() noexcept       { return static_cast<Derived&>(*this); }
    [[nodiscard]] Derived const&  derived_const() const noexcept { return static_cast<Derived const&>(*this); }
};

}  // namespace comdare::cache_engine::allocator::axis_06_allocator
