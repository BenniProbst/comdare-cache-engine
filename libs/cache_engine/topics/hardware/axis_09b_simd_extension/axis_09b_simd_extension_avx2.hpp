#pragma once
// V41.F.6.1.R7.5.j axis_09b Avx2SimdExtension (x86_64 Haswell+, 256-bit)

#include "axis_09b_simd_extension_strategy_base.hpp"
#include "axis_09b_simd_extension_subaxes_se1_to_se3.hpp"
#include "concepts/axis_09b_simd_extension_cache_engine_permutation_concept.hpp"
#include "axis_09b_simd_extension_flags.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {

/// Avx2SimdExtension — AVX2 256-bit Vector-Extension (Haswell+ 2013).
/// ZIH-Cluster Barnard (AMD EPYC Zen 3 = AVX2 native), Capella + Alpha Centauri
/// (Intel Xeon). Default Optimierungs-Stufe fuer Server-DBMS auf x86_64.
class Avx2SimdExtension : public SimdExtensionStrategyBase<Avx2SimdExtension> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::vector_width_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::avx2_enabled;

    [[nodiscard]] static constexpr bool             is_active()             noexcept { return true; }
    [[nodiscard]] static constexpr int              vector_width_bits()     noexcept { return 256; }
    [[nodiscard]] static constexpr bool             compatible_with_x86()   noexcept { return true; }
    [[nodiscard]] static constexpr bool             compatible_with_arm()   noexcept { return false; }
    [[nodiscard]] static constexpr bool             compatible_with_riscv() noexcept { return false; }
    [[nodiscard]] static constexpr bool             compatible_with_powerpc() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()                  noexcept { return "simd_ext_avx2"; }
    [[nodiscard]] static constexpr std::string_view family_name()           noexcept { return "Avx2SimdExtension (x86_64 Haswell+ 256-bit, ZIH Barnard/Capella default)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()           noexcept { return "AVX2"; }

    // ─── R7.7.b SSE+AVX-Schichten (rueckwaerts-kumulativ Haswell+ 2013) ──────
    [[nodiscard]] static constexpr bool provides_sse()              noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_sse2()             noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_sse3()             noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_ssse3()            noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_sse4_1()           noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_sse4_2()           noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_avx()              noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_avx2()             noexcept { return true; }
    // AVX-512 false (Defaults)

    // ─── R7.7.c Topologie: 2 AVX2-Units/Sockel, alle Cores ───────────────────
    [[nodiscard]] static constexpr int  units_per_socket()                  noexcept { return 2; }
    [[nodiscard]] static constexpr bool accessible_from_efficiency_cores() noexcept { return true; }
};

}  // namespace

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {
    static_assert(concepts::SimdExtensionStrategy<Avx2SimdExtension>);
    static_assert(concepts::CacheEnginePermutationStrategy<Avx2SimdExtension>);
}
