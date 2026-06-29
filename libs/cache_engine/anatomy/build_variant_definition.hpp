#pragma once
// D7 / L-74b (2026-06-02) — BuildVariantDefinitionV1: der je-Knoten-getragene, ABI-stabile DEFINITIONS-POD der
// 3 Build-Achsen page_type(01) / simd_extension(09b) / general_hardware(12). Doc 27 §3: diese Achsen sind reine
// Build-Time-Konstanten (alle Properties static constexpr) → KEIN Laufzeit-Observer (der beobachtete nur eine
// invariante Konstante = Etikettenschwindel), sondern „Achsen-Definition statt Observer" — EXPLIZIT als realer,
// flacher POD je Knoten getragen + durch die ABI-Grenze ziehbar (statt nur einer statischen Klassifikations-Tabelle).
//
// Das ist die EHRLICHE Auflösung von #74-L-74b: aus DefinitionOnly-Etikett wird eine reale, abrufbare Definition.
// Build-Varianten-Emit + Inspection-Symbole = L-74a; R5.B-Achsen-Operativität = L-74c (getrennt). Nur uint64 → ABI-fest.

#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

/// Flacher, ABI-stabiler Definitions-POD der 3 Build-Achsen (cross-boundary, nur uint64 → standard_layout).
struct BuildVariantDefinitionV1 {
    // ── page_type (axis_01) ──
    std::uint64_t page_kind      = 0; // PageKind als uint64 (DenseByte=0 … BPlus=5)
    std::uint64_t page_is_branch = 0; // 0/1
    std::uint64_t page_is_leaf   = 0; // 0/1
    // ── simd_extension (axis_09b) ──
    std::uint64_t simd_width_bits = 0; // vector_width_bits() (z.B. 128/256/512)
    std::uint64_t simd_avx512     = 0; // provides_avx512() 0/1
    // ── general_hardware (axis_12) ──
    std::uint64_t hw_cache_line   = 0; // cache_line_size() (z.B. 64)
    std::uint64_t hw_numa_capable = 0; // 0/1
    // ── present-Bits: welche der 3 Build-Achsen sind in dieser Definition belegt ──
    std::uint64_t present_mask = 0; // Bit0=page_type, Bit1=simd, Bit2=hw

    [[nodiscard]] constexpr bool operator==(BuildVariantDefinitionV1 const&) const noexcept = default;
};
static_assert(std::is_standard_layout_v<BuildVariantDefinitionV1>);
static_assert(std::is_trivially_copyable_v<BuildVariantDefinitionV1>);

inline constexpr std::uint64_t kBuildPagePresent              = 1;
inline constexpr std::uint64_t kBuildSimdPresent              = 2;
inline constexpr std::uint64_t kBuildHwPresent                = 4;
inline constexpr std::uint32_t kBuildVariantDefinitionVersion = 1;

namespace detail {
/// detect_avx512<SE>() — AVX-512-Fähigkeit tolerant aus der simd_extension-Achse ableiten, kompatibel sowohl zum
/// schlanken Stub-API (`provides_avx512()`) als auch zur ECHTEN axis_09b-Wrapper-API (`provides_avx512f()` —
/// Avx512SimdExtension hat KEIN provides_avx512(), sondern die granularen provides_avx512f/cd/bw/… Flags). Fallback:
/// Breite >= 512 bit. So füllt build_variant_definition den POD aus den realen Wrappern OHNE Achsen-Änderung.
template <class SE>
[[nodiscard]] constexpr std::uint64_t detect_avx512() noexcept {
    if constexpr (requires { SE::provides_avx512(); })
        return SE::provides_avx512() ? 1u : 0u; // Stub-/Legacy-API
    else if constexpr (requires { SE::provides_avx512f(); })
        return SE::provides_avx512f() ? 1u : 0u; // echte axis_09b-API
    else
        return SE::vector_width_bits() >= 512u ? 1u : 0u; // Fallback
}
} // namespace detail

/// build_variant_definition<PT,SE,HW>() — compile-time Reader: füllt den POD aus den static-constexpr-Properties
/// der 3 Build-Achsen (KEIN Treiben — reine Definition). PT/SE/HW erfüllen die page_type/simd/hw-Property-API
/// (Stub ODER echter Wrapper — die simd-avx512-Erkennung ist via detail::detect_avx512 tolerant).
template <class PT, class SE, class HW>
[[nodiscard]] constexpr BuildVariantDefinitionV1 build_variant_definition() noexcept {
    BuildVariantDefinitionV1 v{};
    v.page_kind       = static_cast<std::uint64_t>(PT::page_kind());
    v.page_is_branch  = PT::is_branch() ? 1u : 0u;
    v.page_is_leaf    = PT::is_leaf() ? 1u : 0u;
    v.simd_width_bits = static_cast<std::uint64_t>(SE::vector_width_bits());
    v.simd_avx512     = detail::detect_avx512<SE>();
    v.hw_cache_line   = static_cast<std::uint64_t>(HW::cache_line_size());
    v.hw_numa_capable = HW::numa_capable() ? 1u : 0u;
    v.present_mask    = kBuildPagePresent | kBuildSimdPresent | kBuildHwPresent;
    return v;
}

} // namespace comdare::cache_engine::anatomy
