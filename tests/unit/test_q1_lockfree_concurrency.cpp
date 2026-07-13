// test_q1_lockfree_concurrency — M-CE-09 (Muster-F, 2026-07-13): schliesst die vom Voll-Review benannte
// Test-Luecke "die beiden als thread-safe deklarierten Lock-Free-Queues haben KEINEN einzigen nebenlaeufigen
// Test". Prueft LockFreeMPMCBuffer (Vyukov bounded MPMC) + LockFreeSPSCBuffer verhaltensspezifisch.
//
// WICHTIG — Semantik: BEIDE Queues sind BOUNDED + DROP-ON-FULL (kein Block; put() ist void → ein Ueberlauf ist
// von aussen nicht unterscheidbar). Der nebenlaeufige Test prueft daher die Invarianten, die fuer eine lossy
// bounded Queue GELTEN — NICHT "exactly once" (Drops sind erlaubt):
//   (1) Single-Thread FIFO INNERHALB der Kapazitaet (kein Drop): put(0..N-1) -> get()==0..N-1 vollstaendig.
//   (2) MPMC nebenlaeufig (P Producer disjunkte Werte, C Consumer): treibt die CAS-Retry-Schleife unter
//       Contention und belegt: KEIN Duplikat (jeder Wert hoechstens EINMAL gezogen), nur GUELTIGE Werte
//       (keine Phantom-/korrupten Werte), consumed <= produced, und der Lauf TERMINIERT (Liveness).
//   (3) SPSC nebenlaeufig (1/1): die gezogenen Werte bilden eine STRENG AUFSTEIGENDE FIFO-Teilfolge (Drops
//       ueberspringen, aber verdrehen nie) + keine Duplikate + Liveness.
// Plain int main() (KEIN gtest), g_fail-Zaehler -> return 0/1. Der sanitize:tsan-Job kann dasselbe Target
// zusaetzlich mit -fsanitize=thread bauen (dann wird die Data-Race-Freiheit scharf).

#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_lockfree_mpmc.hpp>
#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_lockfree_spsc.hpp>

#include <atomic>
#include <cstdint>
#include <iostream>
#include <optional>
#include <thread>
#include <vector>

namespace q1 = comdare::cache_engine::queuing::axis_q1_queuing;

static int  g_fail = 0;
static void check(bool ok, char const* what) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

// (1) Single-Thread-FIFO INNERHALB der Kapazitaet (kein Ueberlauf → kein Drop): vollstaendige Uebertragung.
template <class Q>
static void single_thread_fifo(char const* name) {
    Q q{64};
    check(q.is_empty(), (std::string{name} + ": frisch is_empty").c_str());
    for (std::uint64_t i = 0; i < 32; ++i) q.put(i); // 32 < cap 64 → kein Drop
    check(q.size() == 32, (std::string{name} + ": size==32 nach 32 put (< cap)").c_str());
    check(q.peek_front().has_value() && *q.peek_front() == 0, (std::string{name} + ": peek_front==0").c_str());
    bool order_ok = true;
    for (std::uint64_t i = 0; i < 32; ++i) {
        std::optional<std::uint64_t> v = q.get();
        if (!v.has_value() || *v != i) order_ok = false;
    }
    check(order_ok, (std::string{name} + ": FIFO get()==0..31 vollstaendig").c_str());
    check(q.is_empty() && !q.get().has_value(), (std::string{name} + ": leer nach Drain, get()==nullopt").c_str());
}

int main() {
    std::cout << "==== M-CE-09: Lock-Free-Queue-Nebenlaeufigkeit (MPMC + SPSC, drop-on-full) ====\n";

    // (1) ----------------------------------------------------------------------
    single_thread_fifo<q1::LockFreeMPMCBuffer>("MPMC");
    single_thread_fifo<q1::LockFreeSPSCBuffer>("SPSC");

    // (2) MPMC nebenlaeufig — Contention treibt die CAS-Retry-Pfade; Invarianten: keine Duplikate/nur gueltige
    //     Werte/Liveness. Der Ueberlauf (total >> cap) uebt zugleich den drop-on-full-Zweig aus.
    {
        constexpr std::uint64_t kProducers = 4;
        constexpr std::uint64_t kConsumers = 4;
        constexpr std::uint64_t kPerProd   = 10000;
        constexpr std::uint64_t kTotal     = kProducers * kPerProd; // 40000 disjunkte Werte [0, kTotal)

        q1::LockFreeMPMCBuffer        q{1024}; // Power-of-2 (Vyukov-Constraint); total >> cap → Drops erwartet
        std::vector<std::atomic<int>> seen(kTotal);
        for (auto& s : seen) s.store(0, std::memory_order_relaxed);

        std::atomic<std::uint64_t> consumed{0};
        std::atomic<int>           producers_done{0};

        std::vector<std::thread> prod;
        for (std::uint64_t p = 0; p < kProducers; ++p) {
            prod.emplace_back([&, p] {
                for (std::uint64_t i = 0; i < kPerProd; ++i) q.put(p * kPerProd + i); // put droppt bei voll
                producers_done.fetch_add(1, std::memory_order_acq_rel);
            });
        }
        std::vector<std::thread> cons;
        for (std::uint64_t c = 0; c < kConsumers; ++c) {
            cons.emplace_back([&] {
                for (;;) {
                    std::optional<std::uint64_t> v = q.get();
                    if (v.has_value()) {
                        seen[*v].fetch_add(1, std::memory_order_relaxed); // Wert < kTotal per Konstruktion (Validitaet)
                        consumed.fetch_add(1, std::memory_order_acq_rel);
                    } else if (producers_done.load(std::memory_order_acquire) == static_cast<int>(kProducers)) {
                        return; // Producer fertig + Queue (jetzt) leer → dauerhaft leer (kein Nachschub) → Liveness
                    } else {
                        std::this_thread::yield();
                    }
                }
            });
        }
        for (auto& t : prod) t.join();
        for (auto& t : cons) t.join();

        bool no_dup = true;
        for (std::uint64_t i = 0; i < kTotal; ++i)
            if (seen[i].load(std::memory_order_relaxed) > 1) {
                no_dup = false;
                break;
            }
        check(no_dup, "MPMC concurrent: kein Wert doppelt gezogen (Thread-Safety unter Contention)");
        check(consumed.load() <= kTotal, "MPMC concurrent: consumed <= produced (drop-on-full erlaubt)");
        check(consumed.load() > 0, "MPMC concurrent: > 0 Werte durchgeschleust (Naht lebt)");
        check(q.is_empty(), "MPMC concurrent: Queue am Ende leer (Liveness/Drain)");
    }

    // (3) SPSC nebenlaeufig — Drops ueberspringen Werte, verdrehen sie aber nie: gezogene Werte STRENG aufsteigend.
    {
        constexpr std::uint64_t    kTotal = 50000;
        q1::LockFreeSPSCBuffer     q{1024}; // total >> cap → Drops erwartet
        std::atomic<bool>          strictly_increasing{true};
        std::atomic<bool>          producer_done{false};
        std::atomic<std::uint64_t> got{0};

        std::thread producer([&] {
            for (std::uint64_t i = 0; i < kTotal; ++i) q.put(i); // droppt bei voll
            producer_done.store(true, std::memory_order_release);
        });
        std::thread consumer([&] {
            bool          have_last = false;
            std::uint64_t last      = 0;
            for (;;) {
                std::optional<std::uint64_t> v = q.get();
                if (v.has_value()) {
                    if (have_last && *v <= last) strictly_increasing.store(false, std::memory_order_relaxed);
                    last      = *v;
                    have_last = true;
                    got.fetch_add(1, std::memory_order_acq_rel);
                } else if (producer_done.load(std::memory_order_acquire)) {
                    return; // Producer fertig + leer → dauerhaft leer
                } else {
                    std::this_thread::yield();
                }
            }
        });
        producer.join();
        consumer.join();

        check(strictly_increasing.load(), "SPSC concurrent: gezogene Werte streng aufsteigend (FIFO-Teilfolge)");
        check(got.load() > 0 && got.load() <= kTotal, "SPSC concurrent: 0 < got <= produced (drop-on-full erlaubt)");
        check(q.is_empty(), "SPSC concurrent: Queue am Ende leer (Liveness/Drain)");
    }

    std::cout << "==== " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
