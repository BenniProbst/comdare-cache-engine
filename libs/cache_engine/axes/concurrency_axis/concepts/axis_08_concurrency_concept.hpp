#pragma once
// V41.F.6.1.F1 axis_08_concurrency Standard-Concept (Skelett-Stufe-A)

#include <topics/concurrency/concepts/topic_concurrency_concept.hpp>
#include <concepts>

namespace comdare::cache_engine::concurrency_axis::concepts {

enum class ConcurrencyPattern : int {
    None = 0, Blocking, ReaderWriter, Optimistic, LockFree, WaitFree, RCU, HazardPtr
};

/// ConcurrencyStrategy — Pflicht-API: concurrency_pattern() Enum-Klassifikation.
template <typename C>
concept ConcurrencyStrategy =
    ::comdare::cache_engine::concurrency::concepts::ConcurrencyComponent<C>
    && requires {
        { C::concurrency_pattern() } noexcept -> std::convertible_to<ConcurrencyPattern>;
    };

}  // namespace
