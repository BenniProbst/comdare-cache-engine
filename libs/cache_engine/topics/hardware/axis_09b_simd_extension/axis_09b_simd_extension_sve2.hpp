#pragma once
// V41.F.6.1.R7.5.j axis_09b Sve2SimdExtension (Aarch64 ARMv9, Grace Hopper)

#include "axis_09b_simd_extension_strategy_base.hpp"
#include "axis_09b_simd_extension_subaxes_se1_to_se3.hpp"
#include "concepts/axis_09b_simd_extension_cache_engine_permutation_concept.hpp"
#include "axis_09b_simd_extension_flags.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {

/// Sve2SimdExtension — ARM SVE2 (Scalable Vector Extension 2, ARMv9 Mandatory).
/// ZIH-Cluster Grace Hopper (NVIDIA Grace = ARM Neoverse V2 = SVE2 128-bit).
/// Wichtig: vector_width_bits = "scalable" (-1) — Hardware-spezifisch
/// zwischen 128 und 2048 bit. Bei Grace: 128 bit pro Vector-Register.
class Sve2SimdExtension : public SimdExtensionStrategyBase<Sve2SimdExtension> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::vector_width_tag;
    using family_id = std::integral_constant<int, 5>;

    static constexpr bool enabled = flags::sve2_enabled;

    [[nodiscard]] static constexpr bool             is_active() noexcept { return true; }
    [[nodiscard]] static constexpr int              vector_width_bits() noexcept { return -1; } // scalable (128..2048)
    [[nodiscard]] static constexpr bool             compatible_with_x86() noexcept { return false; }
    [[nodiscard]] static constexpr bool             compatible_with_arm() noexcept { return true; }
    [[nodiscard]] static constexpr bool             compatible_with_riscv() noexcept { return false; }
    [[nodiscard]] static constexpr bool             compatible_with_powerpc() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "simd_ext_sve2"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "Sve2SimdExtension (ARM SVE2 scalable 128-2048bit, ARMv9, ZIH Grace Hopper)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "SVE2"; }

    // ─── R7.7.c Topologie: 1 SVE2-Unit/Sockel (Grace V2: alle Cores SVE2) ───
    [[nodiscard]] static constexpr int  units_per_socket() noexcept { return 1; }
    [[nodiscard]] static constexpr bool accessible_from_efficiency_cores() noexcept { return true; }
};

} // namespace comdare::cache_engine::hardware::axis_09b_simd_extension

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {
static_assert(concepts::SimdExtensionStrategy<Sve2SimdExtension>);
static_assert(concepts::CacheEnginePermutationStrategy<Sve2SimdExtension>);
} // namespace comdare::cache_engine::hardware::axis_09b_simd_extension
