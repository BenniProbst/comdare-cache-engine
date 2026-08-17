#pragma once
// V41 Umstufung-A s4 (Task #43) — SurfMapTraversal-Concept + SurfMapTraversalOrgan.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Die exakte Such-Logik der SuRF-Map-Schale (is_original=false, S1 correctness-base): Binaersuche-Insert/
// Lookup/Erase ueber dem sortierten K->V-Substrat. Exakt (KEINE false positives) — das ist der autoritative
// Teil, der std::map-Aequivalenz traegt; das approximative LOUDS-may-contain ist das separate axis_filter-
// Organ. [[no-runtime-switch]]: rein statische Templates.

#include "surf_fst_map_pool_concept.hpp"
#include "surf_fst_map_pool_store.hpp" // fuer den Selbstbeweis am Dateiende

#include <concepts>
#include <cstddef>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

/// SURF-MAP-TRAVERSAL-Organ-Concept: statische insert_into/lookup_in/erase_from auf einem SurfFstMapPool.
template <class T, class Pool>
concept SurfMapTraversal =
    SurfFstMapPool<Pool> && requires(Pool& p, Pool const& cp, typename Pool::key_type k, typename Pool::value_type v) {
        { T::template insert_into<Pool>(p, k, v) } -> std::same_as<void>;
        { T::template lookup_in<Pool>(cp, k) } -> std::same_as<std::optional<typename Pool::value_type>>;
        { T::template erase_from<Pool>(p, k) } -> std::same_as<bool>;
    };

/// SuRF-Map-Schale-Traversal-Organ: exaktes sortiertes K->V via Binaersuche.
struct SurfMapTraversalOrgan {
    template <class Pool>
    static void insert_into(Pool& p, typename Pool::key_type key, typename Pool::value_type value) {
        std::size_t const i = p.lower_bound(key);
        if (i < p.size() && p.key_at(i) == key) {
            p.set_value_at(i, value);
            return;
        } // Update
        p.insert_at(i, key, value);
    }

    template <class Pool>
    static std::optional<typename Pool::value_type> lookup_in(Pool const& p, typename Pool::key_type key) {
        std::size_t const i = p.lower_bound(key);
        if (i < p.size() && p.key_at(i) == key) return p.value_at(i);
        return std::nullopt;
    }

    template <class Pool>
    static bool erase_from(Pool& p, typename Pool::key_type key) {
        std::size_t const i = p.lower_bound(key);
        if (i < p.size() && p.key_at(i) == key) {
            p.erase_at(i);
            return true;
        }
        return false;
    }
};

// Selbstbeweis: SurfMapTraversalOrgan erfuellt das SurfMapTraversal-Concept ueber dem Pilot-Pool.
static_assert(SurfMapTraversal<SurfMapTraversalOrgan, SurfFstMapPoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
