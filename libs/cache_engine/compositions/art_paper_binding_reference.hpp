#pragma once
// V41.F.6.1.R3.2 ArtPaperBindingComposition (15 Achsen, Paper-Bindung)
//
// Identisch zu ArtComposition AUSSER search_algo:
//   - ArtComposition           → Array256SearchAlgo (CE-Re-Impl Dense)
//   - ArtPaperBindingComposition → OriginalArtSearchAlgo (P01 ART, Habich-SHA256)
//
// Beweis: beide instantiieren SearchAlgorithmAnatomy<C> mit identischen 16 Organen
// und unterschiedlichem Skelett (search_algo). Tier-Organ-Metapher demonstriert
// dass alternative Auspraegungen einer einzelnen Achse legitime "Tier-Varianten" sind.
//
// @paper P01 ART (Leis/Kemper/Neumann ICDE 2013)
// @paper_binding OriginalArtSearchAlgo (S04 im Registry, unodb::db Paper-Source)
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §14.2 (KORRIGIERT)

// #42 Umstufung-B: search_algo = SEZIERTES ART-Organ (Composition statt Tier). Der OriginalArtSearchAlgo-
// Wrapper bleibt included, dient aber nur noch als paper_source-Provenienz-Traeger (Habich/SHA256/is_original),
// NICHT mehr als Achsen-Wert (Doku 14 §3.4; Memory feedback_legacy_code_sha256_validation).
#include "../topics/traversal/axis_03a_search_algo/composable/tier_to_organ_mapping.hpp"
#include "../topics/traversal/axis_03a_search_algo/axis_03a_search_algo_original_art.hpp"
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

/// ArtPaperBindingComposition — ART mit Habich-konformer Paper-Bindung (OriginalArtSearchAlgo).
///
/// Unterschied zu ArtComposition: search_algo wechselt von Array256SearchAlgo (CE-Re-Impl)
/// zu OriginalArtSearchAlgo (Paper-Original mit SHA256-Validation gegen art.hpp).
/// Alle anderen 16 Achsen identisch.
struct ArtPaperBindingComposition {
    using search_algo        = traversal::axis_03a_search_algo::composable::ObservableArtTrieOrgan;
    // Provenienz-Slot (#42): traegt is_original/SHA256/get_compiler (Habich, Doku 14 §3.4), OHNE Achsen-Wert
    // zu sein. So bleibt die Paper-Bindung ueber die Composition erreichbar, das Skelett ist aber ein Organ.
    using paper_source       = traversal::axis_03a_search_algo::OriginalArtSearchAlgo;
    using cache_traversal    = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping            = traversal::axis_03m_mapping::DirectPlacement;
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

    static constexpr std::string_view paper_id    = "P01 Leis/Kemper/Neumann ICDE 2013 (Paper-Binding)";
    static constexpr std::string_view paper_title = "The Adaptive Radix Tree: ARTful Indexing (unodb::db Paper-Source)";
    static constexpr std::string_view name        = "ArtPaperBindingComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION(
        "::comdare::cache_engine::compositions::ArtPaperBindingComposition",
        "compositions/art_paper_binding_reference.hpp");
};

}  // namespace
