#pragma once
// V41.F.6.1.R7.5.j axis_09b_simd_extension Zentrale Registry

#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_flags.hpp>

#include "axis_09b_simd_extension_no_extension.hpp"
#include "axis_09b_simd_extension_sse2.hpp"
#include "axis_09b_simd_extension_avx2.hpp"
#include "axis_09b_simd_extension_avx512.hpp"
#include "axis_09b_simd_extension_neon.hpp"
#include "axis_09b_simd_extension_sve2.hpp"
#include "axis_09b_simd_extension_rvv.hpp"
#include "axis_09b_simd_extension_cuda_gh200.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {

namespace mp = boost::mp11;

using AllExtensions = mp::mp_list<NoSimdExtension, Sse2SimdExtension, Avx2SimdExtension, Avx512SimdExtension,
                                  NeonSimdExtension, Sve2SimdExtension, RvvSimdExtension, CudaGh200SimdExtension>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledExtensions = mp::mp_filter<is_enabled, AllExtensions>;

static_assert(mp::mp_size<EnabledExtensions>::value > 0,
              "Axis 09b SimdExtension: at least one extension (incl NoSimdExtension) must be enabled");

} // namespace comdare::cache_engine::hardware::axis_09b_simd_extension
