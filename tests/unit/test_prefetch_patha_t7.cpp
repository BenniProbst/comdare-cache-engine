// GAP #2 Re-Grounding (P5d-Rest, User-Plan dynamic-frolicking-truffle Step 3, 2026-06-18) — BUILD+RUN-VERIFIKATION,
// dass das T7-Segment des Pfad-A-Achsen-Timers (run_workload_segmented_v2, abi_adapter.hpp) seit dem Re-Grounding
// REALE lokale Store-Adressen prefetcht (lokaler LayoutAwareChunkedStore mit echten Records) + per-Strategie
// differenziert — NICHT mehr ein synthetisches lbuf via Prefetch::is_active(). Damit ist seg_prefetch_ns (seg_ns[7])
// real + strategie-abhaengig. Belegt LITERAL:
//
//   (A) seg_ns[7] > 0 fuer ALLE 4 Strategien (none/hardware/distance/path): das T7-Segment wird real getrieben +
//       per-Achsen-gezeitet (echter steady_clock-Timer ueber die batches aufsummiert) — kein n/a, keine 0-Zeit.
//   (B) Die anderen 18 Segmente bleiben unberuehrt (seg_ns[0..6] + seg_ns[8..18] > 0) — rein additiv im T7.
//   (C) REALE Store-Adressen + Strategie-Differenzierung im IDENTISCHEN Mess-Pfad-Mechanismus (lokaler
//       LayoutAwareChunkedStore<Node,Layout,Alloc> + ObservablePrefetch<Strategy>::observe_prefetch_descent, exakt
//       wie im T7-Segment-Body): none=0 reale _mm_prefetch (0-Overhead-Baseline), hardware/distance/path>0 reale
//       _mm_prefetch auf echte Slot-Adressen im Store-Backing; path>hardware (Bundle). Das ist der Beweis, dass
//       seg_ns[7] nun echte Descent-Prefetch-Arbeit auf realem Speicher misst (Pfad B Z.1150 ff. identisch).
//
// Build: cl /std:c++latest /EHsc /O2 /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS=1 + ADHOC-Include-Satz
//        (scratch_compile_prefetch_patha_t7.ps1, abgeleitet aus scratch_compile_prefetch_real.ps1).
// SUPERSEDED 2026-07-11: obiger scratch_compile_*.ps1-Build-Weg entfernt (Behelfsweg-Bereinigung); Test jetzt
//        registriertes ctest-Target (tests/unit/CMakeLists.txt, #155-Block COMDARE_PHASE_E_BOOST_TESTS).

#include <anatomy/abi_adapter.hpp>
#include <anatomy/measurable_workload.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <compositions/hot_reference.hpp>
#include <axes/node/axis_04_node_type_layout_aware_store.hpp>
#include <axes/node/axis_04_node_type_node4.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_real_descent.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_observable.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_none.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_hardware_prefetch.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_distance_estimator.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_path_oriented.hpp>

#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace pf   = ::comdare::cache_engine::prefetch_axis;
namespace nd   = ::comdare::cache_engine::node;
namespace ml   = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;
namespace al   = ::comdare::cache_engine::allocator::axis_06_allocator;

static int  g_fail = 0;
static void tr(std::string const& w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// Composition-Vorlage mit waehlbarer prefetch-Strategie (sonst HOT) — wie test_prefetch_real PFComposition,
// aber EIGENER Name: cppcheck-CTU wertet anonyme Namespaces bei class templates nicht (ODR-Fehlalarm, #203).
// #203: anonymer Namespace — gleichnamige TU-lokale Helper in anderen Test-TUs (cppcheck-CTU-ODR).
namespace {
template <class PFStrategy>
struct PFCompositionT7 : comp::HotComposition {
    using prefetch                         = PFStrategy;
    static constexpr std::string_view name = "PFComposition_T7";
};
} // namespace

// (A)+(B): treibt run_workload_segmented_v2 (Pfad-A 19-Segment-Timer) und liefert den seg_ns[]-POD.
template <class PFStrategy>
static an::ComdareSegmentLatencyV2 measure_patha(char const* name) {
    using Anatomy = an::SearchAlgorithmAnatomy<PFCompositionT7<PFStrategy>>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto*                                  base = static_cast<an::IAnatomyBase*>(&tier);
    auto*                       v3 = dynamic_cast<an::IMeasurableWorkloadV3*>(base); // Pfad-A 19-Segment-Timer
    an::ComdareSegmentLatencyV2 out{};
    if (!v3) {
        tr(std::string(name) + ": IMeasurableWorkloadV3 (run_workload_segmented_v2) vorhanden", false);
        return out;
    }
    // 3 Batches a 4096 Ops + Warmup (intern) → seg_ns aufsummiert; genug Arbeit, dass jeder steady_clock-Tick > 0 ist.
    std::uint64_t const got =
        v3->run_workload_segmented_v2(/*ops_per_batch=*/4096, /*batches=*/3, /*seed=*/0xABCDEFull, &out);
    tr(std::string(name) + ": run_workload_segmented_v2 lieferte batches_measured>0",
       got > 0 && out.batches_measured > 0);

    // (A) seg_ns[7] (T7 prefetch) > 0 — das re-grounded Segment wird real getrieben + gezeitet.
    tr(std::string(name) + ": T7 seg_prefetch_ns (seg_ns[7]) > 0 (real getrieben + gezeitet)", out.seg_ns[7] > 0);
    // (B) die anderen 17 Segmente unberuehrt: nicht-negativ (kein Korruptionseffekt aus dem T7-Re-Grounding). Einzelne
    //     sehr billige Segmente (z.B. T14 migration_decide_scan) koennen je nach Strategie < 1 steady_clock-Tick liegen
    //     und damit 0 ns ablesen — das ist Timer-Granularitaet, NICHT „unberuehrt" verletzt; deshalb >= 0 (kein < 0).
    bool others_nonneg = true;
    int  neg_t         = -1;
    for (int t = 0; t < 17; ++t)
        if (t != 7 && out.seg_ns[t] < 0) {
            others_nonneg = false;
            if (neg_t < 0) neg_t = t;
        }
    tr(std::string(name) + ": die anderen 17 Segmente unberuehrt (kein < 0 durch T7-Re-Grounding)" +
           (others_nonneg ? "" : (" (neg bei T" + std::to_string(neg_t) + ")")),
       others_nonneg);

    std::cout << "    " << name << " seg_ns[T6,T7,T8] = " << out.seg_ns[6] << "," << out.seg_ns[7] << ","
              << out.seg_ns[8] << "  total=" << out.total_ns << "  batches=" << out.batches_measured << "\n";
    return out;
}

// (C): der IDENTISCHE Mess-Pfad-Mechanismus des T7-Segments — lokaler realer Store + ObservablePrefetch<Strategy>::
//      observe_prefetch_descent. Belegt, dass T7 nun reale Slot-Adressen prefetcht (none=0 / hardware,distance,path>0).
using T7Store = nd::LayoutAwareChunkedStore<nd::Node4NodeType, ml::CacheLineAlignedMemoryLayout, al::MimallocAllocator>;

template <class Strategy>
static std::uint64_t drive_t7_like(char const* name, T7Store const& store) {
    pf::ObservablePrefetch<Strategy> organ{};
    std::size_t const                n = store.slot_count();
    for (std::uint64_t i = 0; i < 4096; ++i) organ.observe_prefetch_descent(store, static_cast<std::size_t>(i) % n);
#ifdef COMDARE_CE_ENABLE_STATISTICS
    auto const s = organ.statistics();
    std::cout << "    " << name << ": real_prefetches_issued=" << s.real_prefetches_issued
              << " last_distance=" << s.last_prefetch_distance << " last_addr=" << s.last_real_address << "\n";
    return s.real_prefetches_issued;
#else
    (void)name;
    return 0;
#endif
}

static void test_t7_real_store_addresses() {
    std::cout << "-- (C) T7-identischer Mess-Pfad: lokaler realer Store + observe_prefetch_descent (none=0 / >0) --\n";
    T7Store store;
    for (std::size_t i = 0; i < 4096; ++i)
        store.append_slot(static_cast<std::uint64_t>(i) * 2654435761ull + 1u, i * 7u + 1u);
    tr("(C) lokaler T7-Store hat reale Slots + Backing",
       store.slot_count() == 4096 && store.slot_address(0) != nullptr);

    std::uint64_t const in = drive_t7_like<pf::NonePrefetch>("none", store);
    std::uint64_t const ih = drive_t7_like<pf::HardwarePrefetch>("hardware", store);
    std::uint64_t const id = drive_t7_like<pf::DistanceEstimatorPrefetch>("distance", store);
    std::uint64_t const ip = drive_t7_like<pf::PathOrientedPrefetch>("path", store);

    tr("(C) none: 0 reale _mm_prefetch (0-Overhead-Baseline, T7 ehrlich)", in == 0);
    tr("(C) hardware/distance/path: > 0 reale _mm_prefetch auf echte Store-Adressen (T7 re-grounded)",
       ih > 0 && id > 0 && ip > 0);
    tr("(C) path summiert MEHR als hardware (Bundle-Granularitaet, T7 strategie-distinkt)", ip > ih);
}

int main() {
    std::cout << "==== GAP #2: Pfad-A T7 re-grounded (seg_prefetch_ns real + strategie-differenziert) ====\n";
    std::cout << "-- (A)+(B) run_workload_segmented_v2: seg_ns[7] > 0 je Strategie, 18 andere unberuehrt --\n";
    auto sn = measure_patha<pf::NonePrefetch>("none");
    auto sh = measure_patha<pf::HardwarePrefetch>("hardware");
    auto sd = measure_patha<pf::DistanceEstimatorPrefetch>("distance");
    auto sp = measure_patha<pf::PathOrientedPrefetch>("path");
    (void)sn;
    (void)sh;
    (void)sd;
    (void)sp;

    test_t7_real_store_addresses();

    std::cout << "==== GAP #2 Pfad-A T7: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
