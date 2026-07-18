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
// V42 L-74c: die ObservableXxx-Huellen der 4 OperativeCapable-Achsen (via die erweiterten Strategie-Shims,
// die observable-Header + Namespace-Alias tragen) — all_axes_umbrella zieht sie nicht automatisch.
#include <topics/telemetry/axis_11_telemetry/axis_11_telemetry_observable.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/serialization/axis_10_serialization/axis_10_serialization_raw_binary.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_node256.hpp>
// Doc 30 §8.0: queuing q1/q2 als reguläre SA-Achsen T17/T18 — Durchreich-Defaults NoBuffer/LazyFlush.
#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_no_buffer.hpp>
#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_lazy.hpp>

COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(
    ::comdare::cache_engine::traversal::axis_03a_search_algo::Array256SearchAlgo,
    ::comdare::cache_engine::traversal::axis_03b_cache_traversal::LinearFanout,
    ::comdare::cache_engine::traversal::axis_03m_mapping::DirectPlacement,
    ::comdare::cache_engine::nodes::axis_02_path_compression::PathCompressionNone,
    // V42 L-74c: die 4 OperativeCapable-Achsen als ObservableXxx-Huellen (= Emitter-Output nach der
    // Registry-Umstellung, StaticAxisVariants→Huellen) → die DLL traegt telemetry/memory_layout/
    // serialization/node_type als getriebene ObservableAxis (telemetry/scan>0 ueber die echte .dll-Grenze).
    ::comdare::cache_engine::nodes::axis_04_node_type::ObservableNodeType<
        ::comdare::cache_engine::nodes::axis_04_node_type::Node256NodeType>,
    ::comdare::cache_engine::memory_layout::axis_05_memory_layout::ObservableMemoryLayout<
        ::comdare::cache_engine::memory_layout::axis_05_memory_layout::CacheLineAlignedMemoryLayout>,
    ::comdare::cache_engine::allocator::axis_06_allocator::MimallocAllocator,
    ::comdare::cache_engine::prefetch::axis_07_prefetch::NonePrefetch,
    ::comdare::cache_engine::concurrency::axis_08_concurrency::OlcOptimisticConcurrency,
    ::comdare::cache_engine::serialization::axis_10_serialization::ObservableSerialization<
        ::comdare::cache_engine::serialization::axis_10_serialization::RawBinarySerialization>,
    ::comdare::cache_engine::value_handle::axis_14_value_handle::InlineValueHandle,
    // Bau-INC-2d: isa (axis_09 Amd64Isa) entfaellt — Target-ISA-System-Achse, kein Kompositions-Slot mehr.
    ::comdare::cache_engine::search_engine::axis_01_index_organization::IotIndexOrganization,
    ::comdare::cache_engine::io::axis_io::InMemoryOnly, ::comdare::cache_engine::migration::axis_migration::NoMigration,
    ::comdare::cache_engine::filter::axis_filter::BloomFilter,
    // ── T17/T18 queuing (Doc 30 §8.0): explizit gewählter Durchreich-Algorithmus (kein „weglassen") ──
    ::comdare::cache_engine::queuing::axis_q1_queuing::NoBuffer,
    ::comdare::cache_engine::queuing::axis_q2_queuing::LazyFlush)
