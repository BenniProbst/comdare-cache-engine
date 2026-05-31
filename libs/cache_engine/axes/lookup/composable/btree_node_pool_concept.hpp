#pragma once
// V41 Umstufung-A (Task #41) — BTreeNodePool-Concept.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// **Warum ein EIGENES Concept (nicht TreeNodePool):** Ein B-Baum hat MEHRWEGE-Knoten (n Schluessel +
// bis 2t Kinder), KEINE binaeren left/right-Zeiger. Die balancierenden Transformations-Organe (Split/
// Merge/Borrow) sind Such-/Algorithmus-Logik und gehoeren ins Traversal-Organ, NICHT ins Substrat. Daher
// ein SEPARATES Pool-Concept — TreeNodePool + seine static_asserts bleiben unberuehrt.
//
// Substrat OHNE Such-Logik: der Pool verwaltet nur Mehrwege-Knoten-Slots (Schluessel/Werte/Kinder + n +
// leaf-Flag), die logische Schlusselzahl (size, via inc_size/dec_size durch das Organ) sowie Allokation
// (new_node/free_node mit Free-List). Split/Merge/Borrow leben im BTreeTraversalOrgan. Geist gewahrt:
// gemeinsamer uint64-Key; KEIN noexcept-Zwang (new_node darf via vector werfen, [[allocation-failure-exception]]).

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::lookup::composable {

/// B-TREE-NODE-POOL-Concept: balancierter Mehrwege-Knoten-Pool (Bayer/McCreight / CLRS Kap. 18) ueber
/// uint64-Key. Erfuellt von BTreeNodePoolStore; konsumiert von BTreeTraversalOrgan.
template <class S>
concept BTreeNodePool =
    requires {
        typename S::key_type;
        typename S::value_type;
        { S::kNil }         -> std::convertible_to<std::size_t>;
        { S::kT }           -> std::convertible_to<int>;   // Minimum-Degree
        { S::kMaxKeys }     -> std::convertible_to<int>;   // 2t-1
        { S::kMaxChildren } -> std::convertible_to<int>;   // 2t
    }
    && std::same_as<typename S::key_type, std::uint64_t>
    && std::same_as<typename S::value_type, std::uint64_t>
    && requires(S& s, S const& cs, std::size_t i, int j, bool b,
                typename S::key_type k, typename S::value_type v) {
        // (A) const Inspektion — PFLICHT
        { cs.root() }                 -> std::convertible_to<std::size_t>;
        { cs.size() }                 -> std::convertible_to<std::size_t>;   // logische Schluesselzahl
        { cs.node_n(i) }              -> std::convertible_to<int>;            // belegte Schluessel im Knoten
        { cs.node_leaf(i) }           -> std::same_as<bool>;
        { cs.node_key_at(i, j) }      -> std::same_as<typename S::key_type>;
        { cs.node_value_at(i, j) }    -> std::same_as<typename S::value_type>;
        { cs.node_child_at(i, j) }    -> std::convertible_to<std::size_t>;
        // (B) Mutation — PFLICHT
        { s.new_node(b) }             -> std::convertible_to<std::size_t>;    // alle child=kNil; darf werfen
        { s.free_node(i) }            -> std::same_as<void>;
        { s.set_root(i) }             -> std::same_as<void>;
        { s.set_node_n(i, j) }        -> std::same_as<void>;
        { s.set_node_leaf(i, b) }     -> std::same_as<void>;
        { s.set_node_key_at(i, j, k) }   -> std::same_as<void>;
        { s.set_node_value_at(i, j, v) } -> std::same_as<void>;
        { s.set_node_child_at(i, j, i) } -> std::same_as<void>;   // (node, slot, target)
        { s.inc_size() }              -> std::same_as<void>;       // +1 logischer Schluessel (Organ-Insert)
        { s.dec_size() }              -> std::same_as<void>;       // -1 logischer Schluessel (Organ-Erase)
        { s.clear() }                 -> std::same_as<void>;
    };

}  // namespace comdare::cache_engine::lookup::composable
