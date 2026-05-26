#pragma once
// V41.F.6.1 axis_03a_search_algo Sub-Concept SimdCapableStrategy (2026-05-26)
//
// @topic traversal @achse 03a
//
// Sub-Concept fuer Such-Algorithmen mit SIMD-Fast-Path-Implementation.
// Erfuellt von Wrappern die `simd_lookup(key) -> std::optional<value_type>`
// als zusaetzliche Schnellpfad-Methode bieten (typisch dense Strategien mit
// linear scan + SSE/AVX).

#include "axis_03a_search_algo_concept.hpp"

#include <concepts>
#include <optional>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::concepts {

/**
 * @brief SimdCapableStrategy — Pflicht-API fuer SIMD-faehige Such-Strategien
 *
 * Verfeinerung des SearchAlgoVariant-Basiskonzepts. Pflicht-Pattern:
 * jede Klasse mit `supports_simd() == true` MUSS dieses Sub-Concept
 * erfuellen (semantisch: `simd_lookup` MUSS gleiche Resultate wie `lookup`
 * liefern, nur schneller).
 *
 * **Concept-Erfuellung:** Array256 (dense + SIMD), VectorU8U8 (Patricia + SIMD).
 * **Nicht erfuellt von:** VectorU16U16 (Multilevel-DP, kein SIMD).
 */
template <typename S>
concept SimdCapableStrategy =
    SearchAlgoVariant<S>
    && requires(S const& sc, typename S::key_type k) {
        { sc.simd_lookup(k) } -> std::same_as<std::optional<typename S::value_type>>;
    };

}  // namespace
