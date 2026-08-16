#pragma once
// V41 Umstufung-A s4 (Task #43, Doku 14 §13) — SurfFstMapPool-Concept (SuRF exakte Map-Schale).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// **SuRF-Sonderfall (User-Direktive 2026-05-30 BEIDES):** SuRF (Zhang/Lim/Andersen SIGMOD 2018) ist ein
// approximativer SUCCINCT RANGE-FILTER (bool, false positives, KEINE false negatives — KEIN exaktes K->V).
// Damit es am einheitlichen std::map-Interface VERGLEICHBAR ist, traegt die Map-Schale (dieses axis_03a-Organ)
// AUTORITATIV das exakte K->V; das echte LOUDS-Succinct-Filter-Organ lebt separat in axis_filter (Gattungs-
// Trennung: Filter-Gattung bool/may-contain != SearchAlgorithm-Gattung optional<value>). is_original=false
// ([[pseudocode-papers-fallback]]; Apache-2.0 erlaubt Linking, aber Re-Impl zuerst).
//
// S1 (diese Charge): exaktes sortiertes K->V (correctness-base, std::map-aequivalent). S2 (Folge): das
// Substrat wird durch die echte LOUDS-FST (dense/sparse + Suffix + value-Slot) ersetzt — der Filter wird
// dann ein zweiter statischer View auf dasselbe Substrat (Compile-Time-Adapter, KEIN Runtime-Switch).
//
// Substrat OHNE Such-Logik: der Pool haelt sortierte Keys + exakte Werte; insert/lookup/erase lebt im
// SurfMapTraversalOrgan. Gemeinsamer uint64-Key (F15). KEIN noexcept-Zwang ([[allocation-failure-exception]]).

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::lookup::composable {

/// SURF-FST-MAP-POOL-Concept: exaktes sortiertes K->V-Substrat (SuRF-Map-Schale).
/// Erfuellt von SurfFstMapPoolStore; konsumiert von SurfMapTraversalOrgan.
template <class S>
concept SurfFstMapPool =
    requires {
        typename S::key_type;
        typename S::value_type;
    } && std::same_as<typename S::key_type, std::uint64_t> && std::same_as<typename S::value_type, std::uint64_t> &&
    requires(S& s, S const& cs, std::size_t i, typename S::key_type k, typename S::value_type v) {
        { cs.size() } -> std::convertible_to<std::size_t>;
        { cs.lower_bound(k) } -> std::convertible_to<std::size_t>; // erster Index mit key_at>=k (== size falls keiner)
        { cs.key_at(i) } -> std::same_as<typename S::key_type>;
        { cs.value_at(i) } -> std::same_as<typename S::value_type>;
        { s.set_value_at(i, v) } -> std::same_as<void>;
        { s.insert_at(i, k, v) } -> std::same_as<void>; // sortiert einfuegen an Index i; darf werfen
        { s.erase_at(i) } -> std::same_as<void>;
        { s.clear() } -> std::same_as<void>;
    };

} // namespace comdare::cache_engine::lookup::composable
