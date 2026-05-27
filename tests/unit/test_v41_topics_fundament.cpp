// V41.F.6.1.F1 Topics-Fundament Smoke-Test (2026-05-26)
//
// Verifiziert dass alle 12 Skelett-Stufe-A Topic-Achsen ihre Concept-Conformance
// erfüllen. Wird inkrementell erweitert mit jedem fundament-Phase F1/F2/F3.

#include <gtest/gtest.h>

#include <topics/axis_base.hpp>
#include <concepts/legacy_original_code_strategy_concept.hpp>

// Phase F1 Topics
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_none.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_node256.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/concurrency/axis_08_concurrency/axis_08_concurrency_olc.hpp>
#include <topics/prefetch/axis_07_prefetch/axis_07_prefetch_none.hpp>
// Phase F2 Topics
#include <topics/telemetry/axis_11_telemetry/axis_11_telemetry_leaf_only.hpp>
#include <topics/serialization/axis_10_serialization/axis_10_serialization_raw_binary.hpp>
#include <topics/value_handle/axis_14_value_handle/axis_14_value_handle_inline.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_scalar.hpp>
// Phase F3 Topics
#include <topics/search_engine/axis_01_index_organization/axis_01_index_organization_std_map_like.hpp>
#include <topics/io/axis_io/axis_io_in_memory_only.hpp>
#include <topics/migration/axis_migration/axis_migration_none.hpp>
#include <topics/filter/axis_filter/axis_filter_bloom.hpp>

namespace ce_topics   = ::comdare::cache_engine::topics;
namespace ce_concepts = ::comdare::cache_engine::concepts;

// Phase F1 Type-Aliases
using PathCompressionNone   = ::comdare::cache_engine::nodes::axis_02_path_compression::PathCompressionNone;
using Node256Layout           = ::comdare::cache_engine::nodes::axis_04_node_type::Node256Layout;
using CacheLineAlignedMemoryLayout = ::comdare::cache_engine::memory_layout::axis_05_memory_layout::CacheLineAlignedMemoryLayout;
using OlcOptimisticConcurrency         = ::comdare::cache_engine::concurrency::axis_08_concurrency::OlcOptimisticConcurrency;
using NonePrefetch          = ::comdare::cache_engine::prefetch::axis_07_prefetch::NonePrefetch;

// ─────────────────────────────────────────────────────────────────────────────
// Phase F1 — 5 Achsen × 3 Pflicht-Checks (AxisBaseConcept + LegacyOriginalCodePflicht + Concept-Specific)
// ─────────────────────────────────────────────────────────────────────────────

TEST(PhaseF1_Nodes, PathCompressionNoneAxisBase) {
    static_assert(ce_topics::AxisBaseConcept<PathCompressionNone>);
    static_assert(ce_concepts::LegacyOriginalCodePflicht<PathCompressionNone>);
    SUCCEED();
}

TEST(PhaseF1_Nodes, Node256LayoutAxisBase) {
    static_assert(ce_topics::AxisBaseConcept<Node256Layout>);
    static_assert(ce_concepts::LegacyOriginalCodePflicht<Node256Layout>);
    static_assert(Node256Layout::max_capacity() == 256);
    SUCCEED();
}

TEST(PhaseF1_MemoryLayout, CacheLineAlignedAxisBase) {
    static_assert(ce_topics::AxisBaseConcept<CacheLineAlignedMemoryLayout>);
    static_assert(ce_concepts::LegacyOriginalCodePflicht<CacheLineAlignedMemoryLayout>);
    static_assert(CacheLineAlignedMemoryLayout::cache_line_size() == 64);
    SUCCEED();
}

TEST(PhaseF1_Concurrency, OlcOptimisticAxisBase) {
    static_assert(ce_topics::AxisBaseConcept<OlcOptimisticConcurrency>);
    static_assert(ce_concepts::LegacyOriginalCodePflicht<OlcOptimisticConcurrency>);
    static_assert(OlcOptimisticConcurrency::concurrency_pattern() ==
                  ::comdare::cache_engine::concurrency::axis_08_concurrency::concepts::ConcurrencyPattern::Optimistic);
    SUCCEED();
}

TEST(PhaseF1_Prefetch, PrefetchNoneAxisBase) {
    static_assert(ce_topics::AxisBaseConcept<NonePrefetch>);
    static_assert(ce_concepts::LegacyOriginalCodePflicht<NonePrefetch>);
    static_assert(!NonePrefetch::is_active());
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// Cross-Topic: alle 5 Wrappers nutzen AxisBase Default get_compiler="original"
// ─────────────────────────────────────────────────────────────────────────────

TEST(PhaseF1_CrossTopic, AllUseAxisBaseDefaultCompiler) {
    static_assert(PathCompressionNone::get_compiler() == std::string_view{"original"});
    static_assert(Node256Layout::get_compiler() == std::string_view{"original"});
    static_assert(CacheLineAlignedMemoryLayout::get_compiler() == std::string_view{"original"});
    static_assert(OlcOptimisticConcurrency::get_compiler() == std::string_view{"original"});
    static_assert(NonePrefetch::get_compiler() == std::string_view{"original"});
    SUCCEED();
}

TEST(PhaseF1_CrossTopic, AllUseAxisBaseDefaultIsOriginalModule) {
    static_assert(!PathCompressionNone::is_original_module());
    static_assert(!Node256Layout::is_original_module());
    static_assert(!CacheLineAlignedMemoryLayout::is_original_module());
    static_assert(!OlcOptimisticConcurrency::is_original_module());
    static_assert(!NonePrefetch::is_original_module());
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase F2 — telemetry + serialization + value_handle + hardware
// ─────────────────────────────────────────────────────────────────────────────

using LeafOnlyCounter        = ::comdare::cache_engine::telemetry::axis_11_telemetry::LeafOnlyCounter;
using RawBinarySerialization = ::comdare::cache_engine::serialization::axis_10_serialization::RawBinarySerialization;
using InlineValueHandle           = ::comdare::cache_engine::value_handle::axis_14_value_handle::InlineValueHandle;
using IsaScalar              = ::comdare::cache_engine::hardware::axis_09_isa::IsaScalar;

TEST(PhaseF2_Telemetry, LeafOnlyCounterAxisBase) {
    static_assert(ce_topics::AxisBaseConcept<LeafOnlyCounter>);
    static_assert(LeafOnlyCounter::is_leaf_only());
    SUCCEED();
}
TEST(PhaseF2_Serialization, RawBinaryAxisBase) {
    static_assert(ce_topics::AxisBaseConcept<RawBinarySerialization>);
    static_assert(!RawBinarySerialization::supports_compression());
    SUCCEED();
}
TEST(PhaseF2_ValueHandle, InlineHandleAxisBase) {
    static_assert(ce_topics::AxisBaseConcept<InlineValueHandle>);
    static_assert(InlineValueHandle::is_inline());
    SUCCEED();
}
TEST(PhaseF2_Hardware, IsaScalarAxisBase) {
    static_assert(ce_topics::AxisBaseConcept<IsaScalar>);
    static_assert(!IsaScalar::supports_simd());
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase F3 — search_engine + io + migration + filter
// ─────────────────────────────────────────────────────────────────────────────

using StdMapLike   = ::comdare::cache_engine::search_engine::axis_01_index_organization::StdMapLike;
using InMemoryOnly = ::comdare::cache_engine::io::axis_io::InMemoryOnly;
using NoMigration  = ::comdare::cache_engine::migration::axis_migration::NoMigration;
using BloomFilter  = ::comdare::cache_engine::filter::axis_filter::BloomFilter;

TEST(PhaseF3_SearchEngine, StdMapLikeAxisBase) {
    static_assert(ce_topics::AxisBaseConcept<StdMapLike>);
    static_assert(StdMapLike::is_ordered());
    SUCCEED();
}
TEST(PhaseF3_Io, InMemoryOnlyAxisBase) {
    static_assert(ce_topics::AxisBaseConcept<InMemoryOnly>);
    static_assert(InMemoryOnly::is_in_memory_only());
    SUCCEED();
}
TEST(PhaseF3_Migration, NoMigrationAxisBase) {
    static_assert(ce_topics::AxisBaseConcept<NoMigration>);
    static_assert(!NoMigration::is_active());
    SUCCEED();
}
TEST(PhaseF3_Filter, BloomFilterAxisBase) {
    static_assert(ce_topics::AxisBaseConcept<BloomFilter>);
    static_assert(!BloomFilter::supports_range_query());
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// AllTopicsCoverage — alle 13 Achsen-Wrappers (Phase F1 5 + F2 4 + F3 4)
// nutzen AxisBase Default get_compiler="original" + is_original_module=false
// ─────────────────────────────────────────────────────────────────────────────

TEST(AllTopicsCoverage, ThirteenAxisWrappersAllUseAxisBaseDefaults) {
    // 5 Phase F1
    static_assert(PathCompressionNone::get_compiler() == std::string_view{"original"});
    static_assert(Node256Layout::get_compiler() == std::string_view{"original"});
    static_assert(CacheLineAlignedMemoryLayout::get_compiler() == std::string_view{"original"});
    static_assert(OlcOptimisticConcurrency::get_compiler() == std::string_view{"original"});
    static_assert(NonePrefetch::get_compiler() == std::string_view{"original"});
    // 4 Phase F2
    static_assert(LeafOnlyCounter::get_compiler() == std::string_view{"original"});
    static_assert(RawBinarySerialization::get_compiler() == std::string_view{"original"});
    static_assert(InlineValueHandle::get_compiler() == std::string_view{"original"});
    static_assert(IsaScalar::get_compiler() == std::string_view{"original"});
    // 4 Phase F3
    static_assert(StdMapLike::get_compiler() == std::string_view{"original"});
    static_assert(InMemoryOnly::get_compiler() == std::string_view{"original"});
    static_assert(NoMigration::get_compiler() == std::string_view{"original"});
    static_assert(BloomFilter::get_compiler() == std::string_view{"original"});
    SUCCEED();
}
