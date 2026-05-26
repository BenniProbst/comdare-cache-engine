#pragma once
// V41.F.6.1.F1 axis_02_path_compression Standard-Concept (2026-05-26 Skelett-Stufe-A)

#include "../../concepts/topic_nodes_concept.hpp"
#include <concepts>
#include <cstddef>

namespace comdare::cache_engine::nodes::axis_02_path_compression::concepts {

/**
 * @brief PathCompressionStrategy — Pflicht-API fuer Path-Compression-Strategien
 *
 * Standard-Pflicht-API (analog SearchAlgoVariant Pattern):
 *   - compress(path) → komprimierte Repräsentation
 *   - decompress(rep) → Original-Pfad
 *   - compression_ratio() → Mass fuer Kompressions-Effizienz (0.0-1.0)
 */
template <typename P>
concept PathCompressionStrategy =
    ::comdare::cache_engine::nodes::concepts::NodesComponent<P>
    && requires(P p) {
        { p.compression_ratio() } noexcept -> std::convertible_to<double>;
    };

}  // namespace
