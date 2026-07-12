#pragma once
// GO-3 A1 (Task #5 Hebel-A-Rest, 2026-07-12) — axis_09b Build-Kohaerenz-Guard: haertet die je Binary
// DEKLARIERTE simd_extension-Achse (POD simd_width_bits via build_variant_definition.hpp) compile-time gegen
// die REAL kompilierte ISA-Stufe (Compiler-Makros __AVX512F__/__AVX2__/SSE2-Baseline/__ARM_NEON). Vorher
// konnte eine Binary Avx2SimdExtension deklarieren (simd_width_bits=256), waehrend ihr Maschinencode mangels
// -mavx2 den SSE2-Pfad fuhr (axis_09_isa_amd64.hpp-Dispatch) — Mess-Etikett != Mess-Gegenstand (Dossier GO3
// §2.3). Konsumiert vom ADDITIVEN Makro COMDARE_DEFINE_BUILD_VARIANT_INSPECTION_CHECKED
// (include/cache_engine/abi/build_variant_inspection.hpp); die passende Compiler-Flag setzt
// comdare_apply_simd_extension_flags(<target> <EXT>) in cmake/isa_features.cmake.
//
// Benanntes Muster: Meta-driven Concept Hardening ([[reference_meta_driven_concept_hardening_pattern]]) —
// eine Deklaration wird per consteval-Praedikat gegen den Ist-Build gehaertet. Reine compile-time-Mechanik
// (consteval + static_assert), kein Runtime-Switch.
//
// BEWUSST SELBSTGENUEGSAM (nur <concepts>/<string_view>): KEIN Include der 8 Extension-Klassen und KEIN
// generierter flags.hpp-Header — so bleibt build_variant_inspection.hpp fuer die leichten, boost-freien
// Stub-DLL-Targets inkludierbar. Die 512/256/128-Kaskade ist hier EINMAL zentralisiert (Dossier-GO3-Risiko 1);
// sie spiegelt exakt den Kernel-Dispatch von Amd64Isa::simd_field_sum (axis_09_isa_amd64.hpp) und die
// lane_width_-Kaskade der Mess-Huelle (axis_09_isa_observable.hpp). Folge-Refactor (separater Slice,
// T12-Messzeile -> Doppel-Kartierung): lane_width_() aus dieser Funktion ableiten.

#include <concepts>
#include <string_view>

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {

/// Die REALE Build-ISA-Stufe DIESER Uebersetzungseinheit in Vektor-Bits — exakt die Dispatch-Kaskade von
/// Amd64Isa::simd_field_sum: __AVX512F__ -> 512, __AVX2__ -> 256, x86-64 -> 128 (SSE2 = ABI-Baseline),
/// __ARM_NEON -> 128 (aarch64-Baseline), sonst 0 (kein natives SIMD im Build).
[[nodiscard]] consteval int actual_build_simd_width_bits() noexcept {
#if defined(__AVX512F__)
    return 512;
#elif defined(__AVX2__)
    return 256;
#elif defined(__x86_64__) || defined(_M_X64)
    return 128; // SSE2 ist Pflicht-Teil der x86-64-ABI (immer vorhanden)
#elif defined(__ARM_NEON)
    return 128; // NEON ist aarch64-Baseline
#else
    return 0;
#endif
}

[[nodiscard]] consteval bool build_targets_x86_64() noexcept {
#if defined(__x86_64__) || defined(_M_X64)
    return true;
#else
    return false;
#endif
}

[[nodiscard]] consteval bool build_targets_arm() noexcept {
#if defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM_NEON)
    return true;
#else
    return false;
#endif
}

[[nodiscard]] consteval bool build_targets_riscv() noexcept {
#if defined(__riscv)
    return true;
#else
    return false;
#endif
}

/// Skalierbare/Nicht-Vektor-Extensions (vector_width_bits()==-1): Wahrheit ueber das jeweilige
/// Feature-Makro statt ueber die feste Breiten-Kaskade. CUDA_GH200 kann KEIN Host-CPU-Build wahr machen
/// -> immer false (die CMake-Seite bricht dort mit FATAL_ERROR + #276-Verweis ab).
[[nodiscard]] consteval bool scalable_extension_matches_build(std::string_view flag_suffix) noexcept {
    if (flag_suffix == std::string_view{"SVE2"}) {
#if defined(__ARM_FEATURE_SVE2)
        return true;
#else
        return false;
#endif
    }
    if (flag_suffix == std::string_view{"RVV"}) {
#if defined(__riscv_vector)
        return true;
#else
        return false;
#endif
    }
    return false;
}

/// Concept-Haertung: die ECHTE axis_09b-Wrapper-API, die der Kohaerenz-Guard konsumiert. BEWUSST KEINE
/// Stub-Toleranz — das CHECKED-Makro ist fuer die realen Wrapper; die real-geformten Stubs bleiben beim
/// Legacy-Makro (POD-Roundtrip-Beweis, s. tests/unit/genus_buildvariant_{avx2,avx512}.cpp).
template <class SE>
concept BuildCoherenceCheckableSimdExtension = requires {
    { SE::is_active() } noexcept -> std::convertible_to<bool>;
    { SE::vector_width_bits() } noexcept -> std::convertible_to<int>;
    { SE::compatible_with_x86() } noexcept -> std::convertible_to<bool>;
    { SE::compatible_with_arm() } noexcept -> std::convertible_to<bool>;
    { SE::compatible_with_riscv() } noexcept -> std::convertible_to<bool>;
    { SE::flag_suffix() } noexcept -> std::convertible_to<std::string_view>;
};

/// declared_extension_matches_build<SE>() — true gdw. die DEKLARIERTE Extension zur REALEN Build-ISA-Stufe
/// passt:
///  * NoSimdExtension (is_active()==false): EXEMPT — sie deklariert Nicht-NUTZUNG, nicht Nicht-Existenz
///    (die x86-64-ABI erzwingt SSE2; ein NoSimd-Etikett bleibt auf SIMD-Baseline-Plattformen wahrhaftig).
///  * Feste Breiten (SSE2/AVX2/AVX512/NEON): Plattform-Kompatibilitaet UND EXAKTE Breiten-Gleichheit mit
///    der Kaskade — Avx2 auf einem AVX-512-Build ist ebenso INKOHAERENT wie auf einem SSE2-Build (das
///    Etikett muss die Stufe treffen, nicht nur eine Untermenge davon).
///  * Skalierbar (SVE2/RVV, width==-1): Feature-Makro-Check; CUDA_GH200: nie wahr im Host-CPU-Build.
template <class SE>
    requires BuildCoherenceCheckableSimdExtension<SE>
[[nodiscard]] consteval bool declared_extension_matches_build() noexcept {
    if constexpr (!SE::is_active()) {
        return true;
    } else if constexpr (SE::vector_width_bits() < 0) {
        return scalable_extension_matches_build(SE::flag_suffix());
    } else {
        constexpr bool platform_ok = (build_targets_x86_64() && SE::compatible_with_x86()) ||
                                     (build_targets_arm() && SE::compatible_with_arm()) ||
                                     (build_targets_riscv() && SE::compatible_with_riscv());
        return platform_ok && (SE::vector_width_bits() == actual_build_simd_width_bits());
    }
}

} // namespace comdare::cache_engine::hardware::axis_09b_simd_extension
