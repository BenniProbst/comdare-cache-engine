#pragma once
// V41.F.6.1 axis_q2_queuing Sub-Concept IterableAspectFlushStrategy (2026-05-26)
//
// @topic queuing @achse Q2
//
// Sub-Concept fuer Flush-Policies mit hybrider Laufzeit-Permutation
// (iterable_aspect_t — analog Q1 IterableAspectStrategy). Erfuellt von Policies
// die einen iterable Aspekt zur Laufzeit umstellen koennen.
//
// Pflicht-Pattern: jede Policy-Klasse mit `using iterable_aspect_t = ...` (kein
// void) MUSS dieses Sub-Concept erfuellen. Konsolidierter Setter-Name
// `set_iterable_aspect()` analog Q1-Schablone.

#include "axis_q2_queuing_concept.hpp"

#include <concepts>
#include <span>
#include <type_traits>

namespace comdare::cache_engine::queuing::axis_q2_queuing::concepts {

/**
 * @brief IterableAspectFlushStrategy — Pflicht-API fuer hybride Laufzeit-Permutation
 *
 * Verfeinerung des FlushPolicy-Basiskonzepts. Policy-Klassen mit
 * `using iterable_aspect_t = T` (kein void) muessen:
 *   - `static constexpr std::span<T const> iterable_values() noexcept`
 *   - `set_iterable_aspect(T value) noexcept` — Runtime-Setter
 *
 * **Concept-Erfuellung:**
 *   - WatermarkFlush (FS2 threshold, unsigned pct {50/65/75/85/95})
 *   - TimedFlush     (FS3 time, std::size_t window_ms {10/100/1000/10000})
 *
 * **Nicht erfuellt von:** EagerFlush, LazyFlush (event-driven, kein iterable
 * Aspekt), AdaptiveLsmFlush (lernt selbst aus Workload, kein iterable Aspekt
 * im klassischen Sinn).
 *
 * PermutationEngine erkennt via HasIterableAspect<P> und generiert 1 Binary
 * mit Runtime-Loop ueber iterable_values() statt N separate Binaries.
 */
template <typename P>
concept IterableAspectFlushStrategy = FlushPolicy<P> && requires {
    typename P::iterable_aspect_t;
} && (!std::is_void_v<typename P::iterable_aspect_t>) && requires {
    { P::iterable_values() } noexcept -> std::convertible_to<std::span<typename P::iterable_aspect_t const>>;
} && requires(P p, typename P::iterable_aspect_t v) {
    { p.set_iterable_aspect(v) } noexcept;
};

} // namespace comdare::cache_engine::queuing::axis_q2_queuing::concepts
