#pragma once
// V41 Umstufung-A (Task #41) — ComposedHashSearch<Traversal, Pool>: Such-Algorithmus = Hash-Probe ⊕ HashBucketPool.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Pendant zu ComposedTreeSearch<Traversal, Pool>, aber ueber einem HashBucketPool (ungeordnet, Open-Addressing).
// Identische std::map-Schnittstelle (insert/lookup/erase/occupied_count/clear) ueber GEMEINSAMEM uint64-Key.
// ComposedTreeSearch ist NICHT wiederverwendbar (es `requires TreeTraversalOrgan` + ruft `pool_.node_count()`;
// der Hash-Pool hat `occupied()`, kein `node_count()`), daher eine eigene 17-Zeilen-Kompositions-Schale.
// Laesst ComposedSearch/ComposedTreeSearch + ihre Concepts/static_asserts voellig unberuehrt.

#include "hash_bucket_pool_concept.hpp"
#include "hash_probe_traversal_organ.hpp"

#include <cstddef>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

/// KOMPOSITION: ein hash-basierter Such-Algorithmus = Hash-Probe-Organ ⊕ HashBucketPool, mit std::map-
/// Interface. Genetisches Experiment: Probe-Organ frei austauschbar bei gleichem Pool (Doku 14 §1.2).
template <class Traversal, class Pool>
    requires HashProbeTraversal<Traversal, Pool>
class ComposedHashSearch {
public:
    using key_type   = typename Pool::key_type;
    using value_type = typename Pool::value_type;

    void insert(key_type k, value_type v) { Traversal::template insert_into<Pool>(pool_, k, v); }
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        return Traversal::template lookup_in<Pool>(pool_, k);
    }
    bool                      erase(key_type k) { return Traversal::template erase_from<Pool>(pool_, k); }
    [[nodiscard]] std::size_t occupied_count() const noexcept { return pool_.occupied(); }
    /// #188-4b-DEG1 - besucht JEDEN gespeicherten Record GENAU EINMAL als sink(key, value).
    /// Reihenfolge familien-spezifisch, NICHT vertraglich (OA: Bucket-Scan; Chaining: Bucket-Loop plus
    /// Ketten-Walk in aktueller Link-Reihenfolge). Tombstones/Empty werden uebersprungen, Chaining-Nodes exakt
    /// einmal besucht. Reines Lesen: KEIN Substrat-/Statistik-Effekt. Rueckgabe = Anzahl besuchter Records.
    template <class Sink>
    std::size_t for_each_record(Sink&& sink) const {
        std::size_t visited = 0;
        if constexpr (Pool::kOpenAddressing) {
            for (std::size_t i = 0; i < pool_.bucket_count(); ++i) {
                if (!pool_.slot_is_occupied(i)) continue;
                sink(pool_.slot_key(i), pool_.slot_value(i));
                ++visited;
            }
        } else {
            for (std::size_t bucket = 0; bucket < pool_.bucket_count(); ++bucket) {
                for (std::size_t node = pool_.chain_head(bucket); node != Pool::kNil; node = pool_.node_next(node)) {
                    sink(pool_.slot_key(node), pool_.slot_value(node));
                    ++visited;
                }
            }
        }
        return visited;
    }
    void                      clear() noexcept { pool_.clear(); }
    [[nodiscard]] Pool const& pool() const noexcept { return pool_; }

private:
    Pool pool_{};
};

} // namespace comdare::cache_engine::lookup::composable
