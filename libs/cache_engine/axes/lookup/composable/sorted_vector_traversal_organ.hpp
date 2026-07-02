#pragma once
// #188-4c-ii (2026-07-02) -- SortedVectorTraversal fuer VectorU8U8/VectorU16U16.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Traversal-Organ fuer die sortierten Vektor-Flach-Wrapper. Die Wrapper speichern parallele, sortierte Key-/Value-
// Vektoren und suchen mit std::lower_bound. Der store-traversierbare Pfad fuehrt dieselbe Semantik ueber den
// kompaktierten sortierten Store: lower_bound fuer lookup, sortierte Insert-/Erase-Invariante und geordneter Scan
// identisch zur bewaehrten SortedBinaryTraversal-Mechanik. Der eigene Typ erhaelt die Achsen-Identitaet.

#include "composable_search.hpp" // StorageOrgan-API + SortedBinaryTraversal (lower_bound/insert/erase/scan)

#include <cstddef>
#include <optional>
#include <utility>

namespace comdare::cache_engine::lookup::composable {

/// Sorted-vector Traversal-Organ: lower_bound ueber den sortierten Store. KEIN Eigenspeicher.
struct SortedVectorTraversal {
    template <class Store>
    static std::size_t lower_bound_index(Store const& s, typename Store::key_type k) {
        return SortedBinaryTraversal::template lower_bound_index<Store>(s, k);
    }

    template <class Store>
    static void insert_into(Store& s, typename Store::key_type k, typename Store::value_type v) {
        SortedBinaryTraversal::template insert_into<Store>(s, k, v);
    }

    template <class Store>
    static std::optional<typename Store::value_type> lookup_in(Store const& s, typename Store::key_type k) {
        std::size_t const i = lower_bound_index(s, k);
        if (i < s.slot_count() && s.key_at(i) == k) return s.value_at(i);
        return std::nullopt;
    }

    template <class Store>
    static bool erase_from(Store& s, typename Store::key_type k) {
        return SortedBinaryTraversal::template erase_from<Store>(s, k);
    }

    /// GoF-Iterator (YCSB-E #214): lower_bound + sequenzieller Walk, identisch zum SortedBinary-Storepfad.
    template <class Store, class Sink>
    static std::size_t scan_into(Store const& s, typename Store::key_type start_key, std::size_t max_count,
                                 Sink&& sink) {
        return SortedBinaryTraversal::template scan_into<Store>(s, start_key, max_count, std::forward<Sink>(sink));
    }
};

// Selbstbeweis: TraversalOrgan + additiver Scan-Vertrag ueber dem Pilot-Storage-Organ.
static_assert(TraversalOrgan<SortedVectorTraversal, RawSlotStore>);
static_assert(ScannableTraversalOrgan<SortedVectorTraversal, RawSlotStore>);

} // namespace comdare::cache_engine::lookup::composable
