#pragma once
// V41.F.6.1.R7.1.d axis_04_node_type Zentrale Registry

#include <topics/nodes/axis_04_node_type/axis_04_node_type_flags.hpp>

#include "axis_04_node_type_node4.hpp"
#include "axis_04_node_type_node16.hpp"
#include "axis_04_node_type_node48.hpp"
#include "axis_04_node_type_node256.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_04_node_type {

namespace mp = boost::mp11;

using AllNodeTypes = mp::mp_list<
    Node4Type,
    Node16Type,
    Node48Type,
    Node256Type
>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledNodeTypes = mp::mp_filter<is_enabled, AllNodeTypes>;

static_assert(mp::mp_size<EnabledNodeTypes>::value > 0,
    "Axis 04 NodeType: at least one node-type must be enabled");

}  // namespace
