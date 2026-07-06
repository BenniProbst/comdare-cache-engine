#pragma once
// V41 Umstufung-A s4 (Task #43) — ComposedSurfMapSearch<Traversal, Pool>: SuRF-Map-Schale = exaktes K->V.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Pendant zu ComposedWormholeSearch/ComposedBTreeSearch, aber die AUTORITATIVE exakte K->V-Schale des SuRF-
// Tiers (Zhang/Lim/Andersen SIGMOD 2018). Identische std::map-Schnittstelle ueber GEMEINSAMEM uint64-Key —
// so ist SuRF am einheitlichen Interface vergleichbar (das echte LOUDS-may-contain-Filter-Organ in axis_filter
// ist die zweite, approximative Haelfte der Sezierung). is_original=false ([[pseudocode-papers-fallback]]).

#include "surf_fst_map_pool_concept.hpp"
#include "surf_map_traversal_organ.hpp"

#include <cstddef>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

/// KOMPOSITION: die exakte SuRF-Map-Schale = SurfMapTraversal-Organ ⊕ SurfFstMapPool, mit std::map-Interface.
template <class Traversal, class Pool>
    requires SurfMapTraversal<Traversal, Pool>
class ComposedSurfMapSearch {
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
    /// Reihenfolge familien-spezifisch, NICHT vertraglich (SuRF-Map: sortierte exakte K->V-Vektoren).
    /// Reines Lesen: KEIN Substrat-/Statistik-Effekt. Rueckgabe = Anzahl besuchter Records (== occupied_count()).
    template <class Sink>
    std::size_t for_each_record(Sink&& sink) const {
        std::size_t const n = pool_.size();
        for (std::size_t i = 0; i < n; ++i) sink(pool_.key_at(i), pool_.value_at(i));
        return n;
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
