#pragma once
// V41.F.6.1.R7.5.i axis_09 IsaNeon (ARM NEON 128-bit, ARMv8+)

#include "axis_09_isa_strategy_base.hpp"
#include "axis_09_isa_subaxes_is1_to_is3.hpp"
#include "concepts/axis_09_isa_cache_engine_permutation_concept.hpp"
#include "axis_09_isa_flags.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09_isa {

/// IsaNeon — ARM NEON 128-bit Vector-Operationen (ARMv8+ Standard).
/// Pflicht-Teil der AArch64 ABI. Apple M-Series + ARM Server (Graviton).
/// 16-byte aligned, Integer + Float Vector-Operations. Geringere
/// Vector-Breite als AVX2, aber niedrigere Latenz pro Operation.
class IsaNeon : public IsaStrategyBase<IsaNeon> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::vector_arch_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::neon_enabled;

    [[nodiscard]] static constexpr bool             supports_simd() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()          noexcept { return "isa_neon"; }
    [[nodiscard]] static constexpr std::string_view family_name()   noexcept { return "IsaNeon (ARM NEON 128-bit, ARMv8+ AArch64 ABI-baseline, Apple M/Graviton)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()   noexcept { return "NEON"; }
};

}  // namespace

namespace comdare::cache_engine::hardware::axis_09_isa {
    static_assert(concepts::IsaStrategy<IsaNeon>);
    static_assert(concepts::CacheEnginePermutationStrategy<IsaNeon>);
}
