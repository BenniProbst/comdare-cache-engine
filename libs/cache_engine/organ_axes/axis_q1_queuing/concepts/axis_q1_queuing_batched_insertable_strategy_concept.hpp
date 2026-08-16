#pragma once
// V41.F.6.1 axis_q1_queuing Sub-Concept BatchedInsertableStrategy (2026-05-26)
//
// @topic queuing @achse Q1
//
// Sub-Concept fuer Buffer-Strategien die Batch-Insert effizient unterstuetzen
// (OLAP-Indexe, ART-Bulk-Insert). Erfuellt von Wrappern die `bulk_insert(span)`
// als amortisierten Schnellpfad bieten — alternativ zu N × put().

#include "axis_q1_queuing_concept.hpp"

#include <concepts>
#include <span>

namespace comdare::cache_engine::queuing::axis_q1_queuing::concepts {

/**
 * @brief BatchedInsertableStrategy — Pflicht-API fuer Batch-Insert-Strategien
 *
 * Verfeinerung des BufferStrategy-Basiskonzepts. Wrapper-Klassen mit Subaxis
 * QS5 batched_access SOLLEN dieses Sub-Concept erfuellen.
 *
 * **Concept-Erfuellung:**
 *   - BatchedInsertBuffer (Q12, QS5 batched_access)
 *
 * Andere Strategien koennten es spaeter ebenfalls erfuellen (z.B. SkiplistBuffer
 * mit Bulk-Insert-Optimierung, BoundedRingBuffer mit bulk_put). Nicht-Pflicht.
 *
 * Semantik: bulk_insert(span) ist semantisch aequivalent zu N × put(span[i]),
 * aber amortisiert effizienter (z.B. via SIMD-Vectorization, Cache-Line-Burst,
 * Pre-Allocation).
 *
 * **bulk_insert darf werfen** ([[allocation-failure-exception]]): bei
 * Allokations-Fehlern std::bad_alloc moeglich.
 */
template <typename B>
concept BatchedInsertableStrategy =
    BufferStrategy<B> && requires(B b, std::span<typename B::element_type const> batch) {
        { b.bulk_insert(batch) }; // darf werfen
    };

} // namespace comdare::cache_engine::queuing::axis_q1_queuing::concepts
