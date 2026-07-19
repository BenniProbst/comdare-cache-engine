// measurement/extension_hardware_family_axis.hpp -- extension_hardware als AKTIVER Familien-Knoten der
// 6. CEB-Konfig-System-Haupt-Achse (GN-1/E-4, Q2-Ruling Option C; Roadmap 2026-07-19).
//
// LUECKE (Register E-4 / Audit G2): seit F-SIMD (2026-07-18) traegt NUR die DEPRECATED-Insel
// extension_hardware_system_axis.hpp das Label "extension_hardware" -- SimdSubAxis::parent_axis_label()
// zeigte damit auf einen VERWAISTEN String (kein aktiver CebSystemAxis mit diesem Label). Dieser Header
// schliesst die Luecke: der aktive Haupt-Achsen-Knoten, ANALOG CompilerSystemAxis (compiler -> gcc|clang),
// symmetrisch fuer gcc/clang (die je-Dialekt-Flag-Reflexion liefern die drangehaengten Unter-Achsen-
// Optionen: SimdSubAxis fuehrt gcc_march_flag/clang_march_flag/msvc_march_flag spiegelbildlich zu
// OptimizationLevelSubAxis). Die DEPRECATED-F-SIMD-Insel wird NICHT reaktiviert -- sie bleibt reiner
// Kontrast (test_striktheit_axis_dach_guard Block F); dieser Knoten ist der EINE aktive Traeger des Labels.
//
// STRUKTUR (Ebenen-Symmetrie compiler/opt_level <-> extension_hardware/simd, F-SIMD):
//   extension_hardware (Haupt-System-Achse, DIESER Knoten)
//     -> Familie {simd (heute), gpu (spaeter, Q2: SIMD->GPU)}  -- je Familie eine dynamische Unter-Achse
//        -> simd-Optionen {no_extension, avx2, avx512}         -- SimdSubAxis (simd_sub_axis.hpp)
//
// Die Familien-Kennung ist zugleich das Unter-Achsen-Label (Konvention "extension_hardware.simd=...";
// Single-Source-Muster wie LoadFrameworkSystemAxis::sub_axis_label()=="workload"). binary_id-NEUTRAL
// (system_config, steht nie in kCompositionAxisNames, golden unberuehrt); der -march-WERT kommt aus der
// Unter-Achsen-Option, der ORT ist die CompileFn-Naht (opt-g-Facade), die PROVENIENZ das H-10-Sidecar.
// Metaprog: CRTP + Concept, static-dispatch, keine vtable (Lehrbuch-Pattern, kein Runtime-Switch).

#pragma once

#include <cache_engine/measurement/ceb_system_axis.hpp>
#include <cache_engine/measurement/simd_sub_axis.hpp> // SimdSubAxis-Familie (die drangehaengte Unter-Achse)

#include <array>
#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

/// extension_hardware -- der aktive Familien-Knoten der Erweiterungs-Hardware-Haupt-Achse. Jede Auspraegung
/// ist eine HARDWARE-FAMILIE (simd heute, gpu spaeter), deren Auspraegungs-Raum die gleichnamige dynamische
/// Unter-Achse traegt. Analog CompilerSystemAxis (Haupt-Knoten, Familien-Kennung je Auspraegung).
template <class Derived>
struct ExtensionHardwareFamilyAxis : CebSystemAxis<Derived> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "extension_hardware"; }

    /// Familien-Kennung (Ordner-/Sidecar-Etikett): "simd" | spaeter "gpu".
    [[nodiscard]] static constexpr std::string_view family_id() noexcept { return Derived::do_family_id(); }

    /// Label der dynamischen Unter-Achse dieser Familie (Single-Source der Serialisierungs-/Permutations-
    /// Konvention "extension_hardware.<sub>=..."; Muster LoadFrameworkSystemAxis::sub_axis_label()).
    /// Per Konvention == family_id (die Familie SPANNT ihre gleichnamige Unter-Achse).
    [[nodiscard]] static constexpr std::string_view sub_axis_label() noexcept { return Derived::do_family_id(); }

protected:
    constexpr ExtensionHardwareFamilyAxis() noexcept = default;
};

template <class A>
concept ExtensionHardwareFamilyAxisConcept =
    CebSystemAxisConcept<A> && std::derived_from<A, ExtensionHardwareFamilyAxis<A>> &&
    std::is_empty_v<ExtensionHardwareFamilyAxis<A>> && (!std::is_polymorphic_v<ExtensionHardwareFamilyAxis<A>>) &&
    requires {
        { A::family_id() } -> std::same_as<std::string_view>;
        { A::sub_axis_label() } -> std::same_as<std::string_view>;
    };

/// SIMD ist die erste (und aktuell einzige) Erweiterungs-Hardware-Familie (Q2 Option C: SIMD -> GPU).
/// Ihre Auspraegungen {no_extension, avx2, avx512} leben in der SimdSubAxis-Familie (simd_sub_axis.hpp).
struct SimdExtensionHardwareFamily final : ExtensionHardwareFamilyAxis<SimdExtensionHardwareFamily> {
    [[nodiscard]] static constexpr std::string_view do_family_id() noexcept { return "simd"; }
};

/// CEB-Default-Familie -- simd (die einzige angebundene). Beweglicher Startwert, KEIN Pin.
using DefaultExtensionHardwareFamily = SimdExtensionHardwareFamily;

/// Single-Source der gueltigen Familien-Kennungen (Design-Space-Vokabular; gpu folgt mit dem GPU-Increment).
inline constexpr std::array<std::string_view, 1> kAllExtensionHardwareFamilyIds = {
    SimdExtensionHardwareFamily::family_id()};

static_assert(ExtensionHardwareFamilyAxisConcept<SimdExtensionHardwareFamily>);
static_assert(SimdExtensionHardwareFamily::axis_kind() == topics::AxisKind::system_config);
static_assert(SimdExtensionHardwareFamily::axis_label() == std::string_view{"extension_hardware"});
static_assert(SimdExtensionHardwareFamily::family_id() == std::string_view{"simd"});

// ── GN-1-AUFLOESUNG ("SimdSubAxis dranhaengen"): parent_axis_label() der Unter-Achse zeigt jetzt auf DIESEN
//    aktiven Knoten (nicht mehr auf den verwaisten String der DEPRECATED-Insel). Drift bricht hier. ──
static_assert(SimdNoExtOption::parent_axis_label() == SimdExtensionHardwareFamily::axis_label(),
              "GN-1: SimdSubAxis.parent_axis_label muss auf den aktiven extension_hardware-Knoten aufloesen.");
static_assert(SimdNoExtOption::do_axis_label() == SimdExtensionHardwareFamily::sub_axis_label(),
              "GN-1: die simd-Familie spannt die gleichnamige Unter-Achse (Label-Konvention).");
// Symmetrie gcc/clang (analog CompilerSystemAxis gcc|clang): jede drangehaengte simd-Option reflektiert BEIDE
// Dialekt-Flags compile-time (SimdSubAxisConcept erzwingt gcc_march_flag UND clang_march_flag; hier der Anker).
static_assert(SimdAvx2Option::gcc_march_flag() == SimdAvx2Option::clang_march_flag(),
              "Q3-Symmetrie: gcc/clang teilen die -mavx2-Schreibweise.");
static_assert(SimdAvx512Option::gcc_march_flag() == SimdAvx512Option::clang_march_flag(),
              "Q3-Symmetrie: gcc/clang teilen die -mavx512f-Schreibweise.");

} // namespace comdare::cache_engine::measurement
