// test_e24_c6_view_wire_aggregate -- E-24 C6 (b-Teil, ABI-EREIGNIS-SERIE).
//
// GEGENSTAND: ViewObserverAggregate<5> + IViewTierV2 (anatomy/view_tier_v2.hpp).
// Die View ist die Gattungs-Ebene mit den MEISTEN eigenen Achsen (3 von 5 Slots) -- und genau die
// waren bis C6-V messtechnisch stumm. Hier queren sie erstmals die ABI-Grenze.
//
//   (A) APPEND-ONLY   -- ViewObserverSnapshotV1 byte-identisch; die bewusste API-Asymmetrie von
//                        IViewTier (kein clear/insert/erase) bleibt bestehen.
//   (B) EIGENE FLAECHE-- 1x kalter dynamic_cast; eine Fremd-Gattung liefert nullptr.
//   (C) DREI ZEILEN   -- extent/layout/accessor landen alle drei real auf dem Draht, mit Werten aus
//                        einem ECHTEN Lauf ueber die ViewAnatomy (bind + reads).
//   (D) ELISIONS-BEWEIS- statische Ausdehnung: bounds_checks_performed == 0, aber die ZEILE ist
//                        BEFUELLT. Genau daran ist das MESSERGEBNIS 0 von "kein Wert" unterscheidbar.
//   (E) LAYOUT-ECHTHEIT- non_contiguous_steps kommt aus den TATSAECHLICH gelieferten Offsets, nicht
//                        aus dem Layout-TYP: dieselbe Index-Folge liefert unter LayoutRight 0 und
//                        unter LayoutStrided<2> Spruenge.
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes.

#include "anatomy/view_abi_adapter.hpp"

#include "anatomy/sequence_abi_adapter.hpp" // Fremd-Gattung fuer die Degradier-Probe
#include "anatomy/sequence_anatomy.hpp"
#include "anatomy/view_policies_observable.hpp" // die drei beobachtenden Huellen
#include "anatomy/view_tier.hpp"                // ViewObserverSnapshotV1 (eingefrorener V1-POD)
#include "topics/view/view_policies.hpp"        // StaticExtent<N> / LayoutStrided / AlignedAccessor

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen gleichnamiger Fixture-Typen
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace cea = comdare::cache_engine::anatomy;
namespace cev = comdare::cache_engine::view;

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

template <class Extent, class Layout, class Accessor>
using ViewCompWith = cea::ViewComposition<PlainAxis, PlainAxis, Extent, Layout, Accessor>;

/// (a) dynamische Ausdehnung + row-major + Default-Accessor -- alle drei beobachtet.
using DynComp  = ViewCompWith<cea::ObservableExtent<cea::DynamicExtent>, cea::ObservableLayout<cea::LayoutRight>,
                              cea::ObservableAccessor<cea::DefaultAccessor>>;
using DynOrgan = cea::ViewAnatomy<DynComp>;
using ViewWire = cea::ViewAbiAdapter<DynOrgan>;

/// (b) STATISCHE Ausdehnung -- der Elisions-Beweis.
using StatComp  = ViewCompWith<cea::ObservableExtent<cev::StaticExtent<8>>, cea::ObservableLayout<cea::LayoutRight>,
                               cea::ObservableAccessor<cea::DefaultAccessor>>;
using StatOrgan = cea::ViewAnatomy<StatComp>;

/// (c) SPRINGENDES Layout -- die Layout-Echtheits-Gegenprobe.
using StridedComp =
    ViewCompWith<cea::ObservableExtent<cea::DynamicExtent>, cea::ObservableLayout<cev::LayoutStrided<2>>,
                 cea::ObservableAccessor<cea::DefaultAccessor>>;
using StridedOrgan = cea::ViewAnatomy<StridedComp>;

/// Fremd-Gattung (Sequence) fuer die Degradier-Probe.
using SeqComp    = cea::SequenceComposition<PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                            PlainAxis, cea::DoublingGrowth>;
using SeqAdapter = cea::SequenceAbiAdapter<cea::SequenceAnatomy<SeqComp>>;

// (A) APPEND-ONLY: der eingefrorene V1-POD ist byte-identisch geblieben.
static_assert(sizeof(cea::ViewObserverSnapshotV1) == 6 * 8, "V1-POD: 6 uint64-Felder, unveraendert");
static_assert(offsetof(cea::ViewObserverSnapshotV1, read_count) == 0);
static_assert(offsetof(cea::ViewObserverSnapshotV1, read_oob_count) == 8);
static_assert(offsetof(cea::ViewObserverSnapshotV1, bound_size) == 16);
static_assert(offsetof(cea::ViewObserverSnapshotV1, bind_count) == 24);
static_assert(offsetof(cea::ViewObserverSnapshotV1, observable_axis_count) == 32);
static_assert(offsetof(cea::ViewObserverSnapshotV1, organ_count) == 40);
static_assert(cea::kViewObserverSnapshotVersion == 1);

// Die bewusste API-Asymmetrie der View bleibt bestehen (non-owning: kein clear/insert/erase).
template <class T>
concept HatTierClear = requires(T& t) { t.tier_clear(); };
static_assert(!HatTierClear<cea::IViewTier>,
              "View bleibt non-owning: kein tier_clear an der V1-Flaeche (Ist-Vertrag, unveraendert)");

// Kein vtable-Anhang.
static_assert(!std::is_base_of_v<cea::IViewTier, cea::IViewTierV2>);
static_assert(std::is_base_of_v<cea::IViewTier, ViewWire>);
static_assert(std::is_base_of_v<cea::IViewTierV2, ViewWire>);

static_assert(cea::kViewGenusSlotCount == DynComp::slot_count);
static_assert(cea::kViewSchemaFilledAxisCount == 3, "die View traegt DREI benannte Achsen-Zeilen");
static_assert(cea::ViewObserverAggregateWire::genus == cea::AnatomyGenus::View);

/// Bindet einen Puffer, liest eine Index-Folge und liefert die Wire-Form (dieselbe Einsammlung wie
/// im ABI-Adapter).
template <class Organ>
static cea::ViewObserverAggregateWire treibe(std::uint64_t const* buf, std::size_t n, std::size_t reads) {
    Organ organ{};
    organ.bind(buf, n);
    for (std::size_t i = 0; i < reads; ++i) (void)organ.read(i);
    cea::ViewObserverAggregateWire w{};
    cea::fill_view_observer_aggregate(organ, w);
    return w;
}

} // namespace

int main() {
    std::cout << "=== E-24 C6 (f) -- ViewObserverAggregate<5>: die drei View-eigenen Achsen auf dem Draht ===\n";

    std::uint64_t const buf[8] = {10, 11, 12, 13, 14, 15, 16, 17};

    ViewWire adapter{};
    adapter.tier_bind(buf, 8);
    std::uint64_t v = 0;
    check_true("tier_read(0)", adapter.tier_read(0, &v));
    check_true("tier_read(1)", adapter.tier_read(1, &v));
    check_true("tier_read(99) ist out-of-bounds", !adapter.tier_read(99, &v));

    std::cout << "\n[A] Der eingefrorene V1-Pfad liefert unveraendert\n";
    cea::ViewObserverSnapshotV1 v1{};
    adapter.tier_observe_view(&v1);
    check_eq("v1.read_count", v1.read_count, std::uint64_t{3});
    check_eq("v1.read_oob_count", v1.read_oob_count, std::uint64_t{1});
    check_eq("v1.organ_count", v1.organ_count, std::uint64_t{5});

    std::cout << "\n[B] Die NEUE Flaeche wird 1x kalt per dynamic_cast geholt\n";
    cea::IAnatomyBase* base = &adapter;
    auto*              v2   = dynamic_cast<cea::IViewTierV2*>(base);
    check_true("dynamic_cast<IViewTierV2*> auf der View-Gattung trifft", v2 != nullptr);
    SeqAdapter         fremd{};
    cea::IAnatomyBase* fremd_base = &fremd;
    check_true("dieselbe Abfrage an der Sequence-Gattung liefert nullptr (sauberes Degradieren)",
               dynamic_cast<cea::IViewTierV2*>(fremd_base) == nullptr);

    if (v2 == nullptr) {
        std::cout << "\n=== FEHLER: ohne IViewTierV2 sind die Folge-Pruefungen gegenstandslos ===\n";
        return 1;
    }

    cea::ViewObserverAggregateWire agg{};
    v2->tier_observe_view_axes(&agg);

    std::cout << "\n[C] DREI Achsen-Zeilen queren die Grenze\n";
    check_eq("agg.filled_axis_count", agg.filled_axis_count, std::uint64_t{3});
    check_eq("agg.observable_axis_count", agg.observable_axis_count, std::uint64_t{3});
    check_eq("agg.total_slots (LIVE)", agg.total_slots, std::uint64_t{5});
    check_true("extent[0] bounds_checks_performed > 0 (dynamische Ausdehnung prueft real)", agg.axis_stats[2][0] > 0);
    check_eq("extent[1] oob_rejects (der eine reale Fehlgriff)", agg.axis_stats[2][1], std::uint64_t{1});
    check_true("layout[0] index_translations > 0", agg.axis_stats[3][0] > 0);
    check_true("accessor[0] access_count > 0", agg.axis_stats[4][0] > 0);
    check_eq("accessor[2] conversion_ops (ehrlich 0: die Ist-Accessoren konvertieren nicht)", agg.axis_stats[4][2],
             std::uint64_t{0});
    check_true("alle drei Zeilen tragen Schema-Namen", cea::kViewAxisSchema[2].names[0] != nullptr &&
                                                           cea::kViewAxisSchema[3].names[0] != nullptr &&
                                                           cea::kViewAxisSchema[4].names[0] != nullptr);

    std::cout << "\n[D] ELISIONS-BEWEIS: statische Ausdehnung -> 0, aber die ZEILE ist befuellt\n";
    {
        auto const stat = treibe<StatOrgan>(buf, 8, 4);
        auto const dyn  = treibe<DynOrgan>(buf, 8, 4);
        check_eq("StaticExtent<8>: bounds_checks_performed == 0 (MESSERGEBNIS)", stat.axis_stats[2][0],
                 std::uint64_t{0});
        check_true("DynamicExtent: bounds_checks_performed > 0 (real geprueft)", dyn.axis_stats[2][0] > 0);
        check_eq("die statische Zeile ist trotzdem BEFUELLT (3 Zeilen wie im dynamischen Fall)", stat.filled_axis_count,
                 std::uint64_t{3});
        check_true("und genau daran ist die 0 von 'kein Wert' unterscheidbar",
                   stat.filled_axis_count == dyn.filled_axis_count);
    }

    std::cout << "\n[E] LAYOUT-ECHTHEIT: die Offsets kommen aus dem LAUF, nicht aus dem Typ\n";
    {
        auto const gerade    = treibe<DynOrgan>(buf, 8, 4);
        auto const springend = treibe<StridedOrgan>(buf, 8, 4);
        check_eq("LayoutRight: non_contiguous_steps", gerade.axis_stats[3][1], std::uint64_t{0});
        check_true("LayoutStrided<2>: non_contiguous_steps > 0", springend.axis_stats[3][1] > 0);
        check_true("und der groesste Sprung ist entsprechend groesser",
                   springend.axis_stats[3][2] > gerade.axis_stats[3][2]);
        check_eq("die Zahl der Uebersetzungen ist dabei identisch (nur die Lokalitaet unterscheidet)",
                 gerade.axis_stats[3][0], springend.axis_stats[3][0]);
    }

    std::cout << "\n=== " << (g_fail == 0 ? "ALLE PRUEFUNGEN GRUEN" : "FEHLER") << " (" << g_fail << ") ===\n";
    return g_fail == 0 ? 0 : 1;
}
