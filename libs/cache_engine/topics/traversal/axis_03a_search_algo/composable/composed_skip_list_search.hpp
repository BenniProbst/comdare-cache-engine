#pragma once
// V41 Umstufung-A (Task #41) — ComposedSkipListSearch<Traversal, Pool>: Such-Algorithmus = Skip-Walk ⊕ SkipListNodePool.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Pendant zu ComposedTreeSearch<Traversal, Pool>, aber ueber einem SkipListNodePool (geordnet, probabilistisch).
// Identische std::map-Schnittstelle (insert/lookup/erase/occupied_count/clear) ueber GEMEINSAMEM uint64-Key.
// ComposedTreeSearch ist NICHT wiederverwendbar (anderes Pool-Concept; `node_count()` statt `live_count()`),
// daher eine eigene 17-Zeilen-Kompositions-Schale. Laesst alle Bestands-Kompositionen/-Concepts unberuehrt.

#include "skip_list_node_pool_concept.hpp"
#include "skip_list_traversal_organ.hpp"

#include <cstddef>
#include <optional>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable {

/// KOMPOSITION: ein skip-listen-basierter Such-Algorithmus = Skip-Walk-Organ ⊕ SkipListNodePool, mit
/// std::map-Interface. Genetisches Experiment: Walk-Organ frei austauschbar bei gleichem Pool (Doku 14 §1.2).
template <class Traversal, class Pool>
    requires SkipListTraversal<Traversal, Pool>
class ComposedSkipListSearch {
public:
    using key_type   = typename Pool::key_type;
    using value_type = typename Pool::value_type;

    void insert(key_type k, value_type v)                            { Traversal::template insert_into<Pool>(pool_, k, v); }
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const { return Traversal::template lookup_in<Pool>(pool_, k); }
    bool erase(key_type k)                                           { return Traversal::template erase_from<Pool>(pool_, k); }
    [[nodiscard]] std::size_t occupied_count()        const noexcept { return pool_.live_count(); }
    void clear()                                            noexcept { pool_.clear(); }
    [[nodiscard]] Pool const& pool()                  const noexcept { return pool_; }

private:
    Pool pool_{};
};

}  // namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable
