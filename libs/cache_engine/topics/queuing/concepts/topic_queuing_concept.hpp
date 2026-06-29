#pragma once
// V41.F.6.1 Topic queuing Marker-Concept (2026-05-26)
//
// @topic queuing
// @stand V41.F.6.1 Topic queuing Pilot
//
// Topic-Marker fuer Buffer/Queue-Strategien (W2-Recherche, Doku §11.2).
// 2 Achsen unter diesem Topic (User-Entscheidung 2026-05-26):
//   - axis_q1_queuing (13 Strategien Vollausbau, Pilot: 4)
//   - axis_q2_queuing    (5 Policies Vollausbau, Pilot: 3)

#include <concepts>

namespace comdare::cache_engine::queuing::concepts {

/**
 * @brief QueuingTopicTag — Marker-Struct fuer Topic-Klassifikation
 *
 * Jeder queuing-Wrapper hat `using topic_tag = QueuingTopicTag;`
 * CacheEngineBuilder erkennt damit zu welchem Topic eine Klasse gehoert.
 */
struct QueuingTopicTag {};

/**
 * @brief QueuingComponent — Topic-Ebene Concept (breit, fuer alle Achsen)
 *
 * Pflicht: jede Klasse die unter queuing/ liegt MUSS topic_tag == QueuingTopicTag haben.
 */
template <typename T>
concept QueuingComponent = requires { typename T::topic_tag; } && std::same_as<typename T::topic_tag, QueuingTopicTag>;

} // namespace comdare::cache_engine::queuing::concepts
