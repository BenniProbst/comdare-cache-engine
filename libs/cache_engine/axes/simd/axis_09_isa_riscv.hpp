#pragma once
// V41.F.6.1.R7.5.i.2 axis_09 RiscVIsa (RV64GC Open-Source ISA)

#include "axis_09_isa_strategy_base.hpp"
#include "axis_09_isa_subaxes_is1_to_is3.hpp"
#include "concepts/axis_09_isa_cache_engine_permutation_concept.hpp"
#include <axes/simd/axis_09_isa_flags.hpp>
#include <topics/hardware/concepts/topic_hardware_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::simd {

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

    [[nodiscard]] static constexpr bool             is_64bit() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view cpu_family() noexcept { return "riscv64"; }
    [[nodiscard]] static constexpr bool             supports_native_simd() noexcept {
        return false;
    } // RVV optional, nicht baseline
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "isa_riscv"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "RiscVIsa (RV64GC Open-Source ISA, SiFive/Alibaba Xuantie/ETH PULP, Forschungs-Boards)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "RISCV"; }

    // V41.F.6.1 — verhaltens-tragende Laufzeit-API (ISA-Achse T12 F15-operativ, Goldstandard-Signatur).
    // EHRLICHE KENNZEICHNUNG: RV64GC hat KEIN baseline-SIMD (supports_native_simd()==false, RVV ist
    // optional). Daher bewusst ein PLAIN-Skalar-Pfad (NICHT entrollt) — modelliert die fehlende
    // Vektoreinheit der Basis-ISA und erzeugt damit eine reale, gegenueber Amd64-SSE2 (vektor) UND
    // Aarch64/PowerPc (entrollt) messbar abweichende, langsamere Laufzeit. KEINE erfundene Konstante.
    [[nodiscard]] static std::uint64_t simd_field_sum(unsigned char const* buf, std::size_t n) noexcept {
        std::uint64_t s = 0;
        for (std::size_t i = 0; i < n; ++i) { // plain scalar — kein baseline-SIMD auf RV64GC
            std::uint32_t v;
            std::memcpy(&v, buf + i * 4, sizeof(v));
            s += v;
        }
        return s;
    }
};

} // namespace comdare::cache_engine::simd

namespace comdare::cache_engine::simd {
static_assert(concepts::IsaStrategy<RiscVIsa>);
static_assert(concepts::CacheEnginePermutationStrategy<RiscVIsa>);
} // namespace comdare::cache_engine::simd
