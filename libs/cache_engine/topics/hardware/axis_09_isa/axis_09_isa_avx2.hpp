#pragma once
// V41.F.6.1.R7.5.i axis_09 IsaAvx2 (AVX2 256-bit, modern x86_64)

#include "axis_09_isa_strategy_base.hpp"
#include "axis_09_isa_subaxes_is1_to_is3.hpp"
#include "concepts/axis_09_isa_cache_engine_permutation_concept.hpp"
#include "axis_09_isa_flags.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09_isa {

/// IsaAvx2 — AVX2 256-bit Vector-Operationen (modern x86_64 ab Haswell 2013).
/// Doppelte Vector-Breite vs SSE2: 256-bit Integer + Float + Gather/Scatter +
/// AVX2-Permute. Standard fuer ART/HOT-Optimierungen (8-Wege parallel).
class IsaAvx2 : public IsaStrategyBase<IsaAvx2> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::simd_width_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::avx2_enabled;

    [[nodiscard]] static constexpr bool             supports_simd() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()          noexcept { return "isa_avx2"; }
    [[nodiscard]] static constexpr std::string_view family_name()   noexcept { return "IsaAvx2 (AVX2 256-bit, Haswell+ 2013, ART/HOT 8-Wege parallel)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()   noexcept { return "AVX2"; }
};

}  // namespace

namespace comdare::cache_engine::hardware::axis_09_isa {
    static_assert(concepts::IsaStrategy<IsaAvx2>);
    static_assert(concepts::CacheEnginePermutationStrategy<IsaAvx2>);
}
