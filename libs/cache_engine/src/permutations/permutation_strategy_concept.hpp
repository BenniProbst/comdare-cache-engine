#pragma once
// V41.F.6.1.A Allgemeines Permutations-Konzept (Topic-uebergreifend) (2026-05-25)
//
// @stand V41.F.6.1.A Skelett
//
// **Architektur-Ebene (User-Direktive 2026-05-25):**
//   src/permutations/ = allgemeines Permutations-Konzept (Topic-uebergreifend)
//
//   Jede Achse in jedem Topic (z.B. allocator/axis_06) spezialisiert dieses
//   Concept fuer ihre eigene Permutations-Strategie. Lokal-Registrierung der
//   Achsen-Permutationen passiert in der jeweiligen Achse (z.B. via
//   AxisVariantList<...> mit allen Vendor-Implementierungen) — Compile-Time
//   meta-programmiert.
//
// **NOCH SKELETT (V41.F.6.1.A Pilot):** dieses File ist Architektur-Marker.
// Konkrete Permutations-Komponenten (AxisVariantList, PermutationVisitor,
// cartesian_product_t, ValidatedPermutation mit Cross-Constraints) folgen in
// F.6.1.E-Iterationen (vgl. docs/architektur/11_*.md §11.7 + §11.8).

#include <concepts>

namespace comdare::cache_engine::permutations::concepts {

/**
 * @brief PermutationStrategy - Topic-uebergreifendes Permutations-Concept (Skelett)
 *
 * **Verwendung in Topic-Achsen:** jede Achse spezialisiert dieses Concept mit ihrer
 * Achsen-spezifischen Permutations-Strategie (z.B. CacheEnginePermutationStrategy
 * fuer allocator, TraversalPermutationStrategy fuer traversal, etc.).
 *
 * Compile-Time-Eigenschaften die alle Permutations-Strategien gemeinsam haben:
 *   - typename axis_tag
 *   - typename family_id
 *   - static constexpr Eigenschaften (is_thread_safe etc.)
 *   - Identifikation (name, family_name)
 *
 * TODO V41.F.6.1.X: gemeinsame Pflicht-API extrahieren wenn 2. Topic angewandt wird.
 *
 * Heute (Pilot): Marker-Concept ohne API-Vertrag. Spezialisierung pro Achse:
 *   topics/<topic>/<axis_NN>/concepts/<axis_NN>_<topic>_cache_engine_permutation_concept.hpp
 */
template <typename T>
concept PermutationStrategy =
    // TODO: gemeinsame Permutations-API extrahieren wenn Pattern auf 2. Topic angewandt wird
    true;

}  // namespace comdare::cache_engine::permutations::concepts
