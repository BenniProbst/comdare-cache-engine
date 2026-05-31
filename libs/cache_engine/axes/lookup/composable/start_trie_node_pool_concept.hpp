#pragma once
// V41 Umstufung-A s4 (Task #43, Doku 14 §13) — StartTrieNodePool-Concept (START Multibyte-Span-Radix).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// **Warum ein EIGENES Concept (nicht ArtTrieNodePool):** START (Fent/Jungmair/Kipf/Neumann, ICDEW 2020) ist
// ein Self-Tuning Adaptive Radix Tree — ARTs CHARAKTERISTISCHE Erweiterung ist der MULTIBYTE-SPAN: ein Knoten
// dispatcht ueber einen 1-, 2- oder 3-BYTE-Diskriminator (Rewired64K=16-Bit, Rewired16M=24-Bit), nicht ueber
// ARTs fixen 1-Byte. ARTs find_child(ref, uint8) ist hart 1-Byte und schuetzt den ART-Korrektheits-Anker —
// daher ein SEPARATES Pool-Concept mit `find_child(ref, uint32 disc)` + per-Node `span()`. Distinkt von ART
// (fixer 1-Byte-Span), HOT (bit), Wormhole (Leaf-Liste). is_original=false ([[pseudocode-papers-fallback]];
// volle START-Quelle fehlt (nur sosd-Adapter), MIT-Upstream nicht einkopiert -> reine Re-Impl aus Verstaendnis).
//
// Substrat OHNE Such-Logik: der Pool verwaltet Leaf + Inner-Knoten (per-Node span 1/2/3 als DATEN-Feld +
// sparse Diskriminator->Kind-Dispatch) + Path-Compression-Prefix (ByteWiseKeyPrefix, axis_02 wiederverwendet).
// Span-aware Descent + Leaf/Prefix-Split lebt im StartTrieTraversalOrgan. Das Cost-DP-Self-Tuning (welcher
// Span je Subtree) ist die Folge-Achse axis_03t_node_tuning (data-driven Struktur-Wartung wie ART-Growth,
// KEIN Runtime-Switch). [[no-runtime-switch]]; KEIN noexcept-Zwang ([[allocation-failure-exception]]).

#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_byte_wise.hpp>  // ByteWiseKeyPrefix

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::lookup::composable {

/// START-TRIE-NODE-POOL-Concept: Multibyte-Span Adaptive Radix Tree (Fent et al. ICDEW 2020) ueber uint64-Key.
/// Erfuellt von StartTrieNodePoolStore; konsumiert von StartTrieTraversalOrgan.
template <class S>
concept StartTrieNodePool =
    requires {
        typename S::key_type;
        typename S::value_type;
        typename S::prefix_type;                            // ByteWiseKeyPrefix (axis_02)
        { S::kNil } -> std::convertible_to<std::size_t>;
    }
    && std::same_as<typename S::key_type,   std::uint64_t>
    && std::same_as<typename S::value_type, std::uint64_t>
    && requires(S& s, S const& cs, std::size_t ref, std::uint32_t disc, unsigned sp,
                typename S::key_type k, typename S::value_type v, typename S::prefix_type p, unsigned n) {
        // (A) Wurzel + Groesse
        { cs.root() }   -> std::convertible_to<std::size_t>;
        { cs.size() }   -> std::convertible_to<std::size_t>;
        { s.set_root(ref) } -> std::same_as<void>;
        { s.inc_size() }    -> std::same_as<void>;
        { s.dec_size() }    -> std::same_as<void>;
        { s.clear() }       -> std::same_as<void>;
        // (B) Leaf
        { cs.is_leaf(ref) }      -> std::same_as<bool>;
        { cs.leaf_key(ref) }     -> std::same_as<typename S::key_type>;
        { cs.leaf_value(ref) }   -> std::same_as<typename S::value_type>;
        { s.set_leaf_value(ref, v) } -> std::same_as<void>;
        { s.new_leaf(k, v) }     -> std::convertible_to<std::size_t>;   // darf werfen
        // (C) Inner-Knoten (Multibyte-Span) + Path-Compression-Prefix
        { s.new_inner(sp) }      -> std::convertible_to<std::size_t>;   // leerer Inner mit Span sp (1/2/3); darf werfen
        { cs.span(ref) }         -> std::convertible_to<unsigned>;      // 1/2/3 (Diskriminator-Byte-Breite)
        { cs.prefix_of(ref) }    -> std::same_as<typename S::prefix_type>;
        { s.set_prefix(ref, p) } -> std::same_as<void>;
        { s.prefix_cut(ref, n) } -> std::same_as<void>;
        { cs.find_child(ref, disc) }     -> std::convertible_to<std::size_t>;  // Kind-Ref oder kNil (span-breiter Disk)
        { s.add_child(ref, disc, ref) }  -> std::same_as<void>;        // (disc nicht vorhanden); darf werfen
        { s.set_child(ref, disc, ref) }  -> std::same_as<void>;        // bestehenden Kind-Slot ersetzen
        { s.remove_child(ref, disc) }    -> std::same_as<void>;
        { s.free_node(ref) }     -> std::same_as<void>;
    };

}  // namespace comdare::cache_engine::lookup::composable
