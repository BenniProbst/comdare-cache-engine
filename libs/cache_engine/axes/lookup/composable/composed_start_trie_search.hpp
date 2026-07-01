#pragma once
// V41 Umstufung-A s4 (Task #43) — ComposedStartTrieSearch<Traversal, Pool>: START = Multibyte-Span-Descent ⊕ StartTrieNodePool.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Pendant zu ComposedArtTrieSearch, aber ueber einem Multibyte-Span-Radix (START, Fent et al. ICDEW 2020).
// Identische std::map-Schnittstelle ueber GEMEINSAMEM uint64-Key — so wird der Effekt des MULTIBYTE-SPAN
// (statt ARTs fixem 1-Byte) am einheitlichen Interface messbar (F15). is_original=false
// ([[pseudocode-papers-fallback]]; volle Quelle fehlt, MIT-Upstream nicht einkopiert -> Re-Impl).

#include "start_trie_node_pool_concept.hpp"
#include "start_trie_traversal_organ.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

/// KOMPOSITION: ein START-basierter Such-Algorithmus = Multibyte-Span-Traversal-Organ ⊕ StartTrieNodePool,
/// mit std::map-Interface. Genetisches Experiment: Descent-Organ frei austauschbar bei gleichem Pool (Doku 14 §1.2).
template <class Traversal, class Pool>
    requires StartTrieTraversal<Traversal, Pool>
class ComposedStartTrieSearch {
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
    /// Reihenfolge familien-spezifisch, NICHT vertraglich (START: sparse sortierte Diskriminator-Kindliste).
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
            for (std::size_t i = pool_.child_count(ref); i > 0; --i) {
                std::size_t const child = pool_.child_at(ref, i - 1);
                if (child != Pool::kNil) stack.push_back(child);
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
