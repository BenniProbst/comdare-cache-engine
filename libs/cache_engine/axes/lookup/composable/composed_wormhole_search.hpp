#pragma once
// V41 Umstufung-A s4 (Task #43) — ComposedWormholeSearch<Traversal, Pool>: Such-Algorithmus = Hash-Jump ⊕ WormholeLeafListPool.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Pendant zu ComposedHotPatriciaSearch/ComposedBTreeSearch, aber ueber einem Wormhole-Hybrid (sortierte
// Leaf-Liste + geordneter Anchor-Jump-Index; Wu/Ni/Jiang EuroSys 2019). Identische std::map-Schnittstelle
// ueber GEMEINSAMEM uint64-Key. So wird der Effekt des HASH-JUMP (statt Wurzel-Abstieg) am einheitlichen
// Interface messbar (F15). is_original=false ([[pseudocode-papers-fallback]]; wh.c GPL-3.0, KEIN Linking).

#include "wormhole_leaf_list_pool_concept.hpp"
#include "wormhole_jump_traversal_organ.hpp"

#include <cstddef>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

/// KOMPOSITION: ein wormhole-basierter Such-Algorithmus = Hash-Jump-Traversal-Organ ⊕ WormholeLeafListPool,
/// mit std::map-Interface. Genetisches Experiment: Jump-Organ frei austauschbar bei gleichem Pool (Doku 14 §1.2).
template <class Traversal, class Pool>
    requires WormholeTraversal<Traversal, Pool>
class ComposedWormholeSearch {
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
    /// Reihenfolge familien-spezifisch, NICHT vertraglich (Wormhole: doppelt verkettete Leaf-Liste ab root).
    /// Reines Lesen: KEIN Substrat-/Statistik-Effekt. Rueckgabe = Anzahl besuchter Records (== occupied_count()).
    template <class Sink>
    std::size_t for_each_record(Sink&& sink) const {
        std::size_t leaf    = pool_.root();
        std::size_t visited = 0;
        while (leaf != Pool::kNil) {
            int const n = pool_.leaf_n(leaf);
            for (int i = 0; i < n; ++i) {
                sink(pool_.leaf_key_at(leaf, i), pool_.leaf_value_at(leaf, i));
                ++visited;
            }
            leaf = pool_.leaf_next(leaf);
        }
        return visited;
    }
    void                      clear() noexcept { pool_.clear(); }
    [[nodiscard]] Pool const& pool() const noexcept { return pool_; }

private:
    Pool pool_{};
};

} // namespace comdare::cache_engine::lookup::composable
