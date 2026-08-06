#pragma once
// V41.F.6.1.A Standard-Allokator Pflicht-Concept (2026-05-25, W1 Web-Recherche revidiert)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A
//
// **Pflicht-Concept fuer ALLE Allokator-Familien** (libc malloc, jemalloc, tcmalloc,
// mimalloc, snmalloc, Hoard, dlmalloc, ptmalloc2, std::allocator, std::pmr::memory_resource).
//
// Diese Pflicht-API ist die SCHNITTMENGE aller modernen Allokator-Bibliotheken
// (ISO C11/C17 §7.22.3, C++17 [mem.res.public], C++23 [allocator.requirements]).
//
// Nicht-standardisierbare API (statistics, mallopt, vendor-spezifisch) ist in
// separaten Sub-Concepts (siehe Geschwister-Files):
//   - axis_06_allocator_zeroing_strategy_concept.hpp       (calloc)
//   - axis_06_allocator_overallocating_strategy_concept.hpp (C++23 allocate_at_least)
//   - axis_06_allocator_introspectable_strategy_concept.hpp (usable_size)
//   - axis_06_allocator_reclaimable_strategy_concept.hpp    (collect/purge)
//   - axis_06_allocator_resettable_strategy_concept.hpp     (Pool/Arena reset)
//   - axis_06_allocator_reallocating_strategy_concept.hpp   (realloc)
//
// cache-engine-spezifische Pflicht-API (axis_tag/family_id/name/...):
//   - axis_06_allocator_cache_engine_permutation_concept.hpp (parallel zu AllocatorStrategy)
//
// A1-Wurf-Vertrag (2026-08-06): die achsen-eigene Wurf-Uebersetzung `allocate_or_throw` steht als
// ThrowTranslatingStrategy UNTEN in DIESER Datei -- bewusst NICHT in AllocatorStrategy hineingezogen
// (Begruendung dort).

#include <topics/allocator/concepts/topic_allocator_concept.hpp>

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::alloc::concepts {

/**
 * @brief AllocatorStrategy - PFLICHT-Concept fuer alle Allokator-Familien
 * @topic allocator
 * @achse 6
 *
 * Schnittmenge aller modernen Allokator-Familien (ISO C11/C17, C++17 PMR, C++23):
 *
 * **(1) Typedefs (Pflicht):**
 *   - typename A::value_type      (z.B. std::byte fuer raw allokator)
 *   - typename A::size_type       (typischerweise std::size_t)
 *
 * **(2) Runtime-API (Pflicht):**
 *   - allocate(bytes, alignment)   -> void*   (PMR-konform)
 *   - deallocate(p, bytes, align)  noexcept   (sized+aligned, C++17 [mem.res.public])
 *
 * **(3) Identitaet (Pflicht):**
 *   - operator==(other)            (STL/PMR Allocator-Gleichheit)
 *   - std::copy_constructible<A>
 *   - std::is_nothrow_destructible_v<A>
 *
 * **Beispiel-Wrapper (mimalloc):**
 * ```cpp
 * struct MimallocStrategy {
 *     using value_type = std::byte;
 *     using size_type  = std::size_t;
 *     void* allocate(std::size_t bytes, std::size_t align) {
 *         return ::mi_malloc_aligned(bytes, align);
 *     }
 *     void deallocate(void* p, std::size_t bytes, std::size_t align) noexcept {
 *         ::mi_free_size_aligned(p, bytes, align);
 *     }
 *     bool operator==(const MimallocStrategy&) const noexcept { return true; }
 * };
 * static_assert(AllocatorStrategy<MimallocStrategy>);
 * ```
 *
 * **WICHTIG zu C-konformem free:**
 *   `deallocate(p, bytes, alignment)` ist die C++17-PMR-Form von `free` — sized + aligned.
 *   Naked `free(p)` (libc) ist trivial drauf zurueckfuehrbar im Wrapper.
 *   `reset()` ist KEIN free-Aequivalent — gehoert in Sub-Concept ResettableStrategy.
 */
template <typename A>
concept AllocatorStrategy = ::comdare::cache_engine::allocator::concepts::AllocatorComponent<A> && requires {
    typename A::value_type;
    typename A::size_type;
} && requires(A a, void* p, std::size_t bytes, std::size_t align) {
    // (2) Pflicht Runtime-API
    { a.allocate(bytes, align) } -> std::same_as<void*>;
    { a.deallocate(p, bytes, align) } noexcept;
} && requires(A const& a, A const& b) {
    // (3) Identitaet
    { a == b } -> std::convertible_to<bool>;
} && std::copy_constructible<A> && std::is_nothrow_destructible_v<A>;

/**
 * @brief ThrowTranslatingStrategy - Sub-Concept: die Strategie traegt die Wurf-Uebersetzung der Achse.
 * @topic allocator
 * @achse 6
 *
 * **WOZU (A1-Wurf-Vertrag, Nachbesserung 2026-08-06):** achsen-INNEN gilt "OOM == nullptr"; ein
 * Konsument, der die Strategie ROH haelt (`A alloc_;` als Kompositions-Template-Parameter) und die
 * Rueckgabe direkt beschreibt, braucht die Uebersetzung nach AUSSEN. Sie steht als
 * `AllocatorStrategyBase::allocate_or_throw` an der CRTP-Wurzel -- jede Strategie der Achse traegt sie
 * also geerbt, ohne eine einzige registrierte Variante anzufassen. Was FEHLTE, war der Ausdruck dieser
 * Anforderung im Typsystem: der Store-Kopf (axis_04_node_type_layout_aware_store.hpp) verlangte nur
 * `AllocatorStrategy<A>`, rief aber `allocate_or_throw` -- eine Strategie ohne diesen Member haette den
 * Kopf-Constraint erfuellt und waere erst tief im Rumpf als Instanziierungs-Fehler aufgefallen.
 *
 * **WARUM NICHT IN `AllocatorStrategy` HINEIN (Entscheid, nicht Bequemlichkeit):** AllocatorStrategy ist
 * per Datei-Doktrin die SCHNITTMENGE der Standard-Allokator-Familien (ISO C11/C17, C++17 PMR, C++23) --
 * `allocate_or_throw` ist dagegen cache-engine-eigen und in keinem dieser Standards vorgesehen. Es dort
 * einzutragen wuerde die dokumentierte Trennung "Standard-Pflicht vs. cache-engine-Pflicht" aufweichen
 * und jede kuenftige, standardnah gedachte Fremd-Strategie am falschen Concept scheitern lassen. Die
 * Achse fuehrt ihre Zusatz-Faehigkeiten deshalb seit jeher als Sub-Concepts (Zeroing/Reallocating/
 * Introspectable/Reclaimable/Resettable/Overallocating) -- dies ist das siebte, und es liegt in der
 * Allokator-Achse, weil es eine Allokator-Eigenschaft ist (generalisierte Schnitt-Regel).
 *
 * **DIAGNOSE-WIRKUNG:** als Kopf-Constraint des Konsumenten ist die Anforderung SFINAE-freundlich und
 * benennt beim Bruch das Concept (statt eines Fehlers im Rumpf ohne Bezug zur Ursache).
 */
template <typename A>
concept ThrowTranslatingStrategy = AllocatorStrategy<A> && requires(A a, std::size_t bytes, std::size_t align) {
    { a.allocate_or_throw(bytes, align) } -> std::same_as<void*>;
};

} // namespace comdare::cache_engine::alloc::concepts
