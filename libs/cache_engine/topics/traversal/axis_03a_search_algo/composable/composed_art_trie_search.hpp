#pragma once
// V41 Umstufung-A s4 (Task #43) — ComposedArtTrieSearch<Traversal, Pool>: Such-Algorithmus = ART-Descent ⊕ ArtTrieNodePool.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Pendant zu ComposedTreeSearch/ComposedBTreeSearch, aber ueber einem adaptiven Byte-Trie-Pool (Leis ICDE 2013).
// Identische std::map-Schnittstelle (insert/lookup/erase/occupied_count/clear) ueber GEMEINSAMEM uint64-Key.
// Eigene 17-Zeilen-Schale (ComposedTreeSearch ist NICHT wiederverwendbar — anderes Pool-Concept; occupied_count
// via size()). So wird der Effekt der ADAPTIVEN Knoten + Path-Compression am einheitlichen Interface messbar (F15).

#include "art_trie_node_pool_concept.hpp"
#include "art_trie_traversal_organ.hpp"

#include <cstddef>
#include <optional>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable {

/// KOMPOSITION: ein ART-basierter Such-Algorithmus = ART-Trie-Traversal-Organ ⊕ ArtTrieNodePool, mit std::map-
/// Interface. Genetisches Experiment: Descent-Organ frei austauschbar bei gleichem Pool (Doku 14 §1.2).
template <class Traversal, class Pool>
    requires ArtTrieTraversal<Traversal, Pool>
class ComposedArtTrieSearch {
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
