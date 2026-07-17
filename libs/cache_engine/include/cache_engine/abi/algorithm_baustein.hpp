#pragma once
// ---------------------------------------------------------------------------
// TEST-ONLY (Marker 2026-07-17 F6): Teil des REV7-Baustein-Clusters. Einziger
// Includer ist baustein_variants.hpp (siehe dort); der Cluster wird nur von
// tests/unit/test_abi_interface.cpp konsumiert. KEIN Lib-/App-Konsument
// (repo-weiter grep verifiziert). Rein additiver Marker; NICHT loeschen.
// ---------------------------------------------------------------------------
// algorithm_baustein.hpp — Compile-time std::variant Pattern (REV 7.6 V8.8)
//
// User-Direktive 2026-05-13/14 (STRUCTURAL_CORRECTION_diplomarbeit.md §2.3):
//   "Formal ist jeder Algorithmus-Baustein eine Ansammlung an compile time
//    std::variants dieser bestimmten Algorithmus Komponente. Formal handelt
//    es sich also synchron auf jeder spezifischen Ebene des Stacks einer
//    ExecutionEngine Implementierung wie SearchEngine - je Ebene um einen
//    full join der angebotenen Algorithmen des oder der multiplen Pruefling
//    mit der Ebene bekannter Layer-Baustein-Algorithmen, die bereits in der
//    CacheEngine zum Stand der Technik gehoeren."
//
// Pro Achse (Page, Node, Traversal, ValueHandle, Concurrency, Allocator,
// Prefetch, Telemetry, ISA, Layout, Reclamation) wird ein
// `algorithm_axis<...>` deklariert mit einer std::variant ueber alle
// Konkretisierungen. Der CacheEngineBuilder generiert dann
// Cartesian-Product-Permutationen ueber alle Achsen (full join).

#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>

namespace comdare::cache_engine::baustein {

// Eine Achse = std::variant der angebotenen Konkretisierungen
template <typename... Variants>
struct algorithm_axis {
    using variant_t                   = std::variant<Variants...>;
    static constexpr std::size_t size = sizeof...(Variants);
};

// Helper: extrahiere die N-te Konkretisierung einer Achse als Typ
template <typename Axis, std::size_t I>
using axis_alternative_t = std::variant_alternative_t<I, typename Axis::variant_t>;

// Full-Join Pattern: Cartesian product zweier Achsen (Pruefling x SOTA)
template <typename Axis1, typename Axis2>
struct full_join {
    template <std::size_t I, std::size_t J>
    using pair_t = std::pair<axis_alternative_t<Axis1, I>, axis_alternative_t<Axis2, J>>;

    static constexpr std::size_t total = Axis1::size * Axis2::size;
};

// 11-Achsen-Permutation als compile-time Tuple-of-Variants
template <typename PageAxis, typename NodeAxis, typename TraversalAxis, typename ValueHandleAxis,
          typename ConcurrencyAxis, typename AllocatorAxis,
          typename PrefetchAxis  = algorithm_axis<>, // optional
          typename TelemetryAxis = algorithm_axis<>, typename IsaAxis = algorithm_axis<>,
          typename LayoutAxis = algorithm_axis<>, typename ReclamationAxis = algorithm_axis<>>
struct eleven_axes_permutation {
    using page_v         = typename PageAxis::variant_t;
    using node_v         = typename NodeAxis::variant_t;
    using traversal_v    = typename TraversalAxis::variant_t;
    using value_handle_v = typename ValueHandleAxis::variant_t;
    using concurrency_v  = typename ConcurrencyAxis::variant_t;
    using allocator_v    = typename AllocatorAxis::variant_t;
    using prefetch_v     = typename PrefetchAxis::variant_t;
    using telemetry_v    = typename TelemetryAxis::variant_t;
    using isa_v          = typename IsaAxis::variant_t;
    using layout_v       = typename LayoutAxis::variant_t;
    using reclamation_v  = typename ReclamationAxis::variant_t;

    page_v         page{};
    node_v         node{};
    traversal_v    traversal{};
    value_handle_v value_handle{};
    concurrency_v  concurrency{};
    allocator_v    allocator{};
    prefetch_v     prefetch{};
    telemetry_v    telemetry{};
    isa_v          isa{};
    layout_v       layout{};
    reclamation_v  reclamation{};

    static constexpr std::size_t total_permutations =
        PageAxis::size * NodeAxis::size * TraversalAxis::size * ValueHandleAxis::size * ConcurrencyAxis::size *
        AllocatorAxis::size * (PrefetchAxis::size > 0 ? PrefetchAxis::size : 1) *
        (TelemetryAxis::size > 0 ? TelemetryAxis::size : 1) * (IsaAxis::size > 0 ? IsaAxis::size : 1) *
        (LayoutAxis::size > 0 ? LayoutAxis::size : 1) * (ReclamationAxis::size > 0 ? ReclamationAxis::size : 1);
};

} // namespace comdare::cache_engine::baustein
