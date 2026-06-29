#pragma once
// V41.F.6.1.F1 Topic memory_layout Marker-Concept (2026-05-26 Fundament-Sprint)
//
// @topic memory_layout
// @stand V41.F.6.1.F1 Stufe-A Skelett
//
// Topic-Marker fuer Speicher-Layouts (Master-Doc §11.7.A). 1 Achse:
//   - axis_05_memory_layout (AoS/SoA/AoSoA/CacheLineAligned/PackedStructs)

#include <concepts>

namespace comdare::cache_engine::memory_layout::concepts {

struct MemoryLayoutTopicTag {};

template <typename T>
concept MemoryLayoutComponent =
    requires { typename T::topic_tag; } && std::same_as<typename T::topic_tag, MemoryLayoutTopicTag>;

} // namespace comdare::cache_engine::memory_layout::concepts
