#pragma once
// V41.F.6.1.R7.5.i.2 axis_09 Amd64Isa (x86_64 / Intel 64, dominante Server-CPU)

#include "axis_09_isa_strategy_base.hpp"
#include "axis_09_isa_subaxes_is1_to_is3.hpp"
#include "concepts/axis_09_isa_cache_engine_permutation_concept.hpp"
#include "axis_09_isa_flags.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09_isa {

/// Amd64Isa — x86_64 / Intel 64 (AMD/Intel Server+Desktop).
/// Dominante Server-Plattform. ZIH-Cluster: Barnard (AMD EPYC 7763 Zen 3),
/// Capella (Intel Xeon), Alpha Centauri (NVIDIA HGX + Intel Xeon).
/// SIMD-Sub-ISAs kompatibel: SSE2/SSE4/AVX/AVX2/AVX-512/BMI/AES-NI.
class Amd64Isa : public IsaStrategyBase<Amd64Isa> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::vendor_tag;
    using family_id = std::integral_constant<int, 0>;

    static constexpr bool enabled = flags::amd64_enabled;

    [[nodiscard]] static constexpr bool             is_64bit()             noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view cpu_family()           noexcept { return "x86_64"; }
    [[nodiscard]] static constexpr bool             supports_native_simd() noexcept { return true; }  // SSE2 baseline
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "isa_amd64"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "Amd64Isa (x86_64/Intel 64, ZIH Barnard/Capella, AMD EPYC + Intel Xeon)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "AMD64"; }
};

}  // namespace

namespace comdare::cache_engine::hardware::axis_09_isa {
    static_assert(concepts::IsaStrategy<Amd64Isa>);
    static_assert(concepts::CacheEnginePermutationStrategy<Amd64Isa>);
}
