#pragma once
// V41 Umstufung-A s4 (Task #43, Doku 14 §26 Gattungen) — SurfFilterOrgan-Concept (axis_filter Filter-Gattung).
//
// @topic filter @achse axis_filter @schicht composable (Organ-statt-Tier)
//
// **SuRF-Dual-Sezierung (User-Direktive 2026-05-30 BEIDES), Teil 2 (Filter-Organ):** SuRF (Zhang/Lim/Andersen
// SIGMOD 2018) ist primaer ein approximativer SUCCINCT RANGE-FILTER — may-contain (bool) mit false positives
// aber GARANTIERT KEINEN false negatives. Das ist die Filter-GATTUNG (Doku 14 §26): bool/may-contain, KEIN
// optional<value> wie die SearchAlgorithm-Map-Gattung. Daher ein EIGENES Filter-Organ-Concept (NICHT die
// bestehende FilterStrategy-Klassifikations-Strategie zweckentfremden, [[axis-gold-standard-checklist]]).
//
// Die exakte K->V-Vergleichbarkeit traegt die SuRF-Map-Schale (axis_03a, ComposedSurfMapSearch); dieses
// Filter-Organ liefert NUR das approximative may-contain — verifiziert ueber die no-false-negative-Property
// (jeder gebaute Key wird gefunden) + eine messbare FP-Rate. S1: exakte correctness-base (FP=0, Ground-Truth
// fuer S2). S2 (Folge): echtes LoudsDense+LoudsSparse+Suffix (FP>0, tunbar). is_original=false.

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace comdare::cache_engine::filter_axis::composable {

/// SURF-FILTER-Organ-Concept: approximativer Membership-/Range-Filter (Filter-Gattung, bool/may-contain).
/// Erfuellt von ComposedExactSurfFilter (S1) bzw. ComposedSurfLoudsFilter (S2).
template <class F>
concept SurfFilterOrgan =
    requires { typename F::key_type; }
    && std::same_as<typename F::key_type, std::uint64_t>   // gemeinsamer breiter Key (F15)
    && requires(F& f, F const& cf, std::uint64_t k, std::uint64_t lo, std::uint64_t hi,
                std::span<std::uint64_t const> sorted) {
        { f.build_from_sorted_keys(sorted) } -> std::same_as<void>;   // einzige Bulk-API (SuRF read-only/bulk-load)
        { cf.contains(k) }                   -> std::same_as<bool>;    // may-contain (FP erlaubt, KEINE FN)
        { cf.range_may_exist(lo, hi) }       -> std::same_as<bool>;    // [lo,hi] may-overlap (FP erlaubt, KEINE FN)
        { cf.bit_size() }                    -> std::convertible_to<std::size_t>;   // Observer (Saeule-2: Space)
        { cf.bits_per_key() }                -> std::convertible_to<double>;        // Observer (Saeule-2: Space/Key)
        { cf.key_count() }                   -> std::convertible_to<std::size_t>;
        { f.clear() }                        -> std::same_as<void>;
    };

}  // namespace comdare::cache_engine::filter_axis::composable
