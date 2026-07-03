#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <compositions/art_reference.hpp>

#include <gtest/gtest.h>

#include <cstdint>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;

namespace {

using Tier = an::SearchAlgorithmAbiAdapter<an::SearchAlgorithmAnatomy<comp::ArtComposition>>;

} // namespace

TEST(StatResetAfterLoad216H2, ResetKeepsDataAndDropsLoadInsertStats) {
    Tier  tier;
    auto* obs = dynamic_cast<an::IObservableTier*>(static_cast<an::IAnatomyBase*>(&tier));
    ASSERT_NE(obs, nullptr);

    constexpr std::uint64_t kLoadRecords = 32;
    constexpr std::uint64_t kRunInserts  = 7;
    constexpr std::uint64_t kRunLookups  = 5;

    obs->tier_clear();
    for (std::uint64_t i = 1; i <= kLoadRecords; ++i) { ASSERT_TRUE(obs->tier_insert(i, i * 7u + 1u)); }
    ASSERT_EQ(obs->tier_size(), kLoadRecords);

    obs->tier_reset_statistics();

    for (std::uint64_t i = 1; i <= kRunLookups; ++i) {
        std::uint64_t value = 0;
        ASSERT_TRUE(obs->tier_lookup(i, &value));
        EXPECT_EQ(value, i * 7u + 1u);
    }
    EXPECT_EQ(obs->tier_size(), kLoadRecords);

    for (std::uint64_t i = 1; i <= kRunInserts; ++i) {
        std::uint64_t const key = kLoadRecords + i;
        ASSERT_TRUE(obs->tier_insert(key, key * 7u + 1u));
    }

    an::ComdareTierObserverSnapshot snapshot{};
    obs->tier_observe(&snapshot);

    EXPECT_EQ(snapshot.axis_stats[0][3], kRunInserts);
    EXPECT_NE(snapshot.axis_stats[0][3], kLoadRecords + kRunInserts);
    EXPECT_EQ(snapshot.axis_stats[0][0], kRunLookups);
    EXPECT_EQ(snapshot.axis_stats[0][1], kRunLookups);
    EXPECT_EQ(snapshot.tier_fill_level, kLoadRecords + kRunInserts);

    std::uint64_t loaded_value = 0;
    EXPECT_TRUE(obs->tier_lookup(1, &loaded_value));
    EXPECT_EQ(loaded_value, 8u);
}