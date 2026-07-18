// test_axis_sweep_coverage — #18 Golden-Coverage-Gate (2026-07-11), erweitert um #26/GO-5 (2026-07-12).
//
// Task #18 (User): der golden-320-Lauf soll zu einem goldenen Coverage-Test erweitert werden, der JEDE Achse mit
// >=1 Konfiguration beruehrt. Dieser Gate stellt sicher, dass die Achsen-Sweep-Maschinerie (source_catalog.hpp)
// fuer JEDE der 19 Kompositions-Achsen einen materialisierbaren Sweep liefert.
//
// #26/GO-5 (B.4.1-a): AUCH die 4 Basis-Achsen (search_algo / node_type / memory_layout / prefetch) tragen jetzt
// je einen dedizierten Sweep-Katalog (volle Enabled-Liste gegen die Index-0-Baseline). Frueher sweepten sie NUR
// ueber die Basis-320-View — das setzte eine materialisierte, VARIIERENDE Basis voraus und verlor die 4 Achsen
// in jedem Profil mit gepinnter Basis (m3_smoke_coverage). Unter den Default-Enables sind ihre Sweep-ids eine
// TEILMENGE der golden-320 (idempotent); der fruehere "BasisAxesAreNotDeepened"-Test ist entsprechend zum
// Positiv-Gate "BasisAxesHaveDedicatedSweepCatalogs" fortgeschrieben.
// Der Test ist der ECHTE Konsument der SweepCatalog-Instanzen (Anti-Null-Consumer, Doc 20 §I).

#include "source_catalog.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace tlz = comdare::cache_engine::thesis_lazy;

namespace {

// ALLE 19 Kompositions-Achsen — jede braucht einen materialisierbaren Sweep-Katalog (#18 + #26/GO-5).
std::vector<std::string> const kSweepAxes = {
    // 4 Basis-Achsen (#26/GO-5: neu katalogisiert; frueher nur Basis-320-View)
    "search_algo", "node_type", "memory_layout", "prefetch",
    // 4 vertiefte (im Basis-320 gepinnt, eigener Katalog historisch, #168)
    "path_compression", "value_handle", "migration_policy", "filter",
    // 9 uebrige (#18; INC-2c: ohne telemetry / INC-2d: ohne isa)
    "cache_traversal", "mapping", "allocator", "concurrency", "serialization", "index_organization", "io_dispatch",
    "queuing_q1", "queuing_q2"};

} // namespace

// Jede Achse liefert eine nicht-leere Sweep-Quellen-map (mind. die Baseline-Auspraegung) + 18-slotige
// Sweep-Levels (Single-Source mit der Map) -> beruehrbar.
TEST(AxisSweepCoverage, AllAxesMaterialize) {
    for (auto const& ax : kSweepAxes) {
        EXPECT_TRUE(tlz::is_deepened_axis(ax)) << "Achse nicht als sweep-faehig markiert: " << ax;
        auto const m = tlz::axis_sweep_source_map(ax);
        EXPECT_FALSE(m.empty()) << "Sweep-Katalog leer (nicht materialisierbar) fuer Achse: " << ax;
        auto const lv = tlz::axis_sweep_levels(ax);
        EXPECT_EQ(lv.size(), 17u) << "Sweep-Levels nicht 17-slotig fuer Achse: " << ax;
    }
}

// make_all_axis_sweeps_source_map vereinigt ALLE 18 Achsen-Sweeps (disjunkte binary_id-Raeume bis auf
// die idempotente Baseline) -> jede per-Achse-id ist in der Vereinigung enthalten.
TEST(AxisSweepCoverage, UnionCoversAllAxes) {
    auto const all = tlz::make_all_axis_sweeps_source_map();
    EXPECT_FALSE(all.empty());
    for (auto const& ax : kSweepAxes) {
        auto const m = tlz::axis_sweep_source_map(ax);
        for (auto const& [id, src] : m)
            EXPECT_TRUE(all.count(id) == 1) << "Sweep-binary_id fehlt in der Vereinigung (" << ax << "): " << id;
    }
}

// #26/GO-5-Fortschreibung des frueheren "BasisAxesAreNotDeepened"-Gates: die 4 Basis-Achsen sind jetzt
// BEWUSST deepened (eigener Sweep-Katalog fuer Profile mit gepinnter Basis). Ihre Sweep-Map ist sweep-faehig
// (>=2 Auspraegungen) und enthaelt die Index-0-Baseline (idempotenter golden-Key).
TEST(AxisSweepCoverage, BasisAxesHaveDedicatedSweepCatalogs) {
    for (auto const& ax : {"search_algo", "node_type", "memory_layout", "prefetch"}) {
        EXPECT_TRUE(tlz::is_deepened_axis(ax)) << "Basis-Achse ohne Sweep-Katalog: " << ax;
        auto const m = tlz::axis_sweep_source_map(ax);
        EXPECT_GE(m.size(), 2u) << "Basis-Achsen-Sweep nicht sweep-faehig (<2 Auspraegungen): " << ax;
    }
    // Die search_algo-Sweep-Map enthaelt die k_ary-Baseline (Index-0-Wert der Enabled-Liste = golden-Baseline-
    // Segment am id-Anfang) — der idempotente Baseline-Key existiert.
    auto const sa    = tlz::axis_sweep_source_map("search_algo");
    bool       found = false;
    for (auto const& [id, src] : sa)
        if (id.rfind("search_algo=k_ary/", 0) == 0) found = true;
    EXPECT_TRUE(found) << "search_algo-Sweep enthaelt die k_ary-Baseline nicht";
}
