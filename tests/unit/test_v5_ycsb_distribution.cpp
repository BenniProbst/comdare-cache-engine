// V5-#49 Pfad A — YCSB-Treue: die Key-Verteilung ist jetzt Zipfian/Latest (das Definitionsmerkmal von YCSB),
// nicht mehr uniform. Beweist den echten Skew (wenige heiße Keys) gegen die Uniform-Referenz.
//
// YCSB = Cooper et al., ACM SoCC 2010; Zipfian-Algorithmus = Gray et al., SIGMOD 1994 (s. workload_config.hpp).

#include <builder/workload_driver/workload_generator.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <vector>

namespace wd = ::comdare::cache_engine::builder::workload_driver;

namespace {

// Häufigkeit des meistgenutzten Keys über eine reine Lookup-Sequenz (Op-Mix 100% Lookup → jeder Op ein Key).
std::size_t max_key_frequency(wd::KeyDistribution dist, std::uint64_t key_max, std::size_t ops, std::uint64_t seed) {
    wd::WorkloadConfig cfg;
    cfg.seed = seed; cfg.num_operations = ops;
    cfg.key_min = 1; cfg.key_max = key_max;
    cfg.pct_insert = 0.0; cfg.pct_lookup = 1.0; cfg.pct_erase = 0.0; cfg.pct_clear = 0.0;
    cfg.key_distribution = dist;
    wd::WorkloadGenerator gen{cfg};
    std::map<std::uint64_t, std::size_t> freq;
    for (auto const& op : gen.generate_all()) ++freq[op.key];
    std::size_t mx = 0;
    for (auto const& [k, f] : freq) mx = std::max(mx, f);
    return mx;
}

}  // namespace

// Zipfian erzeugt einen ECHTEN Skew: der heißeste Key kommt VIEL häufiger als unter Gleichverteilung.
TEST(V5YcsbDistribution, ZipfianIsSkewedVsUniform) {
    constexpr std::uint64_t kKeyMax = 1000;
    constexpr std::size_t   kOps    = 20000;
    // Uniform-Erwartung pro Key ≈ ops/range = 20000/1000 = 20.
    std::size_t const uni_max  = max_key_frequency(wd::KeyDistribution::Uniform, kKeyMax, kOps, 7);
    std::size_t const zipf_max = max_key_frequency(wd::KeyDistribution::Zipfian, kKeyMax, kOps, 7);
    // Zipf-Top-Key muss deutlich über der Uniform-Spitze liegen (Skew-Definitionsmerkmal). Konservativ: ≥ 5×.
    EXPECT_GT(zipf_max, uni_max * 5u) << "Zipfian-Top=" << zipf_max << " Uniform-Top=" << uni_max;
    // Uniform-Spitze bleibt nahe der Erwartung (kein Skew): < 4× Erwartung.
    EXPECT_LT(uni_max, 80u) << "Uniform sollte ~20 sein, war " << uni_max;
}

// Latest verschiebt den Skew auf die HÖCHSTEN Keys (YCSB-D „read latest").
TEST(V5YcsbDistribution, LatestSkewsTowardHighestKeys) {
    constexpr std::uint64_t kKeyMax = 1000;
    constexpr std::size_t   kOps    = 20000;
    wd::WorkloadConfig cfg;
    cfg.seed = 7; cfg.num_operations = kOps; cfg.key_min = 1; cfg.key_max = kKeyMax;
    cfg.pct_insert = 0.0; cfg.pct_lookup = 1.0; cfg.pct_erase = 0.0; cfg.pct_clear = 0.0;
    cfg.key_distribution = wd::KeyDistribution::Latest;
    wd::WorkloadGenerator gen{cfg};
    std::map<std::uint64_t, std::size_t> freq;
    for (auto const& op : gen.generate_all()) ++freq[op.key];
    // Der meistgenutzte Key liegt im OBEREN Viertel des Key-Raums (latest = höchste Keys heiß).
    std::uint64_t hottest = 0; std::size_t best = 0;
    for (auto const& [k, f] : freq) if (f > best) { best = f; hottest = k; }
    EXPECT_GT(hottest, static_cast<std::uint64_t>(kKeyMax * 3 / 4)) << "hottest key=" << hottest;
}

// Reproduzierbarkeit bleibt erhalten: gleiche Zipfian-Config + Seed → identische Sequenz.
TEST(V5YcsbDistribution, ZipfianIsReproducible) {
    auto cfg = wd::make_ycsb_c(123, 2000);
    wd::WorkloadGenerator a{cfg}, b{cfg};
    EXPECT_EQ(a.generate_all(), b.generate_all());
}
