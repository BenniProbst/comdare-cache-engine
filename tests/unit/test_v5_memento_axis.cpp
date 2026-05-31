// V5-I5 — MementoAxis-Concept + save_axis/restore_axis Round-Trip (Fundament für memento_all).
#include "anatomy/memento_aggregate.hpp"

#include <gtest/gtest.h>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace an = comdare::cache_engine::anatomy;

// Stateful Achse mit trivialem Memento (uint64-Zustand).
struct StatefulScalar {
    using memento_t = std::uint64_t;
    std::uint64_t state = 0;
    [[nodiscard]] memento_t save_state() const { return state; }
    void restore_state(memento_t const& m) { state = m; }
};

// Stateful Achse mit RICHEM (heap-haltendem) Memento — „einfacher Snapshot reicht nicht".
struct StatefulRich {
    using memento_t = std::vector<std::uint64_t>;
    std::vector<std::uint64_t> data;
    [[nodiscard]] memento_t save_state() const { return data; }
    void restore_state(memento_t const& m) { data = m; }
};

// Stateless Achse (kein save_state/restore_state).
struct StatelessConfig { int cfg = 7; };

TEST(V5MementoAxis, ConceptDiscriminatesStatefulVsStateless) {
    static_assert(an::MementoAxis<StatefulScalar>);
    static_assert(an::MementoAxis<StatefulRich>);
    static_assert(!an::MementoAxis<StatelessConfig>);
    static_assert(std::is_same_v<an::memento_of_t<StatefulScalar>, std::uint64_t>);
    static_assert(std::is_same_v<an::memento_of_t<StatefulRich>, std::vector<std::uint64_t>>);
    static_assert(std::is_same_v<an::memento_of_t<StatelessConfig>, an::EmptyMemento>);
    SUCCEED();
}

TEST(V5MementoAxis, ScalarRoundTrip) {
    StatefulScalar a; a.state = 42;
    auto const saved = an::save_axis(a);   // Warmup-Vor-Zustand
    a.state = 999;                         // Warmup-Op mutiert
    an::restore_axis(a, saved);            // rollback-all
    EXPECT_EQ(a.state, 42u);
}

TEST(V5MementoAxis, RichHeapRoundTrip) {
    StatefulRich a; a.data = {1, 2, 3};
    auto const saved = an::save_axis(a);
    a.data.assign({9, 9, 9, 9});           // Warmup-Op mutiert (andere Größe + Werte)
    an::restore_axis(a, saved);
    EXPECT_EQ(a.data, (std::vector<std::uint64_t>{1, 2, 3}));
}

TEST(V5MementoAxis, StatelessIsNoOp) {
    StatelessConfig s;
    auto const e = an::save_axis(s);       // EmptyMemento
    an::restore_axis(s, e);                // no-op
    EXPECT_EQ(s.cfg, 7);
    static_assert(std::is_trivially_copyable_v<an::EmptyMemento>);
    SUCCEED();
}
