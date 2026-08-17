// measurement/ceb_complex_system_axis.hpp -- die AEUSSERE System-Komplex-Haupt-Achse
// (O-8 Schritt 6 / Bauplan IV.2.7, O-1r; Owner-GO OP-5, Ledger 76).
//
// WAS SIE IST: der Command-Pattern-Wrapper ueber der REKOMBINATION der drei System-Haupt-Achsen
// target_isa x operating_system x external_utils. Owner-KERN (R-D): "alle 3 sind Haupt-Achsen,
// verhalten sich aber wie EINE". Genau das leistet dieser Typ -- er nimmt die drei als TYP-Parameter
// und exponiert sie als EINE Achse, ohne dass eine von ihnen ihren Haupt-Status verliert.
//
// WOFUER SIE DA IST: sie traegt die Unter-Achsen-GRUPPE compiler + opt_level + atomic128 (O-1r).
// Diese Gruppe haengt AUSDRUECKLICH hier und NICHT an target_isa allein: sie beschreibt, WIE aus der
// Rekombination aller drei Achsen gebaut wird, nicht nur aus der Ziel-ISA. compiler hat mit A3
// (Schritt 4) seinen Haupt-Achsen-Status verloren und kommt hier als Gruppen-Glied wieder an.
//
// -- REKURSION (Ledger 3521 "rekursiv gewrappt") -------------------------------------------------
// Der aeussere Wrapper ist Ebene 0. Eine Ebene tiefer sitzt TargetIsaComplexAxis, die ihrerseits ein
// Komplex ist (RAM-Frequenz x CAS x CPU-Fabrikation). Dasselbe Command-Pattern also zweimal, ineinander.
// Die Rekursion ist NICHT auf zwei Ebenen begrenzt -- es gibt kein festes Maximum, genau wie bei den
// Unter-Achsen (Layer-Modell D4, ceb_sub_axis.hpp).
//
// -- WAS NICHT DAZUGEHOERT -----------------------------------------------------------------------
// load_framework ist KEIN Glied: es hat die System-Welt mit A3 ERSATZLOS verlassen und ist
// Meta-Meta-Haupt-Achse des MESS-Realms (Ledger 69.1). Die System-Meta-Metas (SIMD/AVX, externe
// Hardware, GPU/FPGA/NPU) fahren unter dem external_utils-Hub mit, nicht als eigene Glieder.
//
// -- KEIN EINTRAG IN kSystemAxisOrder ------------------------------------------------------------
// Der Wrapper ist KEINE vierte Haupt-Achse -- er ist die Klammer UM die drei. kSystemAxisOrder bleibt
// bei GENAU DREI (A3, Owner-KERN). Wer ihn dort eintraegt, hat aus der Klammer ein Glied gemacht.
// Der static_assert am Dateiende haelt das fest.
//
// Benannter Pattern-Stapel (compile-time-only, keine vtable): Command (GoF) als Wrapper-Buendelung
// nach dem Vorbild builder/commands/axis_library_registry.hpp + CRTP (Coplien) + Concept-based
// Static Interface (C++20/23). Kein std::variant, kein Runtime-Switch.

#pragma once

#include <cache_engine/measurement/ceb_system_axis.hpp>
#include <system_axes/compiler_atomic_sub_axis.hpp>
#include <system_axes/compiler_system_axis.hpp>
#include <system_axes/external_utils_family_axis.hpp>
#include <system_axes/operating_system_axis.hpp>
#include <system_axes/optimization_level_sub_axis.hpp>
#include <system_axes/simd_sub_axis.hpp>
#include <system_axes/target_isa_complex_axis.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

/// Eine Unter-Achsen-GRUPPE: mehrere Unter-Achsen, die gemeinsam an EINEM Wrapper haengen und
/// gemeinsam benannt sind. Sie ist selbst keine Achse -- sie ist die Buendelung, die das
/// Command-Pattern braucht, damit "drei Unter-Achsen" ein Gegenstand mit einem Namen wird.
template <class... SubAxes>
struct SubAxisGroup {
    /// Der Gruppen-Name (Registry-Etikett der Buendelung, nicht der einzelnen Unter-Achse).
    [[nodiscard]] static constexpr std::string_view group_label() noexcept { return "build_toolchain"; }

    /// Wie viele Unter-Achsen die Gruppe buendelt -- aus dem Pack, nie als Literal.
    [[nodiscard]] static constexpr std::size_t size() noexcept { return sizeof...(SubAxes); }

    /// Die Labels der Glieder in Deklarations-Reihenfolge (Single-Source fuer Emitter und Wachen).
    [[nodiscard]] static constexpr std::array<std::string_view, sizeof...(SubAxes)> labels() noexcept {
        return {SubAxes::axis_label()...};
    }
};

/// Die Unter-Achsen-Gruppe der aeusseren Komplex-Achse (O-1r): compiler + opt_level + atomic128.
/// Die Glieder sind die REALEN Achsen-Typen, nie Literale -- ein Rename zieht hier automatisch mit.
using BuildToolchainSubAxes = SubAxisGroup<GccCompilerAxis, OptO3Option, Cx16Option>;

/// Der aeussere Wrapper. Die drei Haupt-Achsen kommen als TYP-Parameter herein; die Gruppe haengt
/// als vierter Parameter daran (nicht als Glied der Rekombination -- sie beschreibt den BAU).
template <class TargetIsaComplex, class OperatingSystem, class ExternalUtils, class SubAxes>
struct CompoundSystemAxis
    : CebSystemAxis<CompoundSystemAxis<TargetIsaComplex, OperatingSystem, ExternalUtils, SubAxes>> {
    using target_isa_axis       = TargetIsaComplex;
    using operating_system_axis = OperatingSystem;
    using external_utils_axis   = ExternalUtils;
    using sub_axis_group        = SubAxes;

    /// Ein EIGENER Achsen-Name: der Komplex ist nicht eine seiner Teil-Achsen, sondern ihre Klammer.
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "build_target_complex"; }

    /// Die Labels der drei gewrappten Haupt-Achsen, in kanonischer Ordnung (target_isa,
    /// operating_system, external_utils). Aus den TYPEN gezogen, damit sie nicht driften koennen.
    [[nodiscard]] static constexpr std::array<std::string_view, 3> member_labels() noexcept {
        return {TargetIsaComplex::axis_label(), OperatingSystem::axis_label(), ExternalUtils::axis_label()};
    }

    /// Die gebuendelte Unter-Achsen-Gruppe (O-1r) -- die eigentliche Daseins-Berechtigung des Wrappers.
    [[nodiscard]] static constexpr std::string_view sub_axis_group_label() noexcept { return SubAxes::group_label(); }
    [[nodiscard]] static constexpr std::size_t      sub_axis_count() noexcept { return SubAxes::size(); }

protected:
    constexpr CompoundSystemAxis() noexcept = default;
};

template <class A>
concept CompoundSystemAxisConcept = CebSystemAxisConcept<A> && requires {
    typename A::target_isa_axis;
    typename A::operating_system_axis;
    typename A::external_utils_axis;
    typename A::sub_axis_group;
    { A::member_labels() } -> std::same_as<std::array<std::string_view, 3>>;
    { A::sub_axis_group_label() } -> std::same_as<std::string_view>;
    { A::sub_axis_count() } -> std::same_as<std::size_t>;
};

/// DIE aeussere Komplex-Achse der CEB: die Rekombination der drei Default-Auspraegungen, mit der
/// Bau-Gruppe daran. "Default" heisst hier beweglicher Startwert, KEIN Pin -- der Planer bzw. die XML
/// waehlt die Auspraegungen; die KLAMMER bleibt dieselbe.
using CebCompoundSystemAxis =
    CompoundSystemAxis<DefaultTargetIsaComplex, DefaultOperatingSystem, SimdExternalUtilsFamily, BuildToolchainSubAxes>;

static_assert(CompoundSystemAxisConcept<CebCompoundSystemAxis>);
static_assert(CebCompoundSystemAxis::axis_label() == std::string_view{"build_target_complex"});

// GLIEDER-WACHE: der Wrapper klammert GENAU die drei Haupt-Achsen, in kanonischer Ordnung. Die
// Erwartung steht hier als Label-Vergleich und NICHT als Literal-Liste -- sie kommt aus den Typen.
static_assert(CebCompoundSystemAxis::member_labels()[0] == std::string_view{"target_isa"});
static_assert(CebCompoundSystemAxis::member_labels()[1] == std::string_view{"operating_system"});
static_assert(CebCompoundSystemAxis::member_labels()[2] == std::string_view{"external_utils"});

// GRUPPEN-WACHE (O-1r): DREI Unter-Achsen, und zwar genau compiler/opt_level/atomic128. Wer ein
// viertes Glied anhaengt oder eines herausnimmt, bricht hier -- die Gruppe ist owner-benannt.
static_assert(BuildToolchainSubAxes::size() == 3);
static_assert(BuildToolchainSubAxes::labels()[0] == std::string_view{"compiler"});
static_assert(BuildToolchainSubAxes::labels()[1] == std::string_view{"opt_level"});
static_assert(BuildToolchainSubAxes::labels()[2] == std::string_view{"atomic128"});

// ABGRENZUNGS-WACHE: der Wrapper ist KEINE der drei Achsen und traegt einen eigenen Namen.
static_assert(CebCompoundSystemAxis::axis_label() != std::string_view{"target_isa"});
static_assert(CebCompoundSystemAxis::axis_label() != std::string_view{"operating_system"});
static_assert(CebCompoundSystemAxis::axis_label() != std::string_view{"external_utils"});
// load_framework ist KEIN Glied (Ledger 69.1): es lebt im Mess-Realm, nicht in dieser Klammer.
static_assert(BuildToolchainSubAxes::labels()[0] != std::string_view{"load_framework"} &&
                  BuildToolchainSubAxes::labels()[1] != std::string_view{"load_framework"} &&
                  BuildToolchainSubAxes::labels()[2] != std::string_view{"load_framework"},
              "load_framework hat die System-Welt mit A3 ERSATZLOS verlassen und gehoert weder in die "
              "Rekombination noch in ihre Bau-Gruppe.");

} // namespace comdare::cache_engine::measurement
