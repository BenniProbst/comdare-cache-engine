// test_e24_c3_set_anatomy -- E-24 / S12.1, Stufe C3 (Produktionstiefe der Set-Gattung, 2026-08-04).
//
// GEGENSTAND (Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C3 +
// Gate-Definition Paragraf 4.2 "je Genus: Organ-Member-Praesenz-Asserts gegen axis_names()-Liste"):
//   (A) PRAESENZ-WACHE -- die Anatomie haelt fuer JEDEN Slot ihrer Gattung ein reales Organ-Member mit
//       Accessor. Die Wache kreuzt die Praesenz-Deklaration SetAnatomy::axis_organ_names() elementweise
//       gegen die AUTORITATIVE Liste GenusBindingTraits<Set>::axis_names() und pinnt zusaetzlich je Slot
//       den Accessor-RUECKGABETYP gegen den Kompositions-Slot-Typ. Ein neuer Slot ohne Organ bricht hier.
//   (B) SLOT-PIN bleibt gruen: 13 (container_framework.hpp:94) -- aus der Bau-Bruecke gerechnet, nicht
//       als Literal gepflegt.
//   (C) OBSERVE-VERDRAHTUNG (Luecke-L2-Rest) -- observe_axes() sammelt die Beobachtung Slot fuer Slot ein,
//       exakt nach dem SA-Muster. Belegt an einem Slot MIT statistics() (echte Werte) und an den Slots
//       OHNE (EmptyAxisSnapshot, kein erfundener Wert).
//   (D) ABI-NEUTRALITAETS-PROBE -- observe_all() liefert unveraendert den flachen SetObserverSnapshot mit
//       derselben Semantik wie vor C3 (der Wire-Spiegel set_abi_adapter.hpp:66-80 bleibt gueltig).
//   (E) SetExecutionContext -- die builder-seitige Treiber-Flaeche liefert beide Beobachtungs-Ebenen.
//
// FIXTURE-UNABHAENGIGER ABLEITUNGSWEG (Lehre "gruene Tests zementieren alte Ordnung"): die Praesenz-Wache
// vergleicht zwei LISTEN miteinander statt gegen handgepflegte Strings, und die Slot-Zahl wird aus der
// Bau-Bruecke gezogen. Waechst die Gattung um einen Slot, bricht diese TU, bis das Organ existiert.
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes (Liste
// COMDARE_GOALV6_BOOST_DTESTS in tests/unit/CMakeLists.txt -- Waisen-TU-Lehre: registriert).

#include "anatomy/set_anatomy.hpp"

#include "anatomy/container_framework.hpp"                    // Slot-Pins 13/9/11/5 (Gate-Klasse IV)
#include "anatomy/set_default_organ.hpp"                      // SortedArrayKeySet: echtes search_algo-Kern-Organ
#include "builder/anatomy_commands/set_execution_context.hpp" // E-24 C3: die Treiber-Flaeche
#include "builder/experiment_tree/genus_binding_traits.hpp"   // autoritative axis_names() der Set-Gattung

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen der
// gleichnamigen Fixture-Typen ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace cea = comdare::cache_engine::anatomy;
namespace cco = comdare::container;
namespace cmd = comdare::cache_engine::builder::anatomy_commands;
namespace ex  = comdare::cache_engine::builder::experiment;

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

// ---------------------------------------------------------------------------------------------
// Achsen-Fixtures: eine Achse OHNE Beobachtung (der Normalfall am Ist) und eine MIT -- letztere ist
// das Minimalmodell dessen, was ObservableAxis fordert (observer_aggregate.hpp:39-43).
// ---------------------------------------------------------------------------------------------
struct PlainAxis {};

struct CountingAxisSnapshot {
    std::uint64_t touches = 0;
};

/// ObservableProbeAxis -- erfuellt ObservableAxis (snapshot_t + statistics()). Steht hier stellvertretend
/// fuer JEDE beobachtbare Achsen-Huelle, auch fuer die Cross-Genus-Huelle ObservableOrgan<X> (C3-Cross).
struct ObservableProbeAxis {
    using snapshot_t = CountingAxisSnapshot;

    void                     touch() noexcept { ++snap_.touches; }
    [[nodiscard]] snapshot_t statistics() const noexcept { return snap_; }

private:
    snapshot_t snap_{};
};

using SetTraits = ex::GenusBindingTraits<cea::AnatomyGenus::Set>;

/// Die Referenz-Komposition dieser TU: echtes Kern-Organ auf Slot 0, sonst stumme Achsen.
using StillComp =
    SetTraits::CompositionFor<cea::SortedArrayKeySet, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                              PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis>;
using StillOrgan = SetTraits::AnatomyFor<StillComp>;

/// Dieselbe Gattung mit einer BEOBACHTBAREN Achse im node_type-Slot (Slot 3) -- der Slot, in den das
/// Fenster spaeter produktiv ein fremdes Genus-Organ setzt.
using ObservedComp =
    SetTraits::CompositionFor<cea::SortedArrayKeySet, PlainAxis, PlainAxis, ObservableProbeAxis, PlainAxis, PlainAxis,
                              PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis>;
using ObservedOrgan = SetTraits::AnatomyFor<ObservedComp>;

// =============================================================================================
// (A) PRAESENZ-WACHE: je Slot ein Organ-Member -- Liste gegen Liste, Typ gegen Typ
// =============================================================================================

// A.1 Die Praesenz-Deklaration der Anatomie hat exakt die Laenge der autoritativen Achsen-Liste.
static_assert(StillOrgan::axis_organ_names().size() == SetTraits::axis_names().size(),
              "E-24 C3: jeder Achsen-Slot der Set-Gattung braucht ein reales Organ-Member.");

/// axis_names_match() -- elementweiser Vergleich der beiden Listen zur Compile-Zeit. Kein handgepflegter
/// String im Test: die eine Liste wird gegen die andere gehalten.
[[nodiscard]] constexpr bool axis_names_match() noexcept {
    auto const& declared  = StillOrgan::axis_organ_names();
    auto const& canonical = SetTraits::axis_names();
    for (std::size_t i = 0; i < declared.size(); ++i) {
        if (declared[i] != canonical[i]) return false;
    }
    return true;
}
static_assert(axis_names_match(), "E-24 C3: SetAnatomy::axis_organ_names() muss GenusBindingTraits<Set>::axis_names() "
                                  "elementweise treffen (Reihenfolge inklusive).");

// A.2 Je Slot der Accessor-Rueckgabetyp == Kompositions-Slot-Typ. Damit ist die Praesenz nicht nur ein
//     Name in einem Array, sondern ein Member des richtigen Typs an der richtigen Stelle.
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().search_algo_organ()), StillComp::search_algo&>);
static_assert(
    std::is_same_v<decltype(std::declval<StillOrgan&>().cache_traversal_organ()), StillComp::cache_traversal&>);
static_assert(
    std::is_same_v<decltype(std::declval<StillOrgan&>().path_compression_organ()), StillComp::path_compression&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().node_type_organ()), StillComp::node_type&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().memory_layout_organ()), StillComp::memory_layout&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().allocator_organ()), StillComp::allocator&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().prefetch_organ()), StillComp::prefetch&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().concurrency_organ()), StillComp::concurrency&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().serialization_organ()), StillComp::serialization&>);
static_assert(
    std::is_same_v<decltype(std::declval<StillOrgan&>().index_organization_organ()), StillComp::index_organization&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().io_dispatch_organ()), StillComp::io_dispatch&>);
static_assert(
    std::is_same_v<decltype(std::declval<StillOrgan&>().migration_policy_organ()), StillComp::migration_policy&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan&>().filter_organ()), StillComp::filter&>);
// Die const-Ueberladung existiert ebenfalls je Slot (stellvertretend an den beiden Raendern gepinnt).
static_assert(
    std::is_same_v<decltype(std::declval<StillOrgan const&>().search_algo_organ()), StillComp::search_algo const&>);
static_assert(std::is_same_v<decltype(std::declval<StillOrgan const&>().filter_organ()), StillComp::filter const&>);

// =============================================================================================
// (B) SLOT-PIN bleibt gruen (Gate-Klasse IV) -- aus der Bau-Bruecke gerechnet
// =============================================================================================
static_assert(cco::type_traits<cea::AnatomyGenus::Set>::slot_count == StillOrgan::organ_count());
static_assert(StillOrgan::axis_organ_names().size() == cco::type_traits<cea::AnatomyGenus::Set>::slot_count);
static_assert(StillOrgan::axis_observation_t::total_slots() == StillOrgan::organ_count());

// =============================================================================================
// (C) OBSERVE-VERDRAHTUNG: der Vertrag der per-Achsen-Sicht
// =============================================================================================
static_assert(StillOrgan::observable_axis_count() == 0, "stumme Komposition: kein Slot liefert statistics()");
static_assert(ObservedOrgan::observable_axis_count() == 1, "genau der eine beobachtbare Slot");
static_assert(std::is_same_v<decltype(cea::SetAxisObservation<ObservedComp>::node_type), CountingAxisSnapshot>,
              "der Snapshot-Typ kommt aus der Achse selbst (snapshot_of_t), er wird nicht vereinheitlicht");
static_assert(std::is_same_v<decltype(cea::SetAxisObservation<ObservedComp>::filter), cea::EmptyAxisSnapshot>,
              "eine Achse ohne statistics() bleibt Empty -- die Anatomie erfindet keinen Wert");

// (C.2) Die Anatomie bleibt Organ im Sinne von C0/C1 -- die Produktionstiefe bricht den Vertrag nicht.
static_assert(cea::OrganConcept<StillOrgan> && cea::KeyedOrganOps<StillOrgan> && cea::OrganOpSurface<StillOrgan>);
static_assert(cea::organ_op_family_count<StillOrgan>() == 1);
static_assert(std::is_base_of_v<cea::OrganGuard<StillOrgan>, StillOrgan>);
static_assert(std::is_same_v<cea::organ_snapshot_t<StillOrgan>, cea::SetObserverSnapshot>,
              "ABI-neutral: observe_all() liefert unveraendert den flachen Gattungs-POD");
static_assert(!std::is_polymorphic_v<StillOrgan>, "kein vtable-Ereignis durch die Organ-Member");

} // namespace

int main() {
    std::cout << "E-24 C3: Produktionstiefe der Set-Gattung (Organ-Member + observe-Verdrahtung):\n";

    // -- (D) ABI-NEUTRALITAET: die flache Gattungs-Sicht zaehlt wie vor C3 --
    StillOrgan set{};
    check_true("insert(10) ist neu", set.insert(10));
    check_true("insert(20) ist neu", set.insert(20));
    check_true("insert(10) erneut ist NICHT neu", !set.insert(10));
    check_true("contains(20) trifft", set.contains(20));
    check_true("contains(99) trifft nicht", !set.contains(99));
    check_true("erase(10) entfernt", set.erase(10));
    cea::SetObserverSnapshot const flat = set.observe_all();
    check_eq("flach: insert_count", flat.insert_count, std::uint64_t{2});
    check_eq("flach: contains_count", flat.contains_count, std::uint64_t{2});
    check_eq("flach: contains_hit_count", flat.contains_hit_count, std::uint64_t{1});
    check_eq("flach: contains_miss_count", flat.contains_miss_count, std::uint64_t{1});
    check_eq("flach: erase_count", flat.erase_count, std::uint64_t{1});
    check_eq("flach: current_size", flat.current_size, std::uint64_t{1});
    check_eq("flach: peak_size", flat.peak_size, std::uint64_t{2});

    // -- (A-Beleg) die Organ-Accessoren zeigen auf DIESELBEN Member, ueber die die Gattung treibt --
    check_eq("search_algo_organ() ist das getriebene Kern-Organ (occupied_count)",
             set.search_algo_organ().occupied_count(), std::size_t{1});
    check_true("Accessor liefert Referenz, keine Kopie",
               &set.search_algo_organ() == &static_cast<StillOrgan const&>(set).search_algo_organ());

    // -- (C-Beleg) die per-Achsen-Sicht sammelt real ein --
    std::cout << "\nper-Achsen-Sicht (observe_axes, Luecke-L2-Rest):\n";
    StillOrgan::axis_observation_t const still_axes = set.observe_axes();
    (void)still_axes; // stumme Komposition: alle Slots EmptyAxisSnapshot (compile-time in (C) gepinnt)
    check_eq("stumme Komposition: observable_axis_count", StillOrgan::observable_axis_count(), std::size_t{0});

    ObservedOrgan observed{};
    observed.node_type_organ().touch();
    observed.node_type_organ().touch();
    observed.node_type_organ().touch();
    ObservedOrgan::axis_observation_t const axes = observed.observe_axes();
    check_eq("beobachtbarer node_type-Slot: touches im per-Achsen-Aggregat", axes.node_type.touches, std::uint64_t{3});
    check_eq("beobachtbare Komposition: observable_axis_count", ObservedOrgan::observable_axis_count(), std::size_t{1});
    check_eq("total_slots aus der Komposition gerechnet", ObservedOrgan::axis_observation_t::total_slots(),
             std::size_t{13});

    // Gegenprobe: die FLACHE Sicht der beobachteten Komposition bleibt davon unberuehrt (zwei Ebenen,
    // zwei Fragen -- die per-Achsen-Sicht schreibt nichts in den Wire-Spiegel).
    check_eq("flache Sicht bleibt unberuehrt: insert_count", observed.observe_all().insert_count, std::uint64_t{0});

    // -- (E) SetExecutionContext: dieselbe Gattung ueber die builder-seitige Treiber-Flaeche --
    std::cout << "\nSetExecutionContext (builder-seitige Treiber-Flaeche):\n";
    cmd::SetExecutionContext<ObservedComp> ctx;
    check_true("ctx.insert(7)", ctx.insert(7));
    check_true("ctx.insert(8)", ctx.insert(8));
    check_true("ctx.contains(7)", ctx.contains(7));
    check_eq("ctx.size()", ctx.size(), std::size_t{2});
    check_true("ctx.empty() == false", !ctx.empty());
    ctx.anatomy().node_type_organ().touch();
    check_eq("ctx.observe_all().insert_count (flache Ebene)", ctx.observe_all().insert_count, std::uint64_t{2});
    check_eq("ctx.observe_axes().node_type.touches (per-Achsen-Ebene)", ctx.observe_axes().node_type.touches,
             std::uint64_t{1});
    check_eq("ctx.observable_axis_count()", cmd::SetExecutionContext<ObservedComp>::observable_axis_count(),
             std::size_t{1});
    check_true("ctx.genus() == Set", ctx.genus() == cea::AnatomyGenus::Set);
    check_eq("ctx.organ_count()", ctx.organ_count(), std::size_t{13});
    ctx.clear();
    check_true("ctx.clear() leert die Menge", ctx.empty());

    std::cout << "\n==== E-24 C3 Set-Produktionstiefe: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
