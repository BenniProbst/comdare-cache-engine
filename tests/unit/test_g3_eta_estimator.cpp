// test_g3_eta_estimator -- G3 / #46b Lagerhaltung, Scheibe B4 (ETA-Rechner).
//
// Literal-Tests der reinen ETA-/avg_size-Arithmetik: ETA = Sum(t_i)/N mit Untergrenze max(t_i),
// avg_size, Slice-Projektion, kombinierte Block-Kalibrierung. Keine Zeit-/IO-Abhaengigkeit.

#include "bestandslog/eta_estimator.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace bl = comdare::cache_engine::builder::bestandslog;

TEST(G3EtaEstimator, EtaIsSumOverThreads) {
    std::vector<double> times{10.0, 20.0, 30.0, 40.0};    // Sum = 100
    EXPECT_DOUBLE_EQ(bl::estimate_eta_s(times, 2), 50.0); // 100/2 = 50 > max(40)
}

TEST(G3EtaEstimator, EtaLowerBoundIsMax) {
    std::vector<double> times{10.0, 20.0, 30.0, 40.0}; // Sum = 100
    // 100/8 = 12.5, aber die Untergrenze max(t_i)=40 greift.
    EXPECT_DOUBLE_EQ(bl::estimate_eta_s(times, 8), 40.0);
}

TEST(G3EtaEstimator, EtaSingleThreadIsSum) {
    std::vector<double> times{5.0, 5.0, 5.0};
    EXPECT_DOUBLE_EQ(bl::estimate_eta_s(times, 1), 15.0);
}

TEST(G3EtaEstimator, EtaEdgeCases) {
    std::vector<double> empty{};
    EXPECT_DOUBLE_EQ(bl::estimate_eta_s(empty, 4), 0.0);
    std::vector<double> one{7.5};
    EXPECT_DOUBLE_EQ(bl::estimate_eta_s(one, 0), 7.5); // n_threads==0 -> als 1 behandelt
}

TEST(G3EtaEstimator, AverageSize) {
    std::vector<std::uint64_t> sizes{100, 200, 300};
    EXPECT_EQ(bl::average_size_bytes(sizes), 200u);
    std::vector<std::uint64_t> one{428032};
    EXPECT_EQ(bl::average_size_bytes(one), 428032u);
    std::vector<std::uint64_t> empty{};
    EXPECT_EQ(bl::average_size_bytes(empty), 0u);
}

TEST(G3EtaEstimator, SliceProjection) {
    // 4096 Binaries, avg 2s/Binary, 32 Threads -> 4096*2/32 = 256s (> longest 5s).
    EXPECT_DOUBLE_EQ(bl::project_slice_eta_s(2.0, 4096, 32, 5.0), 256.0);
    // Kleine Restmenge -> Untergrenze (laengster beobachteter Compile) greift.
    EXPECT_DOUBLE_EQ(bl::project_slice_eta_s(2.0, 10, 32, 50.0), 50.0);
}

TEST(G3EtaEstimator, CalibrateBlockCombines) {
    std::vector<double>        times{2.0, 2.0, 2.0, 2.0}; // 8/4 = 2, max 2 -> 2.0
    std::vector<std::uint64_t> sizes{100, 300};           // avg 200
    auto                       r = bl::calibrate_block(times, sizes, 4);
    EXPECT_DOUBLE_EQ(r.eta_s, 2.0);
    EXPECT_EQ(r.avg_size_bytes, 200u);
    EXPECT_EQ(r, (bl::EtaResult{2.0, 200}));
}
