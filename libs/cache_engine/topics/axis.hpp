// topics/axis.hpp -- gemeinsames Achsen-Dach (Bau-INC-1a, Q1-Ruling 2026-07-17).
//
// Semantik-FREIES Dach aller Achsen-Familien: Layer Supertype (Fowler, PoEAA) als
// Marker-Concept + CRTP-Tag, kombiniert mit Separation of Hierarchies. Das Dach traegt
// NUR Identitaet + Familien-Diskriminator; jede Familien-Semantik bleibt in den
// Unter-Wurzeln (Blut-Direktive: Organ- vs System-Achsen NIE mischen — die Familien
// schneiden sich nicht, sie teilen nur dieses zustandslose Dach).

#pragma once

#include <concepts>
#include <type_traits>

namespace comdare::cache_engine::topics {

/// Familien-Diskriminator des Achsen-Dachs.
enum class AxisKind : unsigned char {
    organ,              ///< Organ-/Tier-Binary-Achse (permutiert die binary_id, 18 Slots; STRUKT-R ORG-18)
    system_measurement, ///< Mess-System-Achse ("Blut", host-seitig immer praesent, golden-neutral)
    system_config,      ///< CEB-Konfig-System-Achse (Bau-/Steuer-Parameter, beruehrt NIE die binary_id)
    /// Meta-Meta-Achse (Lane A A1, Auftrag Abschnitt 1.4): eine VOLLE CT-Haupt-Achse, die unter dem
    /// external_utils-HUB haengt und selbst RT-Unter-Achsen tragen darf (externe HW, SIMD/AVX,
    /// Mess-Framework; load_framework ist die ERSTE Meta-Meta). Rein ADDITIVER Diskriminator: solange
    /// keine Achse ihn zurueckgibt, ist er verhaltens- und byte-neutral. Er ist KEIN drittes festes
    /// Level -- die Rekursion bleibt offen (Layer-Modell D4: Unter-Achse == Voll-Achse).
    system_meta_meta,
};

/// Das Dach: empty base, keine vtable, keine Familien-Semantik. Der Concept-Guard
/// (is_empty && !is_polymorphic) schlaegt beim Kompilieren fehl, falls je eine vtable
/// eingeschleppt wird (Anti-Runtime-Switch-Sicherung).
template <class Derived>
struct Axis {
protected:
    constexpr Axis() noexcept = default;
};

template <class D>
concept AxisConcept =
    std::derived_from<D, Axis<D>> && std::is_empty_v<Axis<D>> && (!std::is_polymorphic_v<Axis<D>>) && requires {
        { D::axis_kind() } -> std::convertible_to<AxisKind>;
    };

} // namespace comdare::cache_engine::topics
