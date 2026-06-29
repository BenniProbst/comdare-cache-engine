// V41 Roadmap-3 (Doku 24 §2.1/§2.2) — TierObserveTrace: Builder-seitiger Fuellstands-Mess-Treiber.
// Beweist: pro Fuellstand-Checkpoint werden BEIDE Mess-Dimensionen aus EINEM Lauf erhoben —
// (a) observe_all-Trace (search_algo + allocator REAL) + RAM-Proxy, (b) Tier-Wall-Clock getrennt
// nach read/write/delete ueber den Element-Fuellstand. In-Process (DLL-Grenze = R6, nicht hier).

#include <gtest/gtest.h>

#include <builder/anatomy_commands/tier_observe_trace.hpp>
#include <builder/commands/latency_stats.hpp>
#include <anatomy/known_algorithms.hpp>

#include <cstdint>
#include <span>

namespace bcmd      = ::comdare::cache_engine::builder::anatomy_commands;
namespace ce_compos = ::comdare::cache_engine::compositions;
namespace stats     = ::comdare::cache_engine::builder::commands::stats;

// T1 — Kurven-Vollstaendigkeit: 3 Checkpoints, Fuellstaende {10,100,1000}, load_factor streng monoton.
TEST(Roadmap3_TierObserveTrace, FillLevelCurvesAreComplete) {
    bcmd::TierTraceConfig cfg;
    cfg.fill_checkpoints       = {10, 100, 1000};
    cfg.lookups_per_checkpoint = 500;
    cfg.deletes_per_checkpoint = 50;
    auto const trace           = bcmd::drive_tier_observe_trace<ce_compos::ArtComposition>(cfg);

    ASSERT_EQ(trace.checkpoints.size(), 3u);
    EXPECT_EQ(trace.checkpoints[0].fill_level, 10u);
    EXPECT_EQ(trace.checkpoints[1].fill_level, 100u);
    EXPECT_EQ(trace.checkpoints[2].fill_level, 1000u);
    EXPECT_LT(trace.checkpoints[0].load_factor, trace.checkpoints[1].load_factor);
    EXPECT_LT(trace.checkpoints[1].load_factor, trace.checkpoints[2].load_factor);
    EXPECT_DOUBLE_EQ(trace.checkpoints[2].load_factor, 1.0);
    for (auto const& s : trace.checkpoints) {
        EXPECT_FALSE(s.write_ns.empty());
        EXPECT_FALSE(s.read_ns.empty());
    }
}

// T2 — read/write/delete sind GETRENNT erhoben.
TEST(Roadmap3_TierObserveTrace, ReadWriteDeleteSeparated) {
    bcmd::TierTraceConfig cfg;
    cfg.fill_checkpoints       = {50, 200};
    cfg.lookups_per_checkpoint = 300;
    cfg.deletes_per_checkpoint = 40;
    auto const trace           = bcmd::drive_tier_observe_trace<ce_compos::ArtComposition>(cfg);
    ASSERT_EQ(trace.checkpoints.size(), 2u);
    for (auto const& s : trace.checkpoints) {
        EXPECT_EQ(s.read_ns.size(), 300u); // genau lookups_per_checkpoint
        EXPECT_FALSE(s.write_ns.empty());
        EXPECT_FALSE(s.delete_ns.empty()); // r/w/d als drei unabhaengige Vektoren
    }
}

// T7 — Wiederverwendung der bestehenden latency_stats: p50 <= p99 je Operationstyp.
TEST(Roadmap3_TierObserveTrace, LatencyStatsReuseP50LeP99) {
    auto const trace = bcmd::drive_tier_observe_trace<ce_compos::ArtComposition>({});
    ASSERT_FALSE(trace.checkpoints.empty());
    auto const& s   = trace.checkpoints.back();
    auto const  r50 = stats::latency_p50_ns(std::span<const std::int64_t>{s.read_ns});
    auto const  r99 = stats::latency_p99_ns(std::span<const std::int64_t>{s.read_ns});
    auto const  w50 = stats::latency_p50_ns(std::span<const std::int64_t>{s.write_ns});
    auto const  w99 = stats::latency_p99_ns(std::span<const std::int64_t>{s.write_ns});
    EXPECT_LE(r50.count(), r99.count());
    EXPECT_LE(w50.count(), w99.count());
}

// T9 — Treiber ist composition-agnostisch (HotComposition).
TEST(Roadmap3_TierObserveTrace, WorksForHotComposition) {
    auto const trace = bcmd::drive_tier_observe_trace<ce_compos::HotComposition>({});
    ASSERT_EQ(trace.checkpoints.size(), 3u);
    EXPECT_EQ(trace.checkpoints.back().fill_level, 1000u);
    EXPECT_FALSE(trace.composition_name.empty());
}

#ifdef COMDARE_CE_ENABLE_STATISTICS
// T3 — observe_all real pro Checkpoint: search_algo + allocator getrieben (kumulativ wachsend).
TEST(Roadmap3_TierObserveTrace, ObserveAllRealPerCheckpoint) {
    bcmd::TierTraceConfig cfg;
    cfg.fill_checkpoints       = {10, 100, 1000};
    cfg.lookups_per_checkpoint = 500;
    cfg.deletes_per_checkpoint = 50;
    auto const trace           = bcmd::drive_tier_observe_trace<ce_compos::ArtComposition>(cfg);
    ASSERT_EQ(trace.checkpoints.size(), 3u);

    // search_algo: insert_count > 0 und monoton; lookup_count kumulativ >= lookups*(i+1).
    EXPECT_GT(trace.checkpoints[0].search_algo_at_checkpoint.total_insert_count, 0u);
    EXPECT_LE(trace.checkpoints[0].search_algo_at_checkpoint.total_insert_count,
              trace.checkpoints[2].search_algo_at_checkpoint.total_insert_count);
    EXPECT_GE(trace.checkpoints[0].search_algo_at_checkpoint.total_lookup_count, 500u);
    EXPECT_GE(trace.checkpoints[2].search_algo_at_checkpoint.total_lookup_count, 1500u);

    // allocator: real getrieben durch den inneren ComposedStore-Vector.
    EXPECT_GT(trace.checkpoints[2].allocator_at_checkpoint.allocation_count, 0u);
    EXPECT_GT(trace.checkpoints[2].allocator_at_checkpoint.total_bytes_in_use, 0u);
}

// T4 — RAM-Proxy (allocator total_bytes_in_use) waechst monoton mit dem Fuellstand.
TEST(Roadmap3_TierObserveTrace, RamProxyMonotonicWithFillLevel) {
    bcmd::TierTraceConfig cfg;
    cfg.fill_checkpoints = {10, 100, 1000};
    auto const trace     = bcmd::drive_tier_observe_trace<ce_compos::ArtComposition>(cfg);
    ASSERT_EQ(trace.checkpoints.size(), 3u);
    EXPECT_GT(trace.checkpoints[0].ram_bytes_in_use, 0u);
    EXPECT_LE(trace.checkpoints[0].ram_bytes_in_use, trace.checkpoints[1].ram_bytes_in_use);
    EXPECT_LE(trace.checkpoints[1].ram_bytes_in_use, trace.checkpoints[2].ram_bytes_in_use);
}
#endif
