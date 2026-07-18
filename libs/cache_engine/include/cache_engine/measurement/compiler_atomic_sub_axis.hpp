// measurement/compiler_atomic_sub_axis.hpp -- atomic128 als dynamische Unter-Achse der Compiler-System-Haupt-Achse
// (INC-0, User-Ruling 2026-07-18: "die anderen Flags gehoeren auf die Compiler Achse der System-Achsen").
//
// SPIEGELBILDLICH zu opt_level unter compiler (optimization_level_sub_axis.hpp) und simd unter extension_hardware
// (simd_sub_axis.hpp):
//   compiler (Haupt-System-Achse)
//     -> atomic128 (Unter-Achse, parent_axis_label()=="compiler")
//        -> Optionen {no_cx16, cx16}  (NoCx16Option / Cx16Option)
//
// WARUM COMPILER-ACHSE, NICHT extension_hardware (INC-0-Fork, aufgeloest): -mcx16 schaltet dem Compiler das
// Emittieren von CMPXCHG16B/DCAS (128-bit lock-free CAS) frei. Syntaktisch ein -m<feature>-Flag wie -mavx2, ABER
// semantisch ANDERS: CMPXCHG16B ist BASELINE-x86-64 (auf jeder x86-64-CPU seit ~2004, KEIN __builtin_cpu_supports-
// Present-Check noetig), waehrend die extension_hardware-Achse (Q2-Ruling) OPTIONALE, ISA-praesenz-gepruefte
// Erweiterungen (AVX2/AVX512, spaeter GPU) traegt. -mcx16 ist also ein reines Compiler-Codegen-Flag -> Compiler-
// System-Achse (User-INC-0-Ruling verbatim). Freigabe-Prinzip: die Compiler-System-Achse GIBT -mcx16 frei, das
// snmalloc-Organ (Umbrella) SETZT es durch (ds/aba.h #error You must compile with -mcx16).
//
// Fluss (wie opt_level/simd): CompileFn-Flag + H-10-Sidecar-Provenienz; binary_id-NEUTRAL (system_config, steht nie
// in kCompositionAxisNames, golden==320 unberuehrt). ISA-gegated (nur x86_64). Metaprog: CRTP + Concept, static-
// dispatch, keine vtable.

#pragma once

#include <cache_engine/measurement/ceb_system_axis.hpp>

#include <array>
#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

/// atomic128 -- Unter-Achse der Compiler-Haupt-Achse. Jede Auspraegung liefert je Compiler-Dialekt das konkrete
/// Codegen-Flag, das 128-bit-CAS (CMPXCHG16B/DCAS) freischaltet. gcc/clang teilen -mcx16; MSVC braucht kein Flag
/// (emittiert cmpxchg16b ohnehin). Analog OptimizationLevelSubAxis (opt_level/compiler) + SimdSubAxis.
template <class Derived>
struct CompilerAtomicSubAxis : CebSystemAxis<Derived> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "atomic128"; }

    /// Deklarative Zugehoerigkeit: atomic128 haengt UNTER der Compiler-Haupt-Achse (INC-0).
    /// Single-Source fuer die Serialisierungs-/Permutations-Konvention "compiler.atomic128=...".
    [[nodiscard]] static constexpr std::string_view parent_axis_label() noexcept { return "compiler"; }

    /// Deklarative Auspraegungs-Kennung (Ordner-/Sidecar-Etikett).
    [[nodiscard]] static constexpr std::string_view atomic128_id() noexcept { return Derived::do_atomic128_id(); }

    /// Codegen-Flag je Compiler-Dialekt (leer = kein Flag, Ist-Verhalten byte-identisch). gcc/clang teilen -mcx16;
    /// MSVC braucht keins.
    [[nodiscard]] static constexpr std::string_view gcc_flag() noexcept { return Derived::do_gcc_flag(); }
    [[nodiscard]] static constexpr std::string_view clang_flag() noexcept { return Derived::do_clang_flag(); }
    [[nodiscard]] static constexpr std::string_view msvc_flag() noexcept { return Derived::do_msvc_flag(); }

protected:
    constexpr CompilerAtomicSubAxis() noexcept = default;
};

template <class A>
concept CompilerAtomicSubAxisConcept =
    CebSystemAxisConcept<A> && std::derived_from<A, CompilerAtomicSubAxis<A>> &&
    std::is_empty_v<CompilerAtomicSubAxis<A>> && (!std::is_polymorphic_v<CompilerAtomicSubAxis<A>>) && requires {
        { A::atomic128_id() } -> std::same_as<std::string_view>;
        { A::parent_axis_label() } -> std::same_as<std::string_view>;
        { A::gcc_flag() } -> std::same_as<std::string_view>;
        { A::clang_flag() } -> std::same_as<std::string_view>;
        { A::msvc_flag() } -> std::same_as<std::string_view>;
    };

// ── Die atomic128-Auspraegungs-Familie {no_cx16, cx16}. Jede eine leere CRTP-Struct (Design-Space-Vokabular);
//    der Planer/die XML waehlt+permutiert (ISA-gegated x86_64), nichts gepinnt. ──

/// Kein 128-bit-CAS-Flag (Default = Ist-Verhalten): der Compiler emittiert CMPXCHG16B nicht garantiert.
struct NoCx16Option final : CompilerAtomicSubAxis<NoCx16Option> {
    [[nodiscard]] static constexpr std::string_view do_atomic128_id() noexcept { return "no_cx16"; }
    [[nodiscard]] static constexpr std::string_view do_gcc_flag() noexcept { return ""; }
    [[nodiscard]] static constexpr std::string_view do_clang_flag() noexcept { return ""; }
    [[nodiscard]] static constexpr std::string_view do_msvc_flag() noexcept { return ""; }
};

/// -mcx16: schaltet CMPXCHG16B/DCAS frei (snmalloc-Voraussetzung, ds/aba.h). gcc/clang == -mcx16; MSVC braucht keins.
struct Cx16Option final : CompilerAtomicSubAxis<Cx16Option> {
    [[nodiscard]] static constexpr std::string_view do_atomic128_id() noexcept { return "cx16"; }
    [[nodiscard]] static constexpr std::string_view do_gcc_flag() noexcept { return "-mcx16"; }
    [[nodiscard]] static constexpr std::string_view do_clang_flag() noexcept { return "-mcx16"; }
    [[nodiscard]] static constexpr std::string_view do_msvc_flag() noexcept { return ""; }
};

/// CEB-Default-Auspraegung -- kein Flag (Ist-Verhalten byte-identisch). Beweglicher Startwert, KEIN Pin; das
/// snmalloc-Organ erzwingt cx16 im Umbrella-Bau (Freigabe-Prinzip), sonst waehlt die XML/der Planer.
using DefaultCompilerAtomicOption = NoCx16Option;

/// Single-Source der gueltigen atomic128-ids (analog kAllSimdIds/kAllOptLevelIds).
inline constexpr std::array<std::string_view, 2> kAllAtomic128Ids = {NoCx16Option::atomic128_id(),
                                                                     Cx16Option::atomic128_id()};

static_assert(CompilerAtomicSubAxisConcept<NoCx16Option>);
static_assert(CompilerAtomicSubAxisConcept<Cx16Option>);
static_assert(Cx16Option::do_axis_label() == std::string_view{"atomic128"});
static_assert(Cx16Option::parent_axis_label() == std::string_view{"compiler"});
static_assert(Cx16Option::gcc_flag() == std::string_view{"-mcx16"});
static_assert(Cx16Option::clang_flag() == std::string_view{"-mcx16"});
static_assert(NoCx16Option::gcc_flag().empty() && NoCx16Option::clang_flag().empty());
static_assert(DefaultCompilerAtomicOption::atomic128_id() == std::string_view{"no_cx16"});

} // namespace comdare::cache_engine::measurement
