// test_container_genus — Gattungs-Generik Schritt 3 (2026-06-02) — die CONTAINER-Gattung (queuing q1) ist
// baubar + messbar, als 2. GenusBindingTraits-Instanz neben SearchAlgorithm. „Alle Gattungen bauen."
//
// Belegt: ContainerAnatomy<ContainerComposition<FIFOQueueBuffer>> treibt das ECHTE Q1-Buffer-Organ (put/get),
// liefert einen EIGENEN Container-Observer (NICHT ObserverAggregate<17>), genus()==Adapter; GenusBound<Adapter>
// jetzt true. Cross-Genus type-getrennt (Doku 14 §32).
//
// Build: cl /std:c++latest /EHsc /I<…> /I<build/generated…> (queuing-Achse + container_anatomy)

#include "builder/experiment_tree/genus_binding_traits.hpp"
#include "anatomy/container_anatomy.hpp"
#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_registry.hpp>   // FIFOQueueBuffer etc.
#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_registry.hpp>   // WatermarkFlush etc. (D4 q2-Slot)
#include <topics/queuing/axis_q2_queuing/concepts/axis_q2_queuing_concept.hpp>  // FlushDecision (Konvention-Guard)

#include <cstdint>
#include <iostream>
#include <string>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace cea = comdare::cache_engine::anatomy;
namespace q1  = comdare::cache_engine::queuing::axis_q1_queuing;
namespace q2  = comdare::cache_engine::queuing::axis_q2_queuing;
namespace q2c = comdare::cache_engine::queuing::axis_q2_queuing::concepts;

// D4-Konvention-Guard: der gespiegelte ContainerFlushDecision MUSS numerisch mit der echten q2-FlushDecision
// uebereinstimmen (ContainerAnatomy vergleicht numerisch). Bricht der Build, wenn die Enums auseinanderlaufen.
static_assert(static_cast<std::uint8_t>(q2c::FlushDecision::FullFlush)
              == static_cast<std::uint8_t>(cea::ContainerFlushDecision::FullFlush),
              "FlushDecision-Konvention (FullFlush) muss zwischen q2-Achse und container_anatomy identisch sein");

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) { std::cout << "  (erwartet: " << want << ")"; ++g_fail; }
    std::cout << "\n";
}
void check_true(char const* what, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n"; if (!c) ++g_fail; }

int main() {
    std::cout << "Gattungs-Generik Schritt 3: die CONTAINER-Gattung (queuing q1) baubar + messbar:\n";

    using Q1   = q1::FIFOQueueBuffer;                       // ein reales Q1-Buffer-Organ (FIFO)
    using Comp = cea::ContainerComposition<Q1>;
    cea::ContainerAnatomy<Comp> queue;

    // Gattungs-Identität (eigene Gattung, NICHT SearchAlgorithm).
    check_true("genus() == Adapter (Container/Queue)", queue.genus() == cea::AnatomyGenus::Adapter);
    check_eq("composition_name", std::string{queue.composition_name()}, std::string{"ContainerComposition"});
    check_eq("organ_count == 2 (buffer_strategy + flush_policy)", queue.organ_count(), std::size_t{2});

    // ECHTER Antrieb des Q1-Organs (put/get/size) — die Container-Gattungs-API.
    queue.put(10); queue.put(20); queue.put(30);
    check_eq("size == 3 nach 3× put", queue.size(), std::size_t{3});
    auto const first = queue.get();
    check_true("get() liefert 10 (FIFO-Reihenfolge, echtes Organ)", first.has_value() && *first == 10u);
    check_eq("size == 2 nach get", queue.size(), std::size_t{2});

    // EIGENER Container-Observer (gattungs-korrekt, getrennt von ObserverAggregate<17>).
    cea::ContainerObserverSnapshot const obs = queue.observe_all();
    check_eq("Container-Observer: put_count == 3", obs.put_count, std::uint64_t{3});
    check_eq("Container-Observer: get_count == 1", obs.get_count, std::uint64_t{1});
    check_eq("Container-Observer: peak_occupancy == 3", obs.peak_occupancy, std::uint64_t{3});
    check_eq("Container-Observer: current_occupancy == 2", obs.current_occupancy, std::uint64_t{2});

    // Gattungs-Generik: die Container-Gattung ist jetzt als 2. Instanz GEBUNDEN.
    check_true("GenusBound<Adapter> == true (Container-Gattung jetzt baubar)", ex::GenusBound<cea::AnatomyGenus::Adapter>);
    check_true("GenusBound<SearchAlgorithm> == true (weiterhin)", ex::GenusBound<cea::AnatomyGenus::SearchAlgorithm>);
    check_eq("GenusBindingTraits<Adapter>::name", std::string{ex::GenusBindingTraits<cea::AnatomyGenus::Adapter>::name}, std::string{"Container"});
    check_eq("GenusBindingTraits<Adapter>::slot_count == 2 (Q1+Q2)", ex::GenusBindingTraits<cea::AnatomyGenus::Adapter>::slot_count, std::size_t{2});

    // ── D4 / L-75: Q2 flush_policy als ECHTES 2. Organ — WatermarkFlush (75%) + capacity 8 → flush bei fill>=6 ──
    std::cout << "\nD4: Q2 flush_policy (WatermarkFlush 75%, capacity 8) treibt das 2. Organ:\n";
    using CompF = cea::ContainerComposition<q1::FIFOQueueBuffer, q2::WatermarkFlush>;
    cea::ContainerAnatomy<CompF> q2queue(/*capacity=*/8);
    for (std::uint64_t i = 0; i < 20; ++i) q2queue.put(i);   // 20 puts; flush bei jedem 6. (size>=6)
    cea::ContainerObserverSnapshot const f = q2queue.observe_all();
    check_eq("Q2: flush_decisions_evaluated == 20 (je put eine Entscheidung)", f.flush_decisions_evaluated, std::uint64_t{20});
    check_true("Q2: full_flush_count > 0 (Watermark hat real gespuelt)", f.full_flush_count > 0);
    check_eq("Q2: flush_complete_count == full_flush_count (on_flush_complete je Spuelung)", f.flush_complete_count, f.full_flush_count);
    check_true("Q2: current_occupancy < 6 nach Spuelungen (Buffer wurde geleert)", f.current_occupancy < 6);
    // Default-NoFlush (capacity 0) spuelt NIE → rueckwaerts-kompatibles Verhalten:
    check_eq("Default-NoFlush: full_flush_count == 0 (kein Auto-Flush bei capacity 0)", queue.observe_all().full_flush_count, std::uint64_t{0});

    std::cout << "\n==== Container-Gattung (Schritt 3): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
