#pragma once
// E-Welle-A2 (Befund-2 / #188-4a, 2026-06-28) — KAryTraversal: k-Wege-Such-Organ auf flachem sortiertem Store.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Viertes store-traversierbares Traversal-Organ (neben LinearScan/SortedBinary/Interpolation): k-ary search
// (Schlegel/Gemulla/Lehner, DaMoN 2009) — Verallgemeinerung der Binaersuche: pro Iteration K gleichverteilte
// Separatoren -> Partition in K+1 Segmente (ceil(log_(K+1) n) Iterationen). #188-4a macht die k_ary-search_algo-
// Achse STORE-TRAVERSIERBAR (Weg-A): container_ fuehrt sie ueber DENSELBEN node/layout/allocator-getriebenen Store
// (statt SortedBinary-Spiegel ueber search_organ_ / Weg-B). Damit misst die k_ary-Achse ihren ECHTEN Organ-Pfad
// (Meta-Lehre #3), nicht den eines Fremd-Apparats.
//
// **Organ-Disziplin (wie InterpolationTraversalOrgan):** KEIN Eigenspeicher. insert_into/erase_from/scan_into halten
// die SORTIER-Invariante und delegieren an SortedBinaryTraversal (identische sortierte Repraesentation); NUR
// lookup_in unterscheidet sich — die k-Wege-Partition statt 2-Wege-Binaer. Dadurch ist die Korrektheit (std::map-
// Konformitaet) per Konstruktion identisch zu SortedBinaryTraversal (run_conformance_gate-Bar, test_conformance_gate.cpp)
// und es entsteht KEIN neuer O(n)-Rebuild (kein #211-Konflikt; gleiche flatten/rebuild-Kosten wie SortedBinary).
//
// **Arity K = COMPILE-TIME-Template-Parameter (#188-4a-C, User-Entscheid 2026-06-29):** K ist eine COMPILE-TIME-
// Permutation der search_algo-Achse — je K in {2,4,8,16} ein DISTINKTER Organ-Typ KAryTraversal<K> -> eine eigene
// statisch kompilierte Tier-Binary (StaticAxisNode im Permutations-B+-Baum), gesteuert als Aspekt der search_algo-
// Achse durch eine eigene Tier-build-Permutation. KEIN Laufzeit-Kanal (reale k-ary-Impls waehlen K ebenfalls statisch,
// z.B. SIMD-breiten-angepasst). So bleibt die Trennung der Achsen-Kanaele intakt (iterable-Laufzeit-Kanal = nur fuer
// echt-runtime Aspekte wie hash-Kapazitaet/queuing, NICHT k-ary). Default Arity=4 (= KArySearchAlgo::kDefaultArity,
// "5-Wege-Partition", Paper-Beispiel). Das Organ bleibt STATELESS (reine Funktion von (Store, K)); K compile-time fixiert.
//
// **lookup_in = bit-identisch zur erprobten KArySearchAlgo::lookup (axis_03a_search_algo_k_ary.hpp:119-153), nur auf
// der StorageOrgan-API (key_at/value_at statt keys_/values_).** Halb-offenes Intervall [lo,hi); je Iteration bis zu K
// Separatoren; Direkt-Treffer-Abkuerzung; Rest-Segment (Breite <= K) linearer Scan -> exakt lower_bound-Semantik fuer
// vorhandene Keys (sonst nullopt). Keine Allokation, kein throw (reine Lese-Arithmetik).

#include "composable_search.hpp" // StorageOrgan-API + SortedBinaryTraversal (insert/erase/scan-Delegation) + Concepts

#include <cstddef>
#include <optional>
#include <utility>

namespace comdare::cache_engine::lookup::composable {

/// Traversal-Organ: k-Wege-Suche auf sortiertem Storage-Organ. KEIN Eigenspeicher (Organ, nicht Tier).
/// Arity = COMPILE-TIME-Parameter (StaticAxisNode-Permutation); je K ein distinkter Typ -> eigene Tier-Binary.
template <unsigned Arity = 4u>
struct KAryTraversal {
    static_assert(Arity >= 2u, "k-ary Arity muss >= 2 sein (Arity=1 waere lineare Suche)");
    /// Such-METHODE-Arity (Separatoren je Iteration); compile-time fixiert. Partition in K+1 Segmente.
    static constexpr unsigned kArity = Arity;

    template <class Store>
    static void insert_into(Store& s, typename Store::key_type k, typename Store::value_type v) {
        SortedBinaryTraversal::template insert_into<Store>(s, k, v); // Sortier-Invariante identisch
    }
    template <class Store>
    static bool erase_from(Store& s, typename Store::key_type k) {
        return SortedBinaryTraversal::template erase_from<Store>(s, k);
    }
    /// k-ary search (Schlegel DaMoN 2009): pro Iteration bis zu K=Arity gleichverteilte Separatoren -> Partition in
    /// K+1 Segmente; Restbreite <= K -> linearer Scan. Auf dem SORTIERTEN Store bit-identisch zu SortedBinaryTraversal::
    /// lookup_in (Wert iff Key vorhanden, sonst nullopt) = std::map-konform — fuer JEDES compile-time K.
    template <class Store>
    static std::optional<typename Store::value_type> lookup_in(Store const& s, typename Store::key_type k) {
        std::size_t const     n  = s.slot_count();
        std::size_t           lo = 0, hi = n; // halb-offenes Intervall [lo, hi)
        constexpr std::size_t K = Arity;      // #188-4a-C: K = compile-time-Subachse (Arity>=2 per static_assert)
        while (hi - lo > K) {
            std::size_t const width  = hi - lo;
            std::size_t       new_lo = lo, new_hi = hi;
            bool              narrowed = false;
            for (std::size_t j = 1; j <= K; ++j) {
                std::size_t const pos =
                    lo +
                    (width * j) /
                        (K +
                         1); // Separator in (lo,hi); Formel identisch zur Referenz; width<=slot_count -> kein realer Overflow
                typename Store::key_type const sep = s.key_at(pos);
                if (sep == k) return s.value_at(pos); // Direkt-Treffer auf Separator
                if (k < sep) {
                    new_hi   = pos;
                    narrowed = true;
                    break;
                } // Ziel im Segment vor pos
                new_lo = pos + 1; // Ziel hinter diesem Separator
            }
            lo = new_lo;
            if (narrowed) hi = new_hi; // sonst: k > alle Separatoren -> [letzter+1, hi)
        }
        for (std::size_t i = lo; i < hi; ++i) // Rest-Segment (Breite <= K): linearer Scan
            if (s.key_at(i) == k) return s.value_at(i);
        return std::nullopt;
    }
    /// GoF-Iterator (YCSB-E #214): Store sortiert (insert/erase delegieren an SortedBinary) -> geordneter Range-Scan
    /// delegiert ebenfalls an SortedBinaryTraversal::scan_into (lower_bound + Walk, O(log n + scan_len)).
    template <class Store, class Sink>
    static std::size_t scan_into(Store const& s, typename Store::key_type start_key, std::size_t max_count,
                                 Sink&& sink) {
        return SortedBinaryTraversal::template scan_into<Store>(s, start_key, max_count, std::forward<Sink>(sink));
    }
};

// Selbstbeweis: KAryTraversal<4> (Default-Arity) erfuellt TraversalOrgan + den additiven Scan-Vertrag (#214) ueber
// dem Pilot-Storage-Organ. Je weitere compile-time Arity (2/8/16) wird im test_conformance_gate gegen std::map geprueft.
static_assert(TraversalOrgan<KAryTraversal<4u>, RawSlotStore>);
static_assert(ScannableTraversalOrgan<KAryTraversal<4u>, RawSlotStore>);

} // namespace comdare::cache_engine::lookup::composable
