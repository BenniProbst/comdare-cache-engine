#pragma once
// parallel_measure_pool.hpp -- #45 (paralleler Mess-Loop, §16.2-M1/§61-MODI, 2026-07-21): der ordered-collect
// Worker-Pool des Debug-Modus-Mess-Loops.
//
// PROBLEM: der Mess-Vollzug (run_lazy_static_then_dynamic-Binary-Schleife) lief bislang STRIKT 1-Thread. Der §61-MODI
// Debug-Modus ("DASS es funktioniert", keine Mess-golden-Zahlen) darf UEBER die Mess-Zellen parallelisieren -- der
// Measure-/Release-Modus bleibt sequentiell (Mess-Methodik, byte-/verhaltens-identisch).
//
// LOESUNG: collect_ordered() fuehrt process(ctx, index) fuer index in [0,n) aus und liefert die Outcomes in
// DETERMINISTISCHER INDEX-Reihenfolge (results[index]) -- unabhaengig von der Ausfuehrungsreihenfolge. Damit bleibt die
// Debug-CSV STRUKTURELL identisch zum sequentiellen Lauf (nur die MESSWERTE sind im Debug ohne Garantie, §61-MODI).
//   - pool_size <= 1  => SEQUENTIELL (EIN ctx, in Reihenfolge) -> verhaltens-identisch zum Ist (Measure/Release/Default).
//   - pool_size >  1  => pool_size Worker, JE Worker EIN eigener ctx (make_ctx -- z.B. ein eigener IPmcSource, da ein
//                        realer PMC-Source NICHT thread-safe teilbar ist); die Indizes via atomic-Counter gezogen.
// observed_max_concurrency (optional) meldet die beobachtete Spitze gleichzeitig laufender process()-Aufrufe
// (Test-Wache: >1 nur im Parallel-Fall, ==1 sequentiell). process/make_ctx MUESSEN thread-safe sein (kein geteilter
// mutabler Zustand ausser results[index], das je index eindeutig ist -> KEIN Merge-Lock). Header-only, C++23, nur stdlib.

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <thread>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

/// Fuehrt process(ctx, i) fuer i in [0,n) aus und gibt die Outcomes positions-treu (results[i]) zurueck. Outcome muss
/// default-konstruierbar sein (Slot-Vorbelegung). Siehe Datei-Kopf fuer die Modus-/Thread-Semantik.
template <class Outcome, class CtxFactory, class Process>
[[nodiscard]] std::vector<Outcome> collect_ordered(std::size_t n, std::size_t pool_size, CtxFactory make_ctx,
                                                   Process process, std::size_t* observed_max_concurrency = nullptr) {
    std::vector<Outcome> results(n);
    if (n == 0) {
        if (observed_max_concurrency) *observed_max_concurrency = 0;
        return results;
    }
    if (pool_size <= 1) {
        auto ctx = make_ctx(); // EIN ctx fuer den ganzen sequentiellen Lauf (z.B. der eine geteilte pmc)
        for (std::size_t i = 0; i < n; ++i) results[i] = process(ctx, i);
        if (observed_max_concurrency) *observed_max_concurrency = 1;
        return results;
    }
    std::size_t const        workers = std::min(pool_size, n);
    std::atomic<std::size_t> next{0};
    std::atomic<std::size_t> active{0};
    std::atomic<std::size_t> max_active{0};
    auto                     worker = [&] {
        auto ctx = make_ctx(); // JE Worker EIN eigener ctx (per-Worker-Ressource, z.B. eigener IPmcSource)
        for (;;) {
            std::size_t const i = next.fetch_add(1);
            if (i >= n) return;
            std::size_t const cur = active.fetch_add(1) + 1; // gleichzeitig laufende process()-Aufrufe
            for (std::size_t prev = max_active.load(); cur > prev && !max_active.compare_exchange_weak(prev, cur);) {}
            results[i] = process(ctx, i); // results[i] eindeutig je i -> kein Lock
            active.fetch_sub(1);
        }
    };
    std::vector<std::thread> pool;
    pool.reserve(workers);
    for (std::size_t w = 0; w < workers; ++w) pool.emplace_back(worker);
    for (auto& t : pool) t.join();
    if (observed_max_concurrency) *observed_max_concurrency = max_active.load();
    return results;
}

} // namespace comdare::cache_engine::builder::experiment
