#pragma once
// V41.F.6.1.R7.1.d axis_04_node_type Zentrale Registry

#include <axes/node/axis_04_node_type_flags.hpp>

#include "axis_04_node_type_node4.hpp"
#include "axis_04_node_type_node16.hpp"
#include "axis_04_node_type_node48.hpp"
#include "axis_04_node_type_node256.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::node {

namespace mp = boost::mp11;

using AllNodeTypes = mp::mp_list<Node4NodeType, Node16NodeType, Node48NodeType, Node256NodeType>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledNodeTypes = mp::mp_filter<is_enabled, AllNodeTypes>;

static_assert(mp::mp_size<EnabledNodeTypes>::value > 0, "Axis 04 NodeType: at least one node-type must be enabled");

} // namespace comdare::cache_engine::node
