#pragma once
// #188-4a (2026-07-02) -- EytzingerTraversalOrgan.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Stateless Traversal-Organ fuer Khuong/Morin JEA 2017: sortierte Basis wird per lower_bound gepflegt, der
// lookup laeuft ueber den 1-indexed Eytzinger-BFS-Puffer. Der lookup traegt ggf. den lazy Rebuild (Option b,
// dokumentierte Mess-Eigenschaft).

#include "eytzinger_layout_pool_concept.hpp"
#include "eytzinger_layout_store.hpp" // Selbstbeweis am Dateiende

#include <bit>
#include <cstddef>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

struct EytzingerTraversalOrgan {
    template <class Pool>
    static std::size_t lower_bound_index(Pool const& p, typename Pool::key_type k) {
        std::size_t lo = 0, hi = p.slot_count();
        while (lo < hi) {
            std::size_t const m = lo + (hi - lo) / 2u;
            if (p.key_at(m) < k)
                lo = m + 1u;
            else
                hi = m;
        }
        return lo;
    }

    template <class Pool>
    static void insert_into(Pool& p, typename Pool::key_type k, typename Pool::value_type v) {
        std::size_t const i = lower_bound_index(p, k);
        if (i < p.slot_count() && p.key_at(i) == k) {
            p.set_value_at(i, v);
            return;
        }
        p.insert_slot_at(i, k, v);
    }

    template <class Pool>
    [[nodiscard]] static std::optional<typename Pool::value_type> lookup_in(Pool const& p, typename Pool::key_type k) {
        p.rebuild_if_dirty();
        std::size_t const n = p.slot_count();
        if (n == 0) return std::nullopt;
        std::size_t j = 1u;
        while (j <= n) {
            j = 2u * j + (p.eyt_key_at(j) < k ? 1u : 0u);
        }
        std::size_t const idx = j >> (static_cast<unsigned>(std::countr_one(j)) + 1u);
        if (idx >= 1u && idx <= n && p.eyt_key_at(idx) == k) return p.eyt_value_at(idx);
        return std::nullopt;
    }

    template <class Pool>
    static bool erase_from(Pool& p, typename Pool::key_type k) {
        std::size_t const i = lower_bound_index(p, k);
        if (i < p.slot_count() && p.key_at(i) == k) {
            p.erase_slot_at(i);
            return true;
        }
        return false;
    }
};

static_assert(EytzingerTraversalOrganConcept<EytzingerTraversalOrgan, EytzingerLayoutStore>);

} // namespace comdare::cache_engine::lookup::composable
