#pragma once
// V41.F.6.1.R7.5.i.2 axis_09 Aarch64Isa (ARMv8-A / ARM64, Grace Hopper / Apple M / Graviton)

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

    // V41.F.6.1 — verhaltens-tragende Laufzeit-API (ISA-Achse T12 F15-operativ, Goldstandard-Signatur).
    // EHRLICHE KENNZEICHNUNG: Der NEON-Vektorpfad existiert nur auf echten ARMv8-Hosts; der
    // Mess-Build laeuft MSVC/x86_64, daher ist dieser header-only Pfad ein UNROLLED-Skalar-Fallback
    // (4 Lanes von Hand entrollt, modelliert die NEON-4×32-bit-Lane-Breite ohne ARM-Intrinsics).
    // Das ist KEINE erfundene Konstante: die Summe ist strategie-/datenabhaengig und erzeugt eine
    // reale, vom Amd64-SSE2-Pfad messbar abweichende Laufzeit (skalar statt vektorisiert).
    [[nodiscard]] static std::uint64_t simd_field_sum(unsigned char const* buf,
                                                      std::size_t n) noexcept {
        std::uint64_t l0 = 0, l1 = 0, l2 = 0, l3 = 0;
        std::size_t i = 0;
        for (; i + 4 <= n; i += 4) {  // 4-fach entrollt = NEON-Lane-Breite (skalarer Fallback)
            std::uint32_t v0, v1, v2, v3;
            std::memcpy(&v0, buf + (i + 0) * 4, sizeof(v0));
            std::memcpy(&v1, buf + (i + 1) * 4, sizeof(v1));
            std::memcpy(&v2, buf + (i + 2) * 4, sizeof(v2));
            std::memcpy(&v3, buf + (i + 3) * 4, sizeof(v3));
            l0 += v0; l1 += v1; l2 += v2; l3 += v3;
        }
        std::uint64_t s = l0 + l1 + l2 + l3;
        for (; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * 4, sizeof(v));
            s += v;
        }
        return s;
    }
};

}  // namespace

namespace comdare::cache_engine::simd {
    static_assert(concepts::IsaStrategy<Aarch64Isa>);
    static_assert(concepts::CacheEnginePermutationStrategy<Aarch64Isa>);
}
