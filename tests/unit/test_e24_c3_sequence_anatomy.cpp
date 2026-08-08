// test_e24_c3_sequence_anatomy -- E-24 / S12.1, Stufe C3 (Produktionstiefe der Sequence-Gattung, 2026-08-04).
//
// GEGENSTAND identisch zum Set-Geschwister tests/unit/test_e24_c3_set_anatomy.cpp (Bauplan-Dossier
// docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C3 + Gate Paragraf 4.2):
//   (A) PRAESENZ-WACHE  -- axis_organ_names() elementweise gegen GenusBindingTraits<Sequence>::axis_names()
//                          + je Slot der Accessor-Rueckgabetyp gegen den Kompositions-Slot-Typ.
//   (B) SLOT-PIN        -- 9 (container_framework.hpp:95), aus der Bau-Bruecke gerechnet.
//   (C) OBSERVE-VERDRAHTUNG (Luecke-L2-Rest) -- observe_axes() sammelt Slot fuer Slot ein.
//   (D) ABI-NEUTRALITAET -- observe_all() liefert unveraendert den flachen SequenceObserverSnapshot.
//   (E) SequenceExecutionContext -- die builder-seitige Treiber-Flaeche liefert beide Ebenen.
//
// SEQUENCE-SPEZIFISCHE ZUGABE: der growth_policy-Slot ist das REAL GETRIEBENE Achsen-Organ dieser Gattung
// (er steuert ueber next_capacity die growth_events). Diese TU belegt, dass der C3-Umbau ihn unveraendert
// treibt -- die Produktionstiefe darf die bestehende Achsen-Wirkung nicht verlieren.
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes (Liste
// COMDARE_GOALV6_BOOST_DTESTS in tests/unit/CMakeLists.txt -- Waisen-TU-Lehre: registriert).

#include "anatomy/sequence_anatomy.hpp"

#include "builder/anatomy_commands/sequence_execution_context.hpp" // E-24 C3: die Treiber-Flaeche
#include "builder/experiment_tree/container_type_traits.hpp"       // SF-1: Slot-Pins (Gate-Klasse IV), type_traits<G>
#include "builder/experiment_tree/genus_binding_traits.hpp"        // autoritative axis_names()

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen der
// gleichnamigen Fixture-Typen ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace cea = comdare::cache_engine::anatomy;
namespace cmd = comdare::cache_engine::builder::anatomy_commands;
namespace ex  = comdare::cache_engine::builder::experiment; // SF-1: liefert auch type_traits<G> (Adapter)

static int g_fail = 0;

template <class A, class B>
static void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

static void check_true(char const* what, bool cond) {
    std::cout << (cond ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!cond) ++g_fail;
}

struct PlainAxis {};

struct CountingAxisSnapshot {
    std::uint64_t touches = 0;
};

/// ObservableProbeAxis -- Minimalmodell von ObservableAxis (observer_aggregate.hpp:39-43).
struct ObservableProbeAxis {
    using snapshot_t = CountingAxisSnapshot;

    void                     touch() noexcept { ++snap_.touches; }
    [[nodiscard]] snapshot_t statistics() const noexcept { return snap_; }

private:
    snapshot_t snap_{};
};

using SeqTraits = ex::GenusBindingTraits<cea::AnatomyGenus::Sequence>;

/// Stumme Referenz-Komposition (Default-Growth ueber den Default-Template-Parameter).
using StillComp =
    SeqTraits::CompositionFor<PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis>;
using StillOrgan = SeqTraits::AnatomyFor<StillComp>;

/// Dieselbe Gattung mit einer BEOBACHTBAREN Achse im memory_layout-Slot (Slot 0).
using ObservedComp  = SeqTraits::CompositionFor<ObservableProbeAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                                PlainAxis, PlainAxis, PlainAxis>;
using ObservedOrgan = SeqTraits::AnatomyFor<ObservedComp>;

// =============================================================================================
// (A) PRAESENZ-WACHE
// =============================================================================================
static_assert(StillOrgan::axis_organ_names().size() == SeqTraits::axis_names().size(),
              "E-24 C3: jeder Achsen-Slot der Sequence-Gattung braucht ein reales Organ-Member.");

[[nodiscard]] constexpr bool axis_names_match() noexcept {
    auto const& declared  = StillOrgan::axis_organ_names();
    auto const& canonical = SeqTraits::axis_names();
    for (std::size_t i = 0; i < declared.size(); ++i) {
        if (declared[i] != canonical[i]) return false;
    }
    return true;
}
static_assert(axis_names_match(),
              "E-24 C3: SequenceAnatomy::axis_organ_names() muss GenusBindingTraits<Sequence>::axis_names() "
              "elementweise treffen (Reihenfolge inklusive).");

static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().memory_layout_organ()), StillComp::memory_layout&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().allocator_organ()), StillComp::allocator&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().prefetch_organ()), StillComp::prefetch&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().concurrency_organ()), StillComp::concurrency&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().serialization_organ()), StillComp::serialization&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().value_handle_organ()), StillComp::value_handle&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().io_dispatch_organ()), StillComp::io_dispatch&>);
static_assert(
    std::is_same_v<decltype(std::declval<StillOrgan&>().migration_policy_organ()), StillComp::migration_policy&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().growth_policy_organ()), StillComp::growth_policy&>);
static_assert(
    std::is_same_v<decltype(std::declval<StillOrgan const&>().growth_policy_organ()), StillComp::growth_policy const&>);

// =============================================================================================
// (B) SLOT-PIN bleibt gruen (Gate-Klasse IV)
// =============================================================================================
static_assert(ex::type_traits<cea::AnatomyGenus::Sequence>::slot_count == StillOrgan::organ_count());
static_assert(StillOrgan::axis_organ_names().size() == ex::type_traits<cea::AnatomyGenus::Sequence>::slot_count);
static_assert(StillOrgan::axis_observation_t::total_slots() == StillOrgan::organ_count());

// =============================================================================================
// (C) OBSERVE-VERDRAHTUNG + der C0/C1-Vertrag bleibt unverletzt
// =============================================================================================
static_assert(StillOrgan::observable_axis_count() == 0);
static_assert(ObservedOrgan::observable_axis_count() == 1);
static_assert(
    std::is_same_v<decltype(cea::SequenceAxisObservation<ObservedComp>::memory_layout), CountingAxisSnapshot>);
static_assert(std::is_same_v<decltype(cea::SequenceAxisObservation<ObservedComp>::allocator), cea::EmptyAxisSnapshot>,
              "eine Achse ohne statistics() bleibt Empty -- die Anatomie erfindet keinen Wert");

static_assert(cea::OrganConcept<StillOrgan> && cea::IndexedOrganOps<StillOrgan> && cea::OrganOpSurface<StillOrgan>);
static_assert(cea::organ_op_family_count<StillOrgan>() == 1,
              "C1-Diagonale haelt: die Produktionstiefe stellt die Gattung in GENAU eine Op-Teilmenge");
static_assert(!cea::AxisOrganAccessOps<StillOrgan>,
              "Q-8-Anker: eine Container-Gattung traegt kein persistence_target/mapping/queuing-Organ");
static_assert(std::is_base_of_v<cea::OrganGuard<StillOrgan>, StillOrgan>);
static_assert(std::is_same_v<cea::organ_snapshot_t<StillOrgan>, cea::SequenceObserverSnapshot>,
              "ABI-neutral: observe_all() liefert unveraendert den flachen Gattungs-POD");
static_assert(!std::is_polymorphic_v<StillOrgan>);

} // namespace

int main() {
    std::cout << "E-24 C3: Produktionstiefe der Sequence-Gattung (Organ-Member + observe-Verdrahtung):\n";

    // -- (D) ABI-NEUTRALITAET: die flache Sicht zaehlt wie vor C3, inkl. der REALEN growth-Wirkung --
    StillOrgan seq{};
    for (std::uint64_t i = 0; i < 5; ++i) seq.push_back(100 + i);
    check_eq("size nach 5x push_back", seq.size(), std::size_t{5});
    auto const hit = seq.at(2); // bewusst EIN Aufruf je Probe -- at() zaehlt jeden Zugriff mit
    check_true("at(2) trifft", hit.has_value() && *hit == 102u);
    auto const oob = seq.at(99);
    check_true("at(99) ist out-of-bounds", !oob.has_value());
    cea::SequenceObserverSnapshot const flat = seq.observe_all();
    check_eq("flach: push_count", flat.push_count, std::uint64_t{5});
    check_eq("flach: at_count", flat.at_count, std::uint64_t{2}); // at(2) + at(99)
    check_eq("flach: at_oob_count", flat.at_oob_count, std::uint64_t{1});
    check_eq("flach: current_size", flat.current_size, std::uint64_t{5});
    check_eq("flach: peak_size", flat.peak_size, std::uint64_t{5});
    check_true("flach: growth_events > 0 (die growth_policy-Achse wirkt real)", flat.growth_events > 0);

    // -- (A-Beleg) der growth_policy-Accessor zeigt auf das getriebene Achsen-Organ --
    check_eq("growth_policy_organ().growth_factor() (DoublingGrowth-Default)",
             seq.growth_policy_organ().growth_factor(), 2.0);
    check_true("Accessor liefert Referenz, keine Kopie",
               &seq.growth_policy_organ() == &static_cast<StillOrgan const&>(seq).growth_policy_organ());

    // -- (C-Beleg) die per-Achsen-Sicht sammelt real ein --
    std::cout << "\nper-Achsen-Sicht (observe_axes, Luecke-L2-Rest):\n";
    check_eq("stumme Komposition: observable_axis_count", StillOrgan::observable_axis_count(), std::size_t{0});
    ObservedOrgan observed{};
    observed.memory_layout_organ().touch();
    observed.memory_layout_organ().touch();
    ObservedOrgan::axis_observation_t const axes = observed.observe_axes();
    check_eq("beobachtbarer memory_layout-Slot: touches im per-Achsen-Aggregat", axes.memory_layout.touches,
             std::uint64_t{2});
    check_eq("beobachtbare Komposition: observable_axis_count", ObservedOrgan::observable_axis_count(), std::size_t{1});
    check_eq("total_slots aus der Komposition gerechnet", ObservedOrgan::axis_observation_t::total_slots(),
             std::size_t{9});
    check_eq("flache Sicht bleibt unberuehrt: push_count", observed.observe_all().push_count, std::uint64_t{0});

    // -- (E) SequenceExecutionContext --
    std::cout << "\nSequenceExecutionContext (builder-seitige Treiber-Flaeche):\n";
    cmd::SequenceExecutionContext<ObservedComp> ctx;
    ctx.push_back(11);
    ctx.push_back(12);
    ctx.push_back(13);
    check_eq("ctx.size()", ctx.size(), std::size_t{3});
    auto const ctx_hit = ctx.at(1);
    check_true("ctx.at(1) == 12", ctx_hit.has_value() && *ctx_hit == 12u);
    check_true("ctx.empty() == false", !ctx.empty());
    ctx.anatomy().memory_layout_organ().touch();
    check_eq("ctx.observe_all().push_count (flache Ebene)", ctx.observe_all().push_count, std::uint64_t{3});
    check_eq("ctx.observe_axes().memory_layout.touches (per-Achsen-Ebene)", ctx.observe_axes().memory_layout.touches,
             std::uint64_t{1});
    check_true("ctx.genus() == Sequence", ctx.genus() == cea::AnatomyGenus::Sequence);
    check_eq("ctx.organ_count()", ctx.organ_count(), std::size_t{9});
    ctx.clear();
    check_true("ctx.clear() leert die Sequenz", ctx.empty());

    std::cout << "\n==== E-24 C3 Sequence-Produktionstiefe: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
