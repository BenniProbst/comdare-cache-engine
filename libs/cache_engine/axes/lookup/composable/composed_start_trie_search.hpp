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

#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>

namespace comdare::cache_engine::lookup::composable {

/// KOMPOSITION: ein START-basierter Such-Algorithmus = Multibyte-Span-Traversal-Organ ⊕ StartTrieNodePool,
/// mit std::map-Interface. Genetisches Experiment: Descent-Organ frei austauschbar bei gleichem Pool (Doku 14 §1.2).
template <class Traversal, class Pool>
    requires StartTrieTraversal<Traversal, Pool>
class ComposedStartTrieSearch {
public:
    using key_type   = typename Pool::key_type;
    using value_type = typename Pool::value_type;
    /// A8-S5-01b: CT-Kappe des Abstiegs-Stacks, aus dem SCHLUESSEL-TYP abgeleitet (nie ein Literal) --
    /// mindestens ein Schluessel-Byte je Inner-Knoten (span >= 1), plus ein Rahmen Reserve.
    static constexpr std::size_t kMaxDescent = sizeof(key_type) + 1U;
    /// A8-S5-01b -- Form-B-Ausweis des ORGANS: der Speicher dieses Organs IST der seines Substrats, und das
    /// Substrat laeuft real ueber die Allokator-Achse (Scheibe 01a; am Objekt belegt durch
    /// probe_organ_wiring in test_s5_01a_pool_stores_alloc_conformance). Das Organ selbst haelt seit dieser
    /// Scheibe KEINEN eigenen Heap mehr -- sein Walk-Stack ist inline mit CT-Kappe (Form A oben). Der Ausweis
    /// ist damit keine blosse using-Zeile neben einem Default-Allokator-Container, sondern die Weitergabe der
    /// EINEN Speicher-Wahl dieser Komposition.
    using allocator_type = typename Pool::allocator_type;

    void insert(key_type k, value_type v) { Traversal::template insert_into<Pool>(pool_, k, v); }
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        return Traversal::template lookup_in<Pool>(pool_, k);
    }
    bool                      erase(key_type k) { return Traversal::template erase_from<Pool>(pool_, k); }
    [[nodiscard]] std::size_t occupied_count() const noexcept { return pool_.size(); }
    /// #188-4b-DEG1 - besucht JEDEN gespeicherten Record GENAU EINMAL als sink(key, value).
    /// Reihenfolge familien-spezifisch, NICHT vertraglich (START: sparse sortierte Diskriminator-Kindliste).
    /// Reines Lesen: KEIN Substrat-/Statistik-Effekt. Rueckgabe = Anzahl besuchter Records (== occupied_count()).
    /// A8-S5-01b -- FORM A (heap-frei): der Abstiegs-Stack liegt als INLINE-Array im Rahmen, ohne jede
    /// Allokation. Das geht hier, weil die Trie-Tiefe COMPILE-TIME beschraenkt ist: jeder Inner-Knoten
    /// verbraucht mindestens EIN Schluessel-Byte (span >= 1; groessere Spans verbrauchen MEHR, nie weniger),
    /// also hoechstens sizeof(key_type) Inner-Ebenen. kMaxDescent leitet die Kappe aus dem TYP ab, nie aus
    /// einem Literal -- besonders wichtig hier, wo die Kindliste je Knoten bis 256^span Eintraege haelt und
    /// die alte "alle Kinder auf den Stack"-Form gar nicht deckelbar gewesen waere. Besuchsreihenfolge
    /// unveraendert (Tiefensuche, Diskriminatoren aufsteigend), belegt vom std::map-Orakel in test_188_4bb0.
    template <class Sink>
    std::size_t for_each_record(Sink&& sink) const {
        struct Frame {
            std::size_t node{};
            std::size_t next_child{};
        };
        std::array<Frame, kMaxDescent> stack{};
        std::size_t                    top     = 0;
        std::size_t                    visited = 0;

        std::size_t const root = pool_.root();
        if (root == Pool::kNil) return 0;
        if (pool_.is_leaf(root)) {
            sink(pool_.leaf_key(root), pool_.leaf_value(root));
            return 1;
        }
        stack[top++] = Frame{root, 0};
        while (top > 0) {
            Frame& f = stack[top - 1];
            if (f.next_child >= pool_.child_count(f.node)) {
                --top;
                continue;
            }
            std::size_t const child = pool_.child_at(f.node, f.next_child);
            ++f.next_child;
            if (child == Pool::kNil) continue;
            if (pool_.is_leaf(child)) {
                sink(pool_.leaf_key(child), pool_.leaf_value(child));
                ++visited;
                continue;
            }
            // Ueberlauf == verletzte Span-Invariante (Inner-Ebene ohne Byte-Verbrauch). Struktur kaputt ->
            // laut abbrechen statt still weniger Records zu ernten.
            if (top >= kMaxDescent)
                throw std::logic_error("ComposedStartTrieSearch: Trie tiefer als sizeof(key_type) Inner-Ebenen");
            stack[top++] = Frame{child, 0};
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
