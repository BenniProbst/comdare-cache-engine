#pragma once
// V41 Umstufung-A (Task #41) — BTreeTraversal-Concept + BTreeTraversalOrgan.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Analog zum TreeTraversalOrgan, aber ueber einem BTreeNodePool (Mehrwege-Knoten): statische insert_into/
// lookup_in/erase_from + ALLE CLRS-Transformations-Helfer (split_child/insert_nonfull/remove_from/
// remove_from_nonleaf/fill/borrow_from_prev/borrow_from_next/merge), KEIN Eigenspeicher. Portiert aus
// axis_03a_search_algo_btree.hpp (Z.78-353), aber als austauschbares ORGAN ueber dem generischen uint64-
// Pool. Jeder `nodes_[i].xxx`-Zugriff wird zu p.node_xxx(i) / p.set_node_xxx(i,...) — dadurch entfaellt die
// Reallokations-Falle des Monolithen (kein Node&-Cache ueber new_node hinweg gehalten; index-stabil).
//
// CLRS B-TREE-INSERT (top-down-split) + B-TREE-DELETE (borrow/merge). [[no-runtime-switch]]: rein statisch.

#include "btree_node_pool_concept.hpp"
#include "btree_node_pool_store.hpp"   // fuer den Selbstbeweis am Dateiende

#include <concepts>
#include <cstddef>
#include <optional>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable {

/// B-TREE-TRAVERSAL-Organ-Concept: statische insert_into/lookup_in/erase_from auf einem BTreeNodePool.
template <class T, class Pool>
concept BTreeTraversal = BTreeNodePool<Pool> && requires(Pool& p, Pool const& cp,
                                  typename Pool::key_type k, typename Pool::value_type v) {
    { T::template insert_into<Pool>(p, k, v) } -> std::same_as<void>;
    { T::template lookup_in<Pool>(cp, k) }     -> std::same_as<std::optional<typename Pool::value_type>>;
    { T::template erase_from<Pool>(p, k) }     -> std::same_as<bool>;
};

/// B-Baum-Traversal-Organ: balancierter Mehrwege-Suchbaum (Bayer/McCreight / CLRS Kap. 18). Navigiert +
/// transformiert ausschliesslich ueber die Pool-Getter/Setter (KEIN gecachetes Node&).
struct BTreeTraversalOrgan {
    // ---- Such-Helfer (const) ----
    template <class Pool>
    [[nodiscard]] static bool contains(Pool const& p, typename Pool::key_type k) {
        std::size_t const NIL = Pool::kNil;
        std::size_t cur = p.root();
        while (cur != NIL) {
            int const n = p.node_n(cur);
            int i = 0;
            while (i < n && k > p.node_key_at(cur, i)) ++i;
            if (i < n && p.node_key_at(cur, i) == k) return true;
            cur = p.node_leaf(cur) ? NIL : p.node_child_at(cur, i);
        }
        return false;
    }

    template <class Pool>
    [[nodiscard]] static bool update_existing(Pool& p, typename Pool::key_type k, typename Pool::value_type v) {
        std::size_t const NIL = Pool::kNil;
        std::size_t cur = p.root();
        while (cur != NIL) {
            int const n = p.node_n(cur);
            int i = 0;
            while (i < n && k > p.node_key_at(cur, i)) ++i;
            if (i < n && p.node_key_at(cur, i) == k) { p.set_node_value_at(cur, i, v); return true; }
            cur = p.node_leaf(cur) ? NIL : p.node_child_at(cur, i);
        }
        return false;
    }

    template <class Pool>
    [[nodiscard]] static int find_key(Pool const& p, std::size_t x, typename Pool::key_type k) {
        int const n = p.node_n(x);
        int idx = 0;
        while (idx < n && p.node_key_at(x, idx) < k) ++idx;
        return idx;
    }

    // ---- CLRS INSERT-Helfer ----
    /// SPLIT-CHILD: volles Kind C[i] von x in zwei Knoten teilen, Median nach x hochziehen.
    template <class Pool>
    static void split_child(Pool& p, std::size_t x_idx, int i) {
        constexpr int T = Pool::kT;
        std::size_t const y_idx  = p.node_child_at(x_idx, i);
        bool const        y_leaf = p.node_leaf(y_idx);
        std::size_t const z_idx  = p.new_node(y_leaf);   // index-stabil → kein Re-Fetch noetig
        p.set_node_n(z_idx, T - 1);
        for (int j = 0; j < T - 1; ++j) {
            p.set_node_key_at(z_idx, j,   p.node_key_at(y_idx, j + T));
            p.set_node_value_at(z_idx, j, p.node_value_at(y_idx, j + T));
        }
        if (!y_leaf) for (int j = 0; j < T; ++j) p.set_node_child_at(z_idx, j, p.node_child_at(y_idx, j + T));
        typename Pool::key_type   const med_k = p.node_key_at(y_idx, T - 1);
        typename Pool::value_type const med_v = p.node_value_at(y_idx, T - 1);
        p.set_node_n(y_idx, T - 1);
        int const xn = p.node_n(x_idx);
        for (int j = xn; j >= i + 1; --j)     p.set_node_child_at(x_idx, j + 1, p.node_child_at(x_idx, j));
        p.set_node_child_at(x_idx, i + 1, z_idx);
        for (int j = xn - 1; j >= i; --j) {
            p.set_node_key_at(x_idx, j + 1,   p.node_key_at(x_idx, j));
            p.set_node_value_at(x_idx, j + 1, p.node_value_at(x_idx, j));
        }
        p.set_node_key_at(x_idx, i, med_k);
        p.set_node_value_at(x_idx, i, med_v);
        p.set_node_n(x_idx, xn + 1);
    }

    /// INSERT-NONFULL: x_idx garantiert nicht voll; k existiert hier garantiert noch nicht.
    template <class Pool>
    static void insert_nonfull(Pool& p, std::size_t x_idx, typename Pool::key_type k, typename Pool::value_type v) {
        constexpr int MAXK = Pool::kMaxKeys;
        for (;;) {
            int const xn = p.node_n(x_idx);
            if (p.node_leaf(x_idx)) {
                int i = xn - 1;
                while (i >= 0 && k < p.node_key_at(x_idx, i)) {
                    p.set_node_key_at(x_idx, i + 1,   p.node_key_at(x_idx, i));
                    p.set_node_value_at(x_idx, i + 1, p.node_value_at(x_idx, i));
                    --i;
                }
                p.set_node_key_at(x_idx, i + 1, k);
                p.set_node_value_at(x_idx, i + 1, v);
                p.set_node_n(x_idx, xn + 1);
                return;
            }
            int i = xn - 1;
            while (i >= 0 && k < p.node_key_at(x_idx, i)) --i;
            ++i;  // Kind-Index zum Absteigen
            if (p.node_n(p.node_child_at(x_idx, i)) == MAXK) {
                split_child(p, x_idx, i);
                if (k > p.node_key_at(x_idx, i)) ++i;   // Median bestimmt Richtung neu
            }
            x_idx = p.node_child_at(x_idx, i);   // iterativ absteigen
        }
    }

    // ---- CLRS DELETE-Helfer (Kap. 18.3) ----
    template <class Pool>
    static void remove_from(Pool& p, std::size_t x_idx, typename Pool::key_type k) {
        constexpr int T = Pool::kT;
        int const idx = find_key(p, x_idx, k);
        int const xn  = p.node_n(x_idx);
        if (idx < xn && p.node_key_at(x_idx, idx) == k) {
            if (p.node_leaf(x_idx)) {
                for (int i = idx + 1; i < xn; ++i) {
                    p.set_node_key_at(x_idx, i - 1,   p.node_key_at(x_idx, i));
                    p.set_node_value_at(x_idx, i - 1, p.node_value_at(x_idx, i));
                }
                p.set_node_n(x_idx, xn - 1);
            } else {
                remove_from_nonleaf(p, x_idx, idx);
            }
        } else {
            if (p.node_leaf(x_idx)) return;   // nicht vorhanden (durch contains() ausgeschlossen)
            bool const last = (idx == xn);
            if (p.node_n(p.node_child_at(x_idx, idx)) < T) fill(p, x_idx, idx);
            int const xn2 = p.node_n(x_idx);   // fill kann gemergt haben → x.n neu lesen
            if (last && idx > xn2) remove_from(p, p.node_child_at(x_idx, idx - 1), k);
            else                   remove_from(p, p.node_child_at(x_idx, idx), k);
        }
    }

    template <class Pool>
    static void remove_from_nonleaf(Pool& p, std::size_t x_idx, int idx) {
        constexpr int T = Pool::kT;
        typename Pool::key_type const k = p.node_key_at(x_idx, idx);
        if (p.node_n(p.node_child_at(x_idx, idx)) >= T) {
            // Vorgaenger (groesster Schluessel im linken Teilbaum)
            std::size_t cur = p.node_child_at(x_idx, idx);
            while (!p.node_leaf(cur)) cur = p.node_child_at(cur, p.node_n(cur));
            int const cn = p.node_n(cur);
            typename Pool::key_type   const pk = p.node_key_at(cur, cn - 1);
            typename Pool::value_type const pv = p.node_value_at(cur, cn - 1);
            p.set_node_key_at(x_idx, idx, pk);
            p.set_node_value_at(x_idx, idx, pv);
            remove_from(p, p.node_child_at(x_idx, idx), pk);
        } else if (p.node_n(p.node_child_at(x_idx, idx + 1)) >= T) {
            // Nachfolger (kleinster Schluessel im rechten Teilbaum)
            std::size_t cur = p.node_child_at(x_idx, idx + 1);
            while (!p.node_leaf(cur)) cur = p.node_child_at(cur, 0);
            typename Pool::key_type   const sk = p.node_key_at(cur, 0);
            typename Pool::value_type const sv = p.node_value_at(cur, 0);
            p.set_node_key_at(x_idx, idx, sk);
            p.set_node_value_at(x_idx, idx, sv);
            remove_from(p, p.node_child_at(x_idx, idx + 1), sk);
        } else {
            merge(p, x_idx, idx);
            remove_from(p, p.node_child_at(x_idx, idx), k);
        }
    }

    /// Stellt sicher, dass C[idx] vor dem Abstieg >= t Schluessel hat (borrow oder merge).
    template <class Pool>
    static void fill(Pool& p, std::size_t x_idx, int idx) {
        constexpr int T = Pool::kT;
        int const xn = p.node_n(x_idx);
        if (idx != 0 && p.node_n(p.node_child_at(x_idx, idx - 1)) >= T)        borrow_from_prev(p, x_idx, idx);
        else if (idx != xn && p.node_n(p.node_child_at(x_idx, idx + 1)) >= T)  borrow_from_next(p, x_idx, idx);
        else {
            if (idx != xn) merge(p, x_idx, idx);
            else           merge(p, x_idx, idx - 1);
        }
    }

    template <class Pool>
    static void borrow_from_prev(Pool& p, std::size_t x_idx, int idx) {
        std::size_t const child = p.node_child_at(x_idx, idx);
        std::size_t const sib   = p.node_child_at(x_idx, idx - 1);
        int const cn = p.node_n(child);
        int const sn = p.node_n(sib);
        for (int i = cn - 1; i >= 0; --i) {
            p.set_node_key_at(child, i + 1,   p.node_key_at(child, i));
            p.set_node_value_at(child, i + 1, p.node_value_at(child, i));
        }
        if (!p.node_leaf(child)) for (int i = cn; i >= 0; --i) p.set_node_child_at(child, i + 1, p.node_child_at(child, i));
        p.set_node_key_at(child, 0,   p.node_key_at(x_idx, idx - 1));
        p.set_node_value_at(child, 0, p.node_value_at(x_idx, idx - 1));
        if (!p.node_leaf(child)) p.set_node_child_at(child, 0, p.node_child_at(sib, sn));
        p.set_node_key_at(x_idx, idx - 1,   p.node_key_at(sib, sn - 1));
        p.set_node_value_at(x_idx, idx - 1, p.node_value_at(sib, sn - 1));
        p.set_node_n(child, cn + 1);
        p.set_node_n(sib, sn - 1);
    }

    template <class Pool>
    static void borrow_from_next(Pool& p, std::size_t x_idx, int idx) {
        std::size_t const child = p.node_child_at(x_idx, idx);
        std::size_t const sib   = p.node_child_at(x_idx, idx + 1);
        int const cn = p.node_n(child);
        int const sn = p.node_n(sib);
        p.set_node_key_at(child, cn,   p.node_key_at(x_idx, idx));
        p.set_node_value_at(child, cn, p.node_value_at(x_idx, idx));
        if (!p.node_leaf(child)) p.set_node_child_at(child, cn + 1, p.node_child_at(sib, 0));
        p.set_node_key_at(x_idx, idx,   p.node_key_at(sib, 0));
        p.set_node_value_at(x_idx, idx, p.node_value_at(sib, 0));
        for (int i = 1; i < sn; ++i) {
            p.set_node_key_at(sib, i - 1,   p.node_key_at(sib, i));
            p.set_node_value_at(sib, i - 1, p.node_value_at(sib, i));
        }
        if (!p.node_leaf(sib)) for (int i = 1; i <= sn; ++i) p.set_node_child_at(sib, i - 1, p.node_child_at(sib, i));
        p.set_node_n(child, cn + 1);
        p.set_node_n(sib, sn - 1);
    }

    /// Merge C[idx+1] in C[idx], x.key[idx] wandert mit hinunter. C[idx+1] wird freigegeben.
    template <class Pool>
    static void merge(Pool& p, std::size_t x_idx, int idx) {
        constexpr int T = Pool::kT;
        std::size_t const sib_idx = p.node_child_at(x_idx, idx + 1);
        std::size_t const child   = p.node_child_at(x_idx, idx);
        int const sn = p.node_n(sib_idx);
        int const xn = p.node_n(x_idx);
        p.set_node_key_at(child, T - 1,   p.node_key_at(x_idx, idx));
        p.set_node_value_at(child, T - 1, p.node_value_at(x_idx, idx));
        for (int i = 0; i < sn; ++i) {
            p.set_node_key_at(child, i + T,   p.node_key_at(sib_idx, i));
            p.set_node_value_at(child, i + T, p.node_value_at(sib_idx, i));
        }
        if (!p.node_leaf(child)) for (int i = 0; i <= sn; ++i) p.set_node_child_at(child, i + T, p.node_child_at(sib_idx, i));
        for (int i = idx + 1; i < xn; ++i) {
            p.set_node_key_at(x_idx, i - 1,   p.node_key_at(x_idx, i));
            p.set_node_value_at(x_idx, i - 1, p.node_value_at(x_idx, i));
        }
        for (int i = idx + 2; i <= xn; ++i) p.set_node_child_at(x_idx, i - 1, p.node_child_at(x_idx, i));
        int const cn = p.node_n(child);
        p.set_node_n(child, cn + sn + 1);
        p.set_node_n(x_idx, xn - 1);
        p.free_node(sib_idx);
    }

    // ---- Oeffentliche Organ-API (Pflicht-Trio) ----
    template <class Pool>
    static void insert_into(Pool& p, typename Pool::key_type k, typename Pool::value_type v) {
        constexpr int MAXK = Pool::kMaxKeys;
        std::size_t const NIL = Pool::kNil;
        if (p.root() == NIL) {
            std::size_t const r = p.new_node(/*leaf=*/true);
            p.set_node_key_at(r, 0, k);
            p.set_node_value_at(r, 0, v);
            p.set_node_n(r, 1);
            p.set_root(r);
            p.inc_size();
            return;
        }
        if (update_existing(p, k, v)) return;   // std::map-Update: keine neue Schluesselzahl
        if (p.node_n(p.root()) == MAXK) {        // volle Wurzel splitten (Hoehe waechst nur hier)
            std::size_t const s = p.new_node(/*leaf=*/false);
            p.set_node_child_at(s, 0, p.root());
            p.set_root(s);
            split_child(p, s, 0);
        }
        insert_nonfull(p, p.root(), k, v);
        p.inc_size();
    }

    template <class Pool>
    static std::optional<typename Pool::value_type> lookup_in(Pool const& p, typename Pool::key_type k) {
        std::size_t const NIL = Pool::kNil;
        std::size_t cur = p.root();
        while (cur != NIL) {
            int const n = p.node_n(cur);
            int i = 0;
            while (i < n && k > p.node_key_at(cur, i)) ++i;
            if (i < n && p.node_key_at(cur, i) == k) return p.node_value_at(cur, i);
            cur = p.node_leaf(cur) ? NIL : p.node_child_at(cur, i);
        }
        return std::nullopt;
    }

    template <class Pool>
    static bool erase_from(Pool& p, typename Pool::key_type k) {
        std::size_t const NIL = Pool::kNil;
        if (p.root() == NIL || !contains(p, k)) return false;
        remove_from(p, p.root(), k);
        if (p.node_n(p.root()) == 0) {   // Wurzel auf 0 Schluessel geschrumpft → Hoehe verkleinern
            std::size_t const old = p.root();
            p.set_root(p.node_leaf(old) ? NIL : p.node_child_at(old, 0));
            p.free_node(old);
        }
        p.dec_size();
        return true;
    }
};

// Selbstbeweis: BTreeTraversalOrgan erfuellt das BTreeTraversal-Concept ueber dem Pilot-Pool.
static_assert(BTreeTraversal<BTreeTraversalOrgan, BTreeNodePoolStore>);

}  // namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable
