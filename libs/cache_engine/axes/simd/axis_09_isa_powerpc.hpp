#pragma once
// V41.F.6.1.R7.5.i.2 axis_09 PowerPcIsa (POWER9/10 ppc64le, IBM Power Systems)

#include "axis_09_isa_strategy_base.hpp"
#include "axis_09_isa_subaxes_is1_to_is3.hpp"
#include "concepts/axis_09_isa_cache_engine_permutation_concept.hpp"
#include <axes/simd/axis_09_isa_flags.hpp>
#include <topics/hardware/concepts/topic_hardware_concept.hpp>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::simd {

/// PowerPcIsa — IBM Power-Architecture POWER9/POWER10 (ppc64le Little-Endian).
/// IBM Power Systems (AC922, IC922, S1022). Vor allem fuer HPC Mixed-Workloads
/// (NVLink-Connected GPU bei AC922). Verfuegbar an Summit/Sierra (Oak Ridge).
/// Bei TU Dresden / ZIH: nicht primaer, aber relevant fuer DBMS-Forschung.
/// SIMD-Sub-ISAs kompatibel: VSX (Vector-Scalar Extension, 128-bit).
class PowerPcIsa : public IsaStrategyBase<PowerPcIsa> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::vendor_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::powerpc_enabled;

    [[nodiscard]] static constexpr bool             is_64bit()             noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view cpu_family()           noexcept { return "ppc64le"; }
    [[nodiscard]] static constexpr bool             supports_native_simd() noexcept { return true; }  // VSX baseline
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "isa_powerpc"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "PowerPcIsa (POWER9/10 ppc64le, IBM Power Systems, AC922/IC922 HPC)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "POWERPC"; }
};

}  // namespace

namespace comdare::cache_engine::simd {
    static_assert(concepts::IsaStrategy<PowerPcIsa>);
    static_assert(concepts::CacheEnginePermutationStrategy<PowerPcIsa>);
}
