#pragma once
// V41.F.6.1.R2 Erweitert: SurfComposition (alle 15 Achsen)

#include "../topics/traversal/axis_03a_search_algo/axis_03a_search_algo_vector_u16u16.hpp"
#include "../topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_linear_fanout.hpp"
#include "../topics/traversal/axis_03m_mapping/axis_03m_mapping_pool_relative.hpp"
#include "../topics/nodes/axis_02_path_compression/axis_02_path_compression_none.hpp"
#include "../topics/nodes/axis_04_node_type/axis_04_node_type_node256.hpp"
#include "../topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp"
#include "../topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp"
#include "../topics/prefetch/axis_07_prefetch/axis_07_prefetch_none.hpp"
#include "../topics/concurrency/axis_08_concurrency/axis_08_concurrency_olc.hpp"
#include "../topics/serialization/axis_10_serialization/axis_10_serialization_raw_binary.hpp"
#include "../topics/telemetry/axis_11_telemetry/axis_11_telemetry_leaf_only.hpp"
#include "../topics/value_handle/axis_14_value_handle/axis_14_value_handle_inline.hpp"
#include "../topics/hardware/axis_09_isa/axis_09_isa_scalar.hpp"
#include "../topics/search_engine/axis_01_index_organization/axis_01_index_organization_std_map_like.hpp"
#include "../topics/io/axis_io/axis_io_in_memory_only.hpp"
#include "../topics/migration/axis_migration/axis_migration_none.hpp"
#include "../topics/filter/axis_filter/axis_filter_bloom.hpp"

// R5.G
#include "../anatomy/composition_concept.hpp"

#include <string_view>

namespace comdare::cache_engine::compositions {

/// SurfComposition — SuRF als 15-Achsen-Permutations-Konfiguration.
///
/// SuRF Charakteristik (Zhang/Lim/Andersen SIGMOD 2018):
/// - Bulk-Loaded Succinct LOUDS-Trie (Read-Only-Index)
/// - Pool-Relative Mapping (succinct kompakt, axis_03m PoolRelative)
/// - **KERN-Funktion: Range-Query-Filter** → filter::BloomFilter ist Composition-konstitutiv
///   (im Gegensatz zu ART/HOT wo Filter optional ist)
struct SurfComposition {
    using search_algo        = traversal::axis_03a_search_algo::VectorU16U16;
    using cache_traversal    = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping            = traversal::axis_03m_mapping::PoolRelative;
    using path_compression   = nodes::axis_02_path_compression::PathCompressionNone;
    using node_type          = nodes::axis_04_node_type::Node256Type;
    using memory_layout      = memory_layout::axis_05_memory_layout::CacheLineAlignedLayout;
    using allocator          = allocator::axis_06_allocator::MimallocAllocator;
    using prefetch           = prefetch::axis_07_prefetch::PrefetchNone;
    using concurrency        = concurrency::axis_08_concurrency::OlcOptimistic;
    using serialization      = serialization::axis_10_serialization::RawBinarySerialization;
    using telemetry          = telemetry::axis_11_telemetry::LeafOnlyCounter;
    using value_handle       = value_handle::axis_14_value_handle::InlineHandle;
    using isa                = hardware::axis_09_isa::IsaScalar;
    using index_organization = search_engine::axis_01_index_organization::StdMapLike;
    using io_dispatch        = io::axis_io::InMemoryOnly;
    using migration_policy   = migration::axis_migration::NoMigration;
    using filter             = filter::axis_filter::BloomFilter;  // SuRF kann mit BloomFilter ergaenzt werden

    static constexpr std::string_view paper_id    = "P10 Zhang/Lim/Andersen SIGMOD 2018";
    static constexpr std::string_view paper_title = "SuRF: Practical Range Query Filtering with Fast Succinct Tries";
    static constexpr std::string_view name        = "SurfComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION(
        "::comdare::cache_engine::compositions::SurfComposition",
        "compositions/surf_reference.hpp");
};

}  // namespace
