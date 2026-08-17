#pragma once
// V41 Umstufung-A (Task #41) — ComposedBTreeSearch<Traversal, Pool>: Such-Algorithmus = B-Tree-Walk ⊕ BTreeNodePool.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Pendant zu ComposedTreeSearch<Traversal, Pool>, aber ueber einem BTreeNodePool (geordnet, balanciert,
// block-orientiert). Identische std::map-Schnittstelle ueber GEMEINSAMEM uint64-Key. ComposedTreeSearch
// ist NICHT wiederverwendbar (BTreeNodePool ist ein ANDERES Concept als TreeNodePool — Mehrwege statt
// left/right; occupied_count via size() statt node_count()), daher eine eigene 17-Zeilen-Kompositions-Schale.

#include "btree_node_pool_concept.hpp"
#include "btree_traversal_organ.hpp"

#include "axis_bound_scratch.hpp" // A8-S5-01b: Walk-Stack ueber die Allokator-Achse

#include <cstddef>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

/// KOMPOSITION: ein b-baum-basierter Such-Algorithmus = B-Tree-Walk-Organ ⊕ BTreeNodePool, mit std::map-
/// Interface. Genetisches Experiment: Walk-Organ frei austauschbar bei gleichem Pool (Doku 14 §1.2).
template <class Traversal, class Pool>
    requires BTreeTraversal<Traversal, Pool>
class ComposedBTreeSearch {
public:
    using key_type   = typename Pool::key_type;
    using value_type = typename Pool::value_type;
    /// A8-S5-01b (Form-B-Ausweis MIT realer Verdrahtung): die Allokator-Achsen-Strategie der Schale.
    using allocator_type = composition_allocator_t<Pool>;
    /// Ein Rahmen ist entweder "Knoten expandieren" oder "Record ausgeben" (record=true).
    struct WalkFrame {
        std::size_t node{};
        int         key_index{};
        bool        record{};
    };
    using walk_stack_t = AxisBoundBuffer<allocator_type, WalkFrame>;

    void insert(key_type k, value_type v) { Traversal::template insert_into<Pool>(pool_, k, v); }
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        return Traversal::template lookup_in<Pool>(pool_, k);
    }
    bool                      erase(key_type k) { return Traversal::template erase_from<Pool>(pool_, k); }
    [[nodiscard]] std::size_t occupied_count() const noexcept { return pool_.size(); }
    // Phase 0.3a: pool-native LEBENDE Knotenzahl (growth-unabhaengig) fuer Shape-Struktur-Beweise.
    [[nodiscard]] std::size_t pool_node_count() const noexcept { return pool_.pool_node_count(); }
    /// #188-4b-DEG1 - besucht JEDEN gespeicherten Record GENAU EINMAL als sink(key, value).
    /// Reihenfolge familien-spezifisch, NICHT vertraglich (B-Tree: In-Order ueber child/key-Arrays).
    /// Reines Lesen: KEIN Substrat-/Statistik-Effekt. Rueckgabe = Anzahl besuchter Records (== occupied_count()).
    /// A8-S5-01b -- FORM B (ueber die Allokator-ACHSE): die Stack-Tiefe ist Baumhoehe MAL Knoten-Aritaet, und die
    /// Hoehe waechst mit der Record-Zahl -- datenabhaengig, nicht compile-time-gedeckelt. Eine Compile-Time-Kappe gibt es hier
    /// deshalb NICHT -- Form A waere hier keine staerkere Aussage, sondern eine falsche. Der Stack laeuft
    /// ueber das Achsen-Interface; sein Zaehler ist BEWUSST vom T6-Store-Zaehler getrennt (Doppelzaehlungs-
    /// Regel, Posten 68) und ueber walk_allocator_statistics() nachweisbar -- Form B behauptet hier nicht
    /// nur, sie ist am Objekt belegbar (Form-B-Grenze, s5_family_alloc_conformance.hpp:31).
    template <class Sink>
    std::size_t for_each_record(Sink&& sink) const {
        using Frame = WalkFrame;
        walk_stack_.clear();
        walk_stack_t& stack = walk_stack_;
        if (pool_.root() != Pool::kNil) stack.push_back(Frame{pool_.root(), 0, false});
        std::size_t visited = 0;
        while (!stack.empty()) {
            Frame const f = stack.back();
            stack.pop_back();
            if (f.record) {
                sink(pool_.node_key_at(f.node, f.key_index), pool_.node_value_at(f.node, f.key_index));
                ++visited;
                continue;
            }
            int const  n    = pool_.node_n(f.node);
            bool const leaf = pool_.node_leaf(f.node);
            if (!leaf) {
                std::size_t const c = pool_.node_child_at(f.node, n);
                if (c != Pool::kNil) stack.push_back(Frame{c, 0, false});
            }
            for (int i = n; i > 0; --i) {
                int const key_i = i - 1;
                stack.push_back(Frame{f.node, key_i, true});
                if (!leaf) {
                    std::size_t const c = pool_.node_child_at(f.node, key_i);
                    if (c != Pool::kNil) stack.push_back(Frame{c, 0, false});
                }
            }
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

    /// A8-S5-01b: die Achsen-Strategie des WALK-Speichers dieses Organs. Getrennter Zaehler, damit die
    /// T6-Store-Statistik unvermischt bleibt (Praezedenz: traversal_allocator_statistics der Scheibe 01d).
#ifdef COMDARE_CE_ENABLE_STATISTICS
    [[nodiscard]] auto walk_allocator_statistics() const noexcept { return walk_stack_.allocator_statistics(); }
#endif

private:
    Pool pool_{};
    /// mutable: s. ComposedTreeSearch -- der Walk-Speicher ist Arbeitsband, kein Organ-Zustand.
    mutable walk_stack_t walk_stack_{};
};

} // namespace comdare::cache_engine::lookup::composable
