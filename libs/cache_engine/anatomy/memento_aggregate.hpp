#pragma once
// V5-I5 — MementoAggregate<Composition>: das Fundament für memento_all (parallel zu ObserverAggregate).
//
// User-Direktive 2026-05-31: memento_all rollt den GESAMTEN Zustand einer Tier-Binary nach Warmup über ALLE
// stateful Achsen zurück (Memento-Pattern). Einheitliche Memento-Hilfsfunktionen je stateful Achsen-Interface;
// IO/Disk-Persistenz möglich → „ein einfacher Snapshot reicht NICHT" ⇒ memento ist eine RICHE, binary-INTERNE
// Struktur (kein flacher ABI-POD wie ComdareTierObserverSnapshotV1). Der Host sieht den Memento NIE; er triggert
// nur tier_save_all()/tier_rollback_all() (IRollbackableTier, V5-I6) — der Zustand lebt IN der Binary.
//
// Spiegelt observer_aggregate.hpp exakt: ObservableAxis ⟺ „hat statistics()" → hier MementoAxis ⟺ „hat
// save_state()/restore_state()". Achsen ohne Memento liefern EmptyMemento (stateless → no-op-Rollback).
//
// @doku docs/architecture/messarchitektur_v5_design.md §3 (Memento_all, 9 stateful Achsen) + §8

#include "composition_concept.hpp"

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace comdare::cache_engine::anatomy {

// ─────────────────────────────────────────────────────────────────────────────
// EmptyMemento — Fallback für stateless Achsen (kein Rollback nötig)
// ─────────────────────────────────────────────────────────────────────────────

/// POD-Marker für Achsen ohne save_state()/restore_state() (stateless: isa/memory_layout/mapping/…).
/// Rollback einer stateless Achse = no-op. standard_layout + trivially_copyable garantiert.
struct EmptyMemento {
    [[nodiscard]] constexpr bool operator==(EmptyMemento const&) const noexcept { return true; }
};
static_assert(std::is_standard_layout_v<EmptyMemento>);
static_assert(std::is_trivially_copyable_v<EmptyMemento>);

// ─────────────────────────────────────────────────────────────────────────────
// MementoAxis — Concept für stateful Achsen mit save_state()/restore_state()
// ─────────────────────────────────────────────────────────────────────────────

/// Eine Achse ist MementoAxis, wenn sie ihren Zustand kapseln + zurückrollen kann:
///   memento_t          — der (riche) Zustands-Schnappschuss-Typ (darf STL/heap halten, KEIN POD-Zwang)
///   save_state() const  — kapselt den aktuellen Zustand → memento_t (Warmup-Vor-Zustand)
///   restore_state(m)    — setzt den Zustand auf m zurück (nach dem Warmup-Op, vor der Messung)
template <class A>
concept MementoAxis = requires(A& a, A const& ca, typename A::memento_t const& m) {
    typename A::memento_t;
    { ca.save_state() } -> std::same_as<typename A::memento_t>;
    { a.restore_state(m) } -> std::same_as<void>;
};

/// memento_of<A> — A::memento_t wenn MementoAxis, sonst EmptyMemento.
template <class A>
struct memento_of { using type = EmptyMemento; };
template <MementoAxis A>
struct memento_of<A> { using type = typename A::memento_t; };
template <class A>
using memento_of_t = typename memento_of<A>::type;

// ─────────────────────────────────────────────────────────────────────────────
// save_axis / restore_axis — einheitliche Hilfsfunktionen je Achsen-Interface
// ─────────────────────────────────────────────────────────────────────────────

/// save_axis(a) — kapselt den Achsen-Zustand (oder EmptyMemento für stateless Achsen).
template <class A>
[[nodiscard]] memento_of_t<A> save_axis(A const& a) {
    if constexpr (MementoAxis<A>) { return a.save_state(); }
    else                          { return EmptyMemento{}; }
}

/// restore_axis(a, m) — rollt den Achsen-Zustand zurück (no-op für stateless Achsen).
template <class A>
void restore_axis(A& a, memento_of_t<A> const& m) {
    if constexpr (MementoAxis<A>) { a.restore_state(m); }
    else                          { (void)a; (void)m; }
}

// ─────────────────────────────────────────────────────────────────────────────
// MementoAggregate<Composition> — 19 named Memento-Members (17 + queuing q1/q2, Doc 30 §8.0)
// ─────────────────────────────────────────────────────────────────────────────

/// Binary-interner Zustands-Halter pro Composition: je Achse 1 Memento (named nach Composition-Alias).
/// Anders als ObserverAggregate NICHT trivially_copyable-pflichtig — er quert die ABI-Grenze NICHT, sondern
/// lebt in der Tier-Binary (Host triggert nur tier_save_all/tier_rollback_all über IRollbackableTier, V5-I6).
template <IsComposition Composition>
struct MementoAggregate {
    memento_of_t<typename Composition::search_algo>        search_algo;
    memento_of_t<typename Composition::cache_traversal>    cache_traversal;
    memento_of_t<typename Composition::mapping>            mapping;
    memento_of_t<typename Composition::path_compression>   path_compression;
    memento_of_t<typename Composition::node_type>          node_type;
    memento_of_t<typename Composition::memory_layout>      memory_layout;
    memento_of_t<typename Composition::allocator>          allocator;
    memento_of_t<typename Composition::prefetch>           prefetch;
    memento_of_t<typename Composition::concurrency>        concurrency;
    memento_of_t<typename Composition::serialization>      serialization;
    memento_of_t<typename Composition::telemetry>          telemetry;
    memento_of_t<typename Composition::value_handle>       value_handle;
    memento_of_t<typename Composition::isa>                isa;
    memento_of_t<typename Composition::index_organization> index_organization;
    memento_of_t<typename Composition::io_dispatch>        io_dispatch;
    memento_of_t<typename Composition::migration_policy>   migration_policy;
    memento_of_t<typename Composition::filter>             filter;
    // Doc 30 §8.0: queuing q1/q2 als reguläre SA-Achsen (EmptyMemento solange stateless via memento_of_t).
    memento_of_t<typename Composition::queuing_q1>         queuing_q1;
    memento_of_t<typename Composition::queuing_q2>         queuing_q2;

    /// Anzahl der stateful (nicht-Empty) Achsen — Diagnose für den Zwei-Phasen-Treiber.
    [[nodiscard]] static constexpr std::size_t stateful_count() noexcept {
        std::size_t n = 0;
        if constexpr (MementoAxis<typename Composition::search_algo>)        ++n;
        if constexpr (MementoAxis<typename Composition::cache_traversal>)    ++n;
        if constexpr (MementoAxis<typename Composition::mapping>)            ++n;
        if constexpr (MementoAxis<typename Composition::path_compression>)   ++n;
        if constexpr (MementoAxis<typename Composition::node_type>)          ++n;
        if constexpr (MementoAxis<typename Composition::memory_layout>)      ++n;
        if constexpr (MementoAxis<typename Composition::allocator>)          ++n;
        if constexpr (MementoAxis<typename Composition::prefetch>)           ++n;
        if constexpr (MementoAxis<typename Composition::concurrency>)        ++n;
        if constexpr (MementoAxis<typename Composition::serialization>)      ++n;
        if constexpr (MementoAxis<typename Composition::telemetry>)          ++n;
        if constexpr (MementoAxis<typename Composition::value_handle>)       ++n;
        if constexpr (MementoAxis<typename Composition::isa>)                ++n;
        if constexpr (MementoAxis<typename Composition::index_organization>) ++n;
        if constexpr (MementoAxis<typename Composition::io_dispatch>)        ++n;
        if constexpr (MementoAxis<typename Composition::migration_policy>)   ++n;
        if constexpr (MementoAxis<typename Composition::filter>)             ++n;
        if constexpr (MementoAxis<typename Composition::queuing_q1>)         ++n;
        if constexpr (MementoAxis<typename Composition::queuing_q2>)         ++n;
        return n;
    }

    [[nodiscard]] static constexpr std::size_t total_slots() noexcept { return 19; }
};

}  // namespace comdare::cache_engine::anatomy
