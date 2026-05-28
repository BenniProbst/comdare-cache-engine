#pragma once
// V41.F.6.1.R7.5.d axis_14_value_handle Zentrale Registry

#include <topics/value_handle/axis_14_value_handle/axis_14_value_handle_flags.hpp>

#include "axis_14_value_handle_inline.hpp"
#include "axis_14_value_handle_external_pool.hpp"
#include "axis_14_value_handle_immutable_shared_ref.hpp"
#include "axis_14_value_handle_versioned_pointer.hpp"
#include "axis_14_value_handle_chain_ref.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::value_handle::axis_14_value_handle {

namespace mp = boost::mp11;

using AllHandles = mp::mp_list<
    InlineValueHandle,
    ExternalPoolValueHandle,
    ImmutableSharedRefValueHandle,
    VersionedPointerValueHandle,
    ChainRefValueHandle
>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledHandles = mp::mp_filter<is_enabled, AllHandles>;

static_assert(mp::mp_size<EnabledHandles>::value > 0,
    "Axis 14 ValueHandle: at least one handle must be enabled");

}  // namespace
