#pragma once
// V41.F.6.1 axis_03a_search_algo Registry (W6-Pattern, 2026-05-26)
//
// @topic traversal @achse 03a search_algo

#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_flags.hpp>

#include "axis_03a_search_algo_array256.hpp"
#include "axis_03a_search_algo_vector_u8u8.hpp"
#include "axis_03a_search_algo_vector_u16u16.hpp"
// V41.F.6.1.P2.D.tr.s2 Original-Paper-Wrappers (Habich-Compliance via Paper-Mixin)
#include "axis_03a_search_algo_original_art.hpp"
#include "axis_03a_search_algo_original_hot.hpp"
#include "axis_03a_search_algo_original_start.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::traversal::axis_03a_search_algo {

namespace mp = boost::mp11;

/// AllStrategies — Komplette Liste aller bekannten 03a-Such-Strategien (Pilot 3 + Paper-Bindung 3)
using AllStrategies = mp::mp_list<
    // Pilot Batch 1 (2026-05-26) — Cache-Engine Standalone Re-Impl
    Array256,
    VectorU8U8,
    VectorU16U16,
    // V41.F.6.1.P2.D.tr.s2 (2026-05-26) — Original-Paper-Wrappers (Habich-Compliance)
    OriginalArtSearchAlgo,    // S04, P01 ART (Leis ICDE 2013, 4/4 originall)
    OriginalHotSearchAlgo,    // S05, P02 HOT (Binna PVLDB 2018, 2/4 originall + 2 Luecken)
    OriginalStartSearchAlgo   // S06, P05 START (Mertens ICDE 2024, 2/4 originall + 2 Luecken)
    // Vollausbau-Roadmap (Folge-Batches):
    // S07 Array65535 (Masstree-fanout), S08 LinearProbeHashSet, S09 RadixIndex,
    // S10 BinarySearchTree, S11 SkiplistIndex, S12 BloomFilteredArray, ...
>;

template <typename S>
using is_enabled = mp::mp_bool<S::enabled>;

using EnabledStrategies = mp::mp_filter<is_enabled, AllStrategies>;

static_assert(mp::mp_size<EnabledStrategies>::value > 0,
    "axis_03a_search_algo: at least one strategy must be enabled");

}  // namespace
