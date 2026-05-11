// Test fuer 28 Saeule-B Plattform-Concepts (REV 5 K07 + REV 3 K3.2 generisch)
// Termin 7 / 13_saeule_b_plattform_modell + 10_korrektur_architektur §K3.2

#include <cache_engine/platform/cache_topology.hpp>
#include <cache_engine/platform/concurrency_protocol.hpp>
#include <cache_engine/platform/core_layout.hpp>
#include <cache_engine/platform/i_platform_probe.hpp>
#include <cache_engine/platform/interconnect.hpp>
#include <cache_engine/platform/isa_features.hpp>
#include <cache_engine/platform/live_platform_model.hpp>
#include <cache_engine/platform/primitives.hpp>
#include <cache_engine/platform/rebuild.hpp>
#include <cache_engine/platform/storage.hpp>
#include <cache_engine/platform/workload_model.hpp>

#include <gtest/gtest.h>
#include <type_traits>

namespace cep = comdare::cache_engine::platform;

// ─────────────────────────────────────────────────────────────────────────────
// REV 3 K3.2 — Verbot CPU-spezifischer Klassen, Auto-Discovery generisch
// ─────────────────────────────────────────────────────────────────────────────

TEST(PlatformProbe, AbstractInterfaceIsPolymorphic) {
    EXPECT_TRUE(std::is_abstract_v<cep::IPlatformProbe>);
    EXPECT_TRUE(std::is_abstract_v<cep::IPlatformPropertyClassifier>);
    EXPECT_TRUE(std::is_abstract_v<cep::ICacheEngineOptionPublisher>);
}

TEST(PlatformPropertySet, DefaultValuesAreFalseAndZero) {
    cep::PlatformPropertySet p{};
    EXPECT_FALSE(p.has_asymmetric_l3);
    EXPECT_FALSE(p.has_hybrid_cores);
    EXPECT_FALSE(p.has_hbm_tier);
    EXPECT_FALSE(p.has_software_prefetch);
    EXPECT_FALSE(p.has_hardware_transactional);
    EXPECT_FALSE(p.cpu_core_atom_perf_separation);
    EXPECT_EQ(p.preferred_pinning_policy, 0);
    EXPECT_EQ(p.usable_simd_width_bytes, 0);
    EXPECT_TRUE(p.measured_metrics.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Cache-Topology
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheTopology, AllAbstractInterfacesArePolymorphic) {
    EXPECT_TRUE(std::is_abstract_v<cep::ICacheLevel>);
    EXPECT_TRUE(std::is_abstract_v<cep::ICacheLine>);
    EXPECT_TRUE(std::is_abstract_v<cep::ICacheTopology>);
    EXPECT_TRUE(std::is_abstract_v<cep::ICacheResidency>);
}

TEST(CacheLineState, FiveMesiOesiStatesExist) {
    EXPECT_EQ(static_cast<int>(cep::CacheLineState::Modified),  0);
    EXPECT_EQ(static_cast<int>(cep::CacheLineState::Exclusive), 1);
    EXPECT_EQ(static_cast<int>(cep::CacheLineState::Shared),    2);
    EXPECT_EQ(static_cast<int>(cep::CacheLineState::Invalid),   3);
    EXPECT_EQ(static_cast<int>(cep::CacheLineState::Owned),     4);
}

TEST(CacheResidencyTier, FiveTiersExist) {
    EXPECT_EQ(static_cast<int>(cep::CacheResidencyTier::L1Cached),     0);
    EXPECT_EQ(static_cast<int>(cep::CacheResidencyTier::HeaderCached), 1);
    EXPECT_EQ(static_cast<int>(cep::CacheResidencyTier::L2Cached),     2);
    EXPECT_EQ(static_cast<int>(cep::CacheResidencyTier::L3Cached),     3);
    EXPECT_EQ(static_cast<int>(cep::CacheResidencyTier::Uncached),     4);
}

// ─────────────────────────────────────────────────────────────────────────────
// Core-Layout (REV 3 K3.2 generisch — KEINE Intel/Ryzen-Spez.)
// ─────────────────────────────────────────────────────────────────────────────

TEST(CoreClass, FourGenericClassesExist) {
    EXPECT_EQ(static_cast<int>(cep::CoreClass::Generic),     0);
    EXPECT_EQ(static_cast<int>(cep::CoreClass::HighIpc),     1);  // generisch fuer P-Cores/V-Cache
    EXPECT_EQ(static_cast<int>(cep::CoreClass::LowIpc),      2);  // generisch fuer E-Cores
    EXPECT_EQ(static_cast<int>(cep::CoreClass::Specialized), 3);  // generisch fuer PEs
}

TEST(PinningPolicyId, FiveGenericPoliciesExist) {
    // REV 3 K3.2: Largest-L3-CCD (statt X3DAware), HotPath-on-HighIpc (statt IntelHybrid)
    EXPECT_EQ(static_cast<int>(cep::PinningPolicyId::None),             0);
    EXPECT_EQ(static_cast<int>(cep::PinningPolicyId::LargestL3Ccd),     1);
    EXPECT_EQ(static_cast<int>(cep::PinningPolicyId::HotPathOnHighIpc), 2);
    EXPECT_EQ(static_cast<int>(cep::PinningPolicyId::NumaLocal),        3);
    EXPECT_EQ(static_cast<int>(cep::PinningPolicyId::RoundRobin),       4);
}

TEST(CoreLayoutInterfaces, AllAbstract) {
    EXPECT_TRUE(std::is_abstract_v<cep::ICpuCore>);
    EXPECT_TRUE(std::is_abstract_v<cep::ICoreLayout>);
    EXPECT_TRUE(std::is_abstract_v<cep::IPinningPolicy>);
    EXPECT_TRUE(std::is_abstract_v<cep::ICoreToThreadMap>);
}

// ─────────────────────────────────────────────────────────────────────────────
// ISA-Features
// ─────────────────────────────────────────────────────────────────────────────

TEST(IsaFeatureSet, AbstractInterfaceIsPolymorphic) {
    EXPECT_TRUE(std::is_abstract_v<cep::IIsaFeatureSet>);
    EXPECT_TRUE(std::is_abstract_v<cep::IHardwareExtension>);
}

TEST(HardwareExtensionFamily, SevenFamiliesExist) {
    EXPECT_EQ(static_cast<int>(cep::HardwareExtensionFamily::BaseISA),          0);
    EXPECT_EQ(static_cast<int>(cep::HardwareExtensionFamily::SimdExtension),    1);
    EXPECT_EQ(static_cast<int>(cep::HardwareExtensionFamily::BitManipulation),  2);
    EXPECT_EQ(static_cast<int>(cep::HardwareExtensionFamily::MemoryHints),      3);
    EXPECT_EQ(static_cast<int>(cep::HardwareExtensionFamily::Synchronization),  4);
    EXPECT_EQ(static_cast<int>(cep::HardwareExtensionFamily::Security),         5);
    EXPECT_EQ(static_cast<int>(cep::HardwareExtensionFamily::DatabaseSpecific), 6);
}

// ─────────────────────────────────────────────────────────────────────────────
// Storage / Concurrency-Protocol / Rebuild / Workload
// ─────────────────────────────────────────────────────────────────────────────

TEST(StorageMediumKind, SixMediaKindsExist) {
    EXPECT_EQ(static_cast<int>(cep::StorageMediumKind::Dram),        0);
    EXPECT_EQ(static_cast<int>(cep::StorageMediumKind::Hbm),         1);
    EXPECT_EQ(static_cast<int>(cep::StorageMediumKind::Nvram),       2);
    EXPECT_EQ(static_cast<int>(cep::StorageMediumKind::Cxl),         3);
    EXPECT_EQ(static_cast<int>(cep::StorageMediumKind::SsdNvme),     4);
    EXPECT_EQ(static_cast<int>(cep::StorageMediumKind::HddSpinning), 5);
}

TEST(RcuFlavor, SixFlavorsExist) {
    // REV 5 K08: 6-Flavor-RCU-Klassifikation
    EXPECT_EQ(static_cast<int>(cep::RcuFlavor::Classic),    0);
    EXPECT_EQ(static_cast<int>(cep::RcuFlavor::Qsbr),       1);
    EXPECT_EQ(static_cast<int>(cep::RcuFlavor::Bp),         2);
    EXPECT_EQ(static_cast<int>(cep::RcuFlavor::Membarrier), 3);
    EXPECT_EQ(static_cast<int>(cep::RcuFlavor::Signal),     4);
    EXPECT_EQ(static_cast<int>(cep::RcuFlavor::Sysmemb),    5);
}

TEST(RebuildTrigger, FiveTriggersExist) {
    EXPECT_EQ(static_cast<int>(cep::RebuildTrigger::OfflineSelfTuning),      0);
    EXPECT_EQ(static_cast<int>(cep::RebuildTrigger::OnlineProbabilityBased), 1);
    EXPECT_EQ(static_cast<int>(cep::RebuildTrigger::OnEveryStructureMod),    2);
    EXPECT_EQ(static_cast<int>(cep::RebuildTrigger::PeriodicGlobal),         3);
    EXPECT_EQ(static_cast<int>(cep::RebuildTrigger::BulkLoadLevelByLevel),   4);
}

TEST(WorkloadDistribution, SevenDistributionsExist) {
    EXPECT_EQ(static_cast<int>(cep::WorkloadDistribution::Uniform),     0);
    EXPECT_EQ(static_cast<int>(cep::WorkloadDistribution::Zipfian),     1);
    EXPECT_EQ(static_cast<int>(cep::WorkloadDistribution::Pareto),      2);
    EXPECT_EQ(static_cast<int>(cep::WorkloadDistribution::PrefixHeavy), 5);
    EXPECT_EQ(static_cast<int>(cep::WorkloadDistribution::Sequential),  6);
}

TEST(WorkloadOpMix, IncludesYcsbVariants) {
    EXPECT_EQ(static_cast<int>(cep::WorkloadOpMix::YcsbA), 10);
    EXPECT_EQ(static_cast<int>(cep::WorkloadOpMix::YcsbB), 11);
    EXPECT_EQ(static_cast<int>(cep::WorkloadOpMix::YcsbC), 12);
}

// ─────────────────────────────────────────────────────────────────────────────
// Interconnect / Bandwidth
// ─────────────────────────────────────────────────────────────────────────────

TEST(InterconnectClass, SixLinkClassesExist) {
    EXPECT_EQ(static_cast<int>(cep::InterconnectClass::IntraCcd),  0);
    EXPECT_EQ(static_cast<int>(cep::InterconnectClass::InterCcd),  1);
    EXPECT_EQ(static_cast<int>(cep::InterconnectClass::MemoryBus), 2);
    EXPECT_EQ(static_cast<int>(cep::InterconnectClass::NumaLink),  3);
    EXPECT_EQ(static_cast<int>(cep::InterconnectClass::PcIe),      4);
    EXPECT_EQ(static_cast<int>(cep::InterconnectClass::NvLink),    5);
}

TEST(InterconnectInterfaces, AllAbstract) {
    EXPECT_TRUE(std::is_abstract_v<cep::IInterconnect>);
    EXPECT_TRUE(std::is_abstract_v<cep::IBusTopology>);
    EXPECT_TRUE(std::is_abstract_v<cep::IMemoryBandwidthModel>);
}

// ─────────────────────────────────────────────────────────────────────────────
// Primitives + LiveModel
// ─────────────────────────────────────────────────────────────────────────────

TEST(Primitives, AllInterfacesAbstract) {
    EXPECT_TRUE(std::is_abstract_v<cep::IRankSelectPrimitive>);
    EXPECT_TRUE(std::is_abstract_v<cep::IBranchPredictorModel>);
    EXPECT_TRUE(std::is_abstract_v<cep::IBitManipulationFeatureGate>);
}

TEST(LivePlatformModel, BothAbstract) {
    EXPECT_TRUE(std::is_abstract_v<cep::ILivePlatformModel>);
    EXPECT_TRUE(std::is_abstract_v<cep::ILiveCpuModel>);
}

TEST(StorageInterfaces, AllAbstract) {
    EXPECT_TRUE(std::is_abstract_v<cep::IStorageMedium>);
    EXPECT_TRUE(std::is_abstract_v<cep::IPageCacheModel>);
}

TEST(WorkloadInterfaces, AllAbstract) {
    EXPECT_TRUE(std::is_abstract_v<cep::IWorkloadModel>);
    EXPECT_TRUE(std::is_abstract_v<cep::ICellProbeModel>);
    EXPECT_TRUE(std::is_abstract_v<cep::IWordProbeModel>);
}

TEST(RebuildInterfaces, BothAbstract) {
    EXPECT_TRUE(std::is_abstract_v<cep::IRebuildScheduler>);
    EXPECT_TRUE(std::is_abstract_v<cep::IRebuildCostModel>);
}
