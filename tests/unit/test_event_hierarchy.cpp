// Test fuer Event-Hierarchie (Termin 7 / 02_uml_cache_engine §5)

#include <cache_engine/concepts/event.hpp>

#include <gtest/gtest.h>
#include <type_traits>

namespace ce = comdare::cache_engine;

TEST(Event, BaseEventConstructible) {
    ce::Event e{};
    EXPECT_EQ(e.module_id, 0u);
    EXPECT_EQ(e.kind, ce::EventKind::Error);
}

TEST(Event, AllNineConcreteEventsInheritFromEvent) {
    EXPECT_TRUE((std::is_base_of_v<ce::Event, ce::PageRelocationEvent>));
    EXPECT_TRUE((std::is_base_of_v<ce::Event, ce::PageTypeChangeEvent>));
    EXPECT_TRUE((std::is_base_of_v<ce::Event, ce::PrefetchAdjustmentEvent>));
    EXPECT_TRUE((std::is_base_of_v<ce::Event, ce::HotPathRecognitionEvent>));
    EXPECT_TRUE((std::is_base_of_v<ce::Event, ce::TelemetryUpdateEvent>));
    EXPECT_TRUE((std::is_base_of_v<ce::Event, ce::WriteEvent>));
    EXPECT_TRUE((std::is_base_of_v<ce::Event, ce::ConsolidationBarrierEvent>));
    EXPECT_TRUE((std::is_base_of_v<ce::Event, ce::SamplingEvent>));
    EXPECT_TRUE((std::is_base_of_v<ce::Event, ce::ErrorEvent>));
}

TEST(Event, KindEnumKnowsNineValues) {
    // Ueber Cast pruefen wir, dass alle 9 EventKind-Werte definiert sind
    EXPECT_EQ(static_cast<int>(ce::EventKind::PageRelocation), 0);
    EXPECT_EQ(static_cast<int>(ce::EventKind::PageTypeChange), 1);
    EXPECT_EQ(static_cast<int>(ce::EventKind::PrefetchAdjustment), 2);
    EXPECT_EQ(static_cast<int>(ce::EventKind::HotPathRecognition), 3);
    EXPECT_EQ(static_cast<int>(ce::EventKind::TelemetryUpdate), 4);
    EXPECT_EQ(static_cast<int>(ce::EventKind::Write), 5);
    EXPECT_EQ(static_cast<int>(ce::EventKind::ConsolidationBarrier), 6);
    EXPECT_EQ(static_cast<int>(ce::EventKind::Sampling), 7);
    EXPECT_EQ(static_cast<int>(ce::EventKind::Error), 8);
}

TEST(Event, WriteEventCarriesBlockANCacheCoherenceCostInputs) {
    // Block AN Kuehn: WriteEvent muss node_depth + num_cores_sharing + cache_line_size haben
    ce::WriteEvent w{};
    w.node_depth            = 5;
    w.num_cores_sharing     = 16;
    w.cache_line_size_bytes = 64;
    EXPECT_EQ(w.node_depth, 5);
    EXPECT_EQ(w.num_cores_sharing, 16);
    EXPECT_EQ(w.cache_line_size_bytes, 64);
}

TEST(Event, ConsolidationBarrierCarriesEpoch) {
    ce::ConsolidationBarrierEvent b{};
    b.epoch = 42;
    EXPECT_EQ(b.epoch, 42u);
}

TEST(Event, SamplingEventTracksAdjustmentInputs) {
    ce::SamplingEvent s{};
    s.current_sampling_n = 1000;
    s.cpu_load           = 0.75;
    s.cache_miss_rate    = 0.05;
    EXPECT_EQ(s.current_sampling_n, 1000u);
    EXPECT_DOUBLE_EQ(s.cpu_load, 0.75);
}

TEST(Event, TelemetryUpdateEventCarriesStrategyTag) {
    ce::TelemetryUpdateEvent t{};
    t.telemetry_strategy = 2; // LEAFONLY_SAMPLED
    t.counter_delta      = 1;
    EXPECT_EQ(t.telemetry_strategy, 2);
}
