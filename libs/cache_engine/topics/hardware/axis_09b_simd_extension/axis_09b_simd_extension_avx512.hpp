#pragma once
// V41.F.6.1.R7.5.j axis_09b Avx512SimdExtension (x86_64 Skylake-X+, 512-bit)

#include "axis_09b_simd_extension_strategy_base.hpp"
#include "axis_09b_simd_extension_subaxes_se1_to_se3.hpp"
#include "concepts/axis_09b_simd_extension_cache_engine_permutation_concept.hpp"
#include "axis_09b_simd_extension_flags.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {

/// Avx512SimdExtension — AVX-512 512-bit Vector-Extension (Skylake-X+, Ice Lake Server,
/// AMD Zen 4+). NICHT auf allen x86_64-CPUs (Haswell hat keinen AVX-512).
/// Optional Pflicht-Check: compatible_with_x86() = true, aber Subset von CPUs.
class Avx512SimdExtension : public SimdExtensionStrategyBase<Avx512SimdExtension> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::vector_width_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::avx512_enabled;

    [[nodiscard]] static constexpr bool             is_active()             noexcept { return true; }
    [[nodiscard]] static constexpr int              vector_width_bits()     noexcept { return 512; }
    [[nodiscard]] static constexpr bool             compatible_with_x86()   noexcept { return true; }
    [[nodiscard]] static constexpr bool             compatible_with_arm()   noexcept { return false; }
    [[nodiscard]] static constexpr bool             compatible_with_riscv() noexcept { return false; }
    [[nodiscard]] static constexpr bool             compatible_with_powerpc() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()                  noexcept { return "simd_ext_avx512"; }
    [[nodiscard]] static constexpr std::string_view family_name()           noexcept { return "Avx512SimdExtension (x86_64 Skylake-X+ 512-bit, Ice Lake Server/Zen 4+)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()           noexcept { return "AVX512"; }

    // ─── R7.7.b Vollstaendige SSE/AVX/AVX-512 Schichten (Server-Default) ────
    // Annahme: Skylake-X/Cascade Lake/Ice Lake Server/Zen 4 — Server-CPU mit
    // VNNI (DBMS-relevant), BW/DQ/VL/VPOPCNTDQ (Bitmap-Index-Beschleunigung).
    // Xeon Phi KNL-Sub-Flags (ER/PF) = false (irrelevant fuer DBMS).
    // BF16/FP16 = true ab Sapphire Rapids/Cooper Lake fuer ML-Workloads.
    [[nodiscard]] static constexpr bool provides_sse()              noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_sse2()             noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_sse3()             noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_ssse3()            noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_sse4_1()           noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_sse4_2()           noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_avx()              noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_avx2()             noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_avx512f()          noexcept { return true; }   // Pflicht-Basis
    [[nodiscard]] static constexpr bool provides_avx512cd()         noexcept { return true; }   // Conflict Detection
    // ER+PF nur Xeon Phi KNL — default false (Defaults aus Base)
    [[nodiscard]] static constexpr bool provides_avx512bw()         noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_avx512dq()         noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_avx512vl()         noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_avx512ifma()       noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_avx512vbmi()       noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_avx512vbmi2()      noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_avx512vnni()       noexcept { return true; }   // DBMS Vector-Indexes
    [[nodiscard]] static constexpr bool provides_avx512bitalg()     noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_avx512vpopcntdq()  noexcept { return true; }   // Bitmap-Indexes
    [[nodiscard]] static constexpr bool provides_avx512bf16()       noexcept { return true; }   // Cooper Lake+ / Sapphire Rapids+
    [[nodiscard]] static constexpr bool provides_avx512fp16()       noexcept { return true; }   // Sapphire Rapids+

    // ─── R7.7.c Topologie: 1 AVX-512 Unit/Sockel, P-Core-only (Hybrid) ──────
    // KRITISCH: Intel Alder Lake/Raptor Lake/Meteor Lake disabled AVX-512 in BIOS
    // wegen E-Cores die kein AVX-512 unterstuetzen. AMD Zen 4 hat AVX-512 in
    // allen Cores. Konservativer Default: false (sicher fuer Hybrid).
    [[nodiscard]] static constexpr int  units_per_socket()                  noexcept { return 1; }
    [[nodiscard]] static constexpr bool accessible_from_efficiency_cores() noexcept { return false; }
};

}  // namespace

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {
    static_assert(concepts::SimdExtensionStrategy<Avx512SimdExtension>);
    static_assert(concepts::CacheEnginePermutationStrategy<Avx512SimdExtension>);
}
