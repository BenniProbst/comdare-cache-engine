#pragma once
// V41.F.6.1 axis_q1_queuing Sub-Concept BoundedBufferStrategy (2026-05-26)
//
// @topic queuing @achse Q1
//
// Sub-Concept fuer bounded Buffer-Strategien (analog Allocator-Sub-Concept-
// Pattern, z.B. IntrospectableStrategy). Erfuellt von Wrappern die eine
// feste maximale Kapazitaet haben.
//
// Pflicht-Pattern: jede Klasse mit `is_bounded() == true` MUSS dieses
// Sub-Concept erfuellen. Konsistenz-Garantie: PermutationEngine kann via
// `BoundedBufferStrategy<B>` filtern und generisch `b.capacity()` aufrufen.

#include "axis_q1_queuing_concept.hpp"

#include <concepts>
#include <cstddef>

namespace comdare::cache_engine::queuing::axis_q1_queuing::concepts {

/**
 * @brief BoundedBufferStrategy — Pflicht-API fuer bounded Buffer-Strategien
 *
 * Verfeinerung des BufferStrategy-Basiskonzepts. Wrapper-Klassen mit
 * `is_bounded() == true` muessen `capacity() const noexcept -> std::size_t`
 * anbieten.
 *
 * **Concept-Erfuellung:**
 *   - BoundedRing
 *   - LockFreeSPSC
 *   - LockFreeMPMC
 *
 * **Unbounded Wrapper (NICHT Concept-erfuellend):** FIFOQueue, LIFOStack,
 * AppendOnly, PriorityHeap, DeltaChain, SkiplistBuffer, TombstoneBuffer,
 * CopyOnWrite, EpochBuffer, BatchedInsertBuffer, NoBuffer.
 */
template <typename B>
concept BoundedBufferStrategy =
    BufferStrategy<B>
    && requires(B const& bc) {
        { bc.capacity() } noexcept -> std::convertible_to<std::size_t>;
    };

}  // namespace
