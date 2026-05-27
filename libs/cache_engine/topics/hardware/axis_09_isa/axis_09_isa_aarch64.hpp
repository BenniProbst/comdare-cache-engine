#pragma once
// V41.F.6.1.R7.5.i.2 axis_09 Aarch64Isa (ARMv8-A / ARM64, Grace Hopper / Apple M / Graviton)

#include "axis_09_isa_strategy_base.hpp"
#include "axis_09_isa_subaxes_is1_to_is3.hpp"
#include "concepts/axis_09_isa_cache_engine_permutation_concept.hpp"
#include "axis_09_isa_flags.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09_isa {

/// Aarch64Isa — ARMv8-A 64-bit (ARM Holdings).
/// ZIH-Cluster: NVIDIA Grace Hopper (Neoverse V2 + Hopper GPU GH200).
/// Auch Apple M-Series, AWS Graviton 2/3/4, Ampere Altra. Pflicht-SIMD: NEON.
/// SIMD-Sub-ISAs kompatibel: NEON (baseline), SVE (ARMv8.2+), SVE2 (ARMv9/Grace), SME.
class Aarch64Isa : public IsaStrategyBase<Aarch64Isa> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::vendor_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::aarch64_enabled;

    [[nodiscard]] static constexpr bool             is_64bit()             noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view cpu_family()           noexcept { return "aarch64"; }
    [[nodiscard]] static constexpr bool             supports_native_simd() noexcept { return true; }  // NEON baseline
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "isa_aarch64"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "Aarch64Isa (ARMv8-A 64-bit, ZIH Grace Hopper GH200, Apple M/Graviton)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "AARCH64"; }
};

}  // namespace

namespace comdare::cache_engine::hardware::axis_09_isa {
    static_assert(concepts::IsaStrategy<Aarch64Isa>);
    static_assert(concepts::CacheEnginePermutationStrategy<Aarch64Isa>);
}
