// Test fuer InMemoryMeasurementBuffer (F5) + Measure-Matrix (F1) + F-EXTRA-7 + F11
// Termin 7 / 06_uml_persistence + 03_uml_measurement

#include <cache_engine/measurement/in_memory_measurement_buffer.hpp>
#include <cache_engine/measurement/measure.hpp>
#include <cache_engine/measurement/measurement_category.hpp>
#include <cache_engine/measurement/measurement_record.hpp>
#include <cache_engine/measurement/thread_arena.hpp>

#include <gtest/gtest.h>
#include <thread>

namespace cem = comdare::cache_engine::measurement;

// ─────────────────────────────────────────────────────────────────────────────
// MeasurementRecord
// ─────────────────────────────────────────────────────────────────────────────

TEST(MeasurementRecord, Is32BytesAlignedAndPacked) {
    EXPECT_EQ(sizeof(cem::MeasurementRecord), 32u);
    EXPECT_EQ(alignof(cem::MeasurementRecord), 32u);
    EXPECT_TRUE(std::is_trivially_copyable_v<cem::MeasurementRecord>);
}

TEST(MeasurementRecord, FieldsAccessible) {
    cem::MeasurementRecord r{};
    r.category = static_cast<std::uint8_t>(cem::MeasurementCategory::CLU);
    r.algo_detail = static_cast<std::uint16_t>(cem::AlgoDetail::ART_NODE256);
    r.metric_value = 0.873;
    r.timestamp_ns = 1000;
    EXPECT_EQ(r.category, 0);
    EXPECT_EQ(r.algo_detail, 4);
    EXPECT_DOUBLE_EQ(r.metric_value, 0.873);
}

// ─────────────────────────────────────────────────────────────────────────────
// MeasurementCategory + AlgoDetail
// ─────────────────────────────────────────────────────────────────────────────

TEST(MeasurementCategory, SixteenCategoriesExist) {
    EXPECT_EQ(static_cast<int>(cem::MeasurementCategory::CLU),                   0);
    EXPECT_EQ(static_cast<int>(cem::MeasurementCategory::CACHE_MISS_L1),         1);
    EXPECT_EQ(static_cast<int>(cem::MeasurementCategory::CACHE_MISS_L3),         3);
    EXPECT_EQ(static_cast<int>(cem::MeasurementCategory::LATENCY_P999),         12);
    EXPECT_EQ(static_cast<int>(cem::MeasurementCategory::FILL_BUFFER_OCCUPANCY),15);
}

TEST(AlgoDetail, PrtartTagsAreInDistinctRange) {
    EXPECT_EQ(static_cast<int>(cem::AlgoDetail::PRTART_DENSEBYTE),    100);
    EXPECT_EQ(static_cast<int>(cem::AlgoDetail::PRTART_CUSTOMCACHE), 104);
    EXPECT_LT(static_cast<int>(cem::AlgoDetail::ART_NODE256),
              static_cast<int>(cem::AlgoDetail::PRTART_DENSEBYTE));
}

// ─────────────────────────────────────────────────────────────────────────────
// ThreadArena
// ─────────────────────────────────────────────────────────────────────────────

TEST(ThreadArena, AppendIncrementsSizeAndBytes) {
    cem::ThreadArena arena{std::this_thread::get_id()};
    cem::MeasurementRecord r{};
    arena.append(r);
    arena.append(r);
    EXPECT_EQ(arena.size(), 2u);
    EXPECT_EQ(arena.bytes(), 64u);    // 2 * 32 Byte
}

TEST(ThreadArena, ResetClearsRecords) {
    cem::ThreadArena arena{std::this_thread::get_id()};
    cem::MeasurementRecord r{};
    arena.append(r);
    arena.reset();
    EXPECT_EQ(arena.size(), 0u);
    EXPECT_EQ(arena.bytes(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryMeasurementBuffer (F5)
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryMeasurementBuffer, EmptyBufferTotalIsZero) {
    cem::InMemoryMeasurementBuffer buf;
    EXPECT_EQ(buf.total_records(), 0u);
    EXPECT_EQ(buf.arena_count(), 0u);
}

TEST(InMemoryMeasurementBuffer, AppendIncreasesTotal) {
    cem::InMemoryMeasurementBuffer buf;
    cem::MeasurementRecord r{};
    buf.append_record(r);
    buf.append_record(r);
    buf.append_record(r);
    EXPECT_EQ(buf.total_records(), 3u);
    EXPECT_EQ(buf.arena_count(), 1u);  // alle in dieser Thread-Arena
}

TEST(InMemoryMeasurementBuffer, ResetForRunClearsArenas) {
    cem::InMemoryMeasurementBuffer buf;
    cem::MeasurementRecord r{};
    buf.append_record(r);
    buf.reset_for_run(42);
    EXPECT_EQ(buf.total_records(), 0u);
    EXPECT_EQ(buf.run_id(), 42u);
}

TEST(InMemoryMeasurementBuffer, MakeDumpHeaderReflectsState) {
    cem::InMemoryMeasurementBuffer buf;
    buf.reset_for_run(7);
    cem::MeasurementRecord r{};
    buf.append_record(r);
    auto h = buf.make_dump_header(0xCAFE);
    EXPECT_EQ(h.run_id, 7u);
    EXPECT_EQ(h.platform_sig, 0xCAFEu);
    EXPECT_EQ(h.record_count, 1u);
    EXPECT_EQ(h.record_size, sizeof(cem::MeasurementRecord));
}

// ─────────────────────────────────────────────────────────────────────────────
// Measure<Cat, Detail> constexpr template (F1)
// ─────────────────────────────────────────────────────────────────────────────

TEST(MeasureTemplate, DefaultPrimaryTemplateIsNoOp) {
    cem::Context ctx{};
    using M = cem::Measure<cem::MeasurementCategory::CLU, cem::AlgoDetail::CSS_NODE>;
    M::at_node_visit(64, ctx);
    EXPECT_EQ(ctx.cache_lines_used, 0u);
    EXPECT_DOUBLE_EQ(M::extract(ctx), 0.0);
}

TEST(MeasureTemplate, ArtNode256SpecializationCountsCacheLines) {
    cem::Context ctx{};
    using M = cem::Measure<cem::MeasurementCategory::CLU, cem::AlgoDetail::ART_NODE256>;
    M::at_node_visit(128, ctx);
    EXPECT_EQ(ctx.cache_lines_used, 2u);
    EXPECT_DOUBLE_EQ(M::extract(ctx), 1.0);
}

TEST(MeasureTemplate, LatencyMeanTemplateAcceptsAnyDetail) {
    cem::Context ctx{};
    ctx.lookup_start_ns = 100;
    ctx.lookup_end_ns   = 250;
    using M = cem::Measure<cem::MeasurementCategory::LATENCY_MEAN,
                           cem::AlgoDetail::HOT_COMPOUND_K32>;
    EXPECT_DOUBLE_EQ(M::extract(ctx), 150.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// MeasurementHooks (F11 Trigger-Modus)
// ─────────────────────────────────────────────────────────────────────────────

TEST(MeasurementHooks, ContinuousAlwaysRecords) {
    cem::MeasurementHooks h{cem::MeasurementTrigger::CONTINUOUS};
    for (int i = 0; i < 10; ++i) EXPECT_TRUE(h.should_record());
}

TEST(MeasurementHooks, SampledOnlyEveryNth) {
    cem::MeasurementHooks h{cem::MeasurementTrigger::SAMPLED_1_N, 5};
    int hits = 0;
    for (int i = 0; i < 25; ++i) {
        if (h.should_record()) ++hits;
    }
    EXPECT_EQ(hits, 5);     // jeder 5. von 25 → 5 Hits
}

// ─────────────────────────────────────────────────────────────────────────────
// DumpHeader / DumpFooter
// ─────────────────────────────────────────────────────────────────────────────

TEST(DumpHeader, MagicStringIsCorrect) {
    cem::DumpHeader h{};
    EXPECT_STREQ(h.magic, "COMDARE-MEASUREMENT-V1");
    EXPECT_EQ(h.version, 1u);
    EXPECT_EQ(h.record_size, 32u);
}

TEST(DumpFooter, EndMarkerIsCorrect) {
    cem::DumpFooter f{};
    EXPECT_STREQ(f.end_marker, "END-COMDARE");
}
