#pragma once
// V41.F.6.1.R7.5.j axis_09b Sse2SimdExtension (x86_64-only, 128-bit baseline)

#include "axis_09b_simd_extension_strategy_base.hpp"
#include "axis_09b_simd_extension_subaxes_se1_to_se3.hpp"
#include "concepts/axis_09b_simd_extension_cache_engine_permutation_concept.hpp"
#include "axis_09b_simd_extension_flags.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {

/// Sse2SimdExtension — SSE2 128-bit Vector-Extension (x86_64 ABI-baseline seit 2003).
/// NUR mit Amd64Isa kompatibel. Pflicht-Teil der x86_64 ABI.
class Sse2SimdExtension : public SimdExtensionStrategyBase<Sse2SimdExtension> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::compat_family_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::sse2_enabled;

    [[nodiscard]] static constexpr bool             is_active() noexcept { return true; }
    [[nodiscard]] static constexpr int              vector_width_bits() noexcept { return 128; }
    [[nodiscard]] static constexpr bool             compatible_with_x86() noexcept { return true; }
    [[nodiscard]] static constexpr bool             compatible_with_arm() noexcept { return false; }
    [[nodiscard]] static constexpr bool             compatible_with_riscv() noexcept { return false; }
    [[nodiscard]] static constexpr bool             compatible_with_powerpc() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "simd_ext_sse2"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "Sse2SimdExtension (x86_64 ABI-baseline 128-bit, nur Amd64-compat)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "SSE2"; }

    // ─── R7.7.b SSE-Schichten (Baseline: nur SSE+SSE2) ───────────────────────
    [[nodiscard]] static constexpr bool provides_sse() noexcept { return true; }
    [[nodiscard]] static constexpr bool provides_sse2() noexcept { return true; }
    // SSE3+ false (Defaults aus Base) — explizit ABI-baseline ist nur SSE2

    // ─── R7.7.c Topologie: typisch 2 SSE-Units/Sockel, alle Cores (auch E) ──
    [[nodiscard]] static constexpr int  units_per_socket() noexcept { return 2; }
    [[nodiscard]] static constexpr bool accessible_from_efficiency_cores() noexcept { return true; }
};

} // namespace comdare::cache_engine::hardware::axis_09b_simd_extension

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {
static_assert(concepts::SimdExtensionStrategy<Sse2SimdExtension>);
static_assert(concepts::CacheEnginePermutationStrategy<Sse2SimdExtension>);
} // namespace comdare::cache_engine::hardware::axis_09b_simd_extension
