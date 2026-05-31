#pragma once
// V41 Roadmap-2 (Doku 14 §1.2, Doku 24 §6) — GallopingTraversalOrgan.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Viertes Traversal-Organ auf dem FLACHEN, sortierten StorageOrgan: Galloping-/Exponential-Suche
// (Bentley/Yao 1976) — exponentielle Spruenge (1,2,4,8,…) bis ein oberer Bound >= Schluessel liegt,
// dann binaere Verfeinerung im geklammerten Fenster. O(log(Zielindex)) — schnell bei Treffern nahe dem
// Anfang. insert_into/erase_from halten die SORTIER-Invariante (delegieren an SortedBinaryTraversal);
// nur die Lokalisierung in lookup_in unterscheidet sich → reine Organ-Austauschbarkeit.

#include "composable_search.hpp"   // StorageOrgan-API + SortedBinaryTraversal (insert/erase-Delegation)

#include <cstddef>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

/// Traversal-Organ: Galloping-/Exponential-Suche auf sortiertem Storage-Organ. KEIN Eigenspeicher.
struct GallopingTraversalOrgan {
    template <class Store>
    static void insert_into(Store& s, typename Store::key_type k, typename Store::value_type v) {
        SortedBinaryTraversal::template insert_into<Store>(s, k, v);   // Sortier-Invariante identisch
    }
    template <class Store>
    static bool erase_from(Store& s, typename Store::key_type k) {
        return SortedBinaryTraversal::template erase_from<Store>(s, k);
    }
    template <class Store>
    static std::optional<typename Store::value_type> lookup_in(Store const& s, typename Store::key_type k) {
        std::size_t const n = s.slot_count();
        if (n == 0) return std::nullopt;
        // Exponentielles Galoppieren: lo = letzte Position mit key < k, bound = erstes 2^i jenseits davon.
        std::size_t lo = 0;
        std::size_t bound = 1;
        while (bound < n && s.key_at(bound) < k) { lo = bound; bound *= 2; }
        std::size_t hi = (bound < n) ? bound : (n - 1);
        // Binaere Verfeinerung im inklusiven Fenster [lo, hi].
        while (lo <= hi) {
            std::size_t const m = lo + (hi - lo) / 2;
            typename Store::key_type const km = s.key_at(m);
            if (km == k) return s.value_at(m);
            if (km < k) { if (m == hi) break; lo = m + 1; }
            else        { if (m == 0)  break; hi = m - 1; }
        }
        return std::nullopt;
    }
};

// Selbstbeweis: erfuellt das TraversalOrgan-Concept ueber dem Pilot-Storage-Organ.
static_assert(TraversalOrgan<GallopingTraversalOrgan, RawSlotStore>);

}  // namespace comdare::cache_engine::lookup::composable
