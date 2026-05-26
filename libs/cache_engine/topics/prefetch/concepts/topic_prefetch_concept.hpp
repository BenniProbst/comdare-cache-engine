#pragma once
// V41.F.6.1.F1 Topic prefetch Marker-Concept (2026-05-26 Fundament-Sprint)
//
// @topic prefetch
// @stand V41.F.6.1.F1 Stufe-A Skelett
//
// Topic-Marker fuer Prefetch-Strategien (Master-Doc §11.7.A). 1 Achse:
//   - axis_07_prefetch (None/Stride/DistanceEstimator/PathOriented/HierarchicalBundle)

#include <concepts>

namespace comdare::cache_engine::prefetch::concepts {

struct PrefetchTopicTag {};

template <typename T>
concept PrefetchComponent = requires {
    typename T::topic_tag;
} && std::same_as<typename T::topic_tag, PrefetchTopicTag>;

}  // namespace
