#pragma once
// STRANG A KORRIGIERT — Increment 5 / S6a (2026-06-18). PrtArtComposition: PRT-ART als reale
// 19-Achsen-Komposition (das Thesis-EIGENE Prüfling-Lebewesen, #8/#162 P-MD6).
//
// PRT-ART = "Probst Redirect Tree — ART" (Memory feedback_namens_inkonsistenzen / Task #11). Es ist
// KEINE neue Selektion, sondern eine reale AdHocComposition-Ausprägung: identisch zu ArtComposition
// (adaptiver Node4/16/48/256-Radix-Trie), ABER mit dem REDIRECT-Organ in der path_compression-Achse
// (PatriciaPathCompression = kollabierter eindeutiger Restpfad, CoCo-/Redirect-Idee — vgl.
// nodes/axis_01_page_type/axis_01_page_type_redirect.hpp RedirectPageType, „collapsed unique
// remainder-path"). Damit ist PRT-ART eine ECHT distinkte Komposition gegenüber dem reinen ART
// (das PathCompressionNone trägt) — distinkte binary_id, eigene reale DLL.
//
// Diese Komposition ist der Pruefling-VARIANT, der in den SOTA-Reihen B (Stufe2_PrueflingReplace)
// und C (Stufe3_FullJoin) den path_compression-Slot belegt (siehe prt_art_merge_reference.hpp +
// pruefling_merge.hpp MergeAxis). Die Gattung ist SearchAlgorithm (assert_pruefling_slot_genus).
//
// @paper PRT (Probst Redirect Tree, ART-basiert; #8 Prüfling-Einbindung)
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §18-§19 (3 Kompositionale Joins)

#include "../topics/traversal/axis_03a_search_algo/composable/tier_to_organ_mapping.hpp"
#include "../topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_linear_fanout.hpp"
#include "../topics/traversal/axis_03m_mapping/axis_03m_mapping_direct_placement.hpp"
// Topic-4 nodes — DER UNTERSCHIED zu ART: Patricia-Pfadkompression (Redirect/collapsed-path = das „R" in PRT).
// HINWEIS: der path_compression-Slot der AdHocComposition trägt die BARE Strategie (PatriciaPathCompression
// erfüllt PathCompressionStrategy mit compression_ratio()) — NICHT die ObservablePathCompression-Hülle (die
// compression_ratio() NICHT durchreicht und so das Konzept im abi_adapter bräche). Parität zu ArtComposition,
// die ebenfalls die bare PathCompressionNone trägt.
#include "../topics/nodes/axis_02_path_compression/axis_02_path_compression_patricia.hpp"
#include "../topics/nodes/axis_04_node_type/axis_04_node_type_node256.hpp"
#include "../topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp"
#include "../topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp"
#include "../topics/prefetch/axis_07_prefetch/axis_07_prefetch_none.hpp"
#include "../topics/concurrency/axis_08_concurrency/axis_08_concurrency_olc.hpp"
#include "../topics/serialization/axis_10_serialization/axis_10_serialization_raw_binary.hpp"
#include "../topics/telemetry/axis_11_telemetry/axis_11_telemetry_leaf_only.hpp"
#include "../topics/telemetry/axis_11_telemetry/axis_11_telemetry_observable.hpp"
#include "../topics/value_handle/axis_14_value_handle/axis_14_value_handle_inline.hpp"
#include "../topics/hardware/axis_09_isa/axis_09_isa_amd64.hpp"
#include "../topics/search_engine/axis_01_index_organization/axis_01_index_organization_index_organized_table.hpp"
#include "../topics/io/axis_io/axis_io_in_memory_only.hpp"
#include "../topics/migration/axis_migration/axis_migration_none.hpp"
#include "../topics/filter/axis_filter/axis_filter_bloom.hpp"
#include "../topics/queuing/axis_q1_queuing/axis_q1_queuing_no_buffer.hpp"
#include "../topics/queuing/axis_q2_queuing/axis_q2_queuing_lazy.hpp"

#include "../anatomy/composition_concept.hpp"

#include <string_view>

namespace comdare::cache_engine::compositions {

/// PrtArtPathCompressionOrgan — das REDIRECT-Organ des Prüflings: die bare Patricia-(collapsed-path)-Strategie.
/// Dieses Organ ist der Pruefling-Slot-Wert, den die SOTA-Reihen B/C in eine Host-Komposition einsetzen
/// (pruefling_merge::MergeAxis path_compression-Achse). Bare (nicht observable-gehüllt), damit es
/// PathCompressionStrategy (compression_ratio()) erfüllt — der abi_adapter verlangt das Konzept im Slot.
using PrtArtPathCompressionOrgan = nodes::axis_02_path_compression::PatriciaPathCompression;

/// PrtArtComposition — PRT-ART als 19-Achsen-Komposition (Reihe-A Stufe1: das Prüfling-Lebewesen isoliert).
/// = ArtComposition mit path_compression = PrtArtPathCompressionOrgan (Redirect/Patricia statt None).
struct PrtArtComposition {
    using search_algo      = traversal::axis_03a_search_algo::composable::ObservableArtTrieOrgan;
    using cache_traversal  = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping          = traversal::axis_03m_mapping::DirectPlacement;
    using path_compression = PrtArtPathCompressionOrgan; // ← der PRT-Redirect-Slot (das „R" in PRT-ART)
    using node_type        = nodes::axis_04_node_type::ObservableNodeType<nodes::axis_04_node_type::Node256NodeType>;
    using memory_layout    = memory_layout::axis_05_memory_layout::ObservableMemoryLayout<
        memory_layout::axis_05_memory_layout::CacheLineAlignedMemoryLayout>;
    using allocator     = allocator::axis_06_allocator::MimallocAllocator;
    using prefetch      = prefetch::axis_07_prefetch::NonePrefetch;
    using concurrency   = concurrency::axis_08_concurrency::OlcOptimisticConcurrency;
    using serialization = serialization::axis_10_serialization::ObservableSerialization<
        serialization::axis_10_serialization::RawBinarySerialization>;
    using telemetry = telemetry::axis_11_telemetry::ObservableTelemetry<telemetry::axis_11_telemetry::LeafOnlyCounter>;
    using value_handle       = value_handle::axis_14_value_handle::InlineValueHandle;
    using isa                = hardware::axis_09_isa::Amd64Isa;
    using index_organization = search_engine::axis_01_index_organization::IotIndexOrganization;
    using io_dispatch        = io::axis_io::InMemoryOnly;
    using migration_policy   = migration::axis_migration::NoMigration;
    using filter             = filter::axis_filter::BloomFilter;
    using queuing_q1         = queuing::axis_q1_queuing::NoBuffer;
    using queuing_q2         = queuing::axis_q2_queuing::LazyFlush;

    static constexpr std::string_view paper_id = "PRT Probst Redirect Tree (ART-basiert)";
    static constexpr std::string_view paper_title =
        "Probst Redirect Tree — ART trie with collapsed-remainder-path (Patricia) redirect organ";
    static constexpr std::string_view name = "PrtArtComposition";

    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::PrtArtComposition",
                                        "compositions/prt_art_reference.hpp");
};

} // namespace comdare::cache_engine::compositions
