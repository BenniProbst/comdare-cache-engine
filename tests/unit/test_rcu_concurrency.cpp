// SPDX-License-Identifier: Apache-2.0
// Concurrency-/Eigentumsmodell-Tests fuer comdare::rcu (Feature #25, honest-100 %)
//
// Deckt die beiden zuvor UNGETESTETEN Enden des Reader-Lifetimes ab, die der Ownership-Inversion-
// Fix (rcu.hpp) strukturell schliesst:
//   1. Thread-Ende: Reader-Slots muessen die Reader-Threads ueberleben, ohne dangling zu werden
//      (frueher: rohe Zeiger auf ein thread_local-Objekt in readers_ → Use-after-free in
//      synchronize()/reader_count() nach Thread-Join).
//   2. Stack-Domain-Zerstoerung + Adress-Wiederverwendung: eine neue Stack-RcuDomain an der Adresse
//      einer soeben zerstoerten muss einen FRISCHEN Slot bekommen (frueher: geteilter thread_local
//      "registered"-Flag → zweite Stack-Domain blieb faelschlich leer / Stale-Deref).
//
// Die Test-LOGIK ist deterministisch (feste Slot-Zahlen, feste reader_count()); zusaetzlich sind die
// Faelle als TSan-Ziel out-of-tree gedacht (g++-16 -fsanitize=thread). Der bestehende
// GracePeriodProtects…-Test in test_rcu.cpp bleibt unveraendert.

#include "rcu.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>

namespace rcu = comdare::rcu;

// §2.1 — Thread-Ende hinterlaesst keinen dangling Reader.
// N Reader-Threads registrieren sich (RcuReadGuard) und JOINEN. DANACH iteriert synchronize() /
// reader_count() die Domain-owned Slots. Vor dem Fix waren das N tote thread_local-Zeiger → UAF hier.
// Deterministisch: jeder Thread erzeugt genau EINEN Guard → genau EIN Slot; der Main-Thread
// registriert sich auf d NICHT → reader_count() == N und ueber viele synchronize()-Runden stabil.
TEST(RcuConcurrency, ThreadEndLeavesNoDanglingReader) {
    rcu::RcuDomain    d;
    constexpr int     kThreads = 8;
    std::atomic<long> reads{0};

    {
        std::vector<std::thread> workers;
        workers.reserve(kThreads);
        for (int i = 0; i < kThreads; ++i) {
            workers.emplace_back([&d, &reads]() {
                rcu::RcuReadGuard g{d};
                reads.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            });
        }
        for (auto& t : workers) { t.join(); }
    }

    // Alle Reader-Threads sind beendet; ihre Slots leben Domain-owned (inert, active=false) weiter.
    EXPECT_EQ(reads.load(), static_cast<long>(kThreads));
    EXPECT_EQ(d.reader_count(), static_cast<std::size_t>(kThreads));

    // Mehrfaches synchronize()/reader_count() ueber tote-Thread-Slots MUSS sicher sein (kein UAF).
    for (int round = 0; round < 128; ++round) {
        d.synchronize();
        EXPECT_EQ(d.reader_count(), static_cast<std::size_t>(kThreads));
    }
    SUCCEED();
}

// §2.2/§2.3 — Stack-Domain-Zerstoerung + Adress-Wiederverwendung liefert einen frischen Slot.
// In einer Schleife je Iteration eine Stack-RcuDomain bauen, EINEN Reader registrieren, Domain
// zerstoeren. Der Stack-Slot der Variablen 'd' wird typ. wiederverwendet (Adressgleichheit) — der
// monotone serial_ + generation-gekeyter Cache erzwingt dennoch Cache-Miss → frischer Slot.
// Vor dem Fix waere reader_count() ab i>=1 faelschlich 0 (geteilter registered-Flag) bzw. Stale-Deref.
TEST(RcuConcurrency, StackDomainAddressReuseGivesFreshSlot) {
    std::uintptr_t first_addr = 0;

    for (int i = 0; i < 64; ++i) {
        rcu::RcuDomain d;
        auto const     addr = reinterpret_cast<std::uintptr_t>(&d);
        if (i == 0) { first_addr = addr; }

        // Frische Domain → noch kein Reader registriert.
        EXPECT_EQ(d.reader_count(), 0u);
        {
            rcu::RcuReadGuard g{d};
            EXPECT_EQ(d.reader_count(), 1u); // genau EIN frischer Slot fuer diese (serial'd) Domain
            auto* s = d.current_thread_state();
            EXPECT_TRUE(s->active.load());
        }
        // synchronize() darf KEINEN Stale-Slot einer frueheren (zerstoerten) Domain anfassen.
        d.synchronize();
        EXPECT_EQ(d.reader_count(), 1u);
    }

    // Adress-Wiederverwendung ist wahrscheinlich, aber nicht standardgarantiert → nur informativ.
    (void)first_addr;
    SUCCEED();
}

// TSan-Ziel: Reader-Threads, die WAEHREND laufender synchronize()-Runden ENTSTEHEN UND ENDEN
// (Thread-Churn), gleichzeitig zum Writer, der ueber viele Grace-Periods alte Versionen freigibt.
// Uebt den "Slot ueberlebt Thread"-Pfad NEBENLAEUFIG zu synchronize() aus. Determiniert im Ergebnis
// (kein Crash, reads>0, Post-Join-synchronize() sicher), race-frei unter -fsanitize=thread.
TEST(RcuConcurrency, WriterSyncsWhileReaderThreadsChurnNoUseAfterFree) {
    rcu::RcuDomain    d;
    rcu::RcuDeferred  def;
    std::atomic<int*> slot{new int(0)};
    std::atomic<bool> stop{false};
    std::atomic<long> reads{0};

    auto churn = [&]() {
        for (int k = 0; k < 300 && !stop.load(std::memory_order_relaxed); ++k) {
            rcu::RcuReadGuard g{d};
            int const*        p = slot.load(std::memory_order_acquire);
            volatile int      v = *p; // Deref waehrend der read-side critical section MUSS gueltig bleiben
            (void)v;
            reads.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> readers;
    readers.reserve(4);
    for (int i = 0; i < 4; ++i) { readers.emplace_back(churn); }

    for (int gen = 1; gen <= 500; ++gen) {
        rcu::rcu_replace(slot, new int(gen), def); // publiziert neue Version, defer alte
        def.flush(d);                              // synchronize (Grace-Period) + delete alte NACH GP
    }

    stop.store(true, std::memory_order_relaxed);
    for (auto& t : readers) { t.join(); }

    // Threads beendet → ihre Slots bleiben Domain-owned; Post-Join-synchronize() MUSS sicher sein.
    d.synchronize();
    EXPECT_GT(reads.load(), 0);
    delete slot.load();
    SUCCEED();
}
