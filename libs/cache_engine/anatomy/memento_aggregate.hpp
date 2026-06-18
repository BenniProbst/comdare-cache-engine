#pragma once
// V5-I5 — Per-Achsen-Memento-Fundament für memento_all: MementoAxis-Concept + save_axis/restore_axis + EmptyMemento.
// (K10-PMAJOR-02, 2026-06-18) Der frühere Aggregat-Halter MementoAggregate<Composition> wurde als toter Rev.1-
// Apparat ENTFERNT — er hatte null Konsumenten; der kanonische Memento läuft im abi_adapter über CoW Rev.2 +
// den per-Achsen-MementoAxis-Fallback. Dieser Header trägt nur noch die LEBENDIGEN per-Achsen-Bausteine.
//
// User-Direktive 2026-05-31: memento_all rollt den GESAMTEN Zustand einer Tier-Binary nach Warmup über ALLE
// stateful Achsen zurück (Memento-Pattern). Einheitliche Memento-Hilfsfunktionen je stateful Achsen-Interface;
// IO/Disk-Persistenz möglich → „ein einfacher Snapshot reicht NICHT" ⇒ memento ist eine RICHE, binary-INTERNE
// Struktur (kein flacher ABI-POD wie der konsolidierte Observer-POD). Der Host sieht den Memento NIE; er triggert
// nur tier_save_all()/tier_rollback_all() (IRollbackableTier, V5-I6) — der Zustand lebt IN der Binary.
//
// Spiegelt observer_aggregate.hpp exakt: ObservableAxis ⟺ „hat statistics()" → hier MementoAxis ⟺ „hat
// save_state()/restore_state()". Achsen ohne Memento liefern EmptyMemento (stateless → no-op-Rollback).
//
// @doku docs/architecture/messarchitektur_v5_design.md §3 (Memento_all, 9 stateful Achsen) + §8

// (K10-PMAJOR-02) composition_concept.hpp-Include entfernt — IsComposition wurde nur vom getilgten
// MementoAggregate<Composition> gebraucht; die verbliebenen per-Achsen-Bausteine sind composition-unabhängig.
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
// (K10-PMAJOR-02, 2026-06-18) MementoAggregate<Composition> ENTFERNT — toter Rev.1-Apparat.
// ─────────────────────────────────────────────────────────────────────────────
// Der frühere 19-Slot-Aggregat-Halter MementoAggregate<Composition> (+ stateful_count()/total_slots()) hatte
// grep-bestätigt NULL Konsumenten (kein Mess-Pfad, kein Test, kein Adapter — verifiziert
//   grep -rIn "MementoAggregate|stateful_count|total_slots" libs apps tests benchmarks → nur dieser Header).
// Der kanonische Zwei-Phasen-Memento läuft im abi_adapter über (a) Copy-on-Write Rev.2 (cow_materialize_copy_,
// O(1)-save + lazy Vollkopie) und (b) den PER-ACHSEN-MementoAxis-Fallback (saved_search_m_/saved_container_m_)
// — NICHT über ein Aggregat aller 19 Slots. Der Aggregat-Halter wäre nur eine nie-instanziierte Parallel-
// struktur gewesen → entfernt statt etikett-getragen (Lehrbuch-Pattern: kein toter Memento-Apparat).
//
// LEBENDIG BLEIBT (von abi_adapter + tests test_v5_memento_axis/test_v5_disk_memento/test_v5_organ_memento
// real konsumiert, daher NICHT entfernt): das MementoAxis-Concept, memento_of_t<>, EmptyMemento sowie die
// Hilfsfunktionen save_axis()/restore_axis() oben.

}  // namespace comdare::cache_engine::anatomy
