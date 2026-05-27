#pragma once
// V41.F.6.1.R7.5.j axis_09b NeonExtension (Aarch64 ABI-baseline, 128-bit)

#include "axis_09b_simd_extension_strategy_base.hpp"
#include "axis_09b_simd_extension_subaxes_se1_to_se3.hpp"
#include "concepts/axis_09b_simd_extension_cache_engine_permutation_concept.hpp"
#include "axis_09b_simd_extension_flags.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {

/// NeonExtension — ARM NEON AdvSIMD 128-bit (Aarch64 ABI-baseline ARMv8+).
/// Auf jeder AArch64-CPU vorhanden: Apple M-Series, AWS Graviton 2/3/4,
/// NVIDIA Grace, Ampere Altra.
class NeonExtension : public SimdExtensionStrategyBase<NeonExtension> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::compat_family_tag;
    using family_id = std::integral_constant<int, 4>;

    static constexpr bool enabled = flags::neon_enabled;

    [[nodiscard]] static constexpr bool             is_active()             noexcept { return true; }
    [[nodiscard]] static constexpr int              vector_width_bits()     noexcept { return 128; }
    [[nodiscard]] static constexpr bool             compatible_with_x86()   noexcept { return false; }
    [[nodiscard]] static constexpr bool             compatible_with_arm()   noexcept { return true; }
    [[nodiscard]] static constexpr bool             compatible_with_riscv() noexcept { return false; }
    [[nodiscard]] static constexpr bool             compatible_with_powerpc() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()                  noexcept { return "simd_ext_neon"; }
    [[nodiscard]] static constexpr std::string_view family_name()           noexcept { return "NeonExtension (ARM AdvSIMD 128-bit, AArch64 ABI-baseline, Apple M/Graviton)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()           noexcept { return "NEON"; }
};

}  // namespace

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {
    static_assert(concepts::SimdExtensionStrategy<NeonExtension>);
    static_assert(concepts::CacheEnginePermutationStrategy<NeonExtension>);
}
