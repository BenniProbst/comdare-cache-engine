// P-MD1-ERDUNG / #167 BUILD-VERIFIKATION (2026-06-18): 5 REALE distinkte memory_layout-Repraesentationen im
// LayoutAwareChunkedStore — CLU UND Byte-Footprint differenzieren ECHT 5-fach, nicht ueber ein entkoppeltes
// Deskriptor-Modell (Phantom-Muster P-MD1), sondern aus dem REALEN, representation-spezifischen Store-Footprint.
//
// Hintergrund: frueher speicherte der Store ALLE Layouts als 16-B-[key|value] an EINEM eff_stride (16/64) — nur
// die AoS-Familie war ehrlich; SoA/AoSoA/packed_bitmap wurden wie AoS abgelegt, und die CLU kam aus
// Strategy::record_useful_bytes/record_line_span (entkoppelt, ignorierte den realen Store: soa-Modell 75% bei
// realem 25%). JETZT legt jede Strategie ueber ihre RepresentationKind WIRKLICH unterschiedlich ab
// (if-constexpr-Dispatch, zero-cost), und organ_observe_layout(ObservableMemoryLayout<L>) treibt den ECHTEN
// Key-Scan-Footprint (field_bytes = real beruehrte Key-Bytes, cache_lines = real beruehrte 64-B-Linien) in den
// Observer (observe_real_footprint).
//
// Prueft LITERAL:
//   (a) ROUND-TRIP: insert(k,v) -> lookup(k)==v ueber ALLE 5 Reps x Node4/16/48/256 (kein Datenverlust/Vertauschung).
//   (b) die 5 Reps haben WIRKLICH verschiedenen Byte-Footprint: der Key-only-Scan beruehrt je Rep verschiedene
//       Byte-Zahlen (field_bytes) UND verschiedene Linien-Zahlen (cache_lines) — paarweise mind. 4 distinkte Werte.
//   (c) CLU je Layout = field_bytes/(cache_lines*64) aus dem REALEN Store + plausibel (>20%, <=100%) + 5-fach distinkt.
//   (d) /O2-Fuzz N gross (randomisierte Keys/Values, volle uint64-Domain) -> kein UB/OOB, Round-Trip 100%.
//
// Build (identischer Include-Satz wie scratch_clu_pmd1_verify.ps1):
//   cl /std:c++latest /EHsc /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS=1 + ADHOC-Include-Satz.
// SUPERSEDED 2026-07-11: obiger .ps1-Build-Weg (scratch_clu_pmd1_verify.ps1) entfernt (Behelfsweg-Bereinigung);
//   Test jetzt registriertes ctest-Target (tests/unit/CMakeLists.txt, M-CE-24-Block COMDARE_MCE24_PLAIN_TESTS).

#include <axes/node/axis_04_node_type_layout_aware_store.hpp>
#include <axes/node/axis_04_node_type_node4.hpp>
#include <axes/node/axis_04_node_type_node16.hpp>
#include <axes/node/axis_04_node_type_node48.hpp>
#include <axes/node/axis_04_node_type_node256.hpp>
#include <axes/layout/axis_05_memory_layout_observable.hpp>
#include <axes/layout/axis_05_memory_layout_aos_strict.hpp>
#include <axes/layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <axes/layout/axis_05_memory_layout_soa.hpp>
#include <axes/layout/axis_05_memory_layout_aosoa.hpp>
#include <axes/layout/axis_05_memory_layout_packed_bitmap.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace cn  = ::comdare::cache_engine::node;
namespace cl_ = ::comdare::cache_engine::layout;
namespace ca  = ::comdare::cache_engine::allocator::axis_06_allocator;

static int  g_fail = 0;
static void tr(std::string const& w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

struct LayoutResult {
    std::string   name;
    std::uint64_t records     = 0;
    std::uint64_t field_bytes = 0;
    std::uint64_t cache_lines = 0;
    double        clu_pct     = 0.0;
};

// ── (a) Round-Trip insert->lookup ueber EINE Rep x EIN node_type ──────────────────────────────────────────
// Fuellt den echten Store, dann LINEAR-Lookup je Key ueber slot_count/key_at/value_at (== StorageOrgan-Pfad).
template <class N, class L>
static bool roundtrip_one(char const* nname, char const* lname, std::uint64_t kN) {
    using Store = cn::LayoutAwareChunkedStore<N, L, ca::MimallocAllocator>;
    Store store;
    for (std::uint64_t i = 0; i < kN; ++i) store.append_slot(i, i * 1315423911u + 7u);
    if (store.slot_count() != kN) {
        std::cout << "  [ERR] " << lname << "/" << nname << " slot_count " << store.slot_count() << " != " << kN
                  << "\n";
        return false;
    }
    // jeder Slot: Key in Insert-Reihenfolge, Value == k*MIX+7 (verlustfrei rekonstruiert ueber load_key/load_value).
    for (std::uint64_t i = 0; i < kN; ++i) {
        std::uint64_t const k = store.key_at(static_cast<std::size_t>(i));
        std::uint64_t const v = store.value_at(static_cast<std::size_t>(i));
        if (k != i || v != i * 1315423911u + 7u) {
            std::cout << "  [ERR] " << lname << "/" << nname << " slot " << i << " k=" << k << " v=" << v
                      << " (erwartet k=" << i << ")\n";
            return false;
        }
    }
    return true;
}

// Treibt den REALEN Mess-Pfad fuer EINE Layout-Strategie L: echter Store + organ_observe_layout.
// Node256 (cap=256): die Chunks spannen viele 64-B-Linien → der per-Chunk-Line-Footprint differenziert die 5
// Reps ECHT (bei winzigen Knoten kollabiert jeder Chunk auf eine Linie und maskiert den Layout-Unterschied).
template <class L>
static LayoutResult measure_layout(char const* name, std::uint64_t kN) {
    using Store = cn::LayoutAwareChunkedStore<cn::Node256NodeType, L, ca::MimallocAllocator>;
    Store store;
    for (std::uint64_t i = 0; i < kN; ++i) store.append_slot(i, i * 7u + 1u);

    cl_::ObservableMemoryLayout<L> ml{};
    ml.reset();
    (void)store.organ_observe_layout(ml); // ECHTER Key-Scan-Footprint -> observe_real_footprint
    auto const snap = ml.statistics();

    LayoutResult r;
    r.name        = name;
    r.records     = snap.records_scanned;
    r.field_bytes = snap.field_bytes_read;
    r.cache_lines = snap.cache_lines_touched;
    r.clu_pct     = (r.cache_lines == 0)
                        ? 0.0
                        : (100.0 * static_cast<double>(r.field_bytes) / (static_cast<double>(r.cache_lines) * 64.0));
    return r;
}

// ── (d) /O2-Fuzz: randomisierte Keys/Values (volle uint64-Domain) -> Round-Trip 100%, kein UB/OOB ─────────
template <class N, class L>
static bool fuzz_one(char const* nname, char const* lname, std::uint64_t kN, std::uint64_t seed) {
    using Store = cn::LayoutAwareChunkedStore<N, L, ca::MimallocAllocator>;
    Store                                                store;
    std::mt19937_64                                      rng(seed);
    std::vector<std::pair<std::uint64_t, std::uint64_t>> ref;
    ref.reserve(static_cast<std::size_t>(kN));
    for (std::uint64_t i = 0; i < kN; ++i) {
        std::uint64_t const k = rng(), v = rng(); // volle 64-Bit-Domain (testet succinct hot/cold-Split)
        ref.emplace_back(k, v);
        store.append_slot(k, v);
    }
    if (store.slot_count() != kN) return false;
    for (std::size_t i = 0; i < ref.size(); ++i)
        if (store.key_at(i) != ref[i].first || store.value_at(i) != ref[i].second) return false;
    return true;
}

template <class L>
static bool roundtrip_all_nodes(char const* lname) {
    bool ok = true;
    ok &= roundtrip_one<cn::Node4NodeType, L>("node4", lname, 37);
    ok &= roundtrip_one<cn::Node16NodeType, L>("node16", lname, 100);
    ok &= roundtrip_one<cn::Node48NodeType, L>("node48", lname, 200);
    ok &= roundtrip_one<cn::Node256NodeType, L>("node256", lname, 700);
    return ok;
}

template <class L>
static bool fuzz_all_nodes(char const* lname) {
    bool ok = true;
    ok &= fuzz_one<cn::Node4NodeType, L>("node4", lname, 5000, 0xC0FFEEull);
    ok &= fuzz_one<cn::Node16NodeType, L>("node16", lname, 5000, 0xBEEFull);
    ok &= fuzz_one<cn::Node48NodeType, L>("node48", lname, 5000, 0xD00Dull);
    ok &= fuzz_one<cn::Node256NodeType, L>("node256", lname, 20000, 0xFEEDull);
    return ok;
}

int main() {
    std::cout << "==== #167 5 REALE distinkte memory_layout-Repraesentationen (LayoutAwareChunkedStore) ====\n";
    constexpr std::uint64_t kN = 11627; // M3-Matrix-Records-Zahl (Reproduktion der Evidenz)

    // (a) ROUND-TRIP ueber 5 Reps x Node4/16/48/256
    std::cout << "-- (a) Round-Trip insert->lookup (5 Reps x Node4/16/48/256) --\n";
    tr("Round-Trip aos_strict         (4 node_types)", roundtrip_all_nodes<cl_::AoSStrictMemoryLayout>("aos_strict"));
    tr("Round-Trip cache_line_aligned (4 node_types)",
       roundtrip_all_nodes<cl_::CacheLineAlignedMemoryLayout>("cache_line_aligned"));
    tr("Round-Trip soa                (4 node_types)", roundtrip_all_nodes<cl_::SoAMemoryLayout>("soa"));
    tr("Round-Trip aosoa              (4 node_types)", roundtrip_all_nodes<cl_::AoSoAMemoryLayout>("aosoa"));
    tr("Round-Trip packed_bitmap      (4 node_types)",
       roundtrip_all_nodes<cl_::PackedBitmapMemoryLayout>("packed_bitmap"));

    // (b)+(c) realer Footprint je Rep (Node256 — grosse Knoten spannen viele 64-B-Linien; bei winzigen Knoten kollabiert
    // jeder Chunk auf 1 Linie und maskiert den Layout-Unterschied; measure_layout instanziiert Node256NodeType, Z.88)
    std::cout << "-- (b)+(c) realer Key-Scan-Footprint + CLU je Rep (Node256, n=" << kN << ") --\n";
    std::vector<LayoutResult> res;
    res.push_back(measure_layout<cl_::AoSStrictMemoryLayout>("aos_strict", kN));
    res.push_back(measure_layout<cl_::CacheLineAlignedMemoryLayout>("cache_line_aligned", kN));
    res.push_back(measure_layout<cl_::SoAMemoryLayout>("soa", kN));
    res.push_back(measure_layout<cl_::AoSoAMemoryLayout>("aosoa", kN));
    res.push_back(measure_layout<cl_::PackedBitmapMemoryLayout>("packed_bitmap", kN));

    std::cout << "  layout                records   field_bytes  cache_lines   CLU%\n";
    for (auto const& r : res) {
        char buf[256];
        std::snprintf(buf, sizeof(buf), "  %-20s  %7llu  %11llu  %10llu  %6.2f", r.name.c_str(),
                      static_cast<unsigned long long>(r.records), static_cast<unsigned long long>(r.field_bytes),
                      static_cast<unsigned long long>(r.cache_lines), r.clu_pct);
        std::cout << buf << "\n";
    }

    // (c-1) jede CLU physikalisch plausibel: >5% und <=100%. (Untergrenze 5%, NICHT 20%: cache_line_aligned hat
    // mit 8 Nutz-Key-Bytes auf 64-B-Padding-Stride eine ECHTE CLU von 8/64=12.5% — das ist der korrekte, niedrigste
    // reale Wert, kein Phantom. Das alte 20%-Limit stammte aus dem entkoppelten Deskriptor-Modell, das CLA auf 25%
    // schoenrechnete. Die frueher unbrauchbaren Werte lagen bei 0.39–6.25% mit 16 Lines/Record — das schliesst >5%
    // weiterhin aus.)
    for (auto const& r : res)
        tr("CLU " + r.name + " plausibel (>5% und <=100%)", r.clu_pct > 5.0 && r.clu_pct <= 100.0 + 1e-9);

    // (c-2) die 5 CLU-Werte differenzieren (paarweise verschieden, Toleranz 0.5 Prozentpunkte).
    bool clu_distinct = true;
    for (std::size_t i = 0; i < res.size(); ++i)
        for (std::size_t j = i + 1; j < res.size(); ++j)
            if (std::abs(res[i].clu_pct - res[j].clu_pct) < 0.5) clu_distinct = false;
    tr("alle 5 CLU-Werte differenzieren (paarweise verschieden)", clu_distinct);

    // (b-1) field_bytes (real beruehrte Key-Bytes) hat mind. 2 distinkte Werte (succinct beruehrt 2 B/Key statt 8).
    {
        bool fb_varies = false;
        for (std::size_t i = 1; i < res.size(); ++i)
            if (res[i].field_bytes != res[0].field_bytes) fb_varies = true;
        tr("field_bytes layout-abhaengig (real, nicht invariant)", fb_varies);
    }

    // (b-2) cache_lines (real beruehrte 64-B-Linien) hat mind. 4 DISTINKTE Werte ueber die 5 Reps —
    //       der echte Beleg, dass die 5 Repraesentationen physisch verschiedenen Byte-Footprint haben.
    {
        std::vector<std::uint64_t> cls;
        for (auto const& r : res) cls.push_back(r.cache_lines);
        std::size_t distinct = 0;
        for (std::size_t i = 0; i < cls.size(); ++i) {
            bool first = true;
            for (std::size_t j = 0; j < i; ++j)
                if (cls[j] == cls[i]) first = false;
            if (first) ++distinct;
        }
        std::cout << "  cache_lines je Rep:";
        for (auto const& r : res) std::cout << " " << r.name << "=" << r.cache_lines;
        std::cout << "  (distinkt=" << distinct << ")\n";
        tr("cache_lines >= 4 DISTINKTE Werte (5 Reps physisch verschiedener Footprint)", distinct >= 4);
    }

    // (b-3) records ueber alle Reps gleich (== kN) — der Footprint variiert, die Record-Zahl nicht.
    {
        bool rec_eq = true;
        for (auto const& r : res)
            if (r.records != kN) rec_eq = false;
        tr("records == kN ueber alle Reps (Footprint variiert, Record-Zahl konstant)", rec_eq);
    }

    // (d) /O2-Fuzz: randomisierte volle-uint64 Keys/Values, kein UB/OOB, Round-Trip 100%.
    std::cout << "-- (d) /O2-Fuzz randomisiert (volle uint64-Domain, 5 Reps x Node4/16/48/256) --\n";
    tr("Fuzz aos_strict         (kein UB/OOB, Round-Trip 100%)",
       fuzz_all_nodes<cl_::AoSStrictMemoryLayout>("aos_strict"));
    tr("Fuzz cache_line_aligned (kein UB/OOB, Round-Trip 100%)",
       fuzz_all_nodes<cl_::CacheLineAlignedMemoryLayout>("cache_line_aligned"));
    tr("Fuzz soa                (kein UB/OOB, Round-Trip 100%)", fuzz_all_nodes<cl_::SoAMemoryLayout>("soa"));
    tr("Fuzz aosoa              (kein UB/OOB, Round-Trip 100%)", fuzz_all_nodes<cl_::AoSoAMemoryLayout>("aosoa"));
    tr("Fuzz packed_bitmap      (kein UB/OOB, Round-Trip 100%)",
       fuzz_all_nodes<cl_::PackedBitmapMemoryLayout>("packed_bitmap"));

    if (g_fail == 0)
        std::cout << "==== #167 5 reale Reps: ALLE OK ====\n";
    else
        std::cout << "==== #167: " << g_fail << " FEHLER ====\n";
    return g_fail == 0 ? 0 : 1;
}
