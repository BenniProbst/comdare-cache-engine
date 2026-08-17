#pragma once
// #188-4a (2026-07-02) -- EytzingerLayoutPool-Concept.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Layout-Familie fuer Eytzinger: kein Tree-/Hash-Pool und kein FLAT-Store-Traversal, sondern ein eigener Store,
// der den sortierten Primaerzustand und den abgeleiteten 1-indexed BFS-Puffer in EINEM Apparat haelt.
// Suchlogik lebt im EytzingerTraversalOrgan; das Substrat invalidiert seinen BFS-Puffer bei jeder Mutation selbst.

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

/// Eytzinger-LAYOUT-POOL-Concept: sortierter Slot-Store plus lazy Eytzinger-BFS-Puffer ueber uint64-Key.
template <class P>
concept EytzingerLayoutPool =
    requires {
        typename P::key_type;
        typename P::value_type;
    } && std::same_as<typename P::key_type, std::uint64_t> && std::same_as<typename P::value_type, std::uint64_t> &&
    requires(P& p, P const& cp, std::size_t i, typename P::key_type k, typename P::value_type v) {
        // (A) sortierter Primaerzustand -- Slot-API wie RawSlotStore.
        { cp.slot_count() } -> std::convertible_to<std::size_t>;
        { cp.key_at(i) } -> std::same_as<typename P::key_type>;
        { cp.value_at(i) } -> std::same_as<typename P::value_type>;
        { p.set_value_at(i, v) } -> std::same_as<void>;
        { p.insert_slot_at(i, k, v) } -> std::same_as<void>;
        { p.erase_slot_at(i) } -> std::same_as<void>;
        { p.append_slot(k, v) } -> std::same_as<void>;
        { p.clear() } -> std::same_as<void>;
        // (B) abgeleiteter BFS-Zustand -- lazy rebuild durch lookup.
        { cp.rebuild_if_dirty() } -> std::same_as<void>;
        { cp.eyt_key_at(i) } -> std::same_as<typename P::key_type>;
        { cp.eyt_value_at(i) } -> std::same_as<typename P::value_type>;
        { cp.dirty() } -> std::same_as<bool>;
    };

/// Eytzinger-TRAVERSAL-Organ-Concept: statische insert_into/lookup_in/erase_from auf einem EytzingerLayoutPool.
template <class T, class P>
concept EytzingerTraversalOrganConcept =
    EytzingerLayoutPool<P> && requires(P& p, P const& cp, typename P::key_type k, typename P::value_type v) {
        { T::template insert_into<P>(p, k, v) } -> std::same_as<void>;
        { T::template lookup_in<P>(cp, k) } -> std::same_as<std::optional<typename P::value_type>>;
        { T::template erase_from<P>(p, k) } -> std::same_as<bool>;
    };

} // namespace comdare::cache_engine::lookup::composable
