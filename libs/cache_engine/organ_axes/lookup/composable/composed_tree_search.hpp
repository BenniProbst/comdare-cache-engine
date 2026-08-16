#pragma once
// V41 Roadmap-2 INC-2b — ComposedTreeSearch<Traversal, Pool>: Such-Algorithmus = Tree-Traversal ⊕ TreeNodePool.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Pendant zu ComposedSearch<Traversal, Store> (flacher Store), aber ueber einem index-stabilen TreeNodePool.
// Identische std::map-Schnittstelle (insert/lookup/erase/occupied_count/clear) ueber GEMEINSAMEM uint64-Key.
// Laesst ComposedSearch/StorageOrgan/TraversalOrgan voellig unberuehrt (eigenstaendige Tree-Linie).

#include "tree_node_pool_concept.hpp"
#include "tree_traversal_organ.hpp"

#include "axis_bound_scratch.hpp" // A8-S5-01b: Walk-Stack ueber die Allokator-Achse

#include <cstddef>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

/// KOMPOSITION: ein baum-basierter Such-Algorithmus = Tree-Traversal-Organ ⊕ TreeNodePool, mit
/// std::map-Interface. Genetisches Experiment: Traversal frei austauschbar bei gleichem Pool (Doku 14 §1.2).
template <class Traversal, class Pool>
    requires TreeTraversalOrgan<Traversal, Pool>
class ComposedTreeSearch {
public:
    using key_type   = typename Pool::key_type;
    using value_type = typename Pool::value_type;
    /// A8-S5-01b (Form-B-Ausweis MIT realer Verdrahtung): die Allokator-Achsen-Strategie der Schale.
    using allocator_type = composition_allocator_t<Pool>;
    using walk_stack_t   = AxisBoundBuffer<allocator_type, std::size_t>;

    void insert(key_type k, value_type v) { Traversal::template insert_into<Pool>(pool_, k, v); }
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        return Traversal::template lookup_in<Pool>(pool_, k);
    }
    bool                      erase(key_type k) { return Traversal::template erase_from<Pool>(pool_, k); }
    [[nodiscard]] std::size_t occupied_count() const noexcept { return pool_.node_count(); }
    /// #188-4b-DEG1 - besucht JEDEN gespeicherten Record GENAU EINMAL als sink(key, value).
    /// Reihenfolge familien-spezifisch, NICHT vertraglich (BST: In-Order ueber left/right ab root).
    /// Reines Lesen: KEIN Substrat-/Statistik-Effekt. Rueckgabe = Anzahl besuchter Records (== occupied_count()).
    /// A8-S5-01b -- FORM B (ueber die Allokator-ACHSE): die Stack-Tiefe ist die HOEHE des Suchbaums, und der BST ist
    /// unbalanciert -- sie haengt an den Daten (im degenerierten Fall an der Record-Zahl). Eine Compile-Time-Kappe gibt es hier
    /// deshalb NICHT -- Form A waere hier keine staerkere Aussage, sondern eine falsche. Der Stack laeuft
    /// ueber das Achsen-Interface; sein Zaehler ist BEWUSST vom T6-Store-Zaehler getrennt (Doppelzaehlungs-
    /// Regel, Posten 68) und ueber walk_allocator_statistics() nachweisbar -- Form B behauptet hier nicht
    /// nur, sie ist am Objekt belegbar (Form-B-Grenze, s5_family_alloc_conformance.hpp:31).
    template <class Sink>
    std::size_t for_each_record(Sink&& sink) const {
        walk_stack_.clear();
        walk_stack_t& stack   = walk_stack_;
        std::size_t   cur     = pool_.root();
        std::size_t   visited = 0;
        while (cur != Pool::kNil || !stack.empty()) {
            while (cur != Pool::kNil) {
                stack.push_back(cur);
                cur = pool_.left(cur);
            }
            cur = stack.back();
            stack.pop_back();
            sink(pool_.node_key(cur), pool_.node_value(cur));
            ++visited;
            cur = pool_.right(cur);
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
    /// mutable: for_each_record ist eine reine LESE-Operation am Substrat; der Walk-Speicher ist kein
    /// Zustand des Organs, sondern sein Arbeitsband. Praezedenz: die stats_/observer_-Member der
    /// Observable-Huellen sind aus demselben Grund mutable.
    mutable walk_stack_t walk_stack_{};
};

} // namespace comdare::cache_engine::lookup::composable
