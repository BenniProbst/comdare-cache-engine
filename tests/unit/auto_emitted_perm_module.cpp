// V41.F.6.1 R5.G — AUTO-EMITTER-Modul (repräsentativ): genau die Ausgabe des Auto-Emitters für EINE
// Permutation (vgl. R5G_AutoEmitter.EmitsModuleCppPerPermutation, das exakt diesen Inhalt schreibt).
//
// Wird als SHARED-Lib zu einer Permutations-DLL gebaut → schließt die R5.G-Materialisierung end-to-end:
// eine AUTO-ENUMERIERTE AdHoc-Komposition (NICHT eine benannte Composition wie bei den F.5-Pilots) wird
// zu einer ladbaren, mess-fähigen DLL. Der einzige Include ist der Umbrella-Header; das ADHOC-Makro baut
// AdHocComposition<17 Achsen> intern + exportiert die 4 extern-C-ABI-Symbole.
//
// Achsen-Belegung = Art-Default-Permutation (Array256/LinearFanout/.../Bloom).

#include <builder/codegen/all_axes_umbrella.hpp>

COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(
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
    ::comdare::cache_engine::filter::axis_filter::BloomFilter)
