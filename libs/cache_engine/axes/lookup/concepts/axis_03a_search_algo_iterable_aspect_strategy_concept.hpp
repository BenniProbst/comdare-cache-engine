#pragma once
// V41.F.6.1 axis_03a_search_algo Sub-Concept IterableAspectSearchAlgoStrategy (2026-05-26)
//
// @topic traversal @achse 03a
//
// Sub-Concept fuer Such-Algorithmen mit hybrider Laufzeit-Permutation
// (iterable_aspect_t — analog Q1 IterableAspectStrategy + Q2 IterableAspectFlushStrategy).
// Erfuellt von Wrappern die einen iterable Aspekt zur Laufzeit umstellen koennen
// (z.B. density_threshold bei VectorU8U8SearchAlgo, fanout_size bei adaptiven Strategien).

#include "axis_03a_search_algo_concept.hpp"

#include <concepts>
#include <span>
#include <type_traits>

namespace comdare::cache_engine::lookup::concepts {

/**
 * @brief IterableAspectSearchAlgoStrategy — Pflicht-API fuer hybride Laufzeit-Permutation
 *
 * Verfeinerung des SearchAlgoVariant-Basiskonzepts. Wrapper-Klassen mit
 * `using iterable_aspect_t = T` (kein void) muessen:
 *   - `static constexpr std::span<T const> iterable_values() noexcept`
 *   - `set_iterable_aspect(T value) noexcept` — Runtime-Setter
 *
 * **Konsolidierter Setter-Name** `set_iterable_aspect` analog Q1/Q2 Schablone.
 *
 * **Concept-Erfuellung:** VectorU8U8SearchAlgo (density_threshold pct {10/20/30/50/70}).
 * **Nicht erfuellt von:** Array256SearchAlgo (Dense per Definition), VectorU16U16SearchAlgo (Multilevel
 * Cost-DP — keine threshold-Parametrisierung).
 */
template <typename S>
concept IterableAspectSearchAlgoStrategy =
    SearchAlgoVariant<S>
    && requires { typename S::iterable_aspect_t; }
    && (!std::is_void_v<typename S::iterable_aspect_t>)
    && requires {
        { S::iterable_values() } noexcept
            -> std::convertible_to<std::span<typename S::iterable_aspect_t const>>;
    }
    && requires(S s, typename S::iterable_aspect_t v) {
        { s.set_iterable_aspect(v) } noexcept;
    };

}  // namespace
