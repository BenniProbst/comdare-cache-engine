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
    void                      clear() noexcept { pool_.clear(); }
    [[nodiscard]] Pool const& pool() const noexcept { return pool_; }

private:
    Pool pool_{};
};

} // namespace comdare::cache_engine::lookup::composable
