#pragma once
// V41.F.6.1.R7.5.j axis_09b_simd_extension CRTP-StrategyBase (Goldstandard)
//
// R7.7.b (#726) erweitert: SSE/AVX/AVX-512 Schichten-Modell (rueckwaerts-kumulativ).
// R7.7.c (#725) erweitert: CPU-Sockel-Count + P/E-Cores Topologie.
//
// Default-Werte: alle provides_xxx() = false, Topologie konservativ (Server-x86 baseline).
// Wrappers ueberschreiben selektiv via Name-Hiding (static constexpr methods).
//
// @doku docs/architecture/15_isa_layered_extension_+_paper_backlog.md §4 + §5
// @memory [[reference-isa-layered-extensions-and-avx512-subflags]]

#include "concepts/axis_09b_simd_extension_concept.hpp"
#include "concepts/axis_09b_simd_extension_cache_engine_permutation_concept.hpp"
#include "../../axis_base.hpp"
#include <cstddef>
#include <cstdint> // A6: std::uint32_t fuer provides_avx10_version()

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {

template <typename Derived>
class SimdExtensionStrategyBase : public ::comdare::cache_engine::topics::AxisBase {
public:
    // ─── SSE-Schichten (rueckwaerts-kumulativ, alle x86) ─────────────────────
    [[nodiscard]] static constexpr bool provides_sse() noexcept { return false; }
    [[nodiscard]] static constexpr bool provides_sse2() noexcept { return false; }
    [[nodiscard]] static constexpr bool provides_sse3() noexcept { return false; }
    [[nodiscard]] static constexpr bool provides_ssse3() noexcept { return false; }
    [[nodiscard]] static constexpr bool provides_sse4_1() noexcept { return false; }
    [[nodiscard]] static constexpr bool provides_sse4_2() noexcept { return false; }

    // ─── AVX-Schichten (alle x86) ─────────────────────────────────────────────
    [[nodiscard]] static constexpr bool provides_avx() noexcept { return false; }
    [[nodiscard]] static constexpr bool provides_avx2() noexcept { return false; }

    // ─── AVX-512 Foundation + Sub-Flags (15 separate Flags pro Hardware) ─────
    [[nodiscard]] static constexpr bool provides_avx512f() noexcept { return false; }
    [[nodiscard]] static constexpr bool provides_avx512cd() noexcept { return false; }
    [[nodiscard]] static constexpr bool provides_avx512er() noexcept { return false; } // Xeon Phi KNL only
    [[nodiscard]] static constexpr bool provides_avx512pf() noexcept { return false; } // Xeon Phi KNL only
    [[nodiscard]] static constexpr bool provides_avx512bw() noexcept { return false; }
    [[nodiscard]] static constexpr bool provides_avx512dq() noexcept { return false; }
    [[nodiscard]] static constexpr bool provides_avx512vl() noexcept { return false; }
    [[nodiscard]] static constexpr bool provides_avx512ifma() noexcept { return false; }
    [[nodiscard]] static constexpr bool provides_avx512vbmi() noexcept { return false; }
    [[nodiscard]] static constexpr bool provides_avx512vbmi2() noexcept { return false; }
    [[nodiscard]] static constexpr bool provides_avx512vnni() noexcept { return false; } // DBMS Vector-Indexes
    [[nodiscard]] static constexpr bool provides_avx512bitalg() noexcept { return false; }
    [[nodiscard]] static constexpr bool provides_avx512vpopcntdq() noexcept { return false; } // Bitmap-Indexes
    [[nodiscard]] static constexpr bool provides_avx512bf16() noexcept { return false; }
    [[nodiscard]] static constexpr bool provides_avx512fp16() noexcept { return false; }

    // A6 (G2-2 / AVX10-Feld, 2026-07-23): konvergierte AVX10-Version (0 = kein AVX10, N = AVX10.N; 1/2). Default 0 --
    // heute deklariert KEIN Wrapper AVX10-Hardware (ehrlich); ein kuenftiger AVX10-Wrapper ueberschreibt selektiv via
    // Name-Hiding (exakt das provides_xxx-Muster oben). build_variant_definition liest es via detail::detect_avx10_version.
    [[nodiscard]] static constexpr std::uint32_t provides_avx10_version() noexcept { return 0; }

    // ─── R7.7.c CPU-Sockel-Count + P/E-Cores Topologie ───────────────────────
    // units_per_socket: Anzahl SIMD-Einheiten pro CPU-Sockel.
    // 0 = keine SIMD (NoSimd), 1 = single Unit (z.B. AVX-512), 2 = dual Unit (typisch AVX2),
    // -1 = massive parallel (GPU).
    [[nodiscard]] static constexpr int units_per_socket() noexcept { return 0; }

    // shared_among_cores: true wenn mehrere CPU-Kerne sich Unit teilen.
    // Default true (SIMD-Units sind CPU-intern, geteilt via SMT).
    [[nodiscard]] static constexpr bool shared_among_cores() noexcept { return true; }

    // accessible_from_efficiency_cores: true wenn E-Cores (Intel Hybrid) Zugriff haben.
    // Default true (Standard-CPU ohne Hybrid). Avx512 setzt false (Alder/Raptor Lake disabled).
    [[nodiscard]] static constexpr bool accessible_from_efficiency_cores() noexcept { return true; }

protected:
    SimdExtensionStrategyBase() noexcept {
        static_assert(concepts::SimdExtensionStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::hardware::axis_09b_simd_extension
