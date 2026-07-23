// test_g3_planer_driven -- G3 / #46b Lagerhaltung, Scheibe I1b.
//
// Der testbare Kern des Planer-getriebenen Baus: slice_view_indices (Fensterung), die DETERMINISMUS-
// Basis (Konkatenation der Fenster == Eingabe -> aktiv==inaktiv gleiche Bau-Menge in Reihenfolge), der
// async SlicePlanner + SlicePlanQueue (Producer-Consumer) und die PresenceFn-Miss-Zahl (nur informativ).

#include "bestandslog/planer_driven_build.hpp"

#include <cstddef>
#include <optional>
#include <vector>

#include <gtest/gtest.h>

namespace bl = comdare::cache_engine::builder::bestandslog;

namespace {

// Drain: alle Plaene aus dem async Producer in Reihenfolge einsammeln.
std::vector<bl::BuildSlicePlan> drain(bl::SlicePlanQueue& q) {
    std::vector<bl::BuildSlicePlan> out;
    while (auto p = q.pop()) out.push_back(std::move(*p));
    return out;
}

// Konkatenation der Fenster-view_indices.
std::vector<std::size_t> concat(std::vector<bl::BuildSlicePlan> const& plans) {
    std::vector<std::size_t> out;
    for (auto const& p : plans)
        for (std::size_t i : p.view_indices) out.push_back(i);
    return out;
}

} // namespace

// ---------------------------------------------------------------------------
// slice_view_indices: Fensterung + Determinismus-Basis.
// ---------------------------------------------------------------------------
TEST(G3PlanerDriven, SliceWindows) {
    std::vector<std::size_t> const idx{0, 1, 2, 3, 4};
    auto const                     s = bl::slice_view_indices(idx, 2);
    ASSERT_EQ(s.size(), 3u);
    EXPECT_EQ(s[0], (std::vector<std::size_t>{0, 1}));
    EXPECT_EQ(s[1], (std::vector<std::size_t>{2, 3}));
    EXPECT_EQ(s[2], (std::vector<std::size_t>{4}));
}

TEST(G3PlanerDriven, SliceConcatenationEqualsInput) {
    // Determinismus-Kern: fuer beliebige Groesse + Korn ergibt die Konkatenation EXAKT die Eingabe.
    for (std::size_t n : {0u, 1u, 5u, 100u, 4096u, 5000u}) {
        std::vector<std::size_t> idx(n);
        for (std::size_t i = 0; i < n; ++i) idx[i] = i * 7 + 3; // nicht-triviale, geordnete Indizes
        for (std::size_t grain : {1u, 2u, 3u, 4096u}) {
            auto const               s = bl::slice_view_indices(idx, grain);
            std::vector<std::size_t> flat;
            for (auto const& w : s)
                for (std::size_t v : w) flat.push_back(v);
            EXPECT_EQ(flat, idx) << "n=" << n << " grain=" << grain;
        }
    }
}

TEST(G3PlanerDriven, SliceGrainZeroDefaults) {
    std::vector<std::size_t> const idx{1, 2, 3};
    auto const                     s = bl::slice_view_indices(idx, 0); // 0 -> Default kBuildSliceGrain
    ASSERT_EQ(s.size(), 1u);
    EXPECT_EQ(s[0], idx);
}

// ---------------------------------------------------------------------------
// Async Producer + Queue: deckt alle Indizes in Reihenfolge ab; Queue-Semantik.
// ---------------------------------------------------------------------------
TEST(G3PlanerDriven, PlannerCoversAllInOrderNoPredicate) {
    std::vector<std::size_t> idx;
    for (std::size_t i = 0; i < 10; ++i) idx.push_back(i);

    bl::SlicePlanQueue q;
    {
        bl::SlicePlanner planner(q, idx, /*grain*/ 3, /*present*/ {}); // kein Praedikat
        // dtor joined; die Queue ist danach geschlossen + befuellt.
    }
    auto const plans = drain(q);
    ASSERT_EQ(plans.size(), 4u); // 10/3 -> 4 Fenster
    EXPECT_EQ(concat(plans), idx);
    // ohne Praedikat: alles fehlt -> missing_count == Fenstergroesse.
    for (auto const& p : plans) EXPECT_EQ(p.missing_count, p.view_indices.size());
}

TEST(G3PlanerDriven, PlannerMissingCountFromPredicate) {
    std::vector<std::size_t> idx;
    for (std::size_t i = 0; i < 8; ++i) idx.push_back(i);

    bl::SlicePlanQueue q;
    {
        // present = gerade Indizes vorhanden -> fehlend = ungerade.
        bl::SlicePlanner planner(q, idx, 4, [](std::size_t i) { return (i % 2) == 0; });
    }
    auto const plans = drain(q);
    ASSERT_EQ(plans.size(), 2u); // 8/4
    EXPECT_EQ(concat(plans), idx);
    // je 4er-Fenster: 2 ungerade fehlen.
    for (auto const& p : plans) EXPECT_EQ(p.missing_count, 2u);
}

TEST(G3PlanerDriven, QueueClosedEmptyReturnsNullopt) {
    bl::SlicePlanQueue q;
    q.close();
    EXPECT_FALSE(q.pop().has_value());
}

TEST(G3PlanerDriven, EmptyIndicesProducesNoPlans) {
    bl::SlicePlanQueue q;
    { bl::SlicePlanner planner(q, std::vector<std::size_t>{}, 4, {}); }
    EXPECT_TRUE(drain(q).empty());
}
