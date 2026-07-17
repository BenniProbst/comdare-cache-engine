// measurement/compiler_system_axis.hpp -- Compiler als 5. CEB-Konfig-System-Achse
// (Bau-INC-1h, Q3-Ruling 2026-07-17).
//
// User-Entscheid: der Compiler ist eine VOLLE System-Achse — der Experiment-Planer baut sowohl
// die CEB als auch (durch die CEB) die Tier-Binaries mit BEIDEN Compilern und vergleicht die
// Ergebnisse (Compiler-Fenster bis ~5% Performance-Unterschied). Erste Ausbaustufe strikt
// gcc|clang; die Verfuegbarkeits-Erkennung zur Laufzeit ist die dynamische Unter-Achse (S-6).
// Serialisierung: eigener flacher Ordner-Level Host -> OS -> Compiler -> ISA; Provenienz im
// H-10-Sidecar/build_version — NIE in der binary_id.

#pragma once

#include <cache_engine/measurement/ceb_system_axis.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

template <class Derived>
struct CompilerSystemAxis : CebSystemAxis<Derived> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "compiler"; }

    /// Familien-Kennung (Ordner-Etikett): "gcc" | "clang".
    [[nodiscard]] static constexpr std::string_view compiler_id() noexcept { return Derived::do_compiler_id(); }
    /// Default-Treiber-Binary dieser Auspraegung (env COMDARE_CXX bleibt Laufzeit-Override).
    [[nodiscard]] static constexpr std::string_view driver_default() noexcept { return Derived::do_driver_default(); }
    /// Kennt der Dialekt das GNU-only-Flag -fno-gnu-unique? (clang: nein — hartes Bau-Gate.)
    [[nodiscard]] static constexpr bool supports_fno_gnu_unique() noexcept {
        return Derived::do_supports_fno_gnu_unique();
    }

protected:
    constexpr CompilerSystemAxis() noexcept = default;
};

template <class A>
concept CompilerSystemAxisConcept =
    CebSystemAxisConcept<A> && std::derived_from<A, CompilerSystemAxis<A>> && std::is_empty_v<CompilerSystemAxis<A>> &&
    (!std::is_polymorphic_v<CompilerSystemAxis<A>>) && requires {
        { A::compiler_id() } -> std::same_as<std::string_view>;
        { A::driver_default() } -> std::same_as<std::string_view>;
        { A::supports_fno_gnu_unique() } -> std::same_as<bool>;
    };

struct GccCompilerAxis final : CompilerSystemAxis<GccCompilerAxis> {
    [[nodiscard]] static constexpr std::string_view do_compiler_id() noexcept { return "gcc"; }
    [[nodiscard]] static constexpr std::string_view do_driver_default() noexcept { return "g++-16"; }
    [[nodiscard]] static constexpr bool             do_supports_fno_gnu_unique() noexcept { return true; }
};

struct ClangCompilerAxis final : CompilerSystemAxis<ClangCompilerAxis> {
    [[nodiscard]] static constexpr std::string_view do_compiler_id() noexcept { return "clang"; }
    [[nodiscard]] static constexpr std::string_view do_driver_default() noexcept { return "clang++-22"; }
    [[nodiscard]] static constexpr bool             do_supports_fno_gnu_unique() noexcept { return false; }
};

static_assert(CompilerSystemAxisConcept<GccCompilerAxis>);
static_assert(CompilerSystemAxisConcept<ClangCompilerAxis>);

} // namespace comdare::cache_engine::measurement
