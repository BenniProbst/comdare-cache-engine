// test_e8_bytes_in_use_peak_demo.cpp -- W2 R-13 (E-8): bytes_in_use_peak WAHR machen im
// Demo-Treiber (r3 C5, "zwei unwahre Messgroessen vor dem ersten Batch").
//
// DER BEFUND AM OBJEKT (Schreiber-Zensus, 2026-08-21): experiment_demo.hpp befuellte
// `record.bytes_in_use_peak` mit `stats.total_bytes_allocated` -- der KUMULATIVEN
// Gesamt-Allokation. Das ist keine Fehl-Etikettierung wie am Snapshot-POD (dort traegt das Feld
// wenigstens einen ECHTEN Momentanwert und eine B7-Deklaration), sondern eine glatt falsche
// Zahl: jede Freigabe laesst den wahren Peak hinter der Gesamt-Summe zurueck, der Konsument
// result_aggregator rechnet daraus memory_ratio-Vergleiche.
//
// WARUM WAHR MACHEN moeglich ist (statt nur deklarieren): AllocationStatistics traegt zwar kein
// Peak-Feld (i_allocation_strategy.hpp), aber run_single_experiment treibt JEDEN raw_allocate/
// raw_deallocate SELBST -- der wahre Peak der in Benutzung befindlichen Nutzlast-Bytes ist das
// Maximum des treibereigenen live-Zaehlers ueber den Op-Strom. Kein ABI-Ereignis, kein
// CSV-Schema-Ereignis (gleiche Spalte, wahrer Wert).
//
// KOEDER-DOKTRIN (T-1): dieser Test wurde VOR dem Fix geschrieben und lief am unfixierten
// Objekt ROT (peak == total, literal im Strang-Ergebnis dokumentiert) -- der Riss war die
// Luege, nicht der Test.
//
// NENNER FREMD (T-4): der SOLL-Peak wird HIER aus dem handgeschriebenen Op-Strom hergeleitet
// (Insert/Erase-Zaehlung), nicht aus dem Pruefling zurueckgelesen: 3x Insert, 2x Erase,
// 1x Insert => live-Verlauf 1,2,3,2,1,2 => Peak = 3 Nutzlasten; Gesamt-Allokation = 4 Nutzlasten.

#include <comdare/experiment/experiment_demo.hpp>
#include <comdare/workload_generator/workload_generator.hpp>

#include <cache_engine/allocators/families/a01_hoard/hoard_adapter.hpp>

#include <gtest/gtest.h>

#include <span>
#include <vector>

namespace wg   = comdare::workload_generator;
namespace expt = comdare::experiment;
namespace fam  = comdare::cache_engine::allocator::families;

namespace {
constexpr std::uint32_t kValueSize = 128; // Nutzlast je Insert (Byte)

std::vector<wg::Operation> peak_koeder_ops() {
    // live:            1       2       3       2       1       2
    std::vector<wg::Operation> ops;
    ops.push_back({wg::OperationKind::Insert, 0, 0});
    ops.push_back({wg::OperationKind::Insert, 1, 0});
    ops.push_back({wg::OperationKind::Insert, 2, 0});
    ops.push_back({wg::OperationKind::Erase, 2, 0});
    ops.push_back({wg::OperationKind::Erase, 1, 0});
    ops.push_back({wg::OperationKind::Insert, 3, 0});
    return ops;
}
} // namespace

TEST(E8BytesInUsePeak, DemoTreiberMeldetDenWahrenPeakNichtDieGesamtSumme) {
    auto const ops = peak_koeder_ops();

    fam::a01_hoard::HoardAdapter<> alloc;
    auto const                     result =
        expt::run_single_experiment("peak_koeder", 0xE8, alloc, std::span<wg::Operation const>{ops}, kValueSize);
    ASSERT_TRUE(result.succeeded);

    // T-4-SOLL aus dem Op-Strom (fremder Nenner, oben hergeleitet):
    std::uint64_t const soll_peak  = 3u * kValueSize; // live-Maximum
    std::uint64_t const soll_total = 4u * kValueSize; // kumulative Allokation (4 Inserts)

    EXPECT_EQ(result.record.bytes_in_use_peak, soll_peak)
        << "bytes_in_use_peak muss das live-Maximum des Op-Stroms sein";
    // DER BISS gegen die alte Luege: peak != total, sobald auch nur EINE Freigabe im Strom lag.
    EXPECT_LT(result.record.bytes_in_use_peak, result.record.bytes_allocated)
        << "peak == total_bytes_allocated ist die alte unwahre Befuellung";
    EXPECT_EQ(result.record.bytes_allocated, soll_total)
        << "Gegenprobe: die Gesamt-Summe selbst bleibt die Gesamt-Summe (Hoard zaehlt die "
           "angeforderte Nutzlast)";
}

TEST(E8BytesInUsePeak, OhneFreigabenFallenPeakUndSummeZusammen) {
    // Gegeneingang: ein Strom OHNE Erase -- dann (und nur dann) ist peak == total die WAHRHEIT.
    std::vector<wg::Operation> ops;
    ops.push_back({wg::OperationKind::Insert, 0, 0});
    ops.push_back({wg::OperationKind::Insert, 1, 0});

    fam::a01_hoard::HoardAdapter<> alloc;
    auto const                     result =
        expt::run_single_experiment("peak_monoton", 0xE8, alloc, std::span<wg::Operation const>{ops}, kValueSize);
    ASSERT_TRUE(result.succeeded);
    EXPECT_EQ(result.record.bytes_in_use_peak, 2u * kValueSize);
    EXPECT_EQ(result.record.bytes_in_use_peak, result.record.bytes_allocated);
}

TEST(E8BytesInUsePeak, ReadUndScanBewegenDenPeakNicht) {
    // Gegeneingang: Read/Scan sind allokationsfrei -- der Peak bleibt beim Insert-Maximum.
    std::vector<wg::Operation> ops;
    ops.push_back({wg::OperationKind::Insert, 0, 0});
    ops.push_back({wg::OperationKind::Read, 0, 0});
    ops.push_back({wg::OperationKind::Scan, 0, 8});
    ops.push_back({wg::OperationKind::Erase, 0, 0});
    ops.push_back({wg::OperationKind::Read, 0, 0});

    fam::a01_hoard::HoardAdapter<> alloc;
    auto const                     result =
        expt::run_single_experiment("peak_readscan", 0xE8, alloc, std::span<wg::Operation const>{ops}, kValueSize);
    ASSERT_TRUE(result.succeeded);
    EXPECT_EQ(result.record.bytes_in_use_peak, 1u * kValueSize);
    EXPECT_EQ(result.record.op_count, 5u);
}
