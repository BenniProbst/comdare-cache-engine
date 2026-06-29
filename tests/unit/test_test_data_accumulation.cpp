// Tests fuer TestDataSetAccumulationEngine (Phase 6.5)

#include <comdare/test_data_accumulation/alignment_strategy.hpp>
#include <comdare/test_data_accumulation/test_data_set_accumulation_engine.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace tda = comdare::test_data_accumulation;

struct DummySearchAlgo {
    int dummy = 0;
};

TEST(AlignmentStrategy, AlignmentBytesPerMode) {
    EXPECT_EQ(tda::alignment_bytes_for(tda::AlignmentMode::CacheLine), 64u);
    EXPECT_EQ(tda::alignment_bytes_for(tda::AlignmentMode::HugePage2M), 2u * 1024u * 1024u);
    EXPECT_EQ(tda::alignment_bytes_for(tda::AlignmentMode::HugePage1G), 1ULL * 1024u * 1024u * 1024u);
}

TEST(AlignmentStrategy, AlignedAllocCacheLine) {
    void* p = tda::aligned_alloc_for_mode(128, tda::AlignmentMode::CacheLine);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % 64, 0u);
    tda::aligned_free(p);
}

TEST(TestDataSetAccumulationEngine, EmptyOnConstruction) {
    DummySearchAlgo                                     algo;
    tda::TestDataSetAccumulationEngine<DummySearchAlgo> engine{algo};
    EXPECT_FALSE(engine.all_loaded());
    EXPECT_EQ(engine.loaded_count(), 0u);
}

TEST(TestDataSetAccumulationEngine, LoadFromMemoryWorks) {
    DummySearchAlgo                                     algo;
    tda::TestDataSetAccumulationEngine<DummySearchAlgo> engine{algo};

    std::array<std::byte, 256> data;
    for (std::size_t i = 0; i < data.size(); ++i) { data[i] = static_cast<std::byte>(i & 0xFF); }
    engine.load_dataset_from_memory("ycsb_a", std::span<std::byte const>{data}, 32);

    EXPECT_TRUE(engine.all_loaded());
    EXPECT_EQ(engine.loaded_count(), 1u);

    auto bytes = engine.get_dataset("ycsb_a");
    EXPECT_EQ(bytes.size(), 256u);
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[42]), 42u);
}

TEST(TestDataSetAccumulationEngine, MakeViewProvidesAccess) {
    DummySearchAlgo                                     algo;
    tda::TestDataSetAccumulationEngine<DummySearchAlgo> engine{algo};

    std::vector<std::byte> data(1024);
    for (std::size_t i = 0; i < data.size(); ++i) { data[i] = static_cast<std::byte>(i & 0xFF); }
    engine.load_dataset_from_memory("uniform", std::span<std::byte const>{data.data(), data.size()}, 8);

    auto view = engine.make_view("uniform");
    EXPECT_EQ(view.name, "uniform");
    EXPECT_EQ(view.bytes.size(), 1024u);
    EXPECT_EQ(view.record_size, 8u);
}

TEST(TestDataSetAccumulationEngine, MultipleDatasetsLoaded) {
    DummySearchAlgo                                     algo;
    tda::TestDataSetAccumulationEngine<DummySearchAlgo> engine{algo};

    std::array<std::byte, 64> a;
    a.fill(std::byte{1});
    std::array<std::byte, 128> b;
    b.fill(std::byte{2});
    std::array<std::byte, 256> c;
    c.fill(std::byte{3});

    engine.load_dataset_from_memory("a", std::span<std::byte const>{a});
    engine.load_dataset_from_memory("b", std::span<std::byte const>{b});
    engine.load_dataset_from_memory("c", std::span<std::byte const>{c});

    EXPECT_EQ(engine.loaded_count(), 3u);
    EXPECT_EQ(engine.get_dataset("a").size(), 64u);
    EXPECT_EQ(engine.get_dataset("b").size(), 128u);
    EXPECT_EQ(engine.get_dataset("c").size(), 256u);
}
