#pragma once
// V41.F.6.1.R7.5.i.2 axis_09 PowerPcIsa (POWER9/10 ppc64le, IBM Power Systems)

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

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
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

    [[nodiscard]] static constexpr bool             is_64bit() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view cpu_family() noexcept { return "ppc64le"; }
    [[nodiscard]] static constexpr bool             supports_native_simd() noexcept { return true; } // VSX baseline
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "isa_powerpc"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::simd::PowerPcIsa", "axes/simd/axis_09_isa_powerpc.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "PowerPcIsa (POWER9/10 ppc64le, IBM Power Systems, AC922/IC922 HPC)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "POWERPC"; }

    [[nodiscard]] static constexpr std::uint16_t group_match_mask(std::uint8_t const* ctrl16,
                                                                  std::uint8_t        needle) noexcept {
        std::uint16_t mask = 0;
        for (std::size_t i = 0; i < 16u; ++i) {
            if (ctrl16[i] == needle) mask = static_cast<std::uint16_t>(mask | (std::uint16_t{1} << i));
        }
        return mask;
    }

    // V41.F.6.1 — verhaltens-tragende Laufzeit-API (ISA-Achse T12 F15-operativ, Goldstandard-Signatur).
    // EHRLICHE KENNZEICHNUNG: Der VSX-Vektorpfad (128-bit, 4×32-bit) existiert nur auf echten
    // ppc64le-Hosts; der Mess-Build laeuft MSVC/x86_64, daher ist dies ein UNROLLED-Skalar-Fallback
    // (4 Lanes entrollt, modelliert die VSX-128-bit-Breite ohne ppc-Intrinsics). KEINE Konstante:
    // die Summe ist daten-/strategieabhaengig und erzeugt eine reale, vom Amd64-SSE2-Pfad messbar
    // abweichende Laufzeit.
    [[nodiscard]] static std::uint64_t simd_field_sum(unsigned char const* buf, std::size_t n) noexcept {
        std::uint64_t l0 = 0, l1 = 0, l2 = 0, l3 = 0;
        std::size_t   i = 0;
        for (; i + 4 <= n; i += 4) { // 4-fach entrollt = VSX-Lane-Breite (skalarer Fallback)
            std::uint32_t v0, v1, v2, v3;
            std::memcpy(&v0, buf + (i + 0) * 4, sizeof(v0));
            std::memcpy(&v1, buf + (i + 1) * 4, sizeof(v1));
            std::memcpy(&v2, buf + (i + 2) * 4, sizeof(v2));
            std::memcpy(&v3, buf + (i + 3) * 4, sizeof(v3));
            l0 += v0;
            l1 += v1;
            l2 += v2;
            l3 += v3;
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

} // namespace comdare::cache_engine::simd

namespace comdare::cache_engine::simd {
static_assert(concepts::IsaStrategy<PowerPcIsa>);
static_assert(concepts::CacheEnginePermutationStrategy<PowerPcIsa>);
} // namespace comdare::cache_engine::simd
