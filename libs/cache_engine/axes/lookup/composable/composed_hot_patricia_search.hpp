#pragma once
// V41 Umstufung-A s4 (Task #43) — ComposedHotPatriciaSearch<Traversal, Pool>: Such-Algorithmus = crit-bit-Descent ⊕ HotPatriciaNodePool.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Pendant zu ComposedArtTrieSearch, aber ueber einem binary crit-bit-Patricia-Pool (HOT-Korrektheits-Basis,
// Binna et al. SIGMOD 2018). Identische std::map-Schnittstelle ueber GEMEINSAMEM uint64-Key. So wird der Effekt
// der BIT-level Discrimination (distinct von ARTs byte-level) am einheitlichen Interface messbar (F15).

#include "hot_patricia_node_pool_concept.hpp"
#include "hot_patricia_traversal_organ.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

/// KOMPOSITION: ein HOT-basierter Such-Algorithmus = crit-bit-Patricia-Traversal-Organ ⊕ HotPatriciaNodePool,
/// mit std::map-Interface. Genetisches Experiment: Descent-Organ frei austauschbar bei gleichem Pool (Doku 14 §1.2).
template <class Traversal, class Pool>
    requires HotPatriciaTraversal<Traversal, Pool>
class ComposedHotPatriciaSearch {
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
    /// Reihenfolge familien-spezifisch, NICHT vertraglich (HOT: crit-bit-Kinder 0/1 ab root).
    /// Reines Lesen: KEIN Substrat-/Statistik-Effekt. Rueckgabe = Anzahl besuchter Records (== occupied_count()).
    template <class Sink>
    std::size_t for_each_record(Sink&& sink) const {
        std::vector<std::size_t> stack;
        if (pool_.root() != Pool::kNil) stack.push_back(pool_.root());
        std::size_t visited = 0;
        while (!stack.empty()) {
            std::size_t const ref = stack.back();
            stack.pop_back();
            if (pool_.is_leaf(ref)) {
                sink(pool_.leaf_key(ref), pool_.leaf_value(ref));
                ++visited;
                continue;
            }
            std::size_t const c1 = pool_.child(ref, 1);
            if (c1 != Pool::kNil) stack.push_back(c1);
            std::size_t const c0 = pool_.child(ref, 0);
            if (c0 != Pool::kNil) stack.push_back(c0);
        }
        return visited;
    }
    void                      clear() noexcept { pool_.clear(); }
    [[nodiscard]] Pool const& pool() const noexcept { return pool_; }

    template <class P = Pool>
    [[nodiscard]] auto store_allocator_statistics() const noexcept
        requires requires(P const& p) { p.store_allocator_statistics(); }
    {
        return pool_.store_allocator_statistics();
    }

private:
    Pool pool_{};
};

} // namespace comdare::cache_engine::lookup::composable
