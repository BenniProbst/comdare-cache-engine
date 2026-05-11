// Test fuer 10 Concurrency-Disziplinen + 3 Mechaniken + ConcurrencyManager
// Termin 7 / 02_uml_cache_engine §3

#include <cache_engine/concepts/concurrency_manager.hpp>
#include <cache_engine/concepts/disciplines/array_discipline.hpp>
#include <cache_engine/concepts/disciplines/data_structure_discipline.hpp>
#include <cache_engine/concepts/disciplines/memory_read_discipline.hpp>
#include <cache_engine/concepts/disciplines/memory_read_write_discipline.hpp>
#include <cache_engine/concepts/disciplines/memory_write_discipline.hpp>
#include <cache_engine/concepts/disciplines/node_discipline.hpp>
#include <cache_engine/concepts/disciplines/page_discipline.hpp>
#include <cache_engine/concepts/disciplines/path_discipline.hpp>
#include <cache_engine/concepts/disciplines/simd_flow_discipline.hpp>
#include <cache_engine/concepts/disciplines/simd_thread_discipline.hpp>
#include <cache_engine/concepts/mechanics/comdare_rcu_mechanic.hpp>
#include <cache_engine/concepts/mechanics/olc_mechanic.hpp>
#include <cache_engine/concepts/mechanics/rowex_mechanic.hpp>

#include <gtest/gtest.h>

namespace ce = comdare::cache_engine;

TEST(ConcurrencyDiscipline, AllTenKindsAreDistinct) {
    EXPECT_EQ(ce::PageDiscipline{}.kind(),            ce::ConcurrencyDisciplineKind::Page);
    EXPECT_EQ(ce::NodeDiscipline{}.kind(),            ce::ConcurrencyDisciplineKind::Node);
    EXPECT_EQ(ce::ArrayDiscipline{}.kind(),           ce::ConcurrencyDisciplineKind::Array);
    EXPECT_EQ(ce::DataStructureDiscipline{}.kind(),   ce::ConcurrencyDisciplineKind::DataStructure);
    EXPECT_EQ(ce::PathDiscipline{}.kind(),            ce::ConcurrencyDisciplineKind::Path);
    EXPECT_EQ(ce::MemoryReadDiscipline{}.kind(),      ce::ConcurrencyDisciplineKind::MemoryRead);
    EXPECT_EQ(ce::MemoryWriteDiscipline{}.kind(),     ce::ConcurrencyDisciplineKind::MemoryWrite);
    EXPECT_EQ(ce::MemoryReadWriteDiscipline{}.kind(), ce::ConcurrencyDisciplineKind::MemoryReadWrite);
    EXPECT_EQ(ce::SimdThreadDiscipline{}.kind(),      ce::ConcurrencyDisciplineKind::SimdThread);
    EXPECT_EQ(ce::SimdFlowDiscipline{}.kind(),        ce::ConcurrencyDisciplineKind::SimdFlow);
}

TEST(MemoryWriteDiscipline, BlockANCacheCoherenceCostMonotonicWithCores) {
    ce::MemoryWriteDiscipline d;
    ce::WriteEvent e1{}; e1.node_depth = 5; e1.num_cores_sharing = 8;
    ce::WriteEvent e2 = e1; e2.num_cores_sharing = 64;
    EXPECT_LT(d.compute_cache_coherence_cost(e1),
              d.compute_cache_coherence_cost(e2));
}

TEST(MemoryWriteDiscipline, OnEventCachesLastCost) {
    ce::MemoryWriteDiscipline d;
    ce::WriteEvent e{};
    e.kind = ce::EventKind::Write;
    e.node_depth = 3;
    e.num_cores_sharing = 16;
    d.on_event(e);
    EXPECT_GT(d.last_cost(), 0.0);
    EXPECT_EQ(d.last_event().num_cores_sharing, 16);
}

TEST(OLCMechanic, KindIsOlc) {
    ce::OLCMechanic m;
    EXPECT_EQ(m.kind(), ce::ConcurrencyMechanicKind::OLC);
}

TEST(OLCMechanic, WriterIncrementsVersion) {
    ce::OLCMechanic m;
    auto v0 = m.version();
    m.begin_write();
    m.end_write();
    EXPECT_EQ(m.version(), v0 + 2);
}

TEST(OLCMechanic, ReaderValidWhenNoWriteHappened) {
    ce::OLCMechanic m;
    m.begin_read();
    m.end_read();
    EXPECT_TRUE(m.last_read_valid());
}

TEST(OLCMechanic, ReaderInvalidatedByConcurrentWrite) {
    ce::OLCMechanic m;
    m.begin_read();
    m.begin_write(); m.end_write();
    m.end_read();
    EXPECT_FALSE(m.last_read_valid());
}

TEST(RowexMechanic, KindIsRowex) {
    ce::RowexMechanic m;
    EXPECT_EQ(m.kind(), ce::ConcurrencyMechanicKind::ROWEX);
}

TEST(RowexMechanic, WriterAcquiresAndReleases) {
    ce::RowexMechanic m;
    m.begin_write();
    m.end_write();   // muss ohne deadlock zurueckkehren
    SUCCEED();
}

TEST(RowexMechanic, ReadCounterIncrements) {
    ce::RowexMechanic m;
    m.begin_read(); m.end_read();
    m.begin_read(); m.end_read();
    m.begin_read(); m.end_read();
    EXPECT_EQ(m.read_count(), 3u);
}

TEST(ComdareRcuMechanic, KindIsComdareRcu) {
    ce::ComdareRcuMechanic m;
    EXPECT_EQ(m.kind(), ce::ConcurrencyMechanicKind::ComdareRcu);
}

TEST(ComdareRcuMechanic, RegisterDeregisterTracksThreads) {
    ce::ComdareRcuMechanic m;
    m.register_thread();
    m.register_thread();
    EXPECT_EQ(m.registered_threads(), 2u);
    m.deregister_thread();
    EXPECT_EQ(m.registered_threads(), 1u);
}

TEST(ComdareRcuMechanic, BeginEndReadDecrementsActive) {
    ce::ComdareRcuMechanic m;
    m.begin_read();
    EXPECT_EQ(m.active_readers(), 1u);
    m.end_read();
    EXPECT_EQ(m.active_readers(), 0u);
    EXPECT_GT(m.quiescent_states(), 0u);
}

TEST(ComdareRcuMechanic, SynchronizeReturnsWhenNoActiveReaders) {
    ce::ComdareRcuMechanic m;
    m.synchronize();   // muss sofort zurueckkehren wenn active_readers == 0
    SUCCEED();
}

TEST(ConcurrencyManager, ConfigurableViaTemplate) {
    using Manager = ce::ConcurrencyManager<ce::OLCMechanic,
                                           ce::PageDiscipline,
                                           ce::NodeDiscipline,
                                           ce::MemoryWriteDiscipline>;
    Manager mgr;
    EXPECT_EQ(mgr.discipline_count(), 3u);
    EXPECT_EQ(mgr.mechanic().kind(), ce::ConcurrencyMechanicKind::OLC);
}

TEST(ConcurrencyManager, DispatchPropagatesEventToAllDisciplines) {
    using Manager = ce::ConcurrencyManager<ce::OLCMechanic,
                                           ce::MemoryWriteDiscipline>;
    Manager mgr;
    ce::WriteEvent e{};
    e.kind = ce::EventKind::Write;
    e.node_depth = 2;
    e.num_cores_sharing = 4;
    mgr.dispatch(e);
    EXPECT_GT(mgr.discipline<0>().last_cost(), 0.0);
}
