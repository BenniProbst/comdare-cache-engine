// V41.F.6.1 Topic queuing Tests (2026-05-26)
//
// @topic queuing
// 2 Achsen: axis_Q1 buffer_strategy (4 Pilot) + axis_Q2 flush_policy (3 Pilot)
//
// TYPED_TEST_SUITE pro Achse separat (mp_apply<ToGTestTypes, ...>).
// Schablone analog allocator-Achse — strukturelle Bewaehrung der Vorlage.

#include <topics/queuing/concepts/topic_queuing_concept.hpp>
#include <topics/queuing/axis_q1_buffer_strategy/axis_q1_buffer_strategy_registry.hpp>
#include <topics/queuing/axis_q2_flush_policy/axis_q2_flush_policy_registry.hpp>
#include <topics/queuing/topic_queuing_config_set.hpp>

#include <boost/mp11.hpp>
#include <gtest/gtest.h>

#include <array>
#include <cstdint>

namespace q1      = comdare::cache_engine::queuing::axis_q1_buffer_strategy;
namespace q1_cpts = comdare::cache_engine::queuing::axis_q1_buffer_strategy::concepts;
namespace q2      = comdare::cache_engine::queuing::axis_q2_flush_policy;
namespace q2_cpts = comdare::cache_engine::queuing::axis_q2_flush_policy::concepts;
namespace qing    = comdare::cache_engine::queuing;

// ─────────────────────────────────────────────────────────────────────────────
// TYPED_TEST_SUITE fuer axis_Q1 buffer_strategy (4 Pilot-Strategien)
// ─────────────────────────────────────────────────────────────────────────────

template <class... Vs>
using ToGTestTypes = ::testing::Types<Vs...>;

using AllQ1StrategyTypes = boost::mp11::mp_apply<ToGTestTypes, q1::AllStrategies>;

template <class T> class Q1BufferStrategyTest : public ::testing::Test {};
TYPED_TEST_SUITE(Q1BufferStrategyTest, AllQ1StrategyTypes);

TYPED_TEST(Q1BufferStrategyTest, ConceptConformance) {
    static_assert(q1_cpts::BufferStrategy<TypeParam>);
    static_assert(q1_cpts::CacheEngineBufferPermutationStrategy<TypeParam>);
    SUCCEED();
}

TYPED_TEST(Q1BufferStrategyTest, Identification) {
    static_assert(!TypeParam::name().empty());
    static_assert(!TypeParam::family_name().empty());
    static_assert(!TypeParam::flag_suffix().empty());
    static_assert(TypeParam::family_id::value > 0);
    SUCCEED();
}

TYPED_TEST(Q1BufferStrategyTest, EmptyAfterDefaultConstruct) {
    TypeParam b{};
    EXPECT_TRUE(b.is_empty()) << "Vendor " << TypeParam::name() << " sollte leer sein nach Konstruktion";
    EXPECT_EQ(b.size(), 0u);
}

// Runtime-Permutationen via constexpr Config-Liste ([[runtime-permutations-via-config-array]])
constexpr std::array<std::uint64_t, 5> kTestPutValues{1u, 42u, 100u, 1024u, 1'000'000u};

TYPED_TEST(Q1BufferStrategyTest, PutDoesNotCrash) {
    TypeParam b{};
    for (auto v : kTestPutValues) {
        b.put(v);
    }
    // Spezial-Cases:
    // - NoBuffer: bleibt leer (passthrough)
    // - FIFO/LIFO/BoundedRing: enthaelt jetzt Werte
    SUCCEED();
}

TYPED_TEST(Q1BufferStrategyTest, ClearMakesEmpty) {
    TypeParam b{};
    for (auto v : kTestPutValues) {
        b.put(v);
    }
    b.clear();
    EXPECT_TRUE(b.is_empty());
    EXPECT_EQ(b.size(), 0u);
}

// Sonderfall-Properties Compile-Pflicht
TYPED_TEST(Q1BufferStrategyTest, SonderfallPropertiesQueryable) {
    using PG = q1_cpts::ProgressGuarantee;
    [[maybe_unused]] constexpr bool a = TypeParam::supports_concurrent_producers();
    [[maybe_unused]] constexpr bool b = TypeParam::supports_concurrent_consumers();
    [[maybe_unused]] constexpr bool c = TypeParam::supports_priority_ordering();
    [[maybe_unused]] constexpr bool d = TypeParam::is_versioned();
    [[maybe_unused]] constexpr PG   e = TypeParam::progress_guarantee();
    // Konsistenz-Check: NoBuffer ist WaitFree (trivially)
    if constexpr (std::is_same_v<TypeParam, q1::NoBuffer>) {
        static_assert(e == PG::WaitFree, "NoBuffer ist trivially wait-free (no-op)");
    }
    SUCCEED();
}

// FIFO-spezifischer Funktionstest (nicht TYPED — Verhaltens-Pruefung)
TEST(Q1BufferStrategy_FIFO, FifoOrder) {
    q1::FIFOQueue f{};
    f.put(1); f.put(2); f.put(3);
    EXPECT_EQ(f.size(), 3u);
    EXPECT_EQ(*f.get(), 1u);
    EXPECT_EQ(*f.get(), 2u);
    EXPECT_EQ(*f.get(), 3u);
    EXPECT_TRUE(f.is_empty());
}

TEST(Q1BufferStrategy_LIFO, LifoOrder) {
    q1::LIFOStack s{};
    s.put(1); s.put(2); s.put(3);
    EXPECT_EQ(s.size(), 3u);
    EXPECT_EQ(*s.get(), 3u);
    EXPECT_EQ(*s.get(), 2u);
    EXPECT_EQ(*s.get(), 1u);
    EXPECT_TRUE(s.is_empty());
}

TEST(Q1BufferStrategy_BoundedRing, OverflowDropsOldest) {
    q1::BoundedRing r{4};  // capacity=4
    r.put(1); r.put(2); r.put(3); r.put(4);
    EXPECT_EQ(r.size(), 4u);
    r.put(5);  // overflow: drops 1
    EXPECT_EQ(r.size(), 4u);
    EXPECT_EQ(*r.get(), 2u);  // 1 wurde gedroppt
    EXPECT_EQ(*r.get(), 3u);
    EXPECT_EQ(*r.get(), 4u);
    EXPECT_EQ(*r.get(), 5u);
}

TEST(Q1BufferStrategy_BoundedRing, IterableAspectValuesCount) {
    constexpr auto vals = q1::BoundedRing::iterable_values();
    EXPECT_EQ(vals.size(), 5u);  // {8, 64, 1024, 16384, 65536}
    EXPECT_EQ(vals[0], 8u);
    EXPECT_EQ(vals[4], 65536u);
}

// ─────────────────────────────────────────────────────────────────────────────
// TYPED_TEST_SUITE fuer axis_Q2 flush_policy (3 Pilot-Policies)
// ─────────────────────────────────────────────────────────────────────────────

using AllQ2PolicyTypes = boost::mp11::mp_apply<ToGTestTypes, q2::AllPolicies>;

template <class T> class Q2FlushPolicyTest : public ::testing::Test {};
TYPED_TEST_SUITE(Q2FlushPolicyTest, AllQ2PolicyTypes);

TYPED_TEST(Q2FlushPolicyTest, ConceptConformance) {
    static_assert(q2_cpts::FlushPolicy<TypeParam>);
    static_assert(q2_cpts::CacheEngineFlushPolicyPermutationStrategy<TypeParam>);
    SUCCEED();
}

TYPED_TEST(Q2FlushPolicyTest, Identification) {
    static_assert(!TypeParam::name().empty());
    static_assert(!TypeParam::family_name().empty());
    static_assert(!TypeParam::flag_suffix().empty());
    static_assert(TypeParam::family_id::value > 0);
    SUCCEED();
}

TYPED_TEST(Q2FlushPolicyTest, SonderfallPropertiesQueryable) {
    [[maybe_unused]] constexpr bool a = TypeParam::is_time_based();
    [[maybe_unused]] constexpr bool b = TypeParam::is_threshold_based();
    [[maybe_unused]] constexpr bool c = TypeParam::is_event_driven();
    [[maybe_unused]] constexpr bool d = TypeParam::is_adaptive();
    // Konsistenz: Watermark ist threshold-based
    if constexpr (std::is_same_v<TypeParam, q2::WatermarkFlush>) {
        static_assert(b, "WatermarkFlush sollte threshold_based=true sein");
        static_assert(!a && !c && !d, "WatermarkFlush ist ausschliesslich threshold-based");
    }
    if constexpr (std::is_same_v<TypeParam, q2::EagerFlush>) {
        static_assert(c, "EagerFlush sollte event_driven=true sein");
    }
    if constexpr (std::is_same_v<TypeParam, q2::LazyFlush>) {
        static_assert(c, "LazyFlush sollte event_driven=true sein");
    }
    SUCCEED();
}

// Verhaltens-Tests (nicht TYPED — pro Policy spezifisch)
TEST(Q2FlushPolicy_Eager, AlwaysFullFlush) {
    q2::EagerFlush p{};
    EXPECT_EQ(p.should_flush(0, 100), q2_cpts::FlushDecision::FullFlush);
    EXPECT_EQ(p.should_flush(50, 100), q2_cpts::FlushDecision::FullFlush);
    EXPECT_EQ(p.should_flush(100, 100), q2_cpts::FlushDecision::FullFlush);
}

TEST(Q2FlushPolicy_Lazy, NeverFlush) {
    q2::LazyFlush p{};
    EXPECT_EQ(p.should_flush(0, 100), q2_cpts::FlushDecision::NoFlush);
    EXPECT_EQ(p.should_flush(99, 100), q2_cpts::FlushDecision::NoFlush);
    EXPECT_EQ(p.should_flush(100, 100), q2_cpts::FlushDecision::NoFlush);
}

TEST(Q2FlushPolicy_Watermark, FlushAt75Percent) {
    q2::WatermarkFlush p{75};
    EXPECT_EQ(p.should_flush(0, 100), q2_cpts::FlushDecision::NoFlush);
    EXPECT_EQ(p.should_flush(74, 100), q2_cpts::FlushDecision::NoFlush);
    EXPECT_EQ(p.should_flush(75, 100), q2_cpts::FlushDecision::FullFlush);
    EXPECT_EQ(p.should_flush(100, 100), q2_cpts::FlushDecision::FullFlush);
}

TEST(Q2FlushPolicy_Watermark, IterableAspectValuesCount) {
    constexpr auto vals = q2::WatermarkFlush::iterable_values();
    EXPECT_EQ(vals.size(), 5u);  // {50, 65, 75, 85, 95}
    EXPECT_EQ(vals[2], 75u);
}

// ─────────────────────────────────────────────────────────────────────────────
// TopicConfigSet Cartesian Q1 x Q2 (Pilot: 4 x 3 = 12)
// ─────────────────────────────────────────────────────────────────────────────

TEST(TopicQueuing_ConfigSet, CartesianProductSize) {
    using TCS = qing::TopicConfigSet;
    constexpr auto q1_cnt = boost::mp11::mp_size<TCS::StaticAxisVariants_Q1>::value;
    constexpr auto q2_cnt = boost::mp11::mp_size<TCS::StaticAxisVariants_Q2>::value;
    constexpr auto cartesian_cnt = boost::mp11::mp_size<TCS::CartesianQ1xQ2>::value;
    EXPECT_GE(q1_cnt, 1u);
    EXPECT_GE(q2_cnt, 1u);
    EXPECT_EQ(cartesian_cnt, q1_cnt * q2_cnt);
}
