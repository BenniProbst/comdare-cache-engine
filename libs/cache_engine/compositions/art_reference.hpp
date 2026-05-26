#pragma once
// V41.F.6.1.R2 Erweitert: ArtComposition (alle 15 Achsen, 2026-05-26)
//
// @composition ART (P01 Leis/Kemper/Neumann ICDE 2013)
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md

// Topic-3 traversal (3 Achsen)
#include "../topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp"
#include "../topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_linear_fanout.hpp"
#include "../topics/traversal/axis_03m_mapping/axis_03m_mapping_direct_placement.hpp"
// Topic-4 nodes (2 Achsen)
#include "../topics/nodes/axis_02_path_compression/axis_02_path_compression_none.hpp"
#include "../topics/nodes/axis_04_node_type/axis_04_node_type_node256.hpp"
// Topic-5 memory_layout (1 Achse)
#include "../topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp"
// Topic-6 allocator (1 Achse)
#include "../topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp"
// Topic-7 prefetch (1 Achse)
#include "../topics/prefetch/axis_07_prefetch/axis_07_prefetch_none.hpp"
// Topic-8 concurrency (1 Achse)
#include "../topics/concurrency/axis_08_concurrency/axis_08_concurrency_olc.hpp"
// Topic-10 serialization (1 Achse)
#include "../topics/serialization/axis_10_serialization/axis_10_serialization_raw_binary.hpp"
// Topic-11 telemetry (1 Achse)
#include "../topics/telemetry/axis_11_telemetry/axis_11_telemetry_leaf_only.hpp"
// Topic-14 value_handle (1 Achse)
#include "../topics/value_handle/axis_14_value_handle/axis_14_value_handle_inline.hpp"
// Topic hardware (1 Achse)
#include "../topics/hardware/axis_09_isa/axis_09_isa_scalar.hpp"
// Topic search_engine (1 Achse)
#include "../topics/search_engine/axis_01_index_organization/axis_01_index_organization_std_map_like.hpp"
// Topic io (1 Achse)
#include "../topics/io/axis_io/axis_io_in_memory_only.hpp"
// Topic migration (1 Achse)
#include "../topics/migration/axis_migration/axis_migration_none.hpp"
// Topic filter (1 Achse) — bei ART optional (BloomFilter als CE-Erweiterung)
#include "../topics/filter/axis_filter/axis_filter_bloom.hpp"

#include <string_view>

namespace comdare::cache_engine::compositions {

/// ArtComposition — ART als 15-Achsen-Permutations-Konfiguration (Stufe-A).
///
/// ART Charakteristik (Leis ICDE 2013):
/// - Adaptive Radix Tree mit Node4/16/48/256 (axis_04 Node256 als Pilot-Default)
/// - keine Path-Compression im Original (axis_02 PathCompressionNone)
/// - Cache-Line-aligned 64B Nodes (axis_05)
/// - typisch Mimalloc Allocator (axis_06)
/// - OLC Concurrency (axis_08)
/// - Inline-Values in Nodes (axis_14)
/// - Pure in-memory Index (axis_io)
struct ArtComposition {
    // Topic 3 traversal
    using search_algo        = traversal::axis_03a_search_algo::Array256;
    using cache_traversal    = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping            = traversal::axis_03m_mapping::DirectPlacement;
    // Topic 4 nodes
    using path_compression   = nodes::axis_02_path_compression::PathCompressionNone;
    using node_type          = nodes::axis_04_node_type::Node256Type;
    // Topic 5
    using memory_layout      = memory_layout::axis_05_memory_layout::CacheLineAlignedLayout;
    // Topic 6
    using allocator          = allocator::axis_06_allocator::MimallocAllocator;
    // Topic 7
    using prefetch           = prefetch::axis_07_prefetch::PrefetchNone;
    // Topic 8
    using concurrency        = concurrency::axis_08_concurrency::OlcOptimistic;
    // Topic 10
    using serialization      = serialization::axis_10_serialization::RawBinarySerialization;
    // Topic 11
    using telemetry          = telemetry::axis_11_telemetry::LeafOnlyCounter;
    // Topic 14
    using value_handle       = value_handle::axis_14_value_handle::InlineHandle;
    // Topic hardware
    using isa                = hardware::axis_09_isa::IsaScalar;
    // Topic search_engine
    using index_organization = search_engine::axis_01_index_organization::StdMapLike;
    // Topic io
    using io_dispatch        = io::axis_io::InMemoryOnly;
    // Topic migration
    using migration_policy   = migration::axis_migration::NoMigration;
    // Topic filter — ART hat keinen internen Filter, BloomFilter ist optional CE-Erweiterung
    using filter             = filter::axis_filter::BloomFilter;

    static constexpr std::string_view paper_id    = "P01 Leis/Kemper/Neumann ICDE 2013";
    static constexpr std::string_view paper_title = "The Adaptive Radix Tree: ARTful Indexing for Main-Memory Databases";
    static constexpr std::string_view name        = "ArtComposition";
};

}  // namespace
