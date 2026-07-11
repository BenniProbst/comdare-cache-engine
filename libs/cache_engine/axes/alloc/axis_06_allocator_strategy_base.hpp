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
#include <topics/axis_base.hpp>
#include <axes/cacheline/cacheline_config.hpp> // KF-5: per-Organ Cache-Line-Unterachse

#include <concepts>
#include <cstddef>
#include <memory_resource>
#include <type_traits>

namespace comdare::cache_engine::alloc {

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
 * **V41.F.6.1.R7.4 (2026-05-29):** Adapter-Methoden as_std_allocator<T>() + as_pmr_resource()
 * implementiert (vorher static_assert-Stubs). Beide liefern WERT-basierte Adapter, die an die
 * allocate/deallocate-API der Strategie weiterleiten — KEINE Basis-Datenmember (Empty-Base-
 * Optimization + Wrapper-Groesse bleiben erhalten; der Adapter haelt nur einen Derived*).
 */
// KF-5 (2026-06-02): zusätzlicher defaulted NTTP CacheLineCfg + Erbe von CacheLineAware<Cfg> — macht JEDEN
// Allokator-Wrapper cacheline-fähig (cacheline_config/cacheline_alignment/cacheline_prefetch). Default {} =
// B64/None/None = unverändertes Verhalten (nicht-brechend, ODR-sicher: Default ist ein Literal, kein Makro).
// Die per-Binary-Bäckung wählt der Codegen über eine DISTINKTE Organ-Instanz (KF-6/KF-8), nicht über den Default.
template <typename Derived, ::comdare::cache_engine::cacheline::CacheLineConfig CacheLineCfg =
                                ::comdare::cache_engine::cacheline::CacheLineConfig{}>
class AllocatorStrategyBase : public ::comdare::cache_engine::topics::AxisBase,
                              public ::comdare::cache_engine::cacheline::CacheLineAware<CacheLineCfg> {
public:
    /// Concept-Check im Konstruktor: Pflicht-Set AllocatorStrategy + CacheEnginePermutationStrategy + AxisBase
    constexpr AllocatorStrategyBase() noexcept {
        static_assert(concepts::AllocatorStrategy<Derived>, "Derived must satisfy AllocatorStrategy concept "
                                                            "(see concepts/axis_06_allocator_concept.hpp)");
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>,
                      "Derived must satisfy CacheEnginePermutationStrategy concept "
                      "(see concepts/axis_06_allocator_cache_engine_permutation_concept.hpp)");
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>,
                      "Derived must satisfy AxisBaseConcept (get_compiler() Pflicht-API). "
                      "AllocatorStrategyBase erbt von AxisBase — Derived bekommt Default 'original' automatisch.");
    }

    // ───────────────────────────────────────────────────────────────────────
    // V41.F.6.1.P2.C ENTFERNT: has_original_paper_code + is_original_module
    // Kommen jetzt generisch via AxisBase Default (is_original_module = false).
    // Paper-Wrappers (z.B. MimallocAllocator) ueberschreiben via Mixin-Inheritance.
    // ───────────────────────────────────────────────────────────────────────

    // ───────────────────────────────────────────────────────────────────────
    // V41.F.6.1.R7.4 Cross-Axis-Default resource_ownership() ([[cross-axis-defaults-no-bloat]]):
    // Das Besitz-Modell des memory_resource-Objekts ist fuer die GANZE malloc-Familie (libc +
    // alle Vendor-Allokatoren) None — sie verwalten kein std::pmr::memory_resource. Der Default
    // gehoert daher EINMAL hierher in die CRTP-Wurzel, NICHT 25x in die Wrapper. Nur die beiden
    // PMR-Familien-Wrapper ueberschreiben (PoolResourceAllocator=Owned, PmrResourceAllocator=Borrowed).
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] static constexpr concepts::ResourceOwnership resource_ownership() noexcept {
        return concepts::ResourceOwnership::None;
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
    [[nodiscard]] concepts::AllocationStatistics statistics() const noexcept { return derived_const().statistics(); }

    void reset() noexcept {
        // V41.F.6.1.A User-Klarstellung: reset() = Statistik-Reset, NICHT Pool-Reset!
        derived().reset();
    }

    // Phase 0.3 (Hebel B, Memento-Pattern GoF): Statistik auf einen zuvor via statistics() gezogenen Snapshot
    // zuruecksetzen — spiegelbildlich zu reset(). Nutzung: ein strategie-besitzender Store (z.B. TreeNodePoolStore)
    // verwirft damit im Copy-Ctor/Assign die durch die COW-Vollkopie entstandene transiente Re-Allokations-
    // Pollution (die Zwei-Phasen-Mess-Doppelzaehlung), sodass T6 = save-Stand + measure-Delta bleibt.
    void restore_statistics(concepts::AllocationStatistics const& s) noexcept { derived().restore_statistics(s); }
#endif

    // ───────────────────────────────────────────────────────────────────────
    // V41.F.6.1.R7.4 Adapter-Methoden — Allocator-Achse an std::allocator / std::pmr anbinden.
    // WERT-basiert (kein Basis-Datenmember → EBO + Wrapper-Groesse bleiben erhalten).
    // ───────────────────────────────────────────────────────────────────────

    /**
     * @brief StdAllocatorAdapter<T> — erfuellt die std::allocator-Named-Requirements (C++23) und
     *        leitet allocate/deallocate an die zugrundeliegende Achsen-Strategie weiter.
     *        Nutzbar mit std::vector<T, StdAllocatorAdapter<T>>, std::allocator_traits, rebind.
     */
    template <typename T>
    class StdAllocatorAdapter {
    public:
        using value_type = T;
        explicit StdAllocatorAdapter(Derived* strat) noexcept : strat_(strat) {}
        template <typename U>
        StdAllocatorAdapter(StdAllocatorAdapter<U> const& other) noexcept : strat_(other.strat_) {}

        [[nodiscard]] T* allocate(std::size_t n) {
            return static_cast<T*>(strat_->allocate(n * sizeof(T), alignof(T)));
        }
        void deallocate(T* p, std::size_t n) noexcept { strat_->deallocate(p, n * sizeof(T), alignof(T)); }
        template <typename U>
        [[nodiscard]] bool operator==(StdAllocatorAdapter<U> const& other) const noexcept {
            return strat_ == other.strat_;
        }

    private:
        template <typename U>
        friend class StdAllocatorAdapter;
        Derived* strat_;
    };

    /**
     * @brief PmrResourceAdapter — std::pmr::memory_resource ueber die Achsen-Strategie. Konkreter,
     *        kopierbarer Wert-Typ (haelt nur Derived*); der Aufrufer haelt ihn am Leben und
     *        uebergibt &resource an pmr-Container.
     */
    class PmrResourceAdapter final : public std::pmr::memory_resource {
    public:
        explicit PmrResourceAdapter(Derived* strat) noexcept : strat_(strat) {}

    private:
        void* do_allocate(std::size_t bytes, std::size_t alignment) override {
            return strat_->allocate(bytes, alignment);
        }
        void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
            strat_->deallocate(p, bytes, alignment);
        }
        [[nodiscard]] bool do_is_equal(std::pmr::memory_resource const& other) const noexcept override {
            auto const* o = dynamic_cast<PmrResourceAdapter const*>(&other);
            return o != nullptr && o->strat_ == strat_;
        }
        Derived* strat_;
    };

    /// Liefert einen std::allocator-kompatiblen Adapter (Wert) fuer Element-Typ T.
    template <typename T>
    [[nodiscard]] StdAllocatorAdapter<T> as_std_allocator() noexcept {
        return StdAllocatorAdapter<T>(&derived());
    }

    /// Liefert einen pmr-memory_resource-Adapter (Wert). Aufrufer haelt ihn am Leben und uebergibt
    /// dessen Adresse an pmr-Container (z.B. std::pmr::polymorphic_allocator).
    [[nodiscard]] PmrResourceAdapter as_pmr_resource() noexcept { return PmrResourceAdapter(&derived()); }

protected:
    [[nodiscard]] Derived&       derived() noexcept { return static_cast<Derived&>(*this); }
    [[nodiscard]] Derived const& derived_const() const noexcept { return static_cast<Derived const&>(*this); }
};

} // namespace comdare::cache_engine::alloc
