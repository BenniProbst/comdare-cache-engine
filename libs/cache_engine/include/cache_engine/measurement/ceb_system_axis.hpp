// measurement/ceb_system_axis.hpp -- Konfig-System-Achsen-Wurzel (Bau-INC-1a, 2026-07-17).
//
// Geschwister-Wurzel der Mess-SystemAxis unter dem gemeinsamen Dach topics::Axis
// (Q1-Ruling). Konfig-System-Achsen sind KEINE Mess-Quellen: sie bestimmen als
// CEB-Bau-Parameter, wie die untergeordneten Tier-Binaries statisch gebaut und
// durchgemessen werden (Scheduling / Hardware-ISA / Telemetrie-Regime / Last-Framework /
// Compiler / Erweiterungshardware). Sie permutieren die 17 Organ-Achsen NICHT und
// stehen nie in kCompositionAxisNames (golden==320 bleibt unberuehrt).

#pragma once

#include "../../../topics/axis.hpp"

#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

template <class Derived>
struct CebSystemAxis : topics::Axis<Derived> {
    [[nodiscard]] static constexpr topics::AxisKind axis_kind() noexcept { return topics::AxisKind::system_config; }

    /// Compile-time-Reflexion des Achsen-Etiketts (Serialisierungs-Ordner/Sidecar);
    /// reiner static-dispatch via Derived, KEINE virtuellen Funktionen.
    [[nodiscard]] static constexpr std::string_view axis_label() noexcept { return Derived::do_axis_label(); }

protected:
    constexpr CebSystemAxis() noexcept = default;
};

template <class A>
concept CebSystemAxisConcept =
    topics::AxisConcept<A> && std::derived_from<A, CebSystemAxis<A>> && std::is_empty_v<CebSystemAxis<A>> &&
    (!std::is_polymorphic_v<CebSystemAxis<A>>) && requires {
        { A::do_axis_label() } -> std::convertible_to<std::string_view>;
    };

} // namespace comdare::cache_engine::measurement
