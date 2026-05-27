#pragma once
// V41.F.6.1.R7.5.c axis_10_serialization Zentrale Registry

#include <topics/serialization/axis_10_serialization/axis_10_serialization_flags.hpp>

#include "axis_10_serialization_raw_binary.hpp"
#include "axis_10_serialization_var_len.hpp"
#include "axis_10_serialization_succinct.hpp"
#include "axis_10_serialization_compressed.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::serialization::axis_10_serialization {

namespace mp = boost::mp11;

using AllSerializers = mp::mp_list<
    RawBinarySerialization,
    VarLenSerialization,
    SuccinctSerialization,
    CompressedSerialization
>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledSerializers = mp::mp_filter<is_enabled, AllSerializers>;

static_assert(mp::mp_size<EnabledSerializers>::value > 0,
    "Axis 10 Serialization: at least one serializer must be enabled");

}  // namespace
