#pragma once
// V41.F.6.1.R7.5.i.2 axis_09 RiscVIsa (RV64GC Open-Source ISA)

#include "axis_09_isa_strategy_base.hpp"
#include "axis_09_isa_subaxes_is1_to_is3.hpp"
#include "concepts/axis_09_isa_cache_engine_permutation_concept.hpp"
#include "axis_09_isa_flags.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09_isa {

/// RiscVIsa — RISC-V 64-bit Open-Source ISA (RV64GC = G General + Compressed).
/// Wachsende Bedeutung: SiFive, Alibaba Xuantie, Andes, ETH Zuerich Cores.
/// Bei TU Dresden / ZIH: embedded + Forschungs-Boards.
/// SIMD-Sub-ISAs kompatibel: RVV (RISC-V Vector Extension, scalable 128-65536 bit).
class RiscVIsa : public IsaStrategyBase<RiscVIsa> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::vendor_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::riscv_enabled;

    [[nodiscard]] static constexpr bool             is_64bit()             noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view cpu_family()           noexcept { return "riscv64"; }
    [[nodiscard]] static constexpr bool             supports_native_simd() noexcept { return false; }  // RVV optional, nicht baseline
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "isa_riscv"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "RiscVIsa (RV64GC Open-Source ISA, SiFive/Alibaba Xuantie/ETH PULP, Forschungs-Boards)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "RISCV"; }
};

}  // namespace

namespace comdare::cache_engine::hardware::axis_09_isa {
    static_assert(concepts::IsaStrategy<RiscVIsa>);
    static_assert(concepts::CacheEnginePermutationStrategy<RiscVIsa>);
}
