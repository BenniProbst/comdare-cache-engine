#pragma once
// V41 Roadmap-2 (Doku 14 §1.2, Doku 24 §6) — InterpolationTraversalOrgan.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Drittes Traversal-Organ auf dem FLACHEN, sortierten StorageOrgan (neben LinearScan/SortedBinary):
// Interpolationssuche (Perl/Itai/Avni, CACM 1978) — schaetzt die Position aus der Schluessel-Verteilung.
// insert_into/erase_from halten die SORTIER-Invariante (delegieren an SortedBinaryTraversal); nur die
// Lokalisierung in lookup_in unterscheidet sich → reine Organ-Austauschbarkeit ("genetisches Experiment").
//
// **uint64-Generalisierung (MSVC-portabel):** Die Original-Interpolation castet zu `unsigned` (uint16-sicher)
// und ueberlaeuft bei uint64. MSVC kennt KEIN `unsigned __int128`; daher wird der Interpolations-Anteil als
// `double`-Bruch berechnet und auf [lo,hi] geklemmt. Der Praezisionsverlust betrifft NUR die Probe-Schaetzung —
// die Korrektheit garantiert die umgebende, stets konvergierende lo/hi-Schleife (wie bei Binaersuche).

#include "composable_search.hpp"   // StorageOrgan-API + SortedBinaryTraversal (insert/erase-Delegation)

#include <cstddef>
#include <optional>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable {

/// Traversal-Organ: Interpolationssuche auf sortiertem Storage-Organ. KEIN Eigenspeicher.
struct InterpolationTraversalOrgan {
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
        std::size_t lo = 0, hi = n - 1;
        while (lo <= hi) {
            typename Store::key_type const klo = s.key_at(lo);
            typename Store::key_type const khi = s.key_at(hi);
            if (k < klo || k > khi) break;          // ausserhalb des sortierten Fensters → nicht vorhanden
            std::size_t pos;
            if (khi == klo) {
                pos = lo;                            // alle Schluessel im Fenster gleich
            } else {
                // double-Bruch statt 128-bit-Produkt (MSVC-portabel); Korrektheit via lo/hi-Schleife.
                double const frac = static_cast<double>(k - klo) / static_cast<double>(khi - klo);
                pos = lo + static_cast<std::size_t>(frac * static_cast<double>(hi - lo));
                if (pos < lo) pos = lo;
                if (pos > hi) pos = hi;
            }
            typename Store::key_type const kp = s.key_at(pos);
            if (kp == k) return s.value_at(pos);
            if (kp < k) { if (pos == hi) break; lo = pos + 1; }
            else        { if (pos == lo) break; hi = pos - 1; }
        }
        return std::nullopt;
    }
};

// Selbstbeweis: erfuellt das TraversalOrgan-Concept ueber dem Pilot-Storage-Organ.
static_assert(TraversalOrgan<InterpolationTraversalOrgan, RawSlotStore>);

}  // namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable
