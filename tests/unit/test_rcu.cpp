// SPDX-License-Identifier: Apache-2.0
// Tests fuer comdare::rcu (Aufgabe #104)

#include "rcu.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

namespace rcu = comdare::rcu;

TEST(Rcu, EpochAdvancesOnSynchronize) {
    rcu::RcuDomain d;
    auto const     before = d.current_epoch();
    d.synchronize();
    EXPECT_GT(d.current_epoch(), before);
}

TEST(Rcu, ReadGuardRegistersThread) {
    rcu::RcuDomain d;
    auto const     before = d.reader_count();
    {
        rcu::RcuReadGuard g{d};
        EXPECT_GE(d.reader_count(), before + 1);
    }
    // Thread bleibt registriert (thread_local), aber active=false nach Guard-Destruktor.
}

TEST(Rcu, MultipleReadersDoNotBlock) {
    rcu::RcuDomain   d;
    std::atomic<int> entered{0};

    auto reader_fn = [&]() {
        rcu::RcuReadGuard g{d};
        entered.fetch_add(1, std::memory_order_release);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) threads.emplace_back(reader_fn);
    for (auto& t : threads) t.join();
    EXPECT_EQ(entered.load(), 4);
}

TEST(Rcu, DeferredCallbacksRunAfterFlush) {
    rcu::RcuDomain   d;
    rcu::RcuDeferred def;

    std::atomic<int> calls{0};
    def.defer([&]() { calls.fetch_add(1); });
    def.defer([&]() { calls.fetch_add(10); });
    EXPECT_EQ(def.pending_count(), 2u);
    EXPECT_EQ(calls.load(), 0);

    def.flush(d);
    EXPECT_EQ(calls.load(), 11);
    EXPECT_EQ(def.pending_count(), 0u);
}

TEST(Rcu, DeferredFlushIsIdempotentOnEmpty) {
    rcu::RcuDomain   d;
    rcu::RcuDeferred def;
    def.flush(d); // no-op
    EXPECT_EQ(def.pending_count(), 0u);
}

TEST(Rcu, RcuReplaceDefersOldPointer) {
    rcu::RcuDomain    d;
    rcu::RcuDeferred  def;
    std::atomic<int*> slot{new int(42)};

    rcu::rcu_replace(slot, new int(99), def);
    EXPECT_EQ(*slot.load(), 99);
    EXPECT_EQ(def.pending_count(), 1u);

    def.flush(d);
    EXPECT_EQ(def.pending_count(), 0u);

    delete slot.load();
}

TEST(Rcu, SynchronizeWaitsForActiveReader) {
    rcu::RcuDomain    d;
    std::atomic<bool> reader_started{false};
    std::atomic<bool> reader_finished{false};

    std::thread reader([&]() {
        rcu::RcuReadGuard g{d};
        reader_started.store(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        reader_finished.store(true);
    });

    while (!reader_started.load()) std::this_thread::yield();

    auto const t_start = std::chrono::steady_clock::now();
    d.synchronize();
    auto const elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_start).count();

    // synchronize() should wait until reader leaves; reader sleeps 50ms.
    EXPECT_TRUE(reader_finished.load());
    EXPECT_GE(elapsed, 30); // mit ein bisschen Schlaftoleranz

    reader.join();
}

TEST(Rcu, NestedReadGuardsWorkInSameThread) {
    rcu::RcuDomain d;
    {
        rcu::RcuReadGuard g1{d};
        {
            rcu::RcuReadGuard g2{d};
            // Beide Guards koennen unproblematisch koexistieren
        }
    }
    SUCCEED();
}

// M-CE-13: Nesting-Zaehler. Ein innerer Guard-Destruktor darf den Reader NICHT fuer den ganzen
// Thread als inaktiv markieren, solange ein aeusserer Guard noch im kritischen Abschnitt ist.
// (Ohne Zaehler wuerde ~RcuReadGuard von g2 active=false setzen → Writer koennte die Grace-Period
// vorzeitig beenden → UAF.) Deterministisch ueber den public current_thread_state()-Zugriff.
TEST(Rcu, NestedReadGuardsKeepReaderActiveUntilOutermostRelease) {
    rcu::RcuDomain d;
    auto*          s = d.current_thread_state();
    ASSERT_FALSE(s->active.load());
    EXPECT_EQ(s->nesting, 0u);
    {
        rcu::RcuReadGuard g1{d};
        EXPECT_TRUE(s->active.load());
        EXPECT_EQ(s->nesting, 1u);
        {
            rcu::RcuReadGuard g2{d};
            EXPECT_TRUE(s->active.load());
            EXPECT_EQ(s->nesting, 2u);
        }
        // Innerer Guard zerstoert: Reader MUSS weiterhin aktiv sein.
        EXPECT_TRUE(s->active.load());
        EXPECT_EQ(s->nesting, 1u);
    }
    // Aeusserster Guard zerstoert: jetzt inaktiv.
    EXPECT_FALSE(s->active.load());
    EXPECT_EQ(s->nesting, 0u);
}

// M-CE-14: Store-Load-Race. Stress mit gleichzeitig aktiven Readern, die den geschuetzten Pointer
// dereferenzieren, waehrend der Writer ueber viele Grace-Periods hinweg alte Versionen freigibt.
// Der Fix (seq_cst-Fences in read_lock/synchronize) schliesst das SB-Muster → kein UAF. Timing-
// abhaengig nicht beweisbar, aber unter TSan (out-of-tree) race-frei; hier: kein Crash, korrekte
// Werte, Reader bleiben die gesamte Writer-Phase am Leben (kein dangling-Deregistrierungs-Pfad).
TEST(Rcu, GracePeriodProtectsConcurrentReadersNoUseAfterFree) {
    rcu::RcuDomain    d;
    rcu::RcuDeferred  def;
    std::atomic<int*> slot{new int(0)};
    std::atomic<bool> stop{false};
    std::atomic<long> reads{0};

    auto reader = [&]() {
        while (!stop.load(std::memory_order_relaxed)) {
            rcu::RcuReadGuard g{d};
            int const*        p = slot.load(std::memory_order_acquire);
            volatile int      v = *p; // Deref waehrend der read-side critical section MUSS gueltig bleiben
            (void)v;
            reads.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> readers;
    for (int i = 0; i < 4; ++i) readers.emplace_back(reader);

    for (int gen = 1; gen <= 2000; ++gen) {
        rcu::rcu_replace(slot, new int(gen), def); // publiziert neue Version, defer alte
        def.flush(d);                              // synchronize (Grace-Period) + delete alte NACH GP
    }

    stop.store(true, std::memory_order_relaxed);
    for (auto& t : readers) t.join();

    EXPECT_GT(reads.load(), 0);
    delete slot.load();
    SUCCEED();
}
