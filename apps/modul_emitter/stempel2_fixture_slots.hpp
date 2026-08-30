#pragma once
// STEMPEL TEIL 2 (B-7/RN-78 Emitter-Haelfte, Weiche A) -- die stampbaren HUELLEN-TYPEN der C2-Vertragspaare
// fuer die genus-EIGENEN Achsen der Container-Gattungen Sequence (axis_growth), View (axis_extent/layout/
// accessor) und Adapter (inner_container).
//
// WARUM ES DIESE DATEI GIBT (Objekt-Befund 27.08.2026, ce d3b5a393): die Organ-Zeile einer Gattung wird aus
// name() + algo_version ihrer Slot-Typen gebildet (modul_emitter.hpp, dieselbe Grammatik wie
// abi::organ_stamp_line<Comp>). Die 18 SearchAlgorithm-Achsen tragen beides an ihren Registry-Wrappern; die
// genus-eigenen Achsen der drei Container-Gattungen tragen es NICHT: anatomy/DoublingGrowth, DynamicExtent,
// LayoutRight, DefaultAccessor und DequeInner<> (dort ist 'name' ein Daten-Member) ebenso wenig wie die
// topics/-Registries axis_growth_registry.hpp / view_registries.hpp -- und topics/ ist TABU-Menge (832er
// Manifest). Eine reale Sequence-/View-/Adapter-Komposition ist damit heute NICHT stempelbar.
//
// WAS DIESE HUELLEN SIND UND NICHT SIND: sie ERBEN das Verhalten der Anatomie-Defaults byte-gleich (die
// Anatomien treiben next_capacity / index_of / access / push_back-front-pop_front ueber die Basisklasse) und
// deklarieren dazu EHRLICH benannt name() + algo_version -- also genau das, was die produktive Stempelbarkeit
// dieser Achsen eines Tages an ihren Varianten tragen wird. Sie sind Fixture-Seite (nur der CLI-Emitter und
// die daraus emittierten Vertragspaar-Module sehen sie), KEIN Produktionscode und KEIN Ersatz fuer den
// W4/W7-Posten "genus-eigene Achsen stampbar machen" (TABU-beruehrend, eigener Zug).
//
// KONVENTION: name() als FUNKTION (wie jeder Organ-Achsen-Wrapper, SlotStampbar-Concept), algo_version
// in Flag-Grammatik v2 mit CPU-Flag (ce-eigen, COMDARE_VERSION_HW_FLAG_ENFORCE).
//
// @doku ~/backups-workflow/20260826-stempel-teil2/STAND.md (Objekt-Befund Stempelbarkeit)

#include <anatomy/adapter_anatomy.hpp>      // DequeInner<>
#include <anatomy/sequence_composition.hpp> // DoublingGrowth
#include <anatomy/view_composition.hpp>     // DynamicExtent / LayoutRight / DefaultAccessor

#include <string_view>

namespace comdare::cache_engine::stempel2_fixture {

/// axis_growth (Sequence): std::vector-Standard x2, Verhalten = anatomy::DoublingGrowth.
struct DoublingGrowthStampbar : ::comdare::cache_engine::anatomy::DoublingGrowth {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "doubling_growth"; }
    static constexpr std::string_view               algo_version = "1.0.0.c";
};

/// axis_extent (View): Ausdehnung erst zur Laufzeit (bind) bekannt, Verhalten = anatomy::DynamicExtent.
struct DynamicExtentStampbar : ::comdare::cache_engine::anatomy::DynamicExtent {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "dynamic_extent"; }
    static constexpr std::string_view               algo_version = "1.0.0.c";
};

/// axis_layout (View): row-major 1D, Verhalten = anatomy::LayoutRight.
struct LayoutRightStampbar : ::comdare::cache_engine::anatomy::LayoutRight {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "layout_right"; }
    static constexpr std::string_view               algo_version = "1.0.0.c";
};

/// axis_accessor (View): direkter Element-Zugriff, Verhalten = anatomy::DefaultAccessor.
struct DefaultAccessorStampbar : ::comdare::cache_engine::anatomy::DefaultAccessor {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "default_accessor"; }
    static constexpr std::string_view               algo_version = "1.0.0.c";
};

/// inner_container (Adapter): FIFO-Substrat, Verhalten = anatomy::DequeInner<>. Das Daten-Member 'name'
/// der Basis wird hier durch die FUNKTION name() verdeckt -- die Organ-Zeilen-Grammatik verlangt die
/// Funktionsform (SlotStampbar); das Adapter-Organ selbst liest nur front/back/push/pop/size/clear.
struct DequeInnerStampbar : ::comdare::cache_engine::anatomy::DequeInner<> {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "deque_inner"; }
    static constexpr std::string_view               algo_version = "1.0.0.c";
};

} // namespace comdare::cache_engine::stempel2_fixture
