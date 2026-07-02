#pragma once
// DOSSIER W1/234-K axis_btree_order Zentrale Registry

#include <topics/nodes/axis_btree_order/axis_btree_order_flags.hpp>

#include "axis_btree_order_kt2.hpp"
#include "axis_btree_order_kt3.hpp"
#include "axis_btree_order_kt4.hpp"
#include "axis_btree_order_kt8.hpp"
#include "axis_btree_order_kt16.hpp"

#include <boost/mp11.hpp>

namespace comdare::cache_engine::nodes::axis_btree_order {

namespace mp = boost::mp11;

using AllShapes = mp::mp_list<BtreeOrderKt2, BtreeOrderKt3, BtreeOrderKt4, BtreeOrderKt8, BtreeOrderKt16>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledShapes = mp::mp_filter<is_enabled, AllShapes>;

static_assert(mp::mp_size<EnabledShapes>::value > 0, "axis_btree_order: at least one shape must be enabled");

} // namespace comdare::cache_engine::nodes::axis_btree_order