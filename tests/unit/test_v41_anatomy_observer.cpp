// V41.F.6.1.R5.A — ObserverAggregate ABI-Stabilitaet + observe_all() Tests
//
// Beweist:
// 1. EmptyAxisSnapshot ist standard_layout + trivially_copyable
// 2. ObservableAxis Concept funktioniert (Detection von statistics())
// 3. snapshot_of_t graceful fallback fuer non-observable Achsen
// 4. ObserverAggregate<Composition> hat 17 named Snapshot-Members
// 5. SearchAlgorithmAnatomy<C>::observe_all() liefert ObserverAggregate
// 6. observable_axis_count() korrekte Compile-Time-Diagnose
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §17.2 + §20
// @task #697 V41.F.6.1.R5.A

#include <gtest/gtest.h>

#include <anatomy/observer_aggregate.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <anatomy/known_algorithms.hpp>

#include <type_traits>
#include <cstring>   // V42 L-74c: std::memcpy im memory_layout-Test
#include <cstdint>
#include <cstddef>

namespace ana       = ::comdare::cache_engine::anatomy;
namespace ce_compos = ::comdare::cache_engine::compositions;

// ─────────────────────────────────────────────────────────────────────────────
// §1 — EmptyAxisSnapshot POD-Eigenschaften (ABI-Stabilitaet)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5A_EmptyAxisSnapshot, IsStandardLayoutAndTriviallyCopyable) {
    static_assert(std::is_standard_layout_v<ana::EmptyAxisSnapshot>);
    static_assert(std::is_trivially_copyable_v<ana::EmptyAxisSnapshot>);
    static_assert(std::is_trivially_default_constructible_v<ana::EmptyAxisSnapshot>);
    SUCCEED();
}

TEST(R5A_EmptyAxisSnapshot, EqualityIsTrivialTrue) {
    ana::EmptyAxisSnapshot a;
    ana::EmptyAxisSnapshot b;
    EXPECT_TRUE(a == b);
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — ObservableAxis Concept Detection
// ─────────────────────────────────────────────────────────────────────────────

struct WithStatistics {
    struct snapshot_t { std::uint64_t count{0}; };
    [[nodiscard]] snapshot_t statistics() const noexcept { return {}; }
};

struct WithoutStatistics {
    // keine snapshot_t / statistics()
};

TEST(R5A_ObservableAxisConcept, DetectionWorks) {
    static_assert(ana::ObservableAxis<WithStatistics>);
    static_assert(!ana::ObservableAxis<WithoutStatistics>);
    SUCCEED();
}

TEST(R5A_SnapshotOf, GracefulFallbackForNonObservable) {
    static_assert(std::is_same_v<ana::snapshot_of_t<WithStatistics>, WithStatistics::snapshot_t>);
    static_assert(std::is_same_v<ana::snapshot_of_t<WithoutStatistics>, ana::EmptyAxisSnapshot>);
    SUCCEED();
}

TEST(R5A_SnapshotAxis, ExtractsSnapshotForObservable) {
    WithStatistics ws;
    auto snap = ana::snapshot_axis(ws);
    EXPECT_EQ(snap.count, 0u);  // Default-initialisierter Snapshot
}

TEST(R5A_SnapshotAxis, EmptyForNonObservable) {
    WithoutStatistics wo;
    auto snap = ana::snapshot_axis(wo);
    static_assert(std::is_same_v<decltype(snap), ana::EmptyAxisSnapshot>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — ObserverAggregate Komposition mit 17 Members
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5A_ObserverAggregate, ArtCompositionHasSeventeenSlots) {
    using Agg = ana::ObserverAggregate<ce_compos::ArtComposition>;
    static_assert(Agg::total_slots() == 17);
    SUCCEED();
}

TEST(R5A_ObserverAggregate, AllSixCompositionsConformLayout) {
    // Pflicht: ObserverAggregate fuer jede der 6 CE-Re-Impl-Compositions instantiierbar
    using AggArt      = ana::ObserverAggregate<ce_compos::ArtComposition>;
    using AggHot      = ana::ObserverAggregate<ce_compos::HotComposition>;
    using AggWormhole = ana::ObserverAggregate<ce_compos::WormholeComposition>;
    using AggSurf     = ana::ObserverAggregate<ce_compos::SurfComposition>;
    using AggMasstree = ana::ObserverAggregate<ce_compos::MasstreeComposition>;
    using AggStart    = ana::ObserverAggregate<ce_compos::StartComposition>;
    static_assert(AggArt::total_slots()      == 17);
    static_assert(AggHot::total_slots()      == 17);
    static_assert(AggWormhole::total_slots() == 17);
    static_assert(AggSurf::total_slots()     == 17);
    static_assert(AggMasstree::total_slots() == 17);
    static_assert(AggStart::total_slots()    == 17);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — SearchAlgorithmAnatomy::observe_all() liefert ObserverAggregate
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5A_AnatomyObserveAll, ArtAnatomyProducesAggregate) {
    ana::Art anatomy;
    auto agg = anatomy.observe_all();
    // R5.A Pilot: Default-Aggregate (Achsen-Member-Aggregation R5.B pending)
    using AggT = decltype(agg);
    static_assert(AggT::total_slots() == 17);
    SUCCEED();
}

TEST(R5A_AnatomyObserveAll, AllElevenAnatomiesProduceAggregate) {
    // 6 CE-Re-Impl + 5 PaperBinding = 11 Anatomien — alle liefern Default-Aggregate
    [[maybe_unused]] auto agg1  = ana::Art{}.observe_all();
    [[maybe_unused]] auto agg2  = ana::Hot{}.observe_all();
    [[maybe_unused]] auto agg3  = ana::Wormhole{}.observe_all();
    [[maybe_unused]] auto agg4  = ana::SuRF{}.observe_all();
    [[maybe_unused]] auto agg5  = ana::Masstree{}.observe_all();
    [[maybe_unused]] auto agg6  = ana::Start{}.observe_all();
    [[maybe_unused]] auto agg7  = ana::ArtPaperBinding{}.observe_all();
    [[maybe_unused]] auto agg8  = ana::HotPaperBinding{}.observe_all();
    [[maybe_unused]] auto agg9  = ana::StartPaperBinding{}.observe_all();
    [[maybe_unused]] auto agg10 = ana::WormholePaperBinding{}.observe_all();
    [[maybe_unused]] auto agg11 = ana::SurfPaperBinding{}.observe_all();
    static_assert(decltype(agg1)::total_slots()  == 17);
    static_assert(decltype(agg11)::total_slots() == 17);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — Observable-Axis-Count Diagnose
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5A_ObservableAxisCount, ArtAnatomyDiagnoseIsCompileTime) {
    // Stand 2026-05-26: Stufe-A-Wrappers haben statistics() nur unter
    // COMDARE_CE_ENABLE_STATISTICS. Hier wird der Count zur Compile-Time geprueft —
    // wenn Statistics-Flag aus ist, sind 0 oder wenig Achsen observable.
    constexpr auto count = ana::Art::observable_axis_count();
    static_assert(count <= 17);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — ABI-Stabilitaet: ObserverAggregate mit nur EmptyAxisSnapshot ist POD
// ─────────────────────────────────────────────────────────────────────────────

// Composition wo ALLE Achsen non-observable sind (EmptyAxisSnapshot ueberall)
struct AllEmptyComposition {
    using search_algo        = WithoutStatistics;
    using cache_traversal    = WithoutStatistics;
    using mapping            = WithoutStatistics;
    using path_compression   = WithoutStatistics;
    using node_type          = WithoutStatistics;
    using memory_layout      = WithoutStatistics;
    using allocator          = WithoutStatistics;
    using prefetch           = WithoutStatistics;
    using concurrency        = WithoutStatistics;
    using serialization      = WithoutStatistics;
    using telemetry          = WithoutStatistics;
    using value_handle       = WithoutStatistics;
    using isa                = WithoutStatistics;
    using index_organization = WithoutStatistics;
    using io_dispatch        = WithoutStatistics;
    using migration_policy   = WithoutStatistics;
    using filter             = WithoutStatistics;
    static constexpr std::string_view name     = "AllEmptyComposition";
    static constexpr std::string_view paper_id = "P00 EmptyTest";
};

TEST(R5A_AbiStability, AllEmptyAggregateIsStandardLayoutPod) {
    using Agg = ana::ObserverAggregate<AllEmptyComposition>;
    static_assert(std::is_standard_layout_v<Agg>);
    static_assert(std::is_trivially_copyable_v<Agg>);
    static_assert(std::is_trivially_default_constructible_v<Agg>);
    static_assert(Agg::observable_count() == 0);
    static_assert(Agg::total_slots()      == 17);
    SUCCEED();
}

// V41 Saeule-2-Korrektur (Doku 24 §2.2/§3): observe_all() liefert jetzt ECHTE Per-Achsen-statistics()
// statt EmptyAxisSnapshot-Stub. Beweis: das search_algo-Organ der Anatomie wird getrieben → seine
// Insert/Lookup/Hit/Miss-Statistik fliesst real in das ObserverAggregate (= Per-Achsen-Statistics-Trace).
TEST(Saeule2_ObserveAllReal, DrivenSearchAlgoOrganFlowsIntoAggregate) {
    ana::SearchAlgorithmAnatomy<ce_compos::ArtComposition> anat;
    using Organ = ce_compos::ArtComposition::search_algo;  // Array256SearchAlgo
    if constexpr (ana::ObservableAxis<Organ>) {
        using K = typename Organ::key_type;
        auto& organ = anat.search_algo_organ();
        for (int i = 0; i < 40; ++i) organ.insert(static_cast<K>(i), static_cast<std::uint64_t>(i) * 3u + 1u);
        for (int i = 0; i < 40; ++i) (void)organ.lookup(static_cast<K>(i));   // Treffer

        auto const agg = anat.observe_all();
        // Vorher (Stub): alle Werte 0. Jetzt: ECHTE search_algo-Statistik aus dem getriebenen Organ.
        EXPECT_EQ(agg.search_algo.total_insert_count, 40u);
        EXPECT_GE(agg.search_algo.total_lookup_count, 40u);
        EXPECT_GE(agg.search_algo.total_hit_count, 40u);
        EXPECT_GT(agg.search_algo.peak_occupancy, 0u);
    } else {
        GTEST_SKIP() << "search_algo nicht ObservableAxis (STATISTICS=OFF)";
    }
}

// V42 L-74c Composition-Driver: telemetry als 2. getriebenes Organ. ArtComposition::telemetry ist jetzt die
// ObservableTelemetry-Huelle → getrieben via telemetry_organ() fliesst sie real in observe_all().telemetry.
// Die LeafOnlyCounter-Strategie VERWIRFT Inner-Node-Touches (node_updates==0) — der messbare Achsen-Unterschied.
TEST(Saeule2_ObserveAllReal, DrivenTelemetryOrganFlowsIntoAggregate) {
    ana::SearchAlgorithmAnatomy<ce_compos::ArtComposition> anat;
    using TelOrgan = ce_compos::ArtComposition::telemetry;  // ObservableTelemetry<LeafOnlyCounter>
    if constexpr (ana::ObservableAxis<TelOrgan>) {
        auto& tel = anat.telemetry_organ();
        tel.record_node_touch(true);    // Blatt
        tel.record_node_touch(false);   // Inner → leaf-only verwirft
        tel.record_node_touch(true);    // Blatt

        auto const agg = anat.observe_all();
        EXPECT_EQ(agg.telemetry.total_events, 3u);
        EXPECT_EQ(agg.telemetry.leaf_updates, 2u);
        EXPECT_EQ(agg.telemetry.node_updates, 0u);   // LeafOnlyCounter-Strategie verwirft Inner-Touch
        EXPECT_EQ(agg.telemetry.peak_tracked, 2u);
        // observable_count steigt: search_algo + telemetry beide getrieben + observierbar.
        EXPECT_GE(ana::ObserverAggregate<ce_compos::ArtComposition>::observable_count(), 2u);
    } else {
        GTEST_SKIP() << "telemetry nicht ObservableAxis (STATISTICS=OFF)";
    }
}

// V42 L-74c Composition-Driver: memory_layout als 3. getriebenes Organ. ArtComposition::memory_layout ist
// jetzt die ObservableMemoryLayout-Huelle; observe_scan() treibt die Layout-Scan-Statistik in observe_all().
TEST(Saeule2_ObserveAllReal, DrivenMemoryLayoutOrganFlowsIntoAggregate) {
    ana::SearchAlgorithmAnatomy<ce_compos::ArtComposition> anat;
    using MlOrgan = ce_compos::ArtComposition::memory_layout;  // ObservableMemoryLayout<CacheLineAligned>
    if constexpr (ana::ObservableAxis<MlOrgan>) {
        constexpr std::size_t record_size = 64, n = 4;
        unsigned char buf[record_size * n] = {};
        std::uint32_t const vals[n] = {10u, 20u, 30u, 40u};   // Summe = 100
        for (std::size_t i = 0; i < n; ++i) std::memcpy(buf + i * record_size, &vals[i], sizeof(std::uint32_t));

        auto& ml = anat.memory_layout_organ();
        std::uint64_t const checksum = ml.observe_scan(buf, n, record_size);
        EXPECT_EQ(checksum, 100u);

        auto const agg = anat.observe_all();
        EXPECT_EQ(agg.memory_layout.scan_count, 1u);
        EXPECT_EQ(agg.memory_layout.records_scanned, 4u);
        EXPECT_EQ(agg.memory_layout.last_checksum, 100u);
        EXPECT_GT(agg.memory_layout.cache_lines_touched, 0u);
        EXPECT_GE(ana::ObserverAggregate<ce_compos::ArtComposition>::observable_count(), 3u);  // search_algo+telemetry+memory_layout
    } else {
        GTEST_SKIP() << "memory_layout nicht ObservableAxis (STATISTICS=OFF)";
    }
}

// V42 L-74c Composition-Driver: serialization als 4. getriebenes Organ. ArtComposition::serialization ist
// jetzt die ObservableSerialization-Huelle; observe_serialize() treibt die Statistik in observe_all().
TEST(Saeule2_ObserveAllReal, DrivenSerializationOrganFlowsIntoAggregate) {
    ana::SearchAlgorithmAnatomy<ce_compos::ArtComposition> anat;
    using SerOrgan = ce_compos::ArtComposition::serialization;  // ObservableSerialization<RawBinary>
    if constexpr (ana::ObservableAxis<SerOrgan>) {
        constexpr std::size_t record_size = 8, n = 4;
        unsigned char buf[record_size * n] = {};
        std::uint32_t const vals[n] = {10u, 20u, 30u, 40u};   // Summe = 100
        for (std::size_t i = 0; i < n; ++i) std::memcpy(buf + i * record_size, &vals[i], sizeof(std::uint32_t));

        auto& ser = anat.serialization_organ();
        std::uint64_t const checksum = ser.observe_serialize(buf, n, record_size);
        EXPECT_EQ(checksum, 100u);

        auto const agg = anat.observe_all();
        EXPECT_EQ(agg.serialization.serialize_count, 1u);
        EXPECT_EQ(agg.serialization.records_serialized, 4u);
        EXPECT_EQ(agg.serialization.bytes_serialized, 32u);     // 4 * 8
        EXPECT_EQ(agg.serialization.last_checksum, 100u);
        EXPECT_GE(ana::ObserverAggregate<ce_compos::ArtComposition>::observable_count(), 4u);  // +serialization
    } else {
        GTEST_SKIP() << "serialization nicht ObservableAxis (STATISTICS=OFF)";
    }
}

// V42 L-74c Composition-Driver: node_type als 5. getriebenes Organ — schliesst die 4 OperativeCapable-Achsen
// (telemetry+memory_layout+serialization+node_type) ab. ArtComposition::node_type ist die ObservableNodeType-
// Huelle (zugleich N in ComposedStore<N,L,A>); observe_node_find() treibt die Statistik in observe_all().
TEST(Saeule2_ObserveAllReal, DrivenNodeTypeOrganFlowsIntoAggregate) {
    ana::SearchAlgorithmAnatomy<ce_compos::ArtComposition> anat;
    using NtOrgan = ce_compos::ArtComposition::node_type;  // ObservableNodeType<Node256>
    if constexpr (ana::ObservableAxis<NtOrgan>) {
        std::uint8_t const stored[4]  = {1u, 2u, 3u, 4u};
        std::uint8_t const queries[3] = {2u, 4u, 9u};   // 2(+2),4(+4),9(miss) -> Summe 6
        auto& nt = anat.node_type_organ();
        std::uint64_t const checksum = nt.observe_node_find(stored, 4, queries, 3);
        EXPECT_EQ(checksum, 6u);

        auto const agg = anat.observe_all();
        EXPECT_EQ(agg.node_type.find_count, 1u);
        EXPECT_EQ(agg.node_type.keys_stored, 4u);
        EXPECT_EQ(agg.node_type.queries_run, 3u);
        EXPECT_EQ(agg.node_type.last_checksum, 6u);
        // ALLE 4 OperativeCapable-Achsen + search_algo observable.
        EXPECT_GE(ana::ObserverAggregate<ce_compos::ArtComposition>::observable_count(), 5u);
    } else {
        GTEST_SKIP() << "node_type nicht ObservableAxis (STATISTICS=OFF)";
    }
}
