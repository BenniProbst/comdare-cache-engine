#pragma once
// V41.F.6.1.R4 — Composition-Factory: PermTuple → AdHocComposition
//
// Brueckenkopf zwischen PermutationEngine (PermTuple<V...>) und
// SearchAlgorithmAnatomy<Composition>. Materialisiert pro Cartesian-Produkt-
// Punkt eine konkrete Composition-Struct mit 17 named using-Aliases (15 Such-Achsen + queuing q1/q2;
// Bau-INC-2c: telemetry ist System-Achse; Bau-INC-2d: isa ist Target-ISA-System-Achse — beide kein Slot mehr).
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
/// Pflicht-Reihenfolge (Topic-Slot-Convention V41.F.6.1.R4; Doc 30 §8.0 erweitert
/// um die queuing-Achse als reguläre, mandatorische SA-Slots T16/T17):
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
///   T10 = value_handle       (value_handle::axis_14)
///   T11 = index_organization (search_engine::axis_01)
///   T12 = io_dispatch        (io::axis_io)
///   T13 = migration_policy   (migration::axis_migration)
///   T14 = filter             (filter::axis_filter)
///   T15 = queuing_q1         (queuing::axis_q1_queuing — buffer_strategy)
///   T16 = queuing_q2         (queuing::axis_q2_queuing — flush_policy)
///
/// Bau-INC-2c (F12iii, ABI-5): Telemetrie ist KEIN Kompositions-Slot mehr — sie wurde als
/// CEB-System-Achse aus der binary_id-permutierenden Organ-Komposition herausgeloest
/// (TelemetryConfig Active/Silent = System-Achsen-Belegung, versioniert im H-10-Sidecar
/// NEBEN dem Binary; vorher T10 in der damals 19-slotigen Komposition). Das Telemetrie-ORGAN (axes/telemetry) bleibt
/// als Mess-Infrastruktur bestehen — es permutiert nur nicht mehr.
///
/// Bau-INC-2d (ABI-6): isa ist EBENFALLS KEIN Kompositions-Slot mehr — die Ziel-ISA wird zur
/// Compile-Zeit build-config-gewählt (Target-ISA-System-Achse, +target=-Sidecar; vorher T11 von 18,
/// urspruenglich Slot 12 von 19 — die Komposition zaehlt heute 17 Slots). Das isa-ORGAN (hardware::axis_09
/// Amd64Isa/ObservableIsa) bleibt als
/// Codegen-Träger bestehen — es permutiert nur nicht mehr (exakt telemetry-treu).
///
/// queuing ist eine ACHSE der SearchAlgorithm-Tier-Unterklasse (Doc 30 §8 — KEINE
/// Gattung). Ein nicht-pufferndes Tier wählt EXPLIZIT den Durchreich-Algorithmus
/// (NoBuffer/LazyFlush) — das ist ein Algorithmus, kein „weglassen". KEINE
/// Template-Defaults: jedes Tier deklariert q1/q2 ebenso explizit wie die 15 davor.
template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10,
          class T11, class T12, class T13, class T14, class T15, class T16>
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
    using value_handle       = T10;
    using index_organization = T11;
    using io_dispatch        = T12;
    using migration_policy   = T13;
    using filter             = T14;
    using queuing_q1         = T15;
    using queuing_q2         = T16;

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
                  "PermTuple muss exakt 17 Achsen-Werte enthalten (Bau-INC-2c/F12iii: telemetry ist "
                  "System-Achse; Bau-INC-2d/ABI-6: isa ist Target-ISA-System-Achse — beide kein Slot mehr; "
                  "15 Such-Achsen + queuing q1/q2)");
    using type = AdHocComposition<Vs...>;
};

} // namespace detail

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
// (Doc 30 §8.0 i.V.m. Bau-INC-2c/2d: 15 Such-Achsen + queuing q1/q2)
// ─────────────────────────────────────────────────────────────────────────────

template <class PermT>
concept IsPermTuple19 = requires { typename detail::CompositionFromPermTupleImpl<PermT>::type; } &&
                        IsComposition<typename detail::CompositionFromPermTupleImpl<PermT>::type>;

/// Rückwärts-kompatible Aliase (historische Slot-Zahlen 17/19 im Namen; die
/// Slot-Zahl ist seit Bau-INC-2d 17 — Namen bleiben für bestehende Verwender stabil).
template <class PermT>
concept IsPermTuple17 = IsPermTuple19<PermT>;

} // namespace comdare::cache_engine::anatomy
