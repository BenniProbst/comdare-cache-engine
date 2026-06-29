#pragma once
// V41.F.6.1.R7.3 axis_08_concurrency Subaxes-Tags
//
// Topic-Doku (Master §11.7.A) nennt 3 Sub-Dimensionen: Pattern / Locking-Mode /
// Coherence. In der Achsen-Implementierung fallen Locking-Mode + Coherence
// natuerlich zusammen zur Reclamation-Dimension (sichere Speicher-Freigabe):
//   - CC1 synchronization_pattern_tag: das Synchronisations-Pattern selbst
//   - CC2 reclamation_scheme_tag: sichere Speicher-Reclamation (RCU/Hazard)

namespace comdare::cache_engine::concurrency_axis::subaxes {

// CC1: Synchronisations-Pattern (none / blocking / reader-writer / optimistic / lock-free / wait-free)
struct synchronization_pattern_tag {};

// CC2: Reclamation-Schema (RCU / Hazard-Pointer) — orthogonal zum Pattern
struct reclamation_scheme_tag {};

} // namespace comdare::cache_engine::concurrency_axis::subaxes
