// #214 — GoF-Iterator-Organ: geordneter Range-Scan (scan_range) auf den vier Flach-Store-Traversal-Organen.
//
// Beweist (Audit K4 / Meta-Lehre #3 / YCSB-E):
//   (a) scan_range liefert ab start_key bis max_count Records IN KEY-REIHENFOLGE — exakt das std::map-Orakel
//       (gleiche Datenmenge → gleiche geordnete Ausgabe), für ALLE vier Organe.
//   (b) Das UNSORTIERTE LinearScan-Organ liefert dieselbe korrekte geordnete Ausgabe wie das sortierte
//       SortedBinary-Organ — verschiedene Organ-Pfade (O(n) vs O(log n + scan_len)), gleiche Scan-SEMANTIK
//       (Achsen-Austauschbarkeit; der Apparat verfälscht das Ergebnis NICHT).
//   (c) Edge-Cases: leerer Store, start_key vor/innerhalb/nach dem Schlüsselraum, max_count > Füllung, max_count=0.
//   (d) Die ObservableComposedSearch-Hülle reicht scan_range durch und verändert WEDER Daten NOCH Statistik
//       (reines Lesen — kein lookup-/insert-Zähler-Effekt).
//
// Ersetzt-Bezug: der frühere abi_adapter::tier_scan kopierte je Scan-Op den ganzen Store (save_state) + std::sort
// (O(n log n)); der GoF-Iterator hier ist der O(log n + scan_len)-Kern, den tier_scan nun uniform aufruft.

#include <axes/lookup/composable/composable_search.hpp>
#include <axes/lookup/composable/interpolation_traversal_organ.hpp>
#include <axes/lookup/composable/galloping_traversal_organ.hpp>
#include <axes/lookup/composable/k_ary_traversal_organ.hpp> // #188-4a
#include <axes/lookup/composable/observable_composed_search.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <random>
#include <utility>
#include <vector>

namespace cmp = ::comdare::cache_engine::lookup::composable;

namespace {

using KvList = std::vector<std::pair<std::uint64_t, std::uint64_t>>;

// Orakel: was ein geordneter Range-Scan ab start über max Records liefern MUSS (std::map ist key-sortiert).
KvList oracle_scan(std::map<std::uint64_t, std::uint64_t> const& ref, std::uint64_t start, std::size_t max) {
    KvList out;
    for (auto it = ref.lower_bound(start); it != ref.end() && out.size() < max; ++it)
        out.emplace_back(it->first, it->second);
    return out;
}

// Baut eine ComposedSearch<Traversal,RawSlotStore> aus `data` und sammelt das scan_range-Ergebnis.
template <class Traversal>
KvList collect_scan(std::map<std::uint64_t, std::uint64_t> const& data, std::uint64_t start, std::size_t max) {
    cmp::ComposedSearch<Traversal, cmp::RawSlotStore> s;
    for (auto const& kv : data) s.insert(kv.first, kv.second);
    KvList            got;
    std::size_t const n =
        s.scan_range(start, max, [&got](std::uint64_t k, std::uint64_t v) { got.emplace_back(k, v); });
    EXPECT_EQ(n, got.size()); // Rückgabe == Zahl der gesink'ten Records
    return got;
}

template <class Traversal>
void expect_matches_oracle(std::map<std::uint64_t, std::uint64_t> const& data) {
    std::uint64_t const starts[] = {0u, 1u, 5u, 50u, 500u, 5000u, 99999u};
    std::size_t const   maxes[]  = {0u, 1u, 3u, 10u, 1000u};
    for (auto start : starts)
        for (auto mx : maxes) {
            KvList const got  = collect_scan<Traversal>(data, start, mx);
            KvList const want = oracle_scan(data, start, mx);
            ASSERT_EQ(got.size(), want.size()) << "start=" << start << " max=" << mx;
            for (std::size_t i = 0; i < got.size(); ++i) {
                EXPECT_EQ(got[i].first, want[i].first) << "start=" << start << " max=" << mx << " i=" << i;
                EXPECT_EQ(got[i].second, want[i].second) << "start=" << start << " max=" << mx << " i=" << i;
            }
            for (std::size_t i = 1; i < got.size(); ++i) // strikt aufsteigend nach Key (geordneter Scan)
                EXPECT_LT(got[i - 1].first, got[i].first);
        }
}

std::map<std::uint64_t, std::uint64_t> make_data() {
    std::map<std::uint64_t, std::uint64_t> d;
    std::mt19937_64                        rng{12345u};
    for (int i = 0; i < 200; ++i) {
        std::uint64_t const k = rng() % 10000u;
        d[k]                  = k * 7u + 1u;
    }
    return d;
}

} // namespace

TEST(V41ScanRangeOrgan, SortedBinaryMatchesOracle) { expect_matches_oracle<cmp::SortedBinaryTraversal>(make_data()); }
TEST(V41ScanRangeOrgan, LinearScanMatchesOracle) { expect_matches_oracle<cmp::LinearScanTraversal>(make_data()); }
TEST(V41ScanRangeOrgan, InterpolationMatchesOracle) {
    expect_matches_oracle<cmp::InterpolationTraversalOrgan>(make_data());
}
TEST(V41ScanRangeOrgan, GallopingMatchesOracle) { expect_matches_oracle<cmp::GallopingTraversalOrgan>(make_data()); }
TEST(V41ScanRangeOrgan, KAryMatchesOracle) {
    expect_matches_oracle<cmp::KAryTraversal<4u>>(make_data());
} // #188-4a (compile-time Default-Arity 4)

// Meta-Lehre #3: das UNSORTIERTE LinearScan-Organ und das sortierte SortedBinary-Organ durchlaufen verschiedene
// Organ-Pfade, liefern aber DIESELBE korrekte geordnete Scan-Ausgabe (der Apparat verfälscht das Ergebnis nicht).
TEST(V41ScanRangeOrgan, LinearScanEqualsSortedBinaryOutput) {
    auto const          d        = make_data();
    std::uint64_t const starts[] = {0u, 37u, 5000u};
    for (auto start : starts) {
        KvList const a = collect_scan<cmp::LinearScanTraversal>(d, start, 25u);
        KvList const b = collect_scan<cmp::SortedBinaryTraversal>(d, start, 25u);
        EXPECT_EQ(a, b) << "start=" << start;
    }
}

// Leerer Store → 0 Records, kein OOB.
TEST(V41ScanRangeOrgan, EmptyStoreYieldsNothing) {
    std::map<std::uint64_t, std::uint64_t> const empty;
    EXPECT_TRUE(collect_scan<cmp::SortedBinaryTraversal>(empty, 0u, 10u).empty());
    EXPECT_TRUE(collect_scan<cmp::LinearScanTraversal>(empty, 0u, 10u).empty());
}

// Die ObservableComposedSearch-Hülle reicht scan_range durch UND verändert die Statistik nicht (reines Lesen).
TEST(V41ScanRangeOrgan, ObservableWrapperScanCorrectAndStatNeutral) {
    cmp::ObservableComposedSearch<cmp::SortedBinaryTraversal, cmp::RawSlotStore> s;
    for (std::uint64_t k = 1; k <= 20; ++k) s.insert(k, k * 3u);
    std::size_t const occ_before = s.occupied_count();
#ifdef COMDARE_CE_ENABLE_STATISTICS
    auto const before = s.statistics();
#endif
    std::uint64_t     sum = 0;
    std::size_t const cnt = s.scan_range(5u, 7u, [&sum](std::uint64_t, std::uint64_t v) { sum += v; });
    EXPECT_EQ(cnt, 7u); // Keys 5..11
    EXPECT_EQ(sum, (5u + 6u + 7u + 8u + 9u + 10u + 11u) * 3u);
    EXPECT_EQ(s.occupied_count(), occ_before); // Scan verändert die Daten nicht
#ifdef COMDARE_CE_ENABLE_STATISTICS
    auto const after = s.statistics();
    EXPECT_EQ(before.total_lookup_count, after.total_lookup_count); // Scan ist KEIN lookup
    EXPECT_EQ(before.total_insert_count, after.total_insert_count); // und kein insert
#endif
}
