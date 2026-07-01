#pragma once
// V41.F.6.1 axis_03a_search_algo Registry (W6-Pattern, 2026-05-26)
//
// @topic traversal @achse 03a search_algo

#include <axes/lookup/axis_03a_search_algo_flags.hpp>

#include "axis_03a_search_algo_array256.hpp"
#include "axis_03a_search_algo_vector_u8u8.hpp"
#include "axis_03a_search_algo_vector_u16u16.hpp"
// V41.F.6.1.F.6 (2026-05-29) F.6-Migration aus prt-art internal_search/array_65535.hpp
#include "axis_03a_search_algo_array65535.hpp"
// V41.F.6.1.P2.D.tr.s2 Original-Paper-Wrappers (Habich-Compliance via Paper-Mixin)
#include "axis_03a_search_algo_original_art.hpp"
#include "axis_03a_search_algo_original_hot.hpp"
#include "axis_03a_search_algo_original_start.hpp"
// V41.F.6.1.P2.D.tr.s3 Batch 1 Original-Paper-Wrappers (P03 Masstree DEFERRED — siehe CMakeLists)
#include "axis_03a_search_algo_original_wormhole.hpp"
#include "axis_03a_search_algo_original_surf.hpp"
// V41.F.6.1.R7.2 (2026-05-29) S10 k-ary search (Such-METHODE, Schlegel/Gemulla/Lehner DaMoN 2009)
#include "axis_03a_search_algo_k_ary.hpp"
// V41.F.6.1.R7.2 (2026-05-29) S11 interpolation search (Such-METHODE, Perl/Itai/Avni CACM 1978)
#include "axis_03a_search_algo_interpolation.hpp"
// V41.F.6.1.R7.2 (2026-05-29) S12 eytzinger layout search (Such-METHODE, Khuong/Morin JEA 2017)
#include "axis_03a_search_algo_eytzinger.hpp"
// V41.F.6.1.R7.2 (2026-05-29) S13 skip list (geordnete STRUKTUR, Pugh CACM 1990)
#include "axis_03a_search_algo_skip_list.hpp"
// V41.F.6.1.R7.2 (2026-05-29) S14 hash search (ungeordnete open-addressing Hashtabelle, Knuth)
#include "axis_03a_search_algo_hash_search.hpp"
// V41.F.6.1.R7.2 (2026-05-29) S15 linear scan (unsortierte ART-Node4-Baseline, Leis ICDE 2013)
#include "axis_03a_search_algo_linear_scan.hpp"
// V41.F.6.1.R7.2 (2026-05-29) S16 binary search tree (unbalancierter BST, Hibbard, Knuth TAOCP 3)
#include "axis_03a_search_algo_bst.hpp"
// V41.F.6.1.R7.2 (2026-05-29) S17 B-tree (balancierter block-orientierter Mehrwege-Baum, Bayer/McCreight 1972)
#include "axis_03a_search_algo_btree.hpp"
// (E-Welle-A2 / Befund-2 / A2.4-S1) Klassifikations-Concept store-traversierbarer Such-Algorithmen (Array vs Pool, G3)
#include "composable/store_traversable_search_algo.hpp"
// (E-Welle-A2 / Befund-2 / A2.4-S2) Mapping store-traversierbarer Such-Algo -> treues Traversal-Organ (für A2.5)
#include "composable/traversal_for_search_algo.hpp"
// (#188-4b-a, 2026-07-01) Pool-Analogon: Mapping Weg-B-Pool-Familie -> ihr natives Composed*Search-Organ (für 4b-b
// container_t-Umstellung auf ObservableComposedContainer statt SortedBinary-Spiegel). Additiv: self-proving trait.
#include "composable/organ_for_search_algo.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::lookup {

namespace mp = boost::mp11;

/// AllStrategies — Komplette Liste aller bekannten 03a-Such-Strategien (Pilot 3 + Paper-Bindung 5 + F.6-Migration 1)
using AllStrategies = mp::mp_list<
    // Pilot Batch 1 (2026-05-26) — Cache-Engine Standalone Re-Impl
    Array256SearchAlgo, VectorU8U8SearchAlgo, VectorU16U16SearchAlgo,
    // V41.F.6.1.P2.D.tr.s2 (2026-05-26) — Original-Paper-Wrappers (Habich-Compliance)
    OriginalArtSearchAlgo,   // S04, P01 ART (Leis ICDE 2013, 4/4 originall)
    OriginalHotSearchAlgo,   // S05, P02 HOT (Binna PVLDB 2018, 2/4 originall + 2 Luecken)
    OriginalStartSearchAlgo, // S06, P05 START (Fent/Jungmair/Kipf/Neumann, ICDEW 2020, 2/4 originall + 2 Luecken)
    // V41.F.6.1.P2.D.tr.s3 Batch 1 (2026-05-26) — 2 weitere Paper-Wrappers (Masstree DEFERRED)
    OriginalWormholeSearchAlgo, // S07, P07 Wormhole (Wu/Ni/Jiang ATC 2019, 3/4 originall + 1 Luecke)
    OriginalSurfSearchAlgo,     // S08, P10 SuRF (Zhang/Lim/Andersen SIGMOD 2018, 1/4 originall + 3 Luecken)
    // V41.F.6.1.F.6 (2026-05-29) — F.6-Migration aus prt-art internal_search/array_65535.hpp
    Array65535SearchAlgo, // S09, prt-art REV6 §5.17 mid-density direct-addressed uint16 (kein Paper)
    // V41.F.6.1.R7.2 (2026-05-29) — Such-METHODEN (Re-Impl, is_original=false): 3 distinkte Paradigmen
    KArySearchAlgo,             // S10, k-ary search (Schlegel/Gemulla/Lehner DaMoN 2009) — SIMD-Partition
    InterpolationSearchAlgo,    // S11, interpolation search (Perl/Itai/Avni CACM 1978) — verteilungsbewusst
    EytzingerSearchAlgo,        // S12, Eytzinger BFS-Layout branch-free (Khuong/Morin JEA 2017) — Cache-Layout
    SkipListSearchAlgo,         // S13, Skip-Liste (Pugh CACM 1990) — probabilistische geordnete STRUKTUR
    HashSearchAlgo,             // S14, open-addressing Hashtabelle (Knuth TAOCP 3 §6.4) — UNGEORDNET, O(1)
    LinearScanSearchAlgo,       // S15, unsortierter linearer Scan (ART Node4-Baseline, Leis ICDE 2013)
    BinarySearchTreeSearchAlgo, // S16, unbalancierter BST (Hibbard-Deletion, Knuth TAOCP 3 §6.2.2)
    BTreeSearchAlgo, // S17, balancierter Mehrwege-B-Baum (Bayer/McCreight Acta Inf. 1972, t=4, block-orientiert)
    // #188 per-K Increment 2 (2026-07-01) — compile-time-K k-ary Wrapper-Familie (Weg-A): je K eine EIGENE Tier-
    // Binary (KArySearchAlgoT<K> -> KAryTraversal<K>, traversal_for_search_algo). ANS ENDE angehaengt, damit
    // mp_take_c<EnabledStrategies,4> (= golden-320-First-4 [k_ary, interpolation, eytzinger, linear_scan]) UNBERUEHRT
    // bleibt. Je EIGENES Enable-Flag (COMDARE_AXIS_03A_ENABLE_K_ARY_K2..K16, Default OFF wie der OriginalXxx-
    // Praezedenzfall) -> opt-in, nicht-disruptiv (EnabledStrategies waechst NICHT durch die Registrierung); distinkte
    // name() k_ary_k2..k16 (binary_id-Trennung). Konkrete Emission der 4 Binaries via dediziertes per-K-Profil+Katalog
    // (Increment 2b) — die bloße Registrierung baut NICHTS (profil-gated), sie macht die per-K nur selektierbar.
    KArySearchAlgoK2,  // S18, k-ary K=2 (Binaersuch-Baseline)
    KArySearchAlgoK4,  // S19, k-ary K=4 (Paper-Default, 5-Wege-Partition)
    KArySearchAlgoK8,  // S20, k-ary K=8
    KArySearchAlgoK16 // S21, k-ary K=16
    // Vollausbau-Roadmap (Folge-Batches, Tree-STRUKTUR-Paper-Wrappers):
    // P03 Masstree DEFERRED — masstree.hh hat keine direkten Function-Bodies (alle Templates)
    // S13 P04 CoCo-trie (Read-Only, 0/4 originall — deferred wegen kein CRUD-API)
    // S14 P06 B²tree, S15 P20 BTreesAreBack, S16 P25 Mahling, S17 P29 RCU, S18 P30 HazardPointers
    >;

template <typename S>
using is_enabled = mp::mp_bool<S::enabled>;

using EnabledStrategies = mp::mp_filter<is_enabled, AllStrategies>;

static_assert(mp::mp_size<EnabledStrategies>::value > 0, "axis_03a_search_algo: at least one strategy must be enabled");

// (E-Welle-A2 / Befund-2 / A2.4-S1) Verifikation der store-traversierbaren Klassifikation über die ZIEL-Population
// (Meta-Lehre #1/#2: echte Registry-Typen, nicht nur Referenz-Kompositionen). Array-Familie → store-traversierbar
// (Suche über DENSELBEN node/layout/allocator-getriebenen LayoutAwareChunkedStore); k-ary jetzt store-traversierbar
// (#188-4a-C5: treues compile-time KAryTraversal<Arity>, test_conformance_gate-grün). Tree/Trie/Hash + Eytzinger
// (BFS-Layout) → Weg-B (G3-Cross-Achsen-Constraint, Doc 34 §3/§9.1). A2.5 nutzt dies für die container_t-if-constexpr-Umstellung.
static_assert(composable::StoreTraversableSearchAlgo<KArySearchAlgo>,
              "#188-4a-C5: k-ary = Array-Familie + treues compile-time KAryTraversal<Arity> -> store-traversierbar (Weg-A)");
// #188 per-K Increment 2: die 4 registrierten per-K-Wrapper sind ebenfalls store-traversierbar (Weg-A-Marker) ->
// container_traversal_t (abi_adapter:1890) fuehrt jeden ueber SEIN KAryTraversal<K>, NICHT SortedBinary. Ueber die
// ZIEL-Population verifiziert (nicht nur Referenz-Kompositionen).
static_assert(composable::StoreTraversableSearchAlgo<KArySearchAlgoK2> &&
                  composable::StoreTraversableSearchAlgo<KArySearchAlgoK4> &&
                  composable::StoreTraversableSearchAlgo<KArySearchAlgoK8> &&
                  composable::StoreTraversableSearchAlgo<KArySearchAlgoK16>,
              "#188 per-K Inc2: alle 4 per-K-Wrapper store-traversierbar (Weg-A)");
static_assert(composable::StoreTraversableSearchAlgo<InterpolationSearchAlgo>,
              "A2.4-S1: interpolation = Array-Familie -> store-traversierbar");
static_assert(composable::StoreTraversableSearchAlgo<LinearScanSearchAlgo>,
              "A2.4-S1: linear_scan = Array-Familie -> store-traversierbar");
static_assert(!composable::StoreTraversableSearchAlgo<EytzingerSearchAlgo>,
              "A2.4-S1: eytzinger = BFS-Layout -> konservativ Weg-B (kein Marker)");
static_assert(!composable::StoreTraversableSearchAlgo<HashSearchAlgo>,
              "A2.4-S1: hash = Pool-Familie (ungeordnet) -> Weg-B (G3)");
static_assert(!composable::StoreTraversableSearchAlgo<BinarySearchTreeSearchAlgo>,
              "A2.4-S1: BST = Pool-Familie (Knoten-Pool) -> Weg-B (G3)");

} // namespace comdare::cache_engine::lookup
