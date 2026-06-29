#pragma once
// V41.F.6.1.F1 Topic concurrency Marker-Concept (2026-05-26 Fundament-Sprint)
//
// @topic concurrency
// @stand V41.F.6.1.F1 Stufe-A Skelett
//
// Topic-Marker fuer Concurrency-Patterns (Master-Doc §11.7.A). 1 Achse mit Subs:
//   - axis_08_concurrency (Sub 8.1 Pattern, 8.2 Locking-Mode, 8.3 Coherence)
//     Pattern: OLC, HTM, STM, lock-free, wait-free, RCU, HP
//     Locking-Mode: read-only, read-write, upgradeable, optimistic-validation

#include <concepts>

namespace comdare::cache_engine::concurrency::concepts {

struct ConcurrencyTopicTag {};

template <typename T>
concept ConcurrencyComponent =
    requires { typename T::topic_tag; } && std::same_as<typename T::topic_tag, ConcurrencyTopicTag>;

} // namespace comdare::cache_engine::concurrency::concepts
