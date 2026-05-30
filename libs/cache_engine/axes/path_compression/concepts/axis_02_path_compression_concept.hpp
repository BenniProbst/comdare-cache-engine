#pragma once
// V41.F.6.1.F1 axis_02_path_compression Standard-Concept (2026-05-26 Skelett-Stufe-A)

#include <topics/nodes/concepts/topic_nodes_concept.hpp>
#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::path_compression::concepts {

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

/**
 * @brief ByteSkipPathCompression — ECHTES Byte-Prefix-Skip-Organ (V41 #43 s4, ART-Anatomie).
 *
 * SEPARATES, optional-detektiertes Concept fuer das eigentliche Path-Compression-ORGAN (das gepackte
 * Byte-Prefix einer Trie-Inner-Node), abgegrenzt vom obigen Tag-Strategy-Concept PathCompressionStrategy
 * (nur compression_ratio()). Bricht die bestehenden Strategy-static_asserts NICHT (additiv).
 *
 * Pflicht-API (verbatim aus unodb::key_prefix, art_internal_impl.hpp:877-1070):
 *   - length()                       → Anzahl Prefix-Bytes
 *   - operator[](i)                  → Prefix-Byte i
 *   - common_prefix_len(shifted_key) → gemeinsame Byte-Laenge mit Schluessel (geklemmt auf length())
 *   - cut(n)                         → fuehrende n Bytes entfernen (Abstieg)
 */
template <typename P>
concept ByteSkipPathCompression =
    requires(P p, P const& cp, std::uint64_t key, unsigned i) {
        { cp.length() }                  -> std::convertible_to<unsigned>;
        { cp[i] }                        -> std::convertible_to<std::uint8_t>;
        { cp.common_prefix_len(key) }    -> std::convertible_to<unsigned>;
        { p.cut(i) }                     -> std::same_as<void>;
    };

}  // namespace
