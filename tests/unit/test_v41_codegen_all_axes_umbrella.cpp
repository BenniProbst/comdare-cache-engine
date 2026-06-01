// V41.F.6.1 R5.G — all_axes_umbrella.hpp Compile-Verifikation (Auto-Emitter-Plumbing).
//
// Beweist: der EINE Umbrella-Include stellt alle 17 Achsen-Default-Typen + AdHocComposition + das
// ADHOC-Makro bereit — die Basis dafür, dass ein generiertes Permutations-Modul-.cpp nur diesen
// Header inkludieren muss. Eine Beispiel-AdHoc-Composition aus den 17 Default-Achsen ist IsComposition
// + als volle Anatomie + ABI-Adapter instanziierbar.

#include <builder/codegen/all_axes_umbrella.hpp>   // EINZIGER Achsen-Include

#include <gtest/gtest.h>

namespace ana = ::comdare::cache_engine::anatomy;

// Beispiel-Permutation aus den 17 Default-Achsen-Typen (alle via Umbrella verfügbar).
using SampleAdHoc = ana::AdHocComposition<
    ::comdare::cache_engine::traversal::axis_03a_search_algo::Array256SearchAlgo,
    ::comdare::cache_engine::traversal::axis_03b_cache_traversal::LinearFanout,
    ::comdare::cache_engine::traversal::axis_03m_mapping::DirectPlacement,
    ::comdare::cache_engine::nodes::axis_02_path_compression::PathCompressionNone,
    ::comdare::cache_engine::nodes::axis_04_node_type::Node256NodeType,
    ::comdare::cache_engine::memory_layout::axis_05_memory_layout::CacheLineAlignedMemoryLayout,
    ::comdare::cache_engine::allocator::axis_06_allocator::MimallocAllocator,
    ::comdare::cache_engine::prefetch::axis_07_prefetch::NonePrefetch,
    ::comdare::cache_engine::concurrency::axis_08_concurrency::OlcOptimisticConcurrency,
    ::comdare::cache_engine::serialization::axis_10_serialization::RawBinarySerialization,
    ::comdare::cache_engine::telemetry::axis_11_telemetry::LeafOnlyCounter,
    ::comdare::cache_engine::value_handle::axis_14_value_handle::InlineValueHandle,
    ::comdare::cache_engine::hardware::axis_09_isa::Amd64Isa,
    ::comdare::cache_engine::search_engine::axis_01_index_organization::IotIndexOrganization,
    ::comdare::cache_engine::io::axis_io::InMemoryOnly,
    ::comdare::cache_engine::migration::axis_migration::NoMigration,
    ::comdare::cache_engine::filter::axis_filter::BloomFilter>;

static_assert(ana::IsComposition<SampleAdHoc>,
              "Umbrella muss alle 17 Achsen-Typen für eine vollständige AdHoc-Composition liefern.");

TEST(R5G_Umbrella, AllAxesResolveViaSingleIncludeAndComposeFullAnatomy) {
    using Anatomy = ana::SearchAlgorithmAnatomy<SampleAdHoc>;
    static_assert(ana::AnatomyConcept<Anatomy>);
    ana::SearchAlgorithmAbiAdapter<Anatomy> adapter;
    ana::IAnatomyBase* base = &adapter;
    EXPECT_EQ(base->organ_count(), 17u);
    EXPECT_EQ(base->genus(), ana::AnatomyGenus::SearchAlgorithm);
}
