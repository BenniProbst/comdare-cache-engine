#pragma once
// V41.F.6.1.R7.1.c axis_02_path_compression Zentrale Registry

#include <axes/path_compression/axis_02_path_compression_flags.hpp>

#include "axis_02_path_compression_none.hpp"
#include "axis_02_path_compression_patricia.hpp"
#include "axis_02_path_compression_byte_wise.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::path_compression {

namespace mp = boost::mp11;

using AllCompressions = mp::mp_list<PathCompressionNone, PatriciaPathCompression, ByteWisePathCompression>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledCompressions = mp::mp_filter<is_enabled, AllCompressions>;

static_assert(mp::mp_size<EnabledCompressions>::value > 0,
              "Axis 02 PathCompression: at least one compression must be enabled");

} // namespace comdare::cache_engine::path_compression
