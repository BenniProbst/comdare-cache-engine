// measurement/optimization_level_sub_axis.hpp -- opt_level als dynamische Unter-Achse der
// Compiler-System-Haupt-Achse (Bau-INC-2c.opt-a, OF-1/2/3-Rulings 2026-07-17).
//
// User-Entscheid (OF-1/2/3): Die Compiler-System-Haupt-Achse traegt PARALLELE dynamische
// XML-Unterachsen -- opt_level (voll {O0,O1,O2,O3,Ofast}), flags/cflags, commands, CPU-SIMD.
// opt_level ist KEINE Geschwister-System-Achse, sondern eine Unter-Achse UNTER compiler
// (parent_axis_label()=="compiler"). Der Experiment-Planer permutiert compiler x opt_level x ...
// zu verschiedenen Tier-Binaries; alles per XML in ranges/batches/einzeln konfigurierbar,
// NICHTS gepinnt, JEDES TEIL beweglich. CEB-Default = O3 (IEEE-754-deterministisch, Option B, Ruling
// 2026-07-18), per env/XML/Planer ueberschreibbar; Ofast/O0/O1/O2 additiv als Vergleichs-Extreme.
//
// Fluss wie extension_hardware (das strikte Vorbild): CompileFn-Flag (-O<n>) + H-10-Sidecar-
// Provenienz (build_version "+opt="); binary_id-NEUTRAL (steht nie in kCompositionAxisNames,
// golden==320 unberuehrt). Metaprogrammierung: CRTP + Concept, static-dispatch, keine vtable.
//
// WARNUNG (Deep-Research): -Ofast = -O3 + -ffast-math-Familie + -fallow-store-data-races +
// -funsafe-math-optimizations -> bricht IEEE-754 UND Run-to-Run-Determinismus. Fuer das
// permutierende Experiment (Vergleich/Debug) zulaessig; die Wahl je Mess-Reihe trifft die XML
// (OF-2: jedes Teil beweglich).

#pragma once

#include <cache_engine/measurement/ceb_system_axis.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

/// opt_level -- Unter-Achse der Compiler-Haupt-Achse. Jede Auspraegung liefert je Compiler-Dialekt
/// das konkrete Optimierungs-Flag (gcc/clang teilen -O<n>; MSVC nutzt /O<n>). Die Flag-Werte sind
/// deckungsgleich zur axis_library_registry-Achse 15.2 (Vokabular-Fundus, nicht wiederbelebter Pfad).
template <class Derived>
struct OptimizationLevelSubAxis : CebSystemAxis<Derived> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "opt_level"; }

    /// Deklarative Zugehoerigkeit: opt_level haengt UNTER der Compiler-Haupt-Achse (OF-1).
    /// Single-Source fuer die Serialisierungs-/Permutations-Konvention "compiler.opt_level=...".
    [[nodiscard]] static constexpr std::string_view parent_axis_label() noexcept { return "compiler"; }

    /// Deklarative Stufen-Kennung (Ordner-/Sidecar-Etikett), deckungsgleich zur 15.2-Nomenklatur.
    [[nodiscard]] static constexpr std::string_view opt_level_id() noexcept { return Derived::do_opt_level_id(); }

    /// Optimierungs-Flag je Compiler-Dialekt. gcc/clang teilen die -O<n>-Schreibweise; MSVC weicht ab.
    [[nodiscard]] static constexpr std::string_view gcc_opt_flag() noexcept { return Derived::do_gcc_opt_flag(); }
    [[nodiscard]] static constexpr std::string_view clang_opt_flag() noexcept { return Derived::do_clang_opt_flag(); }
    [[nodiscard]] static constexpr std::string_view msvc_opt_flag() noexcept { return Derived::do_msvc_opt_flag(); }

    /// Determinismus-/IEEE-754-Ehrlichkeit: true nur, wenn die Stufe die numerische Semantik + die
    /// Run-to-Run-Reproduzierbarkeit erhaelt (alle ausser Ofast). Der Planer/die XML kann damit
    /// mess-deterministische Reihen von aggressiven Vergleichs-Reihen trennen (OF-2).
    [[nodiscard]] static constexpr bool is_ieee754_deterministic() noexcept {
        return Derived::do_is_ieee754_deterministic();
    }

protected:
    constexpr OptimizationLevelSubAxis() noexcept = default;
};

template <class A>
concept OptimizationLevelSubAxisConcept =
    CebSystemAxisConcept<A> && std::derived_from<A, OptimizationLevelSubAxis<A>> &&
    std::is_empty_v<OptimizationLevelSubAxis<A>> && (!std::is_polymorphic_v<OptimizationLevelSubAxis<A>>) && requires {
        { A::opt_level_id() } -> std::same_as<std::string_view>;
        { A::parent_axis_label() } -> std::same_as<std::string_view>;
        { A::gcc_opt_flag() } -> std::same_as<std::string_view>;
        { A::clang_opt_flag() } -> std::same_as<std::string_view>;
        { A::msvc_opt_flag() } -> std::same_as<std::string_view>;
        { A::is_ieee754_deterministic() } -> std::same_as<bool>;
    };

// ── Die volle {O0,O1,O2,O3,Ofast}-Auspraegungs-Familie (OF-2). Jede ist eine leere CRTP-Struct
//    (Design-Space-Vokabular); der Planer/die XML waehlt+permutiert, nichts ist gepinnt. ──

struct OptO0Option final : OptimizationLevelSubAxis<OptO0Option> {
    [[nodiscard]] static constexpr std::string_view do_opt_level_id() noexcept { return "O0"; }
    [[nodiscard]] static constexpr std::string_view do_gcc_opt_flag() noexcept { return "-O0"; }
    [[nodiscard]] static constexpr std::string_view do_clang_opt_flag() noexcept { return "-O0"; }
    [[nodiscard]] static constexpr std::string_view do_msvc_opt_flag() noexcept { return "/Od"; }
    [[nodiscard]] static constexpr bool             do_is_ieee754_deterministic() noexcept { return true; }
};

struct OptO1Option final : OptimizationLevelSubAxis<OptO1Option> {
    [[nodiscard]] static constexpr std::string_view do_opt_level_id() noexcept { return "O1"; }
    [[nodiscard]] static constexpr std::string_view do_gcc_opt_flag() noexcept { return "-O1"; }
    [[nodiscard]] static constexpr std::string_view do_clang_opt_flag() noexcept { return "-O1"; }
    [[nodiscard]] static constexpr std::string_view do_msvc_opt_flag() noexcept { return "/O1"; }
    [[nodiscard]] static constexpr bool             do_is_ieee754_deterministic() noexcept { return true; }
};

struct OptO2Option final : OptimizationLevelSubAxis<OptO2Option> {
    [[nodiscard]] static constexpr std::string_view do_opt_level_id() noexcept { return "O2"; }
    [[nodiscard]] static constexpr std::string_view do_gcc_opt_flag() noexcept { return "-O2"; }
    [[nodiscard]] static constexpr std::string_view do_clang_opt_flag() noexcept { return "-O2"; }
    [[nodiscard]] static constexpr std::string_view do_msvc_opt_flag() noexcept { return "/O2"; }
    [[nodiscard]] static constexpr bool             do_is_ieee754_deterministic() noexcept { return true; }
};

struct OptO3Option final : OptimizationLevelSubAxis<OptO3Option> {
    [[nodiscard]] static constexpr std::string_view do_opt_level_id() noexcept { return "O3"; }
    [[nodiscard]] static constexpr std::string_view do_gcc_opt_flag() noexcept { return "-O3"; }
    [[nodiscard]] static constexpr std::string_view do_clang_opt_flag() noexcept { return "-O3"; }
    [[nodiscard]] static constexpr std::string_view do_msvc_opt_flag() noexcept { return "/O2"; }
    [[nodiscard]] static constexpr bool             do_is_ieee754_deterministic() noexcept { return true; }
};

/// Ofast = O3 + -ffast-math-Familie + -fallow-store-data-races: aggressivste Stufe, IEEE-754-/
/// Determinismus-brechend (is_ieee754_deterministic()==false). CEB-DEFAULT (OF-2), per XML
/// ueberschreibbar. MSVC hat kein direktes Aequivalent -> /O2 (naechstliegend; /fp:fast waere die
/// separate Fliesskomma-Achse).
struct OptOfastOption final : OptimizationLevelSubAxis<OptOfastOption> {
    [[nodiscard]] static constexpr std::string_view do_opt_level_id() noexcept { return "Ofast"; }
    [[nodiscard]] static constexpr std::string_view do_gcc_opt_flag() noexcept { return "-Ofast"; }
    [[nodiscard]] static constexpr std::string_view do_clang_opt_flag() noexcept { return "-Ofast"; }
    [[nodiscard]] static constexpr std::string_view do_msvc_opt_flag() noexcept { return "/O2"; }
    [[nodiscard]] static constexpr bool             do_is_ieee754_deterministic() noexcept { return false; }
};

/// CEB-Default-Auspraegung — BEWEGLICHER Startwert, KEIN globaler Pin (User-Ruling 2026-07-18, Option B).
/// Korrektur der frueheren OF-2-Buchstaben-Lesart ("Default = Ofast"): der CEB-Default ist **O3**, weil O3
/// IEEE-754-DETERMINISTISCH ist (do_is_ieee754_deterministic()==true) und den 1-Thread-Mess-Determinismus der
/// golden-Reihe wahrt; -Ofast bricht ihn (-fallow-store-data-races/-funsafe-math). Ofast/O0/O1/O2 leben ADDITIV
/// als +opt=-Sidecar-Vergleichs-Extreme (OptOfastOption bleibt konkrete Achse). "Nichts gepinnt, JEDES TEIL
/// beweglich": dies ist NUR der benannte Default-Startwert; env COMDARE_PILOT_OPT_LEVEL + XML/Planer (A3)
/// ueberschreiben jedes Teil. Benannte Single-Source, damit die Default-Wahl nicht als rohes Literal dupliziert wird.
using DefaultOptLevelOption = OptO3Option;

static_assert(OptimizationLevelSubAxisConcept<OptO0Option>);
static_assert(OptimizationLevelSubAxisConcept<OptO1Option>);
static_assert(OptimizationLevelSubAxisConcept<OptO2Option>);
static_assert(OptimizationLevelSubAxisConcept<OptO3Option>);
static_assert(OptimizationLevelSubAxisConcept<OptOfastOption>);
static_assert(DefaultOptLevelOption::opt_level_id() == std::string_view{"O3"}, "Ruling 2026-07-18: CEB-Default = O3");
static_assert(DefaultOptLevelOption::is_ieee754_deterministic(), "O3 ist IEEE-754-deterministisch (Option B)");
static_assert(OptO2Option::gcc_opt_flag() == std::string_view{"-O2"});
static_assert(OptOfastOption::parent_axis_label() == std::string_view{"compiler"}, "opt_level haengt unter compiler");

} // namespace comdare::cache_engine::measurement
