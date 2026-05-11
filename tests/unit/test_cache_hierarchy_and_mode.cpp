// Test fuer CacheHierarchyManager + CacheEngineMode + Scheduler + Heuristic
// Termin 7 / 02_uml_cache_engine §6+§9 + REV 5 §3 K03 + REV 2 §2.6

#include <cache_engine/concepts/cache_engine_mode.hpp>
#include <cache_engine/concepts/cache_hierarchy_manager.hpp>
#include <cache_engine/platform/i_heuristic.hpp>
#include <cache_engine/platform/i_scheduler.hpp>

#include <gtest/gtest.h>
#include <type_traits>

namespace ce  = comdare::cache_engine;
namespace cep = comdare::cache_engine::platform;

// ─────────────────────────────────────────────────────────────────────────────
// CacheEngineMode (Block AP)
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheEngineMode, FourModesExistInOrder) {
    EXPECT_EQ(static_cast<int>(ce::CacheEngineMode::HEURISTIC_STATIC),    0);
    EXPECT_EQ(static_cast<int>(ce::CacheEngineMode::INFORMED_KALIBRIERT), 1);
    EXPECT_EQ(static_cast<int>(ce::CacheEngineMode::AUTOMATIC_ADAPTIVE),  2);
    EXPECT_EQ(static_cast<int>(ce::CacheEngineMode::BASELINE_NO_ENGINE),  3);
}

TEST(CacheEnginePermutationVariant, FourFactoriesProduceDistinctVariants) {
    // REV 5 K03: 4 Permutations-Varianten pro Search-Engine-Permutation
    auto v1 = ce::variant_v1_baseline();
    auto v2 = ce::variant_v2_static();
    auto v3 = ce::variant_v3_informed();
    auto v4 = ce::variant_v4_adaptive();

    EXPECT_FALSE(v1.use_cache_engine_strategy);
    EXPECT_TRUE(v2.use_cache_engine_strategy);
    EXPECT_TRUE(v3.use_cache_engine_strategy);
    EXPECT_TRUE(v4.use_cache_engine_strategy);

    EXPECT_EQ(v1.mode, ce::CacheEngineMode::BASELINE_NO_ENGINE);
    EXPECT_EQ(v2.mode, ce::CacheEngineMode::HEURISTIC_STATIC);
    EXPECT_EQ(v3.mode, ce::CacheEngineMode::INFORMED_KALIBRIERT);
    EXPECT_EQ(v4.mode, ce::CacheEngineMode::AUTOMATIC_ADAPTIVE);
}

// ─────────────────────────────────────────────────────────────────────────────
// CacheHierarchyManager (F4 generisch — REV 3 K3.2)
// ─────────────────────────────────────────────────────────────────────────────

TEST(DataTemperature, FourTemperaturesExist) {
    EXPECT_EQ(static_cast<int>(ce::DataTemperature::Cold),  0);
    EXPECT_EQ(static_cast<int>(ce::DataTemperature::Warm),  1);
    EXPECT_EQ(static_cast<int>(ce::DataTemperature::Hot),   2);
    EXPECT_EQ(static_cast<int>(ce::DataTemperature::Ultra), 3);
}

TEST(AllocatorTier, FiveGenericTiersExist) {
    // REV 3 K3.2: KEINE Ryzen/Intel-Spezialisierung
    EXPECT_EQ(static_cast<int>(ce::AllocatorTier::StandardDimm),  0);
    EXPECT_EQ(static_cast<int>(ce::AllocatorTier::HbmTier),       1);
    EXPECT_EQ(static_cast<int>(ce::AllocatorTier::LargestL3Tier), 2);   // statt X3DAware
    EXPECT_EQ(static_cast<int>(ce::AllocatorTier::PinnedHighIpc), 3);   // statt IntelHybridP-Core
    EXPECT_EQ(static_cast<int>(ce::AllocatorTier::Persistent),    4);
}

TEST(CacheHierarchyManager, AbstractInterface) {
    EXPECT_TRUE(std::is_abstract_v<ce::ICacheHierarchyManager>);
}

// ─────────────────────────────────────────────────────────────────────────────
// Scheduler + Heuristik
// ─────────────────────────────────────────────────────────────────────────────

TEST(Scheduler, AbstractInterface) {
    EXPECT_TRUE(std::is_abstract_v<cep::IScheduler>);
}

TEST(Heuristic, AbstractInterface) {
    EXPECT_TRUE(std::is_abstract_v<cep::IHeuristic>);
}

TEST(HeuristicCategory, SevenCategoriesExist) {
    EXPECT_EQ(static_cast<int>(cep::HeuristicCategory::StaticCostModel),    0);
    EXPECT_EQ(static_cast<int>(cep::HeuristicCategory::AdaptiveRuntime),    1);
    EXPECT_EQ(static_cast<int>(cep::HeuristicCategory::WorkloadAdaptation), 2);
    EXPECT_EQ(static_cast<int>(cep::HeuristicCategory::LayoutInvariant),    3);
    EXPECT_EQ(static_cast<int>(cep::HeuristicCategory::NegativeFinding),    4);
    EXPECT_EQ(static_cast<int>(cep::HeuristicCategory::TelemetryDriven),    5);
    EXPECT_EQ(static_cast<int>(cep::HeuristicCategory::HardwareProbe),      6);
}
