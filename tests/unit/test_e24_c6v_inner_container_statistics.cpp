// test_e24_c6v_inner_container_statistics -- E-24 C6-V (ABI-neutraler Vor-Baustein des b-Teils, E5).
//
// GEGENSTAND: InnerContainerStatistics + ObservableInnerContainer<Inner>
// (anatomy/inner_container_observable.hpp) -- die Observable-Huelle der ADAPTER-eigenen Achse
// inner_container (Katalog-Zeile C-E, docs/sessions/20260804-DOSSIER-achsen-qualitaets-parameter-katalog.md
// Sektion 2 Punkt 1/2; Union-Slot = Entscheid E6 Default a).
//
//   (A) CT-PINS             -- 8 uint64-Felder (ueber sizeof gepinnt), Feld-Typen einzeln, POD-Eigenschaften.
//   (B) LAUFZEIT-TREIBEN    -- die Huelle steckt im ECHTEN Adapter-Slot; die Paragraf-26.4-API
//                              (push/pop_front/pop_back/front/back) treibt sie.
//   (C) UNION-SLOT-ECHTHEIT -- DIESELBE Op-Folge ueber drei Substrate: Deque meldet 0 (O(1)-Beleg),
//                              Vector meldet die REAL verschobenen Elemente (O(n)), Heap die REAL
//                              ausgefuehrten Sift-Vergleiche (O(log n)). Alles vom Substrat
//                              ZURUECKGEMELDET, nie aus dem Substrat-Typ synthetisiert.
//   (D) EHRLICHKEITS-SCHLIESSUNG -- pop auf LEER wird gezaehlt (war unbeobachtet, Katalog C-E).
//   (E) 'OHNE' vs. 'LEER'   -- Katalog 1N.1.
//
// Bau: plain int main() (kein gtest), COMDARE_CE_ENABLE_STATISTICS=1 (die Huelle ist gegated).

#include "anatomy/inner_container_observable.hpp"

#include "anatomy/adapter_anatomy.hpp"   // AdapterAnatomy / AdapterComposition / Deque-/Vector-/HeapInner
#include "anatomy/cross_genus_organ.hpp" // CarriedAxis (die produktive 'getragen'-Belegung, 1N.1)

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
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

using DequeHull  = cea::ObservableInnerContainer<cea::DequeInner<>>;
using VectorHull = cea::ObservableInnerContainer<cea::VectorInner<>>;
using HeapHull   = cea::ObservableInnerContainer<cea::HeapInner<>>;

/// Adapter-Tier-Unterklasse mit der Huelle im inner_container-Slot; die zehn geteilten/delegierten Slots
/// sind GETRAGEN (CarriedAxis).
template <class Inner>
using AdapterComp = cea::AdapterComposition<cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis,
                                            cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis,
                                            cea::CarriedAxis, cea::CarriedAxis, Inner>;
template <class Inner>
using AdapterOrgan = cea::AdapterAnatomy<AdapterComp<Inner>>;

// =============================================================================================
// (A) CT-PINS -- der Mindest-Feldsatz ist FREEZE-Gegenstand: 8 Felder, alle uint64.
// =============================================================================================
static_assert(sizeof(cea::InnerContainerStatistics) == 8 * sizeof(std::uint64_t),
              "C6-V: InnerContainerStatistics traegt GENAU die 8 Katalog-Mindest-Felder (C-E, inkl. Union-Slot).");
static_assert(std::is_standard_layout_v<cea::InnerContainerStatistics> &&
              std::is_trivially_copyable_v<cea::InnerContainerStatistics>);
static_assert(std::is_same_v<decltype(cea::InnerContainerStatistics::push_count), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::InnerContainerStatistics::pop_count), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::InnerContainerStatistics::front_reads), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::InnerContainerStatistics::back_reads), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::InnerContainerStatistics::current_occupancy), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::InnerContainerStatistics::peak_occupancy), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::InnerContainerStatistics::underflow_count), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::InnerContainerStatistics::substrate_ops), std::uint64_t>);

// Die Huelle bleibt ein gueltiges Substrat (Element-Typ + Identitaet durchgereicht).
static_assert(std::is_same_v<typename DequeHull::element_type, std::uint64_t>);
static_assert(DequeHull::name == cea::DequeInner<>::name && HeapHull::name == cea::HeapInner<>::name);

// Die C3-Einsammlung nimmt die Huelle AUTOMATISCH auf (snapshot_of_t).
static_assert(std::is_same_v<decltype(AdapterOrgan<DequeHull>::axis_observation_t::inner_container),
                             cea::InnerContainerStatistics>);
static_assert(std::is_same_v<decltype(AdapterOrgan<cea::DequeInner<>>::axis_observation_t::inner_container),
                             cea::EmptyAxisSnapshot>,
              "Gegenprobe: das NACKTE Substrat hat kein statistics() -- der Slot bleibt Empty (das war die Luecke).");
static_assert(AdapterOrgan<cea::DequeInner<>>::observable_axis_count() == 0, "vor C6-V: stumm");
static_assert(AdapterOrgan<DequeHull>::observable_axis_count() == 1, "nach C6-V: observable_count steigt");

/// Die gemeinsame Treibe-Folge fuer den Substrat-Vergleich: 3 x push, dann 2 x FIFO-Entnahme.
template <class Organ>
static cea::InnerContainerStatistics drain_probe(Organ& organ) {
    organ.push(10);
    organ.push(30);
    organ.push(20);
    (void)organ.pop_front();
    (void)organ.pop_front();
    return organ.observe_axes().inner_container;
}

} // namespace

int main() {
    std::cout << "E-24 C6-V: InnerContainerStatistics + ObservableInnerContainer (Achse inner_container, C-E):\n";

    // -- (B) LAUFZEIT-TREIBEN ueber die ECHTE Paragraf-26.4-API --
    AdapterOrgan<DequeHull> queue{};
    queue.push(10);
    queue.push(30);
    queue.push(20);
    (void)queue.pop_front(); // front()-Lesung + Entnahme
    (void)queue.front();     // reine Lesung
    (void)queue.back();      // reine Lesung
    (void)queue.pop_back();  // back()-Lesung + Entnahme
    cea::InnerContainerStatistics const q = queue.observe_axes().inner_container;
    std::cout << "\nDequeInner-Huelle nach 3 push / 1 pop_front / 1 front / 1 back / 1 pop_back:\n";
    check_eq("push_count", q.push_count, std::uint64_t{3});
    check_eq("pop_count", q.pop_count, std::uint64_t{2});
    check_eq("front_reads (pop_front liest vorne + die reine front-Lesung)", q.front_reads, std::uint64_t{2});
    check_eq("back_reads (pop_back liest hinten + die reine back-Lesung)", q.back_reads, std::uint64_t{2});
    check_eq("current_occupancy", q.current_occupancy, std::uint64_t{1});
    check_eq("peak_occupancy", q.peak_occupancy, std::uint64_t{3});
    check_eq("underflow_count (noch keiner)", q.underflow_count, std::uint64_t{0});
    check_eq("Kreuz-Probe gegen die flache Gattungs-Sicht: push_count", queue.observe_all().push_count, q.push_count);

    // -- (C) UNION-SLOT: DIESELBE Op-Folge, drei Substrate, drei disjunkte Kostenklassen --
    std::cout << "\nUnion-Slot substrate_ops -- dieselbe Folge (3 push, 2 pop_front) je Substrat:\n";
    AdapterOrgan<DequeHull>             deque_organ{};
    AdapterOrgan<VectorHull>            vector_organ{};
    AdapterOrgan<HeapHull>              heap_organ{};
    cea::InnerContainerStatistics const d = drain_probe(deque_organ);
    cea::InnerContainerStatistics const v = drain_probe(vector_organ);
    cea::InnerContainerStatistics const h = drain_probe(heap_organ);
    check_eq("DequeInner: substrate_ops == 0 (O(1)-Beleg, ehrliche Null als MESSERGEBNIS)", d.substrate_ops,
             std::uint64_t{0});
    check_eq("VectorInner: elements_shifted == 2 + 1 (erase(begin) verschiebt real size-1)", v.substrate_ops,
             std::uint64_t{3});
    check_true("HeapInner: sift_ops > 0 (real ausgefuehrte Vergleiche von push_heap/pop_heap)", h.substrate_ops > 0);
    check_true("Kostenklassen-Trennung: Vector > Deque UND Heap > Deque",
               v.substrate_ops > d.substrate_ops && h.substrate_ops > d.substrate_ops);
    check_eq("die Op-Zaehler bleiben ueber alle Substrate gleich (push_count)", v.push_count, d.push_count);
    check_eq("die Op-Zaehler bleiben ueber alle Substrate gleich (pop_count)", h.pop_count, d.pop_count);

    // Die zaehlenden Varianten duerfen die DISZIPLIN nicht veraendern (Heap bleibt Max-Heap).
    AdapterOrgan<HeapHull> heap_discipline{};
    heap_discipline.push(10);
    heap_discipline.push(30);
    heap_discipline.push(20);
    auto const heap_top = heap_discipline.front();
    check_true("PRIORITY-Disziplin unveraendert: front() == 30 (Max-Heap trotz zaehlendem Komparator)",
               heap_top.has_value() && *heap_top == 30u);
    auto const heap_extract = heap_discipline.pop_front();
    check_true("Extract-Max liefert 30", heap_extract.has_value() && *heap_extract == 30u);
    auto const heap_next = heap_discipline.front();
    check_true("danach steht 20 oben (Heap-Invariante haelt)", heap_next.has_value() && *heap_next == 20u);
    check_true("und die Sift-Arbeit ist weiter gewachsen",
               heap_discipline.observe_axes().inner_container.substrate_ops > 0);

    // -- (D) EHRLICHKEITS-SCHLIESSUNG: pop auf LEER (Katalog C-E: heute UNBEOBACHTET) --
    std::cout << "\nEhrlichkeits-Schliessung underflow_count (pop auf leer):\n";
    AdapterOrgan<DequeHull>             empty_organ{};
    auto const                          nothing_front = empty_organ.pop_front();
    auto const                          nothing_back  = empty_organ.pop_back();
    cea::InnerContainerStatistics const u             = empty_organ.observe_axes().inner_container;
    check_true("pop_front auf leer liefert weiterhin nullopt (Verhalten unveraendert)", !nothing_front.has_value());
    check_true("pop_back auf leer liefert weiterhin nullopt", !nothing_back.has_value());
    check_eq("underflow_count zaehlt jetzt BEIDE Ereignisse", u.underflow_count, std::uint64_t{2});
    check_eq("dabei wurde KEINE pop-Op gezaehlt (kein erfundener Wert)", u.pop_count, std::uint64_t{0});

    // -- (E) 'OHNE' vs. 'LEER' (Katalog 1N.1) --
    std::cout << "\n'ohne'-vs-leer-Probe (1N.1: 'ohne' liefert Werte, 'leer' liefert keine):\n";
    AdapterOrgan<DequeHull> const untouched{};
    auto const                    axes = untouched.observe_axes();
    check_true("nicht getriebene Huelle: EHRLICHE NULLEN", axes.inner_container == cea::InnerContainerStatistics{});
    check_eq("...aber MIT Mess-Identitaet: observable_axis_count", AdapterOrgan<DequeHull>::observable_axis_count(),
             std::size_t{1});
    check_true("getragener Slot (CarriedAxis) liefert EmptyAxisSnapshot -- KEINE Mess-Identitaet",
               std::is_same_v<decltype(axes.search_algo), cea::EmptyAxisSnapshot>);
    check_eq("stumme Komposition (nacktes Substrat): observable_axis_count",
             AdapterOrgan<cea::DequeInner<>>::observable_axis_count(), std::size_t{0});
    check_eq("total_slots aus der LIVE-Komposition gerechnet (11, nicht der frozen legacy 13)",
             AdapterOrgan<DequeHull>::axis_observation_t::total_slots(), std::size_t{11});

    std::cout << "\n==== E-24 C6-V InnerContainerStatistics: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
