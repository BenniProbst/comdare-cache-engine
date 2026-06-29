// L-MEAS-THESIS — echtes SearchAlgorithm-DLL-„Tier" für den Diplomarbeit-Mess-Anhang (Gattung Suchalgorithmen).
// EINE DLL = 17-Slot-SearchAlgorithm-Anatomie (4 ABI-Symbole, genus()==SearchAlgorithm) + 3-Build-Achsen-Inspection.
// Isolierter Achsen-Vergleich (F15): die 16 NICHT-Such-Achsen sind über ALLE thesis_sa_*-Tiere IDENTISCH; variiert
// wird AUSSCHLIESSLICH axis_03a_search_algo (das Kern-Such-Organ) → der gemessene Unterschied ist dem Such-Organ
// zurechenbar. Node-Organ-Variante: Node4NodeType (axis_04; vs Node256-Baseline). cl /LD /DCOMDARE_ANATOMY_MODULE_BUILD (CMake-Kontext, schwer).
#include <cache_engine/abi/anatomy_module_abi_v1.hpp>
#include <builder/codegen/all_axes_umbrella.hpp>
#include <topics/nodes/axis_01_page_type/axis_01_page_type_dense_byte.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_avx512.hpp>
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_x86_64.hpp>
// Doc 30 §8.0: queuing q1/q2 als reguläre SA-Achsen T17/T18 — Durchreich-Defaults NoBuffer/LazyFlush.
#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_no_buffer.hpp>
#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_lazy.hpp>

COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_BUILDVARIANT(
    ::comdare::cache_engine::nodes::axis_01_page_type::DenseBytePageType,
    ::comdare::cache_engine::hardware::axis_09b_simd_extension::Avx512SimdExtension,
    ::comdare::cache_engine::hardware::axis_12_general_hardware::X86_64HardwareProfile,
    // ── axis_03a (VARIIERTES Such-Organ) ──
    ::comdare::cache_engine::traversal::axis_03a_search_algo::Array256SearchAlgo,
    // ── die übrigen 16 Achsen FIX (identisch über alle thesis_sa_*-Tiere) ──
    ::comdare::cache_engine::traversal::axis_03b_cache_traversal::LinearFanout,
    ::comdare::cache_engine::traversal::axis_03m_mapping::DirectPlacement,
    ::comdare::cache_engine::nodes::axis_02_path_compression::PathCompressionNone,
    ::comdare::cache_engine::nodes::axis_04_node_type::Node4NodeType,
    ::comdare::cache_engine::memory_layout::axis_05_memory_layout::CacheLineAlignedMemoryLayout,
    ::comdare::cache_engine::allocator::axis_06_allocator::MimallocAllocator,
    ::comdare::cache_engine::prefetch::axis_07_prefetch::NonePrefetch,
    ::comdare::cache_engine::concurrency::axis_08_concurrency::OlcOptimisticConcurrency,
    ::comdare::cache_engine::serialization::axis_10_serialization::RawBinarySerialization,
    ::comdare::cache_engine::telemetry::axis_11_telemetry::LeafOnlyCounter,
    ::comdare::cache_engine::value_handle::axis_14_value_handle::InlineValueHandle,
    ::comdare::cache_engine::hardware::axis_09_isa::Amd64Isa,
    ::comdare::cache_engine::search_engine::axis_01_index_organization::IotIndexOrganization,
    ::comdare::cache_engine::io::axis_io::InMemoryOnly, ::comdare::cache_engine::migration::axis_migration::NoMigration,
    ::comdare::cache_engine::filter::axis_filter::BloomFilter,
    // ── T17/T18 queuing (Doc 30 §8.0): explizit gewählter Durchreich-Algorithmus (kein „weglassen") ──
    ::comdare::cache_engine::queuing::axis_q1_queuing::NoBuffer,
    ::comdare::cache_engine::queuing::axis_q2_queuing::LazyFlush)
