#include "permutation_loop/permutation_sampling.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

using comdare::builder::loop::sample_keep;
using comdare::builder::loop::stable_id_hash;

constexpr std::uint32_t kRateDisabledZero = 0u;
constexpr std::uint32_t kRateDisabledOne  = 1u;
constexpr std::uint32_t kSampleRate       = 1000u;
constexpr std::uint64_t kSeedA            = 7u;
constexpr std::uint64_t kSeedB            = 42u;
constexpr int           kPopulation       = 100000;
constexpr std::size_t   kMinExpectedKept  = 20u;
constexpr std::size_t   kMaxExpectedKept  = 500u;

static_assert(stable_id_hash("") == 0xCBF29CE484222325ULL);
static_assert(stable_id_hash("perm_0") == 0xE07DB80B33990424ULL);
static_assert(stable_id_hash("cache:search:alloc:tds") == 0x3A46BA38E7032099ULL);

[[nodiscard]] std::vector<std::string> kept_ids(std::uint32_t sample_rate, std::uint64_t sample_seed) {
    std::vector<std::string> kept;
    for (int i = 0; i < kPopulation; ++i) {
        std::string id = "perm_" + std::to_string(i);
        if (sample_keep(id, sample_rate, sample_seed)) kept.push_back(std::move(id));
    }
    return kept;
}

} // namespace

TEST(PermutationSampling, RateOneOrZeroKeepsAll) {
    for (std::string const& id : {"", "perm_0", "cache:search:alloc:tds"}) {
        EXPECT_TRUE(sample_keep(id, kRateDisabledZero, kSeedA));
        EXPECT_TRUE(sample_keep(id, kRateDisabledOne, kSeedA));
        EXPECT_TRUE(sample_keep(id, kRateDisabledZero, kSeedB));
        EXPECT_TRUE(sample_keep(id, kRateDisabledOne, kSeedB));
    }
}

TEST(PermutationSampling, Deterministic) {
    auto const first  = kept_ids(kSampleRate, kSeedA);
    auto const second = kept_ids(kSampleRate, kSeedA);
    EXPECT_EQ(first, second);
}

TEST(PermutationSampling, RateApprox) {
    auto const kept = kept_ids(kSampleRate, kSeedA);
    EXPECT_GT(kept.size(), kMinExpectedKept);
    EXPECT_LT(kept.size(), kMaxExpectedKept);
}

TEST(PermutationSampling, SeedSensitivity) {
    auto const seed_a_first  = kept_ids(kSampleRate, kSeedA);
    auto const seed_a_second = kept_ids(kSampleRate, kSeedA);
    auto const seed_b_first  = kept_ids(kSampleRate, kSeedB);
    auto const seed_b_second = kept_ids(kSampleRate, kSeedB);

    EXPECT_EQ(seed_a_first, seed_a_second);
    EXPECT_EQ(seed_b_first, seed_b_second);
    EXPECT_NE(seed_a_first, seed_b_first);
}
