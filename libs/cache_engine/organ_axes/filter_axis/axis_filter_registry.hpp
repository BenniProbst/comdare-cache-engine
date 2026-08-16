#pragma once
// V41.F.6.1.R7.5.e axis_filter Zentrale Registry

#include <organ_axes/filter_axis/axis_filter_flags.hpp>

#include "axis_filter_bloom.hpp"
#include "axis_filter_cuckoo.hpp"
#include "axis_filter_none.hpp"
#include "axis_filter_range_surf.hpp"
#include "axis_filter_xor.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::filter_axis {

namespace mp = boost::mp11;

// E14 (2026-08-06): NoneFilter END-APPEND, Default OFF (axis_filter_flags.hpp.in) -- golden-320-neutral
// (source_catalog.hpp:110/144 L14=filter, K14=1: golden nimmt mp_take_c<EnabledFilters,1>, Index 0 bleibt
// Bloom solange NoneFilter deaktiviert ist; End-Append aendert Index 0 nie). Praezedenz: S18-S21/AP-7a.
using AllFilters = mp::mp_list<BloomFilter, CuckooFilter, RangeSurfFilter, XorFilter, NoneFilter>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledFilters = mp::mp_filter<is_enabled, AllFilters>;

static_assert(mp::mp_size<EnabledFilters>::value > 0, "Axis Filter: at least one filter must be enabled");

} // namespace comdare::cache_engine::filter_axis
