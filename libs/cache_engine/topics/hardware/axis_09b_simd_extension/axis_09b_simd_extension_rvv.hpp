#pragma once
// V41.F.6.1.R7.5.j axis_09b RvvExtension (RISC-V Vector Extension, scalable)

#include "axis_09b_simd_extension_strategy_base.hpp"
#include "axis_09b_simd_extension_subaxes_se1_to_se3.hpp"
#include "concepts/axis_09b_simd_extension_cache_engine_permutation_concept.hpp"
#include "axis_09b_simd_extension_flags.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {

/// RvvExtension — RISC-V Vector Extension (RVV v1.0 ratifiziert 2021).
/// Scalable Vector (VLEN 128 bis 65536 bit). Hardware-spezifisch
/// (z.B. SiFive Performance P870: 128-bit, T-Head Xuantie C908: 128-bit,
/// theoretisch bis 2048-bit fuer HPC-Cores).
class RvvExtension : public SimdExtensionStrategyBase<RvvExtension> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::compat_family_tag;
    using family_id = std::integral_constant<int, 6>;

    static constexpr bool enabled = flags::rvv_enabled;

    [[nodiscard]] static constexpr bool             is_active()             noexcept { return true; }
    [[nodiscard]] static constexpr int              vector_width_bits()     noexcept { return -1; }  // scalable
    [[nodiscard]] static constexpr bool             compatible_with_x86()   noexcept { return false; }
    [[nodiscard]] static constexpr bool             compatible_with_arm()   noexcept { return false; }
    [[nodiscard]] static constexpr bool             compatible_with_riscv() noexcept { return true; }
    [[nodiscard]] static constexpr bool             compatible_with_powerpc() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()                  noexcept { return "simd_ext_rvv"; }
    [[nodiscard]] static constexpr std::string_view family_name()           noexcept { return "RvvExtension (RISC-V Vector v1.0 scalable VLEN, SiFive/T-Head)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()           noexcept { return "RVV"; }
};

}  // namespace

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {
    static_assert(concepts::SimdExtensionStrategy<RvvExtension>);
    static_assert(concepts::CacheEnginePermutationStrategy<RvvExtension>);
}
