// test_e24_c6_sequence_wire_aggregate -- E-24 C6 (b-Teil, ABI-EREIGNIS-SERIE).
//
// GEGENSTAND: SequenceObserverAggregate<9> + ISequenceTierV2 (anatomy/sequence_tier_v2.hpp) und der
// E1-DETAIL-ENTSCHEID growth_factor_milli als FELD der Wire-Form.
//
//   (A) APPEND-ONLY   -- SequenceObserverSnapshotV1 byte-identisch (sizeof + jeder Feld-Offset).
//   (B) EIGENE FLAECHE-- 1x kalter dynamic_cast; eine Fremd-Gattung liefert nullptr.
//   (C) E1-DETAIL     -- growth_factor_milli steht in Spalte [8][7]; der IN-PROCESS-Satz
//                        GrowthStatistics bleibt bei GENAU 7 Feldern (Katalog-Mindest-Feldsatz C-A).
//   (D) GATING        -- eine NACKTE Policy ohne Huelle laesst Spalte [8][7] bei 0: kein
//                        synthetisierter Wert, kein aus dem Typ geratener Faktor.
//   (E) UNTERSCHEIDET -- zwei verschiedene Wachstums-ALGORITHMEN sind auf dem Draht an genau diesem
//                        Feld unterscheidbar (Owner-KERN NACHTRAG 3 (a): das Mess-Objekt IST der
//                        Algorithmus). Ohne das Feld waere die Wirkung da, der Erzeuger nicht.
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes.

#include "anatomy/sequence_abi_adapter.hpp"

#include "anatomy/growth_policy_observable.hpp" // ObservableGrowth (die Huelle mit growth_factor_milli)
#include "anatomy/sequence_tier.hpp"            // SequenceObserverSnapshotV1 (eingefrorener V1-POD)
#include "anatomy/set_abi_adapter.hpp"          // Fremd-Gattung fuer die Degradier-Probe
#include "anatomy/set_default_organ.hpp"
#include "topics/sequence/axis_growth/axis_growth_policies.hpp" // GoldenRatio/FixedChunk/Exact

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

/// (a) Sequence MIT beobachtender Growth-Huelle (Verdopplung, Faktor 2.0).
using SeqCompObs = cea::SequenceComposition<PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                            PlainAxis, cea::ObservableGrowth<cea::DoublingGrowth>>;
using SeqOrganObs   = cea::SequenceAnatomy<SeqCompObs>;
using SeqAdapterObs = cea::SequenceAbiAdapter<SeqOrganObs>;

/// (b) Sequence MIT beobachtender Growth-Huelle um einen ANDEREN Algorithmus (GoldenRatio == x1.5 am Ist).
using SeqCompGolden =
    cea::SequenceComposition<PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                             cea::ObservableGrowth<comdare::cache_engine::sequence::axis_growth::GoldenRatioGrowth>>;
using SeqOrganGolden = cea::SequenceAnatomy<SeqCompGolden>;

/// (c) Sequence MIT Huelle um eine ADDITIVE Policy (FixedChunk: growth_factor() == 0.0 als Sentinel).
using SeqCompChunk =
    cea::SequenceComposition<PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                             cea::ObservableGrowth<comdare::cache_engine::sequence::axis_growth::FixedChunkGrowth<64>>>;
using SeqOrganChunk = cea::SequenceAnatomy<SeqCompChunk>;

/// (d) Sequence mit NACKTER Policy (keine Huelle) -- die Gating-Probe.
using SeqCompNackt  = cea::SequenceComposition<PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                               PlainAxis, PlainAxis, cea::DoublingGrowth>;
using SeqOrganNackt = cea::SequenceAnatomy<SeqCompNackt>;

/// Fremd-Gattung (Set) fuer die Degradier-Probe.
using SetComp    = cea::SetComposition<cea::SortedArrayKeySet, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                       PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis>;
using SetAdapter = cea::SetAbiAdapter<cea::SetAnatomy<SetComp>>;

// (A) APPEND-ONLY: der eingefrorene V1-POD ist byte-identisch geblieben.
static_assert(sizeof(cea::SequenceObserverSnapshotV1) == 8 * 8, "V1-POD: 8 uint64-Felder, unveraendert");
static_assert(offsetof(cea::SequenceObserverSnapshotV1, push_count) == 0);
static_assert(offsetof(cea::SequenceObserverSnapshotV1, at_count) == 8);
static_assert(offsetof(cea::SequenceObserverSnapshotV1, at_oob_count) == 16);
static_assert(offsetof(cea::SequenceObserverSnapshotV1, current_size) == 24);
static_assert(offsetof(cea::SequenceObserverSnapshotV1, peak_size) == 32);
static_assert(offsetof(cea::SequenceObserverSnapshotV1, growth_events) == 40);
static_assert(offsetof(cea::SequenceObserverSnapshotV1, observable_axis_count) == 48);
static_assert(offsetof(cea::SequenceObserverSnapshotV1, organ_count) == 56);
static_assert(cea::kSequenceObserverSnapshotVersion == 1);

// Kein vtable-Anhang: V2 ist ein eigener Typ.
static_assert(!std::is_base_of_v<cea::ISequenceTier, cea::ISequenceTierV2>);
static_assert(std::is_base_of_v<cea::ISequenceTier, SeqAdapterObs>);
static_assert(std::is_base_of_v<cea::ISequenceTierV2, SeqAdapterObs>);

// (C) Der IN-PROCESS-Satz bleibt bei GENAU 7 Feldern -- growth_factor_milli ist KEIN POD-Feld.
static_assert(sizeof(cea::GrowthStatistics) == 7 * 8,
              "E1-Detail: der in-process-7er-Satz bleibt unveraendert; der Faktor lebt auf der Wire-Ebene");
static_assert(cea::kSequenceGrowthSlot == 8);
static_assert(cea::kSequenceGrowthFactorMilliField == 7);
static_assert(cea::kSequenceAxisSchema[8].names[7] != nullptr, "die Spalte traegt ihren Namen im Schema");

static_assert(cea::kSequenceGenusSlotCount == SeqCompObs::slot_count);
static_assert(cea::SequenceObserverAggregateWire::genus == cea::AnatomyGenus::Sequence);

} // namespace

int main() {
    std::cout << "=== E-24 C6 (d) -- SequenceObserverAggregate<9> + E1-Detail growth_factor_milli ===\n";

    SeqAdapterObs adapter{};
    for (std::uint64_t i = 0; i < 5; ++i) adapter.tier_push_back(i);

    std::cout << "\n[A] Der eingefrorene V1-Pfad liefert unveraendert\n";
    cea::SequenceObserverSnapshotV1 v1{};
    adapter.tier_observe_sequence(&v1);
    check_eq("v1.push_count", v1.push_count, std::uint64_t{5});
    check_eq("v1.organ_count", v1.organ_count, std::uint64_t{9});
    check_true("v1.growth_events > 0 (real getrieben)", v1.growth_events > 0);

    std::cout << "\n[B] Die NEUE Flaeche wird 1x kalt per dynamic_cast geholt\n";
    cea::IAnatomyBase* base = &adapter;
    auto*              v2   = dynamic_cast<cea::ISequenceTierV2*>(base);
    check_true("dynamic_cast<ISequenceTierV2*> auf der Sequence-Gattung trifft", v2 != nullptr);
    SetAdapter         fremd{};
    cea::IAnatomyBase* fremd_base = &fremd;
    check_true("dieselbe Abfrage an der Set-Gattung liefert nullptr (sauberes Degradieren)",
               dynamic_cast<cea::ISequenceTierV2*>(fremd_base) == nullptr);

    if (v2 == nullptr) {
        std::cout << "\n=== FEHLER: ohne ISequenceTierV2 sind die Folge-Pruefungen gegenstandslos ===\n";
        return 1;
    }

    cea::SequenceObserverAggregateWire agg{};
    v2->tier_observe_sequence_axes(&agg);

    std::cout << "\n[C] E1-Detail: growth_factor_milli steht als FELD auf dem Draht\n";
    check_eq("axis_stats[8][0] == growth_events", agg.axis_stats[8][0], v1.growth_events);
    check_true("axis_stats[8][1] == elements_copied > 0 (reale Umkopier-Menge)", agg.axis_stats[8][1] > 0);
    check_eq("axis_stats[8][7] == growth_factor_milli (DoublingGrowth: 2.0)", agg.axis_stats[8][7],
             std::uint64_t{2000});
    check_eq("agg.total_slots (LIVE)", agg.total_slots, std::uint64_t{9});
    check_eq("agg.filled_axis_count", agg.filled_axis_count, std::uint64_t{1});
    check_true("seg_ns traegt 'nicht erhoben' (negativ)", agg.seg_ns[8] < 0);

    std::cout << "\n[D] Gating: eine NACKTE Policy synthetisiert KEINEN Faktor\n";
    {
        SeqOrganNackt organ{};
        for (std::uint64_t i = 0; i < 5; ++i) organ.push_back(i);
        cea::SequenceObserverAggregateWire nackt{};
        cea::fill_sequence_observer_aggregate(organ, nackt);
        check_eq("axis_stats[8][7] bleibt 0 (kein Wert erfunden)", nackt.axis_stats[8][7], std::uint64_t{0});
        check_eq("die Zeile ist ueberhaupt nicht befuellt", nackt.filled_axis_count, std::uint64_t{0});
        check_eq("observable_axis_count ist ehrlich 0", nackt.observable_axis_count, std::uint64_t{0});
    }

    std::cout << "\n[E] Zwei ALGORITHMEN, EIN Feld: der Draht unterscheidet sie\n";
    {
        SeqOrganGolden organ{};
        for (std::uint64_t i = 0; i < 5; ++i) organ.push_back(i);
        cea::SequenceObserverAggregateWire golden{};
        cea::fill_sequence_observer_aggregate(organ, golden);
        // GoldenRatioGrowth ist am Ist x1.5 (axis_growth_policies.hpp:27 -- der Name meint die
        // folly/MSVC-Bauform, nicht die Zahl 1.618). Der Draht meldet den REALEN Faktor, nicht den Namen.
        check_eq("GoldenRatioGrowth: axis_stats[8][7]", golden.axis_stats[8][7], std::uint64_t{1500});
        check_true("der Faktor unterscheidet die beiden Algorithmen auf dem Draht",
                   golden.axis_stats[8][7] != agg.axis_stats[8][7]);
        check_true("beide melden reale Wachstums-Ereignisse", golden.axis_stats[8][0] > 0 && agg.axis_stats[8][0] > 0);
    }

    std::cout << "\n[F] Die ADDITIVE Policy: 0 ist der Sentinel 'kein multiplikativer Faktor'\n";
    {
        SeqOrganChunk organ{};
        for (std::uint64_t i = 0; i < 5; ++i) organ.push_back(i);
        cea::SequenceObserverAggregateWire chunk{};
        cea::fill_sequence_observer_aggregate(organ, chunk);
        check_eq("FixedChunkGrowth: axis_stats[8][7] == 0 (additiver Sentinel)", chunk.axis_stats[8][7],
                 std::uint64_t{0});
        // Der Unterschied zur NACKTEN Policy ist genau hier sichtbar und ist die D2-Aussage:
        // dort ist die ZEILE nicht befuellt (kein Wert), hier IST sie befuellt und der Wert IST 0.
        check_eq("die Zeile ist befuellt (der Wert 0 ist ein MESSERGEBNIS)", chunk.filled_axis_count, std::uint64_t{1});
        check_true("und sie traegt reale Wachstums-Ereignisse", chunk.axis_stats[8][0] > 0);
    }

    std::cout << "\n=== " << (g_fail == 0 ? "ALLE PRUEFUNGEN GRUEN" : "FEHLER") << " (" << g_fail << ") ===\n";
    return g_fail == 0 ? 0 : 1;
}
