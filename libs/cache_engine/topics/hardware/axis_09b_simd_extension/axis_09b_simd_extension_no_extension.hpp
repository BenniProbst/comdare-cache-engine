#pragma once
// V41.F.6.1.R7.5.j axis_09b NoSimdExtension (Baseline: kein SIMD-Accelerator)

#include "axis_09b_simd_extension_strategy_base.hpp"
#include "axis_09b_simd_extension_subaxes_se1_to_se3.hpp"
#include "concepts/axis_09b_simd_extension_cache_engine_permutation_concept.hpp"
#include "axis_09b_simd_extension_flags.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {

/// NoSimdExtension — Baseline: Permutation NUTZT keinen Beschleuniger.
/// Universell kompatibel zu allen Haupt-ISAs. Pflicht-Variante fuer
/// "ohne SIMD/Accelerator"-Permutations-Vergleich (gegen mit-Beschleuniger).
class NoSimdExtension : public SimdExtensionStrategyBase<NoSimdExtension> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::vector_width_tag;
    using family_id = std::integral_constant<int, 0>;

    static constexpr bool enabled = flags::no_extension_enabled;

    [[nodiscard]] static constexpr bool             is_active() noexcept { return false; }
    [[nodiscard]] static constexpr int              vector_width_bits() noexcept { return 0; }
    [[nodiscard]] static constexpr bool             compatible_with_x86() noexcept { return true; }
    [[nodiscard]] static constexpr bool             compatible_with_arm() noexcept { return true; }
    [[nodiscard]] static constexpr bool             compatible_with_riscv() noexcept { return true; }
    [[nodiscard]] static constexpr bool             compatible_with_powerpc() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "simd_ext_none"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "NoSimdExtension (baseline, no SIMD/accelerator, alle Haupt-ISAs compat)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "NO_EXTENSION"; }
};

} // namespace comdare::cache_engine::hardware::axis_09b_simd_extension

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {
static_assert(concepts::SimdExtensionStrategy<NoSimdExtension>);
static_assert(concepts::CacheEnginePermutationStrategy<NoSimdExtension>);
} // namespace comdare::cache_engine::hardware::axis_09b_simd_extension
