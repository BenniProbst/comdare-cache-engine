// topics/organ_axis.hpp -- Organ-Familien-Wurzel unterm gemeinsamen Achsen-Dach (Bau-INC-1a, 2026-07-18).
//
// Die Organ-Achsen (T0..T18, die kompositions-/binary_id-permutierenden Achsen) haengen -- wie die
// Mess-Wurzel SystemAxis (system_measurement) und die Konfig-Wurzel CebSystemAxis (system_config) --
// unter dem gemeinsamen semantik-freien Dach topics::Axis (Q1-Ruling; Blut-Direktive: die drei Familien
// teilen NUR das zustandslose Dach, ihre Semantik schneidet sich nicht). Vorher trugen die ~20 Organ-
// StrategyBase nur AxisBase (cross-axis Properties) OHNE das Dach -- der AxisKind::organ-Diskriminator war
// definiert, aber real ungenutzt (Konformitaets-Sweep-Befund INC-1, User-Ruling 2026-07-18: "gemeinsamer
// Header fuer Mess-, System- und Organ-Achsen"). OrganAxis<Derived> vereinigt beide Basen -- BEIDE EMPTY
// -> Empty-Base-Optimization: entscheidend ist, dass Axis<Derived> ein UNIKALER leerer Typ ist (es entsteht
// KEIN zweites Axis<Derived>-Subobjekt) -> sizeof/Layout aller Wrapper UNVERAENDERT, d.h. sizeof(alt :AxisBase)
// == sizeof(neu :OrganAxis<Derived>) -- und liefert axis_kind()==organ GENAU EINMAL (DRY). Die golden==320-Byte-
// Identitaet folgt daraus, wird aber SEPARAT per golden-Roundtrip/CI belegt (hier keine Erfolgsmarke behauptet).
//
// Benannter Pattern-Stapel (compile-time-only, keine vtable): CRTP (Coplien) + Layer Supertype (Fowler, PoEAA)
// + Concept-based Static Interface (C++20/23). Der Concept-Guard (is_empty && !is_polymorphic auf der Dach-/
// OrganAxis-Schicht) sichert DIESE Schicht gegen vtable-Einschleppung -- NICHT das Derived selbst; dessen
// Polymorphie prueft der jeweilige per-Wrapper-Concept, wo Derived vollstaendig ist.

#pragma once

#include "axis.hpp"      // topics::Axis<Derived> + AxisKind + AxisConcept (das Dach)
#include "axis_base.hpp" // topics::AxisBase + AxisBaseConcept (cross-axis Pflicht-Properties)

#include <concepts>
#include <type_traits>

namespace comdare::cache_engine::topics {

/// Organ-Familien-Wurzel: vereinigt das gemeinsame Dach (Axis<Derived>) mit den cross-axis Pflicht-
/// Properties (AxisBase) und traegt den Familien-Diskriminator organ. Eine Organ-StrategyBase erbt statt
/// `: public AxisBase` jetzt `: public OrganAxis<Derived>` -- damit ist sie AxisConcept-erfuellend (unter
/// dem Dach) UND behaelt get_compiler()/is_original_module() UNVERAENDERT (AxisBase bleibt Basis).
template <class Derived>
struct OrganAxis : Axis<Derived>, AxisBase {
    [[nodiscard]] static constexpr AxisKind axis_kind() noexcept { return AxisKind::organ; }

protected:
    constexpr OrganAxis() noexcept = default;
};

/// Concept-Guard: eine Organ-Achse haengt unterm Dach (AxisConcept), erfuellt die cross-axis Properties
/// (AxisBaseConcept), leitet von OrganAxis ab und traegt den organ-Diskriminator. Die is_empty-/
/// !is_polymorphic-Pruefungen auf OrganAxis<D> belegen die Leerheit DIESER Schicht (EBO-Voraussetzung); die
/// Wrapper-Byte-Neutralitaet folgt aus dem UNIKATEN Axis<D> (kein zweites Subobjekt), nicht aus is_empty_v<OrganAxis<D>>.
template <class D>
concept OrganAxisConcept =
    AxisConcept<D> && AxisBaseConcept<D> && std::derived_from<D, OrganAxis<D>> && std::is_empty_v<OrganAxis<D>> &&
    (!std::is_polymorphic_v<OrganAxis<D>>) && (D::axis_kind() == AxisKind::organ);

} // namespace comdare::cache_engine::topics
