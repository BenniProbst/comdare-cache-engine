// tests/unit/naht1_visitor_module.cpp -- NAHT-1 BISS, die MODUL-SEITE (09.08.2026, Owner-KERN).
//
// EIN Quelltext, ZWEI .so. Die Komposition ist in beiden Bauten BYTE-IDENTISCH (dieselben 18 Achsen,
// derselbe Adapter); der einzige Unterschied ist der PRAEPROZESSOR-ZUSTAND der Mess-Gates:
//   naht1_modul_mess_an    COMDARE_MEASUREMENT_ON=1, COMDARE_CE_ENABLE_STATISTICS=1
//   naht1_modul_mess_aus   keines davon (COMDARE_NAHT1_GATES_AUS -> die #undef-Praeparation unten)
//
// WAS DIE ZWEI SCHWESTERN BELEGEN SOLLEN -- und warum es ZWEI sein muessen:
//   Ein Koeder, der nur die AN-Seite prueft, kann nicht zwischen "die Naht traegt Werte" und "der
//   Test misst irgendetwas Benachbartes" unterscheiden. Erst die AUS-Schwester macht die Aussage
//   scharf: DIESELBE Komposition, DERSELBE Quelltext, nur die Gates anders -- und die Mess-Flaeche
//   ist WEG (dynamic_cast nullptr, Symbol nicht in der Tabelle). Ein Koeder, der auch an der
//   AUS-Schwester beisst, haette nie die Naht gemessen.
//
// WARUM DIE ABSCHALTUNG PER #undef UND NICHT PER "target ohne -D": die Wurzel-CMakeLists setzt
// COMDARE_MEASUREMENT_ON=1 per add_compile_definitions GLOBAL (MEASUREMENT_MODE=ON ist Default). Ein
// Target kann eine Verzeichnis-Definition nicht abwaehlen. Die #undef-Praeparation VOR jedem Include
// ist der im Baum bereits sanktionierte, portable Weg -- Praezedenz r3_mess_gate_stamp_module.cpp und
// test_a8s4_release_pfad_neutralitaet.cpp. Der so praeparierte Zustand IST der Release-Baumodus
// (COMDARE_RELEASE_MODE=ON), also ein real erreichbarer Bau, kein Kunstgriff.
//
// ASCII-only.

#ifdef COMDARE_NAHT1_GATES_AUS
#undef COMDARE_MEASUREMENT_ON
#undef COMDARE_CE_ENABLE_STATISTICS
#undef COMDARE_EXPERIMENT_MODE_ON
#if defined(COMDARE_MEASUREMENT_ON) && COMDARE_MEASUREMENT_ON
#error "NAHT-1 BISS: die MESS-AUS-Variante muss ohne COMDARE_MEASUREMENT_ON uebersetzen."
#endif
#ifdef COMDARE_CE_ENABLE_STATISTICS
#error "NAHT-1 BISS: die MESS-AUS-Variante muss ohne COMDARE_CE_ENABLE_STATISTICS uebersetzen."
#endif
#else
// Die MESS-AN-Variante bekommt ihre Defines vom Target; die Wachen halten fest, dass sie WIRKLICH
// ankommen -- ein Biss, dessen beide Seiten still denselben Zustand haetten, waere kein Biss.
#if !defined(COMDARE_MEASUREMENT_ON) || !COMDARE_MEASUREMENT_ON
#error "NAHT-1 BISS: die MESS-AN-Variante braucht COMDARE_MEASUREMENT_ON=1 (target_compile_definitions)."
#endif
#ifndef COMDARE_CE_ENABLE_STATISTICS
#error "NAHT-1 BISS: die MESS-AN-Variante braucht COMDARE_CE_ENABLE_STATISTICS=1."
#endif
#endif

#include <builder/codegen/all_axes_umbrella.hpp>
// V42 L-74c: die ObservableXxx-Huellen der 4 OperativeCapable-Achsen (via die erweiterten Strategie-Shims,
// die observable-Header + Namespace-Alias tragen) -- all_axes_umbrella zieht sie nicht automatisch.
#include <topics/telemetry/axis_11_telemetry/axis_11_telemetry_observable.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/serialization/axis_10_serialization/axis_10_serialization_raw_binary.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_node256.hpp>
// Doc 30 §8.0: queuing q1/q2 als regulaere SA-Achsen T17/T18 -- Durchreich-Defaults NoBuffer/LazyFlush.
#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_no_buffer.hpp>
#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_lazy.hpp>
// STRUKT-R ORG-18: 18. Organ-Slot (der Umbrella zieht die Registry, der Baustein-Header explizit).
#include <axes/persistence_target/axis_persistence_target_memory_only.hpp>

COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(
    ::comdare::cache_engine::traversal::axis_03a_search_algo::Array256SearchAlgo,
    ::comdare::cache_engine::traversal::axis_03b_cache_traversal::LinearFanout,
    ::comdare::cache_engine::traversal::axis_03m_mapping::DirectPlacement,
    ::comdare::cache_engine::nodes::axis_02_path_compression::PathCompressionNone,
    // V42 L-74c: die 4 OperativeCapable-Achsen als ObservableXxx-Huellen (= Emitter-Output nach der
    // Registry-Umstellung, StaticAxisVariants->Huellen) -> die DLL traegt telemetry/memory_layout/
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
    // Bau-INC-2d: isa (axis_09 Amd64Isa) entfaellt -- Target-ISA-System-Achse, kein Kompositions-Slot mehr.
    ::comdare::cache_engine::search_engine::axis_01_index_organization::IotIndexOrganization,
    ::comdare::cache_engine::io::axis_io::InMemoryOnly, ::comdare::cache_engine::migration::axis_migration::NoMigration,
    ::comdare::cache_engine::filter::axis_filter::BloomFilter,
    // -- T17/T18 queuing (Doc 30 §8.0): explizit gewaehlter Durchreich-Algorithmus (kein "weglassen") --
    ::comdare::cache_engine::queuing::axis_q1_queuing::NoBuffer,
    ::comdare::cache_engine::queuing::axis_q2_queuing::LazyFlush,
    // STRUKT-R ORG-18: T17 persistence_target -- ebenso explizit gewaehlter Durchreich-Wert (kein Weglassen).
    ::comdare::cache_engine::persistence_target::MemoryOnlyTarget)
