// Tests fuer Mikrobenchmark-Suite (Phase 6.6)

#include <comdare/benchmark_suite/custom_allocation_1_measurements.hpp>
#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen
#include <comdare/benchmark_suite/custom_allocation_2_state_log.hpp>
#include <comdare/benchmark_suite/benchmark_runner.hpp>
#include <comdare/benchmark_suite/binary_blob_writer.hpp>
#include <comdare/benchmark_suite/conversion/binary_to_csv.hpp>
#include <comdare/benchmark_suite/conversion/binary_to_json.hpp>
#include <comdare/benchmark_suite/conversion/binary_to_tikz.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iostream> // Verlust-NENNER gehoert in die AUSGABE, nicht nur in ein EXPECT
#include <limits>   // UINT64_MAX-Sentinel von CustomAllocation1::append
#include <vector>

namespace bs = comdare::benchmark_suite;

// ─────────────────────────────────────────────────────────────────────────────
// CustomAllocation1 (32B Records, gross genug, nie fail)
// ─────────────────────────────────────────────────────────────────────────────

TEST(CustomAllocation1, AppendAndSnapshot) {
    bs::CustomAllocation1 alloc{1024 * 32}; // 1024 records
    EXPECT_EQ(alloc.records_used(), 0u);

    bs::MeasurementRecord32 r{};
    r.timestamp_ns = 12345;
    r.op_id        = 1;
    auto slot      = alloc.append(r);
    EXPECT_EQ(slot, 0u);
    EXPECT_EQ(alloc.records_used(), 1u);

    auto snap = alloc.snapshot();
    EXPECT_EQ(snap.size(), 1u);
    EXPECT_EQ(snap[0].timestamp_ns, 12345u);
}

TEST(CustomAllocation1, RecordSize32) {
    EXPECT_EQ(sizeof(bs::MeasurementRecord32), 32u);
    EXPECT_EQ(alignof(bs::MeasurementRecord32), 32u);
}

TEST(CustomAllocation1, ManyAppendsLockless) {
    bs::CustomAllocation1 alloc{1024 * 32};
    for (int i = 0; i < 100; ++i) {
        bs::MeasurementRecord32 r{};
        r.op_id = static_cast<std::uint64_t>(i);
        // Rueckgabewert bewusst konsumiert: append() traegt [[nodiscard]], weil sein Rueckgabewert der
        // EINZIGE Ueberlauf-Melder ist -- ihn wegzuwerfen war der Defekt vom 09.08. Verglichen wird
        // gegen die BENANNTE Konstante kOverflow und nicht gegen numeric_limits::max(): wandert der
        // Sentinel je, wandert der Vergleich mit. ASSERT statt EXPECT, weil ein Ueberlauf mitten in
        // der Schleife jede folgende Zusicherung wertlos macht. 1024 Saetze passen in 32 KiB, 100 sind
        // also sicher -- aber "sicher" gehoert geprueft, nicht angenommen.
        ASSERT_NE(alloc.append(r), bs::CustomAllocation1::kOverflow) << "Ueberlauf bei i=" << i;
    }
    EXPECT_EQ(alloc.records_used(), 100u);
    EXPECT_EQ(alloc.records_dropped(), 0u);
}

// -----------------------------------------------------------------------------
// 09.08.2026 -- Warnungs-Runde 1, Klasse SPEICHER.
//
// BEFUND: append() meldet den Ueberlauf mit kOverflow, push_state() mit false. Beide Meldungen
// wurden von BenchmarkRunner verworfen (GCC: viermal -Wunused-result). Zugleich KAPPEN
// records_used() und bytes_used() auf die Kapazitaet. Damit war "Arena randvoll" von "Arena
// uebergelaufen, N Saetze fehlen" nicht mehr unterscheidbar -- eine loechrige Messreihe sah aus wie
// eine vollstaendige. Die Wachen unten machen genau diesen Unterschied sichtbar.
//
// NENNER dieser Gruppe: 4 Tests. Jeder nennt seine Kapazitaet in Saetzen bzw. Bytes und stellt der
// Verlust-Zusicherung einen GEGENEINGANG gegenueber, bei dem sie faellt (T-4).
// -----------------------------------------------------------------------------

TEST(CustomAllocation1, OverflowIsCountedNotHidden) {
    // NENNER: Kapazitaet 4 Saetze (128 Byte / 32 Byte je Satz), 6 Versuche, also 2 erwartete Verluste.
    bs::CustomAllocation1 alloc{4 * 32};
    ASSERT_EQ(alloc.capacity_records(), 4u) << "Vorbedingung: die Arena fasst genau 4 Saetze";

    bs::MeasurementRecord32 r{};
    for (int i = 0; i < 4; ++i) {
        r.op_id = static_cast<std::uint64_t>(i);
        EXPECT_NE(alloc.append(r), bs::CustomAllocation1::kOverflow) << "Satz " << i << " muss noch passen";
    }
    EXPECT_EQ(alloc.records_dropped(), 0u) << "randvoll ist KEIN Verlust";

    for (int i = 4; i < 6; ++i) {
        r.op_id = static_cast<std::uint64_t>(i);
        EXPECT_EQ(alloc.append(r), bs::CustomAllocation1::kOverflow) << "Satz " << i << " muss abgewiesen werden";
    }

    // Der Kern: records_used() steht still (es sind wirklich nur 4 Saetze lesbar) -- der Verlust wird
    // NICHT dort sichtbar, sondern nur am ungekappten Zaehler. Genau diese Trennung war der Defekt.
    EXPECT_EQ(alloc.records_used(), 4u);
    EXPECT_EQ(alloc.records_attempted(), 6u);
    EXPECT_EQ(alloc.records_dropped(), 2u) << "zwei Saetze sind verloren und muessen benannt sein";
}

TEST(BenchmarkRunner, LostMeasurementsAreVisible) {
    // NENNER: Mess-Arena fuer 4 Saetze; eine Phase erzeugt 6 Saetze (begin + 4 events + end).
    bs::BenchmarkRunner runner{4 * 32, 4096};

    auto h = runner.begin_measurement("phase-zu-klein");
    for (int e = 0; e < 4; ++e) { runner.record_event(h, bs::EventKind::Custom, static_cast<std::uint64_t>(e)); }
    runner.end_measurement(h, 999);

    // records_collected() allein LUEGE hier nicht, aber es sagt die Unwahrheit ueber die
    // Vollstaendigkeit: 4 gesammelt sieht aus wie ein sauberer Lauf.
    EXPECT_EQ(runner.records_collected(), 4u);
    EXPECT_EQ(runner.measurements_dropped(), 2u) << "6 erzeugt - 4 Platz = 2 verloren";
    EXPECT_FALSE(runner.measurement_complete()) << "eine loechrige Reihe darf sich nie vollstaendig nennen";
}

TEST(BenchmarkRunner, CompleteRunReportsNoLoss) {
    // GEGENEINGANG zu LostMeasurementsAreVisible (T-4): dieselbe Phase, aber die Arena reicht.
    // Faellt diese Zusicherung, meldet der Zaehler Verlust, wo keiner ist -- dann waere die Wache
    // selbst der Defekt und jede gruene Messung ab sofort verdaechtig.
    bs::BenchmarkRunner runner{1024 * 32, 4096};

    auto h = runner.begin_measurement("phase-passt");
    for (int e = 0; e < 4; ++e) { runner.record_event(h, bs::EventKind::Custom, static_cast<std::uint64_t>(e)); }
    runner.end_measurement(h, 999);

    EXPECT_EQ(runner.records_collected(), 6u);
    EXPECT_EQ(runner.measurements_dropped(), 0u);
    EXPECT_TRUE(runner.measurement_complete());
}

TEST(BenchmarkRunner, LostSparseStatesAreVisible) {
    // NENNER: State-Log 40 Byte; je Eintrag 10 + 8 = 18 Byte, also passen genau 2, der 3. faellt.
    // Die Mess-Arena ist absichtlich gross -- so ist bewiesen, dass measurement_complete() auch am
    // State-Log haengt und nicht nur an den Mess-Saetzen.
    bs::BenchmarkRunner      runner{1024 * 32, 40};
    std::array<std::byte, 8> delta;
    delta.fill(std::byte{0x55});

    runner.log_sparse_state(0x01, std::span<std::byte const>{delta});
    runner.log_sparse_state(0x02, std::span<std::byte const>{delta});
    EXPECT_EQ(runner.states_dropped(), 0u) << "zwei Eintraege passen in 40 Byte";
    EXPECT_TRUE(runner.measurement_complete());

    runner.log_sparse_state(0x03, std::span<std::byte const>{delta});
    EXPECT_EQ(runner.states_dropped(), 1u) << "der dritte Eintrag passt nicht mehr";
    EXPECT_FALSE(runner.measurement_complete()) << "auch ein verlorener State macht die Reihe unvollstaendig";
}

TEST(CustomAllocation2, DeltaLargerThanArenaIsRejectedBeforeArithmetic) {
    // NENNER: Arena 64 Byte, Delta 128 Byte. Das Delta allein ist groesser als die ganze Arena.
    // Die Pruefung sitzt VOR "10 + delta.size()", damit diese Summe nicht umschlagen kann.
    bs::CustomAllocation2      alloc{64};
    std::array<std::byte, 128> big;
    big.fill(std::byte{0x77});

    EXPECT_FALSE(alloc.push_state(0x09, std::span<std::byte const>{big}));
    EXPECT_EQ(alloc.bytes_used(), 0u) << "eine abgewiesene Einfuegung darf den Puffer nicht anfassen";

    // GEGENEINGANG (T-4): ein Delta, das passt, muss weiterhin durchgehen -- sonst haette die neue
    // Schranke die Arena stillgelegt statt sie zu sichern.
    std::array<std::byte, 8> small;
    small.fill(std::byte{0x11});
    EXPECT_TRUE(alloc.push_state(0x0A, std::span<std::byte const>{small}));
    EXPECT_EQ(alloc.bytes_dropped(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// CustomAllocation2 (Sparse Byte States)
// ─────────────────────────────────────────────────────────────────────────────

TEST(CustomAllocation2, PushStateAndSnapshot) {
    bs::CustomAllocation2 alloc{4096};
    EXPECT_EQ(alloc.bytes_used(), 0u);

    std::array<std::byte, 5> delta{std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF}, std::byte{0x42}};
    EXPECT_TRUE(alloc.push_state(0xAA, std::span<std::byte const>{delta}));
    EXPECT_GT(alloc.bytes_used(), 5u);
}

// ─────────────────────────────────────────────────────────────────────────────
// BenchmarkRunner (No-Deprecate Wrapper)
// ─────────────────────────────────────────────────────────────────────────────

TEST(BenchmarkRunner, BeginRecordEnd) {
    bs::BenchmarkRunner runner{1024 * 32, 4096};

    auto h = runner.begin_measurement("phase1");
    runner.record_event(h, bs::EventKind::CacheMiss, 42);
    runner.record_event(h, bs::EventKind::Allocation, 128);
    runner.end_measurement(h, 12345);

    // 4 records: begin, cache_miss, allocation, end
    EXPECT_EQ(runner.records_collected(), 4u);
}

TEST(BenchmarkRunner, MultiplePhases) {
    bs::BenchmarkRunner runner{1024 * 32, 4096};

    for (int p = 0; p < 5; ++p) {
        auto h = runner.begin_measurement("phase");
        for (int e = 0; e < 10; ++e) { runner.record_event(h, bs::EventKind::Custom, e); }
        runner.end_measurement(h, p);
    }
    // 5 phases * (1 begin + 10 events + 1 end) = 60
    EXPECT_EQ(runner.records_collected(), 60u);
    // NENNER-Gegenprobe zum neuen Verlust-Zaehler: in einem Lauf, der die Kapazitaet NICHT sprengt,
    // muss er 0 sein. Ohne diese Richtung koennte die Wache konstant melden und waere wertlos.
    EXPECT_EQ(runner.measurements_dropped(), 0u);
    EXPECT_TRUE(runner.measurement_complete());
}

// ---------------------------------------------------------------------------
// WARNUNGS-REVIEW RUNDE 2b (clang ueber den ce-Test-Bau, 09.08.2026) -- DER STILLE MESSVERLUST.
//
// BEFUND, woertlich aus dem Bau-Log:
//   .../benchmark_suite/benchmark_runner.hpp:47:9: warning: ignoring return value of function
//                       declared with 'nodiscard' attribute [-Wunused-result]   (ebenso :57, :66, :70)
//
// CustomAllocation1::append() meldet einen vollen Puffer mit UINT64_MAX, CustomAllocation2::
// push_state() mit false. Beide Rueckgaben wurden verworfen -- ab dem ersten Ueberlauf ging jeder
// weitere Satz verloren, ohne Wurf, ohne Rueckgabe, ohne Luecke in records_collected() (das klemmt
// auf capacity_). Eine unvollstaendige Messreihe war von einer vollstaendigen NICHT unterscheidbar.
//
// DIESE ZWEI TESTS SIND DIE WACHE. Sie messen den VERLUST, nicht die Kapazitaet: Grundgesamtheit ist
// die Zahl der ANGEBOTENEN Saetze, und die steht in der Ausgabe. Das Orakel kommt NICHT aus dem
// Prueflung -- die Kapazitaet wird aus der KONSTRUKTOR-Angabe und sizeof(MeasurementRecord32)
// gerechnet (beides von aussen bekannt), nicht aus records_collected() erfragt.
// ---------------------------------------------------------------------------
TEST(BenchmarkRunnerVerlust, NENNER_UeberlaufWirdGEZAEHLT_StattStillZuSchlucken) {
    constexpr std::size_t kKapazitaetSaetze = 8;
    constexpr std::size_t kBytes            = kKapazitaetSaetze * sizeof(bs::MeasurementRecord32);
    constexpr std::size_t kAngeboten        = 20; // mehr als hineinpasst -- der Koeder

    bs::BenchmarkRunner runner{kBytes, 4096};
    for (std::size_t i = 0; i < kAngeboten; ++i) {
        auto const h = runner.begin_measurement("ueberlauf");
        (void)h;
    }

    auto const gespeichert = runner.records_collected();
    auto const verworfen   = runner.measurements_dropped();
    std::cout << "[VERLUST-NENNER] angeboten: " << kAngeboten << " -- gespeichert: " << gespeichert
              << " -- verworfen: " << verworfen << " (Kapazitaet " << kKapazitaetSaetze << " Saetze, gerechnet aus "
              << kBytes << " Byte / sizeof(MeasurementRecord32)=" << sizeof(bs::MeasurementRecord32) << ")\n";

    EXPECT_EQ(gespeichert, kKapazitaetSaetze) << "der Puffer nimmt genau seine Kapazitaet auf";
    EXPECT_EQ(verworfen, kAngeboten - kKapazitaetSaetze) << "und JEDER darueber hinaus angebotene Satz ist gezaehlt";
    // DIE EIGENTLICHE ZUSICHERUNG: angeboten == gespeichert + verworfen. Vor der Heilung war die
    // rechte Seite nur `gespeichert`, und die Differenz existierte nirgends.
    EXPECT_EQ(gespeichert + verworfen, kAngeboten) << "kein Satz faellt zwischen die Zaehler";
    EXPECT_FALSE(runner.measurement_complete()) << "eine Reihe mit Verlust darf sich NICHT vollstaendig nennen";
}

TEST(BenchmarkRunnerVerlust, NENNER_AuchDerZustandsLogZaehltSeinenUeberlauf) {
    // push_state braucht 10 + delta.size() Byte. Bei 8-Byte-Delta sind das 18 -- in 32 Byte passt
    // genau EINER, der zweite muss als Verlust erscheinen.
    constexpr std::size_t kLogBytes  = 32;
    constexpr std::size_t kAngeboten = 5;

    bs::BenchmarkRunner      runner{1024 * 32, kLogBytes};
    std::array<std::byte, 8> delta{};
    for (std::size_t i = 0; i < kAngeboten; ++i) runner.log_sparse_state(static_cast<std::uint8_t>(i), delta);

    std::cout << "[VERLUST-NENNER] Zustands-Pushes angeboten: " << kAngeboten
              << " -- verworfen: " << runner.states_dropped() << " (Log-Kapazitaet " << kLogBytes
              << " Byte, Bedarf je Push 10+" << delta.size() << ")\n";
    EXPECT_GT(runner.states_dropped(), 0u) << "der Koeder muss beissen: 5 Pushes passen nicht in 32 Byte";
    EXPECT_FALSE(runner.measurement_complete());

    // GEGENEINGANG (T-4): derselbe Ablauf mit ausreichendem Log verliert NICHTS.
    bs::BenchmarkRunner weit{1024 * 32, 4096};
    for (std::size_t i = 0; i < kAngeboten; ++i) weit.log_sparse_state(static_cast<std::uint8_t>(i), delta);
    EXPECT_EQ(weit.states_dropped(), 0u) << "sonst waere die Wache konstant rot und wuerde nichts unterscheiden";
    EXPECT_TRUE(weit.measurement_complete());
}

TEST(BenchmarkRunner, SparseStateLog) {
    bs::BenchmarkRunner runner{1024 * 32, 4096};

    std::array<std::byte, 8> delta;
    delta.fill(std::byte{0x55});
    runner.log_sparse_state(0x01, std::span<std::byte const>{delta});
    runner.log_sparse_state(0x02, std::span<std::byte const>{delta});

    EXPECT_GT(runner.state_log_bytes(), 16u);
}

// ─────────────────────────────────────────────────────────────────────────────
// BinaryBlobWriter (End-of-Experiment Konsolidierung)
// ─────────────────────────────────────────────────────────────────────────────

TEST(BinaryBlobWriter, WritesValidBlob) {
    bs::BenchmarkRunner runner{1024 * 32, 4096};
    auto                h = runner.begin_measurement("test");
    runner.end_measurement(h, 999);

    auto tmp_dir = ::comdare::test::user_tmp_dir();
    auto path    = tmp_dir / "comdare_test_blob.cdb";

    runner.flush_to_binary_blob(path);
    EXPECT_TRUE(std::filesystem::exists(path));
    EXPECT_GT(std::filesystem::file_size(path), 24u); // header alone

    std::filesystem::remove(path);
}

// ─────────────────────────────────────────────────────────────────────────────
// Conversion-Routinen (POST-EXPERIMENT NUR!)
// ─────────────────────────────────────────────────────────────────────────────

TEST(ConversionRoutines, BinaryToCsv) {
    std::vector<bs::MeasurementRecord32> records(3);
    for (int i = 0; i < 3; ++i) {
        records[i].timestamp_ns    = i * 1000;
        records[i].op_id           = i;
        records[i].cycles_or_value = i * i;
    }
    bs::conversion::BinaryToCsv conv;
    auto                        path = ::comdare::test::user_tmp_dir() / "comdare_test.csv";
    conv.convert(std::span<bs::MeasurementRecord32 const>{records}, path);
    EXPECT_TRUE(std::filesystem::exists(path));
    std::filesystem::remove(path);
}

TEST(ConversionRoutines, BinaryToJson) {
    std::vector<bs::MeasurementRecord32> records(2);
    bs::conversion::BinaryToJson         conv;
    auto                                 path = ::comdare::test::user_tmp_dir() / "comdare_test.json";
    conv.convert(std::span<bs::MeasurementRecord32 const>{records}, path);
    EXPECT_TRUE(std::filesystem::exists(path));
    std::filesystem::remove(path);
}

TEST(ConversionRoutines, BinaryToTikz) {
    std::vector<bs::MeasurementRecord32> records(2);
    records[0].cycles_or_value = 100;
    records[1].cycles_or_value = 200;
    bs::conversion::BinaryToTikz conv;
    auto                         path = ::comdare::test::user_tmp_dir() / "comdare_test.tikz";
    conv.convert(std::span<bs::MeasurementRecord32 const>{records}, path);
    EXPECT_TRUE(std::filesystem::exists(path));
    std::filesystem::remove(path);
}
