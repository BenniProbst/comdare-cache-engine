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
