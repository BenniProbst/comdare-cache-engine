// V41.F.6.1 Topic queuing Tests (2026-05-26)
//
// @topic queuing
// 2 Achsen: axis_Q1 buffer_strategy (4 Pilot) + axis_Q2 flush_policy (3 Pilot)
//
// TYPED_TEST_SUITE pro Achse separat (mp_apply<ToGTestTypes, ...>).
// Schablone analog allocator-Achse — strukturelle Bewaehrung der Vorlage.

#include <topics/queuing/concepts/topic_queuing_concept.hpp>
#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_registry.hpp>
#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_registry.hpp>
#include <topics/queuing/topic_queuing_config_set.hpp>

#include <boost/mp11.hpp>
#include <gtest/gtest.h>

#include <array>
#include <cstdint>

namespace q1      = comdare::cache_engine::queuing::axis_q1_queuing;
namespace q1_cpts = comdare::cache_engine::queuing::axis_q1_queuing::concepts;
namespace q2      = comdare::cache_engine::queuing::axis_q2_queuing;
namespace q2_cpts = comdare::cache_engine::queuing::axis_q2_queuing::concepts;
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
    // - FIFO/LIFO/BoundedRingBuffer: enthaelt jetzt Werte
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

// PutGetRoundtrip: alle Werte rein, dann alle wieder raus. Anzahl gleich.
// (NoBuffer: 0 raus, andere: N raus). Verifiziert vollen Lifecycle pro Vendor.
TYPED_TEST(Q1BufferStrategyTest, PutGetRoundtripAllValues) {
    TypeParam b{};
    for (auto v : kTestPutValues) b.put(v);
    std::size_t before = b.size();
    std::size_t pulled = 0;
    while (auto v = b.get()) {
        ++pulled;
        if (pulled > kTestPutValues.size() + 10) break;  // safety
    }
    EXPECT_EQ(pulled, before) << "Vendor " << TypeParam::name()
                              << ": #put = #get muss konsistent sein";
    EXPECT_TRUE(b.is_empty());
}

// std::queue-API: peek_front/peek_back analog std::queue::front/back
TYPED_TEST(Q1BufferStrategyTest, PeekDoesNotConsume) {
    TypeParam b{};
    if constexpr (std::is_same_v<TypeParam, q1::NoBuffer>) {
        EXPECT_FALSE(b.peek_front().has_value());
        EXPECT_FALSE(b.peek_back().has_value());
        return;
    }
    b.put(42); b.put(99);
    auto f1 = b.peek_front();
    auto f2 = b.peek_front();  // 2x peek = gleicher Wert (kein consume)
    ASSERT_TRUE(f1.has_value());
    ASSERT_TRUE(f2.has_value());
    EXPECT_EQ(*f1, *f2);
    EXPECT_EQ(b.size(), 2u) << "peek darf size nicht aendern";
    auto bk = b.peek_back();
    ASSERT_TRUE(bk.has_value());
    EXPECT_EQ(b.size(), 2u);
}

TYPED_TEST(Q1BufferStrategyTest, EmplaceEquivalentToPut) {
    TypeParam b{};
    b.emplace(77);
    if constexpr (std::is_same_v<TypeParam, q1::NoBuffer>) {
        EXPECT_TRUE(b.is_empty());  // no-op
    } else {
        EXPECT_EQ(b.size(), 1u);
        EXPECT_EQ(*b.get(), 77u);
    }
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
    q1::FIFOQueueBuffer f{};
    f.put(1); f.put(2); f.put(3);
    EXPECT_EQ(f.size(), 3u);
    EXPECT_EQ(*f.get(), 1u);
    EXPECT_EQ(*f.get(), 2u);
    EXPECT_EQ(*f.get(), 3u);
    EXPECT_TRUE(f.is_empty());
}

TEST(Q1BufferStrategy_LIFO, LifoOrder) {
    q1::LIFOStackBuffer s{};
    s.put(1); s.put(2); s.put(3);
    EXPECT_EQ(s.size(), 3u);
    EXPECT_EQ(*s.get(), 3u);
    EXPECT_EQ(*s.get(), 2u);
    EXPECT_EQ(*s.get(), 1u);
    EXPECT_TRUE(s.is_empty());
}

TEST(Q1BufferStrategy_BoundedRing, OverflowDropsOldest) {
    q1::BoundedRingBuffer r{4};  // capacity=4
    r.put(1); r.put(2); r.put(3); r.put(4);
    EXPECT_EQ(r.size(), 4u);
    r.put(5);  // overflow: drops 1
    EXPECT_EQ(r.size(), 4u);
    EXPECT_EQ(*r.get(), 2u);  // 1 wurde gedroppt
    EXPECT_EQ(*r.get(), 3u);
    EXPECT_EQ(*r.get(), 4u);
    EXPECT_EQ(*r.get(), 5u);
}

// [[zero-size-allocation-exception]] User-Direktive 2026-05-26:
// BoundedRingBuffer mit capacity=0 wirft std::invalid_argument (UB-Vermeidung)
TEST(Q1BufferStrategy_BoundedRing, ZeroCapacityThrows) {
    EXPECT_THROW(q1::BoundedRingBuffer{0}, std::invalid_argument);
}

TEST(Q1BufferStrategy_BoundedRing, IterableAspectValuesCount) {
    constexpr auto vals = q1::BoundedRingBuffer::iterable_values();
    EXPECT_EQ(vals.size(), 5u);  // {8, 64, 1024, 16384, 65536}
    EXPECT_EQ(vals[0], 8u);
    EXPECT_EQ(vals[4], 65536u);
}

// Runtime-Permutationen Buffer-Kapazitaeten (User-Direktive 2026-05-26
// [[neue-achse-strict-vorlage-allocator]]):
// Edge-Cases mit Extremfaelle + Power-of-2-Alignment-Werte + NICHT-Power-of-2.
struct CapacityConfig { std::size_t cap; char const* label; };
constexpr std::array<CapacityConfig, 9> kTestBufferCapacities{{
    {     1, "extrem-1"   },    // kleinste sinnvolle (kein FIFO/LIFO-Sinn aber valid)
    {     2, "extrem-2"   },
    {     3, "non-pow2-3" },    // klein, NICHT-Power-of-2
    {     7, "non-pow2-7" },    // klein, NICHT-Power-of-2 (Cache-Line-Edge)
    {     8, "pow2-8"     },    // Power-of-2 (Cache-Line-aligned)
    {    15, "non-pow2-15"},
    {    64, "pow2-64"    },    // Cache-Line (typisch)
    {   100, "non-pow2-100"},   // round-decimal
    { 16384, "pow2-16k"   },    // Page-aligned
}};

TEST(Q1BufferStrategy_BoundedRing, PutGetWithMultipleBufferSizesEdgeCases) {
    for (auto const& cfg : kTestBufferCapacities) {
        q1::BoundedRingBuffer r{cfg.cap};
        // fill bis Overflow
        for (std::size_t i = 0; i < cfg.cap + 3; ++i) {
            r.put(static_cast<std::uint64_t>(i + 1));
        }
        EXPECT_EQ(r.size(), cfg.cap)
            << "Capacity=" << cfg.cap << " (" << cfg.label
            << ") nach Overflow muss size==cap sein";
        // alle wieder raus
        std::size_t pulled = 0;
        while (r.get()) ++pulled;
        EXPECT_EQ(pulled, cfg.cap)
            << "Capacity=" << cfg.cap << " (" << cfg.label << ") roundtrip";
        EXPECT_TRUE(r.is_empty());
    }
}

TEST(Q1BufferStrategy_BoundedRing, PeekWithMultipleBufferSizes) {
    for (auto const& cfg : kTestBufferCapacities) {
        q1::BoundedRingBuffer r{cfg.cap};
        if (cfg.cap < 2) continue;  // peek-back-Test braucht >=2 elements
        r.put(11); r.put(22);
        ASSERT_TRUE(r.peek_front().has_value());
        ASSERT_TRUE(r.peek_back().has_value());
        EXPECT_EQ(*r.peek_front(), 11u) << "cap=" << cfg.cap;
        EXPECT_EQ(*r.peek_back(),  22u) << "cap=" << cfg.cap;
        EXPECT_EQ(r.size(), 2u) << "peek darf size nicht aendern (cap=" << cfg.cap << ")";
    }
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

// Q2 should_flush ist deterministisch: gleicher Input -> gleicher Output
TYPED_TEST(Q2FlushPolicyTest, ShouldFlushDeterministisch) {
    TypeParam p{};
    auto d1 = p.should_flush(50, 100);
    auto d2 = p.should_flush(50, 100);
    EXPECT_EQ(d1, d2) << "Policy " << TypeParam::name()
                      << " should_flush muss deterministisch sein bei gleichem Input";
}

TYPED_TEST(Q2FlushPolicyTest, OnFlushCompleteNoCrash) {
    TypeParam p{};
    p.on_flush_complete();
    p.on_flush_complete();  // 2x sollte safe sein
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

// Runtime-Permutationen Watermark-Werte (User-Direktive 2026-05-26
// [[neue-achse-strict-vorlage-allocator]]):
// Iteriere ueber alle iterable_values() + verifiziere Schwellen-Verhalten an
// Edge-Cases (0, threshold-1, threshold, threshold+1, cap).
struct FillEdge { std::size_t fill; bool expect_flush_at_75; };
constexpr std::array<FillEdge, 7> kTestFillLevelsAt100Cap{{
    {  0, false},   // leer
    {  1, false},   // 1% << 75%
    { 74, false},   // direkt unter Threshold
    { 75, true },   // genau Threshold
    { 76, true },   // direkt darueber
    { 99, true },   // fast voll
    {100, true },   // voll
}};

TEST(Q2FlushPolicy_Watermark, ShouldFlushAtAllIterableThresholds) {
    constexpr auto thresholds = q2::WatermarkFlush::iterable_values();
    for (auto thr : thresholds) {
        q2::WatermarkFlush p{thr};
        // Fill genau auf threshold% -> FullFlush erwartet
        EXPECT_EQ(p.should_flush(thr, 100), q2_cpts::FlushDecision::FullFlush)
            << "WatermarkPct=" << thr << " fill=" << thr << "/100";
        // Fill unter threshold% -> NoFlush
        if (thr > 0) {
            EXPECT_EQ(p.should_flush(thr - 1, 100), q2_cpts::FlushDecision::NoFlush)
                << "WatermarkPct=" << thr << " fill=" << (thr-1) << "/100";
        }
    }
}

TEST(Q2FlushPolicy_Watermark, FillLevelEdgeCasesAt75Default) {
    q2::WatermarkFlush p{75};
    for (auto const& edge : kTestFillLevelsAt100Cap) {
        auto dec = p.should_flush(edge.fill, 100);
        EXPECT_EQ(dec == q2_cpts::FlushDecision::FullFlush, edge.expect_flush_at_75)
            << "fill=" << edge.fill << "/100, threshold=75%";
    }
}

// Watermark mit nicht-Standard-Kapazitaeten (Alignment-Edge-Cases)
TEST(Q2FlushPolicy_Watermark, ShouldFlushWithNonStandardCapacities) {
    q2::WatermarkFlush p{50};
    // cap=7 (NICHT power-of-2): bei fill=3 ergibt 3*100/7 = 42% < 50% -> NoFlush
    EXPECT_EQ(p.should_flush(3, 7), q2_cpts::FlushDecision::NoFlush);
    // cap=7, fill=4: 4*100/7 = 57% >= 50% -> FullFlush
    EXPECT_EQ(p.should_flush(4, 7), q2_cpts::FlushDecision::FullFlush);
    // cap=0 (edge): NoFlush garantiert (division-by-zero guard)
    EXPECT_EQ(p.should_flush(10, 0), q2_cpts::FlushDecision::NoFlush);
    // cap=1, fill=1 (extrem): 100% -> FullFlush
    EXPECT_EQ(p.should_flush(1, 1), q2_cpts::FlushDecision::FullFlush);
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

// PermutationEngine-Integration analog Allocator-Topic (test_v41_topic_allocator_axis_06.cpp)
TEST(TopicQueuing_PermutationEngine, ForEachQ1xQ2) {
    int count = 0;
    boost::mp11::mp_for_each<qing::TopicConfigSet::CartesianQ1xQ2>([&count]<class P>(P){
        ++count;
        using Q1 = boost::mp11::mp_at_c<P, 0>;
        using Q2 = boost::mp11::mp_at_c<P, 1>;
        // Compile-time Pflicht: jede Permutation hat 1 BufferStrategy + 1 FlushPolicy
        static_assert(q1_cpts::BufferStrategy<Q1>);
        static_assert(q2_cpts::FlushPolicy<Q2>);
    });
    EXPECT_EQ(static_cast<std::size_t>(count),
              boost::mp11::mp_size<qing::TopicConfigSet::CartesianQ1xQ2>::value);
}

// Statistics-TYPED_TEST: pro Vendor MUSS statistics()/snapshot()/observer() vorhanden sein (wenn STATISTICS=ON)
#ifdef COMDARE_CE_ENABLE_STATISTICS
TYPED_TEST(Q1BufferStrategyTest, ObserverAliasIsMeasurableObserver) {
    using S = typename TypeParam::snapshot_t;
    using O = typename TypeParam::observer_t;
    static_assert(std::is_same_v<O, ::comdare::cache_engine::measurement::MeasurableObserver<S>>);
    SUCCEED();
}

TYPED_TEST(Q1BufferStrategyTest, ObserverNotifyOnPut) {
    TypeParam b{};
    int events = 0;
    b.observer().on_event([&events](auto const&){ ++events; });
    b.put(42);
    EXPECT_GE(events, 1) << "put muss notify ausloesen (Vendor " << TypeParam::name() << ")";
}

TYPED_TEST(Q2FlushPolicyTest, ObserverAliasIsMeasurableObserver) {
    using S = typename TypeParam::snapshot_t;
    using O = typename TypeParam::observer_t;
    static_assert(std::is_same_v<O, ::comdare::cache_engine::measurement::MeasurableObserver<S>>);
    SUCCEED();
}

TYPED_TEST(Q2FlushPolicyTest, ObserverNotifyOnShouldFlush) {
    TypeParam p{};
    int events = 0;
    p.observer().on_event([&events](auto const&){ ++events; });
    (void)p.should_flush(50, 100);
    EXPECT_GE(events, 1) << "should_flush muss notify ausloesen (Policy " << TypeParam::name() << ")";
}
#endif
