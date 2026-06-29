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
    { t.set_iterable_aspect(v) }; // darf werfen (Rehash kann std::bad_alloc werfen)
};

} // namespace comdare::cache_engine::cache_traversal::concepts
