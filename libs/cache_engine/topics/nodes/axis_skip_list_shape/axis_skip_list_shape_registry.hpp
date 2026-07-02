#pragma once
// DOSSIER W1/234-K axis_skip_list_shape Zentrale Registry

#include <topics/nodes/axis_skip_list_shape/axis_skip_list_shape_flags.hpp>

#include "axis_skip_list_shape_max16_p50.hpp"
#include "axis_skip_list_shape_max8_p50.hpp"
#include "axis_skip_list_shape_max32_p50.hpp"
#include "axis_skip_list_shape_max16_p25.hpp"

#include <boost/mp11.hpp>

namespace comdare::cache_engine::nodes::axis_skip_list_shape {

namespace mp = boost::mp11;

using AllShapes = mp::mp_list<SkipListMax16P50, SkipListMax8P50, SkipListMax32P50, SkipListMax16P25>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledShapes = mp::mp_filter<is_enabled, AllShapes>;

static_assert(mp::mp_size<EnabledShapes>::value > 0, "axis_skip_list_shape: at least one shape must be enabled");

} // namespace comdare::cache_engine::nodes::axis_skip_list_shape