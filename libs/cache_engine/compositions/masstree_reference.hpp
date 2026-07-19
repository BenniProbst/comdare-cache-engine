#pragma once
// V41.F.6.1.R2 Erweitert: MasstreeComposition (alle 17 Achsen, INC-2d-Stand; V41-historisch: 15)
//
// #42-Folge: search_algo ist jetzt das ECHTE sezierte Masstree-Organ (B+Baum-of-Tries, kpermuter-Knoten,
// Multi-Layer-Slice-Tries, SliceBytes=2) statt des frueheren flachen ObservableSortedBinaryOrgan-Platzhalters
// (letzter Platzhalter-Konfigurator -> echtes Organ). is_original=false ([[pseudocode-papers-fallback]];
// masstree.hh template-only). Planrunde wxciy2wjk.

#include "../topics/traversal/axis_03a_search_algo/composable/tier_to_organ_mapping.hpp" // #42: ObservableMasstreeOrgan
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
// Topic queuing (2 Achsen T15/T16 seit INC-2d; Doc 30 §8.0: historisch T17/T18) — Durchreich-Defaults NoBuffer/LazyFlush (Masstree = in-memory)
#include "../topics/queuing/axis_q1_queuing/axis_q1_queuing_no_buffer.hpp"
#include "../topics/queuing/axis_q2_queuing/axis_q2_queuing_lazy.hpp"

// R5.G
#include "../anatomy/composition_concept.hpp"

#include <string_view>

namespace comdare::cache_engine::compositions {

/// MasstreeComposition — Masstree als 15-Achsen-Permutations-Konfiguration.
///
/// Masstree Charakteristik (Mao/Kohler/Morris EuroSys 2012):
/// - B+/Trie-Hybrid mit Layer-Slice pro 8-Byte (axis_03a VectorU16U16SearchAlgo)
/// - Cache-Craftiness (axis_05 CacheLineAligned)
/// - Fine-grained OLC + Versioning (axis_08)
struct MasstreeComposition {
    using search_algo = traversal::axis_03a_search_algo::composable::
        ObservableMasstreeOrgan; // SEZIERT: B+Baum-of-Tries (kpermuter + Multi-Layer)
    using cache_traversal  = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping          = traversal::axis_03m_mapping::DirectPlacement;
    using path_compression = nodes::axis_02_path_compression::PathCompressionNone;
    using node_type =
        nodes::axis_04_node_type::ObservableNodeType<nodes::axis_04_node_type::Node256NodeType>; // V42 L-74c
    using memory_layout = memory_layout::axis_05_memory_layout::ObservableMemoryLayout<
        memory_layout::axis_05_memory_layout::CacheLineAlignedMemoryLayout>; // V42 L-74c
    using allocator     = allocator::axis_06_allocator::MimallocAllocator;
    using prefetch      = prefetch::axis_07_prefetch::NonePrefetch;
    using concurrency   = concurrency::axis_08_concurrency::OlcOptimisticConcurrency;
    using serialization = serialization::axis_10_serialization::ObservableSerialization<
        serialization::axis_10_serialization::RawBinarySerialization>; // V42 L-74c
    using value_handle       = value_handle::axis_14_value_handle::InlineValueHandle;
    using index_organization = search_engine::axis_01_index_organization::IotIndexOrganization;
    using io_dispatch        = io::axis_io::InMemoryOnly;
    using migration_policy   = migration::axis_migration::NoMigration;
    using filter             = filter::axis_filter::BloomFilter;
    // Topic queuing T15/T16 (INC-2d; Doc 30 §8.0: historisch T17/T18) — explizit gewaehlter Durchreich-Algorithmus (kein „weglassen")
    using queuing_q1 = queuing::axis_q1_queuing::NoBuffer;
    using queuing_q2 = queuing::axis_q2_queuing::LazyFlush;

    static constexpr std::string_view paper_id    = "P03 Mao/Kohler/Morris EuroSys 2012";
    static constexpr std::string_view paper_title = "Cache Craftiness for Fast Multicore Key-Value Storage";
    static constexpr std::string_view name        = "MasstreeComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::MasstreeComposition",
                                        "compositions/masstree_reference.hpp");
};

} // namespace comdare::cache_engine::compositions
