#pragma once
// V41 Umstufung-A (Task #41) — SkipListTraversal-Concept + SkipListTraversalOrgan.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Analog zum TreeTraversalOrgan, aber ueber einem SkipListNodePool: statische insert_into/lookup_in/
// erase_from, KEIN Eigenspeicher (auch KEIN RNG — der lebt im Pool). SkipListTraversalOrgan = Multi-Level-
// Walk + Forward-Verkettung (Pugh CACM 1990), portiert aus axis_03a_search_algo_skip_list.hpp (Z.85-152),
// aber als austauschbares ORGAN ueber dem generischen uint64-Pool (statt monolithischem uint16-Wrapper).
//
// KORREKTES erase ([[algorithm-correctness-when-named]]): aus allen Levels unverlinken + Tombstone via
// set_node_live(false) + Listen-Hoehe schrumpfen. [[no-runtime-switch]]: rein statische Templates.

#include "skip_list_node_pool_concept.hpp"
#include "skip_list_node_pool_store.hpp"   // fuer den Selbstbeweis am Dateiende

#include <array>
#include <concepts>
#include <cstddef>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

/// SKIP-LIST-TRAVERSAL-Organ-Concept: statische insert_into/lookup_in/erase_from auf einem SkipListNodePool.
template <class T, class Pool>
concept SkipListTraversal = SkipListNodePool<Pool> && requires(Pool& p, Pool const& cp,
                                  typename Pool::key_type k, typename Pool::value_type v) {
    { T::template insert_into<Pool>(p, k, v) } -> std::same_as<void>;
    { T::template lookup_in<Pool>(cp, k) }     -> std::same_as<std::optional<typename Pool::value_type>>;
    { T::template erase_from<Pool>(p, k) }     -> std::same_as<bool>;
};

/// Skip-List-Traversal-Organ: Multi-Level-Walk von oben nach unten + vorwaerts. Navigiert ueber Pool-Forward-API.
struct SkipListTraversalOrgan {
    /// Fuellt update[i] = Praedezessor-Index auf Level i und liefert den Level-0-Kandidaten (>= k).
    template <class Pool>
    [[nodiscard]] static std::size_t find_update(Pool const& p, typename Pool::key_type k,
                                                 std::array<std::size_t, Pool::kMaxLevel>& update) {
        std::size_t const NIL = Pool::kNil;
        std::size_t x = p.head();
        for (int i = p.list_level() - 1; i >= 0; --i) {
            std::size_t const lvl = static_cast<std::size_t>(i);
            std::size_t nxt = p.forward_at(x, lvl);
            while (nxt != NIL && p.node_key(nxt) < k) { x = nxt; nxt = p.forward_at(x, lvl); }
            update[lvl] = x;
        }
        return p.forward_at(x, 0);
    }

    template <class Pool>
    static void insert_into(Pool& p, typename Pool::key_type k, typename Pool::value_type v) {
        std::array<std::size_t, Pool::kMaxLevel> update{};
        std::size_t const NIL  = Pool::kNil;
        std::size_t const cand = find_update<Pool>(p, k, update);
        if (cand != NIL && p.node_key(cand) == k && p.node_live(cand)) {
            p.set_node_value(cand, v);   // Update vorhandener Key
            return;
        }
        int const lvl = p.draw_level();
        if (lvl > p.list_level()) {
            for (int i = p.list_level(); i < lvl; ++i) update[static_cast<std::size_t>(i)] = p.head();
            p.set_list_level(lvl);
        }
        std::size_t const idx = p.allocate_node(k, v, lvl);
        for (int i = 0; i < lvl; ++i) {
            std::size_t const lv   = static_cast<std::size_t>(i);
            std::size_t const pred = update[lv];
            p.set_forward_at(idx, lv, p.forward_at(pred, lv));
            p.set_forward_at(pred, lv, idx);
        }
    }

    template <class Pool>
    static std::optional<typename Pool::value_type> lookup_in(Pool const& p, typename Pool::key_type k) {
        std::size_t const NIL = Pool::kNil;
        std::size_t x = p.head();
        for (int i = p.list_level() - 1; i >= 0; --i) {
            std::size_t const lvl = static_cast<std::size_t>(i);
            std::size_t nxt = p.forward_at(x, lvl);
            while (nxt != NIL && p.node_key(nxt) < k) { x = nxt; nxt = p.forward_at(x, lvl); }
        }
        std::size_t const cand = p.forward_at(x, 0);
        if (cand != NIL && p.node_key(cand) == k && p.node_live(cand)) return p.node_value(cand);
        return std::nullopt;
    }

    template <class Pool>
    static bool erase_from(Pool& p, typename Pool::key_type k) {
        std::array<std::size_t, Pool::kMaxLevel> update{};
        std::size_t const NIL  = Pool::kNil;
        std::size_t const cand = find_update<Pool>(p, k, update);
        if (cand == NIL || p.node_key(cand) != k || !p.node_live(cand)) return false;
        for (int i = 0; i < p.list_level(); ++i) {
            std::size_t const lvl  = static_cast<std::size_t>(i);
            std::size_t const pred = update[lvl];
            if (p.forward_at(pred, lvl) == cand) p.set_forward_at(pred, lvl, p.forward_at(cand, lvl));
        }
        p.set_node_live(cand, false);   // Tombstone (unverlinkt → unerreichbar)
        while (p.list_level() > 1 && p.forward_at(p.head(), static_cast<std::size_t>(p.list_level() - 1)) == NIL)
            p.set_list_level(p.list_level() - 1);
        p.dec_live();
        return true;
    }
};

// Selbstbeweis: SkipListTraversalOrgan erfuellt das SkipListTraversal-Concept ueber dem Pilot-Pool.
static_assert(SkipListTraversal<SkipListTraversalOrgan, SkipListNodePoolStore>);

}  // namespace comdare::cache_engine::lookup::composable
