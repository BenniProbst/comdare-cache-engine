#pragma once
// V41 Umstufung-A (Task #41) — ComposedBTreeSearch<Traversal, Pool>: Such-Algorithmus = B-Tree-Walk ⊕ BTreeNodePool.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Pendant zu ComposedTreeSearch<Traversal, Pool>, aber ueber einem BTreeNodePool (geordnet, balanciert,
// block-orientiert). Identische std::map-Schnittstelle ueber GEMEINSAMEM uint64-Key. ComposedTreeSearch
// ist NICHT wiederverwendbar (BTreeNodePool ist ein ANDERES Concept als TreeNodePool — Mehrwege statt
// left/right; occupied_count via size() statt node_count()), daher eine eigene 17-Zeilen-Kompositions-Schale.

#include "btree_node_pool_concept.hpp"
#include "btree_traversal_organ.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

/// KOMPOSITION: ein b-baum-basierter Such-Algorithmus = B-Tree-Walk-Organ ⊕ BTreeNodePool, mit std::map-
/// Interface. Genetisches Experiment: Walk-Organ frei austauschbar bei gleichem Pool (Doku 14 §1.2).
template <class Traversal, class Pool>
    requires BTreeTraversal<Traversal, Pool>
class ComposedBTreeSearch {
public:
    using key_type   = typename Pool::key_type;
    using value_type = typename Pool::value_type;

    void insert(key_type k, value_type v) { Traversal::template insert_into<Pool>(pool_, k, v); }
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        return Traversal::template lookup_in<Pool>(pool_, k);
    }
    bool                      erase(key_type k) { return Traversal::template erase_from<Pool>(pool_, k); }
    [[nodiscard]] std::size_t occupied_count() const noexcept { return pool_.size(); }
    /// #188-4b-DEG1 - besucht JEDEN gespeicherten Record GENAU EINMAL als sink(key, value).
    /// Reihenfolge familien-spezifisch, NICHT vertraglich (B-Tree: In-Order ueber child/key-Arrays).
    /// Reines Lesen: KEIN Substrat-/Statistik-Effekt. Rueckgabe = Anzahl besuchter Records (== occupied_count()).
    template <class Sink>
    std::size_t for_each_record(Sink&& sink) const {
        struct Frame {
            std::size_t node{};
            int         key_index{};
            bool        record{};
        };
        std::vector<Frame> stack;
        if (pool_.root() != Pool::kNil) stack.push_back(Frame{pool_.root(), 0, false});
        std::size_t visited = 0;
        while (!stack.empty()) {
            Frame const f = stack.back();
            stack.pop_back();
            if (f.record) {
                sink(pool_.node_key_at(f.node, f.key_index), pool_.node_value_at(f.node, f.key_index));
                ++visited;
                continue;
            }
            int const  n    = pool_.node_n(f.node);
            bool const leaf = pool_.node_leaf(f.node);
            if (!leaf) {
                std::size_t const c = pool_.node_child_at(f.node, n);
                if (c != Pool::kNil) stack.push_back(Frame{c, 0, false});
            }
            for (int i = n; i > 0; --i) {
                int const key_i = i - 1;
                stack.push_back(Frame{f.node, key_i, true});
                if (!leaf) {
                    std::size_t const c = pool_.node_child_at(f.node, key_i);
                    if (c != Pool::kNil) stack.push_back(Frame{c, 0, false});
                }
            }
        }
        return visited;
    }
    void                      clear() noexcept { pool_.clear(); }
    [[nodiscard]] Pool const& pool() const noexcept { return pool_; }

private:
    Pool pool_{};
};

} // namespace comdare::cache_engine::lookup::composable
