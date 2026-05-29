#pragma once
// V41 Roadmap-2 INC-2b — ComposedTreeSearch<Traversal, Pool>: Such-Algorithmus = Tree-Traversal ⊕ TreeNodePool.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Pendant zu ComposedSearch<Traversal, Store> (flacher Store), aber ueber einem index-stabilen TreeNodePool.
// Identische std::map-Schnittstelle (insert/lookup/erase/occupied_count/clear) ueber GEMEINSAMEM uint64-Key.
// Laesst ComposedSearch/StorageOrgan/TraversalOrgan voellig unberuehrt (eigenstaendige Tree-Linie).

#include "tree_node_pool_concept.hpp"
#include "tree_traversal_organ.hpp"

#include <cstddef>
#include <optional>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable {

/// KOMPOSITION: ein baum-basierter Such-Algorithmus = Tree-Traversal-Organ ⊕ TreeNodePool, mit
/// std::map-Interface. Genetisches Experiment: Traversal frei austauschbar bei gleichem Pool (Doku 14 §1.2).
template <class Traversal, class Pool>
    requires TreeTraversalOrgan<Traversal, Pool>
class ComposedTreeSearch {
public:
    using key_type   = typename Pool::key_type;
    using value_type = typename Pool::value_type;

    void insert(key_type k, value_type v)                          { Traversal::template insert_into<Pool>(pool_, k, v); }
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const { return Traversal::template lookup_in<Pool>(pool_, k); }
    bool erase(key_type k)                                         { return Traversal::template erase_from<Pool>(pool_, k); }
    [[nodiscard]] std::size_t occupied_count()      const noexcept { return pool_.node_count(); }
    void clear()                                          noexcept { pool_.clear(); }
    [[nodiscard]] Pool const& pool()                const noexcept { return pool_; }

private:
    Pool pool_{};
};

}  // namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable
