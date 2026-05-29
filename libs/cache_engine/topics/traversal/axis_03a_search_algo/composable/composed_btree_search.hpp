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

namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable {

/// KOMPOSITION: ein b-baum-basierter Such-Algorithmus = B-Tree-Walk-Organ ⊕ BTreeNodePool, mit std::map-
/// Interface. Genetisches Experiment: Walk-Organ frei austauschbar bei gleichem Pool (Doku 14 §1.2).
template <class Traversal, class Pool>
    requires BTreeTraversal<Traversal, Pool>
class ComposedBTreeSearch {
public:
    using key_type   = typename Pool::key_type;
    using value_type = typename Pool::value_type;

    void insert(key_type k, value_type v)                            { Traversal::template insert_into<Pool>(pool_, k, v); }
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const { return Traversal::template lookup_in<Pool>(pool_, k); }
    bool erase(key_type k)                                           { return Traversal::template erase_from<Pool>(pool_, k); }
    [[nodiscard]] std::size_t occupied_count()        const noexcept { return pool_.size(); }
    void clear()                                            noexcept { pool_.clear(); }
    [[nodiscard]] Pool const& pool()                  const noexcept { return pool_; }

private:
    Pool pool_{};
};

}  // namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable
