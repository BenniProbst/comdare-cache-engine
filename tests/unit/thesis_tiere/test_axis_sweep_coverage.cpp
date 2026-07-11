// test_axis_sweep_coverage — #18 Golden-Coverage-Gate (2026-07-11).
//
// Task #18 (User): der golden-320-Lauf soll zu einem goldenen Coverage-Test erweitert werden, der JEDE Achse mit
// >=1 Konfiguration beruehrt. Dieser Gate stellt sicher, dass die Achsen-Sweep-Maschinerie (source_catalog.hpp)
// fuer JEDE der 19 Kompositions-Achsen einen materialisierbaren Sweep liefert:
//   • 4 Basis-Achsen  (search_algo / node_type / memory_layout / prefetch) sweepen ueber die Basis-320-View
//     (voll im 4x4x5x4-Katalog vertreten) -> is_deepened_axis == false, KEIN eigener Sweep-Katalog noetig.
//   • 15 nicht-Basis-Achsen (4 vertiefte + 11 uebrige) brauchen je einen dedizierten Sweep-Katalog
//     (axis_sweep_source_map != leer) -> is_deepened_axis == true.
// Der Test ist der ECHTE Konsument der 11 neuen SweepCatalog-Instanzen (Anti-Null-Consumer, Doc 20 §I).

#include "source_catalog.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace tlz = comdare::cache_engine::thesis_lazy;

namespace {

// Die 15 nicht-Basis-Achsen (4 vertiefte + 11 uebrige) — jede braucht einen materialisierbaren Sweep-Katalog.
std::vector<std::string> const kNonBasisAxes = {
    // 4 vertiefte (im Basis-320 gepinnt, eigener Katalog historisch)
    "path_compression", "value_handle", "migration_policy", "filter",
    // 11 uebrige (#18: neu ergaenzt)
    "cache_traversal", "mapping", "allocator", "concurrency", "serialization", "telemetry", "isa", "index_organization",
    "io_dispatch", "queuing_q1", "queuing_q2"};

} // namespace

// Jede nicht-Basis-Achse liefert eine nicht-leere Sweep-Quellen-map (mind. die Baseline-Auspraegung) -> beruehrbar.
TEST(AxisSweepCoverage, AllNonBasisAxesMaterialize) {
    for (auto const& ax : kNonBasisAxes) {
        EXPECT_TRUE(tlz::is_deepened_axis(ax)) << "Achse nicht als sweep-faehig markiert: " << ax;
        auto const m = tlz::axis_sweep_source_map(ax);
        EXPECT_FALSE(m.empty()) << "Sweep-Katalog leer (nicht materialisierbar) fuer Achse: " << ax;
    }
}

// make_all_axis_sweeps_source_map vereinigt ALLE 15 nicht-Basis-Achsen-Sweeps (disjunkte binary_id-Raeume bis auf
// die idempotente Baseline) -> muss mind. so viele Eintraege wie die groesste Einzel-map tragen.
TEST(AxisSweepCoverage, UnionCoversAllNonBasisAxes) {
    auto const all = tlz::make_all_axis_sweeps_source_map();
    EXPECT_FALSE(all.empty());
    for (auto const& ax : kNonBasisAxes) {
        auto const m = tlz::axis_sweep_source_map(ax);
        for (auto const& [id, src] : m)
            EXPECT_TRUE(all.count(id) == 1) << "Sweep-binary_id fehlt in der Vereinigung (" << ax << "): " << id;
    }
}

// Die 4 Basis-Achsen sweepen ueber die Basis-320-View (kein eigener Katalog) -> is_deepened_axis == false.
TEST(AxisSweepCoverage, BasisAxesAreNotDeepened) {
    for (auto const& ax : {"search_algo", "node_type", "memory_layout", "prefetch"})
        EXPECT_FALSE(tlz::is_deepened_axis(ax)) << "Basis-Achse faelschlich als vertieft: " << ax;
}
