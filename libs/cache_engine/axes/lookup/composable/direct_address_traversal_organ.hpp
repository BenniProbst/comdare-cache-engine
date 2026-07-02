#pragma once
// #188-4c-ii (2026-07-02) -- DirectAddressTraversal fuer markerlose Flach-Wrapper.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Traversal-Organ fuer die direktadressierten Flach-Wrapper Array256SearchAlgo und Array65535SearchAlgo.
// Die Original-Wrapper adressieren lueckenlos direkt (`data_[k]` / `present_[k]`) ueber ihre u8/u16-Domaene.
// Im Store-traversierbaren Pfad liegt dagegen ein kompaktierter, sortierter uint64-Store vor. Dieses Organ bildet
// die Direktadress-Charakteristik deshalb ehrlich als Direktadress-SCHAETZUNG ab: O(1)-Sprung auf
// `key - store.key_at(0)`, geklemmt auf den belegten Store-Bereich, danach lokale Korrektur zum exakten Key.
// Kein synthetischer Zusatzspeicher; insert/erase/scan halten die sortierte Store-Repraesentation via
// SortedBinaryTraversal.

#include "composable_search.hpp" // StorageOrgan-API + SortedBinaryTraversal (insert/erase/scan-Delegation)

#include <cstddef>
#include <optional>
#include <utility>

namespace comdare::cache_engine::lookup::composable {

/// Direktadress-SCHAETZUNG ueber kompaktiertem sortierten Store: O(1)-Sprung + lokale Korrektur.
/// Die Original-Wrapper adressieren lueckenlos direkt; dieser stateless Organ-Pfad bleibt store-faithful ohne
/// kuenstlichen 256/65536-Slot-Spiegel.
struct DirectAddressTraversal {
    template <class Store>
    static void insert_into(Store& s, typename Store::key_type k, typename Store::value_type v) {
        SortedBinaryTraversal::template insert_into<Store>(s, k, v); // kompaktierter Store bleibt sortiert
    }

    template <class Store>
    static bool erase_from(Store& s, typename Store::key_type k) {
        return SortedBinaryTraversal::template erase_from<Store>(s, k);
    }

    template <class Store>
    static std::optional<typename Store::value_type> lookup_in(Store const& s, typename Store::key_type k) {
        std::size_t const n = s.slot_count();
        if (n == 0) return std::nullopt;

        typename Store::key_type const first = s.key_at(0);
        if (k < first) return std::nullopt;

        // Review-F4 (#188-4c-ii): Vergleich in u64 statt key_type — bei der u64-Organ-Invariante identisch,
        // aber robust gegen kuenftige schmalere key_types (kein Truncation-Wrap von n). Setzt sortierte
        // UNIQUE Keys voraus (insert_or_assign-Semantik der SortedBinary-Delegation).
        std::uint64_t const offset = static_cast<std::uint64_t>(k) - static_cast<std::uint64_t>(first);
        std::size_t const direct =
            (offset >= static_cast<std::uint64_t>(n)) ? (n - 1u) : static_cast<std::size_t>(offset);

        typename Store::key_type const kd = s.key_at(direct);
        if (kd == k) return s.value_at(direct);

        // Lokale Korrektur auf dem sortierten kompakten Store. Linear ist absichtlich einfach und korrekt:
        // Luecken im Keyraum koennen die Direktadress-Schaetzung nach rechts schieben; bei k > letztem Key nach links
        // geklemmte Treffer pruefen wir nach rechts bis zum ersten >= k.
        if (kd < k) {
            for (std::size_t i = direct + 1u; i < n; ++i) {
                typename Store::key_type const ki = s.key_at(i);
                if (ki == k) return s.value_at(i);
                if (ki > k) break;
            }
            return std::nullopt;
        }

        std::size_t i = direct;
        while (i > 0u && s.key_at(i - 1u) >= k) --i;
        if (s.key_at(i) == k) return s.value_at(i);
        return std::nullopt;
    }

    /// GoF-Iterator (YCSB-E #214): Store ist sortiert (insert/erase delegieren an SortedBinary) -> geordneter
    /// Range-Scan delegiert an SortedBinaryTraversal::scan_into (lower_bound + Walk).
    template <class Store, class Sink>
    static std::size_t scan_into(Store const& s, typename Store::key_type start_key, std::size_t max_count,
                                 Sink&& sink) {
        return SortedBinaryTraversal::template scan_into<Store>(s, start_key, max_count, std::forward<Sink>(sink));
    }
};

// Selbstbeweis: TraversalOrgan + additiver Scan-Vertrag ueber dem Pilot-Storage-Organ.
static_assert(TraversalOrgan<DirectAddressTraversal, RawSlotStore>);
static_assert(ScannableTraversalOrgan<DirectAddressTraversal, RawSlotStore>);

} // namespace comdare::cache_engine::lookup::composable
