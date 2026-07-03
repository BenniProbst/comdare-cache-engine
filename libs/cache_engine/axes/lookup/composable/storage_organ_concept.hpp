#pragma once
// V41 Saeule-1 (Doku 24 §5.4/§5.5, Doku 14 §3/§11.3) — StorageOrgan-Concept.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// **Zweck:** Formalisiert die heute IMPLIZITE Duck-Typing-Oberflaeche der Pilot-Klasse
// RawSlotStore (composable_search.hpp:34-50) zu einem EXPLIZITEN Vertrag ueber der aktuellen
// Default-Breite (uint64). Es wird KEINE neue Abstraktion erfunden — der Vertrag wird aus der
// bestehenden Ist-Implementierung extrahiert (Selbstbeweis via static_assert in
// composable_search.hpp). Damit kann ein node_type/layout/allocator-getriebenes Speicher-Substrat
// (z.B. NodeTypeSlotStore) als austauschbares Storage-Organ unter dasselbe Concept gestellt werden.
//
// **Trennung (bewusst):** statistics()/snapshot_t sind NICHT Teil dieses Concepts. Die per-Achsen-
// Observable-Anbindung (Doku 24 §2.2, observe_all) ist ein Folge-Increment — sie wird separat als
// ObservableAxis erfuellt, NICHT in den Storage-Organ-Kernvertrag gezwungen (sonst muessten alle
// heutigen Stores um snapshot_t erweitert werden, was den minimalen Increment sprengt).
//
// **const-Correctness ist HART:** slot_count/key_at/value_at muessen auf `S const&` aufrufbar sein,
// weil LinearScanTraversal::lookup_in / SortedBinaryTraversal::lookup_in einen const Store& erhalten.

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::lookup::composable {

/// STORAGE-Organ-Concept: index-basiertes Slot-Substrat ueber aktueller Default-Breite (uint64).
/// Erfuellt von RawSlotStore (Pilot) UND node_type/layout/allocator-getriebenen Stores.
template <class S>
concept StorageOrgan =
    requires {
        typename S::key_type;
        typename S::value_type;
    } && std::same_as<typename S::key_type, std::uint64_t> // #217-2b: aktuelle Default-Breite; spaeter unsigned_integral
    && std::same_as<typename S::value_type, std::uint64_t> // Achsen-Typen bestimmen NICHT den Container-Key
    && requires(S& s, S const& cs, std::size_t i, typename S::key_type k, typename S::value_type v) {
           // (A) const Inspektion — PFLICHT (auf `cs`, erzwingt const-Correctness strukturell)
           { cs.slot_count() } -> std::convertible_to<std::size_t>;
           { cs.key_at(i) } -> std::same_as<typename S::key_type>;
           { cs.value_at(i) } -> std::same_as<typename S::value_type>;
           // (B) Mutation — PFLICHT (auf `s`)
           { s.set_value_at(i, v) } -> std::same_as<void>;
           { s.append_slot(k, v) } -> std::same_as<void>;       // LinearScan (O(1) amortisiert)
           { s.insert_slot_at(i, k, v) } -> std::same_as<void>; // SortedBinary (O(n) Verschiebung)
           { s.erase_slot_at(i) } -> std::same_as<void>;
           { s.clear() } -> std::same_as<void>;
       };
// Bewusst KEIN `noexcept` im Vertrag: append_slot/insert_slot_at/erase_slot_at duerfen allokieren
// (RawSlotStore via std::vector) bzw. bei Kapazitaets-Ueberlauf werfen (NodeTypeSlotStore) — ein
// noexcept-Constraint wuerde diese gueltigen Implementierungen faelschlich ausschliessen.

} // namespace comdare::cache_engine::lookup::composable
