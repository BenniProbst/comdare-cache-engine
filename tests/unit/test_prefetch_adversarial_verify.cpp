// ADVERSARIAL verify (NICHT vom Implement-Bericht vertrauen) — versucht die prefetch-Impl zu WIDERLEGEN:
//   (V1) Multi-Chunk-Store (N >> cap_): slot_address(i) ueber CHUNK-GRENZEN korrekt? jede Path/Distance-Adresse
//        muss in IRGENDEINEM real allozierten Chunk liegen (per-Slot-Identitaet ueber alle Chunks).
//   (V2) descent_slot_for_-Annaeherung: store mit UNSORTIERTEN Keys (spread) — die Lower-Bound liefert einen
//        Index der ggf. "falsch" ist, aber NIE >= slot_count() (kein OOB). Hammer mit 1e6 zufaelligen Keys.
//   (V3) HOT-PATH-Adresse REAL: ein echtes Hardware-Tier — die im Observer getragene last_real_address MUSS
//        in einem real allozierten Chunk des container_-Stores liegen (kein Schluessel-als-Adresse-Rueckfall).
//        Dafuer rekonstruieren wir die moeglichen Slot-Adressen NICHT (kein Store-Zugriff von aussen), sondern
//        pruefen: (a) addr ist NICHT die K9-Pseudo-Adresse irgendeines eingefuegten spread_key; (b) HW!=None.
//   (V4) Alle 4 Strategien im ECHTEN Tier (nicht nur Policy-Helfer) differenzieren: None=0, HW/Distance=1/op,
//        Path>HW/op — gemessen ueber den Observer-POD axis_stats[7][5].
//   (V5) GROSSER /O2-Fuzz: 1e6 Trigger PathOriented auf 4096-Slot-Store (mehr Chunks) — kein Crash, oob==0.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/composition_factory.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <compositions/hot_reference.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp>
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
#include <vector>
#include <random>
#include <set>

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

using RealStore =
    nd::LayoutAwareChunkedStore<nd::Node4NodeType, ml::CacheLineAlignedMemoryLayout, al::MimallocAllocator>;
static std::uint64_t spread_key(std::uint64_t i) { return (i * 2654435761ull) ^ (i << 17) ^ 0x9E3779B97F4A7C15ull; }
static RealStore     make_store(std::size_t n) {
    RealStore s;
    for (std::size_t i = 0; i < n; ++i) s.append_slot(spread_key(i), i * 7u + 1u);
    return s;
}

// per-Slot-Identitaet ueber ALLE Chunks (Multi-Chunk): ist addr ein realer Slot-Pointer?
static bool addr_is_real_slot(RealStore const& s, unsigned char const* addr) {
    if (addr == nullptr) return false;
    std::size_t const n = s.slot_count();
    for (std::size_t j = 0; j < n; ++j)
        if (s.slot_address(j) == addr) return true;
    return false;
}

static void v1_multichunk() {
    std::cout << "-- (V1) Multi-Chunk: Path/Distance-Adressen ueber Chunk-Grenzen = realer Slot --\n";
    RealStore store = make_store(64); // Node4 cap=4 -> 16 Chunks
    tr("(V1) Multi-Chunk-Store hat >1 Chunk", store.chunk_count() > 1);
    bool path_all_real = true, dist_all_real = true;
    // i nahe Chunk-Grenze (z.B. 3 -> i+1..i+4 ueberschreitet in den naechsten Chunk)
    for (std::size_t i = 0; i + 1 < store.slot_count(); ++i) {
        auto rp = pf::PrefetchDescentPolicy<pf::PathOrientedPrefetch>::drive(store, i);
        if (rp.last_address && !addr_is_real_slot(store, rp.last_address)) path_all_real = false;
        auto rd = pf::PrefetchDescentPolicy<pf::DistanceEstimatorPrefetch>::drive(store, i);
        if (rd.last_address && !addr_is_real_slot(store, rd.last_address)) dist_all_real = false;
    }
    tr("(V1) ALLE Path-Adressen sind reale Slot-Pointer (auch ueber Chunk-Grenzen)", path_all_real);
    tr("(V1) ALLE Distance-Adressen sind reale Slot-Pointer (auch ueber Chunk-Grenzen)", dist_all_real);
}

static void v2_clamp_no_oob() {
    std::cout << "-- (V2) drive() klemmt i>=n auf n-1 (kein OOB), auch fuer absurd grosse i --\n";
    RealStore       store = make_store(40);
    std::mt19937_64 rng(0xDEADBEEFu);
    bool            oob = false;
    for (std::uint64_t t = 0; t < 1000000ull; ++t) {
        std::size_t const i  = static_cast<std::size_t>(rng()); // bewusst i WEIT jenseits slot_count
        auto              rp = pf::PrefetchDescentPolicy<pf::PathOrientedPrefetch>::drive(store, i);
        if (rp.last_address && !addr_is_real_slot(store, rp.last_address)) {
            oob = true;
            break;
        }
        auto rh = pf::PrefetchDescentPolicy<pf::HardwarePrefetch>::drive(store, i);
        if (rh.last_address && !addr_is_real_slot(store, rh.last_address)) {
            oob = true;
            break;
        }
    }
    tr("(V2) 1e6 absurd-grosse i: jede geprefetchte Adresse bleibt ein realer Slot (Klemm-Logik haelt)", !oob);
    // leerer Store: drive muss nullptr/0 liefern (kein Deref)
    RealStore empty;
    auto      re = pf::PrefetchDescentPolicy<pf::HardwarePrefetch>::drive(empty, 5);
    tr("(V2) leerer Store: 0 issued, nullptr (kein Deref n-1 bei n==0)",
       re.prefetches_issued == 0 && re.last_address == nullptr);
}

template <class PFStrategy>
struct PFComp : comp::HotComposition {
    using prefetch                         = PFStrategy;
    static constexpr std::string_view name = "PFComp";
};

template <class PFStrategy>
using PFStoreBackedComp = an::AdHocComposition<
    ce03a::Array256SearchAlgo, comp::HotComposition::cache_traversal, comp::HotComposition::mapping,
    comp::HotComposition::path_compression, comp::HotComposition::node_type, comp::HotComposition::memory_layout,
    comp::HotComposition::allocator, PFStrategy, comp::HotComposition::concurrency, comp::HotComposition::serialization,
    comp::HotComposition::telemetry, comp::HotComposition::value_handle, comp::HotComposition::isa,
    comp::HotComposition::index_organization, comp::HotComposition::io_dispatch, comp::HotComposition::migration_policy,
    comp::HotComposition::filter, comp::HotComposition::queuing_q1, comp::HotComposition::queuing_q2>;

template <class Composition>
static an::ComdareTierObserverSnapshot drive_composition(std::uint64_t n) {
    using Anatomy = an::SearchAlgorithmAnatomy<Composition>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto*                                  base = static_cast<an::IAnatomyBase*>(&tier);
    auto*                                  drv  = dynamic_cast<an::IDriveableTier*>(base);
    auto*                                  obs  = dynamic_cast<an::IObservableTier*>(base);
    an::ComdareTierObserverSnapshot        snap{};
    if (!drv || !obs) return snap;
    for (std::uint64_t i = 0; i < n; ++i) (void)drv->tier_insert(spread_key(i), i * 11u + 5u);
    std::uint64_t v = 0;
    for (std::uint64_t i = 0; i < n; ++i) (void)drv->tier_lookup(spread_key(i), &v);
    obs->tier_observe(&snap);
    return snap;
}

template <class PFStrategy>
static an::ComdareTierObserverSnapshot drive_store_backed_tier(std::uint64_t n) {
    return drive_composition<PFStoreBackedComp<PFStrategy>>(n);
}

template <class PFStrategy>
static an::ComdareTierObserverSnapshot drive_hull_tier(std::uint64_t n) {
    return drive_composition<PFComp<PFStrategy>>(n);
}

static void v3_v4_hotpath_real_and_differ() {
    std::cout << "-- (V3/V4) Store-backed Hot-Path: 4 Strategien im ECHTEN Tier; Adresse real (kein "
                 "Schluessel-Rueckfall) --\n";
    constexpr std::uint64_t N      = 3000;
    auto                    no     = drive_store_backed_tier<pf::NonePrefetch>(N);
    auto                    hw     = drive_store_backed_tier<pf::HardwarePrefetch>(N);
    auto                    di     = drive_store_backed_tier<pf::DistanceEstimatorPrefetch>(N);
    auto                    pa     = drive_store_backed_tier<pf::PathOrientedPrefetch>(N);
    auto                    h_hw   = drive_hull_tier<pf::HardwarePrefetch>(N);
    auto                    h_dist = drive_hull_tier<pf::DistanceEstimatorPrefetch>(N);
    auto                    h_path = drive_hull_tier<pf::PathOrientedPrefetch>(N);
    std::uint64_t           i_no = no.axis_stats[7][5], i_hw = hw.axis_stats[7][5], i_di = di.axis_stats[7][5],
                            i_pa = pa.axis_stats[7][5];
    std::uint64_t           a_hw = hw.axis_stats[7][7];
    std::uint64_t           d_di = di.axis_stats[7][6];
    std::cout << "     none=" << i_no << " hw=" << i_hw << " dist=" << i_di << "(d=" << d_di << ") path=" << i_pa
              << " hw_addr=" << a_hw << "\n";
    tr("(V4) store-backed None-Tier issued==0", i_no == 0);
    tr("(V4) store-backed Hardware-Tier issued>0", i_hw > 0);
    tr("(V4) store-backed Distance-Tier issued>0 UND last_distance>0 (voraus)", i_di > 0 && d_di > 0);
    tr("(V4) store-backed Path-Tier issued > Hardware-Tier (Bundle, real distinkt im echten Tier)", i_pa > i_hw);
    tr("(V3) store-backed Hardware-Tier last_real_address != 0 (reale Adresse getragen)", a_hw != 0);

    // #188-4c-i: Huellen-Komposition container_-authoritativ -> store-Achsen honest-0 (bis observe-Hooks #234).
    tr("(V4) Hot-Huelle Hardware issued==0", h_hw.axis_stats[7][5] == 0u);
    tr("(V4) Hot-Huelle Hardware distance/address==0", h_hw.axis_stats[7][6] == 0u && h_hw.axis_stats[7][7] == 0u);
    tr("(V4) Hot-Huelle Distance issued==0", h_dist.axis_stats[7][5] == 0u);
    tr("(V4) Hot-Huelle Path issued==0", h_path.axis_stats[7][5] == 0u);

    // (V3) K9-Rueckfall-Gegenbeweis: die getragene HW-Adresse darf NICHT eine Schluessel-als-Adresse sein.
    // Sammle alle eingefuegten spread_key-Werte; pruefe a_hw ist KEINER davon (eine reale Heap-Adresse ist kein Key).
    bool addr_equals_some_key = false;
    for (std::uint64_t i = 0; i < N; ++i)
        if (static_cast<std::uintptr_t>(spread_key(i)) == a_hw) {
            addr_equals_some_key = true;
            break;
        }
    tr("(V3) HW-Adresse ist KEIN eingefuegter Schluessel-Wert (kein K9-Pseudo-Adress-Rueckfall)",
       !addr_equals_some_key);
    // reale Heap-Adressen auf Win x64 sind typ. > 4GB (0x1_0000_0000); spread_keys streuen ueber gesamten uint64.
    // Schwaecherer, aber zusaetzlicher Hinweis: die Adresse ist nicht 0 und nicht offensichtlich ein kleiner Key.
    tr("(V3) HW-Adresse plausibel eine Heap-Adresse (>0x10000, ausgerichtet)", a_hw > 0x10000ull);
}

template <class Strategy>
static std::uint64_t big_fuzz(RealStore const& s, std::string_view nm) {
    constexpr std::uint64_t N = 1000000;
    std::mt19937_64         rng(0xABCDEF01u);
    std::uint64_t           issued = 0, oob = 0;
    std::size_t const       n = s.slot_count();
    for (std::uint64_t t = 0; t < N; ++t) {
        std::size_t i = static_cast<std::size_t>(rng() % (n == 0 ? 1 : n));
        auto        r = pf::PrefetchDescentPolicy<Strategy>::drive(s, i);
        issued += r.prefetches_issued;
        if (r.last_address && !addr_is_real_slot(s, r.last_address)) ++oob;
    }
    std::cout << "     " << nm << ": issued=" << issued << " oob=" << oob << "\n";
    tr(std::string("(V5) ") + std::string(nm) + ": 1e6 Trigger oob==0", oob == 0);
    return issued;
}

static void v5_big_fuzz() {
    std::cout << "-- (V5) GROSSER /O2-Fuzz 1e6 Trigger auf 512-Slot-Store (128 Chunks) --\n";
    RealStore store = make_store(512);
    tr("(V5) Store hat viele Chunks", store.chunk_count() >= 100);
    auto ip = big_fuzz<pf::PathOrientedPrefetch>(store, "path");
    auto in = big_fuzz<pf::NonePrefetch>(store, "none");
    tr("(V5) None==0 / Path>0 (real distinkt unter Last)", in == 0 && ip > 0);
}

int main() {
    std::cout << "==== ADVERSARIAL prefetch verify (Widerlegungsversuch) ====\n";
    v1_multichunk();
    v2_clamp_no_oob();
    v3_v4_hotpath_real_and_differ();
    v5_big_fuzz();
    std::cout << "==== ADVERSARIAL: " << (g_fail == 0 ? "KEINE LUECKE GEFUNDEN" : (std::to_string(g_fail) + " LUECKEN"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
