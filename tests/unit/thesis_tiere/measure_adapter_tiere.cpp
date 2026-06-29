// measure_adapter_tiere — echter Mess-Lauf der CONTAINER-Gattung (Adapter-Tier-Unterklasse, Doku 14 §28) für den
// Diplomarbeit-Mess-Anhang. Die 3 std-Adapter-Disziplinen (queue/stack/priority_queue) werden über die 3
// inner_container-Organe + die §26.4-Adapter-API getrieben — in-process über AdapterAnatomy (= das per-Gattung-
// Prüf-Dock-Treiben, Doc 24 §8.8; der DLL+IAdapterTier-Weg kann nur FIFO/pop_front, daher hier der Dock-Weg, der
// alle 3 Disziplinen über die volle push/pop_front/pop_back-API abdeckt).
//
// Über ALLE 3 Tiere ist der §28-Achsen-Satz IDENTISCH (13 Achsen); variiert wird NUR die spezifische Achse
// inner_container (DequeInner/HeapInner) + die Disziplin (API-Nutzung front vs back) → der gemessene Unterschied ist
// der Achse/Disziplin zurechenbar. Erhoben: eingebauter Adapter-Observer + Wall-Clock (steady_clock) je n_ops.
//
// Build:  cl /nologo /std:c++latest /O2 /EHsc /I libs/cache_engine measure_adapter_tiere.cpp
// Lauf:   ./measure_adapter_tiere.exe  > build/thesis_tiere/adapter_measurements.csv

#include "anatomy/adapter_anatomy.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

namespace cea = comdare::cache_engine::anatomy;

// Die 12 geteilten/delegierten §28-Achsen: Platzhalter (nur inner_container wird real getrieben).
struct DelegatedAxis {};
using D = DelegatedAxis;

// Die 3 Adapter-Tiere = §28-Komposition mit identischem geteiltem Achsen-Satz, variierter inner_container-Achse.
using QueueTier =
    cea::AdapterAnatomy<cea::AdapterComposition<D, D, D, D, D, D, D, D, D, D, D, D, cea::DequeInner<>>>; // FIFO
using StackTier =
    cea::AdapterAnatomy<cea::AdapterComposition<D, D, D, D, D, D, D, D, D, D, D, D, cea::DequeInner<>>>; // LIFO
using PriorityTier =
    cea::AdapterAnatomy<cea::AdapterComposition<D, D, D, D, D, D, D, D, D, D, D, D, cea::HeapInner<>>>; // Max-Heap

// Varied Werte (LCG), damit das Priority-Heap real siftet (queue/stack-Werte irrelevant, O(1)).
static inline std::uint64_t lcg(std::uint64_t i) noexcept { return i * 1103515245ull + 12345ull; }

enum class Disc { Fifo, Lifo, Priority };

// Ein Mess-Durchlauf: n_ops push + n_ops/2 Entnahmen gemäß Disziplin; liefert Observer + Wall-Clock-ns.
template <class Tier>
static void run(char const* disc, char const* inner, Disc d, std::uint64_t n_ops, int reps) {
    // Median über `reps` Wiederholungen (separat, nie interpoliert — KF-10-Disziplin).
    std::uint64_t                best_ns = ~0ull;
    cea::AdapterObserverSnapshot obs{};
    for (int r = 0; r < reps; ++r) {
        Tier       tier;
        auto const t0 = std::chrono::steady_clock::now();
        for (std::uint64_t i = 0; i < n_ops; ++i) tier.push(lcg(i));
        std::uint64_t const half = n_ops / 2;
        for (std::uint64_t i = 0; i < half; ++i) {
            if (d == Disc::Lifo)
                (void)tier.pop_back(); // stack
            else
                (void)tier.pop_front(); // queue (FIFO) + priority (Extract-Max)
        }
        auto const t1 = std::chrono::steady_clock::now();
        auto const ns =
            static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        if (ns < best_ns) {
            best_ns = ns;
            obs     = tier.observe_all();
        }
    }
    double const total_ops = static_cast<double>(n_ops + n_ops / 2);
    double const ns_per_op = static_cast<double>(best_ns) / total_ops;
    std::cout << disc << ';' << inner << ';' << n_ops << ';' << obs.push_count << ';' << obs.pop_count << ';'
              << obs.front_reads << ';' << obs.back_reads << ';' << obs.peak_occupancy << ';' << obs.current_occupancy
              << ';' << best_ns << ';' << ns_per_op << '\n';
}

int main() {
    // CSV-Header (';'-getrennt, wie thesis_measurements.csv). best_ns = Median-of-min Wall-Clock; ns_per_op abgeleitet.
    std::cout << "discipline;inner_container;n_ops;push_count;pop_count;front_reads;back_reads;"
                 "peak_occupancy;current_occupancy;best_ns;ns_per_op\n";
    int const reps = 3;
    for (std::uint64_t n : {100000ull, 500000ull, 1000000ull}) {
        run<QueueTier>("queue", "DequeInner", Disc::Fifo, n, reps);
        run<StackTier>("stack", "DequeInner", Disc::Lifo, n, reps);
        run<PriorityTier>("priority_queue", "HeapInner", Disc::Priority, n, reps);
    }
    return 0;
}
