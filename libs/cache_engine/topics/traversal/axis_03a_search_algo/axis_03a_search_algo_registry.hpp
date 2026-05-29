#pragma once
// V41.F.6.1 axis_03a_search_algo Registry (W6-Pattern, 2026-05-26)
//
// @topic traversal @achse 03a search_algo

#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_flags.hpp>

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

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::traversal::axis_03a_search_algo {

namespace mp = boost::mp11;

/// AllStrategies — Komplette Liste aller bekannten 03a-Such-Strategien (Pilot 3 + Paper-Bindung 5 + F.6-Migration 1)
using AllStrategies = mp::mp_list<
    // Pilot Batch 1 (2026-05-26) — Cache-Engine Standalone Re-Impl
    Array256SearchAlgo,
    VectorU8U8SearchAlgo,
    VectorU16U16SearchAlgo,
    // V41.F.6.1.P2.D.tr.s2 (2026-05-26) — Original-Paper-Wrappers (Habich-Compliance)
    OriginalArtSearchAlgo,    // S04, P01 ART (Leis ICDE 2013, 4/4 originall)
    OriginalHotSearchAlgo,    // S05, P02 HOT (Binna PVLDB 2018, 2/4 originall + 2 Luecken)
    OriginalStartSearchAlgo,  // S06, P05 START (Mertens ICDE 2024, 2/4 originall + 2 Luecken)
    // V41.F.6.1.P2.D.tr.s3 Batch 1 (2026-05-26) — 2 weitere Paper-Wrappers (Masstree DEFERRED)
    OriginalWormholeSearchAlgo,  // S07, P07 Wormhole (Wu/Ni/Jiang ATC 2019, 3/4 originall + 1 Luecke)
    OriginalSurfSearchAlgo,      // S08, P10 SuRF (Zhang/Lim/Andersen SIGMOD 2018, 1/4 originall + 3 Luecken)
    // V41.F.6.1.F.6 (2026-05-29) — F.6-Migration aus prt-art internal_search/array_65535.hpp
    Array65535SearchAlgo,        // S09, prt-art REV6 §5.17 mid-density direct-addressed uint16 (kein Paper)
    // V41.F.6.1.R7.2 (2026-05-29) — Such-METHODEN (Re-Impl, is_original=false): 3 distinkte Paradigmen
    KArySearchAlgo,              // S10, k-ary search (Schlegel/Gemulla/Lehner DaMoN 2009) — SIMD-Partition
    InterpolationSearchAlgo,     // S11, interpolation search (Perl/Itai/Avni CACM 1978) — verteilungsbewusst
    EytzingerSearchAlgo          // S12, Eytzinger BFS-Layout branch-free (Khuong/Morin JEA 2017) — Cache-Layout
    // Vollausbau-Roadmap (Folge-Batches, Tree-STRUKTUR-Paper-Wrappers):
    // P03 Masstree DEFERRED — masstree.hh hat keine direkten Function-Bodies (alle Templates)
    // S13 P04 CoCo-trie (Read-Only, 0/4 originall — deferred wegen kein CRUD-API)
    // S14 P06 B²tree, S15 P20 BTreesAreBack, S16 P25 Mahling, S17 P29 RCU, S18 P30 HazardPointers
>;

template <typename S>
using is_enabled = mp::mp_bool<S::enabled>;

using EnabledStrategies = mp::mp_filter<is_enabled, AllStrategies>;

static_assert(mp::mp_size<EnabledStrategies>::value > 0,
    "axis_03a_search_algo: at least one strategy must be enabled");

}  // namespace
