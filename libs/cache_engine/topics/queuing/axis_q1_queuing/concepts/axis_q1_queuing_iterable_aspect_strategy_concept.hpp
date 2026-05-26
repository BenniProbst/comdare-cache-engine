#pragma once
// V41.F.6.1 axis_q1_queuing Sub-Concept IterableAspectStrategy (2026-05-26)
//
// @topic queuing @achse Q1
//
// Sub-Concept fuer Buffer-Strategien mit hybrider Laufzeit-Permutation
// (iterable_aspect_t, siehe Master-Doc §15.5). Erfuellt von Wrappern die
// einen iterable Aspekt zur Laufzeit umstellen koennen.
//
// Pflicht-Pattern: jede Klasse mit `using iterable_aspect_t = ...` (kein void)
// MUSS dieses Sub-Concept erfuellen. Vorher hatten 3 betroffene Strategien
// 2 verschiedene Setter-Namen (set_capacity/set_epoch_threshold) + 1 fehlend
// (LockFreeMPMC). Konsolidiert auf `set_iterable_aspect()`.

#include "axis_q1_queuing_concept.hpp"

#include <concepts>
#include <span>
#include <type_traits>

namespace comdare::cache_engine::queuing::axis_q1_queuing::concepts {

/**
 * @brief IterableAspectStrategy — Pflicht-API fuer hybride Laufzeit-Permutation
 *
 * Verfeinerung des BufferStrategy-Basiskonzepts. Wrapper-Klassen mit
 * `using iterable_aspect_t = T` (kein void) muessen:
 *   - `static constexpr std::span<T const> iterable_values() noexcept`
 *   - `set_iterable_aspect(T value)` — Setter mit Reallokation falls notwendig
 *
 * **Concept-Erfuellung:**
 *   - BoundedRing     (capacity, kein-Realloc-noexcept)
 *   - EpochBuffer     (epoch_threshold, noexcept)
 *   - LockFreeSPSC    (capacity, NICHT noexcept — Buffer-Realloc)
 *   - LockFreeMPMC    (capacity, NICHT noexcept — Cell-Array-Realloc, Power-of-2 Check)
 *
 * **Setter darf werfen** ([[allocation-failure-exception]]): bei Reallokation
 * std::bad_alloc moeglich; LockFreeMPMC zusaetzlich std::invalid_argument bei
 * nicht-Power-of-2 ([[zero-size-allocation-exception]]). Daher KEIN noexcept
 * im Concept gefordert.
 *
 * PermutationEngine erkennt via HasIterableAspect<V> (= requires { typename V::iterable_aspect_t; })
 * und generiert 1 Binary mit Runtime-Loop ueber iterable_values() statt
 * N separate Binaries.
 */
template <typename B>
concept IterableAspectStrategy =
    BufferStrategy<B>
    && requires { typename B::iterable_aspect_t; }
    && (!std::is_void_v<typename B::iterable_aspect_t>)
    && requires {
        { B::iterable_values() } noexcept
            -> std::convertible_to<std::span<typename B::iterable_aspect_t const>>;
    }
    && requires(B b, typename B::iterable_aspect_t v) {
        { b.set_iterable_aspect(v) };  // darf werfen — siehe Doku oben
    };

}  // namespace
