// test_e24_c3_view_anatomy -- E-24 / S12.1, Stufe C3 (Produktionstiefe der View-Gattung, 2026-08-04).
//
// GEGENSTAND identisch zu den Geschwistern test_e24_c3_set_anatomy.cpp / test_e24_c3_sequence_anatomy.cpp
// (Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C3 + Gate 4.2):
//   (A) PRAESENZ-WACHE  -- axis_organ_names() elementweise gegen GenusBindingTraits<View>::axis_names()
//                          + je Slot der Accessor-Rueckgabetyp gegen den Kompositions-Slot-Typ.
//   (B) SLOT-PIN        -- 5 (container_framework.hpp:96), aus der Bau-Bruecke gerechnet.
//   (C) OBSERVE-VERDRAHTUNG (Luecke-L2-Rest) -- observe_axes() sammelt Slot fuer Slot ein.
//   (D) ABI-NEUTRALITAET -- observe_all() liefert unveraendert den flachen ViewObserverSnapshot.
//   (E) ViewExecutionContext -- die builder-seitige Treiber-Flaeche liefert beide Ebenen.
//
// VIEW-SPEZIFISCHE ZUGABE: layout_policy und accessor_policy sind die REAL GETRIEBENEN Achsen-Organe
// (read() laeuft ueber index_of + access). Diese TU belegt, dass der C3-Umbau sie unveraendert treibt --
// die Produktionstiefe darf die bestehende Achsen-Wirkung nicht verlieren. Und: die Gattung bleibt
// non-owning (kein clear, kein eigener Puffer), was C1 als OrganClearable-Gegenprobe gepinnt hat.
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes (Liste
// COMDARE_GOALV6_BOOST_DTESTS in tests/unit/CMakeLists.txt -- Waisen-TU-Lehre: registriert).

#include "anatomy/view_anatomy.hpp"

#include "builder/anatomy_commands/view_execution_context.hpp" // E-24 C3: die Treiber-Flaeche
#include "builder/experiment_tree/container_type_traits.hpp"   // SF-1: Slot-Pins (Gate-Klasse IV), type_traits<G>
#include "builder/experiment_tree/genus_binding_traits.hpp"    // autoritative axis_names()

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

using ViewTraits = ex::GenusBindingTraits<cea::AnatomyGenus::View>;

/// Stumme Referenz-Komposition (extent/layout/accessor kommen aus den Default-Template-Parametern,
/// also die ECHTEN Policy-Organe DynamicExtent/LayoutRight/DefaultAccessor).
using StillComp  = ViewTraits::CompositionFor<PlainAxis, PlainAxis>;
using StillOrgan = ViewTraits::AnatomyFor<StillComp>;

/// Dieselbe Gattung mit einer BEOBACHTBAREN Achse im memory_layout-Slot (Slot 0).
using ObservedComp  = ViewTraits::CompositionFor<ObservableProbeAxis, PlainAxis>;
using ObservedOrgan = ViewTraits::AnatomyFor<ObservedComp>;

// =============================================================================================
// (A) PRAESENZ-WACHE
// =============================================================================================
static_assert(StillOrgan::axis_organ_names().size() == ViewTraits::axis_names().size(),
              "E-24 C3: jeder Achsen-Slot der View-Gattung braucht ein reales Organ-Member.");

[[nodiscard]] constexpr bool axis_names_match() noexcept {
    auto const& declared  = StillOrgan::axis_organ_names();
    auto const& canonical = ViewTraits::axis_names();
    for (std::size_t i = 0; i < declared.size(); ++i) {
        if (declared[i] != canonical[i]) return false;
    }
    return true;
}
static_assert(axis_names_match(),
              "E-24 C3: ViewAnatomy::axis_organ_names() muss GenusBindingTraits<View>::axis_names() "
              "elementweise treffen (Reihenfolge inklusive).");

static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().memory_layout_organ()), StillComp::memory_layout&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().value_handle_organ()), StillComp::value_handle&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().extent_policy_organ()), StillComp::extent_policy&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().layout_policy_organ()), StillComp::layout_policy&>);
static_assert(
    std::is_same_v<decltype(std::declval<StillOrgan&>().accessor_policy_organ()), StillComp::accessor_policy&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan const&>().accessor_policy_organ()),
                             StillComp::accessor_policy const&>);

// =============================================================================================
// (B) SLOT-PIN bleibt gruen (Gate-Klasse IV)
// =============================================================================================
static_assert(ex::type_traits<cea::AnatomyGenus::View>::slot_count == StillOrgan::organ_count());
static_assert(StillOrgan::axis_organ_names().size() == ex::type_traits<cea::AnatomyGenus::View>::slot_count);
static_assert(StillOrgan::axis_observation_t::total_slots() == StillOrgan::organ_count());

// =============================================================================================
// (C) OBSERVE-VERDRAHTUNG + der C0/C1-Vertrag bleibt unverletzt
// =============================================================================================
static_assert(StillOrgan::observable_axis_count() == 0);
static_assert(ObservedOrgan::observable_axis_count() == 1);
static_assert(std::is_same_v<decltype(cea::ViewAxisObservation<ObservedComp>::memory_layout), CountingAxisSnapshot>);
static_assert(std::is_same_v<decltype(cea::ViewAxisObservation<ObservedComp>::layout_policy), cea::EmptyAxisSnapshot>,
              "LayoutRight hat kein statistics() -- der Slot bleibt Empty, die Anatomie erfindet keinen Wert");

static_assert(cea::OrganConcept<StillOrgan> && cea::BoundViewOrganOps<StillOrgan> && cea::OrganOpSurface<StillOrgan>);
static_assert(cea::organ_op_family_count<StillOrgan>() == 1,
              "C1-Diagonale haelt: die Produktionstiefe stellt die Gattung in GENAU eine Op-Teilmenge");
static_assert(!cea::AxisOrganAccessOps<StillOrgan>,
              "Q-8-Anker: eine Container-Gattung traegt kein persistence_target/mapping/queuing-Organ");
static_assert(!cea::OrganClearable<StillOrgan>, "View bleibt non-owning -- kein clear (view_anatomy.hpp:4)");
static_assert(std::is_base_of_v<cea::OrganGuard<StillOrgan>, StillOrgan>);
static_assert(std::is_same_v<cea::organ_snapshot_t<StillOrgan>, cea::ViewObserverSnapshot>,
              "ABI-neutral: observe_all() liefert unveraendert den flachen Gattungs-POD");
static_assert(!std::is_polymorphic_v<StillOrgan>);

} // namespace

int main() {
    std::cout << "E-24 C3: Produktionstiefe der View-Gattung (Organ-Member + observe-Verdrahtung):\n";

    // -- (D) ABI-NEUTRALITAET: die flache Sicht zaehlt wie vor C3, ueber die REALEN Policy-Organe --
    std::uint64_t const buffer[4] = {70, 71, 72, 73};
    StillOrgan          view{};
    view.bind(buffer, 4);
    auto const hit = view.read(2);
    check_true("read(2) trifft (ueber layout_policy + accessor_policy)", hit.has_value() && *hit == 72u);
    auto const oob = view.read(99);
    check_true("read(99) ist out-of-bounds", !oob.has_value());
    cea::ViewObserverSnapshot const flat = view.observe_all();
    check_eq("flach: bind_count", flat.bind_count, std::uint64_t{1});
    check_eq("flach: bound_size", flat.bound_size, std::uint64_t{4});
    check_eq("flach: read_count", flat.read_count, std::uint64_t{2});
    check_eq("flach: read_oob_count", flat.read_oob_count, std::uint64_t{1});
    check_eq("size (non-owning: die gebundene Fremd-Groesse)", view.size(), std::size_t{4});

    // -- (A-Beleg) die Policy-Accessoren zeigen auf die getriebenen Achsen-Organe --
    check_eq("layout_policy_organ().index_of(3) (LayoutRight: Offset == Index)", view.layout_policy_organ().index_of(3),
             std::size_t{3});
    check_eq("accessor_policy_organ().access(buffer,1) (DefaultAccessor)",
             view.accessor_policy_organ().access(buffer, 1), std::uint64_t{71});
    check_true("extent_policy_organ().is_static() == false (DynamicExtent-Default)",
               !view.extent_policy_organ().is_static());
    check_true("Accessor liefert Referenz, keine Kopie",
               &view.layout_policy_organ() == &static_cast<StillOrgan const&>(view).layout_policy_organ());

    // -- (C-Beleg) die per-Achsen-Sicht sammelt real ein --
    std::cout << "\nper-Achsen-Sicht (observe_axes, Luecke-L2-Rest):\n";
    check_eq("stumme Komposition: observable_axis_count", StillOrgan::observable_axis_count(), std::size_t{0});
    ObservedOrgan observed{};
    observed.memory_layout_organ().touch();
    ObservedOrgan::axis_observation_t const axes = observed.observe_axes();
    check_eq("beobachtbarer memory_layout-Slot: touches im per-Achsen-Aggregat", axes.memory_layout.touches,
             std::uint64_t{1});
    check_eq("beobachtbare Komposition: observable_axis_count", ObservedOrgan::observable_axis_count(), std::size_t{1});
    check_eq("total_slots aus der Komposition gerechnet", ObservedOrgan::axis_observation_t::total_slots(),
             std::size_t{5});
    check_eq("flache Sicht bleibt unberuehrt: bind_count", observed.observe_all().bind_count, std::uint64_t{0});

    // -- (E) ViewExecutionContext (non-owning: der Puffer bleibt beim Aufrufer) --
    std::cout << "\nViewExecutionContext (builder-seitige Treiber-Flaeche, non-owning):\n";
    cmd::ViewExecutionContext<ObservedComp> ctx;
    check_true("ctx.empty() vor bind()", ctx.empty());
    ctx.bind(buffer, 4);
    auto const ctx_hit = ctx.read(0);
    check_true("ctx.read(0) == 70", ctx_hit.has_value() && *ctx_hit == 70u);
    check_eq("ctx.size()", ctx.size(), std::size_t{4});
    ctx.anatomy().memory_layout_organ().touch();
    check_eq("ctx.observe_all().read_count (flache Ebene)", ctx.observe_all().read_count, std::uint64_t{1});
    check_eq("ctx.observe_axes().memory_layout.touches (per-Achsen-Ebene)", ctx.observe_axes().memory_layout.touches,
             std::uint64_t{1});
    check_true("ctx.genus() == View", ctx.genus() == cea::AnatomyGenus::View);
    check_eq("ctx.organ_count()", ctx.organ_count(), std::size_t{5});

    std::cout << "\n==== E-24 C3 View-Produktionstiefe: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
