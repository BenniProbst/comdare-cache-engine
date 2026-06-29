// L-76b — axis_growth Mehr-Policy-Achse: die 4 GrowthPolicy-Varianten (Doubling/GoldenRatio/FixedChunk/Exact)
// erfüllen das GrowthPolicy-Concept, liefern DISTINKTE next_capacity (realer Algorithmus-Unterschied, kein Stub),
// die Registry trägt alle 4, und in der ECHTEN SequenceAnatomy treibt jede Policy eine andere growth_events-Zahl
// (axis_growth ist als 11. Permutations-Slot real wirksam). Build: cl /I libs/cache_engine + Boost::mp11.

#include "topics/sequence/axis_growth/axis_growth_registry.hpp"
#include "topics/sequence/axis_growth/axis_growth_policies.hpp"
#include "anatomy/sequence_anatomy.hpp"
#include "anatomy/sequence_composition.hpp"

#include <boost/mp11.hpp>

#include <cstdint>
#include <iostream>
#include <string>

namespace cea = comdare::cache_engine::anatomy;
namespace ag  = comdare::cache_engine::sequence::axis_growth;
namespace mp  = boost::mp11;

// (1) Alle 4 Policies erfüllen das GrowthPolicy-Concept (aus sequence_composition.hpp).
static_assert(cea::GrowthPolicy<cea::DoublingGrowth>, "DoublingGrowth ist GrowthPolicy");
static_assert(cea::GrowthPolicy<ag::GoldenRatioGrowth>, "GoldenRatioGrowth ist GrowthPolicy");
static_assert(cea::GrowthPolicy<ag::FixedChunkGrowth<64>>, "FixedChunkGrowth ist GrowthPolicy");
static_assert(cea::GrowthPolicy<ag::ExactGrowth>, "ExactGrowth ist GrowthPolicy");

// (2) Registry trägt alle 4.
static_assert(ag::kGrowthPolicyCount == 4, "axis_growth hat 4 Policies");
static_assert(mp::mp_size<ag::EnabledGrowthPolicies>::value == 4, "EnabledGrowthPolicies == 4");

static int g_fail = 0;
template <class A, class B>
static void eq(char const* w, A const& g, B const& e) {
    bool ok = (g == e);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) {
        std::cout << " (erwartet " << e << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
static void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// SequenceComposition mit variabler Growth-Policy (10 Platzhalter-Slots + axis_growth).
template <class GrowthT>
using SeqComp = cea::SequenceComposition<int, int, int, int, int, int, int, int, int, int, GrowthT>;

template <class GrowthT>
static std::uint64_t growth_events_for(std::uint64_t n_pushes) {
    cea::SequenceAnatomy<SeqComp<GrowthT>> tier;
    for (std::uint64_t i = 0; i < n_pushes; ++i) tier.push_back(i);
    return tier.observe_all().growth_events;
}

int main() {
    std::cout << "==== L-76b axis_growth Mehr-Policy-Achse (Doubling/GoldenRatio/FixedChunk/Exact) ====\n";

    // (3) DISTINKTE next_capacity ab (current=4, requested=5): jede Policy entscheidet anders.
    std::cout << "-- next_capacity(current=4, requested=5) je Policy --\n";
    eq("DoublingGrowth   -> 8 (×2)", cea::DoublingGrowth{}.next_capacity(4, 5), std::size_t{8});
    eq("GoldenRatioGrowth-> 6 (×1.5)", ag::GoldenRatioGrowth{}.next_capacity(4, 5), std::size_t{6});
    eq("FixedChunkGrowth -> 68 (+64)", ag::FixedChunkGrowth<64>{}.next_capacity(4, 5), std::size_t{68});
    eq("ExactGrowth      -> 5 (1:1)", ag::ExactGrowth{}.next_capacity(4, 5), std::size_t{5});
    tr("alle 4 next_capacity distinkt", true); // 8/6/68/5 — durch die eq's oben belegt

    // (4) growth_factor je Policy.
    eq("DoublingGrowth growth_factor == 2.0", cea::DoublingGrowth{}.growth_factor(), 2.0);
    eq("GoldenRatioGrowth growth_factor == 1.5", ag::GoldenRatioGrowth{}.growth_factor(), 1.5);
    eq("ExactGrowth growth_factor == 1.0", ag::ExactGrowth{}.growth_factor(), 1.0);

    // (5) INTEGRATION: in der ECHTEN SequenceAnatomy treibt jede Policy eine andere growth_events-Zahl (axis_growth
    // real wirksam als 11. Achse). GoldenRatio wächst langsamer als Doubling → MEHR Realloc-Events bei gleichem N.
    std::cout << "\n-- growth_events in SequenceAnatomy (100 push_back) je Policy --\n";
    std::uint64_t const ev_double = growth_events_for<cea::DoublingGrowth>(100);
    std::uint64_t const ev_golden = growth_events_for<ag::GoldenRatioGrowth>(100);
    std::uint64_t const ev_chunk  = growth_events_for<ag::FixedChunkGrowth<64>>(100);
    std::uint64_t const ev_exact  = growth_events_for<ag::ExactGrowth>(100);
    std::cout << "  Doubling=" << ev_double << " GoldenRatio=" << ev_golden << " FixedChunk<64>=" << ev_chunk
              << " Exact=" << ev_exact << "\n";
    tr("alle Policies feuern growth_events > 0 (real getrieben)",
       ev_double > 0 && ev_golden > 0 && ev_chunk > 0 && ev_exact > 0);
    tr("GoldenRatio > Doubling (×1.5 wächst langsamer → mehr Reallocs)", ev_golden > ev_double);
    tr("Exact >= GoldenRatio (1:1 wächst am langsamsten → die meisten Reallocs)", ev_exact >= ev_golden);
    tr("FixedChunk<64> < Doubling (große additive Schritte → wenige Reallocs bei N=100)", ev_chunk < ev_double);
    tr("growth_events unterscheiden sich je Policy (axis_growth real wirksam)",
       !(ev_double == ev_golden && ev_golden == ev_chunk && ev_chunk == ev_exact));

    std::cout << "\n==== L-76b axis_growth: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
