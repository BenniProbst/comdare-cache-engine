#pragma once
// DOSSIER W1/234-K axis_bst_shape Zentrale Registry

#include <topics/nodes/axis_bst_shape/axis_bst_shape_flags.hpp>

#include "axis_bst_shape_ptr_size_t.hpp"
#include "axis_bst_shape_ptr_u32.hpp"
#include "axis_bst_shape_ptr_u16.hpp"

#include <boost/mp11.hpp>

namespace comdare::cache_engine::nodes::axis_bst_shape {

namespace mp = boost::mp11;

using AllShapes = mp::mp_list<BstPtrSizeT, BstPtrU32, BstPtrU16>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledShapes = mp::mp_filter<is_enabled, AllShapes>;

static_assert(mp::mp_size<EnabledShapes>::value > 0, "axis_bst_shape: at least one shape must be enabled");

} // namespace comdare::cache_engine::nodes::axis_bst_shape