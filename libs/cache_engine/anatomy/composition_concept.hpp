#pragma once
// V41.F.6.1.R3 — Composition-Concept fuer Saeugetier-Anatomie
//
// Validiert dass jede Composition alle 19 Pflicht-using-Aliases (15 Topics +
// 2 zusaetzliche Achsen aus traversal/nodes + queuing q1/q2) UND paper_id/name liefert.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §11.2
// @doku docs/architecture/30_audit_achsen_delegation_pflichtachsen.md §8.0 (queuing = SA-Achse)

#include <concepts>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// IsComposition — Pflicht-Concept fuer SearchAlgorithmAnatomy<C>.
///
/// Saeugetier-Anatomie-Metapher: jede Composition muss ALLE 19 "Organe"
/// nennen — auch wenn die Auspraegung NoMigration/PathCompressionNone/NoBuffer etc. ist.
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
    // Topic 14
    typename C::value_handle;
    // Topic hardware — Bau-INC-2d: isa ist Target-ISA-System-Achse, KEIN Kompositions-Slot mehr
    // Topic search_engine
    typename C::index_organization;
    // Topic io
    typename C::io_dispatch;
    // Topic migration
    typename C::migration_policy;
    // Topic filter
    typename C::filter;
    // Topic queuing (Doc 30 §8.0: q1 buffer_strategy + q2 flush_policy — reguläre SA-Achse)
    typename C::queuing_q1;
    typename C::queuing_q2;
    // Identifikation
    { C::name } -> std::convertible_to<std::string_view>;
    { C::paper_id } -> std::convertible_to<std::string_view>;
};

/// Helper: zaehlt zur Compile-Zeit wie viele "Organe" eine Composition liefert.
/// Pflicht: 17 (3 traversal + 2 nodes + 10 weitere Topics + 2 queuing q1/q2; Doc 30 §8.0
/// i.V.m. Bau-INC-2c: telemetry ist System-Achse; Bau-INC-2d/ABI-6: isa ist Target-ISA-System-Achse —
/// beide kein Organ-Slot mehr).
template <typename C>
struct composition_organ_count {
    static constexpr std::size_t value = 17; // 3 + 2 + 10 + 2 queuing
};

// ─────────────────────────────────────────────────────────────────────────────
// R5.G — HasCompositionLocation (Optional-Trait fuer CMake-Codegen)
// ─────────────────────────────────────────────────────────────────────────────

/// HasCompositionLocation — Optional-Concept das die Codegen-Tool-Lokalisierung
/// einer Composition liefert. Reference-Compositions (Art/Hot/Wormhole/SuRF/
/// Masstree/Start + 5 PaperBindings) erfuellen das. AdHocComposition<...>
/// erfuellt es NICHT (generisches Cartesian-Element ohne fixe Datei-Lokation).
///
/// Pflicht-Members:
/// - `cpp_type_name`  — fully-qualified C++ Type-Name (fuer COMDARE_DEFINE_ANATOMY_MODULE)
/// - `header_include` — Include-Pfad relativ zu libs/cache_engine/ (fuer Template)
///
/// Verwendung in R5.F+R5.G anatomy_codegen_tool: `descriptor_from_composition<C>()`
/// extrahiert die 3 CompositionDescriptor-Felder zur Compile-Zeit.
///
/// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §49
template <typename C>
concept HasCompositionLocation = IsComposition<C> && requires {
    { C::cpp_type_name } -> std::convertible_to<std::string_view>;
    { C::header_include } -> std::convertible_to<std::string_view>;
};

/// COMDARE_DEFINE_COMPOSITION_LOCATION(type_str, header_str) — Convenience-Macro
/// fuer Reference-Compositions. Im Body der Composition aufrufen:
///
/// ```cpp
/// struct ArtComposition {
///     // ... 19 using-axes (17 Such-Achsen + queuing q1/q2) ...
///     static constexpr std::string_view name     = "ArtComposition";
///     static constexpr std::string_view paper_id = "P01 ...";
///     COMDARE_DEFINE_COMPOSITION_LOCATION(
///         "::comdare::cache_engine::compositions::ArtComposition",
///         "compositions/art_reference.hpp");
/// };
/// ```
#define COMDARE_DEFINE_COMPOSITION_LOCATION(TYPE_NAME, HEADER_PATH)                                                    \
    static constexpr std::string_view cpp_type_name  = TYPE_NAME;                                                      \
    static constexpr std::string_view header_include = HEADER_PATH

} // namespace comdare::cache_engine::anatomy
