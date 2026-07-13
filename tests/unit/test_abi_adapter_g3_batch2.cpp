// G3 Batch-2 — zwei additive Correctness-Guards am realen SearchAlgorithmAbiAdapter (ArtComposition):
//
//  * M-CE-06 (nullptr-Guard): run_workload / _segmented / _segmented_v2 prüfen alloc.allocate() jetzt auf
//    nullptr (OOM-Ablehnung der Alloc-Achsen statt UB-Write). Der OOM-Zweig selbst ist über die öffentliche
//    ABI ohne Mock-Allocator NICHT auslösbar; getestet wird hier die HAPPY-PATH-Neutralität: alle drei
//    Mess-Treiber liefern weiterhin echte Samples/ns (die Guards brechen den Normalbetrieb nicht).
//
//  * M-CE (uint16-Narrow-Cast-Guard): der Mapping-Observer-Seitenpfad in tier_insert/tier_lookup castete key
//    (uint64) ungeguardet auf slot_index_type (uint16) → stille Trunkierung/Slot-Wrap ab key>=2^16. Der
//    additive Kapazitaets-Guard (analog #217-2a) lehnt out-of-range-Keys aus der Mapping-Observer-Kopplung
//    EHRLICH ab. Dieser Test beweist: out-of-range-Keys erzeugen NULL Mapping-Registrierungen (kein Slot-Wrap),
//    während der konstitutive uint64-Store sie regulär distinkt aufnimmt.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/measurable_workload.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <compositions/art_reference.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;

namespace {

using Tier = an::SearchAlgorithmAbiAdapter<an::SearchAlgorithmAnatomy<comp::ArtComposition>>;

[[nodiscard]] an::IObservableTier* observable(Tier& t) {
    return dynamic_cast<an::IObservableTier*>(static_cast<an::IAnatomyBase*>(&t));
}

// axis_stats[T2] == Mapping-Observer: [0] total_register_count, [5] peak_mapped (fill_observer_v3, Schema V3).
constexpr std::size_t kAxisMapping        = 2;
constexpr std::size_t kFieldRegisterCount = 0;
constexpr std::size_t kFieldPeakMapped    = 5;

} // namespace

TEST(AbiAdapterG3Batch2, Uint16NarrowCastGuardRejectsOutOfRangeKeysFromMappingObserver) {
    // Baseline-Adapter: ausschließlich in-range Keys (0..kInRange-1) → Slots 0..kInRange-1.
    constexpr std::uint64_t kInRange = 12;
    Tier                    a;
    auto*                   obsA = observable(a);
    ASSERT_NE(obsA, nullptr);
    obsA->tier_clear();
    for (std::uint64_t i = 0; i < kInRange; ++i) { ASSERT_TRUE(obsA->tier_insert(i, i * 7u + 1u)); }
    an::ComdareTierObserverSnapshot snapA{};
    obsA->tier_observe(&snapA);
    std::uint64_t const reg_baseline  = snapA.axis_stats[kAxisMapping][kFieldRegisterCount];
    std::uint64_t const peak_baseline = snapA.axis_stats[kAxisMapping][kFieldPeakMapped];

    // Sanity: der Mapping-Observer wird bei ArtComposition (DirectPlacement) real getrieben — sonst wäre der
    // Guard-Test vakuum-wahr.
    ASSERT_GT(reg_baseline, 0u) << "Mapping-Observer bei ArtComposition nicht getrieben — Test-Voraussetzung verletzt";
    ASSERT_GT(peak_baseline, 0u);

    // Mixed-Adapter: dieselben in-range Keys + out-of-range Keys (>= 2^16). Die gewählten Big-Keys wrappen
    // (ungeguardet) auf FREIE Slots 1000..1007 (disjunkt von 0..11) → OHNE Fix stiegen register_count UND
    // peak_mapped um kOutOfRange. MIT Fix bleiben beide exakt auf dem in-range-Baseline.
    constexpr std::uint64_t kOutOfRange = 8;
    constexpr std::uint64_t kBigBase    = (std::uint64_t{1} << 16) + 1000u; // 66536.. → uint16 → Slots 1000..1007
    Tier                    b;
    auto*                   obsB = observable(b);
    ASSERT_NE(obsB, nullptr);
    obsB->tier_clear();
    for (std::uint64_t i = 0; i < kInRange; ++i) { ASSERT_TRUE(obsB->tier_insert(i, i * 7u + 1u)); }
    for (std::uint64_t i = 0; i < kOutOfRange; ++i) {
        std::uint64_t const key = kBigBase + i;
        (void)obsB->tier_insert(key, key * 7u + 1u); // konstitutiver Store nimmt den uint64-Key regulär auf
    }
    an::ComdareTierObserverSnapshot snapB{};
    obsB->tier_observe(&snapB);

    // (1) Der konstitutive Store hat ALLE Keys distinkt aufgenommen (uint64, kein Trunkierungs-Kollaps) — belegt
    //     zugleich, dass die out-of-range-Keys den #217-2a-Kapazitaets-Guard oben (Z.854) passiert haben.
    EXPECT_EQ(obsB->tier_size(), kInRange + kOutOfRange);

    // (2) KERN: die out-of-range-Keys erzeugten NULL zusätzliche Mapping-Registrierungen (ehrliche Ablehnung,
    //     kein stiller Slot-Wrap). register_count UND peak_mapped müssen exakt dem in-range-Baseline entsprechen.
    EXPECT_EQ(snapB.axis_stats[kAxisMapping][kFieldRegisterCount], reg_baseline)
        << "out-of-range Keys erzeugten Mapping-Registrierungen (stiller uint16-Slot-Wrap statt Ablehnung)";
    EXPECT_EQ(snapB.axis_stats[kAxisMapping][kFieldPeakMapped], peak_baseline)
        << "out-of-range Keys hoben peak_mapped (Phantom-Slots durch uint16-Trunkierung)";
}

TEST(AbiAdapterG3Batch2, NullptrGuardsAreHappyPathNeutralAcrossAllWorkloadDrivers) {
    Tier  tier;
    auto* base = static_cast<an::IAnatomyBase*>(&tier);

    // (A) run_workload (IMeasurableWorkload) — lbuf-Guard + Churn-Guard im 4-Segment-do_batch.
    auto* mw = dynamic_cast<an::IMeasurableWorkload*>(base);
    ASSERT_NE(mw, nullptr);
    std::array<std::int64_t, 16> lat{};
    std::uint64_t const n1 = mw->run_workload(/*ops*/ 512, /*batches*/ 8, /*seed*/ 0xB15u, lat.data(), lat.size());
    EXPECT_EQ(n1, 8u) << "run_workload lieferte nicht die erwartete Sample-Zahl (Guard bricht Happy-Path)";
    EXPECT_GT(lat[0], 0) << "run_workload: erster Batch-ns muss > 0 sein";

    // (B) run_workload_segmented (IMeasurableWorkloadV2) — 4-Segment-Timer, gleiche Alloc-Pfade.
    auto* v2 = dynamic_cast<an::IMeasurableWorkloadV2*>(base);
    ASSERT_NE(v2, nullptr);
    an::ComdareSegmentLatencyV1 seg1{};
    std::uint64_t const         n2 = v2->run_workload_segmented(/*ops*/ 512, /*batches*/ 8, /*seed*/ 0xC2u, &seg1);
    EXPECT_EQ(n2, 8u) << "run_workload_segmented lieferte nicht die erwartete Batch-Zahl";
    EXPECT_GT(seg1.total_ns, 0) << "run_workload_segmented: total_ns muss > 0 sein";

    // (C) run_workload_segmented_v2 (IMeasurableWorkloadV3) — 19-Segment-Treiber (lbuf-Guard + Churn-Guard T6).
    auto* v3 = dynamic_cast<an::IMeasurableWorkloadV3*>(base);
    ASSERT_NE(v3, nullptr);
    an::ComdareSegmentLatencyV2 seg2{};
    std::uint64_t const         n3 = v3->run_workload_segmented_v2(/*ops*/ 256, /*batches*/ 4, /*seed*/ 0xD3u, &seg2);
    EXPECT_EQ(n3, 4u) << "run_workload_segmented_v2 lieferte nicht die erwartete Batch-Zahl";
    EXPECT_GT(seg2.total_ns, 0) << "run_workload_segmented_v2: total_ns muss > 0 sein";
}
