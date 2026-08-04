// test_e24_c6_axis_row_writer -- E-24 C6 (b-Teil, ABI-EREIGNIS-SERIE).
//
// GEGENSTAND: write_genus_axis_row() (libs/cache_engine/anatomy/genus_axis_row_writer.hpp) -- der EINE
// Schreiber, der einen Achsen-Snapshot in eine 8-Spalten-Zeile der Gattungs-Wire-Form projiziert.
//
//   (A) E1-FIXPUNKT   -- to_axis_milli(): x1000 kaufmaennisch gerundet; negativ/NaN -> ehrliche 0.
//   (B) BELEGUNGEN    -- je C6-V-Form die REALE Spalten-Belegung an ECHTEN Werten geprueft (nicht an
//                        einer nachgebauten Struct-Attrappe: die Werte kommen aus den echten Huellen,
//                        getrieben ueber die echten Gattungs-Anatomien).
//   (C) DOUBLE-NAHT   -- AllocationStatistics: die zwei double-Felder landen als _milli auf dem Draht.
//                        Ohne diesen Zweig wiederholte der Gattungs-Wire den SA-double-Drop.
//   (D) D2-DOKTRIN    -- EmptyAxisSnapshot und eine unbekannte Form schreiben NICHTS und melden 0
//                        (= "keine Belegung"), statt eine genullte Zeile als Messwert auszugeben.
//   (E) DISJUNKTHEIT  -- die Erkennungs-Zweige sind paarweise disjunkt: jede Form trifft GENAU einen.
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes.

#include "anatomy/genus_axis_row_writer.hpp"

#include "anatomy/adapter_anatomy.hpp"            // DequeInner/VectorInner (echte Substrate)
#include "anatomy/growth_policy_observable.hpp"   // GrowthStatistics + ObservableGrowth
#include "anatomy/inner_container_observable.hpp" // InnerContainerStatistics + Huelle
#include "anatomy/view_policies_observable.hpp"   // Extent/Layout/AccessorStatistics + Huellen
#include "axes/alloc/concepts/axis_06_allocator_cache_engine_permutation_concept.hpp" // AllocationStatistics

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen gleichnamiger Fixture-Typen
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace cea = comdare::cache_engine::anatomy;

static int g_fail = 0;

template <class A, class B>
static void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

static void check_true(char const* what, bool cond) {
    std::cout << (cond ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!cond) ++g_fail;
}

/// Eine Snapshot-Form, die der Schreiber BEWUSST nicht kennt (deklarierte Luecke, s. Header-Abgrenzung).
struct UnbekannterSnapshot {
    std::uint64_t irgendwas = 7;
};

// (E) Disjunktheit + Abdeckung compile-time.
static_assert(cea::GenusAxisRowWritable<cea::GrowthStatistics>);
static_assert(cea::GenusAxisRowWritable<cea::InnerContainerStatistics>);
static_assert(cea::GenusAxisRowWritable<cea::ExtentStatistics>);
static_assert(cea::GenusAxisRowWritable<cea::LayoutStatistics>);
static_assert(cea::GenusAxisRowWritable<cea::AccessorStatistics>);
static_assert(cea::GenusAxisRowWritable<comdare::cache_engine::alloc::concepts::AllocationStatistics>);
static_assert(!cea::GenusAxisRowWritable<cea::EmptyAxisSnapshot>);
static_assert(!cea::GenusAxisRowWritable<UnbekannterSnapshot>);

// (A) E1-Fixpunkt compile-time.
static_assert(cea::to_axis_milli(1.5) == 1500);
static_assert(cea::to_axis_milli(0.0) == 0);
static_assert(cea::to_axis_milli(-0.25) == 0);  // negativ -> ehrliche 0
static_assert(cea::to_axis_milli(0.0005) == 1); // kaufmaennisch gerundet (0.5 -> 1)

} // namespace

int main() {
    std::cout << "=== E-24 C6 (b) -- write_genus_axis_row(): Spalten-Belegung + E1-Milli-Fixpunkt ===\n";

    std::uint64_t row[cea::kGenusAxisFieldCount] = {};

    std::cout << "\n[A] E1-Fixpunkt (Laufzeit-Probe)\n";
    check_eq("to_axis_milli(0.375)", cea::to_axis_milli(0.375), std::uint64_t{375});
    check_eq("to_axis_milli(2.0)", cea::to_axis_milli(2.0), std::uint64_t{2000});

    std::cout << "\n[B1] GrowthStatistics aus der ECHTEN Huelle (ExactGrowth-artig ueber DoublingGrowth)\n";
    {
        cea::ObservableGrowth<cea::DoublingGrowth> g{};
        // Zwei REALE Wachstums-Ereignisse treiben (granted > current).
        std::size_t cap = 0;
        cap             = g.next_capacity(cap, 1);
        cap             = g.next_capacity(cap, cap + 1);
        auto const gs   = g.statistics();
        for (auto& c : row) c = 0;
        std::size_t const written = cea::write_genus_axis_row(row, gs);
        check_eq("geschriebene Spalten", written, std::size_t{7});
        check_eq("row[0] == growth_events", row[0], gs.growth_events);
        check_eq("row[5] == final_capacity", row[5], gs.final_capacity);
        check_eq("row[6] == peak_slack", row[6], gs.peak_slack);
        check_true("growth_events > 0 (real getrieben)", gs.growth_events > 0);
        check_eq("row[7] bleibt fuer growth_factor_milli frei", row[7], std::uint64_t{0});
    }

    std::cout << "\n[B2] InnerContainerStatistics aus der ECHTEN Huelle (VectorInner: O(n)-Union-Slot)\n";
    {
        cea::ObservableInnerContainer<cea::VectorInner<>> ic{};
        ic.push_back(1);
        ic.push_back(2);
        ic.push_back(3);
        (void)ic.front();
        ic.pop_front(); // verschiebt real 2 Elemente
        ic.pop_front(); // verschiebt real 1 Element
        auto const is = ic.statistics();
        for (auto& c : row) c = 0;
        std::size_t const written = cea::write_genus_axis_row(row, is);
        check_eq("geschriebene Spalten", written, std::size_t{8});
        check_eq("row[0] == push_count", row[0], std::uint64_t{3});
        check_eq("row[1] == pop_count", row[1], std::uint64_t{2});
        check_eq("row[5] == peak_occupancy", row[5], std::uint64_t{3});
        check_eq("row[7] == substrate_ops (2+1 verschobene Elemente)", row[7], std::uint64_t{3});
    }

    std::cout << "\n[B3] Extent/Layout/AccessorStatistics aus den ECHTEN Huellen\n";
    {
        cea::ObservableExtent<cea::DynamicExtent> ex{};
        ex.note_bind(4);
        ex.note_bind(8); // zweites Binden == Re-Binding
        ex.note_bounds_check(9, 8);
        auto const es = ex.statistics();
        for (auto& c : row) c = 0;
        check_eq("Extent: geschriebene Spalten", cea::write_genus_axis_row(row, es), std::size_t{3});
        check_eq("row[0] == bounds_checks_performed", row[0], std::uint64_t{1});
        check_eq("row[1] == oob_rejects", row[1], std::uint64_t{1});
        check_eq("row[2] == rebind_count", row[2], std::uint64_t{1});

        cea::ObservableLayout<cea::LayoutRight> lay{};
        (void)lay.index_of(0);
        (void)lay.index_of(1);
        auto const ls = lay.statistics();
        for (auto& c : row) c = 0;
        check_eq("Layout: geschriebene Spalten", cea::write_genus_axis_row(row, ls), std::size_t{3});
        check_eq("row[0] == index_translations", row[0], std::uint64_t{2});

        cea::ObservableAccessor<cea::DefaultAccessor> acc{};
        std::uint64_t const                           buf[3] = {10, 11, 12};
        (void)acc.access(buf, 1);
        auto const as = acc.statistics();
        for (auto& c : row) c = 0;
        check_eq("Accessor: geschriebene Spalten", cea::write_genus_axis_row(row, as), std::size_t{3});
        check_eq("row[0] == access_count", row[0], std::uint64_t{1});
        check_eq("row[2] == conversion_ops (ehrlich 0)", row[2], std::uint64_t{0});
    }

    std::cout << "\n[C] AllocationStatistics -- die DOUBLE-NAHT als E1-Milli auf dem Draht\n";
    {
        comdare::cache_engine::alloc::concepts::AllocationStatistics alloc{};
        alloc.total_bytes_allocated  = 4096;
        alloc.total_bytes_in_use     = 3072;
        alloc.allocation_count       = 12;
        alloc.deallocation_count     = 5;
        alloc.failure_count          = 1;
        alloc.external_fragmentation = 0.25;  // 25 %
        alloc.internal_fragmentation = 0.125; // 12,5 %
        for (auto& c : row) c = 0;
        std::size_t const written = cea::write_genus_axis_row(row, alloc);
        check_eq("geschriebene Spalten", written, std::size_t{7});
        check_eq("row[0] == total_bytes_allocated", row[0], std::uint64_t{4096});
        check_eq("row[4] == failure_count", row[4], std::uint64_t{1});
        check_eq("row[5] == external_fragmentation als _milli", row[5], std::uint64_t{250});
        check_eq("row[6] == internal_fragmentation als _milli", row[6], std::uint64_t{125});
        check_true("die double-Werte gehen NICHT verloren (SA-Drop nicht wiederholt)", row[5] != 0 && row[6] != 0);
    }

    std::cout << "\n[D] D2-Doktrin: keine Belegung -> 0 geschriebene Spalten, Zeile unberuehrt\n";
    {
        for (auto& c : row) c = 0xDEAD;
        cea::EmptyAxisSnapshot const empty{};
        check_eq("EmptyAxisSnapshot: geschriebene Spalten", cea::write_genus_axis_row(row, empty), std::size_t{0});
        check_eq("Zeile bleibt unangetastet (kein Phantom-Wert)", row[0], std::uint64_t{0xDEAD});

        UnbekannterSnapshot const unbekannt{};
        check_eq("unbekannte Form: geschriebene Spalten", cea::write_genus_axis_row(row, unbekannt), std::size_t{0});
    }

    std::cout << "\n=== " << (g_fail == 0 ? "ALLE PRUEFUNGEN GRUEN" : "FEHLER") << " (" << g_fail << ") ===\n";
    return g_fail == 0 ? 0 : 1;
}
