// Test fuer 12 Sub-Engine-Slots (Termin 7 + RBMM-Konvention)
// Verzeichnisse behalten cNN_-Praefix fuer Sortierreihenfolge,
// Namespaces sind semantisch (User-Korrektur 2026-05-12).

#include <c01_cost_engine/i_cost_engine.hpp>
#include <c02_pinning_engine/i_pinning_engine.hpp>
#include <c03_prefetch_engine/i_prefetch_engine.hpp>
#include <c04_coherence_engine/i_coherence_engine.hpp>
#include <c05_telemetry_engine/i_telemetry_engine.hpp>
#include <c06_allocation_engine/i_allocation_engine.hpp>
#include <c07_migration_engine/i_migration_engine.hpp>
#include <c08_encoding_engine/i_encoding_engine.hpp>
#include <c09_heuristik_engine/i_heuristik_engine.hpp>
#include <c10_topologie_engine/i_topologie_engine.hpp>
#include <c11_scheduler_engine/i_scheduler_engine.hpp>
#include <c12_filter_engine/i_filter_engine.hpp>

#include <gtest/gtest.h>
#include <type_traits>

namespace cost       = comdare::cache_engine::subsystems::cost;
namespace pinning    = comdare::cache_engine::subsystems::pinning;
namespace prefetch   = comdare::cache_engine::subsystems::prefetch;
namespace coherence  = comdare::cache_engine::subsystems::coherence;
namespace telemetry  = comdare::cache_engine::subsystems::telemetry;
namespace allocation = comdare::cache_engine::subsystems::allocation;
namespace migration  = comdare::cache_engine::subsystems::migration;
namespace encoding   = comdare::cache_engine::subsystems::encoding;
namespace heuristik  = comdare::cache_engine::subsystems::heuristik;
namespace topologie  = comdare::cache_engine::subsystems::topologie;
namespace scheduler  = comdare::cache_engine::subsystems::scheduler;
namespace filter     = comdare::cache_engine::subsystems::filter;

TEST(SubEngines, AllTwelveInterfacesAreAbstract) {
    EXPECT_TRUE(std::is_abstract_v<cost::ICostEngine>);
    EXPECT_TRUE(std::is_abstract_v<pinning::IPinningEngine>);
    EXPECT_TRUE(std::is_abstract_v<prefetch::IPrefetchEngine>);
    EXPECT_TRUE(std::is_abstract_v<coherence::ICoherenceEngine>);
    EXPECT_TRUE(std::is_abstract_v<telemetry::ITelemetryEngine>);
    EXPECT_TRUE(std::is_abstract_v<allocation::IAllocationEngine>);
    EXPECT_TRUE(std::is_abstract_v<migration::IMigrationEngine>);
    EXPECT_TRUE(std::is_abstract_v<encoding::IEncodingEngine>);
    EXPECT_TRUE(std::is_abstract_v<heuristik::IHeuristikEngine>);
    EXPECT_TRUE(std::is_abstract_v<topologie::ITopologieEngine>);
    EXPECT_TRUE(std::is_abstract_v<scheduler::ISchedulerEngine>);
    EXPECT_TRUE(std::is_abstract_v<filter::IFilterEngine>);
}

// ─────────────────────────────────────────────────────────────────────────────
// C02 Pinning — REV 3 K3.2 generisch (kein X3D/Hybrid-spezifisch)
// ─────────────────────────────────────────────────────────────────────────────

TEST(C02PinningEngine, FiveGenericTargetsAreDistinct) {
    EXPECT_EQ(static_cast<int>(pinning::PinningTarget::AnyCore), 0);
    EXPECT_EQ(static_cast<int>(pinning::PinningTarget::LargestL3CcdCore), 1); // statt X3DAware
    EXPECT_EQ(static_cast<int>(pinning::PinningTarget::HighIpcCore), 2);      // statt IntelHybrid
    EXPECT_EQ(static_cast<int>(pinning::PinningTarget::NumaLocalCore), 3);
    EXPECT_EQ(static_cast<int>(pinning::PinningTarget::SpecializedCore), 4);
}

// ─────────────────────────────────────────────────────────────────────────────
// C03 Prefetch — sieben paper-belegte Policies
// ─────────────────────────────────────────────────────────────────────────────

TEST(C03PrefetchEngine, SevenPoliciesIncludeNoneAndPaperVariants) {
    EXPECT_EQ(static_cast<int>(prefetch::PrefetchPolicy::None), 0);
    EXPECT_EQ(static_cast<int>(prefetch::PrefetchPolicy::SoftwareFixed), 1);
    EXPECT_EQ(static_cast<int>(prefetch::PrefetchPolicy::AdaptiveDistance), 2);
    EXPECT_EQ(static_cast<int>(prefetch::PrefetchPolicy::HierarchicalBundle), 3);
    EXPECT_EQ(static_cast<int>(prefetch::PrefetchPolicy::FillBufferAware), 4);
    EXPECT_EQ(static_cast<int>(prefetch::PrefetchPolicy::HotPath), 5);
    EXPECT_EQ(static_cast<int>(prefetch::PrefetchPolicy::JumpPointerArray), 6);
}

// ─────────────────────────────────────────────────────────────────────────────
// C07 Migration
// ─────────────────────────────────────────────────────────────────────────────

TEST(C07MigrationEngine, FiveMigrationKindsExist) {
    EXPECT_EQ(static_cast<int>(migration::MigrationKind::LocalRelocation), 0);
    EXPECT_EQ(static_cast<int>(migration::MigrationKind::GlobalReorganization), 1);
    EXPECT_EQ(static_cast<int>(migration::MigrationKind::HotToUltra), 2);
    EXPECT_EQ(static_cast<int>(migration::MigrationKind::ColdToStandard), 3);
    EXPECT_EQ(static_cast<int>(migration::MigrationKind::Eviction), 4);
}

// ─────────────────────────────────────────────────────────────────────────────
// C08 Encoding — 12 paper-belegte Encodings
// ─────────────────────────────────────────────────────────────────────────────

TEST(C08EncodingEngine, TwelveEncodingsArePresent) {
    EXPECT_EQ(static_cast<int>(encoding::EncodingKind::LoudsJacobson), 0);
    EXPECT_EQ(static_cast<int>(encoding::EncodingKind::LoudsDense), 1);
    EXPECT_EQ(static_cast<int>(encoding::EncodingKind::LoudsSparse), 2);
    EXPECT_EQ(static_cast<int>(encoding::EncodingKind::EliasFano), 3);
    EXPECT_EQ(static_cast<int>(encoding::EncodingKind::PackedArray), 4);
    EXPECT_EQ(static_cast<int>(encoding::EncodingKind::Bitvector), 5);
    EXPECT_EQ(static_cast<int>(encoding::EncodingKind::DenseEncoding), 6);
    EXPECT_EQ(static_cast<int>(encoding::EncodingKind::PointerElimination), 7);
    EXPECT_EQ(static_cast<int>(encoding::EncodingKind::PartialPointerElim), 8);
    EXPECT_EQ(static_cast<int>(encoding::EncodingKind::OrderPreservingHuffman), 9);
    EXPECT_EQ(static_cast<int>(encoding::EncodingKind::PrefixTruncation), 10);
    EXPECT_EQ(static_cast<int>(encoding::EncodingKind::KeyNormalization), 11);
}

// ─────────────────────────────────────────────────────────────────────────────
// C11 Scheduler
// ─────────────────────────────────────────────────────────────────────────────

TEST(C11SchedulerEngine, FourSchedulingKindsExist) {
    EXPECT_EQ(static_cast<int>(scheduler::SchedulingKind::PageTypeUpgrade), 0);
    EXPECT_EQ(static_cast<int>(scheduler::SchedulingKind::PageTypeDowngrade), 1);
    EXPECT_EQ(static_cast<int>(scheduler::SchedulingKind::PageRebalance), 2);
    EXPECT_EQ(static_cast<int>(scheduler::SchedulingKind::OperationDeferral), 3);
}

// ─────────────────────────────────────────────────────────────────────────────
// C12 Filter — SuRF-Variants + klassische Filter
// ─────────────────────────────────────────────────────────────────────────────

TEST(C12FilterEngine, FourSurfVariantsPlusFourClassicFilters) {
    EXPECT_EQ(static_cast<int>(filter::FilterKind::SurfBase), 0);
    EXPECT_EQ(static_cast<int>(filter::FilterKind::SurfHash), 1);
    EXPECT_EQ(static_cast<int>(filter::FilterKind::SurfReal), 2);
    EXPECT_EQ(static_cast<int>(filter::FilterKind::SurfMixed), 3);
    EXPECT_EQ(static_cast<int>(filter::FilterKind::Bloom), 4);
    EXPECT_EQ(static_cast<int>(filter::FilterKind::CountingBloom), 5);
    EXPECT_EQ(static_cast<int>(filter::FilterKind::Cuckoo), 6);
    EXPECT_EQ(static_cast<int>(filter::FilterKind::Arf), 7);
}

// ─────────────────────────────────────────────────────────────────────────────
// Strukturelle Validation — alle 12 Sub-Engine-Slots vorhanden + benannt
// ─────────────────────────────────────────────────────────────────────────────

TEST(SubEngines, NamingFollowsCxxNumberingConvention) {
    static_assert(sizeof(cost::CostEstimate) > 0, "C01 CostEstimate fehlt");
    static_assert(sizeof(coherence::CoherenceMetrics) > 0, "C04 CoherenceMetrics fehlt");
    static_assert(sizeof(migration::MigrationResult) > 0, "C07 MigrationResult fehlt");
    static_assert(sizeof(heuristik::HeuristicEvaluation) > 0, "C09 HeuristicEvaluation fehlt");
    SUCCEED();
}
