#pragma once
// V41.F.6.1 axis_q1_queuing Sub-Concept VersionedBufferStrategy (2026-05-26)
//
// @topic queuing @achse Q1
//
// Sub-Concept fuer versionierte Buffer-Strategien. Erfuellt von Wrappern
// die einen monoton steigenden Versions-Counter exponieren.
//
// Pflicht-Pattern: jede Klasse mit `is_versioned() == true` MUSS dieses
// Sub-Concept erfuellen. Vorher hatten 4 versionierte Strategien 3 verschiedene
// Accessor-Namen (current_version/snapshot_version/epoch_id) + 1 fehlend
// (Tombstone). Konsolidiert auf `version_id()`.

#include "axis_q1_queuing_concept.hpp"

#include <concepts>
#include <cstdint>

namespace comdare::cache_engine::queuing::axis_q1_queuing::concepts {

/**
 * @brief VersionedBufferStrategy — Pflicht-API fuer versionierte Buffer-Strategien
 *
 * Verfeinerung des BufferStrategy-Basiskonzepts. Wrapper-Klassen mit
 * `is_versioned() == true` muessen `version_id() const noexcept -> std::uint64_t`
 * anbieten — monoton steigender Counter, dessen Semantik je nach Versioning-
 * Pattern variiert:
 *   - DeltaChain      → Anzahl appended Deltas (Append-Versioning)
 *   - TombstoneBuffer → Anzahl put-Operations (Operation-Versioning)
 *   - CopyOnWrite     → Snapshot-Index (Snapshot-Versioning)
 *   - EpochBuffer     → aktuelle Epoch-ID (Reclamation-Window-Versioning)
 *
 * Konvention: monotonic steigend, startet bei 0, inkrementiert bei jedem
 * versioning-relevanten Event. CacheEngineBuilder kann via version_id()-Diff
 * den "Versions-Drift" zwischen Mess-Reihen messen.
 */
template <typename B>
concept VersionedBufferStrategy =
    BufferStrategy<B>
    && requires(B const& bc) {
        { bc.version_id() } noexcept -> std::convertible_to<std::uint64_t>;
    };

}  // namespace
