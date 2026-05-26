#pragma once
// V41.F.6.1 axis_03a_search_algo Registry (W6-Pattern, 2026-05-26)
//
// @topic traversal @achse 03a search_algo

#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_flags.hpp>

#include "axis_03a_search_algo_array256.hpp"
#include "axis_03a_search_algo_vector_u8u8.hpp"
#include "axis_03a_search_algo_vector_u16u16.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::traversal::axis_03a_search_algo {

namespace mp = boost::mp11;

/// AllStrategies — Komplette Liste aller bekannten 03a-Such-Strategien (Pilot 3)
using AllStrategies = mp::mp_list<
    // Pilot Batch 1 (2026-05-26)
    Array256,
    VectorU8U8,
    VectorU16U16
    // Vollausbau-Roadmap (Folge-Batches):
    // S04 Array65535 (Masstree-fanout), S05 LinearProbeHashSet, S06 RadixIndex,
    // S07 BinarySearchTree, S08 SkiplistIndex, S09 BloomFilteredArray, ...
>;

template <typename S>
using is_enabled = mp::mp_bool<S::enabled>;

using EnabledStrategies = mp::mp_filter<is_enabled, AllStrategies>;

static_assert(mp::mp_size<EnabledStrategies>::value > 0,
    "axis_03a_search_algo: at least one strategy must be enabled");

}  // namespace
