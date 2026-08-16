#pragma once
// V41 Umstufung-A (Task #41) — SkipListNodePool-Concept.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// **Warum ein EIGENES Concept (nicht TreeNodePool):** Eine Skip-Liste hat KEINE binaeren left/right-
// Kind-Zeiger, sondern eine VARIABLE Anzahl Forward-Indizes je Knoten (1..kMaxLevel, per Muenzwurf gezogen)
// + eine listenweite Hoehe `level_`. Die Level-Ziehung (random_level) ist RNG-mutierend und gehoert ins
// Substrat (Pool-Wachstum), NICHT ins stateless Organ. Daher ein SEPARATES Pool-Concept — TreeNodePool +
// seine static_asserts bleiben unberuehrt.
//
// Substrat OHNE Such-Logik: der Pool verwaltet Knoten + Forward-Verkettung + Listen-Hoehe + RNG; die
// Multi-Level-Walk-/Verkettungs-Navigation lebt im SkipListTraversalOrgan. Geist gewahrt: gemeinsamer
// uint64-Key; KEIN noexcept-Zwang (allocate_node darf via vector werfen, [[allocation-failure-exception]]).

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::lookup::composable {

/// SKIP-LIST-NODE-POOL-Concept: verzeigerte geordnete Struktur mit variabler Forward-Index-Laenge je
/// Knoten + Listen-Hoehe + RNG-Level-Ziehung. Erfuellt von SkipListNodePoolStore; konsumiert vom Organ.
template <class S>
concept SkipListNodePool =
    requires {
        typename S::key_type;
        typename S::value_type;
        { S::kNil } -> std::convertible_to<std::size_t>;  // "kein Nachfolger"
        { S::kHead } -> std::convertible_to<std::size_t>; // Sentinel-Kopf-Index
        { S::kMaxLevel } -> std::convertible_to<int>;
    } && std::same_as<typename S::key_type, std::uint64_t> && std::same_as<typename S::value_type, std::uint64_t> &&
    requires(S& s, S const& cs, std::size_t i, int lvl, bool b, typename S::key_type k, typename S::value_type v) {
        // (A) const Inspektion — PFLICHT
        { cs.head() } -> std::convertible_to<std::size_t>;
        { cs.list_level() } -> std::convertible_to<int>; // listenweite Hoehe
        { cs.live_count() } -> std::convertible_to<std::size_t>;
        { cs.node_key(i) } -> std::same_as<typename S::key_type>;
        { cs.node_value(i) } -> std::same_as<typename S::value_type>;
        { cs.node_live(i) } -> std::same_as<bool>;
        { cs.forward_at(i, i) } -> std::convertible_to<std::size_t>; // (node, level) -> Nachfolger-Index
        // (B) Mutation — PFLICHT
        { s.allocate_node(k, v, lvl) } -> std::convertible_to<std::size_t>; // mit lvl Forward-Slots; darf werfen
        { s.draw_level() } -> std::convertible_to<int>;                     // RNG-Muenzwurf — Pool-Verantwortung
        { s.set_forward_at(i, i, i) } -> std::same_as<void>;                // (node, level, target)
        { s.set_node_value(i, v) } -> std::same_as<void>;
        { s.set_node_live(i, b) } -> std::same_as<void>;
        { s.set_list_level(lvl) } -> std::same_as<void>;
        { s.dec_live() } -> std::same_as<void>;
        { s.clear() } -> std::same_as<void>;
    };

} // namespace comdare::cache_engine::lookup::composable
