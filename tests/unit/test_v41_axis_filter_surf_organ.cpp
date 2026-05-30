// V41 Umstufung-A s4 (Task #43) — SuRF-Filter-Organ-Property-Tests (axis_filter, Teil 2 der Dual-Sezierung).
//
// SuRF ist ein approximativer Range-Filter (Filter-Gattung): may-contain (bool) mit false positives aber
// GARANTIERT KEINEN false negatives. Daher NICHT std::map-Gleichheit (das traegt die Map-Schale in axis_03a),
// sondern die FILTER-PROPERTY:
//   P1 NO-FALSE-NEGATIVE (HART): jeder gebaute Key wird gefunden (das einzige eingefrorene Invariant).
//   P2 FALSE-POSITIVE-RATE (gemessen): S1 exakte Base -> FP-Rate == 0 (Ground-Truth-Anker fuer S2).
//   P3 RANGE-MAY-EXIST: jeder Range mit >=1 gebautem Key liefert true.

#include <gtest/gtest.h>

#include <topics/filter/axis_filter/composable/exact_prefix_filter_organ.hpp>

#include <cstdint>
#include <set>
#include <vector>

namespace fcmp = ::comdare::cache_engine::filter::axis_filter::composable;
using SurfExactFilter = fcmp::ComposedExactSurfFilter<fcmp::ExactPrefixFilterQuery, fcmp::ExactPrefixFilterStore>;

namespace {
// Baut einen Multi-Byte-Schluesselraum (spannt mehrere Bytes -> nichttriviale Trie-Struktur in S2).
std::vector<std::uint64_t> build_key_set(std::set<std::uint64_t>& ground_truth) {
    std::vector<std::uint64_t> keys;
    for (std::uint64_t i = 0; i < 800; ++i) {
        std::uint64_t const k = (i * 2654435761u) % 100000u;   // verstreute uint64-Keys
        ground_truth.insert(k);
    }
    keys.assign(ground_truth.begin(), ground_truth.end());      // sortiert (std::set)
    return keys;
}
}  // namespace

// P1: NO-FALSE-NEGATIVE — jeder gebaute Key MUSS gefunden werden (hart, das einzige Filter-Invariant).
TEST(SurfFilterOrgan, NoFalseNegative) {
    std::set<std::uint64_t> gt;
    auto const keys = build_key_set(gt);
    SurfExactFilter f;
    f.build_from_sorted_keys(keys);
    ASSERT_EQ(f.key_count(), gt.size());
    for (std::uint64_t const k : gt) ASSERT_TRUE(f.contains(k)) << "FALSE NEGATIVE fuer gebauten Key " << k;
}

// P2: FALSE-POSITIVE-RATE — die S1-Correctness-Base ist EXAKT -> FP-Rate == 0 (Ground-Truth-Anker fuer S2).
TEST(SurfFilterOrgan, ExactBaseHasZeroFalsePositives) {
    std::set<std::uint64_t> gt;
    auto const keys = build_key_set(gt);
    SurfExactFilter f;
    f.build_from_sorted_keys(keys);
    std::size_t fp = 0, probed = 0;
    for (std::uint64_t q = 0; q <= 100000u; ++q) {
        bool const in_gt = gt.count(q) != 0u;
        bool const c = f.contains(q);
        if (!in_gt) { ++probed; if (c) ++fp; }
        else        { ASSERT_TRUE(c) << "no-FN-Verletzung q=" << q; }   // doppelte no-FN-Absicherung
    }
    EXPECT_EQ(fp, 0u) << "exakte Base darf KEINE false positives haben (" << fp << "/" << probed << ")";
}

// P3: RANGE-MAY-EXIST — jeder Range mit >=1 Key liefert true; exakte Base: leere Ranges liefern false.
TEST(SurfFilterOrgan, RangeMayExistExactAgainstStdSet) {
    std::set<std::uint64_t> gt;
    auto const keys = build_key_set(gt);
    SurfExactFilter f;
    f.build_from_sorted_keys(keys);
    for (std::uint64_t lo = 0; lo <= 2000u; lo += 7u) {
        for (std::uint64_t w : {std::uint64_t{0}, std::uint64_t{1}, std::uint64_t{50}, std::uint64_t{500}}) {
            std::uint64_t const hi = lo + w;
            auto it = gt.lower_bound(lo);
            bool const truth = (it != gt.end() && *it <= hi);   // >=1 Key in [lo,hi]
            EXPECT_EQ(f.range_may_exist(lo, hi), truth) << "range [" << lo << "," << hi << "]";
        }
    }
}

// Observer + clear-Basics.
TEST(SurfFilterOrgan, ObserversAndClear) {
    std::set<std::uint64_t> gt;
    auto const keys = build_key_set(gt);
    SurfExactFilter f;
    f.build_from_sorted_keys(keys);
    EXPECT_EQ(f.key_count(), gt.size());
    EXPECT_EQ(f.bit_size(), gt.size() * 64u);
    EXPECT_DOUBLE_EQ(f.bits_per_key(), 64.0);
    f.clear();
    EXPECT_EQ(f.key_count(), 0u);
    EXPECT_FALSE(f.contains(*gt.begin()));
}
