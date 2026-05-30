#pragma once
// V41.F.6.1.R3.2 StartPaperBindingComposition (Paper-Binding S06)

// #42: search_algo = SEZIERTES START-Organ; OriginalStartSearchAlgo bleibt nur als paper_source-Provenienz.
#include "../topics/traversal/axis_03a_search_algo/composable/tier_to_organ_mapping.hpp"
#include "../topics/traversal/axis_03a_search_algo/axis_03a_search_algo_original_start.hpp"
#include "../topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_linear_fanout.hpp"
#include "../topics/traversal/axis_03m_mapping/axis_03m_mapping_direct_placement.hpp"
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

/// StartPaperBindingComposition — START mit Paper-Bindung (OriginalStartSearchAlgo, 2/4 originall).
struct StartPaperBindingComposition {
    using search_algo        = traversal::axis_03a_search_algo::composable::ObservableStartTrieOrgan;
    using paper_source       = traversal::axis_03a_search_algo::OriginalStartSearchAlgo;  // #42 is_original/SHA256-Traeger, kein Achsen-Wert
    using cache_traversal    = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping            = traversal::axis_03m_mapping::DirectPlacement;
    using path_compression   = nodes::axis_02_path_compression::PathCompressionNone;
    using node_type          = nodes::axis_04_node_type::Node256Layout;
    using memory_layout      = memory_layout::axis_05_memory_layout::CacheLineAlignedMemoryLayout;
    using allocator          = allocator::axis_06_allocator::MimallocAllocator;
    using prefetch           = prefetch::axis_07_prefetch::NonePrefetch;
    using concurrency        = concurrency::axis_08_concurrency::OlcOptimisticConcurrency;
    using serialization      = serialization::axis_10_serialization::RawBinarySerialization;
    using telemetry          = telemetry::axis_11_telemetry::LeafOnlyCounter;
    using value_handle       = value_handle::axis_14_value_handle::InlineValueHandle;
    using isa                = hardware::axis_09_isa::Amd64Isa;
    using index_organization = search_engine::axis_01_index_organization::IotIndexOrganization;
    using io_dispatch        = io::axis_io::InMemoryOnly;
    using migration_policy   = migration::axis_migration::NoMigration;
    using filter             = filter::axis_filter::BloomFilter;

    static constexpr std::string_view paper_id    = "P05 Mertens ICDE 2024 (Paper-Binding)";
    static constexpr std::string_view paper_title = "START: Self-Tuning Adaptive Radix Tree (Paper-Source)";
    static constexpr std::string_view name        = "StartPaperBindingComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION(
        "::comdare::cache_engine::compositions::StartPaperBindingComposition",
        "compositions/start_paper_binding_reference.hpp");
};

}  // namespace
