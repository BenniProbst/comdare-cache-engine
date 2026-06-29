#pragma once
// V41.F.6.1.A Topic-Concept Allocator (2026-05-25)
//
// @topic allocator
// @stand V41.F.6.1.A Pilot
//
// Topic-Concept erklaert WELCHE Achsen unter dem Allocator-Topic liegen.
// Es ist ein Marker-Concept und enthaelt KEINE eigenen API-Vertraege —
// die konkreten Funktions-Anforderungen liegen in den Achsen-Concepts
// (z.B. topics/allocator/axis_06_allocator/concepts/axis_06_allocator_concept.hpp).
//
// Konvention:
//   - Jede Klasse die zum Allocator-Topic gehoert traegt
//       `using topic_tag = AllocatorTopicTag;`
//   - Konkrete API-Anforderungen kommen aus dem jeweiligen Achsen-Concept
//
// Verwendung:
//   template <typename T>
//   void register_if_allocator(T&) requires AllocatorComponent<T>;

#include <concepts>
#include <type_traits>

namespace comdare::cache_engine::allocator::concepts {

/**
 * @brief AllocatorTopicTag - Compile-Time Marker fuer Topic-Zugehoerigkeit
 * @topic allocator
 */
struct AllocatorTopicTag {};

/**
 * @brief AllocatorComponent - Topic-Marker-Concept
 * @topic allocator
 *
 * Jede Klasse die zur Allocator-Topic-Familie gehoert muss `topic_tag`
 * definieren und es muss `AllocatorTopicTag` sein. Konkrete API-
 * Anforderungen kommen aus den Achsen-Concepts darunter.
 *
 * Beispiel-Klasse die das Topic-Concept erfuellt:
 *   struct StdMalloc {
 *       using topic_tag = AllocatorTopicTag;
 *       // ... weitere Achsen-Concept-API (raw_allocate etc.)
 *   };
 */
template <typename T>
concept AllocatorComponent = requires {
    typename T::topic_tag;
    requires std::same_as<typename T::topic_tag, AllocatorTopicTag>;
};

} // namespace comdare::cache_engine::allocator::concepts
