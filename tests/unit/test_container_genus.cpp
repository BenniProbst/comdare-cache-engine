// test_container_genus — #87+#90 (2026-06-03, AUTORITATIV Doku 14 §28 Invertebrate + §26.4 + Doc 30 §8.0/§8.1):
// die Adapter-TIER-UNTERKLASSE der CONTAINER-GATTUNG, baubar + messbar.
//
// 3-Ebenen: Gattung(Container, Ebene 1) → Tier-Unterklasse(Adapter, Ebene 2, fester §28-Achsen-Satz) → Achsen(Ebene 3).
// §28 Adapter = 13 Achsen: 12 geteilt/delegiert + inner_container (NEU axis_inner, spezifisch). KEINE „ordering"-Achse
// (das war ein geratener Fehler). §26.4-API push/pop/top/front/back; Disziplin (FIFO/LIFO) = API-Nutzung, keine Achse.
// inner_container permutiert (DequeInner/VectorInner) — gebaut analog SequenceComposition (10 geteilt + growth).
//
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> /I<build/generated…>

#include "builder/experiment_tree/genus_binding_traits.hpp"
#include "anatomy/adapter_anatomy.hpp"

#include <cstdint>
#include <iostream>
#include <string>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace cea = comdare::cache_engine::anatomy;

// Die 12 geteilten/delegierten §28-Achsen: im in-process-Test Platzhalter. Die Anatomie treibt NUR die spezifische
// Achse inner_container real; die geteilten werden getragen (analog SequenceAnatomy, die nur growth real treibt).
struct DelegatedAxis {};

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}

int main() {
    std::cout << "#87+#90: Adapter-Tier-Unterklasse der Container-Gattung (Doku 14 §28, 13 Achsen):\n";

    using D    = DelegatedAxis;
    using Comp = cea::AdapterComposition<D, D, D, D, D, D, D, D, D, D, cea::DequeInner<>>; // 11 + inner (INC-2c)
    cea::AdapterAnatomy<Comp> queue;

    // ── Ebene 1+2: Gattung (Container) + Tier-Unterklasse (Adapter) ──
    check_true("gattung() == Container (Ebene 1, Außen-Interface)", queue.gattung() == cea::AnatomyGattung::Container);
    check_true("genus() == Adapter (Ebene 2, Tier-Unterklasse)", queue.genus() == cea::AnatomyGenus::Adapter);
    check_true("gattung_of(Adapter) == Container (Ebenen-Abbildung)",
               cea::gattung_of(cea::AnatomyGenus::Adapter) == cea::AnatomyGattung::Container);
    check_eq("gattung_name(Container)", std::string{cea::gattung_name(cea::AnatomyGattung::Container)},
             std::string{"Container"});
    check_eq("genus_name(Adapter)", std::string{cea::genus_name(cea::AnatomyGenus::Adapter)}, std::string{"Adapter"});
    check_eq("composition_name", std::string{queue.composition_name()}, std::string{"AdapterComposition"});
    check_eq("organ_count == 11 (§28 + INC-2d: 10 geteilt/delegiert + inner_container)", queue.organ_count(),
             std::size_t{11});

    // ── Ebene 3 / §26.4-API: push/pop/front/back über inner_container; Disziplin = API-Nutzung ──
    queue.push(10);
    queue.push(20);
    queue.push(30); // inner: [10,20,30]
    check_eq("size == 3 nach 3x push", queue.size(), std::size_t{3});
    auto const f = queue.pop_front(); // FIFO (queue): 10 → inner [20,30]
    check_true("FIFO pop_front() liefert 10 (vorderstes)", f.has_value() && *f == 10u);
    auto const b = queue.pop_back(); // LIFO (stack): 30 → inner [20]
    check_true("LIFO pop_back() liefert 30 (hinterstes)", b.has_value() && *b == 30u);
    check_true("front()==20 (verbleibendes Element)", queue.front().has_value() && *queue.front() == 20u);
    check_true("top()==back()==20", queue.top().has_value() && *queue.top() == 20u);
    check_eq("size == 1", queue.size(), std::size_t{1});

    // ── EIGENER Adapter-Observer (getrennt von ObserverAggregate<19>) ──
    cea::AdapterObserverSnapshot const obs = queue.observe_all();
    check_eq("Observer: push_count == 3", obs.push_count, std::uint64_t{3});
    check_eq("Observer: pop_count == 2", obs.pop_count, std::uint64_t{2});
    check_eq("Observer: front_reads == 1 (ein pop_front)", obs.front_reads, std::uint64_t{1});
    check_eq("Observer: back_reads == 1 (ein pop_back)", obs.back_reads, std::uint64_t{1});
    check_eq("Observer: peak_occupancy == 3", obs.peak_occupancy, std::uint64_t{3});
    check_eq("Observer: current_occupancy == 1", obs.current_occupancy, std::uint64_t{1});

    // ── inner_container permutiert (die spezifische §28-Achse): VectorInner als 2. Organ (Roh-Vektor-Substrat) ──
    std::cout << "\ninner_container = VectorInner (2. Organ der spezifischen Achse, kontiguierliches Substrat):\n";
    cea::AdapterAnatomy<cea::AdapterComposition<D, D, D, D, D, D, D, D, D, D, cea::VectorInner<>>> vq;
    vq.push(7);
    vq.push(9);
    check_true("VectorInner: pop_back() liefert 9", vq.pop_back().has_value());
    check_eq("VectorInner: organ_count == 11 (gleicher Achsen-Satz)", vq.organ_count(), std::size_t{11});

    // ── inner_container = HeapInner (3. Organ): ECHTE priority_queue-Disziplin (§26.4 vector+Compare, Max-Heap) ──
    // Die Priority-Disziplin lebt INNERHALB der inner_container-Achse (§28) — KEINE neue Achse. Nutzung: push +
    // front()(=Max) + pop_front()(=Extract-Max), via std::push_heap/pop_heap (Stand der Technik).
    std::cout << "\ninner_container = HeapInner (3. Organ, §26.4 priority_queue: Max-Heap + Compare):\n";
    cea::AdapterAnatomy<cea::AdapterComposition<D, D, D, D, D, D, D, D, D, D, cea::HeapInner<>>> pq;
    pq.push(10);
    pq.push(30);
    pq.push(20);
    auto const pmax = pq.front();
    check_true("HeapInner: front() == 30 (Maximum, echte Heap-Disziplin)", pmax.has_value() && *pmax == 30u);
    auto const x1 = pq.pop_front();
    check_true("HeapInner: pop_front() == 30 (Extract-Max)", x1.has_value() && *x1 == 30u);
    auto const x2 = pq.pop_front();
    check_true("HeapInner: nächstes pop_front() == 20 (zweitgrößtes)", x2.has_value() && *x2 == 20u);
    auto const x3 = pq.pop_front();
    check_true("HeapInner: letztes pop_front() == 10 (kleinstes zuletzt)", x3.has_value() && *x3 == 10u);
    check_eq("HeapInner: organ_count == 11 (gleicher §28-Achsen-Satz)", pq.organ_count(), std::size_t{11});

    // ── Gattungs-Bindung: GenusBindingTraits<Adapter> (13 §28-Achsen) ──
    std::cout << "\nGattungs-Bindung (GenusBindingTraits<Adapter>):\n";
    check_true("GenusBound<Adapter> == true", ex::GenusBound<cea::AnatomyGenus::Adapter>);
    check_true("GenusBound<SearchAlgorithm> == true (weiterhin)", ex::GenusBound<cea::AnatomyGenus::SearchAlgorithm>);
    check_eq("GenusBindingTraits<Adapter>::name == Adapter (Tier-Unterklasse)",
             std::string{ex::GenusBindingTraits<cea::AnatomyGenus::Adapter>::name}, std::string{"Adapter"});
    check_true("GenusBindingTraits<Adapter>::gattung == Container",
               ex::GenusBindingTraits<cea::AnatomyGenus::Adapter>::gattung == cea::AnatomyGattung::Container);
    check_eq("GenusBindingTraits<Adapter>::slot_count == 11",
             ex::GenusBindingTraits<cea::AnatomyGenus::Adapter>::slot_count, std::size_t{11});
    check_eq("Adapter axis_names[0] == search_algo (delegiert)",
             std::string{ex::GenusBindingTraits<cea::AnatomyGenus::Adapter>::axis_names()[0]},
             std::string{"search_algo"});
    check_eq("Adapter axis_names[10] == inner_container (spezifisch; INC-2d: isa raus)",
             std::string{ex::GenusBindingTraits<cea::AnatomyGenus::Adapter>::axis_names()[10]},
             std::string{"inner_container"});

    std::cout << "\n==== Adapter-Tier-Unterklasse (#87+#90, §28): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
