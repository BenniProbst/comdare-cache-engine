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

#if defined(__AVX2__) || defined(__AVX512F__)
#include <immintrin.h> // AVX2/AVX-512 intrinsics (+ transitiv SSE2) — Phase 0.1 SIMD-Dispatch
#elif defined(__x86_64__) || defined(_M_X64)
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

    [[nodiscard]] static constexpr std::uint16_t group_match_mask(std::uint8_t const* ctrl16,
                                                                  std::uint8_t        needle) noexcept {
        if consteval {
            std::uint16_t mask = 0;
            for (std::size_t i = 0; i < 16u; ++i) {
                if (ctrl16[i] == needle) mask = static_cast<std::uint16_t>(mask | (std::uint16_t{1} << i));
            }
            return mask;
        } else {
#if defined(__x86_64__) || defined(_M_X64)
            __m128i const ctrl = _mm_loadu_si128(reinterpret_cast<__m128i const*>(ctrl16));
            __m128i const key  = _mm_set1_epi8(static_cast<char>(needle));
            __m128i const eq   = _mm_cmpeq_epi8(ctrl, key);
            return static_cast<std::uint16_t>(_mm_movemask_epi8(eq));
#else
            std::uint16_t mask = 0;
            for (std::size_t i = 0; i < 16u; ++i) {
                if (ctrl16[i] == needle) mask = static_cast<std::uint16_t>(mask | (std::uint16_t{1} << i));
            }
            return mask;
#endif
        }
    }

    // V41.F.6.1 — verhaltens-tragende Laufzeit-API (ISA-Achse T12 F15-operativ, Goldstandard-Signatur
    // analog scan_field_sum/serialize_scan/node_find_scan). simd_field_sum addiert die ersten n
    // 32-bit-Worte des Puffers (lane-weise) und liefert die Summe.
    // Phase 0.1 SIMD-Dispatch (Doc 21 §F): die aktive Vektor-Breite folgt den Compiler-SIMD-Makros
    // (-mavx2 -> __AVX512F__/__AVX2__), sodass die simd_extension-Achse (axis_09b) den Compute waehlen KANN,
    // sobald der Tier-Modul-Build die passende ISA-Flag aus der 09b-Achse setzt. Verdrahtung 09b->Flag seit
    // GO-3 A1 (Task #5, 2026-07-12) GEKOPPELT: comdare_apply_simd_extension_flags(<target> <EXT>) in
    // cmake/isa_features.cmake + consteval-Kohaerenz-Guard axis_09b_build_coherence.hpp (CHECKED-Inspection-
    // Makro erzwingt Deklaration == Build-ISA-Stufe); E2-Voll-Matrix-Emission je 09b-Variante bleibt
    // Folge-Slice der #276-Build-Matrix-Doktrin. Compile-time-strikt, kein
    // Runtime-Switch. Alle Pfade akkumulieren in uint64 (32->64 vor der Addition gewidened) -> das Ergebnis
    // ist die WAHRE Summe, BUILD-INVARIANT ueber SSE2/AVX2/AVX512/Skalar (kein per-lane-32-bit-Overflow-Drift;
    // Review wf_fd87be00). Der Element-Schritt je Iteration (4/8/16) = Lane-Breite fuer die Mess-Huelle
    // (axis_09_isa_observable.hpp::lane_width_ spiegelt dieselbe Kaskade). Nicht-x86 / kein SIMD-Flag -> Skalar.
    [[nodiscard]] static std::uint64_t simd_field_sum(unsigned char const* buf, std::size_t n) noexcept {
        std::uint64_t s = 0;
        std::size_t   i = 0;
#if defined(__AVX512F__)
        // AVX-512F: 16 uint32/Iter, je 8 -> uint64 gewidened (_mm512_cvtepu32_epi64), 8 uint64-Lanes akkumuliert.
        __m512i acc = _mm512_setzero_si512();
        for (; i + 16 <= n; i += 16) {
            __m256i const a = _mm256_loadu_si256(reinterpret_cast<__m256i const*>(buf + i * 4));
            __m256i const b = _mm256_loadu_si256(reinterpret_cast<__m256i const*>(buf + (i + 8) * 4));
            acc             = _mm512_add_epi64(acc, _mm512_cvtepu32_epi64(a));
            acc             = _mm512_add_epi64(acc, _mm512_cvtepu32_epi64(b));
        }
        alignas(64) std::uint64_t lanes[8];
        _mm512_storeu_si512(reinterpret_cast<void*>(lanes), acc);
        for (std::uint64_t l : lanes) s += l;
#elif defined(__AVX2__)
        // AVX2: 8 uint32/Iter, je 4 -> uint64 gewidened (_mm256_cvtepu32_epi64), 4 uint64-Lanes akkumuliert.
        __m256i acc = _mm256_setzero_si256();
        for (; i + 8 <= n; i += 8) {
            __m128i const a = _mm_loadu_si128(reinterpret_cast<__m128i const*>(buf + i * 4));
            __m128i const b = _mm_loadu_si128(reinterpret_cast<__m128i const*>(buf + (i + 4) * 4));
            acc             = _mm256_add_epi64(acc, _mm256_cvtepu32_epi64(a));
            acc             = _mm256_add_epi64(acc, _mm256_cvtepu32_epi64(b));
        }
        alignas(32) std::uint64_t lanes[4];
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(lanes), acc);
        for (std::uint64_t l : lanes) s += l;
#elif defined(__x86_64__) || defined(_M_X64)
        // SSE2: 4 uint32/Iter, via unpack{lo,hi}_epi32 mit 0 -> je 2 uint64, 2 uint64-Lanes akkumuliert.
        __m128i       acc  = _mm_setzero_si128();
        __m128i const zero = _mm_setzero_si128();
        for (; i + 4 <= n; i += 4) {
            __m128i const v = _mm_loadu_si128(reinterpret_cast<__m128i const*>(buf + i * 4));
            acc             = _mm_add_epi64(acc, _mm_unpacklo_epi32(v, zero));
            acc             = _mm_add_epi64(acc, _mm_unpackhi_epi32(v, zero));
        }
        alignas(16) std::uint64_t lanes[2];
        _mm_storeu_si128(reinterpret_cast<__m128i*>(lanes), acc);
        s += lanes[0] + lanes[1];
#endif
        // Skalarer Rest (bzw. vollstaendig skalar auf Nicht-x86-Build-Hosts / ohne SIMD-Flag).
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
