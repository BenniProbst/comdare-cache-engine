#pragma once
// V41 Roadmap-2 INC-2b (Doku 14 §1.2/§11.3, Doku 24 §6) — TreeNodePool-Concept.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// **Warum ein EIGENES Concept (nicht StorageOrgan):** Das flache StorageOrgan (storage_organ_concept.hpp)
// bietet pro Slot nur key_at/value_at (2 uint64) und verschiebt Slots bei insert_slot_at/erase_slot_at
// (Index-INSTABILITAET). Baum-Algorithmen (BST, B-Baum) brauchen aber index-STABILE Knoten mit Kind-Zeigern
// (left/right). Daher fuehrt INC-2b ein SEPARATES, index-stabiles Pool-Concept ein — die bestehenden
// StorageOrgan/TraversalOrgan-Vertraege + ihre static_asserts bleiben voellig unberuehrt.
//
// Geist gewahrt (wie StorageOrgan): gemeinsamer breiter uint64-Key; KEIN noexcept-Zwang (allocate_node
// darf allokieren/werfen); reines Substrat OHNE Such-Logik (die lebt im Tree-Traversal-Organ).

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::lookup::composable {

/// TREE-NODE-POOL-Concept: index-stabiler Knoten-Pool mit Kind-Zeigern (left/right) ueber uint64-Key.
/// Erfuellt von TreeNodePoolStore; konsumiert von Tree-Traversal-Organen (z.B. BSTTraversalOrgan).
template <class S>
concept TreeNodePool =
    requires {
        typename S::key_type;
        typename S::value_type;
        { S::kNil } -> std::convertible_to<std::size_t>; // Sentinel-Index ("kein Knoten")
    } && std::same_as<typename S::key_type, std::uint64_t> && std::same_as<typename S::value_type, std::uint64_t> &&
    requires(S& s, S const& cs, std::size_t i, typename S::key_type k, typename S::value_type v) {
        // (A) const Inspektion — PFLICHT (auf `cs`, erzwingt const-Correctness)
        { cs.root() } -> std::convertible_to<std::size_t>;
        { cs.node_count() } -> std::convertible_to<std::size_t>;
        { cs.node_key(i) } -> std::same_as<typename S::key_type>;
        { cs.node_value(i) } -> std::same_as<typename S::value_type>;
        { cs.left(i) } -> std::convertible_to<std::size_t>;
        { cs.right(i) } -> std::convertible_to<std::size_t>;
        // (B) Mutation — PFLICHT (auf `s`)
        { s.allocate_node(k, v) } -> std::convertible_to<std::size_t>; // darf werfen (kein noexcept)
        { s.free_node(i) } -> std::same_as<void>;
        { s.set_node_key(i, k) } -> std::same_as<void>; // fuer Hibbard-Nachfolger-Kopie
        { s.set_node_value(i, v) } -> std::same_as<void>;
        { s.set_left(i, i) } -> std::same_as<void>;
        { s.set_right(i, i) } -> std::same_as<void>;
        { s.set_root(i) } -> std::same_as<void>;
        { s.clear() } -> std::same_as<void>;
    };

} // namespace comdare::cache_engine::lookup::composable
