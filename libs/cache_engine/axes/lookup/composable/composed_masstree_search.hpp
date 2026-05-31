#pragma once
// V41 (Task #42-Folge) — ComposedMasstreeSearch<Traversal, Pool>: Masstree = B+Baum-of-Tries-Descent ⊕ MasstreeLayerNodePool.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier) @paper P03 Masstree (Mao/Kohler/Morris EuroSys 2012)
//
// Pendant zu ComposedStartTrieSearch/ComposedArtTrieSearch: identische std::map-Schnittstelle ueber gemeinsamem
// uint64-Key. Descent-Organ (Slice-Layer + kpermuter) frei austauschbar bei gleichem Pool (Doku 14 §1.2).
// is_original=false ([[pseudocode-papers-fallback]]).

#include "masstree_layer_pool_concept.hpp"
#include "masstree_layer_traversal_organ.hpp"

#include <cstddef>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

template <class Traversal, class Pool>
    requires MasstreeLayerTraversal<Traversal, Pool>
class ComposedMasstreeSearch {
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

}  // namespace comdare::cache_engine::lookup::composable
