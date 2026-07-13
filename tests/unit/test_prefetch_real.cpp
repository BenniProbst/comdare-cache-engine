// K9-Fix (User-Direktive §4.4 2026-06-04 + User-Entscheid 2026-06-18) — REALER prefetch: belegt LITERAL, dass die
// prefetch-Achse (T7) seit dem K9-Fix ECHTES `_mm_prefetch` auf die TATSAECHLICHEN Speicheradressen des Such-
// Descents absetzt (reale Slot-Adressen aus dem LayoutAwareChunkedStore-Backing), strategie-distinkt je
// none/hardware/distance_estimator/path_oriented — NICHT mehr die K9-Pseudo-Adress-Logik (Schluessel-als-Adresse).
//
//   (A) REALE Adresse: jede von PrefetchDescentPolicy<Strategy>::drive() geprefetchte Adresse liegt im REAL
//       allozierten Store-Chunk-Backing ([backing_begin(), backing_end())) — Adress-Range-Check. Kein erfundener
//       Pointer, kein Schluessel-als-Adresse (Beweis: die Adresse == store.slot_address(i), ein echter Slot).
//   (B) DIFFERENZIERUNG der 4 Strategien (real, messbar):
//       • None      → 0 reale _mm_prefetch (0-Overhead-Baseline), keine Adresse.
//       • Hardware  → 1 _mm_prefetch je Descent auf den AKTUELLEN Slot (last_distance==0).
//       • Distance  → 1 _mm_prefetch auf Slot i+N voraus (last_distance==N>0, andere Zieladresse als HW).
//       • Path      → MEHRERE _mm_prefetch (Bundle entlang des Pfads, last_distance==Bundle), groesste issued-Summe.
//       → unterschiedliche real_prefetches_issued / last_prefetch_distance / last_real_address.
//   (C) KEIN CRASH unter /O2 + grossem N: 200k Descent-Trigger je Strategie auf einem grossen Store (Patricia-Fuzz-
//       Analogon) — laeuft die Schleife durch + bleiben alle geprefetchten Adressen im Backing, ist OOB-Freiheit belegt.
//   (D) HOT-PATH-Verdrahtung end-to-end: ein echtes Tier (SearchAlgorithmAbiAdapter) mit prefetch=Hardware treibt
//       ueber tier_insert/tier_lookup die reale observe_prefetch_descent → der Observer-POD (axis_stats[7]) traegt
//       real_prefetches_issued>0 + eine Adresse im Store-Backing; ein None-Tier traegt 0 (ehrliche Baseline).
//
// Build: cl /std:c++latest /EHsc /O2 /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS=1 + ADHOC-Include-Satz
//        (scratch_compile_prefetch_real.ps1, abgeleitet aus scratch_compile_patricia_real.ps1).
// SUPERSEDED 2026-07-11: obiger scratch_compile_*.ps1-Build-Weg entfernt (Behelfsweg-Bereinigung); Test jetzt
//        registriertes ctest-Target (tests/unit/CMakeLists.txt, #155-Block COMDARE_PHASE_E_BOOST_TESTS).

#include <anatomy/abi_adapter.hpp>
#include <anatomy/composition_factory.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <compositions/hot_reference.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp>
#include <axes/node/axis_04_node_type_layout_aware_store.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_real_descent.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_observable.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_none.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_hardware_prefetch.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_distance_estimator.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_path_oriented.hpp>

#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp>
#include <axes/node/axis_04_node_type_node4.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <random>

namespace an    = ::comdare::cache_engine::anatomy;
namespace comp  = ::comdare::cache_engine::compositions;
namespace ce03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace pf    = ::comdare::cache_engine::prefetch_axis;
namespace nd    = ::comdare::cache_engine::node;
namespace ml    = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;
namespace al    = ::comdare::cache_engine::allocator::axis_06_allocator;

static int  g_fail = 0;
static void tr(std::string const& w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// Realer Store (gleicher Typ wie container_ im abi_adapter-Mess-Pfad): Node4 + CLA-Layout + Mimalloc.
using RealStore =
    nd::LayoutAwareChunkedStore<nd::Node4NodeType, ml::CacheLineAlignedMemoryLayout, al::MimallocAllocator>;

static std::uint64_t spread_key(std::uint64_t i) { return (i * 2654435761ull) ^ (i << 17) ^ 0x9E3779B97F4A7C15ull; }

static RealStore make_store(std::size_t n) {
    RealStore s;
    for (std::size_t i = 0; i < n; ++i) s.append_slot(spread_key(i), i * 7u + 1u);
    return s;
}

// (A) REALE Adresse: die geprefetchte Adresse liegt im realen Chunk-Backing (kein Schluessel-als-Adresse).
static void test_real_address_in_backing() {
    std::cout << "-- (A) geprefetchte Adresse liegt im REALEN Store-Backing (Adress-Range-Check) --\n";
    // Node4::max_capacity()==4 → ein N==4-Store ist EIN Chunk: [backing_begin,backing_end) ist das vollstaendige,
    // real allozierte Backing-Range (kein Cross-Chunk). Damit ist der Single-Range-Adress-Check exakt + valide.
    RealStore            store = make_store(4);
    unsigned char const* beg   = store.backing_begin();
    unsigned char const* end   = store.backing_end();
    tr("(A) Store hat reales Chunk-Backing (begin/end != null, end>begin)", beg != nullptr && end > beg);

    // Hardware: prefetcht slot i -> die last_address MUSS == store.slot_address(i) sein UND im Backing liegen.
    pf::ObservablePrefetch<pf::HardwarePrefetch> hw{};
    bool                                         all_in       = true;
    bool                                         addr_eq_slot = true;
    for (std::size_t i = 0; i < store.slot_count(); ++i) {
        auto const res = pf::PrefetchDescentPolicy<pf::HardwarePrefetch>::drive(store, i);
        if (res.prefetches_issued != 1) all_in = false;
        if (res.last_address != store.slot_address(i)) addr_eq_slot = false;   // GENAU der reale Slot-Pointer
        if (res.last_address < beg || res.last_address >= end) all_in = false; // im realen Backing
    }
    tr("(A) Hardware prefetcht GENAU die reale Slot-Adresse store.slot_address(i)", addr_eq_slot);
    tr("(A) alle geprefetchten Adressen liegen im [backing_begin,backing_end) (kein OOB, real)", all_in);

    // Gegenbeweis K9: ein Schluessel-Wert als Adresse interpretiert laege NICHT im Backing (riesiger uintptr).
    std::uint64_t const a_key = spread_key(3);
    auto const*         fake  = reinterpret_cast<unsigned char const*>(static_cast<std::uintptr_t>(a_key));
    tr("(A) Gegenbeweis: Schluessel-als-Adresse liegt NICHT im realen Backing (K9 war Pseudo-Adresse)",
       fake < beg || fake >= end);
}

// (B) DIFFERENZIERUNG: die 4 Strategien setzen real unterschiedliche Prefetch-Zaehler/Distanzen/Adressen.
static void test_four_strategies_differ() {
    std::cout << "-- (B) 4 Strategien differenzieren real (issued/distance/address) --\n";
    RealStore         store = make_store(64);
    std::size_t const i     = 10; // ein konkreter Descent-Slot

    auto rn = pf::PrefetchDescentPolicy<pf::NonePrefetch>::drive(store, i);
    auto rh = pf::PrefetchDescentPolicy<pf::HardwarePrefetch>::drive(store, i);
    auto rd = pf::PrefetchDescentPolicy<pf::DistanceEstimatorPrefetch>::drive(store, i);
    auto rp = pf::PrefetchDescentPolicy<pf::PathOrientedPrefetch>::drive(store, i);

    std::cout << "     none:     issued=" << rn.prefetches_issued << " dist=" << rn.last_distance << "\n";
    std::cout << "     hardware: issued=" << rh.prefetches_issued << " dist=" << rh.last_distance
              << " addr=" << reinterpret_cast<std::uintptr_t>(rh.last_address) << "\n";
    std::cout << "     distance: issued=" << rd.prefetches_issued << " dist=" << rd.last_distance
              << " addr=" << reinterpret_cast<std::uintptr_t>(rd.last_address) << "\n";
    std::cout << "     path:     issued=" << rp.prefetches_issued << " dist=" << rp.last_distance
              << " addr=" << reinterpret_cast<std::uintptr_t>(rp.last_address) << "\n";

    tr("(B) None: 0 reale Prefetches (0-Overhead-Baseline)", rn.prefetches_issued == 0 && rn.last_address == nullptr);
    tr("(B) Hardware: genau 1 Prefetch auf den AKTUELLEN Slot (dist==0)",
       rh.prefetches_issued == 1 && rh.last_distance == 0);
    tr("(B) Distance: 1 Prefetch VORAUS (dist>0), andere Zieladresse als Hardware",
       rd.prefetches_issued == 1 && rd.last_distance > 0 && rd.last_address != rh.last_address);
    tr("(B) Path: MEHRERE Prefetches (Bundle, issued>1), groesste issued-Summe",
       rp.prefetches_issued > 1 && rp.prefetches_issued >= rh.prefetches_issued &&
           rp.prefetches_issued >= rd.prefetches_issued);
    // Distance-Ziel = slot i+N (real voraus) → muss == store.slot_address(i+dist) sein (echte Traversal-Adresse).
    std::size_t const dtarget = i + rd.last_distance;
    bool const dist_real = (dtarget < store.slot_count()) ? (rd.last_address == store.slot_address(dtarget)) : true;
    tr("(B) Distance-Zieladresse == realer Slot i+N (echte Voraus-Adresse, kein erfundener Pointer)", dist_real);
}

// (C) KEIN CRASH unter /O2 + grossem N: 200k Trigger je Strategie auf grossem Store; alle Adressen im Backing.
template <class Strategy>
static std::uint64_t fuzz_strategy(RealStore const& store, std::string_view name) {
    constexpr std::uint64_t N = 200000;
    std::mt19937_64         rng(0xC0FFEE2026ull);
    std::uint64_t           total_issued = 0;
    std::uint64_t           oob          = 0;
    std::size_t const       n            = store.slot_count();
    // Bei einem Single-Chunk-Store (N<=cap_) ist [backing_begin,backing_end) das vollstaendige Range; bei mehreren
    // Chunks pruefen wir je Trigger gegen den exakten Slot-Pointer (immer real). Hier: Single-Chunk via cap-grossem N.
    for (std::uint64_t t = 0; t < N; ++t) {
        std::size_t const i   = static_cast<std::size_t>(rng() % (n == 0 ? 1 : n));
        auto const        res = pf::PrefetchDescentPolicy<Strategy>::drive(store, i);
        total_issued += res.prefetches_issued;
        if (res.last_address != nullptr) {
            // last_address MUSS ein realer Slot-Pointer sein (in irgendeinem Chunk) → slot_address-Identitaet pruefen.
            bool found = false;
            for (std::size_t j = 0; j < n && !found; ++j)
                if (store.slot_address(j) == res.last_address) found = true;
            if (!found) ++oob;
        }
    }
    std::cout << "     " << name << ": total_issued=" << total_issued << " oob=" << oob << "\n";
    tr(std::string("(C) ") + std::string(name) + ": 200k Trigger ohne OOB (jede Adresse ein realer Slot)", oob == 0);
    return total_issued;
}

static void test_fuzz_no_crash() {
    std::cout << "-- (C) Fuzz/O2: 200k Descent-Trigger je Strategie auf grossem Store (kein Crash, kein OOB) --\n";
    // Node4::max_capacity ist klein → grosser N erzeugt mehrere Chunks. Wir nutzen einen mittelgrossen Store, sodass
    // der per-Slot-Identitaets-Check (alle Chunks) bezahlbar bleibt, aber die Klemm-Logik (i+dist/Bundle) real greift.
    RealStore           store = make_store(256);
    std::uint64_t const in    = fuzz_strategy<pf::NonePrefetch>(store, "none");
    std::uint64_t const ih    = fuzz_strategy<pf::HardwarePrefetch>(store, "hardware");
    std::uint64_t const id    = fuzz_strategy<pf::DistanceEstimatorPrefetch>(store, "distance");
    std::uint64_t const ip    = fuzz_strategy<pf::PathOrientedPrefetch>(store, "path");
    tr("(C) None summiert 0 Prefetches (ehrliche Baseline auch unter Fuzz)", in == 0);
    tr("(C) Hardware/Distance/Path summieren >0 Prefetches (real getrieben)", ih > 0 && id > 0 && ip > 0);
    tr("(C) Path summiert MEHR als Hardware (Bundle-Granularitaet, real distinkt)", ip > ih);
}

// (D) HOT-PATH end-to-end: echtes Tier mit prefetch=Hardware vs None → Observer-POD axis_stats[7] traegt die reale
//     Prefetch-Telemetrie aus tier_insert/tier_lookup (kein Schluessel-als-Adresse, sondern container_-Store-Adresse).
// #203: anonymer Namespace — gleichnamige TU-lokale Helper in anderen Test-TUs (cppcheck-CTU-ODR).
namespace {
template <class PFStrategy>
struct PFComposition : comp::HotComposition {
    using prefetch                         = PFStrategy;
    static constexpr std::string_view name = "PFComposition";
};
} // namespace

template <class PFStrategy>
using PFStoreBackedComposition = an::AdHocComposition<
    ce03a::Array256SearchAlgo, comp::HotComposition::cache_traversal, comp::HotComposition::mapping,
    comp::HotComposition::path_compression, comp::HotComposition::node_type, comp::HotComposition::memory_layout,
    comp::HotComposition::allocator, PFStrategy, comp::HotComposition::concurrency, comp::HotComposition::serialization,
    comp::HotComposition::telemetry, comp::HotComposition::value_handle, comp::HotComposition::isa,
    comp::HotComposition::index_organization, comp::HotComposition::io_dispatch, comp::HotComposition::migration_policy,
    comp::HotComposition::filter, comp::HotComposition::queuing_q1, comp::HotComposition::queuing_q2>;

template <class Composition>
static an::ComdareTierObserverSnapshot drive_composition_and_observe(std::uint64_t n_keys,
                                                                     std::uint64_t key_mask = ~0ull) {
    using Anatomy = an::SearchAlgorithmAnatomy<Composition>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto*                                  base = static_cast<an::IAnatomyBase*>(&tier);
    auto*                                  drv  = dynamic_cast<an::IDriveableTier*>(base);
    auto*                                  obs  = dynamic_cast<an::IObservableTier*>(base);
    an::ComdareTierObserverSnapshot        snap{};
    if (!drv || !obs) {
        tr("(D) IDriveableTier + IObservableTier vorhanden", false);
        return snap;
    }
    for (std::uint64_t i = 0; i < n_keys; ++i) (void)drv->tier_insert(spread_key(i) & key_mask, i * 11u + 5u);
    std::uint64_t v = 0;
    for (std::uint64_t i = 0; i < n_keys; ++i) (void)drv->tier_lookup(spread_key(i) & key_mask, &v);
    obs->tier_observe(&snap);
    return snap;
}

template <class PFStrategy>
static an::ComdareTierObserverSnapshot drive_store_backed_tier_and_observe(std::uint64_t n_keys) {
    // #278 (2026-07-06): seit #188-4c-ii ist container_algorithm_ Array256-TREU (DirectAddressTraversal,
    // Domaene 0..255) — ungespreizte u64-Keys wuerden ALLE abgelehnt (Tier leer -> slot_count()==0 ->
    // Descent-Guard blockt -> issued=0). Die Klemme auf die Byte-Domaene haelt das Tier real befuellt;
    // der Testzweck (REALE _mm_prefetch auf reale Store-Adressen) bleibt unveraendert.
    return drive_composition_and_observe<PFStoreBackedComposition<PFStrategy>>(n_keys, 0xFFull);
}

template <class PFStrategy>
static an::ComdareTierObserverSnapshot drive_hull_tier_and_observe(std::uint64_t n_keys) {
    return drive_composition_and_observe<PFComposition<PFStrategy>>(n_keys);
}

static void test_hotpath_end_to_end() {
    std::cout
        << "-- (D) HOT-PATH end-to-end: store-backed AdHoc prefetch=Hardware vs None (Observer axis_stats[7]) --\n";
    constexpr std::uint64_t N      = 2000;
    auto                    sn_hw  = drive_store_backed_tier_and_observe<pf::HardwarePrefetch>(N);
    auto                    sn_no  = drive_store_backed_tier_and_observe<pf::NonePrefetch>(N);
    auto                    h_hw   = drive_hull_tier_and_observe<pf::HardwarePrefetch>(N);
    auto                    h_dist = drive_hull_tier_and_observe<pf::DistanceEstimatorPrefetch>(N);
    auto                    h_path = drive_hull_tier_and_observe<pf::PathOrientedPrefetch>(N);

    std::uint64_t const hw_issued = sn_hw.axis_stats[7][5]; // real_prefetches_issued
    std::uint64_t const hw_dist   = sn_hw.axis_stats[7][6]; // last_prefetch_distance
    std::uint64_t const hw_addr   = sn_hw.axis_stats[7][7]; // last_real_address
    std::uint64_t const no_issued = sn_no.axis_stats[7][5];

    std::cout << "     store-backed hardware-Tier: real_prefetches_issued=" << hw_issued << " last_distance=" << hw_dist
              << " last_address=" << hw_addr << "\n";
    std::cout << "     store-backed none-Tier:     real_prefetches_issued=" << no_issued << "\n";

    tr("(D) store-backed Hardware-Tier traegt >0 reale _mm_prefetch im Observer (hot-path getrieben)", hw_issued > 0);
    tr("(D) store-backed Hardware-Tier traegt eine reale Adresse (last_real_address != 0)", hw_addr != 0);
    tr("(D) store-backed Hardware-Tier last_distance==0 (HW prefetcht den aktuellen Slot)", hw_dist == 0);
    tr("(D) store-backed None-Tier traegt 0 reale Prefetches (ehrliche 0-Overhead-Baseline)", no_issued == 0);
    tr("(D) Differenzierung im echten store-backed Tier: Hardware-issued > None-issued", hw_issued > no_issued);

    // #188-4c-i: Huellen-Komposition container_-authoritativ -> store-Achsen honest-0 (bis observe-Hooks #234).
    tr("(D) Hot-Huelle Hardware issued==0", h_hw.axis_stats[7][5] == 0u);
    tr("(D) Hot-Huelle Hardware distance/address==0", h_hw.axis_stats[7][6] == 0u && h_hw.axis_stats[7][7] == 0u);
    tr("(D) Hot-Huelle Distance issued==0", h_dist.axis_stats[7][5] == 0u);
    tr("(D) Hot-Huelle Path issued==0", h_path.axis_stats[7][5] == 0u);
}

int main() {
    std::cout
        << "==== K9-Fix: REALER prefetch (_mm_prefetch auf reale Store-Adressen, 4 Strategien differenzieren) ====\n";
    test_real_address_in_backing();
    test_four_strategies_differ();
    test_fuzz_no_crash();
    test_hotpath_end_to_end();
    std::cout << "==== K9-Fix REALER prefetch: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
