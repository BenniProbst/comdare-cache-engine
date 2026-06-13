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
// gemappt: LinearScan→LinearScanTraversal, Interpolation→InterpolationTraversalOrgan. k-ary (k-Wege-SIMD-Partition) +
// Eytzinger (BFS-Layout) haben KEIN treues Flach-Store-Traversal → primäres Template `void` (= Weg-B; A2.4-S1-konsistent).
// Tree/Trie/Hash (Pool-Substrat) ebenso `void`. Ein künftiges KAryTraversal/EytzingerTraversal-Organ macht sie nachträglich mapbar.

#include "composable_search.hpp"               // LinearScanTraversal, SortedBinaryTraversal
#include "interpolation_traversal_organ.hpp"   // InterpolationTraversalOrgan

namespace comdare::cache_engine::lookup {

// Vorwärts-Deklaration der gemappten Such-Algo-Wrapper (vermeidet Voll-Include der Wrapper → keine Zirkularität).
class LinearScanSearchAlgo;
class InterpolationSearchAlgo;

namespace composable {

// Primär: kein treues Flach-Store-Traversal → void (Weg-B-Zweig nutzt das Mapping ohnehin nicht; search_organ_ bleibt).
template <class S> struct traversal_for_search_algo { using type = void; };
template <> struct traversal_for_search_algo<::comdare::cache_engine::lookup::LinearScanSearchAlgo>    { using type = LinearScanTraversal;        };
template <> struct traversal_for_search_algo<::comdare::cache_engine::lookup::InterpolationSearchAlgo> { using type = InterpolationTraversalOrgan; };

template <class S> using traversal_for_search_algo_t = typename traversal_for_search_algo<S>::type;

// Verifikation (kein Raten): die 2 store-traversierbaren Algos mappen auf ihr treues Organ; nicht-gemappte → void.
static_assert(std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::LinearScanSearchAlgo>,    LinearScanTraversal>,
    "A2.4-S2: linear_scan -> LinearScanTraversal");
static_assert(std::is_same_v<traversal_for_search_algo_t<::comdare::cache_engine::lookup::InterpolationSearchAlgo>, InterpolationTraversalOrgan>,
    "A2.4-S2: interpolation -> InterpolationTraversalOrgan");

}  // namespace composable
}  // namespace comdare::cache_engine::lookup
