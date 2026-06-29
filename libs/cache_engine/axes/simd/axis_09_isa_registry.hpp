#pragma once
// V41.F.6.1.R7.5.i.2 axis_09_isa Zentrale Registry (Haupt-CPU-ISAs)

#include <axes/simd/axis_09_isa_flags.hpp>

#include "axis_09_isa_amd64.hpp"
#include "axis_09_isa_aarch64.hpp"
#include "axis_09_isa_riscv.hpp"
#include "axis_09_isa_powerpc.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::simd {

namespace mp = boost::mp11;

using AllIsas = mp::mp_list<Amd64Isa, Aarch64Isa, RiscVIsa, PowerPcIsa>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledIsas = mp::mp_filter<is_enabled, AllIsas>;

static_assert(mp::mp_size<EnabledIsas>::value > 0, "Axis 09 ISA: at least one CPU-ISA must be enabled");

} // namespace comdare::cache_engine::simd
