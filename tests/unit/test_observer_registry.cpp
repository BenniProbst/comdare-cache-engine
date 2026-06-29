// Test fuer ObserverRegistry F2 Push synchron (Termin 7 / 02_uml_cache_engine §5)

#include <cache_engine/concepts/event.hpp>
#include <cache_engine/concepts/i_observer.hpp>

#include <gtest/gtest.h>

namespace ce = comdare::cache_engine;

namespace {
struct CountingObserver : ce::IObserver {
    int  total_events       = 0;
    int  filtered_writes    = 0;
    bool accept_writes_only = false;

    void on_event(ce::Event const& e) noexcept override {
        ++total_events;
        if (e.kind == ce::EventKind::Write) ++filtered_writes;
    }
    [[nodiscard]] bool accepts(ce::EventKind kind) const noexcept override {
        if (accept_writes_only) return kind == ce::EventKind::Write;
        return true;
    }
};
} // namespace

TEST(ObserverRegistry, EmptyRegistryDispatchesNoEvent) {
    ce::ObserverRegistry    reg;
    ce::PageRelocationEvent e{};
    e.module_id = 1;
    e.kind      = ce::EventKind::PageRelocation;
    EXPECT_NO_THROW(reg.dispatch(e));
    EXPECT_EQ(reg.observer_count(1), 0u);
}

TEST(ObserverRegistry, RegistersAndDispatchesPerModule) {
    ce::ObserverRegistry reg;
    CountingObserver     obs;
    reg.register_observer(7, &obs);

    ce::Event e{};
    e.module_id = 7;
    e.kind      = ce::EventKind::Write;
    reg.dispatch(e);
    reg.dispatch(e);
    reg.dispatch(e);

    EXPECT_EQ(obs.total_events, 3);
    EXPECT_EQ(obs.filtered_writes, 3);
    EXPECT_EQ(reg.observer_count(7), 1u);
}

TEST(ObserverRegistry, EventToWrongModuleIsNotDelivered) {
    ce::ObserverRegistry reg;
    CountingObserver     obs;
    reg.register_observer(1, &obs);

    ce::Event e{};
    e.module_id = 2; // anderes Modul
    e.kind      = ce::EventKind::Write;
    reg.dispatch(e);

    EXPECT_EQ(obs.total_events, 0);
}

TEST(ObserverRegistry, RespectsAcceptsFilter) {
    ce::ObserverRegistry reg;
    CountingObserver     obs;
    obs.accept_writes_only = true;
    reg.register_observer(1, &obs);

    ce::Event w{};
    w.module_id = 1;
    w.kind      = ce::EventKind::Write;
    ce::Event b{};
    b.module_id = 1;
    b.kind      = ce::EventKind::ConsolidationBarrier;

    reg.dispatch(w);
    reg.dispatch(b);
    reg.dispatch(w);

    EXPECT_EQ(obs.total_events, 2);
    EXPECT_EQ(obs.filtered_writes, 2);
}

TEST(ObserverRegistry, MultipleObserversPerModule) {
    ce::ObserverRegistry reg;
    CountingObserver     a, b;
    reg.register_observer(1, &a);
    reg.register_observer(1, &b);

    ce::Event e{};
    e.module_id = 1;
    e.kind      = ce::EventKind::Sampling;
    reg.dispatch(e);

    EXPECT_EQ(a.total_events, 1);
    EXPECT_EQ(b.total_events, 1);
    EXPECT_EQ(reg.observer_count(1), 2u);
}

TEST(ObserverRegistry, UnregisterModuleRemovesAllObservers) {
    ce::ObserverRegistry reg;
    CountingObserver     a, b;
    reg.register_observer(1, &a);
    reg.register_observer(1, &b);
    reg.unregister_module(1);

    ce::Event e{};
    e.module_id = 1;
    e.kind      = ce::EventKind::Sampling;
    reg.dispatch(e);

    EXPECT_EQ(a.total_events, 0);
    EXPECT_EQ(b.total_events, 0);
    EXPECT_EQ(reg.observer_count(1), 0u);
}

TEST(ObserverRegistry, ClearRemovesEverything) {
    ce::ObserverRegistry reg;
    CountingObserver     a, b;
    reg.register_observer(1, &a);
    reg.register_observer(2, &b);
    reg.clear();
    EXPECT_EQ(reg.observer_count(1), 0u);
    EXPECT_EQ(reg.observer_count(2), 0u);
}
