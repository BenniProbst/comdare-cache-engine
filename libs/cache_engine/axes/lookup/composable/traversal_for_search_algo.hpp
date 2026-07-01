#pragma once
// E-Welle-A2 (Befund-2 / Q2-Schritt-4) — Schritt 2: Mapping store-traversierbarer Such-Algo → sein TREUES Traversal-Organ.
//
// @topic traversal @achse 03a @schicht composable
//
// **Zweck:** Für einen `StoreTraversableSearchAlgo` liefert dieses Mapping das Traversal-Organ, mit dem A2.5 das
// `container_t` parametrisiert — `ObservableComposedSearch<traversal_for_search_algo_t<Composition::search_algo>,
// LayoutAwareChunkedStore<…>>` statt hart-verdrahtetem SortedBinary. Dadurch sucht die search_algo-Achse über DENSELBEN
// node/layout/allocator-getriebenen Store (Befund-2-SOLL).
//
// **TREUE (kein Fidelitäts-Defekt):** nur Algos mit einem dedizierten, sie korrekt abbildenden Traversal-Organ werden
// gemappt: LinearScan→LinearScanTraversal, Interpolation→InterpolationTraversalOrgan, k-ary→KAryTraversal (#188-4a:
// k-Wege-Partition über den sortierten Flach-Store; lookup_in bit-identisch zur k_ary-Suche, insert/erase/scan an
// SortedBinary). Eytzinger (BFS-Layout) hat (noch) KEIN treues Flach-Store-Traversal → primäres Template `void`
// (= Weg-B; #188-4a-Eytzinger offen: User-Entscheid Mutations-Strategie). Tree/Trie/Hash (Pool-Substrat) ebenso `void` (#188-4b).

#include "composable_search.hpp"             // LinearScanTraversal, SortedBinaryTraversal
#include "interpolation_traversal_organ.hpp" // InterpolationTraversalOrgan
#include "k_ary_traversal_organ.hpp"         // KAryTraversal (#188-4a)

#include <type_traits> // std::is_same_v (A2.4-S2-static_asserts; vorher nur transitiv)

namespace comdare::cache_engine::lookup {

// Vorwärts-Deklaration der gemappten Such-Algo-Wrapper (vermeidet Voll-Include der Wrapper → keine Zirkularität).
class LinearScanSearchAlgo;
class InterpolationSearchAlgo;
class KArySearchAlgo; // #188-4a: store-traversierbar via KAryTraversal
// #188 per-K Increment 1 (2026-07-01): compile-time-K Wrapper-Familie -> je K sein eigenes KAryTraversal<K> (unten).
template <unsigned K>
class KArySearchAlgoT;

namespace composable {

// Primär: kein treues Flach-Store-Traversal → void (Weg-B-Zweig nutzt das Mapping ohnehin nicht; search_organ_ bleibt).
template <class S>
struct traversal_for_search_algo {
    using type = void;
};
template <>
struct traversal_for_search_algo<::comdare::cache_engine::lookup::LinearScanSearchAlgo> {
    using type = LinearScanTraversal;
};
template <>
struct traversal_for_search_algo<::comdare::cache_engine::lookup::InterpolationSearchAlgo> {
    using type = InterpolationTraversalOrgan;
};
template <>
struct traversal_for_search_algo<::comdare::cache_engine::lookup::KArySearchAlgo> {
    using type = KAryTraversal<4u>;
};
// #188 per-K Increment 1 (2026-07-01): jede compile-time-Aritaet K -> ihr eigenes KAryTraversal<K> (Weg-A). EINE
// partielle Spezialisierung deckt K in {2,4,8,16} (und jedes weitere K) ab -> container_ (abi_adapter:1890
// container_traversal_t) fuehrt den per-K-Wrapper ueber KAryTraversal<K>, NICHT SortedBinary. Das ist die per-K-
// Verdrahtung, die Increment 2 (Registry + enable-flags) zu 4 distinkten Tier-Binaries macht (je anderer K-Pfad).
template <unsigned K>
struct traversal_for_search_algo<::comdare::cache_engine::lookup::KArySearchAlgoT<K>> {
    using type = KAryTraversal<K>;
};

template <class S>
using traversal_for_search_algo_t = typename traversal_for_search_algo<S>::type;

// Verifikation (kein Raten): die store-traversierbaren Algos mappen auf ihr treues Organ; nicht-gemappte → void.
static_assert(std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::LinearScanSearchAlgo>,
                             LinearScanTraversal>,
              "A2.4-S2: linear_scan -> LinearScanTraversal");
static_assert(std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::InterpolationSearchAlgo>,
                             InterpolationTraversalOrgan>,
              "A2.4-S2: interpolation -> InterpolationTraversalOrgan");
static_assert(
    std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::KArySearchAlgo>, KAryTraversal<4u>>,
    "A2.4-S2 / #188-4a: k_ary -> KAryTraversal<4> (Default-Arity; per-K = StaticAxisNode-Build-Permutation, "
    "harness-gated)");
// #188 per-K Increment 1: jeder per-K-Wrapper mappt auf SEIN KAryTraversal<K> (nicht pauschal <4>) -> die
// K-Variation ist ein compile-time-realer, ANDERER Separator-Pfad (Meta-Lehre #3). Fuer alle 4 Aritaeten geprueft.
static_assert(std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::KArySearchAlgoT<2u>>,
                             KAryTraversal<2u>>,
              "#188 per-K: KArySearchAlgoT<2> -> KAryTraversal<2>");
static_assert(std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::KArySearchAlgoT<4u>>,
                             KAryTraversal<4u>>,
              "#188 per-K: KArySearchAlgoT<4> -> KAryTraversal<4>");
static_assert(std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::KArySearchAlgoT<8u>>,
                             KAryTraversal<8u>>,
              "#188 per-K: KArySearchAlgoT<8> -> KAryTraversal<8>");
static_assert(std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::KArySearchAlgoT<16u>>,
                             KAryTraversal<16u>>,
              "#188 per-K: KArySearchAlgoT<16> -> KAryTraversal<16>");

} // namespace composable
} // namespace comdare::cache_engine::lookup
