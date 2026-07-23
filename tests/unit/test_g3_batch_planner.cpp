// test_g3_batch_planner -- G3 / #46b Lagerhaltung, Scheibe B5.
//
// Deckt ab: Paritaets-Gleichverteilung, Einzeln-Miss-Erkennung, B13-Typ-Sequenz-Wache, die Slice-
// Queue (Producer-Consumer, Start-vor-Fertig NACHGEWIESEN) und den vollen async Planer-Lauf.

#include "bestandslog/batch_planner.hpp"
#include "bestandslog/slice_queue.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

namespace bl = comdare::cache_engine::builder::bestandslog;

// ---------------------------------------------------------------------------
// Paritaets-Gleichverteilung.
// ---------------------------------------------------------------------------
TEST(G3BatchPlanner, WindowParity) {
    EXPECT_TRUE(bl::window_belongs_to(0, 0, 2));
    EXPECT_FALSE(bl::window_belongs_to(1, 0, 2));
    EXPECT_TRUE(bl::window_belongs_to(1, 1, 2));
    EXPECT_TRUE(bl::window_belongs_to(2, 0, 2));
    EXPECT_TRUE(bl::window_belongs_to(5, 0, 1)); // eine Maschine kriegt alles
    EXPECT_TRUE(bl::window_belongs_to(9, 3, 0)); // n_machines==0 -> alle
}

// ---------------------------------------------------------------------------
// Einzeln-Miss-Erkennung.
// ---------------------------------------------------------------------------
TEST(G3BatchPlanner, DetectMissingIndividually) {
    // Gerade Indizes sind vorhanden -> fehlende = ungerade.
    bl::PresencePredicate even_present = [](std::uint64_t i) { return (i % 2) == 0; };
    auto                  missing      = bl::detect_missing_in_window(0, 10, even_present);
    EXPECT_EQ(missing, (std::vector<std::uint64_t>{1, 3, 5, 7, 9}));
}

TEST(G3BatchPlanner, PresencePredicateFromKeySet) {
    std::unordered_set<std::string> present{"key1", "key3"};
    auto is_present = bl::make_presence_predicate(present, [](std::uint64_t i) { return "key" + std::to_string(i); });
    EXPECT_TRUE(is_present(1));
    EXPECT_FALSE(is_present(2));
    EXPECT_TRUE(is_present(3));
}

// ---------------------------------------------------------------------------
// B13 Batch-Typ-Sequenz-Wache.
// ---------------------------------------------------------------------------
TEST(G3BatchPlanner, TypeSequenceGuard) {
    using T = bl::BatchTyp;
    std::vector<T> good{T::planer_block, T::ceb, T::tier, T::tier};
    EXPECT_TRUE(bl::is_valid_type_sequence(good));
    std::vector<T> also_good{T::tier, T::tier}; // gleiche Phase wiederholt ist ok
    EXPECT_TRUE(bl::is_valid_type_sequence(also_good));
    std::vector<T> empty{};
    EXPECT_TRUE(bl::is_valid_type_sequence(empty));
    std::vector<T> regress{T::tier, T::ceb}; // Rueckfall tier->ceb -> Verletzung
    EXPECT_FALSE(bl::is_valid_type_sequence(regress));
    std::vector<T> regress2{T::ceb, T::planer_block};
    EXPECT_FALSE(bl::is_valid_type_sequence(regress2));
}

// ---------------------------------------------------------------------------
// Slice-Queue: Start-vor-Fertig NACHGEWIESEN -- der Consumer bekommt die erste Slice, BEVOR der
// Producer die zweite ueberhaupt eingereiht hat (Producer wartet auf das Consumer-Signal).
// ---------------------------------------------------------------------------
TEST(G3BatchPlanner, QueueConsumesBeforeProducerFinishes) {
    bl::SliceQueue    q;
    std::atomic<bool> consumed_first{false};

    std::thread producer([&] {
        q.push(bl::BatchSlice{0, 4, bl::BatchTyp::tier, {0, 1, 2, 3}});
        // Warten bis der Consumer die erste Slice hat -> beweist Start-vor-Fertig.
        while (!consumed_first.load()) std::this_thread::yield();
        q.push(bl::BatchSlice{4, 4, bl::BatchTyp::tier, {4, 5, 6, 7}});
        q.close();
    });

    auto s0 = q.pop(); // muss SOFORT kommen, bevor Slice 1 existiert
    ASSERT_TRUE(s0.has_value());
    EXPECT_EQ(s0->begin, 0u);
    consumed_first.store(true); // erst jetzt darf der Producer weitermachen

    auto s1 = q.pop();
    ASSERT_TRUE(s1.has_value());
    EXPECT_EQ(s1->begin, 4u);

    auto s2 = q.pop(); // geschlossen + gedraint
    EXPECT_FALSE(s2.has_value());

    producer.join();
}

TEST(G3BatchPlanner, QueueClosedEmptyReturnsNullopt) {
    bl::SliceQueue q;
    q.close();
    EXPECT_FALSE(q.pop().has_value());
    EXPECT_TRUE(q.closed());
}

// ---------------------------------------------------------------------------
// Voller async Planer-Lauf: rank 0 von 2, Korn 4, total 16 -> Fenster 0,2 gehoeren mir; alle fehlen
// -> zwei Slices (begin 0 und 8, je 4 fehlende). Consumer draint bis nullopt.
// ---------------------------------------------------------------------------
TEST(G3BatchPlanner, PlannerProducesMyParityWindowsWithMissing) {
    std::vector<bl::BatchSlice> collected;
    {
        bl::SliceQueue   q;
        bl::BatchPlanner planner(
            q, /*total*/ 16, bl::BatchTyp::tier, /*rank*/ 0, /*n_machines*/ 2,
            /*is_present*/ [](std::uint64_t) { return false; }, /*grain*/ 4);
        while (auto s = q.pop()) collected.push_back(*s);
        // planner-dtor joined
    }
    ASSERT_EQ(collected.size(), 2u);
    EXPECT_EQ(collected[0].begin, 0u);
    EXPECT_EQ(collected[0].count, 4u);
    EXPECT_EQ(collected[0].missing, (std::vector<std::uint64_t>{0, 1, 2, 3}));
    EXPECT_EQ(collected[1].begin, 8u);
    EXPECT_EQ(collected[1].missing, (std::vector<std::uint64_t>{8, 9, 10, 11}));
}

TEST(G3BatchPlanner, PlannerSkipsFullyPresentWindows) {
    // Alle Binaries in Fenster 0 sind vorhanden, in Fenster 2 fehlen sie -> nur EINE Slice (begin 8).
    std::vector<bl::BatchSlice> collected;
    {
        bl::SliceQueue   q;
        bl::BatchPlanner planner(q, 16, bl::BatchTyp::tier, 0, 2, [](std::uint64_t i) { return i < 4; }, 4);
        while (auto s = q.pop()) collected.push_back(*s);
    }
    ASSERT_EQ(collected.size(), 1u);
    EXPECT_EQ(collected[0].begin, 8u);
}

TEST(G3BatchPlanner, PlannerRankOneGetsOtherWindows) {
    // rank 1 von 2 -> Fenster 1,3 (begin 4 und 12).
    std::vector<bl::BatchSlice> collected;
    {
        bl::SliceQueue   q;
        bl::BatchPlanner planner(q, 16, bl::BatchTyp::tier, 1, 2, [](std::uint64_t) { return false; }, 4);
        while (auto s = q.pop()) collected.push_back(*s);
    }
    ASSERT_EQ(collected.size(), 2u);
    EXPECT_EQ(collected[0].begin, 4u);
    EXPECT_EQ(collected[1].begin, 12u);
}
