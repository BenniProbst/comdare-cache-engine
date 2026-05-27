#pragma once
// V41.F.6.1.R7.5.i axis_09 IsaScalar (Goldstandard-Update, no SIMD baseline)

#include "axis_09_isa_strategy_base.hpp"
#include "axis_09_isa_subaxes_is1_to_is3.hpp"
#include "concepts/axis_09_isa_cache_engine_permutation_concept.hpp"
#include "axis_09_isa_flags.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09_isa {

/// IsaScalar — Default: kein SIMD, scalar baseline.
/// Portable Implementation ohne SSE/AVX/NEON. Allgegenwartig auf jeder CPU.
class IsaScalar : public IsaStrategyBase<IsaScalar> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::simd_width_tag;
    using family_id = std::integral_constant<int, 0>;

    static constexpr bool enabled = flags::scalar_enabled;

    [[nodiscard]] static constexpr bool             supports_simd() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()          noexcept { return "isa_scalar"; }
    [[nodiscard]] static constexpr std::string_view family_name()   noexcept { return "IsaScalar (no SIMD baseline, portable)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()   noexcept { return "SCALAR"; }
};

}  // namespace

namespace comdare::cache_engine::hardware::axis_09_isa {
    static_assert(concepts::IsaStrategy<IsaScalar>);
    static_assert(concepts::CacheEnginePermutationStrategy<IsaScalar>);
}
