#pragma once
// V41 Umstufung-A (Task #41, Doku 14 §1-§3 Organ-Metapher, Doku 24 §6/§7) — HashBucketPool-Concept.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// **Warum ein EIGENES Concept (nicht StorageOrgan, nicht TreeNodePool):** StorageOrgan
// (storage_organ_concept.hpp) hat Index-Shift-Semantik (insert_slot_at/erase_slot_at verschieben Slots) —
// das bricht Open-Addressing + Tombstones, wo ein Slot seine Position behalten muss, damit die Probe-Kette
// nachfolgender Schluessel intakt bleibt. TreeNodePool hat left/right-Kind-Zeiger, die ein Hash-Bucket nicht
// hat. Ein Hash-Pool ist zudem NICHT index-stabil ueber rehash (Resize verteilt alle Schluessel neu). Daher
// fuehrt #41 ein SEPARATES Pool-Concept ein — StorageOrgan/TreeNodePool + ihre static_asserts unberuehrt.
//
// Substrat OHNE Such-Logik: der Pool verwaltet nur Slot-/Node-Speicher + rehash; die Probe-/Hash-Logik
// (Fibonacci-Index, Linear Probing, Tombstone-Reuse bzw. Chaining-Walk) lebt im HashProbeTraversalOrgan.
// Open Addressing behaelt die 3 State-Praedikate (slot_is_empty/occupied/deleted), damit Tombstones die
// Probe-Kette intakt halten. Chaining verwendet dieselben slot_key/slot_value-Zugriffe node-indexiert und
// ergaenzt chain_head/node_next/unlink_erase; Tombstones bleiben dort strukturell 0.
// Geist gewahrt (wie StorageOrgan/TreeNodePool): gemeinsamer breiter uint64-Key; KEIN noexcept-Zwang
// (rehash darf via vector allokieren/werfen, [[allocation-failure-exception]]).

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::lookup::composable {

/// Gemeinsamer Kern aller Hash-Bucket-Pools: uint64-Key/Value, Shape-LF-Reexport und Basisstatistik.
template <class S>
concept HashBucketPoolCore =
    requires {
        typename S::key_type;
        typename S::value_type;
        { S::kOpenAddressing } -> std::convertible_to<bool>;
        { S::kLoadNumerator } -> std::convertible_to<int>;
        { S::kLoadDenominator } -> std::convertible_to<int>;
    } && std::same_as<typename S::key_type, std::uint64_t> && std::same_as<typename S::value_type, std::uint64_t> &&
    (S::kLoadNumerator > 0) && (S::kLoadDenominator > 0) && (S::kLoadNumerator <= S::kLoadDenominator) &&
    requires(S& s, S const& cs, std::size_t n) {
        { cs.bucket_count() } -> std::convertible_to<std::size_t>;
        { cs.occupied() } -> std::convertible_to<std::size_t>;
        { cs.tombstones() } -> std::convertible_to<std::size_t>;
        { s.rehash(n) } -> std::same_as<void>;
        { s.clear() } -> std::same_as<void>;
    };

/// Open-Addressing-Slot-Array mit Tombstones + Rehash ueber uint64-Key.
/// Tombstone-Absatz: Deleted-Slots sind keine leeren Slots; sie halten die Probe-Kette am Leben und koennen
/// durch place_occupied wiederverwendet werden. mark_deleted darf daher NICHT zu Empty loeschen.
template <class S>
concept HashBucketPoolOpenAddressing =
    HashBucketPoolCore<S> && (S::kOpenAddressing == true) &&
    requires(S& s, S const& cs, std::size_t i, typename S::key_type k, typename S::value_type v) {
        // (A) const Inspektion — PFLICHT (auf `cs`, erzwingt const-Correctness)
        { cs.slot_is_empty(i) } -> std::same_as<bool>;    // Kettenende (Probe-Stop)
        { cs.slot_is_occupied(i) } -> std::same_as<bool>; // belegt
        { cs.slot_is_deleted(i) } -> std::same_as<bool>;  // Tombstone (weiter proben + Reuse-Kandidat)
        { cs.slot_key(i) } -> std::same_as<typename S::key_type>;
        { cs.slot_value(i) } -> std::same_as<typename S::value_type>;
        // (B) Mutation — PFLICHT (auf `s`)
        { s.place_occupied(i, k, v) } -> std::same_as<void>; // setzt Slot Occupied (Tombstone->Occupied verbucht)
        { s.set_slot_value(i, v) } -> std::same_as<void>;    // Update eines belegten Slots
        { s.mark_deleted(i) } -> std::same_as<void>;         // Tombstone (Probe-Kette bleibt intakt)
    };

/// Separate-Chaining-Pool: Buckets zeigen auf Node-Ketten; Nodes werden ueber eine Freiliste recycelt.
template <class S>
concept HashBucketPoolChaining =
    HashBucketPoolCore<S> && (S::kOpenAddressing == false) &&
    requires(S& s, S const& cs, std::size_t bucket, std::size_t node, std::size_t prev, typename S::key_type k,
             typename S::value_type v) {
        { S::kNil } -> std::convertible_to<std::size_t>;
        { cs.chain_head(bucket) } -> std::convertible_to<std::size_t>;
        { cs.node_next(node) } -> std::convertible_to<std::size_t>;
        { cs.node_slot_count() } -> std::convertible_to<std::size_t>;
        { cs.slot_is_empty(node) } -> std::same_as<bool>;
        { cs.slot_is_occupied(node) } -> std::same_as<bool>;
        { cs.slot_is_deleted(node) } -> std::same_as<bool>;
        { cs.slot_key(node) } -> std::same_as<typename S::key_type>;
        { cs.slot_value(node) } -> std::same_as<typename S::value_type>;
        { s.set_slot_value(node, v) } -> std::same_as<void>;
        { s.allocate_chained(bucket, k, v) } -> std::same_as<void>;
        { s.unlink_erase(bucket, node, prev) } -> std::same_as<void>;
    };

/// HASH-BUCKET-POOL-Concept: Shape-abhaengig Open Addressing ODER Chaining.
template <class S>
concept HashBucketPool = HashBucketPoolOpenAddressing<S> || HashBucketPoolChaining<S>;

} // namespace comdare::cache_engine::lookup::composable
