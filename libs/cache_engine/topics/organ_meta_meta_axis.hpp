// topics/organ_meta_meta_axis.hpp -- Wurzel der ORGAN-Realm-Meta-Meta-Achsen
// (A13-M2, Owner-Entscheid E2 vom 02.08.2026; OP-11-Rueckbau).
//
// OWNER-WORTLAUT (E2, verbatim): "Die Meta-Meta-Achsen und deren Stempel-Eintraege sind wie alle
// Hauptachsen PFLICHT, das gilt fuer ALLE Hauptachsen. ... Da eine Meta-Meta-Achse immer zu den
// Mess-Achsen, System-Achsen oder Organ-Achsen gehoert, wird sie auch einfach dynamisch ans Ende der
// Kette in den bestehenden Zeilen angehaengt."
//
// WAS HIER SUPERSEDED WIRD: OP-11 (super docs/sessions/20260727-PLAN-o8-fenster-atomar-ultracode.md:813-822,
// Manager-Entscheid 27.07.2026) verbot Meta-Meta-Eintraege in der Organ-Zeile ("KEINE Meta-Meta-Eintraege in
// dieser Zeile (RF-7)"). Der Owner-Wortlaut vom 02.08.2026 nennt die Organ-Achsen ausdruecklich als einen der
// drei Traeger-Realms und verdraengt das Verbot. Der alte Wortlaut bleibt als Historie zitiert (Doku-Doktrin:
// nie loeschen, nur supersedieren).
//
// WAS RF-7 BLEIBT: "je Achsen-Typ EINE Array-Zeile" gilt unveraendert. Eine Organ-Meta-Meta stempelt im
// ORGAN-Realm und NIE zeilen-fremd; die System-Meta-Metas bleiben in der System-Zeile, load_framework in der
// Mess-Zeile. Superseded ist ausschliesslich der Satz "die Organ-Zeile traegt gar keine".
//
// HEUTIGER ZUSTAND, ehrlich: es gibt KEINE Organ-Meta-Meta. Die Typliste (abi/anatomy_version_stamp.hpp:
// kOrganMetaMetas) ist LEER, der Klammer-Anhang damit leer, die Organ-Zeile BYTE-IDENTISCH. Gebaut ist der
// MECHANISMUS -- die Erfuellung der Owner-Forderung E-10/nr798-B3org ("ORGAN-analoges Meta-Meta-Array +
// erweiterbarer Compile-Raum-Stempel", super docs/sessions/20260726-SESSION-...-982-E01-E26.md:54-56): wer
// eine Organ-Meta-Meta baut, haengt sie in die Typliste und sie stempelt, ohne dass eine Zeile Emitter-Code
// angefasst wird.
//
// ABGRENZUNG zu den Nachbar-Wurzeln unter demselben Dach topics::Axis:
//   * OrganAxis                 -- die 18 kompositions-/binary_id-permutierenden Organ-HAUPT-Achsen
//   * CebSystemAxis             -- CEB-Konfig-System-Achsen
//   * SystemMetaMetaAxis        -- System-Meta-Metas unter dem external_utils-Hub
//   * MeasurementMetaMetaAxis   -- Mess-Meta-Metas des Planers (load_framework)
//   * OrganMetaMetaAxis (HIER)  -- Meta-Meta-HAUPT-Achsen des ORGAN-Realms
//
// WARUM NICHT VON OrganAxis ABGELEITET: OrganAxis zieht AxisBase (cross-axis Pflicht-Properties
// get_compiler()/is_original_module()) mit -- die gelten fuer eine permutierende Organ-STRATEGIE, nicht
// zwingend fuer eine Meta-Meta ueber ihr. Dieselbe Entscheidung wie im Mess-Realm
// (MeasurementMetaMetaAxis erbt direkt vom Dach, nicht von SystemAxis). Metaprog: CRTP + Concept,
// static-dispatch, keine vtable, kein Runtime-Switch.

#pragma once

#include "axis.hpp"

#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::topics {

/// Wurzel der ORGAN-Realm-Meta-Meta-HAUPT-Achsen. Gegenstueck zu MeasurementMetaMetaAxis/SystemMetaMetaAxis
/// auf der Organ-Seite: identische Mechanik (CRTP, leeres Dach, static-dispatch), anderer Realm-Diskriminator.
template <class Derived>
struct OrganMetaMetaAxis : Axis<Derived> {
    [[nodiscard]] static constexpr AxisKind axis_kind() noexcept { return AxisKind::organ_meta_meta; }

    /// Compile-time-Reflexion des Achsen-Etiketts; reiner static-dispatch via Derived.
    [[nodiscard]] static constexpr std::string_view axis_label() noexcept { return Derived::do_axis_label(); }

protected:
    constexpr OrganMetaMetaAxis() noexcept = default;
};

/// Das Concept fordert dasselbe wie im System-Realm: Achsen-Etikett, eine EIGENE RT-Unter-Achse (wer keine
/// spannt, ist keine Meta-Meta) und -- Owner-E2 "PFLICHT wie alle Hauptachsen" -- eine eigene bump-bare
/// axis_code_version, damit eine Drift im Stempel sichtbar wird.
template <class A>
concept OrganMetaMetaAxisConcept =
    AxisConcept<A> && std::derived_from<A, OrganMetaMetaAxis<A>> && std::is_empty_v<OrganMetaMetaAxis<A>> &&
    (!std::is_polymorphic_v<OrganMetaMetaAxis<A>>) && requires {
        { A::do_axis_label() } -> std::convertible_to<std::string_view>;
        { A::sub_axis_label() } -> std::convertible_to<std::string_view>;
        { A::axis_code_version } -> std::convertible_to<std::string_view>;
    };

// Die Schicht bleibt leer und nicht-polymorph -- sonst brechen die is_empty_v-Wachen der Achsen.
static_assert(std::is_empty_v<OrganMetaMetaAxis<struct OrganMetaMetaProbe>>);
static_assert(!std::is_polymorphic_v<OrganMetaMetaAxis<struct OrganMetaMetaProbe>>);
// Der Diskriminator ist von den vier Nachbarn VERSCHIEDEN (sonst waeren zwei Realms im Stempel gleich).
static_assert(AxisKind::organ_meta_meta != AxisKind::organ);
static_assert(AxisKind::organ_meta_meta != AxisKind::system_meta_meta);
static_assert(AxisKind::organ_meta_meta != AxisKind::measurement_meta_meta);
static_assert(AxisKind::organ_meta_meta != AxisKind::system_config);
static_assert(AxisKind::organ_meta_meta != AxisKind::system_measurement);

} // namespace comdare::cache_engine::topics
