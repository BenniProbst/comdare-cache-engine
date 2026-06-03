#pragma once
// V41.F.6.1 R5.G — Umbrella-Header: bündelt ALLE 19 Anatomie-Achsen-Registries (+ Varianten) + die
// AdHoc-Composition + das ADHOC-Materialisierungs-Makro in EINEM Include.
//
// Zweck (Auto-Emitter-Plumbing): ein vom Auto-Emitter generiertes Permutations-Modul-.cpp muss alle
// Achsen-Vendor-Typen JEDER Permutation auflösen können. Statt pro Modul die spezifischen Achsen-Header
// zu kennen, inkludiert es nur diesen Umbrella-Header + ruft COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(<19
// FQ-Typ-Namen>) auf (die der Emitter via type_name<T>() aus for_each_composition_type ableitet).
//
// Reihenfolge der 19 Achsen (= AdHocComposition<T0..T18>, Doc 30 §8.0): search_algo, cache_traversal,
// mapping, path_compression, node_type, memory_layout, allocator, prefetch, concurrency, serialization,
// telemetry, value_handle, isa, index_organization, io_dispatch, migration_policy, filter,
// queuing_q1 (buffer_strategy), queuing_q2 (flush_policy).

// Materialisierungs-Makro + AdHocComposition
#include <cache_engine/abi/anatomy_module_abi_v1.hpp>
#include <anatomy/composition_factory.hpp>

// Die 17 Achsen-Registries (jede inkludiert alle Varianten ihrer Achse)
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_registry.hpp>          // T0  search_algo
#include <topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_registry.hpp>  // T1  cache_traversal
#include <topics/traversal/axis_03m_mapping/axis_03m_mapping_registry.hpp>                  // T2  mapping
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_registry.hpp>      // T3  path_compression
#include <topics/nodes/axis_04_node_type/axis_04_node_type_registry.hpp>                    // T4  node_type
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_registry.hpp>    // T5  memory_layout
#include <topics/allocator/axis_06_allocator/axis_06_allocator_registry.hpp>                // T6  allocator
#include <topics/prefetch/axis_07_prefetch/axis_07_prefetch_registry.hpp>                   // T7  prefetch
#include <topics/concurrency/axis_08_concurrency/axis_08_concurrency_registry.hpp>          // T8  concurrency
#include <topics/serialization/axis_10_serialization/axis_10_serialization_registry.hpp>    // T9  serialization
#include <topics/telemetry/axis_11_telemetry/axis_11_telemetry_registry.hpp>                // T10 telemetry
#include <topics/value_handle/axis_14_value_handle/axis_14_value_handle_registry.hpp>       // T11 value_handle
#include <topics/hardware/axis_09_isa/axis_09_isa_registry.hpp>                             // T12 isa
#include <topics/search_engine/axis_01_index_organization/axis_01_index_organization_registry.hpp> // T13 index_organization
#include <topics/io/axis_io/axis_io_registry.hpp>                                           // T14 io_dispatch
#include <topics/migration/axis_migration/axis_migration_registry.hpp>                      // T15 migration_policy
#include <topics/filter/axis_filter/axis_filter_registry.hpp>                               // T16 filter
// Doc 30 §8.0: queuing als reguläre SA-Achse (q1 buffer_strategy / q2 flush_policy). Die Registries ziehen
// alle Varianten inkl. der Durchreich-Defaults NoBuffer (q1) / LazyFlush (q2), die ein nicht-pufferndes Tier wählt.
#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_registry.hpp>                       // T17 queuing_q1
#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_registry.hpp>                       // T18 queuing_q2

// V42 L-74c (2026-06-03, L-LAZY-E2E): die ObservableXxx<Inner>-HÜLLEN. Die TopicConfigSets (BR-1) dekorieren
// node_type/memory_layout/serialization/telemetry via mp_transform<make_observable_*, Enabled*> → die realen
// Komposition-Slots SIND `ObservableNodeType<…>` etc. Der TYP-getriebene Emitter (adhoc_macro_args) schreibt
// genau diese dekorierten FQ-Namen in den Modul-Quelltext → die Hüllen-Header MÜSSEN über den Umbrella sichtbar
// sein, sonst „ObservableSerialization ist kein Member" (C3878, Befund 2026-06-03 beim ersten realen perm-DLL-Build).
#include <axes/node/axis_04_node_type_observable.hpp>             // ObservableNodeType<S>
#include <axes/layout/axis_05_memory_layout_observable.hpp>       // ObservableMemoryLayout<S>
#include <axes/serialization_axis/axis_10_serialization_observable.hpp>  // ObservableSerialization<S>
#include <axes/telemetry_axis/axis_11_telemetry_observable.hpp>   // ObservableTelemetry<S>
