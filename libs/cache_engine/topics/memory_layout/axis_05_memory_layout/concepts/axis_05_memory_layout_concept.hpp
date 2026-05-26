#pragma once
// V41.F.6.1.F1 axis_05_memory_layout Standard-Concept (Skelett-Stufe-A)

#include "../../concepts/topic_memory_layout_concept.hpp"
#include <concepts>
#include <cstddef>

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout::concepts {

/// MemoryLayoutStrategy — Pflicht-API: cache_line_alignment, layout_pattern (enum).
template <typename L>
concept MemoryLayoutStrategy =
    ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutComponent<L>
    && requires {
        { L::cache_line_size() } noexcept -> std::convertible_to<std::size_t>;
    };

}  // namespace
