#pragma once
// V41.F.6.1.R3 — Composition-Concept fuer Saeugetier-Anatomie
//
// Validiert dass jede Composition alle 17 Pflicht-using-Aliases (15 Topics +
// 2 zusaetzliche Achsen aus traversal/nodes) UND paper_id/name liefert.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §11.2

#include <concepts>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// IsComposition — Pflicht-Concept fuer SearchAlgorithmAnatomy<C>.
///
/// Saeugetier-Anatomie-Metapher: jede Composition muss ALLE 17 "Organe"
/// nennen — auch wenn die Auspraegung NoMigration/PathCompressionNone etc. ist.
template <typename C>
concept IsComposition = requires {
    // Topic 3 traversal (3 Achsen)
    typename C::search_algo;
    typename C::cache_traversal;
    typename C::mapping;
    // Topic 4 nodes (2 Achsen)
    typename C::path_compression;
    typename C::node_type;
    // Topic 5
    typename C::memory_layout;
    // Topic 6
    typename C::allocator;
    // Topic 7
    typename C::prefetch;
    // Topic 8
    typename C::concurrency;
    // Topic 10
    typename C::serialization;
    // Topic 11
    typename C::telemetry;
    // Topic 14
    typename C::value_handle;
    // Topic hardware
    typename C::isa;
    // Topic search_engine
    typename C::index_organization;
    // Topic io
    typename C::io_dispatch;
    // Topic migration
    typename C::migration_policy;
    // Topic filter
    typename C::filter;
    // Identifikation
    { C::name }     -> std::convertible_to<std::string_view>;
    { C::paper_id } -> std::convertible_to<std::string_view>;
};

/// Helper: zaehlt zur Compile-Zeit wie viele "Organe" eine Composition liefert.
/// Pflicht: 17 (3 traversal + 2 nodes + 12 weitere Topics).
template <typename C>
struct composition_organ_count {
    static constexpr std::size_t value = 17;  // 3 + 2 + 12
};

}  // namespace comdare::cache_engine::anatomy
