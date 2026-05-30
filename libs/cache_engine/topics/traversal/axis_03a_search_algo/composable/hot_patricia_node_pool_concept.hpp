#pragma once
// V41 Umstufung-A s4 (Task #43, Doku 14 §13) — HotPatriciaNodePool-Concept.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// **Warum ein EIGENES Concept (nicht ArtTrieNodePool, nicht TreeNodePool):** HOT (Binna et al. SIGMOD 2018)
// ist ein BIT-level Height-Optimized-Trie — seine Korrektheits-Essenz ist eine BINARY PATRICIA (crit-bit):
// zwei Schluessel werden durch GENAU EIN Bit (das hoechstwertige divergierende, `countl_zero(a^b)`) getrennt,
// nicht-diskriminative Bits werden uebersprungen (Single-Bit-Split Path-Compression). Das ist eine DISTINKTE
// Anatomie ggue. ART (BYTE-level, 256-Fanout, LSB-first): HOT/crit-bit ist BIT-level, 2-Fanout, MSB-first.
// Daher ein SEPARATES Pool-Concept (2 Kind-Typen Leaf+Internal statt ARTs 5; KEIN explizites Prefix-Organ —
// Path-Compression ist implizit durch die uebersprungenen nicht-diskriminativen Bits).
//
// Substrat OHNE Such-Logik: der Pool verwaltet Leaf (Key+Value inline) + Internal{crit_bit, child[2]} +
// die logische Schluesselzahl. Der crit-bit-Descent + Leaf-Split + Erase-Collapse lebt im
// HotPatriciaTraversalOrgan. Die Multi-Bit-Height-Optimization (SparsePartialKeys + SIMD) ist ein
// Folge-Increment (aendert die Semantik NICHT). [[no-runtime-switch]]; [[pseudocode-papers-fallback]].

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable {

/// HOT-PATRICIA-NODE-POOL-Concept: binary crit-bit-Patricia-Knoten-Pool (HOT-Korrektheits-Basis) ueber uint64-Key.
/// Erfuellt von HotPatriciaNodePoolStore; konsumiert von HotPatriciaTraversalOrgan.
template <class S>
concept HotPatriciaNodePool =
    requires {
        typename S::key_type;
        typename S::value_type;
        { S::kNil } -> std::convertible_to<std::size_t>;
    }
    && std::same_as<typename S::key_type,   std::uint64_t>
    && std::same_as<typename S::value_type, std::uint64_t>
    && requires(S& s, S const& cs, std::size_t ref, unsigned bit,
                typename S::key_type k, typename S::value_type v) {
        // (A) Wurzel + Groesse
        { cs.root() }   -> std::convertible_to<std::size_t>;
        { cs.size() }   -> std::convertible_to<std::size_t>;
        { s.set_root(ref) } -> std::same_as<void>;
        { s.inc_size() }    -> std::same_as<void>;
        { s.dec_size() }    -> std::same_as<void>;
        { s.clear() }       -> std::same_as<void>;
        // (B) Leaf
        { cs.is_leaf(ref) }          -> std::same_as<bool>;
        { cs.leaf_key(ref) }         -> std::same_as<typename S::key_type>;
        { cs.leaf_value(ref) }       -> std::same_as<typename S::value_type>;
        { s.set_leaf_value(ref, v) } -> std::same_as<void>;
        { s.new_leaf(k, v) }         -> std::convertible_to<std::size_t>;   // darf werfen
        // (C) Internal (binaere crit-bit-Patricia, GENAU 2 Kinder)
        { cs.crit_bit(ref) }              -> std::convertible_to<unsigned>;     // 0..63 (MSB-first)
        { cs.child(ref, bit) }            -> std::convertible_to<std::size_t>;  // bit in {0,1}
        { s.set_child(ref, bit, ref) }    -> std::same_as<void>;
        { s.new_internal(bit, ref, ref) } -> std::convertible_to<std::size_t>; // (crit_bit, child0, child1); darf werfen
        { s.free_node(ref) }              -> std::same_as<void>;
    };

}  // namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable
