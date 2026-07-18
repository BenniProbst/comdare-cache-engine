// measurement/extension_hardware_system_axis.hpp -- Erweiterungshardware als 6. CEB-Konfig-
// System-Achse (Bau-INC-1d, Q2-Ruling Option C, 2026-07-17).
//
// DEPRECATED (F-SIMD, 2026-07-18): abgeloest durch measurement/simd_sub_axis.hpp. Dieses FLACHE Modell trug die
// simd-Auspraegungen (no_extension/avx2/avx512) DIREKT an der extension_hardware-Haupt-Achse (asymmetrisch zu
// compiler->opt_level->option). Die konforme Struktur ist jetzt extension_hardware (Haupt) -> simd (SimdSubAxis,
// Unter-Achse) -> Optionen (SimdNoExt/Avx2/Avx512Option), spiegelbildlich zu OptimizationLevelSubAxis. KEINE
// aktiven Consumer mehr (Parser/opt-g-Facade/Validate nutzen SimdSubAxis); NUR test_striktheit_axis_dach_guard
// Block F haelt die Typen als Kontrast kompilierbar. NICHT LOESCHEN (Doku-nie-loeschen + Aufraeumen-unter-
// Absprache) -- endgueltige Entfernung ist user-gated.
//
// User-Ruling verbatim-treu: die CEB bekommt die Einstellungen vom Experiment-Planer, permutiert
// die simd_extension-Auspraegungen aber SELBST zu ihrer Laufzeit durch und stattet Tier-Binaries
// zur compile time mit diesen Eigenschaften aus (Erweiterungshardware = SIMD, spaeter GPU).
// Der -march/-mavx-WERT kommt aus dieser Achse, der ORT ist die injizierte CompileFn-Naht
// (make_gpp_compile_fn-Flag-Kanal), die PROVENIENZ gehoert ins H-10-Sidecar — NIE in die
// binary_id (golden==320 bleibt; 09b/simd_extension bleibt die deklarative Build-Achse).
// Die Flags werden JE COMPILER getrennt gefuehrt (gcc|clang, Q3): -march=native-Defaults
// differieren je Compiler und sind eine Quelle des Compiler-Vergleichs.

#pragma once

#include <cache_engine/measurement/ceb_system_axis.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

template <class Derived>
struct ExtensionHardwareSystemAxis : CebSystemAxis<Derived> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "extension_hardware"; }

    /// Deklarative Auspraegungs-Kennung (deckungsgleich zur 09b-simd_extension-Nomenklatur).
    [[nodiscard]] static constexpr std::string_view simd_extension_id() noexcept {
        return Derived::do_simd_extension_id();
    }
    /// Compile-Flags je Compiler-Dialekt (leer = generisches -O2-Binary, Ist-Verhalten).
    [[nodiscard]] static constexpr std::string_view gcc_march_flag() noexcept { return Derived::do_gcc_march_flag(); }
    [[nodiscard]] static constexpr std::string_view clang_march_flag() noexcept {
        return Derived::do_clang_march_flag();
    }

protected:
    constexpr ExtensionHardwareSystemAxis() noexcept = default;
};

template <class A>
concept ExtensionHardwareSystemAxisConcept =
    CebSystemAxisConcept<A> && std::derived_from<A, ExtensionHardwareSystemAxis<A>> &&
    std::is_empty_v<ExtensionHardwareSystemAxis<A>> && (!std::is_polymorphic_v<ExtensionHardwareSystemAxis<A>>) &&
    requires {
        { A::simd_extension_id() } -> std::same_as<std::string_view>;
        { A::gcc_march_flag() } -> std::same_as<std::string_view>;
        { A::clang_march_flag() } -> std::same_as<std::string_view>;
    };

/// Generisch (Default = Ist-Verhalten): KEINE Erweiterungs-Flags, Binary bleibt -O2-generisch.
struct GenericExtensionHardwareAxis final : ExtensionHardwareSystemAxis<GenericExtensionHardwareAxis> {
    [[nodiscard]] static constexpr std::string_view do_simd_extension_id() noexcept { return "no_extension"; }
    [[nodiscard]] static constexpr std::string_view do_gcc_march_flag() noexcept { return ""; }
    [[nodiscard]] static constexpr std::string_view do_clang_march_flag() noexcept { return ""; }
};

/// AVX2 einkompiliert (H-7: "speziell passend ... einkompiliert"). Flag-Werte deckungsgleich
/// zur codegen-Praezedenz (permutation_codegen_tool.cpp simd_flags / codegen.cmake).
struct Avx2ExtensionHardwareAxis final : ExtensionHardwareSystemAxis<Avx2ExtensionHardwareAxis> {
    [[nodiscard]] static constexpr std::string_view do_simd_extension_id() noexcept { return "avx2"; }
    [[nodiscard]] static constexpr std::string_view do_gcc_march_flag() noexcept { return "-mavx2"; }
    [[nodiscard]] static constexpr std::string_view do_clang_march_flag() noexcept { return "-mavx2"; }
};

/// AVX-512 einkompiliert ODER (via Generic) bewusst weggelassen -> H-7 "volle Kontrolle".
struct Avx512ExtensionHardwareAxis final : ExtensionHardwareSystemAxis<Avx512ExtensionHardwareAxis> {
    [[nodiscard]] static constexpr std::string_view do_simd_extension_id() noexcept { return "avx512"; }
    [[nodiscard]] static constexpr std::string_view do_gcc_march_flag() noexcept { return "-mavx512f"; }
    [[nodiscard]] static constexpr std::string_view do_clang_march_flag() noexcept { return "-mavx512f"; }
};

static_assert(ExtensionHardwareSystemAxisConcept<GenericExtensionHardwareAxis>);
static_assert(ExtensionHardwareSystemAxisConcept<Avx2ExtensionHardwareAxis>);
static_assert(ExtensionHardwareSystemAxisConcept<Avx512ExtensionHardwareAxis>);

} // namespace comdare::cache_engine::measurement
