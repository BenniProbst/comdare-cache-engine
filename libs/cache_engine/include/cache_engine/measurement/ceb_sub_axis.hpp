// measurement/ceb_sub_axis.hpp -- Unter-Achsen-Wurzel als TYP (Lane A, Paket A1, 26.07.2026).
//
// PROBLEM, das dieser Header loest: die Eltern-Beziehung einer Unter-Achse ist heute eine reine
// STRING-Konvention -- jede Unter-Achse schreibt ihr `parent_axis_label()` von Hand hin
// (compiler_atomic_sub_axis.hpp:42 "compiler", simd_sub_axis.hpp, optimization_level_sub_axis.hpp).
// Damit ist die Hierarchie nirgends typ-geprueft: ein Tippfehler oder ein Achsen-Rename haengt eine
// Unter-Achse still an ein Eltern-Label, das es nicht gibt, und NICHTS bricht. Genau diese Klasse von
// Drift hat die konkurrierenden Suffix-Ordnungen ueberhaupt moeglich gemacht.
//
// LOESUNG: der Eltern-Bezug ist ein TYP-Parameter. Das Label wird ABGELEITET, nicht wiederholt.
//
// LAYER-MODELL D4 (Auftrag Abschnitt 1, Satz 2/3): KEIN festes drittes Level. CebSubAxis leitet selbst
// von CebSystemAxis ab -- eine Unter-Achse IST also eine Voll-Achse und darf ihrerseits Eltern einer
// weiteren Unter-Achse sein. Die Rekursion ist offen; EIN Concept gilt fuer alle Achsen.
//
// BYTE-NEUTRALITAET (A1-Auflage): dieser Header ist rein ADDITIV. Die bestehenden Unter-Achsen werden
// hier BEWUSST NICHT umgehaengt -- das ist Paket A3, weil dort die Ordnung ohnehin angefasst wird. Bis
// dahin existiert die Typ-Wurzel als Angebot, ohne einen einzigen emittierten String zu veraendern.
//
// Benannter Pattern-Stapel (compile-time-only, keine vtable): CRTP (Coplien) + Layer Supertype
// (Fowler, PoEAA) + Concept-based Static Interface (C++20/23).

#pragma once

#include "ceb_system_axis.hpp"

#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

/// CebSubAxis<Derived, ParentAxis> -- eine Unter-Achse, deren Eltern-Achse ein TYP ist.
///
/// `parent_axis` ist der Typ selbst (maschinen-pruefbar); `parent_axis_label()` ist daraus ABGELEITET
/// und kann nicht mehr von der Eltern-Achse abweichen. Wer die Eltern-Achse umbenennt, aendert damit
/// automatisch alle Kind-Labels -- kein Nachzieh-Ort, keine Drift.
template <class Derived, class ParentAxis>
struct CebSubAxis : CebSystemAxis<Derived> {
    /// Die Eltern-Achse als TYP -- das ist der eigentliche Fortschritt gegenueber der String-Konvention.
    using parent_axis = ParentAxis;

    /// ABGELEITET aus der Eltern-Achse (NICHT von Hand wiederholt). Signatur und Rueckgabe-Typ sind
    /// identisch zur bestehenden String-Konvention, damit ein Umhaengen in A3 byte-neutral bleibt.
    [[nodiscard]] static constexpr std::string_view parent_axis_label() noexcept {
        return ParentAxis::axis_label();
    }

protected:
    constexpr CebSubAxis() noexcept = default;
};

/// Concept der Unter-Achsen-Schicht. Bewusst KEIN eigenes "Sub"-Universum: eine Unter-Achse muss
/// zusaetzlich das VOLLE Achsen-Concept erfuellen (Layer-Modell D4). Die Eltern-Achse muss ihrerseits
/// eine gueltige Konfig-System-Achse sein -- damit ist die Kette typ-geschlossen, nach oben wie unten.
template <class A>
concept CebSubAxisConcept = CebSystemAxisConcept<A> && requires {
    typename A::parent_axis;
    { A::parent_axis_label() } -> std::convertible_to<std::string_view>;
} && CebSystemAxisConcept<typename A::parent_axis> &&
    std::derived_from<A, CebSubAxis<A, typename A::parent_axis>>;

/// OFFENE REKURSION, compile-time abfragbar: die Tiefe einer Achse in ihrer eigenen Kette. Eine
/// Haupt-Achse hat Tiefe 0, ihre Unter-Achse 1, deren Unter-Achse 2 -- ohne dass irgendwo ein Maximum
/// steht. Das ist die Gegenprobe zu "KEIN festes 3. Level": der Wert ist emergent, nicht deklariert.
template <class A>
struct axis_depth {
    static constexpr std::size_t value = 0;
};
template <class A>
    requires requires { typename A::parent_axis; }
struct axis_depth<A> {
    static constexpr std::size_t value = 1 + axis_depth<typename A::parent_axis>::value;
};
template <class A>
inline constexpr std::size_t axis_depth_v = axis_depth<A>::value;

} // namespace comdare::cache_engine::measurement
