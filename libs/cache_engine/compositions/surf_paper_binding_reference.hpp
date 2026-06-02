#pragma once
// V41.F.6.1.R3.2 SurfPaperBindingComposition (Paper-Binding S08)

// #42: search_algo = SEZIERTE SuRF-Map-Schale; OriginalSurfSearchAlgo bleibt nur als paper_source-Provenienz.
#include "../topics/traversal/axis_03a_search_algo/composable/tier_to_organ_mapping.hpp"
#include "../topics/traversal/axis_03a_search_algo/axis_03a_search_algo_original_surf.hpp"
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
#include "../topics/hardware/axis_09_isa/axis_09_isa_amd64.hpp"
#include "../topics/search_engine/axis_01_index_organization/axis_01_index_organization_index_organized_table.hpp"
#include "../topics/io/axis_io/axis_io_in_memory_only.hpp"
#include "../topics/migration/axis_migration/axis_migration_none.hpp"
#include "../topics/filter/axis_filter/axis_filter_bloom.hpp"

// R5.G
#include "../anatomy/composition_concept.hpp"

#include <string_view>

namespace comdare::cache_engine::compositions {

/// SurfPaperBindingComposition — SuRF mit Paper-Bindung (1/4 originall lookupKey, 3 Luecken).
/// Behaelt PoolRelative mapping wie SurfComposition (succinct succinct kompakt).
struct SurfPaperBindingComposition {
    using search_algo        = traversal::axis_03a_search_algo::OriginalSurfSearchAlgo;
    using cache_traversal    = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping            = traversal::axis_03m_mapping::PoolRelative;
    using path_compression   = nodes::axis_02_path_compression::PathCompressionNone;
    using node_type          = nodes::axis_04_node_type::ObservableNodeType<nodes::axis_04_node_type::Node256NodeType>;  // V42 L-74c
    using memory_layout      = memory_layout::axis_05_memory_layout::ObservableMemoryLayout<memory_layout::axis_05_memory_layout::CacheLineAlignedMemoryLayout>;  // V42 L-74c
    using allocator          = allocator::axis_06_allocator::MimallocAllocator;
    using prefetch           = prefetch::axis_07_prefetch::NonePrefetch;
    using concurrency        = concurrency::axis_08_concurrency::OlcOptimisticConcurrency;
    using serialization      = serialization::axis_10_serialization::ObservableSerialization<serialization::axis_10_serialization::RawBinarySerialization>;  // V42 L-74c
    using telemetry          = telemetry::axis_11_telemetry::ObservableTelemetry<telemetry::axis_11_telemetry::LeafOnlyCounter>;  // V42 L-74c
    using value_handle       = value_handle::axis_14_value_handle::InlineValueHandle;
    using isa                = hardware::axis_09_isa::Amd64Isa;
    using index_organization = search_engine::axis_01_index_organization::IotIndexOrganization;
    using io_dispatch        = io::axis_io::InMemoryOnly;
    using migration_policy   = migration::axis_migration::NoMigration;
    using filter             = filter::axis_filter::BloomFilter;

    static constexpr std::string_view paper_id    = "P10 Zhang/Lim/Andersen SIGMOD 2018 (Paper-Binding)";
    static constexpr std::string_view paper_title = "SuRF: Practical Range Query Filtering (surf.hpp Paper-Source)";
    static constexpr std::string_view name        = "SurfPaperBindingComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION(
        "::comdare::cache_engine::compositions::SurfPaperBindingComposition",
        "compositions/surf_paper_binding_reference.hpp");
};

}  // namespace
