#pragma once
// V41.F.6.1.R7.5.i.2 axis_09 Amd64Isa (x86_64 / Intel 64, dominante Server-CPU)

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

#if defined(__x86_64__) || defined(_M_X64)
#include <emmintrin.h> // SSE2 intrinsics (_mm_loadu_si128 / _mm_add_epi32)
#endif

namespace comdare::cache_engine::simd {

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

    [[nodiscard]] static constexpr bool             is_64bit() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view cpu_family() noexcept { return "x86_64"; }
    [[nodiscard]] static constexpr bool             supports_native_simd() noexcept { return true; } // SSE2 baseline
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "isa_amd64"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "Amd64Isa (x86_64/Intel 64, ZIH Barnard/Capella, AMD EPYC + Intel Xeon)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "AMD64"; }

    // V41.F.6.1 — verhaltens-tragende Laufzeit-API (ISA-Achse T12 F15-operativ, Goldstandard-Signatur
    // analog scan_field_sum/serialize_scan/node_find_scan). simd_field_sum addiert die ersten n
    // 32-bit-Worte des Puffers (lane-weise) und liefert die Summe.
    // Amd64: ECHTES SSE2 (_mm_loadu_si128 + _mm_add_epi32, 4 uint32-Lanes/Iteration) auf x86_64;
    // auf Nicht-x86-Build-Hosts faellt diese Klasse auf den ehrlichen Skalar-Pfad zurueck
    // (im Mess-Build MSVC/x86_64 ist stets der SSE2-Zweig aktiv).
    [[nodiscard]] static std::uint64_t simd_field_sum(unsigned char const* buf, std::size_t n) noexcept {
        std::uint64_t s = 0;
        std::size_t   i = 0;
#if defined(__x86_64__) || defined(_M_X64)
        // ECHTES SSE2: 4 uint32-Lanes pro 128-bit-Register parallel akkumulieren.
        __m128i acc = _mm_setzero_si128();
        for (; i + 4 <= n; i += 4) {
            __m128i v = _mm_loadu_si128(reinterpret_cast<__m128i const*>(buf + i * 4));
            acc       = _mm_add_epi32(acc, v);
        }
        alignas(16) std::uint32_t lanes[4];
        _mm_storeu_si128(reinterpret_cast<__m128i*>(lanes), acc);
        s += static_cast<std::uint64_t>(lanes[0]) + lanes[1] + lanes[2] + lanes[3];
#endif
        // Skalarer Rest (bzw. vollstaendig skalar auf Nicht-x86-Build-Hosts).
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
static_assert(concepts::IsaStrategy<Amd64Isa>);
static_assert(concepts::CacheEnginePermutationStrategy<Amd64Isa>);
} // namespace comdare::cache_engine::simd
