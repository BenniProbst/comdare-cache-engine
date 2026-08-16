#pragma once
// V41.F.6.1 axis_03b_cache_traversal Sub-Concept IterableAspectCacheTraversalStrategy (2026-05-26)
//
// @topic traversal @achse 03b
//
// Sub-Concept fuer Cache-Traversal-Strategien mit hybrider Laufzeit-Permutation
// (iterable_aspect_t — analog Q1/Q2 + 03a Sub-Concepts).

#include "axis_03b_cache_traversal_concept.hpp"

#include <concepts>
#include <span>
#include <type_traits>

namespace comdare::cache_engine::cache_traversal::concepts {

/**
 * @brief IterableAspectCacheTraversalStrategy — Pflicht-API fuer hybride Permutation
 *
 * **Concept-Erfuellung:** HashLookup (initial_capacity {8/16/64/256/1024}).
 * **Nicht erfuellt von:** LinearFanout (kein iterable Aspekt).
 *
 * Konsolidierter Setter-Name `set_iterable_aspect()` analog allen anderen
 * iterable Topic-Schablonen.
 */
template <typename T>
concept IterableAspectCacheTraversalStrategy = CacheTraversalVariant<T> && requires {
    typename T::iterable_aspect_t;
} && (!std::is_void_v<typename T::iterable_aspect_t>) && requires {
    { T::iterable_values() } noexcept -> std::convertible_to<std::span<typename T::iterable_aspect_t const>>;
} && requires(T t, typename T::iterable_aspect_t v) {
    // Darf werfen: der Rehash hinter set_iterable_aspect() allokiert. Posten 70 (2026-08-04) praezisiert
    // die Quelle -- seit dem A8-S5-Schnitt laeuft diese Allokation ueber die Allokator-ACHSE, und der
    // StdAllocatorAdapter uebersetzt deren nullptr-OOM in std::bad_alloc (Posten 64). Kein noexcept
    // fordern ist also weiter richtig; nur der Traeger des Wurfs ist ein anderer als vor dem Schnitt.
    { t.set_iterable_aspect(v) };
};

} // namespace comdare::cache_engine::cache_traversal::concepts
