#pragma once
// V41.F.6.1.R7.5.f axis_io Zentrale Registry

#include <axes/io_dispatch/axis_io_flags.hpp>

#include "axis_io_in_memory_only.hpp"
#include "axis_io_direct.hpp"
#include "axis_io_buffered.hpp"
#include "axis_io_mmap.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::io_dispatch {

namespace mp = boost::mp11;

using AllIos = mp::mp_list<
    InMemoryOnly,
    DirectIo,
    BufferedIo,
    MmapIo
>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledIos = mp::mp_filter<is_enabled, AllIos>;

static_assert(mp::mp_size<EnabledIos>::value > 0,
    "Axis IO: at least one IO strategy must be enabled");

}  // namespace
