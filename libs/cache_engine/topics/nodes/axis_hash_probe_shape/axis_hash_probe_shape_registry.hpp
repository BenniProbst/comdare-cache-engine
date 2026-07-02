#pragma once
// DOSSIER W1/234-K axis_hash_probe_shape Zentrale Registry

#include <topics/nodes/axis_hash_probe_shape/axis_hash_probe_shape_flags.hpp>

#include "axis_hash_probe_shape_oa_lf70.hpp"
#include "axis_hash_probe_shape_oa_lf50.hpp"
#include "axis_hash_probe_shape_oa_lf90.hpp"
#include "axis_hash_probe_shape_chaining.hpp"

#include <boost/mp11.hpp>

namespace comdare::cache_engine::nodes::axis_hash_probe_shape {

namespace mp = boost::mp11;

using AllShapes = mp::mp_list<HashOaLf70, HashOaLf50, HashOaLf90, HashChaining>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledShapes = mp::mp_filter<is_enabled, AllShapes>;

static_assert(mp::mp_size<EnabledShapes>::value > 0, "axis_hash_probe_shape: at least one shape must be enabled");

} // namespace comdare::cache_engine::nodes::axis_hash_probe_shape