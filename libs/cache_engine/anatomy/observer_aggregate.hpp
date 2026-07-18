#pragma once
// V41.F.6.1.R5.A — ObserverAggregate<Composition> (ABI-stabiler 19-Achsen-Snapshot)
//
// Pro Composition wird ein POD-Struct definiert, der 19 Achsen-Snapshots
// (einer pro Achse) sammelt — 17 Such-Achsen + queuing q1/q2 (Doc 30 §8.0).
// ABI-stabil durch standard_layout + trivially_copyable.
// Achsen ohne statistics() liefern EmptyAxisSnapshot (= leerer POD).
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §17.2 + §20
// @task #697 V41.F.6.1.R5.A
// @related [[anatomie-nur-achsen-und-observer]] [[3-kompositionale-joins-anatomie]]

#include "composition_concept.hpp"

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

// ─────────────────────────────────────────────────────────────────────────────
// EmptyAxisSnapshot — Fallback fuer Achsen ohne statistics()
// ─────────────────────────────────────────────────────────────────────────────

/// POD-Marker fuer Achsen die (noch) keine Statistics liefern.
/// Stufe-A-Wrappers ohne COMDARE_CE_ENABLE_STATISTICS bekommen diesen.
struct EmptyAxisSnapshot {
    // explicitly empty — standard_layout + trivially_copyable garantiert
    [[nodiscard]] constexpr bool operator==(EmptyAxisSnapshot const&) const noexcept { return true; }
};

static_assert(std::is_standard_layout_v<EmptyAxisSnapshot>);
static_assert(std::is_trivially_copyable_v<EmptyAxisSnapshot>);

// ─────────────────────────────────────────────────────────────────────────────
// ObservableAxis — Concept fuer Achsen mit statistics()
// ─────────────────────────────────────────────────────────────────────────────

template <class A>
concept ObservableAxis = requires(A const& a) {
    typename A::snapshot_t;
    { a.statistics() } -> std::same_as<typename A::snapshot_t>;
};

/// snapshot_of<A> — typeof(A::snapshot_t) wenn ObservableAxis, sonst EmptyAxisSnapshot.
template <class A>
struct snapshot_of {
    using type = std::conditional_t<
        ObservableAxis<A>,
        typename std::conditional_t<ObservableAxis<A>, A, void // ObservableAxis-Pfad: A::snapshot_t verfuegbar
                                    >::snapshot_t,             // (Achtung: nur gueltig wenn ObservableAxis<A>)
        EmptyAxisSnapshot>;
};

// Specialization fuer Non-ObservableAxis — vermeidet snapshot_t-Lookup-Fehler
template <class A>
    requires(!ObservableAxis<A>)
struct snapshot_of<A> {
    using type = EmptyAxisSnapshot;
};

template <class A>
using snapshot_of_t = typename snapshot_of<A>::type;

// ─────────────────────────────────────────────────────────────────────────────
// snapshot_axis<A> — sample-Helper: extrahiert Snapshot oder Empty
// ─────────────────────────────────────────────────────────────────────────────

template <class A>
[[nodiscard]] constexpr snapshot_of_t<A> snapshot_axis(A const& a) noexcept {
    if constexpr (ObservableAxis<A>) {
        return a.statistics();
    } else {
        return EmptyAxisSnapshot{};
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ObserverAggregate<Composition> — 19 named Snapshot-Members (17 + queuing q1/q2)
// ─────────────────────────────────────────────────────────────────────────────

/// ObserverAggregate ist ABI-stabiler Snapshot-Container pro Composition.
/// Pro Achse 1 Snapshot-Member (named nach Composition-using-Alias).
///
/// Pflicht-Eigenschaften:
/// - standard_layout (pro Member POD wenn alle Achsen-Snapshots POD sind)
/// - trivially_copyable (memcpy-faehig fuer ABI-Loader)
///
/// Aufbau zur Compile-Zeit:
///   ObserverAggregate<MyComposition> agg;
///   agg.search_algo_snapshot = my_anatomy.axis_search_algo_.statistics();
///   // ... 16 weitere ...
template <IsComposition Composition>
struct ObserverAggregate {
    snapshot_of_t<typename Composition::search_algo>        search_algo;
    snapshot_of_t<typename Composition::cache_traversal>    cache_traversal;
    snapshot_of_t<typename Composition::mapping>            mapping;
    snapshot_of_t<typename Composition::path_compression>   path_compression;
    snapshot_of_t<typename Composition::node_type>          node_type;
    snapshot_of_t<typename Composition::memory_layout>      memory_layout;
    snapshot_of_t<typename Composition::allocator>          allocator;
    snapshot_of_t<typename Composition::prefetch>           prefetch;
    snapshot_of_t<typename Composition::concurrency>        concurrency;
    snapshot_of_t<typename Composition::serialization>      serialization;
    snapshot_of_t<typename Composition::value_handle>       value_handle;
    snapshot_of_t<typename Composition::index_organization> index_organization;
    snapshot_of_t<typename Composition::io_dispatch>        io_dispatch;
    snapshot_of_t<typename Composition::migration_policy>   migration_policy;
    snapshot_of_t<typename Composition::filter>             filter;
    // Doc 30 §8.0: queuing q1/q2 als reguläre SA-Achsen (BufferStatistics / FlushPolicyStatistics
    // bei COMDARE_CE_ENABLE_STATISTICS; sonst EmptyAxisSnapshot via snapshot_of_t — wie alle anderen).
    snapshot_of_t<typename Composition::queuing_q1> queuing_q1;
    snapshot_of_t<typename Composition::queuing_q2> queuing_q2;

    /// Anzahl der "echten" (nicht-Empty) Snapshots — Diagnose fuer Mess-Treiber.
    [[nodiscard]] static constexpr std::size_t observable_count() noexcept {
        std::size_t n = 0;
        if constexpr (ObservableAxis<typename Composition::search_algo>) ++n;
        if constexpr (ObservableAxis<typename Composition::cache_traversal>) ++n;
        if constexpr (ObservableAxis<typename Composition::mapping>) ++n;
        if constexpr (ObservableAxis<typename Composition::path_compression>) ++n;
        if constexpr (ObservableAxis<typename Composition::node_type>) ++n;
        if constexpr (ObservableAxis<typename Composition::memory_layout>) ++n;
        if constexpr (ObservableAxis<typename Composition::allocator>) ++n;
        if constexpr (ObservableAxis<typename Composition::prefetch>) ++n;
        if constexpr (ObservableAxis<typename Composition::concurrency>) ++n;
        if constexpr (ObservableAxis<typename Composition::serialization>) ++n;
        if constexpr (ObservableAxis<typename Composition::value_handle>) ++n;
        if constexpr (ObservableAxis<typename Composition::index_organization>) ++n;
        if constexpr (ObservableAxis<typename Composition::io_dispatch>) ++n;
        if constexpr (ObservableAxis<typename Composition::migration_policy>) ++n;
        if constexpr (ObservableAxis<typename Composition::filter>) ++n;
        if constexpr (ObservableAxis<typename Composition::queuing_q1>) ++n;
        if constexpr (ObservableAxis<typename Composition::queuing_q2>) ++n;
        return n;
    }

    /// Total Achsen-Slot-Anzahl (Pflicht: 17 fuer Vollausbau — 15 Such-Achsen + queuing q1/q2;
    /// Doc 30 §8.0 i.V.m. Bau-INC-2c: telemetry / Bau-INC-2d: isa sind System-Achsen, kein Slot mehr)
    [[nodiscard]] static constexpr std::size_t total_slots() noexcept { return 17; }
};

} // namespace comdare::cache_engine::anatomy
