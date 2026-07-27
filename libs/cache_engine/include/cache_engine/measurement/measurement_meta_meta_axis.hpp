// measurement/measurement_meta_meta_axis.hpp -- Wurzel der MESS-Realm-Meta-Meta-Achsen
// (O-8 Schritt 4 / Paket A3-K1; Ledger 70.1 RF-1 "VOLLES GO", Ledger 69.1 R-G).
//
// WARUM EINE EIGENE WURZEL: CebSystemAxis liefert axis_kind() HART als system_config -- eine Achse,
// die davon erbt, IST damit eine CEB-Konfig-System-Achse, egal was ihr Kommentar behauptet. Der
// Mess-Realm braucht deshalb seine eigene Wurzel, nicht bloss ein umgehaengtes Etikett. RF-1
// wortgetreu: "Mess-Achsen im Planer VOELLIG getrennt von CEB-System-/Organ-Achsen" -- je Realm eine
// eigene Meta-Meta-Kategorie (system_meta_meta auf der System-Seite, measurement_meta_meta hier).
//
// ABGRENZUNG zu den drei benachbarten Wurzeln unter demselben Dach topics::Axis:
//   * SystemAxis          -- MESS-Achsen im Sinne "Blut" (host-seitige Messquellen, golden-neutral)
//   * CebSystemAxis       -- CEB-Konfig-System-Achsen (Bau-/Steuer-Parameter der Tier-Binaries)
//   * SystemMetaMetaAxis  -- System-Meta-Metas unter dem external_utils-Hub (SIMD/AVX, externe HW)
//   * MeasurementMetaMetaAxis (HIER) -- Meta-Meta-HAUPT-Achsen des PLANERS: sie beschreiben, WOGEGEN
//     gemessen wird. Der Planer generiert daraus die Laeufe und DELEGIERT ans CEB-Interface, das
//     erfaehrt, welche Binary-Rekombination gegen welche Lasten zu messen ist (Owner-KERN R-G).
//
// Erste und aktuell einzige Traegerin: load_framework (K1-Umzug, load_framework_measurement_axis.hpp).
// Sie hat die System-Welt mit A3 ERSATZLOS verlassen -- kein Platzhalter in kSystemAxisOrder, und
// ausdruecklich KEIN Glied des external_utils-Hubs (der traegt nur System-Meta-Metas).
//
// binary_id-NEUTRAL: Mess-Realm-Achsen permutieren die Organ-Komposition nicht und stehen nie in
// kCompositionAxisNames. Metaprog: CRTP + Concept, static-dispatch, keine vtable.

#pragma once

#include "../../../topics/axis.hpp"

#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

/// Wurzel der Mess-Realm-Meta-Meta-HAUPT-Achsen. Gegenstueck zu CebSystemAxis auf der Mess-Seite:
/// identische Mechanik (CRTP, leeres Dach, static-dispatch), anderer Realm-Diskriminator.
template <class Derived>
struct MeasurementMetaMetaAxis : topics::Axis<Derived> {
    [[nodiscard]] static constexpr topics::AxisKind axis_kind() noexcept {
        return topics::AxisKind::measurement_meta_meta;
    }

    /// Compile-time-Reflexion des Achsen-Etiketts; reiner static-dispatch via Derived.
    [[nodiscard]] static constexpr std::string_view axis_label() noexcept { return Derived::do_axis_label(); }

protected:
    constexpr MeasurementMetaMetaAxis() noexcept = default;
};

template <class A>
concept MeasurementMetaMetaAxisConcept =
    topics::AxisConcept<A> && std::derived_from<A, MeasurementMetaMetaAxis<A>> &&
    std::is_empty_v<MeasurementMetaMetaAxis<A>> && (!std::is_polymorphic_v<MeasurementMetaMetaAxis<A>>) && requires {
        { A::do_axis_label() } -> std::convertible_to<std::string_view>;
    };

} // namespace comdare::cache_engine::measurement
