// test_pressure_state.cpp - REV 5.2 State-Pattern Variant-Round-Trip + Overload-Pattern

#include "cache_engine/concepts/pressure_state.hpp"
#include "cache_engine/concepts/cache_recommendation.hpp"

#include <gtest/gtest.h>

#include <variant>

namespace ce = comdare::cache_engine;

// Overload-Pattern (Modern C++23 std::visit Idiom)
template <typename... Fs>
struct Overload : Fs... { using Fs::operator()...; };
template <typename... Fs>
Overload(Fs...) -> Overload<Fs...>;

TEST(PressureState, DefaultIsIdle) {
    ce::state::PressureState s = ce::state::Idle{};
    EXPECT_EQ(s.index(), 0u);  // Idle ist erste Alternative
}

TEST(PressureState, VariantHoldsAllFiveStates) {
    ce::state::PressureState s;

    s = ce::state::Idle{ .last_check_ns = 100 };
    EXPECT_TRUE(std::holds_alternative<ce::state::Idle>(s));

    s = ce::state::Warmup{ .mpki = 5.0, .pages_loaded = 42 };
    EXPECT_TRUE(std::holds_alternative<ce::state::Warmup>(s));

    s = ce::state::Saturated{ .bandwidth_util = 0.85, .mshr_pressure = 8 };
    EXPECT_TRUE(std::holds_alternative<ce::state::Saturated>(s));

    s = ce::state::CoherenceStorm{ .inter_socket_msgs_per_us = 500, .affected_sockets = 2 };
    EXPECT_TRUE(std::holds_alternative<ce::state::CoherenceStorm>(s));

    s = ce::state::Recovery{ .remaining = std::chrono::milliseconds{50} };
    EXPECT_TRUE(std::holds_alternative<ce::state::Recovery>(s));
}

TEST(PressureState, OverloadVisitProducesRecommendation) {
    using namespace ce::state;
    using ce::CacheRecommendation;

    auto react = [](const PressureState& s) -> CacheRecommendation::Verdict {
        return std::visit(Overload{
            [](const Idle&)           { return CacheRecommendation::Verdict::DoNothing; },
            [](const Warmup&)         { return CacheRecommendation::Verdict::Hint; },
            [](const Saturated&)      { return CacheRecommendation::Verdict::Hint; },
            [](const CoherenceStorm&) { return CacheRecommendation::Verdict::Migrate; },
            [](const Recovery&)       { return CacheRecommendation::Verdict::DoNothing; }
        }, s);
    };

    EXPECT_EQ(react(Idle{}),                CacheRecommendation::Verdict::DoNothing);
    EXPECT_EQ(react(Warmup{ 1.0, 10 }),     CacheRecommendation::Verdict::Hint);
    EXPECT_EQ(react(Saturated{ 0.9, 16 }),  CacheRecommendation::Verdict::Hint);
    EXPECT_EQ(react(CoherenceStorm{ 1000, 4 }), CacheRecommendation::Verdict::Migrate);
}
