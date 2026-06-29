#pragma once
// V41.F.6.1.F1 Topic nodes Marker-Concept (2026-05-26 Fundament-Sprint)
//
// @topic nodes
// @stand V41.F.6.1.F1 Stufe-A Skelett
//
// Topic-Marker fuer Node-Strukturen + Path-Compression (Master-Doc §11.7.A).
// 3 Achsen unter diesem Topic:
//   - axis_01_page_type        (welche Seiten-STRUKTUR: 6 Pflicht-Seitentypen — Achse 1 IPage, V41.F.6)
//   - axis_02_path_compression (wie wird Pfad-Information komprimiert?)
//   - axis_04_node_type        (welche ART-Kapazitaetsklasse: Node4/16/48/256)

#include <concepts>

namespace comdare::cache_engine::nodes::concepts {

struct NodesTopicTag {};

template <typename T>
concept NodesComponent = requires { typename T::topic_tag; } && std::same_as<typename T::topic_tag, NodesTopicTag>;

} // namespace comdare::cache_engine::nodes::concepts
