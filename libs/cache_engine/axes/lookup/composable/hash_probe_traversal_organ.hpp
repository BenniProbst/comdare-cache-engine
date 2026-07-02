#pragma once
// V41 Umstufung-A (Task #41) — HashProbeTraversal-Concept + HashProbeTraversalOrgan.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Analog zum TreeTraversalOrgan, aber ueber einem HashBucketPool (Open-Addressing-Slots statt Kind-Zeiger):
// statische insert_into/lookup_in/erase_from, KEIN Eigenspeicher. HashProbeTraversalOrgan = Linear Probing
// mit multiplikativem (Fibonacci-)Hashing (Knuth TAOCP 3 §6.4), portiert aus axis_03a_search_algo_hash_search.hpp
// (Z.74-137), aber als austauschbares ORGAN ueber dem generischen uint64-Pool (statt monolithischem uint16-
// Wrapper). Die Such-LOGIK (Fibonacci-Index, Probe-Loop, Tombstone-Reuse, Load-Trigger) gehoert ins Organ;
// die Slot-VERWALTUNG (place_occupied/mark_deleted/rehash) in den Pool.
//
// KORREKTES erase ([[algorithm-correctness-when-named]]): Tombstone via p.mark_deleted (NICHT Leeren — das
// wuerde die Probe-Kette nachfolgender Schluessel brechen). [[no-runtime-switch]]: rein statische Templates.

#include "hash_bucket_pool_concept.hpp"
#include "hash_bucket_pool_store.hpp" // fuer den Selbstbeweis am Dateiende

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

/// HASH-PROBE-Organ-Concept: statische insert_into/lookup_in/erase_from auf einem HashBucketPool.
template <class T, class Pool>
concept HashProbeTraversal =
    HashBucketPool<Pool> && requires(Pool& p, Pool const& cp, typename Pool::key_type k, typename Pool::value_type v) {
        { T::template insert_into<Pool>(p, k, v) } -> std::same_as<void>;
        { T::template lookup_in<Pool>(cp, k) } -> std::same_as<std::optional<typename Pool::value_type>>;
        { T::template erase_from<Pool>(p, k) } -> std::same_as<bool>;
    };

/// Hash-Traversal-Organ: Open-Addressing Linear Probing + Fibonacci-Hash. Navigiert ueber Pool-Slot-API.
struct HashProbeTraversalOrgan {
    /// Multiplikative Hash-Konstante 2^64/phi (Knuth) — die Such-Logik (Index-Berechnung) ist Organ-Sache.
    static constexpr std::uint64_t kFibonacciMul = 11400714819323198485ULL;
    static constexpr std::size_t   kNpos         = static_cast<std::size_t>(-1);

    template <class Pool>
    [[nodiscard]] static std::size_t index_of(typename Pool::key_type k, std::size_t cap) noexcept {
        return static_cast<std::size_t>(static_cast<std::uint64_t>(k) * kFibonacciMul) & (cap - 1);
    }

    template <class Pool>
    static void insert_into(Pool& p, typename Pool::key_type k, typename Pool::value_type v) {
        // Load-Trigger >= 0.7 (verbatim Monolith Z.75): vor dem Proben ggf. Kapazitaet verdoppeln.
        if ((p.occupied() + p.tombstones()) * 10 >= p.bucket_count() * 7) p.rehash(p.bucket_count() * 2);
        std::size_t const cap           = p.bucket_count();
        std::size_t const start         = index_of<Pool>(k, cap);
        std::size_t       first_deleted = kNpos;
        for (std::size_t i = 0; i < cap; ++i) {
            std::size_t const pos = (start + i) & (cap - 1);
            if (p.slot_is_empty(pos)) {
                // Tombstone-Reuse: an erstem gesehenen Deleted-Slot platzieren, sonst am Empty-Slot.
                std::size_t const target = (first_deleted != kNpos) ? first_deleted : pos;
                p.place_occupied(target, k, v); // Store verbucht Tombstone->Occupied + ++occupied
                return;
            }
            if (p.slot_is_deleted(pos)) {
                if (first_deleted == kNpos) first_deleted = pos;
            } else if (p.slot_key(pos) == k) { // Occupied + gleicher Key -> Update
                p.set_slot_value(pos, v);
                return;
            }
        }
    }

    template <class Pool>
    static std::optional<typename Pool::value_type> lookup_in(Pool const& p, typename Pool::key_type k) {
        std::size_t const cap   = p.bucket_count();
        std::size_t const start = index_of<Pool>(k, cap);
        for (std::size_t i = 0; i < cap; ++i) {
            std::size_t const pos = (start + i) & (cap - 1);
            if (p.slot_is_empty(pos)) return std::nullopt; // Kette zu Ende -> Miss
            if (p.slot_is_occupied(pos) && p.slot_key(pos) == k) return p.slot_value(pos);
            // Deleted oder Occupied(anderer Key) -> weiter proben
        }
        return std::nullopt;
    }

    template <class Pool>
    static bool erase_from(Pool& p, typename Pool::key_type k) {
        std::size_t const cap   = p.bucket_count();
        std::size_t const start = index_of<Pool>(k, cap);
        for (std::size_t i = 0; i < cap; ++i) {
            std::size_t const pos = (start + i) & (cap - 1);
            if (p.slot_is_empty(pos)) return false;
            if (p.slot_is_occupied(pos) && p.slot_key(pos) == k) {
                p.mark_deleted(pos); // Tombstone — Probe-Kette bleibt intakt
                return true;
            }
        }
        return false;
    }
};

// Selbstbeweis: HashProbeTraversalOrgan erfuellt das HashProbeTraversal-Concept ueber dem Pilot-Pool.
static_assert(HashProbeTraversal<HashProbeTraversalOrgan, HashBucketPoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
