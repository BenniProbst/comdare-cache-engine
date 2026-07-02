#pragma once
// #188-4a (2026-07-02) -- ComposedEytzingerSearch<Traversal, Pool>.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Such-Algorithmus = Eytzinger-Traversal-Organ plus EytzingerLayoutStore. Die sortierte Basis ist authoritativ;
// das BFS-Layout ist abgeleiteter Zustand im selben Store und wird lazy durch lookup aufgebaut.

#include "eytzinger_layout_pool_concept.hpp"
#include "eytzinger_layout_store.hpp"
#include "eytzinger_traversal_organ.hpp"

#include <cstddef>
#include <optional>
#include <utility>

namespace comdare::cache_engine::lookup::composable {

template <class Traversal, class Pool>
    requires EytzingerTraversalOrganConcept<Traversal, Pool>
class ComposedEytzingerSearch {
public:
    using key_type   = typename Pool::key_type;
    using value_type = typename Pool::value_type;

    void insert(key_type k, value_type v) { Traversal::template insert_into<Pool>(pool_, k, v); }
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        return Traversal::template lookup_in<Pool>(pool_, k);
    }
    bool                      erase(key_type k) { return Traversal::template erase_from<Pool>(pool_, k); }
    [[nodiscard]] std::size_t occupied_count() const noexcept { return pool_.slot_count(); }

    /// #188-4a - besucht JEDEN gespeicherten Record GENAU EINMAL als sink(key, value).
    /// Reihenfolge hier: aufsteigende Key-Reihenfolge (sortierter Primaerzustand), NICHT vertraglich.
    /// Reines Lesen: KEIN Rebuild-Trigger, KEIN Statistik-Effekt. Rueckgabe = Anzahl besuchter Records.
    template <class Sink>
    std::size_t for_each_record(Sink&& sink) const {
        std::size_t const n = pool_.slot_count();
        for (std::size_t i = 0; i < n; ++i) sink(pool_.key_at(i), pool_.value_at(i));
        return n;
    }

    void                      clear() noexcept { pool_.clear(); }
    [[nodiscard]] Pool const& pool() const noexcept { return pool_; }

private:
    Pool pool_{};
};

static_assert(EytzingerTraversalOrganConcept<EytzingerTraversalOrgan, EytzingerLayoutStore>);

} // namespace comdare::cache_engine::lookup::composable
