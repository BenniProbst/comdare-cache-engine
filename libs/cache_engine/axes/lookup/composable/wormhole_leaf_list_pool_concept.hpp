#pragma once
// V41 Umstufung-A s4 (Task #43, Doku 14 §13) — WormholeLeafListPool-Concept.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// **Warum ein EIGENES Concept (nicht BTreeNodePool, nicht HotPatriciaNodePool):** Wormhole (Wu/Ni/Jiang
// EuroSys 2019) ist ein HYBRID-ordered-index = sortierte DOPPELT-VERKETTETE B+-Leaf-Liste (Daten-Ebene) +
// Hash-Index ueber Praefix-Anker (Routing-Ebene). Distinkt von B-Baum (Wurzel-Abstieg, KEINE prev/next),
// ART (byte-Trie) und HOT (bit-Patricia): hier Hash-JUMP auf den laengsten Anker statt Wurzel-Abstieg.
// Daher ein SEPARATES Pool-Concept: Leaf-Liste (prev/next/anchor + sortierter KV-Block) + Anchor->Leaf-Index.
// is_original=false ([[pseudocode-papers-fallback]]; wh.c=GPL-3.0, KEIN extern-C-Linking — reine C++23-Re-Impl
// aus dem Verstaendnis).
//
// Substrat OHNE Such-Logik: der Pool verwaltet Leaf-Slots + die logische Schluesselzahl + den geordneten
// Anchor-Index. Der Hash-Jump-Descent + Leaf-Split/Merge + Re-Anchor lebt im WormholeJumpTraversalOrgan.
// Gemeinsamer uint64-Key (F15). KEIN noexcept-Zwang fuer new_leaf ([[allocation-failure-exception]]).
//
// **B-Netz (Korrektheit ⊥ Index):** index_lookup_le DARF kNil liefern (Index sprachlos -> Listen-Fallback);
// die Anchor-Re-Justierung im Organ macht jeden stale/falschen Index-Treffer harmlos. Der Hash-Jump ist NIE
// korrektheitstragend, sondern verschiebt nur den Startpunkt.

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::lookup::composable {

/// WORMHOLE-LEAF-LIST-POOL-Concept: sortierte doppelt-verkettete Leaf-Liste + geordneter Anchor->Leaf-Index.
/// Erfuellt von WormholeLeafListPoolStore; konsumiert von WormholeJumpTraversalOrgan.
template <class S>
concept WormholeLeafListPool =
    requires {
        typename S::key_type;
        typename S::value_type;
        { S::kNil }   -> std::convertible_to<std::size_t>;
        { S::kWhKpn } -> std::convertible_to<int>;   // Keys/Leaf (WH_KPN-Analog)
        { S::kWhMid } -> std::convertible_to<int>;   // Split-Punkt (WH_MID-Analog)
        { S::kWhMrg } -> std::convertible_to<int>;   // Merge/Borrow-Schwelle (WH_KPN_MRG-Analog)
    }
    && std::same_as<typename S::key_type,   std::uint64_t>
    && std::same_as<typename S::value_type, std::uint64_t>
    && requires(S& s, S const& cs, std::size_t i, int j,
                typename S::key_type k, typename S::value_type v) {
        // (A) Wurzel (= Listenkopf, linkester Leaf) / Groesse
        { cs.root() }     -> std::convertible_to<std::size_t>;
        { cs.size() }     -> std::convertible_to<std::size_t>;
        { s.set_root(i) } -> std::same_as<void>;
        { s.inc_size() }  -> std::same_as<void>;
        { s.dec_size() }  -> std::same_as<void>;
        { s.clear() }     -> std::same_as<void>;
        // (B) Leaf (sortierter doppelt-verketteter B+-Block)
        { cs.leaf_n(i) }           -> std::convertible_to<int>;
        { cs.leaf_key_at(i, j) }   -> std::same_as<typename S::key_type>;     // AUFSTEIGEND sortiert
        { cs.leaf_value_at(i, j) } -> std::same_as<typename S::value_type>;
        { cs.leaf_anchor(i) }      -> std::same_as<typename S::key_type>;     // = Minimum-Key des Leaf
        { cs.leaf_prev(i) }        -> std::convertible_to<std::size_t>;
        { cs.leaf_next(i) }        -> std::convertible_to<std::size_t>;
        { s.set_leaf_n(i, j) }            -> std::same_as<void>;
        { s.set_leaf_key_at(i, j, k) }    -> std::same_as<void>;
        { s.set_leaf_value_at(i, j, v) }  -> std::same_as<void>;
        { s.set_leaf_anchor(i, k) }       -> std::same_as<void>;
        { s.set_leaf_prev(i, i) }         -> std::same_as<void>;
        { s.set_leaf_next(i, i) }         -> std::same_as<void>;
        { s.new_leaf() }                  -> std::convertible_to<std::size_t>;   // leer; darf werfen
        { s.free_node(i) }                -> std::same_as<void>;
        // (C) Anchor-Hash-Index (Routing-Ebene; geordnet -> Hash-Jump-Semantik mit Linear-Fallback)
        { s.index_insert(k, i) }  -> std::same_as<void>;                          // anchor-key -> leaf-ref
        { s.index_erase(k) }      -> std::same_as<void>;
        { cs.index_lookup_le(k) } -> std::convertible_to<std::size_t>;            // groesster Anchor<=k ODER kNil
        { s.index_clear() }       -> std::same_as<void>;
    };

}  // namespace comdare::cache_engine::lookup::composable
