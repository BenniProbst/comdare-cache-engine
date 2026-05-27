#pragma once
// V41.F.6.1.R7.5.i axis_09_isa Zentrale Registry

#include <topics/hardware/axis_09_isa/axis_09_isa_flags.hpp>

#include "axis_09_isa_scalar.hpp"
#include "axis_09_isa_sse2.hpp"
#include "axis_09_isa_avx2.hpp"
#include "axis_09_isa_neon.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09_isa {

namespace mp = boost::mp11;

using AllIsas = mp::mp_list<
    IsaScalar,
    IsaSse2,
    IsaAvx2,
    IsaNeon
>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledIsas = mp::mp_filter<is_enabled, AllIsas>;

static_assert(mp::mp_size<EnabledIsas>::value > 0,
    "Axis 09 ISA: at least one ISA must be enabled");

}  // namespace
