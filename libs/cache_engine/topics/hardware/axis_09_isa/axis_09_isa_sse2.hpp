#pragma once
// V41.F.6.1.R7.5.i axis_09 IsaSse2 (SSE2 128-bit, x86_64 baseline)

#include "axis_09_isa_strategy_base.hpp"
#include "axis_09_isa_subaxes_is1_to_is3.hpp"
#include "concepts/axis_09_isa_cache_engine_permutation_concept.hpp"
#include "axis_09_isa_flags.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09_isa {

/// IsaSse2 — SSE2 128-bit Vector-Operationen (x86_64 baseline).
/// Pflicht-Teil der x86_64 ABI seit 2003. Auf jeder x86_64-CPU verfuegbar.
/// 16-byte aligned Loads/Stores, 128-bit Integer/Float-Operations.
class IsaSse2 : public IsaStrategyBase<IsaSse2> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::vector_arch_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::sse2_enabled;

    [[nodiscard]] static constexpr bool             supports_simd() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()          noexcept { return "isa_sse2"; }
    [[nodiscard]] static constexpr std::string_view family_name()   noexcept { return "IsaSse2 (SSE2 128-bit, x86_64 ABI-baseline since 2003)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()   noexcept { return "SSE2"; }
};

}  // namespace

namespace comdare::cache_engine::hardware::axis_09_isa {
    static_assert(concepts::IsaStrategy<IsaSse2>);
    static_assert(concepts::CacheEnginePermutationStrategy<IsaSse2>);
}
