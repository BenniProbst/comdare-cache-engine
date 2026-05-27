#pragma once
// V41.F.6.1.R7.5.j axis_09b Avx512Extension (x86_64 Skylake-X+, 512-bit)

#include "axis_09b_simd_extension_strategy_base.hpp"
#include "axis_09b_simd_extension_subaxes_se1_to_se3.hpp"
#include "concepts/axis_09b_simd_extension_cache_engine_permutation_concept.hpp"
#include "axis_09b_simd_extension_flags.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {

/// Avx512Extension — AVX-512 512-bit Vector-Extension (Skylake-X+, Ice Lake Server,
/// AMD Zen 4+). NICHT auf allen x86_64-CPUs (Haswell hat keinen AVX-512).
/// Optional Pflicht-Check: compatible_with_x86() = true, aber Subset von CPUs.
class Avx512Extension : public SimdExtensionStrategyBase<Avx512Extension> {
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
    [[nodiscard]] static constexpr std::string_view family_name()           noexcept { return "Avx512Extension (x86_64 Skylake-X+ 512-bit, Ice Lake Server/Zen 4+)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()           noexcept { return "AVX512"; }
};

}  // namespace

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {
    static_assert(concepts::SimdExtensionStrategy<Avx512Extension>);
    static_assert(concepts::CacheEnginePermutationStrategy<Avx512Extension>);
}
