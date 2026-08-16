#pragma once
// V41.F.6.1 axis_03b_cache_traversal Sub-Concept HashedTraversalStrategy (2026-05-26)
//
// @topic traversal @achse 03b
//
// Sub-Concept fuer hash-basierte Cache-Traversal-Strategien. Erfuellt von
// Wrappern mit `is_hashed() == true` — Pflicht-API `bucket_count()` und
// `load_factor()` fuer Mess-Reihen + Re-Hash-Heuristik (CacheEngineBuilder
// kann via load_factor() entscheiden ob Resize sinnvoll ist).

#include "axis_03b_cache_traversal_concept.hpp"

#include <concepts>
#include <cstddef>

namespace comdare::cache_engine::cache_traversal::concepts {

/**
 * @brief HashedTraversalStrategy — Pflicht-API fuer hash-basierte Traversal-Strategien
 *
 * Verfeinerung des CacheTraversalVariant-Basiskonzepts. Wrapper-Klassen mit
 * `is_hashed() == true` MUSS dieses Sub-Concept erfuellen.
 *
 * **Concept-Erfuellung:** HashLookup (Fibonacci-Hash).
 * **Nicht erfuellt von:** LinearFanout (linear-scan, kein Hash).
 *
 * Semantik:
 *   - bucket_count() — aktuelle Hash-Tabellen-Groesse (Power-of-2 fuer Fibonacci)
 *   - load_factor()  — tracked_count() / bucket_count() in [0.0, 1.0]
 */
template <typename T>
concept HashedTraversalStrategy = CacheTraversalVariant<T> && requires(T const& tc) {
    { tc.bucket_count() } noexcept -> std::convertible_to<std::size_t>;
    { tc.load_factor() } noexcept -> std::convertible_to<double>;
};

} // namespace comdare::cache_engine::cache_traversal::concepts
