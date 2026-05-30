#pragma once
// V41 Umstufung-A s4 (Task #43) — ExactPrefixFilterQuery + ComposedExactSurfFilter (S1 exakte Correctness-Base).
//
// @topic filter @achse axis_filter @schicht composable (Organ-statt-Tier)
//
// Die exakte Query-Logik der Filter-Correctness-Base + die SurfFilterOrgan-erfuellende Komposition. EXAKT:
// contains(k) == (k tatsaechlich vorhanden) -> FP-Rate 0, no-false-negative trivial. Dient als GROUND-TRUTH-
// Anker fuer das echte succinct LOUDS-Filter-Organ (S2, FP>0). [[no-runtime-switch]]: rein statisch.

#include "surf_filter_organ_concept.hpp"
#include "exact_prefix_filter_store.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace comdare::cache_engine::filter::axis_filter::composable {

/// Exakte Query-Logik (statisch ueber dem Store).
struct ExactPrefixFilterQuery {
    template <class Store>
    [[nodiscard]] static bool contains_in(Store const& s, std::uint64_t k) {
        std::size_t const i = s.lower_bound(k);
        return i < s.key_count() && s.key_at(i) == k;
    }
    /// [lo,hi] enthaelt mind. einen Key: der erste Key >= lo ist <= hi.
    template <class Store>
    [[nodiscard]] static bool range_may_exist_in(Store const& s, std::uint64_t lo, std::uint64_t hi) {
        std::size_t const i = s.lower_bound(lo);
        return i < s.key_count() && s.key_at(i) <= hi;
    }
};

/// KOMPOSITION: das exakte SuRF-Filter-Organ (S1) — erfuellt SurfFilterOrgan. may-contain ist hier exakt
/// (FP=0); S2 ersetzt Store+Query durch echte LoudsDense/Sparse+Suffix mit identischer Schnittstelle.
template <class Query, class Store>
class ComposedExactSurfFilter {
public:
    using key_type = std::uint64_t;

    void build_from_sorted_keys(std::span<key_type const> sorted) { store_.build_from_sorted_keys(sorted); }
    [[nodiscard]] bool contains(key_type k)                       const { return Query::template contains_in<Store>(store_, k); }
    [[nodiscard]] bool range_may_exist(key_type lo, key_type hi)  const { return Query::template range_may_exist_in<Store>(store_, lo, hi); }
    [[nodiscard]] std::size_t bit_size()       const noexcept { return store_.bit_size(); }
    [[nodiscard]] double      bits_per_key()   const noexcept { return store_.bits_per_key(); }
    [[nodiscard]] std::size_t key_count()      const noexcept { return store_.key_count(); }
    void clear()                                     noexcept { store_.clear(); }
    [[nodiscard]] Store const& store()         const noexcept { return store_; }

private:
    Store store_{};
};

// Selbstbeweis: das exakte Filter-Organ erfuellt das SurfFilterOrgan-Concept.
static_assert(SurfFilterOrgan<ComposedExactSurfFilter<ExactPrefixFilterQuery, ExactPrefixFilterStore>>);

}  // namespace comdare::cache_engine::filter::axis_filter::composable
