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
// Substrat OHNE Such-Logik: der Pool verwaltet nur Slots (Empty/Occupied/Deleted) + rehash; die Probe-/
// Hash-Logik (Fibonacci-Index, Linear Probing, Tombstone-Reuse) lebt im HashProbeTraversalOrgan.
// 3 State-Praedikate (slot_is_empty/occupied/deleted) statt SlotState-Enum halten das Concept POD-frei.
// Geist gewahrt (wie StorageOrgan/TreeNodePool): gemeinsamer breiter uint64-Key; KEIN noexcept-Zwang
// (rehash darf via vector allokieren/werfen, [[allocation-failure-exception]]).

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable {

/// HASH-BUCKET-POOL-Concept: Open-Addressing-Slot-Array mit Tombstones + Rehash ueber uint64-Key.
/// Erfuellt von HashBucketPoolStore; konsumiert von HashProbeTraversalOrgan.
template <class S>
concept HashBucketPool =
    requires {
        typename S::key_type;
        typename S::value_type;
    }
    && std::same_as<typename S::key_type, std::uint64_t>
    && std::same_as<typename S::value_type, std::uint64_t>
    && requires(S& s, S const& cs, std::size_t i,
                typename S::key_type k, typename S::value_type v) {
        // (A) const Inspektion — PFLICHT (auf `cs`, erzwingt const-Correctness)
        { cs.bucket_count() }    -> std::convertible_to<std::size_t>;   // Power-of-2 (= mask_+1)
        { cs.slot_is_empty(i) }    -> std::same_as<bool>;               // Kettenende (Probe-Stop)
        { cs.slot_is_occupied(i) } -> std::same_as<bool>;               // belegt
        { cs.slot_is_deleted(i) }  -> std::same_as<bool>;               // Tombstone (weiter proben + Reuse-Kandidat)
        { cs.slot_key(i) }       -> std::same_as<typename S::key_type>;
        { cs.slot_value(i) }     -> std::same_as<typename S::value_type>;
        { cs.occupied() }        -> std::convertible_to<std::size_t>;   // LIVE-Count (= occupied_count)
        { cs.tombstones() }      -> std::convertible_to<std::size_t>;
        // (B) Mutation — PFLICHT (auf `s`)
        { s.place_occupied(i, k, v) } -> std::same_as<void>;   // setzt Slot Occupied (Tombstone->Occupied wird verbucht)
        { s.set_slot_value(i, v) }    -> std::same_as<void>;   // Update eines belegten Slots
        { s.mark_deleted(i) }         -> std::same_as<void>;   // Tombstone (Probe-Kette bleibt intakt)
        { s.rehash(i) }               -> std::same_as<void>;   // Resize + Re-Distribution; darf werfen
        { s.clear() }                 -> std::same_as<void>;
    };

}  // namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable
