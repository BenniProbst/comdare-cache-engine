#pragma once
// V41.F.6.1.F1 axis_07_prefetch Standard-Concept (Skelett-Stufe-A)

#include "../../concepts/topic_prefetch_concept.hpp"
#include <concepts>

namespace comdare::cache_engine::prefetch::axis_07_prefetch::concepts {

/// PrefetchStrategy — Pflicht-API: is_active() (true wenn HW-Prefetch genutzt).
template <typename P>
concept PrefetchStrategy =
    ::comdare::cache_engine::prefetch::concepts::PrefetchComponent<P>
    && requires {
        { P::is_active() } noexcept -> std::convertible_to<bool>;
    };

}  // namespace
