#pragma once
// V41.F.6.1 F.2/F.3 — Axen-zentrische Namespace-Fassade (Erst-Inkrement, rueckwaerts-kompatibel).
//
// @topic axes (Querschnitt)
// @task #12 V41.F.2 (Axen-zentrische Namespaces) + #13 V41.F.3 (Achsen-Concepts)
//
// **Ziel-Struktur (User-Wortlaut, open-todos §261):**
//   comdare::cache_engine::                      // Tools/Werkzeuge (zentral)
//   comdare::cache_engine::<algorithm_axis>::    // ALLE Achsen-Interfaces aller Implementierungen
//   comdare::cache_engine::<axis>::optional_prt_art_impl  // Pruefling-Spezialisierung pro Achse
//   comdare::prt_art::                           // nur prt-art-spezifische Hilfen
//
// **Sichere Migrations-Strategie (root-cause statt Big-Bang-Rename):** die bestehenden topic-/achsen-
// basierten Namespaces (`comdare::cache_engine::<topic>::axis_NN_<name>`) bleiben die physische
// IMPLEMENTIERUNG; diese Fassade fuehrt die axen-zentrischen Namen als ALIASE ein, sodass ab sofort
// `comdare::cache_engine::lookup::Array256SearchAlgo` etc. nutzbar ist, OHNE einen Build-/Test-
// brechenden Zwischenzustand. Der physische Rename (Header-Verschiebung + Definition-Umbenennung) folgt
// inkrementell je Achse darauf auf (Doku 23, Migrationsplan); bis dahin sind alt + neu beide gueltig.
//
// **F.3 (Concepts):** je Achse wird das bereits bestehende, getestete Achsen-Concept unter dem
// axen-zentrischen abstrakten Namen re-exponiert (concept-Alias via `requires`-Delegation). Pilot:
// lookup/alloc/layout; die uebrigen folgen demselben Muster.

// F.3: Concept-Header ALLER 17 Achsen (fuer die abstrakten Achsen-Concepts unten). Traversal-Achsen
// (03a/03b/03m) tragen achsen-spezifische *Variant-Concepts; alloc/layout *Strategy; die uebrigen 12
// das uniforme CacheEnginePermutationStrategy.
#include <topics/traversal/axis_03a_search_algo/concepts/axis_03a_search_algo_concept.hpp>
#include <topics/traversal/axis_03b_cache_traversal/concepts/axis_03b_cache_traversal_concept.hpp>
#include <topics/traversal/axis_03m_mapping/concepts/axis_03m_mapping_concept.hpp>
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_concept.hpp>
#include <topics/memory_layout/axis_05_memory_layout/concepts/axis_05_memory_layout_concept.hpp>
#include <topics/nodes/axis_02_path_compression/concepts/axis_02_path_compression_cache_engine_permutation_concept.hpp>
#include <topics/nodes/axis_04_node_type/concepts/axis_04_node_type_cache_engine_permutation_concept.hpp>
#include <topics/prefetch/axis_07_prefetch/concepts/axis_07_prefetch_cache_engine_permutation_concept.hpp>
#include <topics/concurrency/axis_08_concurrency/concepts/axis_08_concurrency_cache_engine_permutation_concept.hpp>
#include <topics/serialization/axis_10_serialization/concepts/axis_10_serialization_cache_engine_permutation_concept.hpp>
#include <topics/telemetry/axis_11_telemetry/concepts/axis_11_telemetry_cache_engine_permutation_concept.hpp>
#include <topics/value_handle/axis_14_value_handle/concepts/axis_14_value_handle_cache_engine_permutation_concept.hpp>
#include <topics/hardware/axis_09_isa/concepts/axis_09_isa_cache_engine_permutation_concept.hpp>
#include <topics/search_engine/axis_01_index_organization/concepts/axis_01_index_organization_cache_engine_permutation_concept.hpp>
#include <topics/io/axis_io/concepts/axis_io_cache_engine_permutation_concept.hpp>
#include <topics/migration/axis_migration/concepts/axis_migration_cache_engine_permutation_concept.hpp>
#include <topics/filter/axis_filter/concepts/axis_filter_cache_engine_permutation_concept.hpp>

// Forward-Deklaration der physischen topic-/achsen-Namespaces, damit die Aliase auch ohne Include
// jedes Wrapper-Headers gueltig sind (der Nutzer inkludiert die konkreten Wrapper zusaetzlich).
namespace comdare::cache_engine::traversal::axis_03a_search_algo {}
namespace comdare::cache_engine::traversal::axis_03b_cache_traversal {}
namespace comdare::cache_engine::traversal::axis_03m_mapping {}
namespace comdare::cache_engine::nodes::axis_02_path_compression {}
namespace comdare::cache_engine::nodes::axis_04_node_type {}
namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {}
namespace comdare::cache_engine::allocator::axis_06_allocator {}
namespace comdare::cache_engine::prefetch::axis_07_prefetch {}
namespace comdare::cache_engine::concurrency::axis_08_concurrency {}
namespace comdare::cache_engine::serialization::axis_10_serialization {}
namespace comdare::cache_engine::telemetry::axis_11_telemetry {}
namespace comdare::cache_engine::value_handle::axis_14_value_handle {}
namespace comdare::cache_engine::hardware::axis_09_isa {}
namespace comdare::cache_engine::search_engine::axis_01_index_organization {}
namespace comdare::cache_engine::io::axis_io {}
namespace comdare::cache_engine::migration::axis_migration {}
namespace comdare::cache_engine::filter::axis_filter {}

namespace comdare::cache_engine {

// ─────────────────────────────────────────────────────────────────────────────
// F.2 — axen-zentrische Namespace-Aliase (17 Achsen). Ab sofort gueltige Zugriffs-Struktur.
// ─────────────────────────────────────────────────────────────────────────────
namespace lookup             = ::comdare::cache_engine::traversal::axis_03a_search_algo;       // Such-/Lookup-Algorithmus
namespace cache_traversal    = ::comdare::cache_engine::traversal::axis_03b_cache_traversal;   // Cache-Traversal
namespace mapping            = ::comdare::cache_engine::traversal::axis_03m_mapping;           // Slot-Mapping
namespace path_compression   = ::comdare::cache_engine::nodes::axis_02_path_compression;       // Pfad-Kompression
namespace node               = ::comdare::cache_engine::nodes::axis_04_node_type;              // Knoten-Typ
namespace layout             = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;  // Speicher-Layout
namespace alloc              = ::comdare::cache_engine::allocator::axis_06_allocator;          // Allokator
namespace prefetch_axis      = ::comdare::cache_engine::prefetch::axis_07_prefetch;            // Prefetch
namespace concurrency_axis   = ::comdare::cache_engine::concurrency::axis_08_concurrency;      // Concurrency
namespace serialization_axis = ::comdare::cache_engine::serialization::axis_10_serialization;  // Serialisierung
namespace telemetry_axis     = ::comdare::cache_engine::telemetry::axis_11_telemetry;          // Telemetrie
namespace value_handle_axis  = ::comdare::cache_engine::value_handle::axis_14_value_handle;    // Value-Handle
namespace simd               = ::comdare::cache_engine::hardware::axis_09_isa;                 // SIMD/ISA
namespace index_organization = ::comdare::cache_engine::search_engine::axis_01_index_organization; // Index-Organisation
// V41.F.2: io_dispatch ist jetzt der ECHTE physische Namespace (axes/io_dispatch/, Pilot-Migration) —
// KEIN Alias mehr. Rueckwaerts-Alias io::axis_io→io_dispatch liegt in den Forwarding-Headern (topics/io/axis_io/).
// V41.F.2: migration_policy ist jetzt der ECHTE physische Namespace (axes/migration_policy/) — KEIN Alias.
// Rueckwaerts-Alias migration::axis_migration→migration_policy liegt in den Forwardern (topics/migration/axis_migration/).
namespace filter_axis        = ::comdare::cache_engine::filter::axis_filter;                  // Filter (filter_axis: Topic-Namespace 'filter' existiert bereits)

// ─────────────────────────────────────────────────────────────────────────────
// F.3 — abstrakte Achsen-Concepts (Pilot: lookup/alloc/layout). Re-Exposition der bestehenden,
// getesteten Concepts unter dem axen-zentrischen abstrakten Namen (Delegation via requires).
// Jede Achse + jede Basisfunktionalitaet entspricht damit klar einem Concept (User-Direktive F.3).
// ─────────────────────────────────────────────────────────────────────────────
namespace concepts {

// F.3 — je Achse ein abstraktes Concept (Delegation auf das bestehende, getestete Achsen-Concept).
// Damit entspricht JEDE der 17 Achsen klar einem C++23-Concept (User-Direktive F.3).

// Traversal-Achsen: achsen-spezifische *Variant-Concepts.
template <typename T> concept LookupAxis =
    ::comdare::cache_engine::traversal::axis_03a_search_algo::concepts::SearchAlgoVariant<T>;
template <typename T> concept CacheTraversalAxis =
    ::comdare::cache_engine::traversal::axis_03b_cache_traversal::concepts::CacheTraversalVariant<T>;
template <typename T> concept MappingAxis =
    ::comdare::cache_engine::traversal::axis_03m_mapping::concepts::MappingVariant<T>;

// Allocator/Layout: achsen-spezifische *Strategy-Concepts.
template <typename T> concept AllocAxis =
    ::comdare::cache_engine::allocator::axis_06_allocator::concepts::AllocatorStrategy<T>;
template <typename T> concept LayoutAxis =
    ::comdare::cache_engine::memory_layout::axis_05_memory_layout::concepts::MemoryLayoutStrategy<T>;

// Uebrige 12 Achsen: das uniforme CacheEnginePermutationStrategy je Achse.
template <typename T> concept PathCompressionAxis =
    ::comdare::cache_engine::nodes::axis_02_path_compression::concepts::CacheEnginePermutationStrategy<T>;
template <typename T> concept NodeAxis =
    ::comdare::cache_engine::nodes::axis_04_node_type::concepts::CacheEnginePermutationStrategy<T>;
template <typename T> concept PrefetchAxis =
    ::comdare::cache_engine::prefetch::axis_07_prefetch::concepts::CacheEnginePermutationStrategy<T>;
template <typename T> concept ConcurrencyAxis =
    ::comdare::cache_engine::concurrency::axis_08_concurrency::concepts::CacheEnginePermutationStrategy<T>;
template <typename T> concept SerializationAxis =
    ::comdare::cache_engine::serialization::axis_10_serialization::concepts::CacheEnginePermutationStrategy<T>;
template <typename T> concept TelemetryAxis =
    ::comdare::cache_engine::telemetry::axis_11_telemetry::concepts::CacheEnginePermutationStrategy<T>;
template <typename T> concept ValueHandleAxis =
    ::comdare::cache_engine::value_handle::axis_14_value_handle::concepts::CacheEnginePermutationStrategy<T>;
template <typename T> concept SimdAxis =
    ::comdare::cache_engine::hardware::axis_09_isa::concepts::CacheEnginePermutationStrategy<T>;
template <typename T> concept IndexOrganizationAxis =
    ::comdare::cache_engine::search_engine::axis_01_index_organization::concepts::CacheEnginePermutationStrategy<T>;
template <typename T> concept IoDispatchAxis =
    ::comdare::cache_engine::io::axis_io::concepts::CacheEnginePermutationStrategy<T>;
template <typename T> concept MigrationPolicyAxis =
    ::comdare::cache_engine::migration::axis_migration::concepts::CacheEnginePermutationStrategy<T>;
template <typename T> concept FilterAxis =
    ::comdare::cache_engine::filter::axis_filter::concepts::CacheEnginePermutationStrategy<T>;

}  // namespace concepts

}  // namespace comdare::cache_engine

// ─────────────────────────────────────────────────────────────────────────────
// F.2 — optional_prt_art_impl-SLOT je Achse (Pruefling-Erweiterungspunkt), als echtes Code-Artefakt.
// Pro Achse ist `comdare::cache_engine::<axis>::optional_prt_art_impl` der reservierte Slot, in dem ein
// Pruefling (z.B. prt-art) seine pro-Achse-Spezialisierung der Achsen-Interfaces ablegt (muss das
// jeweilige Achsen-Concept aus `cache_engine::concepts::*Axis` erfuellen). Registrierung compile-time per
// CMake-Liste COMDARE_CE_PRUEFLINGE: cache-engine inkludiert je Pruefling+Achse
// `<cache_engine/<axis>/<pruefling>_impl.hpp>` falls vorhanden, sonst bleibt der Slot leer (Dummy).
// Hier als leere Sub-Namespaces im jeweiligen (physischen) Achsen-Namespace deklariert → ueber die
// Achsen-Aliase oben adressierbar (z.B. cache_engine::lookup::optional_prt_art_impl).
// ─────────────────────────────────────────────────────────────────────────────
namespace comdare::cache_engine::traversal::axis_03a_search_algo       { namespace optional_prt_art_impl {} }
namespace comdare::cache_engine::traversal::axis_03b_cache_traversal   { namespace optional_prt_art_impl {} }
namespace comdare::cache_engine::traversal::axis_03m_mapping           { namespace optional_prt_art_impl {} }
namespace comdare::cache_engine::nodes::axis_02_path_compression       { namespace optional_prt_art_impl {} }
namespace comdare::cache_engine::nodes::axis_04_node_type              { namespace optional_prt_art_impl {} }
namespace comdare::cache_engine::memory_layout::axis_05_memory_layout  { namespace optional_prt_art_impl {} }
namespace comdare::cache_engine::allocator::axis_06_allocator          { namespace optional_prt_art_impl {} }
namespace comdare::cache_engine::prefetch::axis_07_prefetch            { namespace optional_prt_art_impl {} }
namespace comdare::cache_engine::concurrency::axis_08_concurrency      { namespace optional_prt_art_impl {} }
namespace comdare::cache_engine::serialization::axis_10_serialization  { namespace optional_prt_art_impl {} }
namespace comdare::cache_engine::telemetry::axis_11_telemetry          { namespace optional_prt_art_impl {} }
namespace comdare::cache_engine::value_handle::axis_14_value_handle    { namespace optional_prt_art_impl {} }
namespace comdare::cache_engine::hardware::axis_09_isa                 { namespace optional_prt_art_impl {} }
namespace comdare::cache_engine::search_engine::axis_01_index_organization { namespace optional_prt_art_impl {} }
namespace comdare::cache_engine::io::axis_io                           { namespace optional_prt_art_impl {} }
namespace comdare::cache_engine::migration::axis_migration             { namespace optional_prt_art_impl {} }
namespace comdare::cache_engine::filter::axis_filter                   { namespace optional_prt_art_impl {} }
