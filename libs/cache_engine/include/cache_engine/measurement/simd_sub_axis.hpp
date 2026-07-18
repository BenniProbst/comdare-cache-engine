// measurement/simd_sub_axis.hpp -- simd als dynamische Unter-Achse der Erweiterungs-Hardware-System-Haupt-Achse
// (F-SIMD, User-Ruling 2026-07-18: "F-SIMD strikt nach Plan-Empfehlung SYMMETRISCH").
//
// SPIEGELBILDLICH zu opt_level unter compiler (optimization_level_sub_axis.hpp, das strikte Vorbild):
//   extension_hardware (Haupt-System-Achse, 6., Q2 Option C)
//     -> simd (Unter-Achse, parent_axis_label()=="extension_hardware")
//        -> Optionen {no_extension, avx2, avx512}  (SimdNoExtOption / SimdAvx2Option / SimdAvx512Option)
//
// Vorher hingen no_extension/avx2/avx512 DIREKT an der extension_hardware-Haupt-Achse (asymmetrisch zu
// compiler->opt_level->option; Konformitaets-Sweep-Befund F-SIMD). Diese Datei fuehrt die simd-UNTER-Achse ein,
// deren Auspraegungen die simd-Optionen sind -- genau die Ebenen-Symmetrie compiler/opt_level ↔ extension_hardware/simd.
//
// Fluss (wie opt_level / das Vorbild extension_hardware): CompileFn-Flag (-march) + H-10-Sidecar-Provenienz
// (build_version "+ext="); binary_id-NEUTRAL (system_config, steht nie in kCompositionAxisNames, golden==320
// unberuehrt). Die -march/-mavx-Werte sind deckungsgleich zur codegen-Praezedenz (permutation_codegen_tool
// simd_flags). ISA-gegated (nur wenn die Ziel-ISA sie bietet). Metaprog: CRTP + Concept, static-dispatch, keine vtable.

#pragma once

#include <cache_engine/measurement/ceb_system_axis.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

/// simd -- Unter-Achse der Erweiterungs-Hardware-Haupt-Achse. Jede Auspraegung liefert je Compiler-Dialekt das
/// konkrete -march-Flag (gcc/clang teilen -mavx2/-mavx512f). Analog OptimizationLevelSubAxis (opt_level/compiler).
template <class Derived>
struct SimdSubAxis : CebSystemAxis<Derived> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "simd"; }

    /// Deklarative Zugehoerigkeit: simd haengt UNTER der Erweiterungs-Hardware-Haupt-Achse.
    /// Single-Source fuer die Serialisierungs-/Permutations-Konvention "extension_hardware.simd=...".
    [[nodiscard]] static constexpr std::string_view parent_axis_label() noexcept { return "extension_hardware"; }

    /// Deklarative Auspraegungs-Kennung (Ordner-/Sidecar-Etikett; deckungsgleich zur 09b-simd_extension-Nomenklatur).
    [[nodiscard]] static constexpr std::string_view simd_id() noexcept { return Derived::do_simd_id(); }

    /// Compile-Flags je Compiler-Dialekt (leer = generisch, Ist-Verhalten byte-identisch).
    [[nodiscard]] static constexpr std::string_view gcc_march_flag() noexcept { return Derived::do_gcc_march_flag(); }
    [[nodiscard]] static constexpr std::string_view clang_march_flag() noexcept {
        return Derived::do_clang_march_flag();
    }

protected:
    constexpr SimdSubAxis() noexcept = default;
};

template <class A>
concept SimdSubAxisConcept = CebSystemAxisConcept<A> && std::derived_from<A, SimdSubAxis<A>> &&
                             std::is_empty_v<SimdSubAxis<A>> && (!std::is_polymorphic_v<SimdSubAxis<A>>) && requires {
                                 { A::simd_id() } -> std::same_as<std::string_view>;
                                 { A::parent_axis_label() } -> std::same_as<std::string_view>;
                                 { A::gcc_march_flag() } -> std::same_as<std::string_view>;
                                 { A::clang_march_flag() } -> std::same_as<std::string_view>;
                             };

// ── Die simd-Auspraegungs-Familie {no_extension, avx2, avx512}. Jede eine leere CRTP-Struct (Design-Space-
//    Vokabular); der Planer/die XML waehlt+permutiert (ISA-gegated), nichts gepinnt. ──

/// Generisch (Default = Ist-Verhalten): KEINE Erweiterungs-Flags, Binary bleibt generisch.
struct SimdNoExtOption final : SimdSubAxis<SimdNoExtOption> {
    [[nodiscard]] static constexpr std::string_view do_simd_id() noexcept { return "no_extension"; }
    [[nodiscard]] static constexpr std::string_view do_gcc_march_flag() noexcept { return ""; }
    [[nodiscard]] static constexpr std::string_view do_clang_march_flag() noexcept { return ""; }
};

struct SimdAvx2Option final : SimdSubAxis<SimdAvx2Option> {
    [[nodiscard]] static constexpr std::string_view do_simd_id() noexcept { return "avx2"; }
    [[nodiscard]] static constexpr std::string_view do_gcc_march_flag() noexcept { return "-mavx2"; }
    [[nodiscard]] static constexpr std::string_view do_clang_march_flag() noexcept { return "-mavx2"; }
};

struct SimdAvx512Option final : SimdSubAxis<SimdAvx512Option> {
    [[nodiscard]] static constexpr std::string_view do_simd_id() noexcept { return "avx512"; }
    [[nodiscard]] static constexpr std::string_view do_gcc_march_flag() noexcept { return "-mavx512f"; }
    [[nodiscard]] static constexpr std::string_view do_clang_march_flag() noexcept { return "-mavx512f"; }
};

/// CEB-Default-Auspraegung -- generisch (kein -march); Ist-Verhalten der Mess-DLLs byte-identisch. Beweglicher
/// Startwert, KEIN Pin (jede Auspraegung per XML/Planer waehlbar).
using DefaultSimdOption = SimdNoExtOption;

static_assert(SimdSubAxisConcept<SimdNoExtOption>);
static_assert(SimdSubAxisConcept<SimdAvx2Option>);
static_assert(SimdSubAxisConcept<SimdAvx512Option>);
static_assert(SimdNoExtOption::do_axis_label() == std::string_view{"simd"});
static_assert(SimdAvx2Option::parent_axis_label() == std::string_view{"extension_hardware"});
// Flag-Deckung zur codegen-Praezedenz (permutation_codegen_tool simd_flags: avx512->-mavx512f, avx2->-mavx2):
static_assert(SimdAvx2Option::gcc_march_flag() == std::string_view{"-mavx2"});
static_assert(SimdAvx512Option::gcc_march_flag() == std::string_view{"-mavx512f"});
static_assert(SimdNoExtOption::gcc_march_flag().empty() && SimdNoExtOption::clang_march_flag().empty());
static_assert(DefaultSimdOption::simd_id() == std::string_view{"no_extension"});

} // namespace comdare::cache_engine::measurement
