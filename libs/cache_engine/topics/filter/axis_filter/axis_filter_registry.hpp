#pragma once
// V41.F.6.1.R7.5.e axis_filter Zentrale Registry

#include <topics/filter/axis_filter/axis_filter_flags.hpp>

#include "axis_filter_bloom.hpp"
#include "axis_filter_cuckoo.hpp"
#include "axis_filter_range_surf.hpp"
#include "axis_filter_xor.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::filter::axis_filter {

namespace mp = boost::mp11;

using AllFilters = mp::mp_list<
    BloomFilter,
    CuckooFilter,
    RangeSurfFilter,
    XorFilter
>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledFilters = mp::mp_filter<is_enabled, AllFilters>;

static_assert(mp::mp_size<EnabledFilters>::value > 0,
    "Axis Filter: at least one filter must be enabled");

}  // namespace
