#pragma once
// V41 (Task #42-Folge) — MasstreeLayerNodePool-Concept (Masstree-Organ, letzter Platzhalter-Konfigurator).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier) @paper P03 Masstree (Mao/Kohler/Morris EuroSys 2012)
//
// **Warum ein EIGENES Concept (nicht BTreeNodePool, nicht ArtTrieNodePool):** Masstree ist ein B+BAUM-OF-TRIES
// mit zwei strukturellen Distinktionen, die kein Bestands-Pool abbildet: (1) kpermuter-Knoten (kpermuter.hh) —
// die Slots bleiben PHYSISCH an fester Position, ein 64-Bit-Permutationswort traegt die SORTIERTE Reihenfolge;
// Insert verschiebt nur Nibbles im Permutationswort, KEIN physischer Array-Shift wie BTreeNodePool. (2) Layer-
// Pointer-Union (masstree_struct.hh): ein Leaf-Slot haelt ENTWEDER einen Wert (keylenx==8) ODER einen Sub-Layer-
// B+Baum (keylenx==128, layer-Pointer). Daher ein SEPARATES Pool-Concept; alle Bestands-Concepts unberuehrt.
//
// Substrat OHNE Such-Logik: verwaltet Leaf-Knoten (kpermuter-Slots: slice/keylenx/value-oder-layer, B-link
// next/prev) + Internode (B+Baum-Branch: sortierte slices + child-Pointer) + logische Schluesselzahl. Der
// span-parametrisierte Slice-Descent + ksearch + make_new_layer + Split lebt im MasstreeLayerTraversalOrgan.
// Gemeinsamer uint64-Key; KEIN noexcept-Zwang (Allokation darf werfen, [[allocation-failure-exception]]).

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::lookup::composable {

/// MASSTREE-LAYER-NODE-POOL-Concept: B+Baum-of-Tries-Knoten-Pool (Mao/Kohler/Morris EuroSys 2012) ueber uint64-Key.
/// Erfuellt von MasstreeLayerNodePoolStore; konsumiert von MasstreeLayerTraversalOrgan.
template <class S>
concept MasstreeLayerNodePool =
    requires {
        typename S::key_type;
        typename S::value_type;
        { S::kNil } -> std::convertible_to<std::size_t>;
    }
    && std::same_as<typename S::key_type, std::uint64_t>
    && std::same_as<typename S::value_type, std::uint64_t>
    && requires(S& s, S const& cs, std::size_t ref, int slot, int logical,
                std::uint64_t sl, std::uint8_t kl, typename S::value_type v) {
        // (A) Wurzel + Groesse
        { cs.root() }   -> std::convertible_to<std::size_t>;
        { cs.size() }   -> std::convertible_to<std::size_t>;     // logische Schluesselzahl
        { s.set_root(ref) } -> std::same_as<void>;
        { s.inc_size() }    -> std::same_as<void>;
        { s.dec_size() }    -> std::same_as<void>;
        { s.clear() }       -> std::same_as<void>;
        { cs.is_leaf(ref) } -> std::same_as<bool>;
        { s.free_node(ref) }-> std::same_as<void>;
        // (B) Leaf — kpermuter-Slots (DISTINKTION: Permutationswort statt physischem Array-Shift)
        { s.new_leaf() }                     -> std::convertible_to<std::size_t>;   // leerer Leaf; darf werfen
        { cs.leaf_size(ref) }                -> std::convertible_to<int>;           // aus Permutation (perm & 15)
        { cs.leaf_perm_at(ref, logical) }    -> std::convertible_to<int>;           // logical -> physischer Slot (perm[i])
        { s.leaf_alloc_slot(ref) }           -> std::convertible_to<int>;           // peek back() (naechster freier phys)
        { s.leaf_perm_insert(ref, logical) } -> std::convertible_to<int>;           // insert_from_back(logical) -> phys
        { s.leaf_perm_remove(ref, logical) } -> std::same_as<void>;                 // remove(logical) (Slot recycelt)
        { s.leaf_set_sorted_size(ref, slot) } -> std::same_as<void>;                // perm = make_sorted(n) (Split-Rebuild)
        { cs.leaf_slice_at(ref, slot) }      -> std::convertible_to<std::uint64_t>;
        { s.leaf_set_slice_at(ref, slot, sl) } -> std::same_as<void>;
        { cs.leaf_keylenx_at(ref, slot) }    -> std::convertible_to<int>;           // 8 = Inline-Wert, 128 = Layer
        { s.leaf_set_keylenx_at(ref, slot, kl) } -> std::same_as<void>;
        { cs.leaf_value_at(ref, slot) }      -> std::convertible_to<typename S::value_type>;
        { s.leaf_set_value_at(ref, slot, v) }-> std::same_as<void>;
        { cs.leaf_layer_at(ref, slot) }      -> std::convertible_to<std::size_t>;   // Sub-Layer-Wurzel (keylenx==128)
        { s.leaf_set_layer_at(ref, slot, ref) } -> std::same_as<void>;
        { cs.leaf_next(ref) }                -> std::convertible_to<std::size_t>;   // B-link
        { cs.leaf_prev(ref) }                -> std::convertible_to<std::size_t>;
        { s.leaf_set_next(ref, ref) }        -> std::same_as<void>;
        { s.leaf_set_prev(ref, ref) }        -> std::same_as<void>;
        // (C) Internode — B+Baum-Branch (n Slices, n+1 Kinder)
        { s.new_internode() }                -> std::convertible_to<std::size_t>;   // darf werfen
        { cs.inode_n(ref) }                  -> std::convertible_to<int>;
        { s.inode_set_n(ref, slot) }         -> std::same_as<void>;
        { cs.inode_slice_at(ref, slot) }     -> std::convertible_to<std::uint64_t>;
        { s.inode_set_slice_at(ref, slot, sl) } -> std::same_as<void>;
        { cs.inode_child_at(ref, slot) }     -> std::convertible_to<std::size_t>;
        { s.inode_set_child_at(ref, slot, ref) } -> std::same_as<void>;
    };

}  // namespace comdare::cache_engine::lookup::composable
