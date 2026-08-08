// test_e24_c3_adapter_anatomy -- E-24 / S12.1, Stufe C3 (Produktionstiefe der Adapter-Gattung, 2026-08-04).
//
// GEGENSTAND identisch zu den drei Geschwistern (Bauplan-Dossier
// docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C3 + Gate 4.2):
//   (A) PRAESENZ-WACHE  -- axis_organ_names() elementweise gegen GenusBindingTraits<Adapter>::axis_names()
//                          + je Slot der Accessor-Rueckgabetyp gegen den Kompositions-Slot-Typ.
//   (B) SLOT-PIN        -- 11 (container_framework.hpp:93), aus der Bau-Bruecke gerechnet.
//   (C) OBSERVE-VERDRAHTUNG (Luecke-L2-Rest) -- observe_axes() sammelt Slot fuer Slot ein.
//   (D) ABI-NEUTRALITAET -- observe_all() liefert unveraendert den flachen AdapterObserverSnapshot.
//   (E) AdapterExecutionContext -- die builder-seitige Treiber-Flaeche liefert beide Ebenen.
//
// ADAPTER-SPEZIFISCHE ZUGABE: der inner_container-Slot ist das REAL GETRIEBENE Achsen-Organ, und die
// Disziplin (FIFO/LIFO/Priority) ist API-NUTZUNG, keine Achse (adapter_anatomy.hpp:10-12). Diese TU faehrt
// den C3-umgebauten Member deshalb ueber ALLE DREI Inner-Substrate (Deque/Vector/Heap) und belegt, dass
// die Priority-Disziplin des HeapInner den Umbau unbeschadet ueberstanden hat.
//
// FROZEN-LEGACY-VERMERK (bewusst NICHT angefasst, Aufraeumpass-Kandidat): kAdapterCompositionSlotCount ist
// 13, die LIVE-Slot-Zahl 11. Diese TU pinnt beide Werte GETRENNT und benennt die Differenz, statt sie
// stillschweigend anzugleichen -- eine Angleichung waere eine Bestandsaenderung ausserhalb des C3-Schnitts.
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes (Liste
// COMDARE_GOALV6_BOOST_DTESTS in tests/unit/CMakeLists.txt -- Waisen-TU-Lehre: registriert).

#include "anatomy/adapter_anatomy.hpp"

#include "builder/anatomy_commands/adapter_execution_context.hpp" // E-24 C3: die Treiber-Flaeche
#include "builder/experiment_tree/container_type_traits.hpp"      // SF-1: Slot-Pins (Gate-Klasse IV), type_traits<G>
#include "builder/experiment_tree/genus_binding_traits.hpp"       // autoritative axis_names()

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

using AdaTraits = ex::GenusBindingTraits<cea::AnatomyGenus::Adapter>;

/// Stumme Referenz-Komposition (inner_container kommt aus dem Default-Template-Parameter: DequeInner).
using StillComp = AdaTraits::CompositionFor<PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                            PlainAxis, PlainAxis, PlainAxis>;
using StillOrgan = AdaTraits::AnatomyFor<StillComp>;

/// Dieselbe Gattung mit einer BEOBACHTBAREN Achse im memory_layout-Slot (Slot 2).
using ObservedComp  = AdaTraits::CompositionFor<PlainAxis, PlainAxis, ObservableProbeAxis, PlainAxis, PlainAxis,
                                                PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis>;
using ObservedOrgan = AdaTraits::AnatomyFor<ObservedComp>;

/// Die drei Inner-Substrate der spezifischen Achse (Paragraf 26.4) am umgebauten Member.
using HeapComp  = cea::AdapterComposition<PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                          PlainAxis, PlainAxis, PlainAxis, cea::HeapInner<>>;
using HeapOrgan = cea::AdapterAnatomy<HeapComp>;
using VecComp   = cea::AdapterComposition<PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                          PlainAxis, PlainAxis, PlainAxis, cea::VectorInner<>>;
using VecOrgan  = cea::AdapterAnatomy<VecComp>;

// =============================================================================================
// (A) PRAESENZ-WACHE
// =============================================================================================
static_assert(StillOrgan::axis_organ_names().size() == AdaTraits::axis_names().size(),
              "E-24 C3: jeder Achsen-Slot der Adapter-Gattung braucht ein reales Organ-Member.");

[[nodiscard]] constexpr bool axis_names_match() noexcept {
    auto const& declared  = StillOrgan::axis_organ_names();
    auto const& canonical = AdaTraits::axis_names();
    for (std::size_t i = 0; i < declared.size(); ++i) {
        if (declared[i] != canonical[i]) return false;
    }
    return true;
}
static_assert(axis_names_match(),
              "E-24 C3: AdapterAnatomy::axis_organ_names() muss GenusBindingTraits<Adapter>::axis_names() "
              "elementweise treffen (Reihenfolge inklusive).");

static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().search_algo_organ()), StillComp::search_algo&>);
static_assert(
    std::is_same_v<decltype(std::declval<StillOrgan&>().cache_traversal_organ()), StillComp::cache_traversal&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().memory_layout_organ()), StillComp::memory_layout&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().allocator_organ()), StillComp::allocator&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().prefetch_organ()), StillComp::prefetch&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().concurrency_organ()), StillComp::concurrency&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().serialization_organ()), StillComp::serialization&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().value_handle_organ()), StillComp::value_handle&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().io_dispatch_organ()), StillComp::io_dispatch&>);
static_assert(
    std::is_same_v<decltype(std::declval<StillOrgan&>().migration_policy_organ()), StillComp::migration_policy&>);
static_assert(
    std::is_same_v<decltype(std::declval<StillOrgan&>().inner_container_organ()), StillComp::inner_container&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan const&>().inner_container_organ()),
                             StillComp::inner_container const&>);

// =============================================================================================
// (B) SLOT-PIN bleibt gruen (Gate-Klasse IV) -- LIVE 11, frozen legacy 13, beide getrennt gepinnt
// =============================================================================================
static_assert(ex::type_traits<cea::AnatomyGenus::Adapter>::slot_count == StillOrgan::organ_count());
static_assert(StillOrgan::axis_organ_names().size() == ex::type_traits<cea::AnatomyGenus::Adapter>::slot_count);
static_assert(StillOrgan::axis_observation_t::total_slots() == StillOrgan::organ_count());
static_assert(cea::kAdapterCompositionSlotCount == 13,
              "frozen legacy Paragraf-28-Count -- BEWUSST nicht an die live 11 angeglichen (Aufraeumpass)");
static_assert(cea::kAdapterCompositionSlotCount != StillOrgan::organ_count(),
              "die Differenz ist benannt, nicht still: 13 (frozen) vs. 11 (live)");

// =============================================================================================
// (C) OBSERVE-VERDRAHTUNG + der C0/C1-Vertrag bleibt unverletzt
// =============================================================================================
static_assert(StillOrgan::observable_axis_count() == 0);
static_assert(ObservedOrgan::observable_axis_count() == 1);
static_assert(std::is_same_v<decltype(cea::AdapterAxisObservation<ObservedComp>::memory_layout), CountingAxisSnapshot>);
static_assert(
    std::is_same_v<decltype(cea::AdapterAxisObservation<ObservedComp>::inner_container), cea::EmptyAxisSnapshot>,
    "DequeInner hat kein statistics() -- der Slot bleibt Empty, die Anatomie erfindet keinen Wert");

static_assert(cea::OrganConcept<StillOrgan> && cea::AdaptedOrganOps<StillOrgan> && cea::OrganOpSurface<StillOrgan>);
static_assert(cea::organ_op_family_count<StillOrgan>() == 1,
              "C1-Diagonale haelt: die Produktionstiefe stellt die Gattung in GENAU eine Op-Teilmenge");
static_assert(!cea::AxisOrganAccessOps<StillOrgan>,
              "Q-8-Anker: eine Container-Gattung traegt kein persistence_target/mapping/queuing-Organ -- "
              "obwohl die Adapter-Gattung als einzige der vier ein search_algo_organ() hat");
static_assert(std::is_base_of_v<cea::OrganGuard<StillOrgan>, StillOrgan>);
static_assert(std::is_same_v<cea::organ_snapshot_t<StillOrgan>, cea::AdapterObserverSnapshot>,
              "ABI-neutral: observe_all() liefert unveraendert den flachen Gattungs-POD");
static_assert(!std::is_polymorphic_v<StillOrgan>);
static_assert(StillOrgan::gattung() == cea::AnatomyGattung::Container, "Ebene-1-Marker bleibt");

} // namespace

int main() {
    std::cout << "E-24 C3: Produktionstiefe der Adapter-Gattung (Organ-Member + observe-Verdrahtung):\n";

    // -- (D) ABI-NEUTRALITAET: die flache Sicht zaehlt wie vor C3 (Werte aus test_container_genus.cpp) --
    StillOrgan queue{};
    queue.push(10);
    queue.push(20);
    queue.push(30);
    check_eq("size == 3 nach 3x push", queue.size(), std::size_t{3});
    auto const f = queue.pop_front(); // FIFO
    check_true("FIFO pop_front() liefert 10", f.has_value() && *f == 10u);
    auto const b = queue.pop_back(); // LIFO
    check_true("LIFO pop_back() liefert 30", b.has_value() && *b == 30u);
    cea::AdapterObserverSnapshot const flat = queue.observe_all();
    check_eq("flach: push_count", flat.push_count, std::uint64_t{3});
    check_eq("flach: pop_count", flat.pop_count, std::uint64_t{2});
    check_eq("flach: front_reads", flat.front_reads, std::uint64_t{1});
    check_eq("flach: back_reads", flat.back_reads, std::uint64_t{1});
    check_eq("flach: peak_occupancy", flat.peak_occupancy, std::uint64_t{3});
    check_eq("flach: current_occupancy", flat.current_occupancy, std::uint64_t{1});

    // -- (A-Beleg) der inner_container-Accessor zeigt auf das getriebene Achsen-Organ --
    check_eq("inner_container_organ().size() == die getriebene Fuellung", queue.inner_container_organ().size(),
             std::size_t{1});
    check_true("Accessor liefert Referenz, keine Kopie",
               &queue.inner_container_organ() == &static_cast<StillOrgan const&>(queue).inner_container_organ());

    // -- (Adapter-spezifisch) alle drei Inner-Substrate am umgebauten Member --
    std::cout << "\ndie drei Inner-Substrate der spezifischen Achse (Paragraf 26.4) nach dem C3-Umbau:\n";
    VecOrgan vec{};
    vec.push(7);
    vec.push(9);
    auto const vpop = vec.pop_back();
    check_true("VectorInner: pop_back() liefert 9", vpop.has_value() && *vpop == 9u);
    HeapOrgan heap{};
    heap.push(10);
    heap.push(30);
    heap.push(20);
    auto const hmax = heap.front();
    check_true("HeapInner: front() == 30 (Max-Heap-Disziplin haelt nach dem Umbau)", hmax.has_value() && *hmax == 30u);
    auto const h1 = heap.pop_front();
    auto const h2 = heap.pop_front();
    check_true("HeapInner: Extract-Max-Folge 30 dann 20", h1.has_value() && *h1 == 30u && h2.has_value() && *h2 == 20u);

    // -- (C-Beleg) die per-Achsen-Sicht sammelt real ein --
    std::cout << "\nper-Achsen-Sicht (observe_axes, Luecke-L2-Rest):\n";
    check_eq("stumme Komposition: observable_axis_count", StillOrgan::observable_axis_count(), std::size_t{0});
    ObservedOrgan observed{};
    observed.memory_layout_organ().touch();
    observed.memory_layout_organ().touch();
    observed.memory_layout_organ().touch();
    observed.memory_layout_organ().touch();
    ObservedOrgan::axis_observation_t const axes = observed.observe_axes();
    check_eq("beobachtbarer memory_layout-Slot: touches im per-Achsen-Aggregat", axes.memory_layout.touches,
             std::uint64_t{4});
    check_eq("beobachtbare Komposition: observable_axis_count", ObservedOrgan::observable_axis_count(), std::size_t{1});
    check_eq("total_slots aus der LIVE-Komposition gerechnet", ObservedOrgan::axis_observation_t::total_slots(),
             std::size_t{11});
    check_eq("flache Sicht bleibt unberuehrt: push_count", observed.observe_all().push_count, std::uint64_t{0});

    // -- (E) AdapterExecutionContext --
    std::cout << "\nAdapterExecutionContext (builder-seitige Treiber-Flaeche):\n";
    cmd::AdapterExecutionContext<ObservedComp> ctx;
    ctx.push(1);
    ctx.push(2);
    ctx.push(3);
    check_eq("ctx.size()", ctx.size(), std::size_t{3});
    auto const ctx_front = ctx.front();
    check_true("ctx.front() == 1 (FIFO-Ende)", ctx_front.has_value() && *ctx_front == 1u);
    auto const ctx_back = ctx.back();
    check_true("ctx.back() == 3 (LIFO-Ende)", ctx_back.has_value() && *ctx_back == 3u);
    auto const ctx_pop = ctx.pop_front();
    check_true("ctx.pop_front() == 1", ctx_pop.has_value() && *ctx_pop == 1u);
    ctx.anatomy().memory_layout_organ().touch();
    check_eq("ctx.observe_all().push_count (flache Ebene)", ctx.observe_all().push_count, std::uint64_t{3});
    check_eq("ctx.observe_axes().memory_layout.touches (per-Achsen-Ebene)", ctx.observe_axes().memory_layout.touches,
             std::uint64_t{1});
    check_true("ctx.genus() == Adapter", ctx.genus() == cea::AnatomyGenus::Adapter);
    check_true("ctx.gattung() == Container (Ebene 1)", ctx.gattung() == cea::AnatomyGattung::Container);
    check_eq("ctx.organ_count()", ctx.organ_count(), std::size_t{11});
    ctx.clear();
    check_true("ctx.clear() leert den Adapter", ctx.empty());

    std::cout << "\n==== E-24 C3 Adapter-Produktionstiefe: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
