#pragma once
// V41.F.6.1 Topic traversal Marker-Concept (2026-05-26)
//
// @topic traversal
// @stand V41.F.6.1 Topic traversal Pilot
//
// Topic-Marker fuer Such-Algorithmen + Cache-Traversal + Mapping (Master-Doc
// §11.7.A). 3 Achsen unter diesem Topic:
//   - axis_03a_search_algo     (Density-basierte Sub-Search-Dispatch)
//   - axis_03b_cache_traversal (Default-Lookup-Traversal)
//   - axis_03m_mapping         (VirtualOffsetAddress-Mapping)
//
// **Meta-driven Pattern** [[meta-driven-concept-hardening-pattern]]:
// dieses Topic-Marker-Concept ist die M2-Schicht (Meta-Modell) — alle
// konkreten Wrapper sind M1-Instanzen. Sub-Concepts der Achsen sind
// M2-Refinements (analog Java interface extends).

#include <concepts>

namespace comdare::cache_engine::traversal::concepts {

/**
 * @brief TraversalTopicTag — Marker-Struct fuer Topic-Klassifikation
 *
 * Jeder traversal-Wrapper hat `using topic_tag = TraversalTopicTag;`
 * CacheEngineBuilder erkennt damit zu welchem Topic eine Klasse gehoert.
 */
struct TraversalTopicTag {};

/**
 * @brief TraversalComponent — Topic-Ebene Concept (breit, fuer alle Achsen)
 *
 * Pflicht: jede Klasse die unter traversal/ liegt MUSS topic_tag == TraversalTopicTag haben.
 */
template <typename T>
concept TraversalComponent =
    requires { typename T::topic_tag; } && std::same_as<typename T::topic_tag, TraversalTopicTag>;

} // namespace comdare::cache_engine::traversal::concepts
