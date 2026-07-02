#pragma once
// V41 Roadmap-2 INC-2b — TreeTraversalOrgan-Concept + BSTTraversalOrgan.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Analog zum flachen TraversalOrgan, aber ueber einem TreeNodePool (index-stabile Knoten mit Kind-Zeigern):
// statische insert_into/lookup_in/erase_from, KEIN Eigenspeicher. BSTTraversalOrgan = unbalancierter
// binaerer Suchbaum mit Hibbard-Deletion (3 Faelle), portiert aus axis_03a_search_algo_bst.hpp, aber als
// austauschbares ORGAN ueber dem generischen uint64-Pool (statt monolithischem uint16-Wrapper).

#include "tree_node_pool_concept.hpp"
#include "tree_node_pool_store.hpp" // fuer den Selbstbeweis am Dateiende

#include <concepts>
#include <cstddef>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

/// TREE-TRAVERSAL-Organ-Concept: statische insert_into/lookup_in/erase_from auf einem TreeNodePool.
template <class T, class Pool>
concept TreeTraversalOrgan =
    TreeNodePool<Pool> && requires(Pool& p, Pool const& cp, typename Pool::key_type k, typename Pool::value_type v) {
        { T::template insert_into<Pool>(p, k, v) } -> std::same_as<void>;
        { T::template lookup_in<Pool>(cp, k) } -> std::same_as<std::optional<typename Pool::value_type>>;
        { T::template erase_from<Pool>(p, k) } -> std::same_as<bool>;
    };

/// Tree-Traversal-Organ 1: unbalancierter BST mit Hibbard-Deletion. Navigiert ueber Pool-Kind-Zeiger.
struct BSTTraversalOrgan {
    template <class Pool>
    static void insert_into(Pool& p, typename Pool::key_type k, typename Pool::value_type v) {
        std::size_t const NIL    = Pool::kNil;
        std::size_t       parent = NIL, cur = p.root();
        bool              is_left = false;
        while (cur != NIL) {
            typename Pool::key_type const ck = p.node_key(cur);
            if (k == ck) {
                p.set_node_value(cur, v);
                return;
            } // Update
            parent = cur;
            if (k < ck) {
                cur     = p.left(cur);
                is_left = true;
            } else {
                cur     = p.right(cur);
                is_left = false;
            }
        }
        std::size_t const nn = p.allocate_node(k, v);
        if (parent == NIL)
            p.set_root(nn);
        else if (is_left)
            p.set_left(parent, nn);
        else
            p.set_right(parent, nn);
    }

    template <class Pool>
    static std::optional<typename Pool::value_type> lookup_in(Pool const& p, typename Pool::key_type k) {
        std::size_t const NIL = Pool::kNil;
        std::size_t       cur = p.root();
        while (cur != NIL) {
            typename Pool::key_type const ck = p.node_key(cur);
            if (k == ck) return p.node_value(cur);
            cur = (k < ck) ? p.left(cur) : p.right(cur);
        }
        return std::nullopt;
    }

    template <class Pool>
    static bool erase_from(Pool& p, typename Pool::key_type k) {
        std::size_t const NIL    = Pool::kNil;
        std::size_t       parent = NIL, cur = p.root();
        bool              is_left = false;
        while (cur != NIL && p.node_key(cur) != k) {
            parent = cur;
            if (k < p.node_key(cur)) {
                cur     = p.left(cur);
                is_left = true;
            } else {
                cur     = p.right(cur);
                is_left = false;
            }
        }
        if (cur == NIL) return false; // nicht gefunden

        // Zwei Kinder: Inorder-Nachfolger (linkster im rechten Teilbaum) nach cur kopieren, dann succ loeschen.
        if (p.left(cur) != NIL && p.right(cur) != NIL) {
            std::size_t succ_parent = cur;
            std::size_t succ        = p.right(cur);
            while (p.left(succ) != NIL) {
                succ_parent = succ;
                succ        = p.left(succ);
            }
            p.set_node_key(cur, p.node_key(succ));
            p.set_node_value(cur, p.node_value(succ));
            cur     = succ; // jetzt succ loeschen (max. ein rechtes Kind)
            parent  = succ_parent;
            is_left = (p.left(succ_parent) == succ);
        }

        // cur hat hoechstens ein Kind.
        std::size_t const child = (p.left(cur) != NIL) ? p.left(cur) : p.right(cur); // evtl. NIL
        if (parent == NIL)
            p.set_root(child);
        else if (is_left)
            p.set_left(parent, child);
        else
            p.set_right(parent, child);
        p.free_node(cur);
        return true;
    }
};

// Selbstbeweis: BSTTraversalOrgan erfuellt das TreeTraversalOrgan-Concept ueber dem Pilot-Pool.
static_assert(TreeTraversalOrgan<BSTTraversalOrgan, TreeNodePoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
