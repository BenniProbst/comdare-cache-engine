#pragma once
// V41.F.6.1 F.6 axis_01_page_type Zentrale Registry (6 Pflicht-Seitentypen)

#include <topics/nodes/axis_01_page_type/axis_01_page_type_flags.hpp>

#include "axis_01_page_type_dense_byte.hpp"
#include "axis_01_page_type_extended_dense.hpp"
#include "axis_01_page_type_sparse_patricia.hpp"
#include "axis_01_page_type_redirect.hpp"
#include "axis_01_page_type_custom_cache.hpp"
#include "axis_01_page_type_bplus.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_01_page_type {

namespace mp = boost::mp11;

using AllPageTypes = mp::mp_list<
    DenseBytePageType,
    ExtendedDensePageType,
    SparsePatriciaPageType,
    RedirectPageType,
    CustomCachePageType,
    BPlusPageType
>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledPageTypes = mp::mp_filter<is_enabled, AllPageTypes>;

static_assert(mp::mp_size<EnabledPageTypes>::value > 0,
    "Axis 01 PageType: at least one page type must be enabled");

}  // namespace
