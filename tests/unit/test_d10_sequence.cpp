// D10 / L-76b — Sequence-Gattung: Typ-Ebene + V-indexed-Anatomie + SequenceAbiAdapter (in-process, leichtgewichtig).
// SequenceComposition (10 §28-Reptile-Achsen + axis_growth) + IsSequenceComposition + SequenceObserverSnapshotV1 +
// SequenceAnatomy (push_back/at, growth-Policy real) + SequenceAbiAdapter (IAnatomyBase + ISequenceTier dynamic_cast).
// Build: cl /I libs/cache_engine (kein Boost — internes V-Organ + DoublingGrowth).

#include "anatomy/sequence_tier.hpp"
#include "anatomy/sequence_composition.hpp"
#include "anatomy/sequence_anatomy.hpp"
#include "anatomy/sequence_abi_adapter.hpp"

#include <cstdint>
#include <iostream>
#include <string>

namespace cea = comdare::cache_engine::anatomy;
namespace eng = comdare::cache_engine::execution_engine;

using SC = cea::SequenceComposition<int, int, int, int, int, int, int, int, int>; // Growth = DoublingGrowth (default)
using SAnat = cea::SequenceAnatomy<SC>;

static_assert(cea::IsSequenceComposition<SC>);
static_assert(SC::slot_count == 10, "Sequence = 9 geteilte + axis_growth (§28, K-B; INC-2c)");
static_assert(cea::GrowthPolicy<cea::DoublingGrowth>, "DoublingGrowth muss GrowthPolicy erfuellen");
static_assert(SAnat::genus() == cea::AnatomyGenus::Sequence);

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

int main() {
    std::cout << "==== D10 Sequence-Typ-Ebene ====\n";
    tr("IsSequenceComposition<SC>", cea::IsSequenceComposition<SC>);
    eq("SC::slot_count == 10 (9 + axis_growth)", SC::slot_count, std::size_t{10});
    tr("genus() == Sequence (Reptil)", SAnat::genus() == cea::AnatomyGenus::Sequence);
    eq("organ_count() == 10", SAnat::organ_count(), std::size_t{10});
    eq("SequenceObserverSnapshotV1 = 8 uint64", sizeof(cea::SequenceObserverSnapshotV1), std::size_t{8 * 8});

    std::cout << "\n==== D10 SequenceAnatomy V-indexed (push_back/at) + axis_growth ====\n";
    SAnat seq;
    for (std::uint64_t i = 0; i < 5; ++i) seq.push_back(100 + i);
    eq("size() == 5 nach 5x push_back", seq.size(), std::size_t{5});
    auto a0 = seq.at(0);
    tr("at(0) == 100", a0.has_value() && *a0 == 100u);
    auto a4 = seq.at(4);
    tr("at(4) == 104", a4.has_value() && *a4 == 104u);
    tr("at(99) == nullopt (out-of-bounds)", !seq.at(99).has_value());
    cea::SequenceObserverSnapshot const o = seq.observe_all();
    eq("Observer push_count == 5", o.push_count, std::uint64_t{5});
    eq("Observer at_count == 3", o.at_count, std::uint64_t{3});
    eq("Observer at_oob_count == 1", o.at_oob_count, std::uint64_t{1});
    eq("Observer peak_size == 5", o.peak_size, std::uint64_t{5});
    tr("Observer growth_events > 0 (axis_growth real getrieben)", o.growth_events > 0);

    std::cout << "\n==== D10 SequenceAbiAdapter über IAnatomyBase + ISequenceTier ====\n";
    cea::SequenceAbiAdapter<SAnat> adapter;
    cea::IAnatomyBase*             base = &adapter;
    tr("genus() == Sequence", base->genus() == cea::AnatomyGenus::Sequence);
    tr("engine_kind() == Anatomy", base->engine_kind() == eng::ExecutionEngineKind::Anatomy);
    eq("organ_count() == 10", base->organ_count(), std::size_t{10});
    auto* st = dynamic_cast<cea::ISequenceTier*>(base);
    tr("dynamic_cast<ISequenceTier*> != null", st != nullptr);
    if (st) {
        for (std::uint64_t i = 0; i < 10; ++i) st->tier_push_back(i * 11);
        eq("tier_size() == 10", st->tier_size(), std::uint64_t{10});
        std::uint64_t out = 0;
        tr("tier_at(3) == 33", st->tier_at(3, &out) && out == 33u);
        tr("tier_at(999) == false (oob)", !st->tier_at(999, &out));
        cea::SequenceObserverSnapshotV1 pod{};
        st->tier_observe_sequence(&pod);
        eq("observe: push_count == 10", pod.push_count, std::uint64_t{10});
        eq("observe: organ_count == 10", pod.organ_count, std::uint64_t{10});
        tr("observe: growth_events > 0", pod.growth_events > 0);
    }

    std::cout << "\n==== D10 Sequence: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
