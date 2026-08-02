// test_g3_planer_driven -- G3 / #46b Lagerhaltung, Scheibe I1b.
//
// Der testbare Kern des Planer-getriebenen Baus: slice_view_indices (Fensterung), die DETERMINISMUS-
// Basis (Konkatenation der Fenster == Eingabe -> aktiv==inaktiv gleiche Bau-Menge in Reihenfolge), der
// async SlicePlanner + SlicePlanQueue (Producer-Consumer) und die PresenceFn-Miss-Zahl.
// Lager-TP1(B)/G-A2: die Miss-Erkennung ist seither BAU-FILTER (filter_window_for_build, unten) --
// die "nur informativ"-Doktrin ist damit abgeloest; ohne Praedikat bleibt das Verhalten byte-identisch.

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

// ---------------------------------------------------------------------------
// Lager-TP1(B) / G-A2: filter_window_for_build -- der BAU-FILTER (LEDGER:3397, per-Binary).
// ---------------------------------------------------------------------------
TEST(G3PlanerDriven, FilterBautNurDieFehlendenInFensterReihenfolge) {
    std::vector<std::size_t> const fenster{10, 11, 12, 13, 14, 15};
    bl::PresenceFn const           gerade_im_bestand = [](std::size_t i) { return i % 2 == 0; };

    auto const erg = bl::filter_window_for_build(fenster, gerade_im_bestand);
    EXPECT_EQ(erg.zu_bauen, (std::vector<std::size_t>{11, 13, 15}))
        << "Jedes fehlende Binary EINZELN erkannt, Reihenfolge des Fensters erhalten.";
    EXPECT_EQ(erg.bestand_uebersprungen, 3u);
    EXPECT_EQ(erg.zu_bauen.size() + erg.bestand_uebersprungen, fenster.size()) << "Buchungs-Invariante.";
}

TEST(G3PlanerDriven, OhnePraedikatBleibtDasVolleFenster) {
    // Der byte-identische Anker des Ist-Pfades: kein Praedikat -> alles fehlt -> volles Fenster,
    // 0 uebersprungen. Genau daran haengt die Golden-Neutralitaet des inaktiven Bestandslogs.
    std::vector<std::size_t> const fenster{3, 1, 4, 1, 5};
    auto const                     erg = bl::filter_window_for_build(fenster, bl::PresenceFn{});
    EXPECT_EQ(erg.zu_bauen, fenster);
    EXPECT_EQ(erg.bestand_uebersprungen, 0u);
}

TEST(G3PlanerDriven, FilterRandfaelleVollUndLeer) {
    bl::PresenceFn const alles_da = [](std::size_t) { return true; };
    auto const           voll     = bl::filter_window_for_build({7, 8, 9}, alles_da);
    EXPECT_TRUE(voll.zu_bauen.empty()) << "Ganz im Bestand -> nichts zu bauen (der Slice wird trotzdem "
                                          "reserviert und Done -- der Claim dokumentiert das Fenster).";
    EXPECT_EQ(voll.bestand_uebersprungen, 3u);
    auto const leer = bl::filter_window_for_build({}, alles_da);
    EXPECT_TRUE(leer.zu_bauen.empty());
    EXPECT_EQ(leer.bestand_uebersprungen, 0u);
}

TEST(G3PlanerDriven, PlannerMissingCountIstDieFilterWahrheit) {
    // EINE Miss-Wahrheit: der missing_count des async Planers ist per Konstruktion die zu_bauen-Zahl
    // des Consumers (beide laufen ueber detect_missing_in_window).
    bl::PresenceFn const praedikat = [](std::size_t i) { return i < 2; };
    bl::SlicePlanQueue   q;
    {
        bl::SlicePlanner planner(q, {0, 1, 2, 3}, 8, praedikat);
        planner.join();
    }
    auto const plan = q.pop();
    ASSERT_TRUE(plan.has_value());
    EXPECT_EQ(plan->missing_count, bl::filter_window_for_build(plan->view_indices, praedikat).zu_bauen.size());
    EXPECT_EQ(plan->missing_count, 2u);
}
