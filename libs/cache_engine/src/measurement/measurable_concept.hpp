#pragma once
// V41.F.6.1.A Allgemeines Mess-Konzept (Topic-uebergreifend) (2026-05-25)
//
// @stand V41.F.6.1.A Skelett
//
// **Architektur-Ebene (User-Direktive 2026-05-25):**
//   src/measurement/ = allgemeines Mess-Konzept (Topic-uebergreifend)
//
//   Jede Achse in jedem Topic (z.B. allocator/axis_06) spezialisiert dieses
//   Concept fuer ihre eigene Statistik-Struktur (z.B. AllocationStatistics).
//   Spezialisierung lebt in der Achse selbst, NICHT hier.
//
// **CMake-Flag COMDARE_CE_ENABLE_STATISTICS:**
//   ON  (Default): MeasurableComponent ist Pflicht — alle Topic+Achsen verlangen
//                  statistics() + reset() in ihren CacheEnginePermutationStrategy-Concepts.
//   OFF: MeasurableComponent ist trivial erfuellt — Topic+Achsen-Implementierungen
//        #ifdef'n statistics()+reset() komplett aus dem Binary (kein Overhead).
//
// **NOCH SKELETT (V41.F.6.1.A Pilot):** dieses File ist Architektur-Marker.
// Konkrete Mess-Komponenten (TestDataSetAccumulationEngine, BenchmarkRunner mit
// 2 Custom-Allocations, sparse Byte-State-Trace, Conversion-Routinen) folgen in
// F.6.1.X-Iterationen (vgl. UML REV7 §7+§8 + docs/sessions/.../session-end §9.5).

#include <concepts>

namespace comdare::cache_engine::measurement::concepts {

/**
 * @brief MeasurableComponent - Topic-uebergreifendes Mess-Concept (Skelett)
 *
 * **Verwendung in Topic-Achsen:** jede Achse spezialisiert dieses Concept mit ihrer
 * konkreten Statistik-Struktur (z.B. AllocationStatistics fuer allocator,
 * TraversalStatistics fuer traversal, etc.).
 *
 * TODO V41.F.6.1.X: konkrete Pflicht-API ergaenzen wenn 2. Topic (queuing, traversal, ...)
 * gleichartige Mess-API benoetigt — dann gemeinsame Anforderungen extrahieren.
 *
 * Heute (Pilot): Marker-Concept ohne API-Vertrag. Spezialisierung pro Achse:
 *   topics/<topic>/<axis_NN>/concepts/<axis_NN>_<topic>_cache_engine_permutation_concept.hpp
 */
template <typename T>
concept MeasurableComponent =
    // TODO: gemeinsame Mess-API extrahieren wenn Pattern auf 2. Topic angewandt wird
    true;

}  // namespace comdare::cache_engine::measurement::concepts
