#pragma once
// V41.F.6.1.R4 — Composition-Factory: PermTuple → AdHocComposition
//
// Brueckenkopf zwischen PermutationEngine (PermTuple<V...>) und
// SearchAlgorithmAnatomy<Composition>. Materialisiert pro Cartesian-Produkt-
// Punkt eine konkrete Composition-Struct mit 17 named using-Aliases.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §11.3+§14.3
// @task V41.F.6.1.R4

#include "composition_concept.hpp"

#include <boost/mp11.hpp>

#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// AdHocComposition — Composition-Struct die zur Compile-Time aus einem
/// PermTuple materialisiert wird. 17 Template-Parameter in fester Reihenfolge:
///
/// Pflicht-Reihenfolge (Topic-Slot-Convention V41.F.6.1.R4):
///   T0  = search_algo        (traversal::axis_03a)
///   T1  = cache_traversal    (traversal::axis_03b)
///   T2  = mapping            (traversal::axis_03m)
///   T3  = path_compression   (nodes::axis_02)
///   T4  = node_type          (nodes::axis_04)
///   T5  = memory_layout      (memory_layout::axis_05)
///   T6  = allocator          (allocator::axis_06)
///   T7  = prefetch           (prefetch::axis_07)
///   T8  = concurrency        (concurrency::axis_08)
///   T9  = serialization      (serialization::axis_10)
///   T10 = telemetry          (telemetry::axis_11)
///   T11 = value_handle       (value_handle::axis_14)
///   T12 = isa                (hardware::axis_09)
///   T13 = index_organization (search_engine::axis_01)
///   T14 = io_dispatch        (io::axis_io)
///   T15 = migration_policy   (migration::axis_migration)
///   T16 = filter             (filter::axis_filter)
template <class T0, class T1, class T2, class T3, class T4,
          class T5, class T6, class T7, class T8, class T9,
          class T10, class T11, class T12, class T13, class T14,
          class T15, class T16>
struct AdHocComposition {
    using search_algo        = T0;
    using cache_traversal    = T1;
    using mapping            = T2;
    using path_compression   = T3;
    using node_type          = T4;
    using memory_layout      = T5;
    using allocator          = T6;
    using prefetch           = T7;
    using concurrency        = T8;
    using serialization      = T9;
    using telemetry          = T10;
    using value_handle       = T11;
    using isa                = T12;
    using index_organization = T13;
    using io_dispatch        = T14;
    using migration_policy   = T15;
    using filter             = T16;

    static constexpr std::string_view paper_id = "P00 AdHoc Permutation R4";
    static constexpr std::string_view name     = "AdHocComposition";
};

// ─────────────────────────────────────────────────────────────────────────────
// PermTuple → AdHocComposition Helper
// ─────────────────────────────────────────────────────────────────────────────

namespace detail {

/// Specialization-Helper: extracts the variants pack from a PermTuple-like type.
template <class PermT>
struct CompositionFromPermTupleImpl;

/// Match PermTuple<Vs...> — PermutationEngine produziert PermTuple<V0,V1,...V16>
template <template <class...> class PermTupleTmpl, class... Vs>
struct CompositionFromPermTupleImpl<PermTupleTmpl<Vs...>> {
    static_assert(sizeof...(Vs) == 17,
        "PermTuple muss exakt 17 Achsen-Werte enthalten "
        "(Topic-Slot-Convention V41.F.6.1.R4 §14.3)");
    using type = AdHocComposition<Vs...>;
};

}  // namespace detail

/// CompositionFromPermTuple<PermT> — extrahiert die 17 Vendor-Typen aus einem
/// PermTuple und materialisiert eine AdHocComposition.
///
/// Typische Verwendung im PermutationEngine-Visitor:
/// @code
///   PermEngine::for_each_permutation([]<class P>(){
///       using AdHoc = CompositionFromPermTuple<P>;
///       SearchAlgorithmAnatomy<AdHoc> algo;
///       // ... mess oder build ...
///   });
/// @endcode
template <class PermT>
using CompositionFromPermTuple = typename detail::CompositionFromPermTupleImpl<PermT>::type;

// ─────────────────────────────────────────────────────────────────────────────
// Concept: PermT ist ein 17-Slot-PermTuple das in AdHocComposition passt
// ─────────────────────────────────────────────────────────────────────────────

template <class PermT>
concept IsPermTuple17 = requires {
    typename detail::CompositionFromPermTupleImpl<PermT>::type;
} && IsComposition<typename detail::CompositionFromPermTupleImpl<PermT>::type>;

}  // namespace comdare::cache_engine::anatomy
