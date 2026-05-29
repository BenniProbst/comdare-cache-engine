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

// F.3-Pilot: Concept-Header der drei Pilot-Achsen (fuer die abstrakten Achsen-Concepts unten).
#include <topics/traversal/axis_03a_search_algo/concepts/axis_03a_search_algo_concept.hpp>
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_concept.hpp>
#include <topics/memory_layout/axis_05_memory_layout/concepts/axis_05_memory_layout_concept.hpp>

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
namespace io_dispatch        = ::comdare::cache_engine::io::axis_io;                           // IO-Dispatch
namespace migration_policy   = ::comdare::cache_engine::migration::axis_migration;            // Migrations-Policy
namespace filter_axis        = ::comdare::cache_engine::filter::axis_filter;                  // Filter (filter_axis: Topic-Namespace 'filter' existiert bereits)

// ─────────────────────────────────────────────────────────────────────────────
// F.3 — abstrakte Achsen-Concepts (Pilot: lookup/alloc/layout). Re-Exposition der bestehenden,
// getesteten Concepts unter dem axen-zentrischen abstrakten Namen (Delegation via requires).
// Jede Achse + jede Basisfunktionalitaet entspricht damit klar einem Concept (User-Direktive F.3).
// ─────────────────────────────────────────────────────────────────────────────
namespace concepts {

/// Achse lookup: ein Such-Algorithmus erfuellt das Lookup-Achsen-Concept (= SearchAlgoVariant).
template <typename T>
concept LookupAxis = ::comdare::cache_engine::traversal::axis_03a_search_algo::concepts::SearchAlgoVariant<T>;

/// Achse alloc: ein Allokator erfuellt das Allocator-Achsen-Concept (= AllocatorStrategy).
template <typename T>
concept AllocAxis = ::comdare::cache_engine::allocator::axis_06_allocator::concepts::AllocatorStrategy<T>;

/// Achse layout: ein Speicher-Layout erfuellt das Layout-Achsen-Concept (= MemoryLayoutStrategy).
template <typename T>
concept LayoutAxis = ::comdare::cache_engine::memory_layout::axis_05_memory_layout::concepts::MemoryLayoutStrategy<T>;

}  // namespace concepts

}  // namespace comdare::cache_engine

// HINWEIS optional_prt_art_impl-Slot (F.2): pro Achse ist der Sub-Namespace
//   comdare::cache_engine::<axis>::optional_prt_art_impl
// der reservierte Slot, in dem ein Pruefling (z.B. prt-art) seine pro-Achse-Spezialisierung der
// Achsen-Interfaces ablegt; per CMake-Liste COMDARE_CE_PRUEFLINGE compile-time registriert
// (open-todos §278-281). Wird beim physischen Rename je Achse materialisiert (Doku 23).
