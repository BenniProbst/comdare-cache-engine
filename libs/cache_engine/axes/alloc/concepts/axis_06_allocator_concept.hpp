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
// (Begruendung dort). Die A1-NACHBESSERUNG desselben Tages ergaenzt drei weitere Sub-Concepts fuer die
// uebrigen Faehigkeiten, die ein die Strategie ROH haltender Konsument braucht (Abschnitt unten):
//   - StdAllocatorAdaptingStrategy  (A::StdAllocatorAdapter<T> + as_std_allocator<T>())
//   - ValueSemanticStrategy         (A{} + alloc_ = A{})
//   - StatisticsReportingStrategy   (A::snapshot_t + statistics(); nur unter COMDARE_CE_ENABLE_STATISTICS)

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

// ==================================================================================================
// A1-NACHBESSERUNG 2026-08-06 (Review-Befund "Kopf verlangt weniger, als der Rumpf braucht", Teil 2)
// ==================================================================================================
// Die Erst-Fassung des A1-Schnitts hat NUR `allocate_or_throw` in den Kopf-Constraint des rohen
// Konsumenten (axis_04_node_type_layout_aware_store.hpp) gehoben. Der Rumpf DIESES Konsumenten greift
// aber auf DREI WEITERE Achsen-Faehigkeiten zu, die `AllocatorStrategy` nicht fordert:
//
//   (a) die STANDARD-CONTAINER-NAHT  -- `A::StdAllocatorAdapter<T>` + `as_std_allocator<T>()`
//       (Fundstellen: axis_04_..._store.hpp:225/:227/:494/:662 -- der Chunk-INDEX haengt seit dem
//       A8-S5-02a-HERZ-Schnitt an dieser Naht).
//   (b) die WERT-SEMANTIK            -- `A{}` (Member-Initialisierung `mutable A alloc_{}`) und die
//       Zuweisung `alloc_ = A{}` im Kopier-Zuweisungs-Operator (:241).
//   (c) die T6-MESS-ROUTE            -- `A::snapshot_t` + `a.statistics()` (:353/:354/:359).
//
// Ohne diese Terme galt fuer sie GENAU derselbe Befund wie fuer allocate_or_throw: eine Strategie ohne
// die Faehigkeit erfuellte den Kopf und zerbrach erst tief im Rumpf, ohne Bezug zur Ursache. Sie stehen
// hier und nicht beim Konsumenten, weil es ALLOKATOR-Eigenschaften sind (generalisierte Schnitt-Regel:
// Achsen-Eigenschaften NUR ueber die Achse). Sie stehen als EINZELNE Sub-Concepts und nicht als ein
// Sammel-Concept, weil nur so beim Bruch der EINE fehlende Baustein benannt wird -- und weil nur so
// jeder Term am Konsumenten-Kopf einzeln negativ pinnbar ist (test_a1_wurf_vertrag_allokator_store).
// Wie die sechs aelteren Sub-Concepts der Achse bleiben sie AUSSERHALB von AllocatorStrategy: das ist
// per Datei-Doktrin die Schnittmenge der Standard-Familien, und keine davon kennt diese drei.
//
// GRENZE, EHRLICH BENANNT: ein Concept kann nicht ueber ALLE Element-Typen T quantifizieren. (a) prueft
// deshalb an EINEM Sonden-Typ (std::byte -- die value_type der Achse); eine Member-Template-Naht, die
// fuer std::byte traegt und fuer einen anderen Typ nicht, waere pathologisch und wird nicht behauptet.
// REGISTRIERTE VARIANTEN UNBERUEHRT: (a) und (b) sitzen an der CRTP-Wurzel AllocatorStrategyBase bzw.
// folgen aus deren Wert-Semantik, (c) deklariert jede der 26 Strategien selbst (Posten 80).

/**
 * @brief StdAllocatorAdaptingStrategy - Sub-Concept: die Strategie traegt die Standard-Container-Naht.
 * @topic allocator
 * @achse 6
 *
 * Wer einen std::-Container ueber die Achse fuehrt (`std::vector<T, A::StdAllocatorAdapter<T>>`),
 * braucht BEIDES: den Adapter-TYP (als Container-Template-Argument) und die Fabrik `as_std_allocator<T>()`
 * (als Container-Konstruktor-Argument). Der Adapter traegt bewusst KEINEN Default-Ktor -- ein stilles
 * Zurueckfallen auf std::allocator ist damit compile-hart ausgeschlossen (A8-S5-02a). Genau deshalb ist
 * die Fabrik Teil der Anforderung und nicht nur der Typ.
 *
 * `noexcept` ist gefordert, nicht geschenkt: die Fabrik wird in Konstruktor-Initialisierungslisten
 * gerufen (axis_04_..._store.hpp:225/:227), und `release_index_()` ist als noexcept deklariert und ruft
 * sie -- eine werfende Fabrik machte daraus ein std::terminate.
 */
template <typename A>
concept StdAllocatorAdaptingStrategy = AllocatorStrategy<A> && requires(A a) {
    typename A::template StdAllocatorAdapter<std::byte>;
    { a.template as_std_allocator<std::byte>() } noexcept
        -> std::same_as<typename A::template StdAllocatorAdapter<std::byte>>;
} && std::same_as<typename A::template StdAllocatorAdapter<std::byte>::value_type, std::byte>;

/**
 * @brief ValueSemanticStrategy - Sub-Concept: die Strategie ist als WERT haltbar und neu setzbar.
 * @topic allocator
 * @achse 6
 *
 * Ein Konsument, der die Strategie ROH als Member haelt (`A alloc_{};` statt eines Zeigers/Verweises),
 * default-konstruiert sie bei der Objekt-Erzeugung und setzt sie bei der Kopier-Zuweisung auf einen
 * frischen Stand zurueck (`alloc_ = A{}`) -- so beschreiben die Zaehler der Kopie ehrlich IHR eigenes
 * Backing und nicht das des Vorgaenger-Inhalts. Gefordert ist GENAU dieser konsumierte Ausschnitt:
 * Default-Initialisierbarkeit und Zuweisbarkeit aus einem Rvalue. Kopier-Konstruierbarkeit fordert
 * bereits AllocatorStrategy; eine breitere `std::regular`-Forderung waere ueber den Bedarf hinaus.
 */
template <typename A>
concept ValueSemanticStrategy = AllocatorStrategy<A> && std::default_initializable<A> && std::assignable_from<A&, A>;

/**
 * @brief StatisticsReportingStrategy - Sub-Concept: die Strategie traegt die T6-Mess-Route der Achse.
 * @topic allocator
 * @achse 6
 *
 * `snapshot_t` + `statistics()` sind die Route, ueber die ein strategie-besitzendes Organ seine
 * T6-Zahlen meldet (Owner-KERN 04.08. abend-11: T6 = Option B strikt). `noexcept` ist gefordert, weil
 * die Konsumenten-Methode ihrerseits noexcept ist (axis_04_..._store.hpp:354/:359).
 *
 * BEDINGTE FORM -- und warum das keine Aufweichung ist: `snapshot_t`/`statistics()` existieren an den
 * Strategien nur unter `COMDARE_CE_ENABLE_STATISTICS` (ebenso wie die konsumierenden Methoden). Die
 * Anforderung spiegelt deshalb GENAU dieselbe bedingte Uebersetzung wie ihr Konsum: in einem Bau ohne
 * Statistik gibt es die Route nicht, sie wird nicht konsumiert, und sie darf folglich auch nicht
 * gefordert werden. Die Alternative -- ein `#ifdef` mitten im requires-Constraint des Konsumenten --
 * haette die Bedingung an den Konsumenten statt an die Achse gehaengt (Schnitt-Regel-Bruch).
 */
#ifdef COMDARE_CE_ENABLE_STATISTICS
template <typename A>
concept StatisticsReportingStrategy = AllocatorStrategy<A> && requires(A a) {
    typename A::snapshot_t;
    { a.statistics() } noexcept -> std::same_as<typename A::snapshot_t>;
};
#else
template <typename A>
concept StatisticsReportingStrategy = AllocatorStrategy<A>;
#endif

} // namespace comdare::cache_engine::alloc::concepts
