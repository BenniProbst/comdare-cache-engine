#pragma once
// V41 Umstufung-A s4 (Task #43, Doku 14 §13) — ArtTrieNodePool-Concept.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// **Warum ein EIGENES Concept (nicht TreeNodePool, nicht StorageOrgan):** Ein Adaptive Radix Tree (Leis
// ICDE 2013) ist ein BYTE-Trie mit (a) ADAPTIVEN Inner-Node-Typen Node4/16/48/256 (Knoten waechst mit der
// Kind-Zahl) und (b) Path-Compression (gepacktes Byte-Prefix je Inner-Node). TreeNodePool (binaer left/right)
// und StorageOrgan (flaches Index-Slot-Array ueber EINEM uint64-Key) koennen einen byte-dispatchenden Trie
// strukturell NICHT abbilden (Befund Planrunde w82140m0g). Daher ein SEPARATES Pool-Concept — alle Bestands-
// Concepts + ihre static_asserts bleiben unberuehrt.
//
// Substrat OHNE Such-Logik: der Pool verwaltet Knoten (Leaf + 4 adaptive Inner-Typen mit Byte-Kind-Dispatch),
// das Byte-Prefix je Inner-Node (ByteWiseKeyPrefix-Organ aus axis_02), adaptives GROWTH (Node4->16->48->256
// daten-getrieben in add_child, KEIN Algorithmus-Runtime-Switch [[no-runtime-switch]]) sowie die logische
// Schluesselzahl. Der Byte-Descent + Leaf-Split + Prefix-Split lebt im ArtTrieTraversalOrgan. Geist gewahrt:
// gemeinsamer uint64-Key; KEIN noexcept-Zwang (Allokation darf werfen, [[allocation-failure-exception]]).

#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_byte_wise.hpp>  // ByteWiseKeyPrefix

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::lookup::composable {

/// ART-TRIE-NODE-POOL-Concept: adaptiver Byte-Trie-Knoten-Pool (Leis ICDE 2013) ueber uint64-Key.
/// Erfuellt von ArtTrieNodePoolStore; konsumiert von ArtTrieTraversalOrgan.
template <class S>
concept ArtTrieNodePool =
    requires {
        typename S::key_type;
        typename S::value_type;
        typename S::prefix_type;                              // ByteWiseKeyPrefix-Organ (axis_02)
        { S::kNil } -> std::convertible_to<std::size_t>;      // "kein Knoten"
    }
    && std::same_as<typename S::key_type, std::uint64_t>
    && std::same_as<typename S::value_type, std::uint64_t>
    && requires(S& s, S const& cs, std::size_t ref, std::uint8_t b,
                typename S::key_type k, typename S::value_type v,
                typename S::prefix_type p, unsigned n) {
        // (A) Wurzel + Groesse
        { cs.root() }   -> std::convertible_to<std::size_t>;
        { cs.size() }   -> std::convertible_to<std::size_t>;   // logische Schluesselzahl
        { s.set_root(ref) } -> std::same_as<void>;
        { s.inc_size() }    -> std::same_as<void>;
        { s.dec_size() }    -> std::same_as<void>;
        { s.clear() }       -> std::same_as<void>;
        // (B) Leaf-Knoten
        { cs.is_leaf(ref) }      -> std::same_as<bool>;
        { cs.leaf_key(ref) }     -> std::same_as<typename S::key_type>;
        { cs.leaf_value(ref) }   -> std::same_as<typename S::value_type>;
        { s.set_leaf_value(ref, v) } -> std::same_as<void>;
        { s.new_leaf(k, v) }     -> std::convertible_to<std::size_t>;   // darf werfen
        // (C) Inner-Knoten (adaptiv) + Path-Compression-Prefix
        { s.new_node4() }        -> std::convertible_to<std::size_t>;   // leerer N4; darf werfen
        { cs.prefix_of(ref) }    -> std::same_as<typename S::prefix_type>;
        { s.set_prefix(ref, p) } -> std::same_as<void>;
        { s.prefix_cut(ref, n) } -> std::same_as<void>;
        { cs.node_n(ref) }       -> std::convertible_to<int>;          // Kind-Zahl
        { cs.find_child(ref, b) }-> std::convertible_to<std::size_t>;  // Kind-Ref oder kNil (Byte-Dispatch)
        { s.add_child(ref, b, ref) } -> std::convertible_to<std::size_t>;  // ggf. GEWACHSENE neue Node-Ref
        { s.set_child(ref, b, ref) } -> std::same_as<void>;            // bestehenden Kind-Slot ersetzen
        { s.remove_child(ref, b) }   -> std::convertible_to<std::size_t>;  // Kind entfernen -> ggf. GESCHRUMPFTER Node-Ref
        { s.free_node(ref) }     -> std::same_as<void>;
    };

}  // namespace comdare::cache_engine::lookup::composable
