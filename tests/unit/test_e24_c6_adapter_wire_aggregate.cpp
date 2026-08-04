// test_e24_c6_adapter_wire_aggregate -- E-24 C6 (b-Teil, ABI-EREIGNIS-SERIE).
//
// GEGENSTAND: AdapterObserverAggregate<11> + IAdapterTierV2 (anatomy/adapter_tier_v2.hpp) und die
// ADAPTER-SYMMETRIE (Katalog Sektion 2 Punkt 5).
//
//   (A) APPEND-ONLY   -- AdapterObserverSnapshotV1 byte-identisch (sizeof + jeder Feld-Offset).
//   (B) EIGENE FLAECHE-- 1x kalter dynamic_cast; eine Fremd-Gattung liefert nullptr.
//   (C) SYMMETRIE 1   -- observable_axis_count quert ERSTMALS die Adapter-Grenze. Der V1-POD ist der
//                        EINZIGE der vier ohne dieses Feld -- gezeigt an den echten V1-PODs.
//   (D) SYMMETRIE 2   -- underflow_count (pop auf LEER) steht auf dem Draht, Zeile [10] Spalte [6].
//   (E) UNION-SLOT    -- substrate_ops traegt je Substrat DISJUNKTE Kostenklassen; die drei Substrate
//                        liefern unter DERSELBEN Op-Folge drei verschiedene Werte auf dem Draht.
//   (F) LIVE-SLOTS    -- total_slots == 11 aus der LIVEN Komposition, NICHT die frozen 13
//                        (kAdapterCompositionSlotCount) -- Katalog Sektion 2 Punkt 6 am Objekt.
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes.

#include "anatomy/adapter_abi_adapter.hpp"

#include "anatomy/adapter_tier.hpp"               // AdapterObserverSnapshotV1 (eingefrorener V1-POD)
#include "anatomy/inner_container_observable.hpp" // ObservableInnerContainer (die beobachtende Huelle)
#include "anatomy/sequence_abi_adapter.hpp"       // Fremd-Gattung fuer die Degradier-Probe
#include "anatomy/sequence_anatomy.hpp"
#include "anatomy/set_tier.hpp"      // SetObserverSnapshotV1      (Symmetrie-Vergleich)
#include "anatomy/sequence_tier.hpp" // SequenceObserverSnapshotV1 (Symmetrie-Vergleich)
#include "anatomy/view_tier.hpp"     // ViewObserverSnapshotV1     (Symmetrie-Vergleich)

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen gleichnamiger Fixture-Typen
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace cea = comdare::cache_engine::anatomy;

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

template <class Inner>
using AdapterCompWith = cea::AdapterComposition<PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                                PlainAxis, PlainAxis, PlainAxis, PlainAxis, Inner>;

using DequeComp   = AdapterCompWith<cea::ObservableInnerContainer<cea::DequeInner<>>>;
using VectorComp  = AdapterCompWith<cea::ObservableInnerContainer<cea::VectorInner<>>>;
using HeapComp    = AdapterCompWith<cea::ObservableInnerContainer<cea::HeapInner<>>>;
using DequeOrgan  = cea::AdapterAnatomy<DequeComp>;
using AdapterWire = cea::AdapterAbiAdapter<DequeOrgan>;

/// Fremd-Gattung (Sequence) fuer die Degradier-Probe.
using SeqComp    = cea::SequenceComposition<PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                            PlainAxis, cea::DoublingGrowth>;
using SeqAdapter = cea::SequenceAbiAdapter<cea::SequenceAnatomy<SeqComp>>;

// (A) APPEND-ONLY: der eingefrorene V1-POD ist byte-identisch geblieben.
static_assert(sizeof(cea::AdapterObserverSnapshotV1) == 7 * 8, "V1-POD: 7 uint64-Felder, unveraendert");
static_assert(offsetof(cea::AdapterObserverSnapshotV1, push_count) == 0);
static_assert(offsetof(cea::AdapterObserverSnapshotV1, pop_count) == 8);
static_assert(offsetof(cea::AdapterObserverSnapshotV1, front_reads) == 16);
static_assert(offsetof(cea::AdapterObserverSnapshotV1, back_reads) == 24);
static_assert(offsetof(cea::AdapterObserverSnapshotV1, current_occupancy) == 32);
static_assert(offsetof(cea::AdapterObserverSnapshotV1, peak_occupancy) == 40);
static_assert(offsetof(cea::AdapterObserverSnapshotV1, organ_count) == 48);
static_assert(cea::kAdapterObserverSnapshotVersion == 1);

// (C) Die Ist-Asymmetrie am Objekt: DREI V1-PODs tragen observable_axis_count, der Adapter-POD NICHT.
/// Die Frage muss ueber ein TEMPLATE gestellt werden -- ein `requires`-Ausdruck ueber einen KONKRETEN
/// Typ ist kein SFINAE-Kontext und wuerde bei fehlendem Member hart brechen statt false zu liefern.
template <class S>
concept HatObservableAxisCount = requires(S const& s) { s.observable_axis_count; };

static_assert(HatObservableAxisCount<cea::SetObserverSnapshotV1>);
static_assert(HatObservableAxisCount<cea::SequenceObserverSnapshotV1>);
static_assert(HatObservableAxisCount<cea::ViewObserverSnapshotV1>);
static_assert(!HatObservableAxisCount<cea::AdapterObserverSnapshotV1>,
              "Ist-Befund (Katalog Punkt 5): der Adapter-V1-POD ist der EINZIGE ohne observable_axis_count -- "
              "er bleibt eingefroren, die Symmetrie stellt der Meta-Block der Wire-Form her");
// Und die Wire-Form traegt es fuer ALLE vier Gattungs-Ebenen (gemeinsamer Meta-Block).
static_assert(HatObservableAxisCount<cea::AdapterObserverAggregateWire>);

// Kein vtable-Anhang.
static_assert(!std::is_base_of_v<cea::IAdapterTier, cea::IAdapterTierV2>);
static_assert(std::is_base_of_v<cea::IAdapterTier, AdapterWire>);
static_assert(std::is_base_of_v<cea::IAdapterTierV2, AdapterWire>);

// (F) LIVE-Kante vs. frozen Legacy-Konstante -- am Objekt belegt.
static_assert(cea::kAdapterGenusSlotCount == DequeComp::slot_count, "die Kante kommt aus der LIVEN Komposition");
static_assert(cea::kAdapterCompositionSlotCount == 13, "die frozen Legacy-Konstante steht weiter bei 13");
static_assert(cea::kAdapterGenusSlotCount != cea::kAdapterCompositionSlotCount,
              "und genau deshalb darf sie NICHT die Quelle der Wire-Kante sein");
static_assert(cea::kAdapterAxisSchema[10].names[6] != nullptr, "underflow_count traegt seinen Namen im Schema");

/// Treibt n put + n pop_front und liefert die Wire-Form (dieselbe Einsammlung wie im ABI-Adapter).
template <class Comp>
static cea::AdapterObserverAggregateWire treibe(std::uint64_t n) {
    cea::AdapterAnatomy<Comp> organ{};
    for (std::uint64_t i = 0; i < n; ++i) organ.put(n - i); // absteigend: der Heap muss real sichten
    for (std::uint64_t i = 0; i < n; ++i) (void)organ.pop_front();
    cea::AdapterObserverAggregateWire w{};
    cea::fill_adapter_observer_aggregate(organ, w);
    return w;
}

} // namespace

int main() {
    std::cout << "=== E-24 C6 (e) -- AdapterObserverAggregate<11> + Adapter-Symmetrie ===\n";

    AdapterWire adapter{};
    adapter.tier_put(7);
    adapter.tier_put(8);
    std::uint64_t v = 0;
    check_true("tier_get liefert das vorderste Element", adapter.tier_get(&v));
    check_true("tier_get auf LEER nach dem zweiten Zug", adapter.tier_get(&v) && !adapter.tier_get(&v));

    std::cout << "\n[A] Der eingefrorene V1-Pfad liefert unveraendert\n";
    cea::AdapterObserverSnapshotV1 v1{};
    adapter.tier_observe_container(&v1);
    check_eq("v1.push_count", v1.push_count, std::uint64_t{2});
    check_eq("v1.pop_count", v1.pop_count, std::uint64_t{2});
    check_eq("v1.organ_count (LIVE aus der Komposition)", v1.organ_count, std::uint64_t{11});

    std::cout << "\n[B] Die NEUE Flaeche wird 1x kalt per dynamic_cast geholt\n";
    cea::IAnatomyBase* base = &adapter;
    auto*              v2   = dynamic_cast<cea::IAdapterTierV2*>(base);
    check_true("dynamic_cast<IAdapterTierV2*> auf der Adapter-Gattung trifft", v2 != nullptr);
    SeqAdapter         fremd{};
    cea::IAnatomyBase* fremd_base = &fremd;
    check_true("dieselbe Abfrage an der Sequence-Gattung liefert nullptr (sauberes Degradieren)",
               dynamic_cast<cea::IAdapterTierV2*>(fremd_base) == nullptr);

    if (v2 == nullptr) {
        std::cout << "\n=== FEHLER: ohne IAdapterTierV2 sind die Folge-Pruefungen gegenstandslos ===\n";
        return 1;
    }

    cea::AdapterObserverAggregateWire agg{};
    v2->tier_observe_container_axes(&agg);

    std::cout << "\n[C] SYMMETRIE 1: observable_axis_count quert erstmals die Adapter-Grenze\n";
    check_eq("agg.observable_axis_count", agg.observable_axis_count, std::uint64_t{1});
    check_eq("agg.filled_axis_count", agg.filled_axis_count, std::uint64_t{1});

    std::cout << "\n[F] LIVE-Slots statt frozen Legacy-Konstante\n";
    check_eq("agg.total_slots (LIVE)", agg.total_slots, std::uint64_t{11});
    check_true("und ausdruecklich NICHT die frozen kAdapterCompositionSlotCount (13)",
               agg.total_slots != cea::kAdapterCompositionSlotCount);

    std::cout << "\n[D] SYMMETRIE 2: underflow_count (pop auf LEER) steht auf dem Draht\n";
    {
        cea::AdapterAnatomy<DequeComp> organ{};
        organ.put(1);
        (void)organ.pop_front(); // regulaer
        (void)organ.pop_front(); // LEER -> Underflow
        (void)organ.pop_back();  // LEER -> Underflow
        cea::AdapterObserverAggregateWire w{};
        cea::fill_adapter_observer_aggregate(organ, w);
        check_eq("axis_stats[10][6] == underflow_count", w.axis_stats[10][6], std::uint64_t{2});
        check_eq("die regulaeren pops bleiben davon unberuehrt", w.axis_stats[10][1], std::uint64_t{1});
        check_true("die Spalte traegt ihren Schema-Namen", cea::kAdapterAxisSchema[10].names[6] != nullptr);
    }

    std::cout << "\n[E] UNION-SLOT substrate_ops: DIESELBE Op-Folge, DISJUNKTE Kostenklassen\n";
    {
        auto const deque_klein  = treibe<DequeComp>(4);
        auto const vector_klein = treibe<VectorComp>(4);
        auto const heap_klein   = treibe<HeapComp>(4);
        auto const deque_gross  = treibe<DequeComp>(16);
        auto const vector_gross = treibe<VectorComp>(16);
        auto const heap_gross   = treibe<HeapComp>(16);

        // Die Op-Zaehler sind ueber alle drei Substrate IDENTISCH -- nur die Kostenklasse unterscheidet.
        check_eq("Op-Zaehler identisch (Deque vs. Vector)", deque_gross.axis_stats[10][0],
                 vector_gross.axis_stats[10][0]);
        check_eq("Op-Zaehler identisch (Deque vs. Heap)", deque_gross.axis_stats[10][0], heap_gross.axis_stats[10][0]);

        // DequeInner: O(1) an beiden Enden -> die 0 ist das MESSERGEBNIS, keine Luecke. Sie bleibt 0,
        // auch wenn die Element-Zahl vervierfacht wird -- genau das ist der O(1)-Beleg.
        check_eq("Deque  n=4  substrate_ops", deque_klein.axis_stats[10][7], std::uint64_t{0});
        check_eq("Deque  n=16 substrate_ops (unveraendert 0 == O(1)-Beleg)", deque_gross.axis_stats[10][7],
                 std::uint64_t{0});

        // VectorInner: erase(begin) verschiebt real n-1, n-2, ... Elemente -> exakt n(n-1)/2.
        check_eq("Vector n=4  substrate_ops == 4*3/2", vector_klein.axis_stats[10][7], std::uint64_t{6});
        check_eq("Vector n=16 substrate_ops == 16*15/2", vector_gross.axis_stats[10][7], std::uint64_t{120});

        // HeapInner: real ausgefuehrte Sift-Vergleiche -> waechst, aber deutlich schwaecher.
        check_true("Heap   n=4  substrate_ops > 0", heap_klein.axis_stats[10][7] > 0);
        check_true("Heap   n=16 substrate_ops > n=4", heap_gross.axis_stats[10][7] > heap_klein.axis_stats[10][7]);

        // DAS ist die Kostenklassen-Aussage: bei Vervierfachung der Element-Zahl waechst der
        // Vector-Aufwand deutlich staerker als der Heap-Aufwand (O(n^2) vs. O(n log n)) -- und beide
        // sind vom O(1)-Substrat unterscheidbar. Der Draht traegt die Klasse, nicht nur eine Zahl.
        std::cout << "        (Heap n=4 -> " << heap_klein.axis_stats[10][7] << ", n=16 -> "
                  << heap_gross.axis_stats[10][7] << ")\n";
        check_true("Vector-Wachstum uebersteigt Heap-Wachstum (O(n^2) vs. O(n log n))",
                   (vector_gross.axis_stats[10][7] - vector_klein.axis_stats[10][7]) >
                       (heap_gross.axis_stats[10][7] - heap_klein.axis_stats[10][7]));
        check_true("alle drei Kostenklassen sind auf dem Draht unterscheidbar",
                   deque_gross.axis_stats[10][7] != vector_gross.axis_stats[10][7] &&
                       vector_gross.axis_stats[10][7] != heap_gross.axis_stats[10][7] &&
                       deque_gross.axis_stats[10][7] != heap_gross.axis_stats[10][7]);
    }

    std::cout << "\n=== " << (g_fail == 0 ? "ALLE PRUEFUNGEN GRUEN" : "FEHLER") << " (" << g_fail << ") ===\n";
    return g_fail == 0 ? 0 : 1;
}
